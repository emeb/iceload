/*
 * oled.c - SSD1306 SPI OLED driver for STM32F373
 * 11-18-2018 E. Brombaugh
 */

#include "spi.h"
#include "tim.h"

/* macros to define pins */
#define SPI_CS_PIN                        GPIO_PIN_4
#define SPI_CS_GPIO_PORT                  GPIOA
#define SPI_RST_PIN                       GPIO_PIN_0
#define SPI_RST_GPIO_PORT                 GPIOA
#define SPI_DONE_PIN                      GPIO_PIN_1
#define SPI_DONE_GPIO_PORT                GPIOA

/* macros for GPIO setting */
#define SPI_CS_LOW()     HAL_GPIO_WritePin(SPI_CS_GPIO_PORT,SPI_CS_PIN,GPIO_PIN_RESET)
#define SPI_CS_HIGH()    HAL_GPIO_WritePin(SPI_CS_GPIO_PORT,SPI_CS_PIN,GPIO_PIN_SET)
#define SPI_RST_LOW()    HAL_GPIO_WritePin(SPI_RST_GPIO_PORT,SPI_RST_PIN,GPIO_PIN_RESET)
#define SPI_RST_HIGH()   HAL_GPIO_WritePin(SPI_RST_GPIO_PORT,SPI_RST_PIN,GPIO_PIN_SET)
#define SPI_DONE_GET()   HAL_GPIO_ReadPin(SPI_DONE_GPIO_PORT, SPI_DONE_PIN)
#define ICE5_SPI_DUMMY_BYTE   0xFF


/* SPI port handle */
static SPI_HandleTypeDef SpiHandle;

/*
 * Initialize the spi port and associated controls
 */
void spi_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	/* Enable SCK, MOSI  GPIO clocks */
	__HAL_RCC_GPIOA_CLK_ENABLE();

	/* Configure SPI1 CLK/MOSI/MISO as alternate function push-pull */
	GPIO_InitStructure.Pin = GPIO_PIN_7|GPIO_PIN_6|GPIO_PIN_5;
	GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Alternate = GPIO_AF0_SPI1;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Setup GPIO control bits */
	SPI_RST_HIGH();
	SPI_CS_HIGH();
	
	/* CS pin configuration */
	GPIO_InitStructure.Pin =  SPI_CS_PIN|SPI_RST_PIN;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
    
	/* Enable the SPI peripheral */
    __HAL_RCC_SPI1_CLK_ENABLE();
    
	SpiHandle.Instance               = SPI1;
	SpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
	SpiHandle.Init.Direction         = SPI_DIRECTION_2LINES;
	SpiHandle.Init.CLKPhase          = SPI_PHASE_1EDGE;
	SpiHandle.Init.CLKPolarity       = SPI_POLARITY_LOW;
	SpiHandle.Init.DataSize          = SPI_DATASIZE_8BIT;
	SpiHandle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
	SpiHandle.Init.TIMode            = SPI_TIMODE_DISABLE;
	SpiHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
	SpiHandle.Init.CRCPolynomial     = 7;
	SpiHandle.Init.NSS               = SPI_NSS_SOFT;
	SpiHandle.Init.Mode              = SPI_MODE_MASTER;
	HAL_SPI_Init(&SpiHandle);

	/* Enable the SPI peripheral */
    __HAL_SPI_ENABLE(&SpiHandle);
}

/*
 * send a byte via spi
 */
void spi_tx(uint8_t data)
{
    uint8_t dummy __attribute__ ((unused));
    
	/* Wait until the transmit buffer is empty */
	while(__HAL_SPI_GET_FLAG(&SpiHandle, SPI_FLAG_TXE) == RESET)
	{
	}

	/* Send the byte */
	*(__IO uint8_t *) ((uint32_t)SPI1+0x0C) = data;
	
	/* Wait to receive a byte*/
	while(__HAL_SPI_GET_FLAG(&SpiHandle, SPI_FLAG_RXNE) == RESET)
	{
	}

	/* get the byte read from the SPI bus */ 
	dummy = *(__IO uint8_t *) ((uint32_t)SPI1+0x0C);
}

/*
 * Send a block of data bytes via SPI
 */
uint32_t spi_send_data(uint8_t *data, uint32_t sz)
{	
    /* send all bytes */
	while(sz--)
    {
        spi_tx(*data++);
	}
    
    return 0;
}

/*
 * start the config process
 */
uint8_t spi_config_start(void)
{
    uint8_t retval = 0;
	uint32_t timeout;
	
	/* drop reset bit */
	SPI_RST_LOW();
	
	/* wait 1us */
	tim_usleep(1);
    
	/* drop CS bit to signal slave mode */
	SPI_CS_LOW();
	
	/* delay >200ns to allow FPGA to recognize reset */
	tim_usleep(200);
    
	/* Wait for done bit to go inactive */
	timeout = 1000;
	while(timeout && (SPI_DONE_GET()==GPIO_PIN_SET))
	{
		timeout--;
	}
	if(!timeout)
	{
		/* Done bit didn't respond to Reset */
		retval = 1;
	}
    
	/* raise reset w/ CS low for SPI SS mode */
	SPI_RST_HIGH();
	
	/* delay >300us to allow FPGA to clear */
	tim_usleep(2000);
    
    return retval;
}

/*
 * finish the config process
 */
uint8_t spi_config_finish(void)
{
    uint8_t retval = 0;
	uint32_t timeout;
	
    /* diagnostic - raise CS for final runout */
	//SPI_CS_HIGH();
    
    /* send clocks while waiting for DONE to assert */
	timeout = 1000;
	while(timeout && (SPI_DONE_GET()==GPIO_PIN_RESET))
	{
		spi_tx(ICE5_SPI_DUMMY_BYTE);
		timeout--;
	}
	if(!timeout)
	{
		/* FPGA didn't configure correctly */
		retval = 1;
	}
	
	/* send at least 49 more clocks */
	spi_tx(ICE5_SPI_DUMMY_BYTE);
	spi_tx(ICE5_SPI_DUMMY_BYTE);
	spi_tx(ICE5_SPI_DUMMY_BYTE);
	spi_tx(ICE5_SPI_DUMMY_BYTE);
	spi_tx(ICE5_SPI_DUMMY_BYTE);
	spi_tx(ICE5_SPI_DUMMY_BYTE);
	spi_tx(ICE5_SPI_DUMMY_BYTE);

	/* Raise CS bit for subsequent slave transactions */
	SPI_CS_HIGH();
	
	/* no error handling for now */
	return retval;
}

