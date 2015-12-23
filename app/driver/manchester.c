#include "driver/manchester.h"
#include "platform.h"

static ManchesterDevice ManchesterDev;

#define DIRECT_READ(pin) GPIO_INPUT_GET(GPIO_ID_PIN(pin_num[pin]))

void ICACHE_FLASH_ATTR manchester_init(ManchesterBautRate baut, ManchesterBitsNum4Char data_bits_tx, ManchesterBitsNum4Char data_bits_rx, ManchesterStopBitsNum stop_bits, uint8_t tx_pin, uint8_t rx_pin, uint8_t task_prio, os_signal_t sig_input)
{
	platform_gpio_mode(rx_pin, PLATFORM_GPIO_INPUT, PLATFORM_GPIO_PULLUP);
	platform_gpio_mode(tx_pin, PLATFORM_GPIO_OUTPUT, PLATFORM_GPIO_PULLUP);

	ManchesterDev.baut_rate = baut;
	ManchesterDev.data_bits_tx = data_bits_tx;
	ManchesterDev.data_bits_rx = data_bits_rx;
	ManchesterDev.stop_bits = stop_bits;
	ManchesterDev.rx_pin = rx_pin;
	ManchesterDev.tx_pin = tx_pin;

	DIRECT_WRITE_HIGH(tx_pin);
}

inline bool wait_for(unsigned start_time, unsigned end_time)
{
	while ((0x7FFFFFFF & system_get_time()) < end_time)
	{
		//If system timer overflow, escape from while loop
		if ((0x7FFFFFFF & system_get_time()) < start_time){return false;}
		else {return true;}
	}

}

void ICACHE_FLASH_ATTR manchester_putc_timer(const uint16_t c)
{
	uint32_t buffer = 0;
	uint32_t start_time = 0;
	uint8_t length = (uint8_t)ManchesterDev.data_bits_tx;
	uint16_t half_bit_time = 500000 / ((uint16_t)ManchesterDev.baut_rate);

START:

	for(; 0 < length; length--)
	{
		buffer <<= 2;
		buffer = (c & 0x01) ? 0x02 : 0x01;
	}

	length = 2 * ((uint16_t)ManchesterDev.data_bits_tx);

	start_time = 0x7FFFFFFF & system_get_time();
	//write start bit
	DIRECT_WRITE_LOW(ManchesterDev.tx_pin);
	if(!wait_for(start_time, start_time + half_bit_time)) goto RETRY;
	start_time = start_time + half_bit_time;
	DIRECT_WRITE_HIGH(ManchesterDev.tx_pin);
	if(!wait_for(start_time, start_time + half_bit_time)) goto RETRY;
	start_time = start_time + half_bit_time;

	for(;0 < length; length--)
	{
		buffer & 0x01 ? DIRECT_WRITE_HIGH(ManchesterDev.tx_pin) : DIRECT_WRITE_LOW(ManchesterDev.tx_pin);
		buffer = buffer >> 1;
		if(!wait_for(start_time, start_time + half_bit_time)) goto RETRY;
		start_time = start_time + half_bit_time;
	}

	//send stopbits
	DIRECT_WRITE_LOW(ManchesterDev.tx_pin);
	if(!wait_for(start_time, start_time + half_bit_time)) goto RETRY;
	start_time = start_time + half_bit_time;
	DIRECT_WRITE_HIGH(ManchesterDev.tx_pin);
	if(!wait_for(start_time, start_time + half_bit_time)) goto RETRY;
	start_time = start_time + half_bit_time;

	if(ManchesterDev.stop_bits == BIT2)
	{
		DIRECT_WRITE_LOW(ManchesterDev.tx_pin);
		if(!wait_for(start_time, start_time + half_bit_time)) goto RETRY;
		start_time = start_time + half_bit_time;
		DIRECT_WRITE_HIGH(ManchesterDev.tx_pin);
		if(!wait_for(start_time, start_time + half_bit_time)) goto RETRY;
		start_time = start_time + half_bit_time;
	}

	os_delay_us(half_bit_time*6);

	return;

RETRY:
	wait_for(0x7FFFFFFF & system_get_time(), 0x7FFFFFFF & system_get_time() + 5*half_bit_time);
	goto START;
}

void ICACHE_FLASH_ATTR manchester_putc(const uint16_t c)
{
	uint32_t buffer = 0;
	uint8_t length = (uint8_t)ManchesterDev.data_bits_tx;

	uint16_t half_bit_time = 500000 / ((uint16_t)ManchesterDev.baut_rate);


	for(; 0 < length; length--)
	{
		buffer <<= 2;
		buffer = (c & 0x01) ? 0x02 : 0x01;
	}

	length = 2 * ((uint16_t)ManchesterDev.data_bits_tx);

	//send start bit
	noInterrupts();
	DIRECT_WRITE_LOW(ManchesterDev.tx_pin);
	delayMicroseconds(half_bit_time);
  DIRECT_WRITE_HIGH(ManchesterDev.tx_pin);
	delayMicroseconds(half_bit_time);

  //send data
  for(;0 < length; length--)
	{
		buffer & 0x01 ? DIRECT_WRITE_HIGH(ManchesterDev.tx_pin) : DIRECT_WRITE_LOW(ManchesterDev.tx_pin);
		buffer = buffer >> 1;
		delayMicroseconds(half_bit_time);
	}

  //send stopbits
	DIRECT_WRITE_LOW(ManchesterDev.tx_pin);
	delayMicroseconds(half_bit_time);
  DIRECT_WRITE_HIGH(ManchesterDev.tx_pin);
	delayMicroseconds(half_bit_time);

	if(ManchesterDev.stop_bits == BIT2)
	{
		DIRECT_WRITE_LOW(ManchesterDev.tx_pin);
		delayMicroseconds(half_bit_time);
		DIRECT_WRITE_HIGH(ManchesterDev.tx_pin);
		delayMicroseconds(half_bit_time);
	}
	interrupts();

}


void ICACHE_FLASH_ATTR manchester_recive(uint16_t* c, uint32_t timeout_us)
{
	uint16_t half_bit_time = 500000 / ((uint16_t)ManchesterDev.baut_rate);
	uint32_t start_time = 0x7FFFFFFF & system_get_time();
	uint32_t end_time = 0;
  uint32_t buffer = 0;

	timeout_us = 0x7FFFFFFF & timeout_us;

	timeout_us = (timeout_us + start_time) & 0x7FFFFFFF;

	while(true)
	{
		//wait for falling edge
		while(DIRECT_READ(ManchesterDev.rx_pin))
		{
			if((0x7FFFFFFF & system_get_time() > timeout_us) && (timeout_us > start_time))
				return;
			else if(((0x7FFFFFFF & system_get_time()) > timeout_us) && ((0x7FFFFFFF & system_get_time()) < start_time))
				return;
		}

		//sample bits on half half bit time
		start_time = (system_get_time() & 0x7FFFFFFF);
		end_time = start_time + half_bit_time/2;
	  wait_for(start_time, end_time);
		if(!READ_DIRECT(ManchesterDev.rx_pin)) //detecting glitch --> rx_pin still low, so valid
		{
			start_time = end_time;
		}
		else
		{
			continue; //glitch detected, continue waiting for falling edge
		}
	}
}
