#include "lualib.h"
#include "lauxlib.h"
#include "auxmods.h"
#include "lrotable.h"
#include "driver/manchester.h"
#include "dali/dali_encode.h"

static int dali_setup( lua_State *L )
{
  int total = lua_gettop( L );
  int tx_pin, rx_pin;

  if(total != 2)
  {
    return luaL_error( L, "usage is dali.setup(tx_pin, rx_pin)" );
  }

  tx_pin = luaL_checkinteger( L, 1 );
  rx_pin = luaL_checkinteger( L, 2 );

  MOD_CHECK_ID( gpio, tx_pin );
  MOD_CHECK_ID( gpio, rx_pin );

  manchester_init(BIT_RATE_1200, SIXTEEN_BITS, EIGHT_BITS, TWO_STOP_BIT, tx_pin, rx_pin);

  return 0;
}

static int dali_arc( lua_State* L )
{
  int total = lua_gettop( L );
  int mode, address, brigthness;
  uint16_t dali_frame;

  if(total != 3)
  {
    return luaL_error( L, "usage is dali.arc(address_mode, address, brightness)" );
  }
  mode = luaL_checkinteger( L, 1 );
  address = luaL_checkinteger( L, 2 );
  brigthness = luaL_checkinteger( L, 3 );

  if(dali_direct_arc(&dali_frame, mode, address, brigthness) == _ERR_OK_)
  {
    manchester_putc(dali_frame);
  }
  else
  {
    return luaL_error( L, "error encoding frame" );
  }

  return 0;
}

static int dali_send( lua_State* L )
{
  int total = lua_gettop( L );
  int mode, command;
  int address = 0;
  int param = 0;
  int type, frame_mode;

  uint16_t dali_frame;

  if(total < 2)
  {
    return luaL_error( L, "usage is dali.send(address_mode, command, [address, param])" );
  }
  mode = luaL_checkinteger( L, 1 );
  command = luaL_checkinteger( L, 2 );

  if((type = dali_get_type(command)) == _ERR_WRONG_COMMAND_)
    luaL_error( L, "unknown command!" );

  frame_mode = dali_get_mode(command);

  if(total == 3)
  {
    address = luaL_checkinteger( L, 3 );
  }
  else
  {
    param = luaL_checkinteger( L, 4 );
  }

  switch(type)
  {
    case _TYPE_COMMAND_:
      if(total < 3)
        return luaL_error( L, "expected function is dali.send(address_mode, command, address)" );
      dali_command(&dali_frame, mode, address, command);
      break;
    case _TYPE_COMMAND_PARAM_:
      if(total != 4)
        return luaL_error( L, "expected function is dali.send(address_mode, command, address, param)" );
      dali_command_with_param(&dali_frame, mode, address, command, param);
      break;
    case _TYPE_SPECIAL_:
      if(total != 4)
        return luaL_error( L, "expected function is dali.send(address_mode, command, address, param)" );
      dali_special_command(&dali_frame, command, param);
      break;
  }

  if(frame_mode == _MODE_SIMPLE_)
  {
    manchester_putc(dali_frame);
  }
  else if(frame_mode == _MODE_REPEAT_TWICE_)
  {
    manchester_putc(dali_frame);
    os_delay_us(9000);
    manchester_putc(dali_frame);
  }
  else
  {
    return luaL_error( L, "frame type not supported" ); //TODO: implement request frame
  }

  return 0;
}

#define KEY(STRING) { LSTRKEY( "STRING" ), LNUMVAL( STRING ) }
#define REG(STRING) MOD_REG_NUMBER( L, "STRING", STRING )

// Module function map
#define MIN_OPT_LEVEL   2
#include "lrodefs.h"
const LUA_REG_TYPE dali_map[] =
{
  { LSTRKEY( "setup" ),  LFUNCVAL( dali_setup ) },
  { LSTRKEY( "arc" ), LFUNCVAL( dali_arc ) },

#if LUA_OPTIMIZE_MEMORY > 0
  { LSTRKEY( "BROADCAST" ), LNUMVAL( BROADCAST ) },
  { LSTRKEY( "SLAVE" ), LNUMVAL( SLAVE ) },
  { LSTRKEY( "GROUP" ), LNUMVAL( GROUP ) },

  KEY(DALI_IMMEDIATE_OFF),
  KEY(DALI_UP_200MS),
  KEY(DALI_DOWN_200MS),
  KEY(DALI_STEP_UP),
  KEY(DALI_STEP_DOWN),
  KEY(DALI_RECALL_MAX_LEVEL),
  KEY(DALI_RECALL_MIN_LEVEL),
  KEY(DALI_STEP_DOWN_AND_OFF),
  KEY(DALI_ON_AND_STEP_UP),
  KEY(DALI_GO_TO_SCENE),

  KEY(DALI_RESET),
  KEY(DALI_STORE_ACTUAL_DIM_LEVEL_IN_DTR),

  KEY(DALI_STORE_THE_DTR_AS_MAX_LEVEL),
  KEY(DALI_STORE_THE_DTR_AS_MIN_LEVEL),
  KEY(DALI_STORE_THE_DTR_AS_SYSTEM_FAILURE_LEVEL),
  KEY(DALI_STORE_THE_DTR_AS_POWER_ON_LEVEL),
  KEY(DALI_STORE_THE_DTR_AS_FADE_TIME),
  KEY(DALI_STORE_THE_DTR_AS_FADE_RATE),
  KEY(DALI_STORE_THE_DTR_AS_SCENE),

  KEY(DALI_REMOVE_FROM_SCENE),
  KEY(DALI_ADD_TO_GROUP),
  KEY(DALI_REMOVE_FROM_GROUP),
  KEY(DALI_STORE_DTR_AS_SHORT_ADDRESS),

  KEY(DALI_QUERY_STATUS),
  KEY(DALI_QUERY_BALLAST),
  KEY(DALI_QUERY_LAMP_FAILURE),
  KEY(DALI_QUERY_LAMP_POWER_ON),
  KEY(DALI_QUERY_LIMIT_ERROR),
  KEY(DALI_QUERY_RESET_STATE),
  KEY(DALI_QUERY_MISSING_SHORT_ADDRESS),
  KEY(DALI_QUERY_VERSION_NUMBER),
  KEY(DALI_QUERY_CONTENT_DTR),
  KEY(DALI_QUERY_DEVICE_TYPE),
  KEY(DALI_QUERY_PHYSICAL_MINIMUM_LEVEL),
  KEY(DALI_QUERY_POWER_FAILURE),

  KEY(DALI_QUERY_ACTUAL_LEVEL),
  KEY(DALI_QUERY_MAX_LEVEL),
  KEY(DALI_QUERY_MIN_LEVEL),
  KEY(DALI_QUERY_POWER_ON_LEVEL),
  KEY(DALI_QUERY_SYSTEM_FAILURE_LEVEL),
  KEY(DALI_QUERY_FADE),

  KEY(DALI_QUERY_SCENE_LEVEL),
  KEY(DALI_QUERY_GROUPS_0_7),
  KEY(DALI_QUERY_GROUPS_8_15),
  KEY(DALI_QUERY_RANDOM_ADDRESS_H),
  KEY(DALI_QUERY_RANDOM_ADDRESS_M),
  KEY(DALI_QUERY_RANDOM_ADDRESS_L),

  KEY(DALI_QUERY_APPLICATION_EXTENTED_COMMAND),

  KEY(DALI_TERMINATE),

  KEY(DALI_DATA_TRANSFER_REGISTER),

  KEY(DALI_INITIALISE),
  KEY(DALI_RANDOMISE),
  KEY(DALI_COMPARE),
  KEY(DALI_WITHDRAW),
  KEY(DALI_SEARCHADDRH),
  KEY(DALI_SEARCHADDRM),
  KEY(DALI_SEARCHADDRL),
  KEY(DALI_PROGRAM_SHORT_ADDRESS),
  KEY(DALI_VERIFY_SHORT_ADDRESS),
  KEY(DALI_QUERY_SHORT_ADDRESS),
  KEY(DALI_PHYSICAL_SELECTION),

  KEY(DALI_ENABLE_DEVICE_TYPE_X),

#endif
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_dali( lua_State *L )
{
#if LUA_OPTIMIZE_MEMORY > 0
  return 0;
#else // #if LUA_OPTIMIZE_MEMORY > 0
  luaL_register( L, AUXLIB_DALI, dali_map );

  // Add the constants
  MOD_REG_NUMBER( L, "BROADCAST", BROADCAST );
  MOD_REG_NUMBER( L, "SLAVE", SLAVE );
  MOD_REG_NUMBER( L, "GROUP", GROUP );

  REG(DALI_IMMEDIATE_OFF);
  REG(DALI_UP_200MS);
  REG(DALI_DOWN_200MS);
  REG(DALI_STEP_UP);
  REG(DALI_STEP_DOWN);
  REG(DALI_RECALL_MAX_LEVEL);
  REG(DALI_RECALL_MIN_LEVEL);
  REG(DALI_STEP_DOWN_AND_OFF);
  REG(DALI_ON_AND_STEP_UP);
  REG(DALI_GO_TO_SCENE);

  REG(DALI_RESET);
  REG(DALI_STORE_ACTUAL_DIM_LEVEL_IN_DTR);

  REG(DALI_STORE_THE_DTR_AS_MAX_LEVEL);
  REG(DALI_STORE_THE_DTR_AS_MIN_LEVEL);
  REG(DALI_STORE_THE_DTR_AS_SYSTEM_FAILURE_LEVEL);
  REG(DALI_STORE_THE_DTR_AS_POWER_ON_LEVEL);
  REG(DALI_STORE_THE_DTR_AS_FADE_TIME);
  REG(DALI_STORE_THE_DTR_AS_FADE_RATE);
  REG(DALI_STORE_THE_DTR_AS_SCENE);

  REG(DALI_REMOVE_FROM_SCENE);
  REG(DALI_ADD_TO_GROUP);
  REG(DALI_REMOVE_FROM_GROUP);
  REG(DALI_STORE_DTR_AS_SHORT_ADDRESS);

  REG(DALI_QUERY_STATUS);
  REG(DALI_QUERY_BALLAST);
  REG(DALI_QUERY_LAMP_FAILURE);
  REG(DALI_QUERY_LAMP_POWER_ON);
  REG(DALI_QUERY_LIMIT_ERROR);
  REG(DALI_QUERY_RESET_STATE);
  REG(DALI_QUERY_MISSING_SHORT_ADDRESS);
  REG(DALI_QUERY_VERSION_NUMBER);
  REG(DALI_QUERY_CONTENT_DTR);
  REG(DALI_QUERY_DEVICE_TYPE);
  REG(DALI_QUERY_PHYSICAL_MINIMUM_LEVEL);
  REG(DALI_QUERY_POWER_FAILURE);

  REG(DALI_QUERY_ACTUAL_LEVEL);
  REG(DALI_QUERY_MAX_LEVEL);
  REG(DALI_QUERY_MIN_LEVEL);
  REG(DALI_QUERY_POWER_ON_LEVEL);
  REG(DALI_QUERY_SYSTEM_FAILURE_LEVEL);
  REG(DALI_QUERY_FADE);

  REG(DALI_QUERY_SCENE_LEVEL);
  REG(DALI_QUERY_GROUPS_0_7);
  REG(DALI_QUERY_GROUPS_8_15);
  REG(DALI_QUERY_RANDOM_ADDRESS_H);
  REG(DALI_QUERY_RANDOM_ADDRESS_M);
  REG(DALI_QUERY_RANDOM_ADDRESS_L);

  REG(DALI_QUERY_APPLICATION_EXTENTED_COMMAND);

  REG(DALI_TERMINATE);

  REG(DALI_DATA_TRANSFER_REGISTER);

  REG(DALI_INITIALISE);
  REG(DALI_RANDOMISE);
  REG(DALI_COMPARE);
  REG(DALI_WITHDRAW);
  REG(DALI_SEARCHADDRH);
  REG(DALI_SEARCHADDRM);
  REG(DALI_SEARCHADDRL);
  REG(DALI_PROGRAM_SHORT_ADDRESS);
  REG(DALI_VERIFY_SHORT_ADDRESS);
  REG(DALI_QUERY_SHORT_ADDRESS);
  REG(DALI_PHYSICAL_SELECTION);

  REG(DALI_ENABLE_DEVICE_TYPE_X);
  return 1;
#endif // #if LUA_OPTIMIZE_MEMORY > 0
}
