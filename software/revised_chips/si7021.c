// SI7021 temperature/humidity sensor driver

//***Libraries***
#include <stdint.h>

#include "si7021.h"
#include "simple_ble.h"

#include "app_twi.h"
#include "app_timer.h"


//***Global data***

// I2C peripheral used for this peripheral
static app_twi_t* twi;

// I2C address of SI7021
#define SI7021_ADDR 0x40

// Command registers
#define SI7021_RESET 	    0xFE
#define SI7021_RH_NO_HOLD   0xF5
#define SI7021_TEMP_NO_HOLD 0xF3
#define SI7021_READ_TEMP    0xE0

// measurement storage
static uint8_t rh_measurement[2] = {0};
static uint8_t temp_measurement[2] = {0};

// reset transaction
static uint8_t const RESET_CMD[1] = {SI7021_RESET};
#define RESET_TXFR_LEN 1
static app_twi_transfer_t const RESET_TXFR[RESET_TXFR_LEN] = {
    APP_TWI_WRITE(SI7021_ADDR, RESET_CMD, 1, 0),
};

// measure temperature transaction
static uint8_t const MEAS_TEMP_CMD[1] = {SI7021_TEMP_NO_HOLD};
#define MEAS_TEMP_TXFR_LEN 1
static app_twi_transfer_t const MEAS_TEMP_TXFR[MEAS_TEMP_TXFR_LEN] = {
    APP_TWI_WRITE(SI7021_ADDR, MEAS_TEMP_CMD, 1, 0),
};

// read temperature transaction
//NOTE: must be used after a MEAS_TEMP_TXFR, with a ~7 ms delay
#define READ_TEMP_TXFR_LEN 1
static app_twi_transfer_t const READ_TEMP_TXFR[READ_TEMP_TXFR_LEN] = {
    APP_TWI_READ(SI7021_ADDR, temp_measurement, 2, 0),
};

// measure relative humidity & temperature transaction
static uint8_t const MEAS_RH_TEMP_CMD[1] = {SI7021_RH_NO_HOLD};
#define MEAS_RH_TEMP_TXFR_LEN 1
static app_twi_transfer_t const MEAS_RH_TEMP_TXFR[MEAS_RH_TEMP_TXFR_LEN] = {
    APP_TWI_WRITE(SI7021_ADDR, MEAS_RH_TEMP_CMD, 1, 0),
};

// read relative humidity & tremperature transaction
//NOTE: must be used after a MEAS_RH_TEMP_CMD, with a ~17 ms delay
static uint8_t const READ_TEMP_CMD[1] = {SI7021_READ_TEMP};
#define READ_RH_TEMP_TXFR_LEN 3
static app_twi_transfer_t const READ_RH_TEMP_TXFR[READ_RH_TEMP_TXFR_LEN] = {
    APP_TWI_READ(SI7021_ADDR, rh_measurement, 2, 0),
    APP_TWI_WRITE(SI7021_ADDR, READ_TEMP_CMD, 1, APP_TWI_NO_STOP),
    APP_TWI_READ(SI7021_ADDR, temp_measurement, 2, 0),
};

// timer for use by driver
APP_TIMER_DEF(si7021_timer);

// delay times for various operations. Typical times from datasheet
#define RESET_DELAY                 APP_TIMER_TICKS( 5, APP_TIMER_PRESCALER)
#define TEMP_CONVERSION_DELAY       APP_TIMER_TICKS( 7, APP_TIMER_PRESCALER)
#define RH_TEMP_CONVERSION_DELAY    APP_TIMER_TICKS(17, APP_TIMER_PRESCALER)

// state of events in driver
typedef enum {
    NONE=0,
    RESET_STARTED,
    RESET_COMPLETE,
    READ_RH_TEMP_STARTED,
    READ_RH_TEMP_READY,
    READ_RH_TEMP_COMPLETE,
    READ_TEMP_STARTED,
    READ_TEMP_READY,
    READ_TEMP_COMPLETE,
} si7021_state_t;
static si7021_state_t state = NONE;


//***Prototypes***
static void si7021_event_handler ();
static void si7021_read_rh_temp_data ();
static void si7021_read_temp_data ();
static void si7021_calculate_humidity (uint16_t*);
static void si7021_calculate_temperature (int16_t*);


//***Functions***

// Note: expects app_twi_init(...) to have already been run
// Note: expects APP_TIMER_INIT(...) to have already been run
void si7021_init (app_twi_t* twi_instance, void(*callback)(void)) {
    twi = twi_instance;

    // initialize timer
    //NOTE: interacting with timers in different contexts (i.e. interrupt and
    //  normal code) can cause goofy things to happen. In this case, using
    //  APP_IRQ_PRIORITY_HIGH for the TWI will cause timer delays in this
    //  driver to have improper lengths
    uint32_t err_code;
    err_code = app_timer_create(&si7021_timer, APP_TIMER_MODE_SINGLE_SHOT, si7021_event_handler);
    APP_ERROR_CHECK(err_code);

    // initialize the SI7021 by resetting it
    //  the default settings on reset are sufficient
    si7021_reset(callback);
}

static void (*reset_callback)(void) = NULL;
void si7021_reset (void (*callback)(void)) {
    // store user callback
    reset_callback = callback;

    // set next state
    state = RESET_STARTED;

    // reset device
    // no need to configure user register, reset has valid settings
    static app_twi_transaction_t const transaction = {
        .p_transfers = RESET_TXFR,
        .number_of_transfers = RESET_TXFR_LEN,
        .callback = si7021_event_handler,
        .p_user_data = NULL,
    };
    app_twi_schedule(twi, &transaction);
}

static void (*read_humidity_and_temp_callback)(uint16_t, int16_t) = NULL;
void si7021_read_humidity_and_temp (void (*callback)(uint16_t humidity, int16_t temperature)) {
    // store user callback
    read_humidity_and_temp_callback = callback;

    // set next state
    state = READ_RH_TEMP_STARTED;

    // start humidity and temperature measurement
    static app_twi_transaction_t const transaction = {
        .p_transfers = MEAS_RH_TEMP_TXFR,
        .number_of_transfers = MEAS_RH_TEMP_TXFR_LEN,
        .callback = si7021_event_handler,
        .p_user_data = NULL,
    };
    app_twi_schedule(twi, &transaction);
}

//NOTE: expected to be run after a successful MEAS_RH_TEMP_TXFR transaction
static void si7021_read_rh_temp_data () {
    // read data from completed measurement
    static app_twi_transaction_t const transaction = {
        .p_transfers = READ_RH_TEMP_TXFR,
        .number_of_transfers = READ_RH_TEMP_TXFR_LEN,
        .callback = si7021_event_handler,
        .p_user_data = NULL,
    };
    app_twi_schedule(twi, &transaction);
}

static void (*read_temp_callback)(int16_t) = NULL;
void si7021_read_temp (void (*callback)(int16_t temperature)) {
    // store user callback
    read_temp_callback = callback;

    // set next state
    state = READ_TEMP_STARTED;

    // start temperature measurement
    uint32_t err_code;
    static app_twi_transaction_t const transaction = {
        .p_transfers = MEAS_TEMP_TXFR,
        .number_of_transfers = MEAS_TEMP_TXFR_LEN,
        .callback = si7021_event_handler,
        .p_user_data = NULL,
    };
    err_code = app_twi_schedule(twi, &transaction);
    APP_ERROR_CHECK(err_code);
}

//NOTE: expected to be run after a successful MEAS_TEMP_TXFR transaction
static void si7021_read_temp_data () {
    // read data from completed measurement
    static app_twi_transaction_t const transaction = {
        .p_transfers = READ_TEMP_TXFR,
        .number_of_transfers = READ_TEMP_TXFR_LEN,
        .callback = si7021_event_handler,
        .p_user_data = NULL,
    };
    app_twi_schedule(twi, &transaction);
}

static void si7021_calculate_humidity(uint16_t* humidity) {
    // calculate relative humidity in hundredths of percent
    //  calculation form si7021 datasheet, multiplied by 100
    uint32_t measurement = (((uint32_t)rh_measurement[0] << 8) | (uint32_t)rh_measurement[1]);
    *humidity = (uint16_t)(((125 * 100 * measurement) / 65536) - 600);
}

static void si7021_calculate_temperature(int16_t* temperature) {
    // calculate temperature in hundredths of degrees centigrade
    //  calculation from si7021 datasheet, multipled by 100
    int32_t measurement = (((int32_t)temp_measurement[0] << 8) | (uint32_t)temp_measurement[1]);
    *temperature = (int16_t)(((17572 * measurement) / 65536) - 4685);
}


// handle si7021 events
static void si7021_event_handler () {
    uint32_t err_code;

    switch (state) {
        case RESET_STARTED:
            // set next state
            state = RESET_COMPLETE;

            // delay until the reset is complete
            err_code = app_timer_start(si7021_timer, RESET_DELAY, NULL);
            APP_ERROR_CHECK(err_code);

            break;

        case RESET_COMPLETE:
            // finished
            state = NONE;

            // alert user of completion
            if (reset_callback) {
                reset_callback();
            }
            break;

        case READ_RH_TEMP_STARTED:
            // set next state
            state = READ_RH_TEMP_READY;

            // delay until measurement is complete
            err_code = app_timer_start(si7021_timer, RH_TEMP_CONVERSION_DELAY, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case READ_RH_TEMP_READY:
            // set next state
            state = READ_RH_TEMP_COMPLETE;

            // read data from sensor
            si7021_read_rh_temp_data();
            break;

        case READ_RH_TEMP_COMPLETE:
            // finished
            state = NONE;

            // alert user of completion
            if (read_humidity_and_temp_callback) {
                // convert measurements
                uint16_t humidity = 0;
                si7021_calculate_humidity(&humidity);
                int16_t temperature = 0;
                si7021_calculate_temperature(&temperature);

                read_humidity_and_temp_callback(humidity, temperature);
            }
            break;

        case READ_TEMP_STARTED:
            // set next state
            state = READ_TEMP_READY;

            // delay until measurement is complete
            err_code = app_timer_start(si7021_timer, TEMP_CONVERSION_DELAY, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case READ_TEMP_READY:
            // set next state
            state = READ_TEMP_COMPLETE;

            // read data from sensor
            si7021_read_temp_data();
            break;

        case READ_TEMP_COMPLETE:
            // finished
            state = NONE;

            // alert user of completion
            if (read_temp_callback) {
                // convert measurement
                int16_t temperature = 0;
                si7021_calculate_temperature(&temperature);

                read_temp_callback(temperature);
            }
            break;

        case NONE:
            // nothing to do
            break;
    }
}

