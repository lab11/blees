#ifndef BLEES_H
#define BLEES_H

#ifndef DEVICE_NAME
    #define DEVICE_NAME "BLEES"
#endif /*DEVICE_NAME*/

#include <stdint.h>

extern uint8_t MAC_ADDR[6];
#define PLATFORM_ID_BYTE 0x30

// Address is written here in flash if the ID command is used
#define ADDRESS_FLASH_LOCATION 0x0003fff8

// FTDI chip for bootloader
#define BOOTLOADER_CTRL_PIN  3
#define BOOTLOADER_CTRL_PULL NRF_GPIO_PIN_PULLUP
#define BOOTLOADER_RX_PIN 28
#define BOOTLOADER_TX_PIN 29

// Blue led on squall
#define SQUALL_LED_PIN 13

// Blue led on BLEES
#define BLEES_LED_PIN 25

// UART on squall
#define UART_RX_PIN 28
#define UART_TX_PIN 29

// I2C through headers
#define I2C_SCL_PIN 7
#define I2C_SDA_PIN 8

// SPI through headers
#define SPI_SCLK_PIN 9
#define SPI_MISO_PIN 10
#define SPI_MOSI_PIN 11

// Light sensor
#define TSL2560_IRQ_PIN 22

// Pressure sensor
#define LPS331AP_IRQ1_PIN 24
#define LPS331AP_IRQ2_PIN 23

// Accelerometer
#define ADXL362_CS_PIN 4
#define ADXL362_IRQ1_PIN 6
#define ADXL362_IRQ2_PIN 5

#endif /*BLEES_H*/

