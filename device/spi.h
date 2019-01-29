/*
 * spi.h - spi driver for F042_usb_spi
 * 01-23-2019 E. Brombaugh
 */

#ifndef __spi__
#define __spi__

#include "stm32f0xx.h"

void spi_init(void);
uint32_t spi_send_data(uint8_t *data, uint32_t sz);
uint8_t spi_config_start(void);
uint8_t spi_config_finish(void);

#endif
