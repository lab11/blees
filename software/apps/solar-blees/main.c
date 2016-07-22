// BLEES-style sensor collection on a solar panel!

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
#include "app_twi.h"
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
#include "si7021.h"
//#include "tsl2561.h"
//#include "lps331ap.h"
//#include "adxl362.h"
//#include "spi_driver.h" // take this out eventually!


/*******************************************************************************
 *   DEFINES
 ******************************************************************************/

#define UMICH_COMPANY_IDENTIFIER      0x02E0
#define APP_BEACON_INFO_LENGTH        16
#define APP_BEACON_INFO_SERVICE_BLEES 0x12 // Registered to BLEES

#define ACCELEROMETER_INTERRUPT_PIN 5
#define LIGHT_INTERRUPT_PIN         22

// Maximum size is 17 characters, counting URLEND if used
// Demo App (using j2x and cdn.rawgit.com)
#define PHYSWEB_URL     "j2x.us/2ImXWJ"


/*******************************************************************************
 *   STATIC AND GLOBAL VARIABLES
 ******************************************************************************/

static app_twi_t twi_instance = APP_TWI_INSTANCE(1);

static ble_uuid_t ESS_SERVICE_UUID[] = {{ESS_UUID_SERVICE, BLE_UUID_TYPE_BLE}};

static uint8_t m_beacon_info[APP_BEACON_INFO_LENGTH];

static bool data_updated = false;

static struct {
    uint32_t pressure;
    uint16_t humidity;
    int16_t temp;
    uint16_t light;
    uint8_t acceleration;
    uint32_t sequence_num;
} m_sensor_info = {0};

static struct {
    bool pressure;
    bool temperature;
    bool humidity;
    bool light;
} sensor_status = {0};

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

static void temp_finish_measurement (uint16_t, int16_t);
static void check_if_adv_ready (void);
static void adv_sensor_data (void);


/*******************************************************************************
 *   HANDLERS AND CALLBACKS
 ******************************************************************************/

void ble_error(uint32_t error_code) {
    led_init(SQUALL_LED_PIN);
    led_on(SQUALL_LED_PIN);
    while(1);
}

static void temp_start_measurement (void) {
    si7021_read_humidity_and_temp(temp_finish_measurement);
}

static void temp_finish_measurement (uint16_t humidity, int16_t temperature) {

    m_sensor_info.temp = temperature;
    m_sensor_info.humidity = humidity;
    data_updated = true;
    sensor_status.temperature = true;
    sensor_status.humidity = true;

    // begin advertising if all data is collected
    check_if_adv_ready();
}

static void check_if_adv_ready (void) {
    if (sensor_status.temperature &&
            sensor_status.humidity) {

        adv_sensor_data();

        //XXX: testing
        nrf_gpio_pin_clear(6);
    }
}

static void adv_sensor_data (void) {
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
 *   INIT FUNCTIONS
 ******************************************************************************/

static void i2c_init (void) {

    // Initialize the I2C module
    nrf_drv_twi_config_t twi_config;
    twi_config.sda                = I2C_SDA_PIN;
    twi_config.scl                = I2C_SCL_PIN;
    twi_config.frequency          = NRF_TWI_FREQ_400K;
    twi_config.interrupt_priority = APP_IRQ_PRIORITY_LOW;

    //XXX: 5 is arbitrary!!
    uint32_t err_code;
    APP_TWI_INIT(&twi_instance, &twi_config, 5, err_code);
    APP_ERROR_CHECK(err_code);

}

static void sensors_init_and_start (void) {

    // initialize humidity and temperature
    //  samples then sensor once init is complete
    si7021_init(&twi_instance, temp_start_measurement);
}


/*******************************************************************************
 *   MAIN LOOP
 ******************************************************************************/

int main (void) {

    //XXX: testing
    nrf_gpio_cfg_output(6);
    nrf_gpio_pin_set(6);

    // Setup BLE
    simple_ble_init(&ble_config);

    // setup the sensor configurations and start sampling
    i2c_init();
    sensors_init_and_start();

    while (1) {
        power_manage();
    }
}

