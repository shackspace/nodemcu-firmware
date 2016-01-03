//#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include "lrotable.h"
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
}

static int manchester_read( lua_State *L )
{

}

static int manchester_write( lua_State *L )
{
  uint16_t c;
  int total = lua_gettop( L );

  if(total != 1)
    luaL_error( L, "usage is manchester.write(integer)" );

  c = luaL_checkinteger( L, 1 );

  manchester_putc_timer(c);

  return 0;
}

// Module function map
#define MIN_OPT_LEVEL   2
#include "lrodefs.h"
const LUA_REG_TYPE manchester_map[] =
{
  { LSTRKEY( "write" ),  LFUNCVAL( manchester_write ) },
  { LSTRKEY( "read" ),  LFUNCVAL( manchester_read ) },
  { LSTRKEY( "setup" ),  LFUNCVAL( manchester_setup ) },

#if LUA_OPTIMIZE_MEMORY > 0
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
#endif
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_manchester( lua_State *L )
{
#if LUA_OPTIMIZE_MEMORY > 0
  return 0;
#else // #if LUA_OPTIMIZE_MEMORY > 0
  luaL_register( L, AUXLIB_MANCHESTER, manchester_map );

  // Add the constants
  MOD_REG_NUMBER( L, "BAUT300", BIT_RATE_300 );
  MOD_REG_NUMBER( L, "BAUT600", BIT_RATE_600 );
  MOD_REG_NUMBER( L, "BAUT1200", BIT_RATE_1200 );
  MOD_REG_NUMBER( L, "BAUT2400", BIT_RATE_2400 );
  MOD_REG_NUMBER( L, "BAUT4800", BIT_RATE_4800 );
  MOD_REG_NUMBER( L, "BAUT9600", BIT_RATE_9600 );
  MOD_REG_NUMBER( L, "BAUT19200", BIT_RATE_19200 );
  MOD_REG_NUMBER( L, "BAUT38400", BIT_RATE_38400 );

  MOD_REG_NUMBER( L, "ONE_STOP_BIT", ONE_STOP_BIT );
  MOD_REG_NUMBER( L, "TWO_STOP_BIT", TWO_STOP_BIT );

  MOD_REG_NUMBER( L, "FIVE_BITS", FIVE_BITS );
  MOD_REG_NUMBER( L, "SIX_BITS", SIX_BITS );
  MOD_REG_NUMBER( L, "SEVEN_BITS", SEVEN_BITS );
  MOD_REG_NUMBER( L, "EIGHT_BITS", EIGHT_BITS );
  MOD_REG_NUMBER( L, "SIXTEEN_BITS", SIXTEEN_BITS );
  return 1;
#endif // #if LUA_OPTIMIZE_MEMORY > 0
}
