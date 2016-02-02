//#include "module.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "auxmods.h"
#include "lrotable.h"
#include "c_stdlib.h"
#include "c_string.h"
#include "user_interface.h"

typedef struct lmdns_request
{
	u16_t	id;
	u16_t	flags;
	u16_t	qd_count;
	u16_t	an_count;
	u16_t	ns_count;
	u16_t	ar_count;
	u8_t	payload[];
} lmdns_request, *plmdns_request;

#define SWAPBYTES_U16(a) ((((a) >> 8) & 0xFF) | (((a) & 0xFF) << 8))

static int ICACHE_FLASH_ATTR bonjour_create_response_v4(lua_State* L)
{
	// parameters:
	//		1 - bonjour
	//		2 - string name as requested
	//		3 - ip string
	size_t length;
	if (!lua_isstring( L, 2 ))
		return luaL_error( L, "wrong parameter type (mdns name)" );
	if (!lua_isstring( L, 3 ))
		return luaL_error( L, "wrong parameter type (ip)" );

	char *name = (char *)luaL_checklstring(L, 2, &length) ;

	// create header
	char buffer[256] ;
	lmdns_request *request = (lmdns_request *)buffer ;
	request->id = SWAPBYTES_U16(0) ;
	request->flags = SWAPBYTES_U16(0x8400) ;
	request->qd_count = SWAPBYTES_U16(0) ;
	request->an_count = SWAPBYTES_U16(1) ;
	request->ns_count = SWAPBYTES_U16(0) ;
	request->ar_count = SWAPBYTES_U16(0) ;
	// add the name
	u16_t namePartIndex = 0 ;
	u16_t namePartPosition = 0 ;
	u16_t namePosition = 0 ;
	while (name[namePosition])
	{
		if (name[namePosition] == '.')
		{
			request->payload[namePartIndex] = namePartPosition ;
			namePartIndex += namePartPosition + 1 ;
			namePartPosition = 0 ;
			namePosition++ ;
		} else
		{
			request->payload[namePartIndex+namePartPosition+1] = name[namePosition] ;
			namePosition++ ;
			namePartPosition++ ;
		}
	}
	request->payload[namePartIndex] = namePartPosition ;
	request->payload[namePartIndex+namePartPosition+1] = 0 ;
	// add type: A (host address)
	request->payload[namePartIndex+namePartPosition+2] = 0 ;
	request->payload[namePartIndex+namePartPosition+3] = 1 ;
	// add class: 1 (IN)
	request->payload[namePartIndex+namePartPosition+4] = 0 ;
	request->payload[namePartIndex+namePartPosition+5] = 1 ;
	// add ttl: 64
	request->payload[namePartIndex+namePartPosition+6] = 0 ;
	request->payload[namePartIndex+namePartPosition+7] = 0 ;
	request->payload[namePartIndex+namePartPosition+8] = 0 ;
	request->payload[namePartIndex+namePartPosition+9] = 64 ;
	// add IP v4 length
	request->payload[namePartIndex+namePartPosition+10] = 0 ;
	request->payload[namePartIndex+namePartPosition+11] = 4 ;
	// add IP bytes
	uint32_t ip = ipaddr_addr((char *)luaL_checklstring(L, 3, &length));
	request->payload[namePartIndex+namePartPosition+15] = (ip >> 24) & 0xFF ;
	request->payload[namePartIndex+namePartPosition+14] = (ip >> 16) & 0xFF ;
	request->payload[namePartIndex+namePartPosition+13] = (ip >> 8) & 0xFF ;
	request->payload[namePartIndex+namePartPosition+12] = (ip >> 0) & 0xFF ;

	lua_pushlstring(L, &buffer[0], sizeof(lmdns_request) + namePartIndex + namePartPosition + 16) ;
	return 1 ;
}

static int ICACHE_FLASH_ATTR bonjour_push_request_to_lua(lua_State* L, char *name, uint16_t requestType, uint16_t classType)
{
	lua_createtable(L, 0, 3) ;
	lua_pushstring(L, "name") ;
	lua_pushstring(L, name) ;
	lua_settable(L, -3) ;
	lua_pushstring(L, "type") ;
	lua_pushnumber(L, requestType) ;
	lua_settable(L, -3) ;
	lua_pushstring(L, "class") ;
	lua_pushnumber(L, classType) ;
	lua_settable(L, -3) ;
}

static uint16_t bonjour_getNameLength(lmdns_request *request, uint16_t requestLength, uint16_t startOffset, uint16_t *highestByteOfName)
{
	// sanitize arguments
	if (!request)
		return 0 ;
	if (startOffset < sizeof(lmdns_request))
		return 0 ;
	if (startOffset >= requestLength)
		return 0 ;
	// init
	uint16_t namePartLengthIndex = startOffset - sizeof(lmdns_request) ;
	if (highestByteOfName)
		*highestByteOfName = startOffset ;
	uint16_t nameLength = 0;
	uint8_t numberOfNameParts = 0 ;
	// iterate through part names
	while ((namePartLengthIndex < (requestLength - sizeof(lmdns_request))) && (request->payload[namePartLengthIndex]))
	{
		if (request->payload[namePartLengthIndex] < 0xC0)	// this is a text chunk
		{
			nameLength += request->payload[namePartLengthIndex] + 1;
			namePartLengthIndex += request->payload[namePartLengthIndex]+1 ;
			if (highestByteOfName)
			{
				if (namePartLengthIndex > *highestByteOfName - sizeof(lmdns_request))
					*highestByteOfName = namePartLengthIndex + sizeof(lmdns_request);
			}
		} else // this is a reference to another name / namepart
		{
			if (namePartLengthIndex + 1 + sizeof(lmdns_request) >= requestLength)
				return 0 ;
			if (highestByteOfName)
			{
				if (namePartLengthIndex+1 > *highestByteOfName - sizeof(lmdns_request))
					*highestByteOfName = namePartLengthIndex + sizeof(lmdns_request) + 1;
			}
			namePartLengthIndex = request->payload[namePartLengthIndex+1] - sizeof(lmdns_request) ;
		}
		// names could possible loop with references, so prevent an infinite loop by limiting the number of parts
		numberOfNameParts++ ;
		if (numberOfNameParts > 128)
			return 0;
	}
	return nameLength ;
}

static int bonjour_getName(lmdns_request *request, uint16_t requestLength, uint16_t startOffset, char *buffer, uint16_t bufferSize)
{
	// sanitize arguments
	if (!request)
		return 0 ;
	if (!buffer)
		return 0 ;
	if (startOffset < sizeof(lmdns_request))
		return 0 ;
	if (startOffset >= requestLength)
		return 0 ;

	uint16_t namePartLengthIndex = startOffset - sizeof(lmdns_request);
	uint16_t namePos = 0 ;
	while ((namePartLengthIndex < (requestLength - sizeof(lmdns_request))) && (request->payload[namePartLengthIndex]))
	{
		if (request->payload[namePartLengthIndex] < 0xC0)	// this is a text chunk
		{
			memcpy(buffer + namePos, &request->payload[namePartLengthIndex + 1], request->payload[namePartLengthIndex]) ;
			namePos += request->payload[namePartLengthIndex] ;
			namePartLengthIndex += request->payload[namePartLengthIndex]+1 ;
			if (request->payload[namePartLengthIndex])
				buffer[namePos++] = '.' ;
			else
				buffer[namePos++] = 0 ;
		} else
		{
			namePartLengthIndex = request->payload[namePartLengthIndex+1] - sizeof(lmdns_request) ;
		}
	}
	return 1 ;
}

static int ICACHE_FLASH_ATTR bonjour_parse_request(lua_State* L)
{
	// get the frame
	size_t length;
	const char *buffer = luaL_checklstring(L, 2, &length);
	// maximum and minimum length we process
	if (length < sizeof(lmdns_request))
	{
		return 0;
	}
	lmdns_request *request = (lmdns_request *)buffer ;
	uint16_t id = SWAPBYTES_U16(request->id) ;
	uint16_t flags = SWAPBYTES_U16(request->flags) ;
	uint16_t qd_count = SWAPBYTES_U16(request->qd_count) ;
	// check id & flags for request
	if ((id) || (flags))
	{
		return 0;
	}
	// create all requests
	uint16_t index = sizeof(lmdns_request) ;
	int i;
	uint16_t farestReadByte = 0 ;
	//prepare returned array
	lua_createtable(L, qd_count, 0) ;
	for (i=0;i<qd_count;i++)
	{
		uint16_t namePartLengthIndex = index ;
		uint16_t nameLength = bonjour_getNameLength(request, length, index, &farestReadByte) ;

		char *name = (char *)c_malloc(nameLength+1) ;
		if (!name)
		{
			return 1 ;
		}

		// alloc memory for name
		memset(name, 0, sizeof(nameLength+1)) ;
		// compose name
		if (nameLength)
		{
			bonjour_getName(request, length, index, name, nameLength) ;
			namePartLengthIndex = farestReadByte-sizeof(lmdns_request) ;
			uint16_t requestType = request->payload[namePartLengthIndex + 1] * 0x100 + request->payload[namePartLengthIndex + 2] ;
			uint16_t requestClass = request->payload[namePartLengthIndex + 3] * 0x100 + request->payload[namePartLengthIndex + 4] ;
			bonjour_push_request_to_lua(
				L,
				name,
				requestType,
				requestClass
			) ;
			lua_rawseti(L, -2, i) ;
		}
		else
		{
			name[0] = 0 ;
		}

		c_free(name) ;
		index = farestReadByte + 1 + 4;
	}

	return 1;
}

#define MIN_OPT_LEVEL   2
#include "lrodefs.h"
const LUA_REG_TYPE bonjour_requesttype_map[] =
{
  { LSTRKEY( "A" ),						LNUMVAL( 1 ) },
  { LSTRKEY( "NS" ),					LNUMVAL( 2 ) },
  { LSTRKEY( "MD" ),					LNUMVAL( 3 ) },
  { LSTRKEY( "MF" ),					LNUMVAL( 4 ) },
  { LSTRKEY( "CNAME" ),					LNUMVAL( 5 ) },
  { LSTRKEY( "SOA" ),					LNUMVAL( 6 ) },
  { LSTRKEY( "MB" ),					LNUMVAL( 7 ) },
  { LSTRKEY( "MG" ),					LNUMVAL( 8 ) },
  { LSTRKEY( "MR" ),					LNUMVAL( 9 ) },
  { LSTRKEY( "NULL" ),					LNUMVAL( 10 ) },
  { LSTRKEY( "WKS" ),					LNUMVAL( 11 ) },
  { LSTRKEY( "PTR" ),					LNUMVAL( 12 ) },
  { LSTRKEY( "HINFO" ),					LNUMVAL( 13 ) },
  { LSTRKEY( "MINFO" ),					LNUMVAL( 14 ) },
  { LSTRKEY( "MX" ),					LNUMVAL( 15 ) },
  { LSTRKEY( "TXT" ),					LNUMVAL( 16 ) },
  { LSTRKEY( "AAAA" ),					LNUMVAL( 28 ) },
  { LSTRKEY( "ANY" ),					LNUMVAL( 255 ) },
  { LNILKEY, LNILVAL}
};

const LUA_REG_TYPE bonjour_map[] =
{
  { LSTRKEY( "create_response_v4" ),	LFUNCVAL( bonjour_create_response_v4)},
  { LSTRKEY( "parse_request" ),			LFUNCVAL( bonjour_parse_request )},
  { LSTRKEY( "TYPE" ),					LROVAL( bonjour_requesttype_map )},
  { LSTRKEY( "PORT" ),					LNUMVAL( 5353 ) },
  { LNILKEY, LNILVAL}
};

int luaopen_bonjour(lua_State *L) {
  return 0;
}
