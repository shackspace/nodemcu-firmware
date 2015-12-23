//#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include "lrotable.h"
#include "driver/onewire.h"


static int manchester_setup( lua_State *L )
{
  int total = lua_gettop( L );
}

static int manchester_read( lua_State *L )
{
  
}

static int manchester_write( lua_State *L )
{
  uint16_t c;
  int total = lua_gettop( L );

  if(total != 1)
    return 0;

  manchester_putc_timer(c);

  return 1;
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

  return 1;
#endif // #if LUA_OPTIMIZE_MEMORY > 0
}
