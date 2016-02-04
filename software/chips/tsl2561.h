#pragma once


typedef enum {
    tsl2561_GAIN_HIGH = 0x10,
    tsl2561_GAIN_LOW = 0x00
} tsl2561_gain_mode_t;

typedef enum {
    tsl2561_INTEGRATION_13p7MS = 0x00,
    tsl2561_INTEGRATION_101MS = 0x01,
    tsl2561_INTEGRATION_402MS = 0x02
} tsl2561_integration_time_mode_t;

typedef enum {
    tsl2561_PACKAGE_TFNCL = 0x00,
    tsl2561_PACKAGE_CS = 0x01,
} tsl2561_package_type_t;

void tsl2561_driver_init(nrf_drv_twi_t* p_instance, uint8_t i2c_addr);

void tsl2561_on(void);
void tsl2561_off(void); // turned sensor off. reading lux data after this will return a zero

void tsl2561_config(tsl2561_gain_mode_t mode_gain, tsl2561_integration_time_mode_t mode_time);

uint32_t tsl2561_read_lux();

void tsl2561_interrupt_enable(void);
void tsl2561_interrupt_disable(void);
