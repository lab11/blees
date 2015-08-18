#ifndef SPI_DRIVER_H

#define SPI_DRIVER_H

// Pin selections
#define LED_PIN 25
#define SPI_SCK_PIN 9
#define SPI_MISO_PIN 10
#define SPI_MOSI_PIN 11
#define SPI_SS_PIN 4
#define NRF_SPI NRF_SPI0

void spi_init();

void spi_write(uint8_t buf);

void spi_read(uint8_t* buf);

void spi_write_reg(uint8_t reg_addr, uint8_t * data, uint8_t num_bytes);

void spi_read_reg(uint8_t reg_addr, uint8_t * data, uint8_t num_bytes);

#endif