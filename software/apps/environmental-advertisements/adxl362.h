#include <stdint.h>


#ifndef ADXL362_H

#define ADXL362_H

// Pin selections
#define LED_PIN 25
#define SPI_SCK_PIN 9
#define SPI_MISO_PIN 10
#define SPI_MOSI_PIN 11
#define SPI_SS_PIN 4
#define NRF_SPI NRF_SPI0

typedef enum
{
    DISABLE_FIFO,
    OLDEST_SAVED_FIFO,
    STREAM_FIFO,
    TRIGGERED_FIFO
} fifo_mode;

typedef enum
{
    DEFAULT,
    LINKED,
    LOOP
} interrupt_mode;

typedef enum
{
    NORMAL,
    LOW_NOISE,
    ULTRALOW_NOISE
} noise_mode;


typedef enum
{
    RANGE_2G,
    RANGE_4G,
    RANGE_8G
} measurement_range;


typedef struct
{
    bool DATA_READY;
    bool FIFO_READY;
    bool FIFO_WATERMARK;
    bool FIFO_OVERRUN;
    bool ACT;
    bool INACT;
    bool AWAKE;
    bool INT_LOW;

} interrupt_map_t;


void config_interrupt_mode(interrupt_mode i_mode, bool use_referenced_activity, bool use_referenced_inactivity);

//if intmap_1 = 1, config for intpin 1
// if intmap_1 = 0, config for intpin 2
void config_INTMAP(interrupt_map_t * int_map, bool intmap_1);



void set_activity_threshold(uint16_t act_threshold);

void set_inactivity_threshold(uint16_t inact_threshold);



//ignored if device is on wake-up mode
void set_activity_time(uint8_t act_time);

void set_inactivity_time(uint16_t inact_time);



void activity_interrupt_enable();

void inactivity_interrupt_enable();

void activity_inactivity_interrupt_enable();



void config_FIFO(fifo_mode f_mode, bool store_temp, uint16_t num_samples);


uint8_t read_status_reg();



void sample_accel_word(uint8_t * x_data, uint8_t * y_data, uint8_t * z_data);

void sample_accel_byte(uint8_t * x_data, uint8_t * y_data, uint8_t * z_data);

void accelerometer_init(noise_mode n_mode, bool measure, bool autosleep_en, bool wakeup_en);

void spi_init();

void config_measurement_range(measurement_range m_range);


#endif // ADXL362_H