#include "driver/manchester.h"
#include "platform.h"
#include "osapi.h"
#define sprintf(...) os_sprintf( __VA_ARGS__ )

extern uint32_t system_get_time();

static ManchesterDevice ManchesterDev;

#define DIRECT_READ(pin)         (0x1 & GPIO_INPUT_GET(GPIO_ID_PIN(pin_num[pin])))
#define DIRECT_MODE_INPUT(pin)   GPIO_DIS_OUTPUT(pin_num[pin])
#define DIRECT_MODE_OUTPUT(pin)
#define DIRECT_WRITE_LOW(pin)    (GPIO_OUTPUT_SET(GPIO_ID_PIN(pin_num[pin]), 0))
#define DIRECT_WRITE_HIGH(pin)   (GPIO_OUTPUT_SET(GPIO_ID_PIN(pin_num[pin]), 1))
#define delayMicroseconds os_delay_us
#define noInterrupts ets_intr_lock
#define interrupts ets_intr_unlock

void ICACHE_FLASH_ATTR manchester_init(ManchesterBautRate baut, ManchesterBitsNum4Char data_bits_tx, ManchesterBitsNum4Char data_bits_rx, ManchesterStopBitsNum stop_bits, uint8_t tx_pin, uint8_t rx_pin)
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

inline bool wait_for(uint32_t start_time, uint32_t end_time)
{
	while ((0x7FFFFFFF & system_get_time()) < end_time)
	{
		//If system timer overflow, escape from while loop
		if ((0x7FFFFFFF & system_get_time()) < start_time) {return false;}
	}
	return true;
}

void ICACHE_FLASH_ATTR manchester_putc(const uint16_t c)
{
	//uint8_t sbuffer[64];
	uint32_t buffer = 0;
	uint32_t start_time = 0;
	uint8_t length = (uint8_t)ManchesterDev.data_bits_tx;
	uint32_t half_bit_time = 500000 / ((uint16_t)ManchesterDev.baut_rate);
	uint16_t _c = c;

	//sprintf((char*)sbuffer,"half_bit_time: %d \n", half_bit_time);
	//uart0_sendStr(sbuffer);

START:
  //uart0_sendStr("START\n");

	for(; 0 < length; length--)
	{
		buffer <<= 2;
		buffer |= (_c & 0x01) ? 0x02 : 0x01;
		_c >>= 1;
	}

	//uart0_sendStr("BUFFER created\n");

	length = 2 * ((uint16_t)ManchesterDev.data_bits_tx);

	start_time = 0x7FFFFFFF & system_get_time();
	//sprintf((char*)sbuffer,"start_time: %d \n", start_time);
	//uart0_sendStr(sbuffer);
	//sprintf((char*)sbuffer,"end_time: %d \n", start_time + half_bit_time);
	//uart0_sendStr(sbuffer);
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
	DIRECT_WRITE_HIGH(ManchesterDev.tx_pin);
	if(!wait_for(start_time, start_time + 2*half_bit_time)) os_delay_us(half_bit_time*6);
	start_time = start_time + 2*half_bit_time;

	if(ManchesterDev.stop_bits == TWO_STOP_BIT)
	{
		DIRECT_WRITE_HIGH(ManchesterDev.tx_pin);
		wait_for(start_time, start_time + 2*half_bit_time);
	}

	return;

RETRY:
	wait_for(0x7FFFFFFF & system_get_time(), 0x7FFFFFFF & system_get_time() + 5*half_bit_time);
	goto START;
}

void ICACHE_FLASH_ATTR manchester_recieve(uint16_t* c, uint32_t timeout_us)
{
	uint16_t half_bit_time = 500000 / ((uint16_t)ManchesterDev.baut_rate);
	uint32_t start_time = 0x7FFFFFFF & system_get_time();
	uint32_t end_time = 0;
  uint32_t buffer = 0;
	uint8_t length = 2 * ((uint16_t)ManchesterDev.data_bits_rx);

	*c = 0xFFFF;

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
		if(!DIRECT_READ(ManchesterDev.rx_pin)) //detecting glitch --> rx_pin still low, so valid
		{
			start_time = end_time;
			end_time = start_time + half_bit_time;
			wait_for(start_time, end_time);
			if(!DIRECT_READ(ManchesterDev.rx_pin)) continue; //signal is not high again

			//reading bits to buffer...
			for(;length > 0; length--)
			{
				start_time = end_time;
				end_time = start_time + half_bit_time;

				wait_for(start_time, end_time);

				buffer <<= 1;
				buffer = buffer | DIRECT_READ(ManchesterDev.rx_pin) ? 0x01 : 0x00;
			}

			start_time = end_time;
			end_time = start_time + half_bit_time;
			wait_for(start_time, end_time);
			if(DIRECT_READ(ManchesterDev.rx_pin)) continue; //signal is not high again

			start_time = end_time;
			end_time = start_time + half_bit_time;
			wait_for(start_time, end_time);
			if(!DIRECT_READ(ManchesterDev.rx_pin)) continue; //signal is not high again

			if(ManchesterDev.stop_bits == TWO_STOP_BIT)
			{
      	start_time = end_time;
				end_time = start_time + half_bit_time;
				wait_for(start_time, end_time);
				if(DIRECT_READ(ManchesterDev.rx_pin)) continue; //signal is not high again

				start_time = end_time;
				end_time = start_time + half_bit_time;
				wait_for(start_time, end_time);
				if(!DIRECT_READ(ManchesterDev.rx_pin)) continue; //signal is not high again
			}

		}
		else
		{
			continue; //glitch detected, continue waiting for falling edge
		}
	}
}
