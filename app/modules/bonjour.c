#include "module.h"
#include "lauxlib.h"
#include "platform.h"
#include "c_stdlib.h"
#include "c_string.h"
#include "c_stdio.h"
#include "user_interface.h"

/** Version (Major) of this user module */
#define BONJOUR_MODULEVERSION_MAJOR            0
/** Version (Minor) of this user module */
#define BONJOUR_MODULEVERSION_MINOR            2

/** Known Request Types */
#define BONJOUR_REQUEST_TYPE_A                 1            /// Request for IP v4 address
#define BONJOUR_REQUEST_TYPE_NS                2
#define BONJOUR_REQUEST_TYPE_MD                3
#define BONJOUR_REQUEST_TYPE_MF                4
#define BONJOUR_REQUEST_TYPE_CNAME             5            /// Request for alias (canonical name)
#define BONJOUR_REQUEST_TYPE_SOA               6
#define BONJOUR_REQUEST_TYPE_MB                7
#define BONJOUR_REQUEST_TYPE_MG                8
#define BONJOUR_REQUEST_TYPE_MR                9
#define BONJOUR_REQUEST_TYPE_NULL              10
#define BONJOUR_REQUEST_TYPE_WKS               11
#define BONJOUR_REQUEST_TYPE_PTR               12
#define BONJOUR_REQUEST_TYPE_HINFO             13
#define BONJOUR_REQUEST_TYPE_MINFO             14
#define BONJOUR_REQUEST_TYPE_MX                15
#define BONJOUR_REQUEST_TYPE_TXT               16
#define BONJOUR_REQUEST_TYPE_RP                17
#define BONJOUR_REQUEST_TYPE_AFSDB             18
#define BONJOUR_REQUEST_TYPE_SIG               24
#define BONJOUR_REQUEST_TYPE_KEY               25
#define BONJOUR_REQUEST_TYPE_AAAA              28           /// Request for IP v6 request
#define BONJOUR_REQUEST_TYPE_SRV               33           /// Request for Service Description
#define BONJOUR_REQUEST_TYPE_ANY               255          /// Request for any known info

#define BONJOUR_REQUEST_TYPE_IN                1
#define BONJOUR_REQUEST_TYPE_CS                2
#define BONJOUR_REQUEST_TYPE_CH                3
#define BONJOUR_REQUEST_TYPE_HS                4
#define BONJOUR_REQUEST_TYPE_QU                0x8000


/** general frame format for mdns / bonjour UDP packets */
typedef struct lmdns_frame
{
    u16_t    id;
    u16_t    flags;
    u16_t    qd_count;
    u16_t    an_count;
    u16_t    ns_count;
    u16_t    ar_count;
    u8_t    payload[];
} lmdns_frame, *plmdns_frame;

/** little helper for endianess */
#define SWAPBYTES_U16(a) ((((a) >> 8) & 0xFF) | (((a) & 0xFF) << 8))

/** Write <character-string> as defined in RFC1035 to the target buffer.
 *  Does not exceed target_max_length
 *  Source is terminated by either \0 or passed termination character.
 *  *end_of_processing will point to the last checked byte of the source (either the termination characters or source+target_buffer_length-1
 *
 *  RFC1035 Part 3.3:
 *    <character-string> is a single
 *    length octet followed by that number of characters.  <character-string>
 *    is treated as binary information, and can be up to 256 characters in
 *    length (including the length octet).
 **/
static int bonjour_write_character_string(char *target, uint8_t target_max_length, const char *source, char terminating_character, char **end_of_processing)
{
    if (!end_of_processing)
        return 0 ;
    *end_of_processing = (char *)source ;
    if (!target || !source)
        return 0 ;
    if (target_max_length == 0)
        return 0 ;
    uint8_t pos = 0 ;
    while ((source[pos] != terminating_character) && (source[pos] != 0))
    {
        if (pos +1 >= target_max_length)
        {
            target[0] = target_max_length-1 ;
            *end_of_processing = (char *)(source + target_max_length-1) ;
            return target_max_length ;
        }
        target[pos+1] = source[pos] ;
        pos++ ;
    }
    target[0] = pos ;
    *end_of_processing = (char *)(source + pos) ;
    return pos+1 ;
}

/** Write <domain-name> as defined in RFC1035 to the target buffer.
 *  Does not exceed target_max_length
 *  Source is terminated by \0.
 *  *end_of_processed will point to the last checked byte of the source (either the termination character or source+target_buffer_length-1
 *
 *  RFC1035 Part 3.3:
 *    <domain-name> is a domain name represented as a series of labels, and
 *    terminated by a label with zero length.
 **/
static int bonjour_write_domain_name(char *target, uint16_t target_max_length, const char *source, char **end_of_processed)
{
    if (!end_of_processed)
        return 0 ;
    char *end_of_last_part = 0 ;
    if (!source || !target)
        return 0 ;
    *end_of_processed = (char *)source ;
    char *current_part = (char *)source ;
    uint16_t complete_length = 0 ;
    do
    {
        int processed_length = bonjour_write_character_string(target, target_max_length, current_part, '.', &end_of_last_part) ;
        target += processed_length ;
        target_max_length -= processed_length ;
        complete_length += processed_length ;
        *end_of_processed = end_of_last_part ;
        current_part = end_of_last_part + 1 ;
    } while (*end_of_last_part != 0) ;
    // terminate the domain string with an empty character string
    int processed_length = bonjour_write_character_string(target, target_max_length, "", '.', &end_of_last_part) ;
    target += processed_length ;
    target_max_length -= processed_length ;
    complete_length += processed_length ;
    *end_of_processed = end_of_last_part ;
    return complete_length ;
}

/** Pushes a LUA table containing all information of a single bonjour request to the LUA stack
 *    table = { name, type, class }
 */
static int bonjour_push_request_to_lua(lua_State* L, char *name, uint16_t requestType, uint16_t classType)
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

/** Pushes a LUA table containing all information of a single bonjour response to the LUA stack
 *    table = { name, type, class, ttl }
 *  The table then needs to be extended with the type specific details
 */
static int bonjour_push_response_to_lua(lua_State* L, char *name, uint16_t requestType, uint16_t classType, u32_t ttl)
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
    lua_pushstring(L, "ttl") ;
    lua_pushnumber(L, ttl) ;
    lua_settable(L, -3) ;
}

/** Returns byte used to encode the character string.
 *  This equals strlen(<character-name>+1) and thus the space required to store the name and a terminating character
 *  If the source buffer is smaller then the indicated character string, the size of the truncated to match character string is returned
 */
static uint16_t bonjour_get_character_string_length(char *source, uint16_t source_length)
{
    if (!source)
        return 0 ;
    if (source_length <= 1)
        return source_length ;
    if (source[0] >= 0xC0)
        return 2 ;
    else if (source[0]+1 < source_length)
        return source[0]+1 ;
    else
        return source_length ;
}

/** Fills target with a null terminated c string copy of the character string
 *  The c string will always be 0 terminated, unless the target buffer has the length of 0.
 *  If the source or target buffer does not have enough place to store the complete character string, the returned string is truncated accordingly
 */
static uint16_t bonjour_get_character_string(char *target, uint16_t target_length, char *source, uint16_t source_length)
{
    if (!target || !source)
        return 0 ;
    if (target_length = 0)
        return 0 ;
    uint16_t requiredLength = bonjour_get_character_string_length(source, source_length) ;
    if (requiredLength <= target_length)
    {
        memcpy(target, source+1, requiredLength-1) ;
        target[requiredLength-1] = 0 ;
        return requiredLength ;
    } else
    {
        memcpy(target, source+1, target_length-1) ;
        target[target_length-1] = 0 ;
        return target_length ;
    }
}

static uint16_t bonjour_get_domain_name_length(lmdns_frame *request, uint16_t requestLength, uint16_t startOffset, uint16_t *highestByteOfName)
{
    // sanitize arguments
    if (!request)
        return 0 ;
    if (startOffset < sizeof(lmdns_frame))
        return 0 ;
    if (startOffset >= requestLength)
        return 0 ;
    // init
    uint16_t namePartLengthIndex = startOffset - sizeof(lmdns_frame) ;
    if (highestByteOfName)
        *highestByteOfName = startOffset ;
    uint16_t nameLength = 0;
    uint8_t numberOfNameParts = 0 ;
    // iterate through part names
    while ((namePartLengthIndex < (requestLength - sizeof(lmdns_frame))) && (request->payload[namePartLengthIndex]))
    {
        if (request->payload[namePartLengthIndex] < 0xC0)    // this is a text chunk
        {
            nameLength += request->payload[namePartLengthIndex] + 1;
            namePartLengthIndex += request->payload[namePartLengthIndex]+1 ;
            if (highestByteOfName)
            {
                if (namePartLengthIndex > *highestByteOfName - sizeof(lmdns_frame))
                    *highestByteOfName = namePartLengthIndex + sizeof(lmdns_frame);
            }
        } else // this is a reference to another name / namepart
        {
            if (namePartLengthIndex + 1 + sizeof(lmdns_frame) >= requestLength)
                return 0 ;
            if (highestByteOfName)
            {
                if (namePartLengthIndex+1 > *highestByteOfName - sizeof(lmdns_frame))
                    *highestByteOfName = namePartLengthIndex + sizeof(lmdns_frame) + 1;
            }
            namePartLengthIndex = request->payload[namePartLengthIndex+1] - sizeof(lmdns_frame) ;
        }
        // names could possible loop with references, so prevent an infinite loop by limiting the number of parts
        numberOfNameParts++ ;
        if (numberOfNameParts > 128)
            return 0;
    }
    return nameLength ;
}

static int bonjour_get_domain_name(lmdns_frame *request, uint16_t requestLength, uint16_t startOffset, char *buffer, uint16_t bufferSize)
{
    // sanitize arguments
    if (!request)
        return 0 ;
    if (!buffer)
        return 0 ;
    if (startOffset < sizeof(lmdns_frame))
        return 0 ;
    if (startOffset >= requestLength)
        return 0 ;

    uint16_t namePartLengthIndex = startOffset - sizeof(lmdns_frame);
    uint16_t namePos = 0 ;
    while ((namePartLengthIndex < (requestLength - sizeof(lmdns_frame))) && (request->payload[namePartLengthIndex]))
    {
        if (request->payload[namePartLengthIndex] < 0xC0)    // this is a text chunk
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
            namePartLengthIndex = request->payload[namePartLengthIndex+1] - sizeof(lmdns_frame) ;
        }
    }
    return 1 ;
}

static int bonjour_parse(lua_State* L)
{
    // get the frame
    size_t length;
    const char *buffer = luaL_checklstring(L, 2, &length);
    // maximum and minimum length we process
    if (length < sizeof(lmdns_frame))
    {
        return 0;
    }
    lmdns_frame *request = (lmdns_frame *)buffer ;
    uint16_t id = SWAPBYTES_U16(request->id) ;
    uint16_t flags = SWAPBYTES_U16(request->flags) ;
    uint16_t qd_count = SWAPBYTES_U16(request->qd_count) ;
    uint16_t an_count = SWAPBYTES_U16(request->an_count) ;
    // check id & flags for request
    if ((id) || ((flags) && (flags!=0x8400)))
    {
        return 0;
    }
    // create all requests
    uint16_t index = sizeof(lmdns_frame) ;
    int i;
    uint16_t farestReadByte = 0 ;
    //prepare returned array
    lua_createtable(L, qd_count, 0) ;
    for (i=0;i<qd_count;i++)
    {
        uint16_t namePartLengthIndex = index ;
        uint16_t nameLength = bonjour_get_domain_name_length(request, length, index, &farestReadByte) ;

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
            bonjour_get_domain_name(request, length, index, name, nameLength) ;
            namePartLengthIndex = farestReadByte-sizeof(lmdns_frame) ;
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

    // after requests: parse responses, if any
    lmdns_frame *response = request ;
    lua_createtable(L, an_count, 0) ;
    for (i=0;i<an_count;i++)
    {
        uint16_t namePartLengthIndex = index ;
        uint16_t nameLength = bonjour_get_domain_name_length(response, length, index, &farestReadByte) ;

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
            bonjour_get_domain_name(response, length, index, name, nameLength) ;
            namePartLengthIndex = farestReadByte-sizeof(lmdns_frame) ;
            uint16_t responseType = response->payload[namePartLengthIndex + 1] * 0x100 + response->payload[namePartLengthIndex + 2] ;
            uint16_t responseClass = response->payload[namePartLengthIndex + 3] * 0x100 + response->payload[namePartLengthIndex + 4] ;
            uint32_t responseTtl =
                response->payload[namePartLengthIndex + 5] * 0x1000000 + response->payload[namePartLengthIndex + 6] * 0x10000 +
                response->payload[namePartLengthIndex + 7] * 0x100 + response->payload[namePartLengthIndex + 8] ;
            uint16_t responseLength = response->payload[namePartLengthIndex + 9] * 0x100 + response->payload[namePartLengthIndex + 10] ;
            bonjour_push_response_to_lua(
                L,
                name,
                responseType,
                responseClass,
                responseTtl
            ) ;
            switch (responseType)
            {
                default:
                    break;
                case BONJOUR_REQUEST_TYPE_A:
                    {
                        char buffer[16] ;
                        c_sprintf(buffer, "%u.%u.%u.%u",
                                                    response->payload[namePartLengthIndex + 11],
                                                    response->payload[namePartLengthIndex + 12],
                                                    response->payload[namePartLengthIndex + 13],
                                                    response->payload[namePartLengthIndex + 14]) ;
                        lua_pushstring(L, "ip") ;
                        lua_pushstring(L, buffer) ;
                        lua_settable(L, -3) ;
                    }
                    break ;
                case BONJOUR_REQUEST_TYPE_CNAME:
                    {
                        uint16_t dummy = 0 ;
                        uint16_t cnameLength = bonjour_get_domain_name_length(response, length, namePartLengthIndex+11, &dummy) ;
                        char *cname = (char *)c_malloc(cnameLength+1) ;
                        bonjour_get_domain_name(response, length, namePartLengthIndex+11, cname, cnameLength) ;
                        lua_pushstring(L, "cname") ;
                        lua_pushstring(L, cname) ;
                        lua_settable(L, -3) ;
                        c_free(cname) ;
                    }
                    break ;
                case BONJOUR_REQUEST_TYPE_SRV:
                    {
                        uint16_t dummy = 0 ;
                        uint16_t hnameLength = bonjour_get_domain_name_length(response, length, namePartLengthIndex+17, &dummy) ;
                        char *hname = (char *)c_malloc(hnameLength+1) ;
                        bonjour_get_domain_name(response, length, namePartLengthIndex+11, hname, hnameLength) ;
                        lua_pushstring(L, "host") ;
                        lua_pushstring(L, hname) ;
                        lua_settable(L, -3) ;
                        c_free(hname) ;
                        uint16_t priority = response->payload[namePartLengthIndex + 11] * 0x100 + response->payload[namePartLengthIndex + 12] ;
                        uint16_t weight = response->payload[namePartLengthIndex + 13] * 0x100 + response->payload[namePartLengthIndex + 14] ;
                        uint16_t port = response->payload[namePartLengthIndex + 15] * 0x100 + response->payload[namePartLengthIndex + 16] ;
                        lua_pushstring(L, "priority") ;
                        lua_pushnumber(L, priority) ;
                        lua_settable(L, -3) ;
                        lua_pushstring(L, "weight") ;
                        lua_pushnumber(L, weight) ;
                        lua_settable(L, -3) ;
                        lua_pushstring(L, "port") ;
                        lua_pushnumber(L, port) ;
                        lua_settable(L, -3) ;
                    }
                    break ;
            }
            farestReadByte += 10 + responseLength;
            lua_rawseti(L, -2, i) ;
        }
        else
        {
            name[0] = 0 ;
        }

        c_free(name) ;
        index = farestReadByte + 1;
    }


    return 2;
}

static int bonjour_create_request(lua_State* L)
{
    // Todo: Sanitize parameters

    lua_pushnil(L) ;
    unsigned long elements = 0 ;
    unsigned long maximum_required_data_size = sizeof(lmdns_frame) ;
    while( lua_next(L, 2))
    {
        // top of stack is now the request struct from lua
        if (lua_istable(L,-1))
        {
            lua_getfield(L,-1,"name") ;
            lua_getfield(L,-2,"type") ;
            lua_getfield(L,-3,"class") ;
            size_t name_length;
            const char *name_buffer = luaL_checklstring(L, -3, &name_length);
            uint16_t request_type = lua_tonumber(L,-2) ;
            uint16_t request_class = lua_tonumber(L,-1) ;
            // the required data is now on the stack
            maximum_required_data_size += name_length+2 ; // add size of uncompressed name string
            maximum_required_data_size += 4 ; // add size of class and type data
            lua_pop(L,3) ;
        }
        lua_pop(L, 1) ;        // pop value
        elements++ ;
    }

    // the number of elements is now known
    // the maximum size required for the requests is now known

    // ToDo: use compression and thus actual_used_data_size <= maximum_required_data_size
    unsigned long actual_used_data_size = maximum_required_data_size ;
    lmdns_frame *frame = (lmdns_frame *)c_malloc(maximum_required_data_size) ;
    memset(frame, 0, maximum_required_data_size) ;

    // create header
    frame->id = 0 ;
    frame->flags = 0 ;
    frame->qd_count = SWAPBYTES_U16(elements) ;
    frame->an_count = SWAPBYTES_U16(0) ;
    frame->ns_count = SWAPBYTES_U16(0) ;
    frame->ar_count = SWAPBYTES_U16(0) ;

    // iterate again and set the name, type and class
    lua_pushnil(L) ;
    unsigned long cur_payload_pos = 0 ;
    while( lua_next(L, 2))
    {
        // top of stack is now the request struct from lua
        if (lua_istable(L,-1))
        {
            lua_getfield(L,-1,"name") ;
            lua_getfield(L,-2,"type") ;
            lua_getfield(L,-3,"class") ;
            size_t name_length;
            const char *name = luaL_checklstring(L, -3, &name_length);
            uint16_t request_type = lua_tonumber(L,-2) ;
            uint16_t request_class = lua_tonumber(L,-1) ;
            // add the data to the payload


            u16_t namePartIndex = cur_payload_pos ;
            u16_t namePartPosition = 0 ;
            u16_t namePosition = 0 ;

            while (name[namePosition])
            {
                if (name[namePosition] == '.')
                {
                    frame->payload[namePartIndex] = namePartPosition ;
                    namePartIndex += namePartPosition + 1 ;
                    namePartPosition = 0 ;
                    namePosition++ ;
                } else
                {
                    frame->payload[namePartIndex+namePartPosition+1] = name[namePosition] ;
                    namePosition++ ;
                    namePartPosition++ ;
                }
            }
            frame->payload[namePartIndex] = namePartPosition ;
            frame->payload[namePartIndex+namePartPosition+1] = 0 ;
            // fill in class and type
            frame->payload[namePartIndex+namePartPosition+2] = request_type >> 8 ;
            frame->payload[namePartIndex+namePartPosition+3] = request_type & 0xFF ;
            frame->payload[namePartIndex+namePartPosition+4] = request_class >> 8;
            frame->payload[namePartIndex+namePartPosition+5] = request_class & 0xFF ;
            cur_payload_pos = namePartIndex+namePartPosition+2+4 ;
            actual_used_data_size = cur_payload_pos + sizeof(lmdns_frame);
            lua_pop(L,3) ;
        } else
        {
            // not a table, throw error?
        }
        lua_pop(L, 1) ;        // pop value
    }

    lua_pushlstring (L, (char *)frame, actual_used_data_size);
    c_free(frame) ;
    return 1 ;
}

static int bonjour_create_response(lua_State* L)
{
    // Sanitize parameters
    if (!lua_istable(L,2))
        return 0 ;

    // iterate through table entries
    lua_pushnil(L) ;
    unsigned long elements = 0 ;
    unsigned long maximum_required_data_size = sizeof(lmdns_frame) ;
    while( lua_next(L, 2))
    {
        // top of stack is now the request struct from lua
        if (lua_istable(L,-1))
        {
            lua_getfield(L,-1,"name") ;
            size_t name_length;
            const char *name_buffer = luaL_checklstring(L, -1, &name_length);
            lua_getfield(L,-2,"type") ;
            uint16_t response_type = lua_tonumber(L, -1) ;

            // the required data is now on the stack
            maximum_required_data_size += name_length+2 ; // add size of uncompressed name string
            switch (response_type)
            {
                default:
                    maximum_required_data_size += 10 ; // add size of class, type, ttl and payloadlength data
                    break ;
                case BONJOUR_REQUEST_TYPE_A:
                    maximum_required_data_size += 10 ; // add size of class, type, ttl and payloadlength data
                    maximum_required_data_size += 4 ; // ip data
                    break ;
                case BONJOUR_REQUEST_TYPE_CNAME:
                    lua_getfield(L,-3,"cname") ;
                    {
                        int length = 0 ;
                        const char * cname = (const char *)luaL_checklstring(L, -1, &length);
                        maximum_required_data_size += 10 ; // add size of class, type, ttl and payloadlength data
                        maximum_required_data_size += length+2 ; // domain name length
                    }
                    lua_pop(L,1) ;
                    break ;
                case BONJOUR_REQUEST_TYPE_SRV:
                    lua_getfield(L,-3,"host") ;
                    {
                        int length = 0 ;
                        const char * hostname = (const char *)luaL_checklstring(L, -1, &length);
                        maximum_required_data_size += 10 ; // add size of class, type, ttl and payloadlength data
                        maximum_required_data_size += length+2 + 6; // host name length + priority + weight + port
                    }
                    lua_pop(L,1) ;
                    break ;
            }
            lua_pop(L,2) ;
            elements++ ;
        }
        lua_pop(L, 1) ;        // pop value
    }

    // the number of elements is now known
    // the maximum size required for the requests is now known

    // ToDo: use compression and thus actual_used_data_size <= maximum_required_data_size
    unsigned long actual_used_data_size = maximum_required_data_size ;
    lmdns_frame *frame = (lmdns_frame *)c_malloc(maximum_required_data_size) ;
    memset(frame, 0, maximum_required_data_size) ;

    // create header
    frame->id = 0 ;
    frame->flags = SWAPBYTES_U16(0x8400) ;
    frame->qd_count = SWAPBYTES_U16(0) ;
    frame->an_count = SWAPBYTES_U16(elements) ;
    frame->ns_count = SWAPBYTES_U16(0) ;
    frame->ar_count = SWAPBYTES_U16(0) ;

    // iterate again and set the name, type and class
    lua_pushnil(L) ;
    unsigned long cur_payload_pos = 0 ;
    while( lua_next(L, 2))
    {
        // top of stack is now the request struct from lua
        if (lua_istable(L,-1))
        {
            lua_getfield(L,-1,"name") ;
            lua_getfield(L,-2,"class") ;
            lua_getfield(L,-3,"type") ;
            lua_getfield(L,-4,"ttl") ;
            size_t name_length;
            uint16_t request_class ;
            uint16_t request_type ;
            uint16_t request_ttl ;
            // Todo: validate name
            const char *name = luaL_checklstring(L, -4, &name_length);
            if (lua_isnumber(L,-3))
                request_class = lua_tonumber(L,-3) ;
            if (lua_isnumber(L,-2))
                request_type = lua_tonumber(L,-2) ;
            if (lua_isnumber(L,-1))
                request_ttl = lua_tonumber(L,-1) ;
            // add the data to the payload

            char *end_of_name = 0 ;
            uint16_t domain_name_length = bonjour_write_domain_name(&frame->payload[cur_payload_pos],
                maximum_required_data_size - (cur_payload_pos +sizeof(lmdns_frame)),
                name, &end_of_name) ;

            cur_payload_pos += domain_name_length ;
            // fill in class and type
            frame->payload[cur_payload_pos+0] = request_type >> 8 ;
            frame->payload[cur_payload_pos+1] = request_type & 0xFF ;
            frame->payload[cur_payload_pos+2] = request_class >> 8;
            frame->payload[cur_payload_pos+3] = request_class & 0xFF ;
            frame->payload[cur_payload_pos+4] = (request_ttl >> 24) & 0xFF;
            frame->payload[cur_payload_pos+5] = (request_ttl >> 16) & 0xFF;
            frame->payload[cur_payload_pos+6] = (request_ttl >> 8) & 0xFF;
            frame->payload[cur_payload_pos+7] = (request_ttl >> 0) & 0xFF;
            cur_payload_pos += 8 ;
            switch (request_type)
            {
                default:
                    frame->payload[cur_payload_pos+0] = 0;
                    frame->payload[cur_payload_pos+1] = 0;
                    cur_payload_pos += 2 ;
                    break ;
                case BONJOUR_REQUEST_TYPE_A:
                    frame->payload[cur_payload_pos+0] = 0;
                    frame->payload[cur_payload_pos+1] = 4;
                    lua_getfield(L,-5,"ip") ;
                    {
                        int length = 0 ;
                        uint32_t ip = ipaddr_addr((char *)luaL_checklstring(L, -1, &length));
                        frame->payload[cur_payload_pos+2] = (ip >> 0) & 0xFF ;
                        frame->payload[cur_payload_pos+3] = (ip >> 8) & 0xFF ;
                        frame->payload[cur_payload_pos+4] = (ip >> 16) & 0xFF ;
                        frame->payload[cur_payload_pos+5] = (ip >> 24) & 0xFF ;
                    }
                    lua_pop(L,1) ;
                    cur_payload_pos += 6 ;
                    break ;
                case BONJOUR_REQUEST_TYPE_CNAME:
                    lua_getfield(L,-5,"cname") ;
                    {
                        int length = 0 ;
                        char *end_of_cname ;
                        const char * cname = (const char *)luaL_checklstring(L, -1, &length);
                        uint16_t cname_length = bonjour_write_domain_name(&frame->payload[cur_payload_pos+2],
                            maximum_required_data_size - (cur_payload_pos + 2 + sizeof(lmdns_frame)),
                            cname, &end_of_cname) ;

                        frame->payload[cur_payload_pos+0] = cname_length >> 8;
                        frame->payload[cur_payload_pos+1] = cname_length & 0xFF;
                        cur_payload_pos += 2 + cname_length ;
                    }
                    lua_pop(L,1) ;
                    break ;
                case BONJOUR_REQUEST_TYPE_SRV:
                    lua_getfield(L,-5,"host") ;
                    {
                        int length = 0 ;
                        char *end_of_hname ;
                        const char * hname = (const char *)luaL_checklstring(L, -1, &length);
                        uint16_t hname_length = bonjour_write_domain_name(&frame->payload[cur_payload_pos+8],
                            maximum_required_data_size - (cur_payload_pos + 8 + sizeof(lmdns_frame)),
                            hname, &end_of_hname) ;

                        lua_getfield(L,-6,"priority") ;
                        uint16_t priority = lua_tonumber(L,-1) ;
                        lua_pop(L, 1) ;
                        lua_getfield(L,-6,"weight") ;
                        uint16_t weight = lua_tonumber(L,-1) ;
                        lua_pop(L, 1) ;
                        lua_getfield(L,-6,"port") ;
                        uint16_t port = lua_tonumber(L,-1) ;
                        lua_pop(L, 1) ;

                        frame->payload[cur_payload_pos+0] = hname_length >> 8;
                        frame->payload[cur_payload_pos+1] = hname_length & 0xFF;
                        frame->payload[cur_payload_pos+2] = priority >> 8;
                        frame->payload[cur_payload_pos+3] = priority & 0xFF;
                        frame->payload[cur_payload_pos+4] = weight >> 8;
                        frame->payload[cur_payload_pos+5] = weight & 0xFF;
                        frame->payload[cur_payload_pos+6] = port >> 8;
                        frame->payload[cur_payload_pos+7] = port & 0xFF;
                        cur_payload_pos += 8 + hname_length ;
                    }
                    lua_pop(L,1) ;
                    break ;
            }
            actual_used_data_size = cur_payload_pos + sizeof(lmdns_frame);
            lua_pop(L,4) ;
        }
        lua_pop(L, 1) ;        // pop value
    }

    lua_pushlstring (L, (char *)frame, actual_used_data_size);
    c_free(frame) ;
    return 1 ;
}

static const LUA_REG_TYPE bonjour_requesttype_map[] =
{
  { LSTRKEY( "A" ),                     LNUMVAL( BONJOUR_REQUEST_TYPE_A ) },
  { LSTRKEY( "NS" ),                    LNUMVAL( BONJOUR_REQUEST_TYPE_NS ) },
  { LSTRKEY( "MD" ),                    LNUMVAL( BONJOUR_REQUEST_TYPE_MD ) },
  { LSTRKEY( "MF" ),                    LNUMVAL( BONJOUR_REQUEST_TYPE_MF ) },
  { LSTRKEY( "CNAME" ),                 LNUMVAL( BONJOUR_REQUEST_TYPE_CNAME ) },
  { LSTRKEY( "SOA" ),                   LNUMVAL( BONJOUR_REQUEST_TYPE_SOA ) },
  { LSTRKEY( "MB" ),                    LNUMVAL( BONJOUR_REQUEST_TYPE_MB ) },
  { LSTRKEY( "MG" ),                    LNUMVAL( BONJOUR_REQUEST_TYPE_MG ) },
  { LSTRKEY( "MR" ),                    LNUMVAL( BONJOUR_REQUEST_TYPE_MR ) },
  { LSTRKEY( "NULL" ),                  LNUMVAL( BONJOUR_REQUEST_TYPE_NULL ) },
  { LSTRKEY( "WKS" ),                   LNUMVAL( BONJOUR_REQUEST_TYPE_WKS ) },
  { LSTRKEY( "PTR" ),                   LNUMVAL( BONJOUR_REQUEST_TYPE_PTR ) },
  { LSTRKEY( "HINFO" ),                 LNUMVAL( BONJOUR_REQUEST_TYPE_HINFO ) },
  { LSTRKEY( "MINFO" ),                 LNUMVAL( BONJOUR_REQUEST_TYPE_MINFO ) },
  { LSTRKEY( "MX" ),                    LNUMVAL( BONJOUR_REQUEST_TYPE_MX ) },
  { LSTRKEY( "TXT" ),                   LNUMVAL( BONJOUR_REQUEST_TYPE_TXT ) },
  { LSTRKEY( "RP" ),                    LNUMVAL( BONJOUR_REQUEST_TYPE_RP ) },
  { LSTRKEY( "AFSDB" ),                 LNUMVAL( BONJOUR_REQUEST_TYPE_AFSDB ) },
  { LSTRKEY( "SIG" ),                   LNUMVAL( BONJOUR_REQUEST_TYPE_SIG ) },
  { LSTRKEY( "KEY" ),                   LNUMVAL( BONJOUR_REQUEST_TYPE_KEY ) },
  { LSTRKEY( "AAAA" ),                  LNUMVAL( BONJOUR_REQUEST_TYPE_AAAA ) },
  { LSTRKEY( "ANY" ),                   LNUMVAL( BONJOUR_REQUEST_TYPE_ANY ) },
  { LNILKEY, LNILVAL}
};

static const LUA_REG_TYPE bonjour_requestclass_map[] =
{
  { LSTRKEY( "IN" ),                    LNUMVAL( BONJOUR_REQUEST_TYPE_IN ) },
  { LSTRKEY( "CS" ),                    LNUMVAL( BONJOUR_REQUEST_TYPE_CS ) },
  { LSTRKEY( "CH" ),                    LNUMVAL( BONJOUR_REQUEST_TYPE_CH ) },
  { LSTRKEY( "HS" ),                    LNUMVAL( BONJOUR_REQUEST_TYPE_HS ) },
  { LSTRKEY( "QU" ),                    LNUMVAL( BONJOUR_REQUEST_TYPE_QU ) }
};

static const LUA_REG_TYPE bonjour_map[] =
{
  { LSTRKEY( "parse" ),                 LFUNCVAL( bonjour_parse )},
  { LSTRKEY( "create_request" ),        LFUNCVAL( bonjour_create_request )},
  { LSTRKEY( "create_response" ),       LFUNCVAL( bonjour_create_response )},
  { LSTRKEY( "TYPE" ),                  LROVAL( bonjour_requesttype_map )},
  { LSTRKEY( "CLASS" ),                 LROVAL( bonjour_requestclass_map )},
  { LSTRKEY( "PORT" ),                  LNUMVAL( 5353 ) },
  { LSTRKEY( "VERSION" ),               LNUMVAL( (BONJOUR_MODULEVERSION_MAJOR << 8) | (BONJOUR_MODULEVERSION_MINOR << 0)) },
  { LNILKEY, LNILVAL}
};

int luaopen_bonjour(lua_State *L) {
  return 0;
}

NODEMCU_MODULE(BONJOUR, "bonjour", bonjour_map, luaopen_bonjour);
