/*
 * Advertises Sensor Data
 */

// Standard Libraries
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "math.h"

// Nordic Libraries
#include "app_util.h"
#include "app_error.h"
#include "app_gpiote.h"
#include "app_trace.h"
#include "app_scheduler.h"
#include "app_util_platform.h"
#include "app_timer.h"
#include "ble_conn_params.h"
#include "ble_hci.h"
#include "ble_hrs_c.h"
#include "ble_bas_c.h"
#include "ble_advdata.h"
#include "ble_gap.h"
#include "ble_gatts.h"
#include "ble_debug_assert_handler.h"
#include "ble.h"
#include "ble_db_discovery.h"
#include "ble_config.h"
#include "ble_ess.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_sdm.h"
#include "nrf_gpio.h"
#include "nrf_soc.h"
#include "nrf_drv_config.h"
#include "nrf_drv_gpiote.h"
#include "nrf_error.h"
#include "nrf_assert.h"
#include "nrf_drv_twi.h"
#include "pstorage.h"
#include "pstorage_platform.h"
#include "softdevice_handler.h"

// Platform, Peripherals, Devices, Services
#include "blees.h"
#include "simple_ble.h"
#include "simple_adv.h"
#include "multi_adv.h"
#include "eddystone.h"
#include "led.h"
#include "tsl2561.h"
#include "si7021.h"
#include "lps331ap.h"
#include "adxl362.h"
#include "spi_driver.h" // take this out eventually!

/*******************************************************************************
 *   DEFINES
 ******************************************************************************/

#define UMICH_COMPANY_IDENTIFIER      0x02E0
#define APP_BEACON_INFO_LENGTH        16
#define APP_BEACON_INFO_SERVICE_BLEES 0x12 // Registered to BLEES

#define ACCELEROMETER_INTERRUPT_PIN 5
#define LIGHT_INTERRUPT_PIN         22

/**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */
#define DEAD_BEEF                   0xDEADBEEF

#define UPDATE_RATE                 APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER)

//Initial Pressure Parameters
#define INIT_PRES_DATA              456
#define PRES_TRIGGER_CONDITION      TRIG_FIXED_INTERVAL
#define PRES_TRIGGER_VAL_OPERAND    470
#define PRES_TRIGGER_VAL_TIME       APP_TIMER_TICKS(3000, APP_TIMER_PRESCALER)

//Initial Humidity Parameters
#define INIT_HUM_DATA               789
#define HUM_TRIGGER_CONDITION       TRIG_FIXED_INTERVAL
#define HUM_TRIGGER_VAL_OPERAND     799
#define HUM_TRIGGER_VAL_TIME        APP_TIMER_TICKS(3000, APP_TIMER_PRESCALER)

//Initial Temperature Parameters
#define INIT_TEMP_DATA              123
#define TEMP_TRIGGER_CONDITION      TRIG_FIXED_INTERVAL
#define TEMP_TRIGGER_VAL_OPERAND    156
#define TEMP_TRIGGER_VAL_TIME       APP_TIMER_TICKS(3000, APP_TIMER_PRESCALER)

//Initial Lux Parameters
#define INIT_LUX_DATA               789
#define LUX_TRIGGER_CONDITION       TRIG_FIXED_INTERVAL
#define LUX_TRIGGER_VAL_OPERAND     799
#define LUX_TRIGGER_VAL_TIME        APP_TIMER_TICKS(6000, APP_TIMER_PRESCALER)

//Initial Acceleration Parameters
#define INIT_ACC_DATA               789
#define ACC_TRIGGER_CONDITION       TRIG_FIXED_INTERVAL
#define ACC_TRIGGER_VAL_OPERAND     799
#define ACC_TRIGGER_VAL_TIME        APP_TIMER_TICKS(60000, APP_TIMER_PRESCALER)


// Maximum size is 17 characters, counting URLEND if used
// Demo App (using j2x and cdn.rawgit.com)
#define PHYSWEB_URL     "j2x.us/2ImXWJ"
#define ADV_SWITCH_MS 1000

/*******************************************************************************
 *   STATIC AND GLOBAL VARIABLES
 ******************************************************************************/

uint8_t MAC_ADDR[6] = {0x00, 0x00, 0x30, 0xe5, 0x98, 0xc0};
nrf_drv_twi_t twi_instance = NRF_DRV_TWI_INSTANCE(1);

static ble_uuid_t ESS_SERVICE_UUID[] = {{ESS_UUID_SERVICE, BLE_UUID_TYPE_BLE}};

static ble_ess_t                    m_ess;
static uint8_t                      m_beacon_info[APP_BEACON_INFO_LENGTH];

APP_TIMER_DEF(m_pres_timer_id);     /**< ESS Pressure timer. */
APP_TIMER_DEF(m_hum_timer_id);      /**< ESS Humidity timer. */
APP_TIMER_DEF(m_temp_timer_id);     /**< ESS Temperature timer. */
APP_TIMER_DEF(m_lux_timer_id);      /**< ESS Lux timer. */
APP_TIMER_DEF(m_acc_timer_id);      /**< ESS Accelerometer timer. */

// Need this for the app_gpiote library
app_gpiote_user_id_t gpiote_user_acc;
app_gpiote_user_id_t gpiote_user_light;

static bool data_updated = false;

static struct {
    uint32_t pressure;
    uint16_t humidity;
    int16_t temp;
    uint16_t light;
    uint8_t acceleration;
    uint32_t sequence_num;
} m_sensor_info = {1.0f, 2.0f, 3.0f, 4.0f, 0.0f, 0};

// Intervals for advertising and connections
static simple_ble_config_t ble_config = {
    .platform_id       = 0x30,              // used as 4th octect in device BLE address
    .device_id         = DEVICE_ID_DEFAULT,
    .adv_name          = DEVICE_NAME,       // used in advertisements if there is room
    .adv_interval      = APP_ADV_INTERVAL,
    .min_conn_interval = MIN_CONN_INTERVAL,
    .max_conn_interval = MAX_CONN_INTERVAL,
};

/*******************************************************************************
 *   FUNCTION PROTOTYPES
 ******************************************************************************/
static void update_timers( ble_evt_t * p_ble_evt );

/*******************************************************************************
 *   HANDLERS AND CALLBACKS
 ******************************************************************************/

void ble_error(uint32_t error_code) {
    led_init(SQUALL_LED_PIN);
    led_on(SQUALL_LED_PIN);
    while(1);
}

void ble_evt_write(ble_evt_t* p_ble_evt) {
    update_timers(p_ble_evt);
}

void ble_evt_user_handler (ble_evt_t* p_ble_evt) {
    ble_ess_on_ble_evt(&m_ess, p_ble_evt);
}

static void acc_interrupt_handler (uint32_t pins_l2h, uint32_t pins_h2l) {
    if (pins_h2l & (1 << ACCELEROMETER_INTERRUPT_PIN)) {
        // High to low transition
        led_on(BLEES_LED_PIN);

        m_sensor_info.acceleration = 0x11;

        for (volatile int i = 0; i < 1000; i++);
        led_off(BLEES_LED_PIN);

        // stop timer if running
        app_timer_stop(m_acc_timer_id);

    } else if (pins_l2h & (1 << ACCELEROMETER_INTERRUPT_PIN)) {
        // Low to high transition
        m_sensor_info.acceleration = 0x01;

        // Call a one-shot timer here to set accel to low if it ever fires
        app_timer_start(m_acc_timer_id, (uint32_t) ACC_TRIGGER_VAL_TIME, NULL);
    }

    data_updated = true;
}

static void light_interrupt_handler (uint32_t pins_l2h, uint32_t pins_h2l) {
    if (pins_h2l & (1 << LIGHT_INTERRUPT_PIN)) {
        // High to low transition

        // Need to re-enable I2C to get reading from sensor
        nrf_drv_twi_enable(&twi_instance);
        // Get the lux value from the stored registers on the chip
        uint16_t lux = (uint16_t)tsl2561_read_lux();

        // Now turn the sensor back off and put it in low power mode
        tsl2561_off();
        // Stop I2C
        nrf_drv_twi_disable(&twi_instance);

        // Update our global state and update our advertisement.
        m_sensor_info.light = lux;
        data_updated = true;
/*
        //XXX: what is going on here?
        uint8_t lux_meas_val[2];
        memcpy(lux_meas_val, &lux, 2);

        // Update the service characteristic as well.
        uint32_t err_code = ble_ess_char_value_update(&m_ess, &(m_ess.lux), lux_meas_val,
                MAX_LUX_LEN, false, &(m_ess.lux_char_handles) );

        if ((err_code != NRF_SUCCESS) &&
            (err_code != NRF_ERROR_INVALID_STATE) &&
            (err_code != BLE_ERROR_NO_TX_BUFFERS) &&
            (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)) {

            APP_ERROR_HANDLER(err_code);
        }
*/
    }
}


static void gpio_init (void) {
    // Need two users: accelerometer and light
    APP_GPIOTE_INIT(2);

    // Register the accelerometer
    app_gpiote_user_register(&gpiote_user_acc,
            1<<ACCELEROMETER_INTERRUPT_PIN,   // Which pins we want the interrupt for low to high
            1<<ACCELEROMETER_INTERRUPT_PIN,   // Which pins we want the interrupt for high to low
            acc_interrupt_handler);

    // Register the light sensor interrupt
    uint32_t err = app_gpiote_user_register(&gpiote_user_light,
            0,                        // Which pins we want the interrupt for low to high
            1<<LIGHT_INTERRUPT_PIN,   // Which pins we want the interrupt for high to low
            light_interrupt_handler);
    APP_ERROR_CHECK(err);


    // Light sensor interrupt pin needs a pull up resistor
    nrf_gpio_cfg_input(LIGHT_INTERRUPT_PIN, NRF_GPIO_PIN_PULLUP);

    // Enable the interrupts!
    app_gpiote_user_enable(gpiote_user_acc);
    app_gpiote_user_enable(gpiote_user_light);
}

static void pres_take_measurement(void * p_context) {

    //lps331ap_power_on();

    UNUSED_PARAMETER(p_context);

    uint8_t  pres_meas_val[4];

    uint32_t meas;
    memset(&meas, 0, sizeof(meas));

    float pres;
    memset(&pres, 0, sizeof(pres));

    nrf_drv_twi_enable(&twi_instance);

    lps331ap_one_shot_enable();

    lps331ap_readPressure(&pres);

    //lps331ap_power_off();

    nrf_drv_twi_disable(&twi_instance);

    meas = (uint32_t)(pres * 1000);
    m_sensor_info.pressure = meas;
    data_updated = true;

    memcpy(pres_meas_val, &meas, 4);

    uint32_t err_code = ble_ess_char_value_update(&m_ess, &(m_ess.pressure), pres_meas_val,
            MAX_PRES_LEN, false, &(m_ess.pres_char_handles) );

    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != BLE_ERROR_NO_TX_BUFFERS) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)) {

        APP_ERROR_HANDLER(err_code);
    }
}

static void hum_take_measurement(void * p_context) {
    UNUSED_PARAMETER(p_context);

    uint8_t  hum_meas_val[2];

    uint32_t meas;
    memset(&meas, 0, sizeof(meas));

    float hum;
    memset(&hum, 0, sizeof(hum));

    nrf_drv_twi_enable(&twi_instance);

    (si7021_read_RH(&hum));

    nrf_drv_twi_disable(&twi_instance);

    meas = (uint16_t)(hum * 100);
    m_sensor_info.humidity = meas;
    data_updated = true;

    memcpy(hum_meas_val, &meas, 2);

    uint32_t err_code = ble_ess_char_value_update(&m_ess, &(m_ess.humidity), hum_meas_val,
            MAX_HUM_LEN, false, &(m_ess.hum_char_handles) );

    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != BLE_ERROR_NO_TX_BUFFERS) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)) {

        APP_ERROR_HANDLER(err_code);
    }
}

static void temp_take_measurement(void * p_context) {
    UNUSED_PARAMETER(p_context);

    uint8_t  temp_meas_val[2];

    uint32_t meas;
    memset(&meas, 0, sizeof(meas));

    float temp;
    memset(&temp, 0, sizeof(temp));

    nrf_drv_twi_enable(&twi_instance);

    si7021_read_temp(&temp);

    nrf_drv_twi_disable(&twi_instance);

    meas = (int16_t)(temp * 100);
    m_sensor_info.temp = meas;
    data_updated = true;

    memcpy(temp_meas_val, &meas, 2);

    uint32_t err_code = ble_ess_char_value_update(&m_ess, &(m_ess.temperature), temp_meas_val,
            MAX_TEMP_LEN, false, &(m_ess.temp_char_handles) );

    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != BLE_ERROR_NO_TX_BUFFERS) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)) {

        APP_ERROR_HANDLER(err_code);
    }
}

static void lux_take_measurement(void * p_context) {
    UNUSED_PARAMETER(p_context);
    nrf_drv_twi_enable(&twi_instance);
    tsl2561_on();
    tsl2561_config(tsl2561_GAIN_LOW, tsl2561_INTEGRATION_101MS);
    tsl2561_interrupt_enable();
    nrf_drv_twi_disable(&twi_instance);
}

static void acc_timeout_handler(void* p_context) {
    UNUSED_PARAMETER(p_context);

    m_sensor_info.acceleration = 0x00;
    data_updated = true;

    ble_ess_char_value_update(&m_ess, &(m_ess.acceleration), &(m_sensor_info.acceleration),
            MAX_ACC_LEN, false, &(m_ess.acc_char_handles));
}

/*******************************************************************************
 *   INIT FUNCTIONS
 ******************************************************************************/

static void timers_init(void) {
    uint32_t err_code;

    // Initialize timer for Pressure Trigger
    err_code = app_timer_create(&m_pres_timer_id,
            APP_TIMER_MODE_REPEATED,
            pres_take_measurement);
    APP_ERROR_CHECK(err_code);

    // Initialize timer for Humidity Trigger
    err_code = app_timer_create(&m_hum_timer_id,
            APP_TIMER_MODE_REPEATED,
            hum_take_measurement);
    APP_ERROR_CHECK(err_code);

    // Initialize timer for Temperature trigger
    err_code = app_timer_create(&m_temp_timer_id,
            APP_TIMER_MODE_REPEATED,
            temp_take_measurement);
    APP_ERROR_CHECK(err_code);

    // Initialize timer for Lux Trigger
    err_code = app_timer_create(&m_lux_timer_id,
            APP_TIMER_MODE_REPEATED,
            lux_take_measurement);
    APP_ERROR_CHECK(err_code);

    // Initialize timer for Acc Trigger
    err_code = app_timer_create(&m_acc_timer_id,
            APP_TIMER_MODE_SINGLE_SHOT,
            acc_timeout_handler);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the pressure.*/
static void pres_char_init(ble_ess_init_t * p_ess_init) {
    p_ess_init->pres_trigger_data.condition = PRES_TRIGGER_CONDITION;
    p_ess_init->init_pres_data = (uint32_t)(INIT_PRES_DATA);
    p_ess_init->pres_trigger_val_var = (uint32_t)(PRES_TRIGGER_VAL_OPERAND);
    p_ess_init->pres_trigger_data.time_interval = (uint32_t)(PRES_TRIGGER_VAL_TIME);
}

/**@brief Function for initializing the humidity.*/
static void hum_char_init(ble_ess_init_t * p_ess_init) {
    p_ess_init->hum_trigger_data.condition = HUM_TRIGGER_CONDITION;
    p_ess_init->init_hum_data = (uint16_t)(INIT_HUM_DATA);
    p_ess_init->hum_trigger_val_var = (uint16_t)(HUM_TRIGGER_VAL_OPERAND);
    p_ess_init->hum_trigger_data.time_interval = (uint32_t)(HUM_TRIGGER_VAL_TIME);
}

/**@brief Function for initializing the temperature.*/
static void temp_char_init(ble_ess_init_t * p_ess_init) {
    p_ess_init->temp_trigger_data.condition = TEMP_TRIGGER_CONDITION;
    p_ess_init->init_temp_data = (int16_t)(INIT_TEMP_DATA);
    p_ess_init->temp_trigger_val_var = (int16_t)(TEMP_TRIGGER_VAL_OPERAND);
    p_ess_init->temp_trigger_data.time_interval = (uint32_t)(TEMP_TRIGGER_VAL_TIME);
}

/**@brief Function for initializing lux.*/
static void lux_char_init(ble_ess_init_t * p_ess_init) {
    p_ess_init->lux_trigger_data.condition = LUX_TRIGGER_CONDITION;
    p_ess_init->init_lux_data = (uint16_t)(INIT_LUX_DATA);
    p_ess_init->lux_trigger_val_var = (uint16_t)(LUX_TRIGGER_VAL_OPERAND);
    p_ess_init->lux_trigger_data.time_interval = (uint32_t)(LUX_TRIGGER_VAL_TIME);
}

/**@brief Function for initializing the acceleration.*/
static void acc_char_init(ble_ess_init_t * p_ess_init) {
    p_ess_init->acc_trigger_data.condition = ACC_TRIGGER_CONDITION;
    p_ess_init->init_acc_data = (uint8_t)(INIT_ACC_DATA);
    p_ess_init->acc_trigger_val_var = (uint16_t)(ACC_TRIGGER_VAL_OPERAND);
    p_ess_init->acc_trigger_data.time_interval = (uint32_t)(ACC_TRIGGER_VAL_TIME);
}


/**@brief Function for initializing the sensor simulators.*/
static void sensors_init(void) {

    nrf_drv_twi_config_t twi_config;

    // Initialize the I2C module
    twi_config.sda                = I2C_SDA_PIN;
    twi_config.scl                = I2C_SCL_PIN;
    twi_config.frequency          = NRF_TWI_FREQ_400K;
    twi_config.interrupt_priority = APP_IRQ_PRIORITY_HIGH;

    nrf_drv_twi_init(&twi_instance, &twi_config, NULL, NULL);
    nrf_drv_twi_enable(&twi_instance);

    //initialize pressure
    lps331ap_init(&twi_instance);
    lps331ap_sw_reset();
    lps331ap_sw_reset_disable();
    lps331ap_power_off();
    lps331ap_config(lps331ap_MODE1, lps331ap_P_RES_10, lps331ap_T_RES_7);
    lps331ap_one_shot_config();
    lps331ap_amp_control(true);
    lps331ap_power_on();
    //lps331ap_power_off();

    //initialize humidity and temperature
    si7021_init(&twi_instance);
    si7021_reset();
    si7021_heater_off();

    //initialize lux
    tsl2561_driver_init(&twi_instance, 0b00101001);

    //initialize accelerometer
    adxl362_accelerometer_init(adxl362_NOISE_NORMAL, true, false, false);
    uint16_t act_thresh = 0x0070;
    adxl362_set_activity_threshold(act_thresh);
    uint16_t inact_thresh = 0x0096;
    adxl362_set_inactivity_threshold(inact_thresh);
    uint8_t a_time = 4;
    adxl362_set_activity_time(a_time);
    uint8_t ia_time = 30;
    adxl362_set_inactivity_time(ia_time);

    adxl362_interrupt_map_t intmap_2;

    //adxl362_interrupt_map_t intmap_1, intmap_2;
    /*
    intmap_1.DATA_READY = 0;
    intmap_1.FIFO_READY = 0;
    intmap_1.FIFO_WATERMARK = 0;
    intmap_1.FIFO_OVERRUN = 0;
    intmap_1.ACT = 1;
    intmap_1.INACT = 0;
    intmap_1.AWAKE = 0;
    intmap_1.INT_LOW = 1;
    adxl362_config_INTMAP(&intmap_1, true);
    */

    intmap_2.DATA_READY = 0;
    intmap_2.FIFO_READY = 0;
    intmap_2.FIFO_WATERMARK = 0;
    intmap_2.FIFO_OVERRUN = 0;
    intmap_2.ACT = 0;
    intmap_2.INACT = 0;
    intmap_2.AWAKE = 1;
    intmap_2.INT_LOW = 1;
    adxl362_config_INTMAP(&intmap_2, false);

    adxl362_config_interrupt_mode(adxl362_INTERRUPT_LOOP, true , true);
    adxl362_activity_inactivity_interrupt_enable();

    adxl362_read_status_reg();

    spi_disable();
    nrf_drv_twi_disable(&twi_instance);


}

static void on_ess_evt (ble_ess_t * p_ess, ble_ess_evt_t * p_evt) {
    // Nothing to do for now
}

// init services
void services_init (void) {

    uint32_t err_code;

    ble_ess_init_t ess_init;
    memset(&ess_init, 0 , sizeof(ess_init));

    ess_init.evt_handler = on_ess_evt;
    ess_init.is_notify_supported = true;

    pres_char_init(&ess_init);  //initialize pres
    hum_char_init(&ess_init);   //initialize hum
    temp_char_init(&ess_init);  //initialize temp
    lux_char_init(&ess_init);   //initialize lux
    acc_char_init(&ess_init);   //initialize acc

    err_code = ble_ess_init(&m_ess, &ess_init);
    APP_ERROR_CHECK(err_code);
}

void dfu_reset_prepare (void) {

    // Disable the interrupts!
    app_gpiote_user_disable(gpiote_user_acc);
    app_gpiote_user_disable(gpiote_user_light);

    // reset accelerometer
    spi_enable();
    adxl362_accelerometer_reset();
    spi_disable();

    // disable pressure
    lps331ap_power_off();

    // disable lux
    tsl2561_off();

    // disable i2c
    nrf_drv_twi_disable(&twi_instance);
}

/*******************************************************************************
 *   HELPER FUNCTIONS
 ******************************************************************************/

static void timers_start(void) {
    uint32_t err_code;

    err_code = app_timer_start(m_pres_timer_id, (uint32_t) PRES_TRIGGER_VAL_TIME, NULL);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_start(m_hum_timer_id, (uint32_t) HUM_TRIGGER_VAL_TIME, NULL);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_start(m_temp_timer_id, (uint32_t) TEMP_TRIGGER_VAL_TIME, NULL);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_start(m_lux_timer_id, (uint32_t) LUX_TRIGGER_VAL_TIME, NULL);
    APP_ERROR_CHECK(err_code);
}

static void update_timers( ble_evt_t * p_ble_evt ){

    ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    uint32_t meas_interval = 0;
    uint32_t err_code;

    if (p_evt_write->handle == m_ess.pressure.trigger_handle) {
        if ( (m_ess.pressure.trigger_val_cond == 0x01) || (m_ess.pressure.trigger_val_cond == 0x02) ){
            memcpy(&meas_interval, m_ess.pressure.trigger_val_buff + 1, 3);
            meas_interval = (uint32_t)APP_TIMER_TICKS(meas_interval, APP_TIMER_PRESCALER);
            app_timer_stop(m_pres_timer_id);
            err_code = app_timer_start(m_pres_timer_id, meas_interval, NULL);
            APP_ERROR_CHECK(err_code);
        }
    } else if (p_evt_write->handle == m_ess.humidity.trigger_handle) {
        if ( (m_ess.humidity.trigger_val_cond == 0x01) || (m_ess.humidity.trigger_val_cond == 0x02) ){
            memcpy(&meas_interval, m_ess.humidity.trigger_val_buff + 1, 3);
            meas_interval = (uint32_t)APP_TIMER_TICKS(meas_interval, APP_TIMER_PRESCALER);
            app_timer_stop(m_hum_timer_id);
            err_code = app_timer_start(m_hum_timer_id, meas_interval, NULL);
            APP_ERROR_CHECK(err_code);
        }
    } else if (p_evt_write->handle == m_ess.temperature.trigger_handle) {
        if ( (m_ess.temperature.trigger_val_cond == 0x01) || (m_ess.temperature.trigger_val_cond == 0x02) ){
            memcpy(&meas_interval, m_ess.temperature.trigger_val_buff + 1, 3);
            meas_interval = (uint32_t)APP_TIMER_TICKS(meas_interval, APP_TIMER_PRESCALER);
            app_timer_stop(m_temp_timer_id);
            err_code = app_timer_start(m_temp_timer_id, meas_interval, NULL);
            APP_ERROR_CHECK(err_code);
        }
    } else if (p_evt_write->handle == m_ess.lux.trigger_handle) {
        if ( (m_ess.lux.trigger_val_cond == 0x01) || (m_ess.lux.trigger_val_cond == 0x02) ){
            memcpy(&meas_interval, m_ess.lux.trigger_val_buff + 1, 3);
            meas_interval = (uint32_t)APP_TIMER_TICKS(meas_interval, APP_TIMER_PRESCALER);
            app_timer_stop(m_lux_timer_id);
            err_code = app_timer_start(m_lux_timer_id, meas_interval, NULL);
            APP_ERROR_CHECK(err_code);
        }
    } else if (p_evt_write->handle == m_ess.acceleration.trigger_handle) {
        if ((m_ess.acceleration.trigger_val_cond == 0x01) || (m_ess.acceleration.trigger_val_cond == 0x02)) {
            // we don't support changing accelerometer sample rate anymore
            /*
            memcpy(&meas_interval, m_ess.acceleration.trigger_val_buff + 1, 3);
            meas_interval = (uint32_t)APP_TIMER_TICKS(meas_interval, APP_TIMER_PRESCALER);
            app_timer_stop(m_acc_timer_id);
            err_code = app_timer_start(m_acc_timer_id, meas_interval, NULL);
            APP_ERROR_CHECK(err_code);
            */
        } else{
            memcpy(&meas_interval, m_ess.acceleration.trigger_val_buff + 1, 3);
            if (m_ess.acceleration.trigger_val_cond == 0x04) {
                uint8_t a_time = (uint8_t)(meas_interval);
                spi_enable();
                adxl362_set_activity_time(a_time);
                spi_disable();
            } else if (m_ess.acceleration.trigger_val_cond == 0x05) {
                uint16_t act_thresh = (uint16_t)(meas_interval);
                spi_enable();
                adxl362_set_activity_threshold(act_thresh);
                spi_disable();
            } else if (m_ess.acceleration.trigger_val_cond == 0x06) {
                uint8_t ia_time = (uint8_t)(meas_interval);
                spi_enable();
                adxl362_set_inactivity_time(ia_time);
                spi_disable();
            } else if (m_ess.acceleration.trigger_val_cond == 0x07) {
                uint16_t inact_thresh = (uint16_t)(meas_interval);
                spi_enable();
                adxl362_set_inactivity_threshold(inact_thresh);
                spi_disable();
            }
        }
    }

}

/*******************************************************************************
 *   Configure Advertisements
 ******************************************************************************/

static void adv_config_eddystone () {
    eddystone_adv(PHYSWEB_URL, NULL);
}

static void adv_config_data () {
    ble_advdata_manuf_data_t manuf_specific_data;

    // update sequence number only if data actually changed
    if (data_updated) {
        m_sensor_info.sequence_num++;
        data_updated = false;
    }

    // Register this manufacturer data specific data as the BLEES service
    m_beacon_info[0] = APP_BEACON_INFO_SERVICE_BLEES;
    memcpy(&m_beacon_info[1],  &m_sensor_info.pressure, 4);
    memcpy(&m_beacon_info[5],  &m_sensor_info.humidity, 2);
    memcpy(&m_beacon_info[7],  &m_sensor_info.temp, 2);
    memcpy(&m_beacon_info[9],  &m_sensor_info.light, 2);
    memcpy(&m_beacon_info[11], &m_sensor_info.acceleration, 1);
    memcpy(&m_beacon_info[12], &m_sensor_info.sequence_num, 4);

    memset(&manuf_specific_data, 0, sizeof(manuf_specific_data));
    manuf_specific_data.company_identifier = UMICH_COMPANY_IDENTIFIER;
    manuf_specific_data.data.p_data = (uint8_t*) m_beacon_info;
    manuf_specific_data.data.size   = APP_BEACON_INFO_LENGTH;

    simple_adv_service_manuf_data(ESS_SERVICE_UUID, &manuf_specific_data);
}

/*******************************************************************************
 *   MAIN LOOP
 ******************************************************************************/

int main(void) {

    // Setup BLE
    simple_ble_init(&ble_config);
    timers_init();
    gpio_init();
    led_init(BLEES_LED_PIN);

    // enable internal DC-DC converter to save power
    // BANISH THIS EVIL LINE
    // sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);

    // Need to init multi adv
    multi_adv_init(ADV_SWITCH_MS);

    // Now register our advertisement configure functions
    multi_adv_register_config(adv_config_eddystone);
    multi_adv_register_config(adv_config_data);

    // setup the sensor configurations
    sensors_init();

    // Start execution
    timers_start();

    // Start rotating the advertisements.
    multi_adv_start();

    while (1) {
        power_manage();
    }
}

