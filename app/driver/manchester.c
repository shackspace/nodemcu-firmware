#include "driver/manchester.h"
#include "platform.h"

static ManchesterDevice ManchesterDev;

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


void ICACHE_FLASH_ATTR manchester_recive(uint16_t* c, uint16_t timeout_us)
{

}
