//#include "lua.h"
#include "module.h"
#include "lauxlib.h"
#include "platform.h"

#include "driver/manchester.h"


static int manchester_setup( lua_State *L )
{
  int total = lua_gettop( L );
  int baut, data_bits_tx, data_bits_rx, stop_bits, tx_pin, rx_pin;

  if(total != 6)
  {
    return luaL_error( L, "usage is manchester.setup(baut, data_buts_tx, data_bits_rx, stop_bits, tx_pin, rx_pin)" );
  }

  baut = luaL_checkinteger( L, 1 );
  data_bits_rx = luaL_checkinteger( L, 2 );
  data_bits_tx = luaL_checkinteger( L, 3 );
  stop_bits = luaL_checkinteger( L, 4 );
  tx_pin = luaL_checkinteger( L, 5 );
  rx_pin = luaL_checkinteger( L, 6 );

  if(baut != BIT_RATE_300 &&
    baut != BIT_RATE_600 &&
    baut != BIT_RATE_1200 &&
    baut != BIT_RATE_2400 &&
    baut != BIT_RATE_4800 &&
    baut != BIT_RATE_9600 &&
    baut != BIT_RATE_19200 &&
    baut != BIT_RATE_38400)
    return luaL_error( L, "invalid baut rate" );

  if(data_bits_rx != FIVE_BITS && data_bits_rx != SIX_BITS && data_bits_rx != SEVEN_BITS && data_bits_rx != EIGHT_BITS && data_bits_rx != SIXTEEN_BITS)
    return luaL_error( L, "invalid data length rx" );

  if(data_bits_tx != FIVE_BITS && data_bits_tx != SIX_BITS && data_bits_tx != SEVEN_BITS && data_bits_tx != EIGHT_BITS && data_bits_tx != SIXTEEN_BITS)
    return luaL_error( L, "invalid data length tx" );

  if(stop_bits != 1 && stop_bits != 2)
    return luaL_error( L, "stop bits must be 1 or 2" );

  MOD_CHECK_ID( gpio, tx_pin );
  MOD_CHECK_ID( gpio, rx_pin );

  manchester_init(baut, data_bits_tx, data_bits_rx, stop_bits, tx_pin, rx_pin);

  return 0;
}

static int manchester_read( lua_State *L )
{
 //TODO
}

static int manchester_write( lua_State *L )
{
  uint16_t c;
  int total = lua_gettop( L );

  if(total != 1)
    luaL_error( L, "usage is manchester.write(integer)" );

  c = luaL_checkinteger( L, 1 );

  manchester_putc(c);

  return 0;
}


static const LUA_REG_TYPE manchester_map[] =
{
  { LSTRKEY( "write" ),  LFUNCVAL( manchester_write ) },
  { LSTRKEY( "read" ),  LFUNCVAL( manchester_read ) },
  { LSTRKEY( "setup" ),  LFUNCVAL( manchester_setup ) },

  { LSTRKEY( "BAUT300" ), LNUMVAL( BIT_RATE_300 ) },
  { LSTRKEY( "BAUT600" ), LNUMVAL( BIT_RATE_600 ) },
  { LSTRKEY( "BAUT1200" ), LNUMVAL( BIT_RATE_1200 ) },
  { LSTRKEY( "BAUT2400" ), LNUMVAL( BIT_RATE_2400 ) },
  { LSTRKEY( "BAUT4800" ), LNUMVAL( BIT_RATE_4800 ) },
  { LSTRKEY( "BAUT9600" ), LNUMVAL( BIT_RATE_9600 ) },
  { LSTRKEY( "BAUT19200" ), LNUMVAL( BIT_RATE_19200 ) },
  { LSTRKEY( "BAUT38400" ), LNUMVAL( BIT_RATE_38400 ) },

  { LSTRKEY( "ONE_STOP_BIT" ), LNUMVAL( ONE_STOP_BIT ) },
  { LSTRKEY( "TWO_STOP_BIT" ), LNUMVAL( TWO_STOP_BIT ) },

  { LSTRKEY( "FIVE_BITS" ), LNUMVAL( FIVE_BITS ) },
  { LSTRKEY( "SIX_BITS" ), LNUMVAL( SIX_BITS ) },
  { LSTRKEY( "SEVEN_BITS" ), LNUMVAL( SEVEN_BITS ) },
  { LSTRKEY( "EIGHT_BITS" ), LNUMVAL( EIGHT_BITS ) },
  { LSTRKEY( "SIXTEEN_BITS" ), LNUMVAL( SIXTEEN_BITS ) },
  { LNILKEY, LNILVAL }
};

NODEMCU_MODULE(MANCHESTER, "manchester", manchester_map, NULL);
