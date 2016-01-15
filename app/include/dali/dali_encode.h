#ifndef __DALI_ENCODE__
#define __DALI_ENCODE__

#include "dali_codes.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {SLAVE, GROUP, BROADCAST} address_mode;

typedef unsigned short uint16_t;
typedef unsigned char  uint8_t;

#define _ERR_WRONG_ADDRESS_    -1
#define _ERR_OK_                0
#define _ERR_WRONG_COMMAND_    -2
#define _ERR_RESERVED_COMMAND_ -3
#define _MODE_SIMPLE_          40
#define _MODE_REPEAT_TWICE_    41
#define _MODE_QUERY_           42
#define _TYPE_COMMAND_         60
#define _TYPE_COMMAND_PARAM_   61
#define _TYPE_SPECIAL_         62
#define _TYPE_ARC_             63

#define dali_command_initialize_broadcast(output) dali_special_command(output, INITIALIZE, 0xFF)
#define dali_command_randomize(output) dali_special_command(output, RANDOMIZE, 0)
#define dali_command_terminate(output) dali_special_command(output, TERMINATE, 0)
#define dali_command_off(output,address) dali_slave_command(output,address,DALI_IMMEDIATE_OFF)

typedef enum {RANDOMIZE = DALI_RANDOMISE,
  INITIALIZE = DALI_INITIALISE,
  TERMINATE = DALI_TERMINATE,
  COMPARE = DALI_COMPARE,
  WITHDRAW = DALI_WITHDRAW,
  PROGRAM_SHORT_ADDRESS = DALI_PROGRAM_SHORT_ADDRESS,
  VERIFY_SHORT_ADDRESS = DALI_VERIFY_SHORT_ADDRESS,
  QUERY_SHORT_ADDRESS = DALI_QUERY_SHORT_ADDRESS,
  STORE_DTR = DALI_STORE_DTR_AS_SHORT_ADDRESS,
  SEARCH_ADDRESS_H = DALI_SEARCHADDRH,
  SEARCH_ADDRESS_M = DALI_SEARCHADDRM,
  SEARCH_ADDRESS_L = DALI_SEARCHADDRL,
  PHYSICAL_SELECTION = DALI_PHYSICAL_SELECTION,
  ENABLE_DEVICE_TYPE = DALI_ENABLE_DEVICE_TYPE_X} special_command_type;

int dali_get_type(uint8_t command);

int dali_get_mode(uint8_t command);

int dali_direct_arc(uint16_t *output, address_mode mode, uint8_t address, uint8_t brightness);

int dali_slave_direct_arc(uint16_t *output, uint8_t address, uint8_t brightness);

int dali_group_direct_arc(uint16_t *output, uint8_t address, uint8_t brightness);

int dali_broadcast_direct_arc(uint16_t* output, uint8_t brightness);

int dali_command(uint16_t *output, address_mode mode, uint8_t address, uint8_t command);

int dali_slave_command(uint16_t *output, uint8_t address, uint8_t command);

int dali_group_command(uint16_t *output, uint8_t address, uint8_t command);

int dali_broadcast_command(uint16_t* output, uint8_t command);

int dali_command_with_param(uint16_t *output, address_mode mode, uint8_t address, uint8_t command, uint8_t param);

int dali_slave_command_with_param(uint16_t *output, uint8_t address, uint8_t command, uint8_t param);

int dali_group_command_with_param(uint16_t *output, uint8_t address, uint8_t command, uint8_t param);

int dali_broadcast_command_with_param(uint16_t* output, uint8_t command, uint8_t param);

int dali_special_command(uint16_t *output, special_command_type command, uint8_t data);

#ifdef __cplusplus
}
#endif

#endif
