#ifndef __MANCHESTER_H__
#define __MANCHESTER_H__

#include "c_types.h"
#include "os_type.h"

typedef enum {
    FIVE_BITS = 0x05,
    SIX_BITS = 0x06,
    SEVEN_BITS = 0x07,
    EIGHT_BITS = 0x08,
    SIXTEEN_BITS = 0x10
} ManchesterBitsNum4Char;

typedef enum {
    ONE_STOP_BIT             = 0,
    TWO_STOP_BIT             = BIT2
} ManchesterStopBitsNum;

typedef enum {
    BIT_RATE_300     = 300,
    BIT_RATE_600     = 600,
    BIT_RATE_1200    = 1200,
    BIT_RATE_2400    = 2400,
    BIT_RATE_4800    = 4800,
    BIT_RATE_9600    = 9600,
    BIT_RATE_19200   = 19200,
    BIT_RATE_38400   = 38400,
} ManchesterBautRate;

typedef struct {
    ManchesterBautRate 	     baut_rate;
    ManchesterBitsNum4Char  data_bits_tx;
    ManchesterBitsNum4Char  data_bits_rx;
    ManchesterStopBitsNum   stop_bits;
    uint8_t tx_pin;
    uint8_t rx_pin;
} ManchesterDevice;

void manchester_init(ManchesterBautRate baut, ManchesterBitsNum4Char data_bits_tx, ManchesterBitsNum4Char data_bits_rx, ManchesterStopBitsNum stop_bits, uint8_t tx_pin, uint8_t rx_pin, uint8_t task_prio, os_signal_t sig_input);
void manchester_putc(const uint16_t c);
void manchester_putc_timer(const uint16_t c);
void manchester_recive(uint16_t* c, uint32_t timeout_us);


#endif
