#include "dali/dali_encode.h"

typedef struct command_mode_map
{
	uint8_t value;
	uint8_t mode;
} command_mode_map;

typedef struct special_mode_map
{
	uint8_t value;
	uint8_t mode;
} special_mode_map;

#define COUNT_COMMANDS 41

const command_mode_map command_list[] = {
	{DALI_IMMEDIATE_OFF, _MODE_SIMPLE_},
	{DALI_UP_200MS, _MODE_SIMPLE_},
	{DALI_DOWN_200MS, _MODE_SIMPLE_},
	{DALI_STEP_UP, _MODE_SIMPLE_},
	{DALI_STEP_DOWN, _MODE_SIMPLE_},
	{DALI_RECALL_MAX_LEVEL, _MODE_SIMPLE_},
	{DALI_RECALL_MIN_LEVEL, _MODE_SIMPLE_},
	{DALI_STEP_DOWN_AND_OFF, _MODE_SIMPLE_},
	{DALI_ON_AND_STEP_UP, _MODE_SIMPLE_},
	{DALI_RESET, _MODE_REPEAT_TWICE_},
	//dtr commands
	{DALI_STORE_ACTUAL_DIM_LEVEL_IN_DTR, _MODE_REPEAT_TWICE_},
	{DALI_STORE_THE_DTR_AS_MAX_LEVEL, _MODE_REPEAT_TWICE_},
	{DALI_STORE_THE_DTR_AS_MIN_LEVEL, _MODE_REPEAT_TWICE_},
	{DALI_STORE_THE_DTR_AS_SYSTEM_FAILURE_LEVEL, _MODE_REPEAT_TWICE_},
	{DALI_STORE_THE_DTR_AS_POWER_ON_LEVEL, _MODE_REPEAT_TWICE_},
	{DALI_STORE_THE_DTR_AS_FADE_TIME, _MODE_REPEAT_TWICE_},
	{DALI_STORE_THE_DTR_AS_FADE_RATE, _MODE_REPEAT_TWICE_},
	{DALI_STORE_DTR_AS_SHORT_ADDRESS, _MODE_REPEAT_TWICE_},
	//query
	{DALI_QUERY_STATUS, _MODE_QUERY_},
	{DALI_QUERY_BALLAST, _MODE_QUERY_},
	{DALI_QUERY_LAMP_FAILURE, _MODE_QUERY_},
 	{DALI_QUERY_LAMP_POWER_ON, _MODE_QUERY_},
	{DALI_QUERY_LIMIT_ERROR, _MODE_QUERY_},
	{DALI_QUERY_RESET_STATE, _MODE_QUERY_},
	{DALI_QUERY_MISSING_SHORT_ADDRESS, _MODE_QUERY_},
	{DALI_QUERY_VERSION_NUMBER, _MODE_QUERY_},
	{DALI_QUERY_CONTENT_DTR, _MODE_QUERY_},
	{DALI_QUERY_DEVICE_TYPE, _MODE_QUERY_},
	{DALI_QUERY_PHYSICAL_MINIMUM_LEVEL, _MODE_QUERY_},
	{DALI_QUERY_POWER_FAILURE, _MODE_QUERY_},
	{DALI_QUERY_ACTUAL_LEVEL, _MODE_QUERY_},
	{DALI_QUERY_MAX_LEVEL, _MODE_QUERY_},
	{DALI_QUERY_MIN_LEVEL, _MODE_QUERY_},
	{DALI_QUERY_POWER_ON_LEVEL, _MODE_QUERY_},
	{DALI_QUERY_SYSTEM_FAILURE_LEVEL, _MODE_QUERY_},
	{DALI_QUERY_FADE, _MODE_QUERY_},
	{DALI_QUERY_GROUPS_0_7, _MODE_QUERY_},
	{DALI_QUERY_GROUPS_8_15, _MODE_QUERY_},
	{DALI_QUERY_RANDOM_ADDRESS_H, _MODE_QUERY_},
	{DALI_QUERY_RANDOM_ADDRESS_M, _MODE_QUERY_},
	{DALI_QUERY_RANDOM_ADDRESS_L, _MODE_QUERY_},
	};

#define COUNT_COMMANDS_WITH_PARAM 6

const command_mode_map command_with_param_list[] = {
	{DALI_GO_TO_SCENE, _MODE_SIMPLE_},
  {DALI_REMOVE_FROM_SCENE, _MODE_REPEAT_TWICE_},
  {DALI_ADD_TO_GROUP, _MODE_REPEAT_TWICE_},
  {DALI_REMOVE_FROM_GROUP, _MODE_REPEAT_TWICE_},
  {DALI_STORE_THE_DTR_AS_SCENE, _MODE_REPEAT_TWICE_},
  {DALI_QUERY_SCENE_LEVEL, _MODE_QUERY_}
	};

#define COUNT_SPECIAL_COMMANDS 14

const special_mode_map special_command_list[] = {
	{INITIALIZE, _MODE_REPEAT_TWICE_},
	{RANDOMIZE, _MODE_REPEAT_TWICE_},
	{STORE_DTR, _MODE_SIMPLE_},
	{TERMINATE, _MODE_SIMPLE_},
	{COMPARE, _MODE_QUERY_},
	{WITHDRAW, _MODE_SIMPLE_},
	{SEARCH_ADDRESS_H, _MODE_SIMPLE_},
	{SEARCH_ADDRESS_M, _MODE_SIMPLE_},
	{SEARCH_ADDRESS_L, _MODE_SIMPLE_},
	{PROGRAM_SHORT_ADDRESS, _MODE_SIMPLE_},
	{VERIFY_SHORT_ADDRESS, _MODE_QUERY_},
	{QUERY_SHORT_ADDRESS, _MODE_QUERY_},
	{PHYSICAL_SELECTION, _MODE_SIMPLE_},
	{ENABLE_DEVICE_TYPE, _MODE_SIMPLE_}
	};


int dali_get_type(uint8_t command)
{
	int i;
	for(i = 0; i < COUNT_COMMANDS; i++)
	{
		if(command == command_list[i].value)
			return _TYPE_COMMAND_;
	}
	for(i = 0; i < COUNT_COMMANDS_WITH_PARAM; i++)
	{
		if(command == command_with_param_list[i].value)
			return _TYPE_COMMAND_PARAM_;
	}
	for(i = 0; i < COUNT_SPECIAL_COMMANDS; i++)
	{
		if(command == special_command_list[i].value)
			return _TYPE_SPECIAL_;
	}

	return _ERR_WRONG_COMMAND_;
}

int dali_get_mode(uint8_t command)
{
	int i;
	for(i = 0; i < COUNT_COMMANDS; i++)
	{
		if(command == command_list[i].value)
			return command_list[i].mode;
	}
	for(i = 0; i < COUNT_COMMANDS_WITH_PARAM; i++)
	{
		if(command == command_with_param_list[i].value)
			return command_with_param_list[i].mode;
	}
	for(i = 0; i < COUNT_SPECIAL_COMMANDS; i++)
	{
		if(command == special_command_list[i].value)
			return special_command_list[i].mode;
	}

	return _ERR_WRONG_COMMAND_;
}


int dali_direct_arc(uint16_t *output, address_mode mode, uint8_t address, uint8_t brightness)
{
	switch(mode)
	{
		case SLAVE:
			return dali_slave_direct_arc(output, address, brightness);
			break;
		case GROUP:
			return dali_group_direct_arc(output, address, brightness);
			break;
		case BROADCAST:
			return dali_broadcast_direct_arc(output, brightness);
			break;
		default:
			return _ERR_WRONG_COMMAND_;
			break;
	}
}


int dali_command(uint16_t *output, address_mode mode, uint8_t address, uint8_t command)
{
	switch(mode)
	{
		case SLAVE:
			return dali_slave_command(output, address, command);
			break;
		case GROUP:
			return dali_group_command(output, address, command);
			break;
		case BROADCAST:
			return dali_broadcast_command(output, command);
			break;
		default:
			return _ERR_WRONG_COMMAND_;
			break;
	}
}

int dali_command_with_param(uint16_t *output, address_mode mode, uint8_t address, uint8_t command, uint8_t param)
{
	switch(mode)
	{
		case SLAVE:
			return dali_slave_command_with_param(output, address, command, param);
			break;
		case GROUP:
			return dali_group_command_with_param(output, address, command, param);
			break;
		case BROADCAST:
			return dali_broadcast_command_with_param(output, command, param);
			break;
		default:
			return _ERR_WRONG_COMMAND_;
			break;
	}
}

int dali_slave_direct_arc(uint16_t *output, uint8_t address, uint8_t brightness)
{
	if(address > 63)
	{
		return _ERR_WRONG_ADDRESS_;
	}

	brightness = brightness > 254 ? 254 : brightness; //direct arc value can be 254 maximum

	*output = ((address << 1) << 8) + brightness; //Frame: 0AAAAAA0 BBBBBBBB
	return _ERR_OK_;
}

int dali_group_direct_arc(uint16_t *output, uint8_t address, uint8_t brightness)
{
	if(address > 15)
	{
		return _ERR_WRONG_ADDRESS_;
	}

	brightness = brightness > 254 ? 254 : brightness; //direct arc value can be 254 maximum

	*output = ((0x80 + (address << 1)) << 8) + brightness; //Frame: 100AAAA0 BBBBBBBB
	return _ERR_OK_;
}

int dali_broadcast_direct_arc(uint16_t *output, uint8_t brightness)
{
	brightness = brightness > 254 ? 254 : brightness; //direct arc value can be 254 maximum

	*output = 0xFE00 + brightness; //Frame: 11111110 BBBBBBBB
	return _ERR_OK_;
}

int dali_slave_command(uint16_t *output, uint8_t address, uint8_t command)
{
	if(address > 63)
		return _ERR_WRONG_ADDRESS_;
	if(!((command & 0xF0) ^ 0x30)) //command 0011xxxx is reserved
		return _ERR_RESERVED_COMMAND_;
	if(!((command & 0xF0) ^ 0x80)) //command 1000xxxx is reserved
		return _ERR_RESERVED_COMMAND_;
	if(!((command & 0xFC) ^ 0x9C)) //command 100111xx is reserved
		return _ERR_RESERVED_COMMAND_;
	if(!((command & 0xF0) ^ 0xA0)) //command 1010xxxx is reserved
		return _ERR_RESERVED_COMMAND_;
	if(!((command & 0xE0) ^ 0xC0)) //command 110xxxxx is reserved
		return _ERR_RESERVED_COMMAND_;

	if((command == DALI_GO_TO_SCENE) ||
	(command == DALI_STORE_THE_DTR_AS_SCENE) ||
	(command == DALI_REMOVE_FROM_SCENE) ||
	(command == DALI_ADD_TO_GROUP) ||
	(command == DALI_REMOVE_FROM_GROUP) ||
	(command == DALI_QUERY_SCENE_LEVEL))
		return _ERR_WRONG_COMMAND_;

	*output = ((0x01 + (address << 1)) << 8) + command;  //Frame: 0AAAAAA1 CCCCCCCC
	return _ERR_OK_;
}

int dali_group_command(uint16_t *output, uint8_t address, uint8_t command)
{
	if(address > 15)
		return _ERR_WRONG_ADDRESS_;
	if(!((command & 0xF0) ^ 0x30)) //command 0011xxxx is reserved
		return _ERR_RESERVED_COMMAND_;
	if(!((command & 0xF0) ^ 0x80)) //command 1000xxxx is reserved
		return _ERR_RESERVED_COMMAND_;
	if(!((command & 0xFC) ^ 0x9C)) //command 100111xx is reserved
		return _ERR_RESERVED_COMMAND_;
	if(!((command & 0xF0) ^ 0xA0)) //command 1010xxxx is reserved
		return _ERR_RESERVED_COMMAND_;
	if(!((command & 0xE0) ^ 0xC0)) //command 110xxxxx is reserved
		return _ERR_RESERVED_COMMAND_;

	if((command == DALI_GO_TO_SCENE) ||
	(command == DALI_STORE_THE_DTR_AS_SCENE) ||
	(command == DALI_REMOVE_FROM_SCENE) ||
	(command == DALI_ADD_TO_GROUP) ||
	(command == DALI_REMOVE_FROM_GROUP) ||
	(command == DALI_QUERY_SCENE_LEVEL))
		return _ERR_WRONG_COMMAND_;

	*output = ((0x81 + (address << 1)) << 8) + command;  //Frame: 100AAAA1 CCCCCCCC
	return _ERR_OK_;
}

int dali_broadcast_command(uint16_t *output, uint8_t command)
{
	if(!((command & 0xF0) ^ 0x30)) //command 0011xxxx is reserved
		return _ERR_RESERVED_COMMAND_;
	if(!((command & 0xF0) ^ 0x80)) //command 1000xxxx is reserved
		return _ERR_RESERVED_COMMAND_;
	if(!((command & 0xFC) ^ 0x9C)) //command 100111xx is reserved
		return _ERR_RESERVED_COMMAND_;
	if(!((command & 0xF0) ^ 0xA0)) //command 1010xxxx is reserved
		return _ERR_RESERVED_COMMAND_;
	if(!((command & 0xE0) ^ 0xC0)) //command 110xxxxx is reserved
		return _ERR_RESERVED_COMMAND_;

	if((command == DALI_GO_TO_SCENE) ||
	(command == DALI_STORE_THE_DTR_AS_SCENE) ||
	(command == DALI_REMOVE_FROM_SCENE) ||
	(command == DALI_ADD_TO_GROUP) ||
	(command == DALI_REMOVE_FROM_GROUP) ||
	(command == DALI_QUERY_SCENE_LEVEL))
		return _ERR_WRONG_COMMAND_;

	*output = 0xFF00 + command;  //Frame: 11111111 CCCCCCCC
	return _ERR_OK_;
}


int dali_slave_command_with_param(uint16_t *output, uint8_t address, uint8_t command, uint8_t param)
{
	if(address > 63)
		return _ERR_WRONG_ADDRESS_;
	if(param > 15)
		return _ERR_WRONG_COMMAND_;

	if((command == DALI_GO_TO_SCENE) ||
	(command == DALI_STORE_THE_DTR_AS_SCENE) ||
	(command == DALI_REMOVE_FROM_SCENE) ||
	(command == DALI_ADD_TO_GROUP) ||
	(command == DALI_REMOVE_FROM_GROUP) ||
	(command == DALI_QUERY_SCENE_LEVEL))
	{
		*output = ((0x01 + (address << 1)) << 8) + command + param;  //Frame: 0AAAAAA1 CCCCPPPP
		return _ERR_OK_;
	}
	else
		return _ERR_WRONG_COMMAND_;
}

int dali_group_command_with_param(uint16_t *output, uint8_t address, uint8_t command, uint8_t param)
{
	if(address > 15)
		return _ERR_WRONG_ADDRESS_;
	if(param > 15)
		return _ERR_WRONG_COMMAND_;

	if((command == DALI_GO_TO_SCENE) ||
	(command == DALI_STORE_THE_DTR_AS_SCENE) ||
	(command == DALI_REMOVE_FROM_SCENE) ||
	(command == DALI_ADD_TO_GROUP) ||
	(command == DALI_REMOVE_FROM_GROUP) ||
	(command == DALI_QUERY_SCENE_LEVEL))
	{
		*output = ((0x81 + (address << 1)) << 8) + command + param;  //Frame: 100AAAA1 CCCCPPPP
		return _ERR_OK_;
	}
	else
		return _ERR_WRONG_COMMAND_;
}

int dali_broadcast_command_with_param(uint16_t *output, uint8_t command, uint8_t param)
{
	if(param > 15)
		return _ERR_WRONG_COMMAND_;

	if((command == DALI_GO_TO_SCENE) ||
	(command == DALI_STORE_THE_DTR_AS_SCENE) ||
	(command == DALI_REMOVE_FROM_SCENE) ||
	(command == DALI_ADD_TO_GROUP) ||
	(command == DALI_REMOVE_FROM_GROUP) ||
	(command == DALI_QUERY_SCENE_LEVEL))
	{
		*output = 0xFF00 + command + param;  //Frame: 100AAAA1 CCCCPPPP
		return _ERR_OK_;
	}
	else
		return _ERR_WRONG_COMMAND_;
}

int dali_special_command(uint16_t *output, special_command_type command, uint8_t data)
{
	switch(command)
	{
	case INITIALIZE:
		*output = (DALI_INITIALISE << 8) + data;  //Frame: 1010 0101 xxxx xxxx
		break;
	case RANDOMIZE:
		*output = DALI_RANDOMISE << 8;   //1010 0111 0000 0000
		break;
	case TERMINATE:
		*output = DALI_TERMINATE << 8;   //1010 0001 0000 0000
		break;
	case STORE_DTR:
		*output = (DALI_DATA_TRANSFER_REGISTER << 8) + data;
		break;
	case PROGRAM_SHORT_ADDRESS:
		*output = (DALI_PROGRAM_SHORT_ADDRESS << 8) + 0x01 + ((0x3F & data) << 1); //1011 0111 0xxx xxx1
		break;
	case VERIFY_SHORT_ADDRESS:
		*output = (DALI_VERIFY_SHORT_ADDRESS << 8) + 0x01 + ((0x3F & data) << 1);
		break;
	case QUERY_SHORT_ADDRESS:
		*output = DALI_QUERY_SHORT_ADDRESS << 8;
		break;
	case COMPARE:
		*output = DALI_COMPARE << 8;
		break;
	case WITHDRAW:
		*output = DALI_WITHDRAW << 8;
		break;
	case SEARCH_ADDRESS_H:
		*output = (DALI_SEARCHADDRH << 8) + data;
		break;
	case SEARCH_ADDRESS_M:
		*output = (DALI_SEARCHADDRM << 8) + data;
		break;
	case SEARCH_ADDRESS_L:
		*output = (DALI_SEARCHADDRL << 8) + data;
		break;
	case PHYSICAL_SELECTION:
		*output = (DALI_PHYSICAL_SELECTION << 8);
		break;
	case ENABLE_DEVICE_TYPE:
		*output = (DALI_ENABLE_DEVICE_TYPE_X << 8) + data;
		break;
	default:
		return _ERR_WRONG_COMMAND_;
		break;
	}
	return _ERR_OK_;
}
