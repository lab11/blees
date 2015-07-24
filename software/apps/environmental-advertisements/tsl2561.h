#ifndef TSL2561_H

#define TSL2561_H


typedef enum
{
    HIGH,
    LOW
} tsl2561_gain_mode_t;

typedef enum
{
    MODE0,
    MODE1,
    MODE2
} tsl2561_integration_time_mode_t;


void tsl2561_readADC(uint16_t * channel0_data, uint16_t * channel1_data);

float tsl2561_readLux(void);

void tsl2561_on(void);

void tsl2561_off(void);

void tsl2561_interrupt_enable(uint16_t * threshold_low, uint16_t * threshold_high);

void tsl2561_interrupt_disable(void);

void tsl2561_config(tsl2561_gain_mode_t mode_gain, tsl2561_integration_time_mode_t mode_time);

void tsl2561_config_manual(tsl2561_gain_mode_t mode_gain, uint32_t man_time);


#endif // TSL2561_H