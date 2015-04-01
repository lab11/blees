/* Send an advertisement periodically
 */

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "app_timer.h"
#include "ble_advdata.h"
#include "ble_debug_assert_handler.h"
#include "boards.h"
#include "led.h"
#include "nordic_common.h"
#include "nrf_gpio.h"
#include "softdevice_handler.h"
#include "twi_master.h"


#define IS_SRVC_CHANGED_CHARACT_PRESENT 0                                 /**< Include or not the service_changed characteristic. if not enabled, the server's database cannot be changed for the lifetime of the device*/
#define ADVERTISING_LED                 LED_0                             /**< Is on when device is advertising. */

#define USE_LEDS                        1

#define APP_CFG_NON_CONN_ADV_TIMEOUT    0                                 /**< Time for which the device must be advertising in non-connectable mode (in seconds). 0 disables timeout. */
#define NON_CONNECTABLE_ADV_INTERVAL    MSEC_TO_UNITS(500, UNIT_0_625_MS)   /**< The advertising interval for non-connectable advertisement (100 ms). This value can vary between 100ms to 10.24s). */



// Insert manufacturing things into the advertisement
#define APP_COMPANY_IDENTIFIER          0x11BB                            /**< Company identifier I made up */

#define APP_BEACON_INFO_LENGTH          0x7                              /**< Total length of information advertised by the Beacon. */
#define APP_ADV_DATA_LENGTH             0x5                              /**< Length of manufacturer specific data in the advertisement. */
#define APP_DEVICE_TYPE                 0x01                              /**< Just chose 0x01 */
#define APP_BEACON_DATA                 70, 85, 90, 66                      // Temp byte, humidity byte, light byte, pressure byte

#define NEW_DEVICE_NAME                     "BLEES EECS 2334"               // 20 bytes between device name, beacon_data, and squall_id

#ifndef APP_SQUALL_ID
#define APP_SQUALL_ID                   0x42
#endif

// Sensor defines
#define LUX_ADDR                        0b01010010
#define TEMP_HUM_ADDR                   0b10000000
#define PRESSURE_ADDR                   0b10111000

// Read/Write def
#define TWI_READ                        0b1
#define TWI_WRITE                       0b0

// information about the advertisement
ble_advdata_t                           advdata;
static ble_gap_adv_params_t             m_adv_params;                     /**< Parameters to be passed to the stack when starting advertising. */

static struct {
    uint8_t temp;
    uint8_t humidity;
    uint8_t light;
    uint8_t pressure;
} m_sensor_info;

static uint8_t m_beacon_info[APP_BEACON_INFO_LENGTH] =                  /**< Information advertised by the Beacon. */
{
    APP_DEVICE_TYPE,
    APP_ADV_DATA_LENGTH,
    APP_BEACON_DATA
};

// Moves data from the m_sensor_info struct
// into the m_beacon_info array
// to set it up to send
void update_beacon_info()
{
    m_beacon_info[2] = m_sensor_info.temp;
    m_beacon_info[3] = m_sensor_info.humidity;
    m_beacon_info[4] = m_sensor_info.light;
    m_beacon_info[5] = m_sensor_info.pressure;

    uint32_t                  err_code;

    // Use the simplest send adv packets only mode
    uint8_t                   flags = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;
    // More manufacturing only stuff that might get added back in
    ble_advdata_manuf_data_t  manuf_specific_data;

    manuf_specific_data.company_identifier = APP_COMPANY_IDENTIFIER;
    manuf_specific_data.data.p_data        = (uint8_t *) m_beacon_info;
    manuf_specific_data.data.size          = APP_BEACON_INFO_LENGTH;

    advdata.p_manuf_specific_data   = &manuf_specific_data;
    advdata.flags.size              = sizeof(flags);
    advdata.flags.p_data            = &flags;

    err_code = ble_advdata_set(&advdata, NULL);
    APP_ERROR_CHECK(err_code);
}

void run_after_timer() {}

/**@brief Function for error handling, which is called when an error has occurred.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of error.
 *
 * @param[in] error_code  Error code supplied to the handler.
 * @param[in] line_num    Line number where the handler is called.
 * @param[in] p_file_name Pointer to the file name.
 */
void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    // This call can be used for debug purposes during application development.
    // @note CAUTION: Activating this code will write the stack to flash on an error.
    //                This function should NOT be used in a final product.
    //                It is intended STRICTLY for development/debugging purposes.
    //                The flash write will happen EVEN if the radio is active, thus interrupting
    //                any communication.
    //                Use with care. Un-comment the line below to use.
    // ble_debug_assert_handler(error_code, line_num, p_file_name);

    // On assert, the system can only recover on reset.
    NVIC_SystemReset();
}


/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}

// Setup TX power and the device name
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_sec_mode_t sec_mode;

    // Set the power. Using really low (-30) doesn't really work
    sd_ble_gap_tx_power_set(4);

    // Make the connection open (no security)
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    // Set the name of the device so its easier to find
    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) NEW_DEVICE_NAME,
                                          strlen(NEW_DEVICE_NAME));
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void advertising_init(void)
{
    uint32_t                  err_code;

    // Use the simplest send adv packets only mode
    uint8_t                   flags = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;

    // More manufacturing only stuff that might get added back in
    ble_advdata_manuf_data_t  manuf_specific_data;

    manuf_specific_data.company_identifier = APP_COMPANY_IDENTIFIER;
    manuf_specific_data.data.p_data        = (uint8_t *) m_beacon_info;
    manuf_specific_data.data.size          = APP_BEACON_INFO_LENGTH;

    // Build and set advertising data.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.flags.size              = sizeof(flags);
    advdata.flags.p_data            = &flags;
    advdata.p_manuf_specific_data   = &manuf_specific_data;

    err_code = ble_advdata_set(&advdata, NULL);
    APP_ERROR_CHECK(err_code);

    // Initialize advertising parameters (used when starting advertising).
    memset(&m_adv_params, 0, sizeof(m_adv_params));

    m_adv_params.type        = BLE_GAP_ADV_TYPE_ADV_NONCONN_IND;
    m_adv_params.p_peer_addr = NULL;                             // Undirected advertisement.
    m_adv_params.fp          = BLE_GAP_ADV_FP_ANY;
    m_adv_params.interval    = NON_CONNECTABLE_ADV_INTERVAL;
    m_adv_params.timeout     = APP_CFG_NON_CONN_ADV_TIMEOUT;
}


/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
    uint32_t err_code;

    err_code = sd_ble_gap_adv_start(&m_adv_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{

    // Initialize the SoftDevice handler module.
    // Use a really crappy clock because we want fast start
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_RC_250_PPM_8000MS_CALIBRATION, false);

    // Enable BLE stack
    uint32_t err_code;
    ble_enable_params_t ble_enable_params;
    memset(&ble_enable_params, 0, sizeof(ble_enable_params));
    ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
    err_code = sd_ble_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for doing power management.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}

/**@brief init sensor data structures and sensors
 */
static void sensors_init(void) {
    m_sensor_info.temp = 0;
    m_sensor_info.humidity = 0;
    m_sensor_info.pressure = 0;
    m_sensor_info.light = 0;
    
    bool ret = twi_master_init();
    //if (ret == false) {
    //    //do something
    //}
    
    // Init temp and humidity sensor
    uint8_t temp_hum_data = 0b11111110;
    twi_master_transfer(
            TEMP_HUM_ADDR | TWI_WRITE,
            &temp_hum_data,
            sizeof(uint8_t),
            TWI_ISSUE_STOP
    );
    
    // Init pressure sensor
    uint8_t pressure_data[] = {0x20, 0b10010100};
    twi_master_transfer(
            PRESSURE_ADDR | TWI_WRITE,
            pressure_data,
            sizeof(pressure_data),
            TWI_ISSUE_STOP
    );
    
    // Init light sensor
    uint8_t lux_data[] = {0b11000000, 0b00000011};
    twi_master_transfer(
            LUX_ADDR | TWI_WRITE,
            lux_data,
            sizeof(lux_data),
            TWI_ISSUE_STOP
    );
}

/**@brief get sensor data and update m_sensor_info
 */
static void get_sensor_data() {
    //Temperature
    uint8_t temp_hum_write[] = {0xF3};
    twi_master_transfer(
            TEMP_HUM_ADDR | TWI_WRITE,
            temp_hum_write,
            sizeof(temp_hum_write),
            TWI_DONT_ISSUE_STOP
    );
    uint8_t temp_hum_data[3];
    while (
        !twi_master_transfer(
                TEMP_HUM_ADDR | TWI_READ,
                temp_hum_data,
                sizeof(temp_hum_data),
                TWI_ISSUE_STOP
        )) {for(int i = 0; i < 10000; ++i) {}}

    float temperature = -46.85 + (175.72 * (((uint32_t) temp_hum_data[0] << 8) | ((uint32_t) temp_hum_data[1] & 0xfc)) / (1 << 16));
    int int_temperature = (int) temperature;
    m_sensor_info.temp = (uint8_t) int_temperature;

    //Humidity
    temp_hum_write[0] = 0xF5;
    twi_master_transfer(
            TEMP_HUM_ADDR | TWI_WRITE,
            temp_hum_write,
            sizeof(temp_hum_write),
            TWI_DONT_ISSUE_STOP
    );
    while (!twi_master_transfer(
            TEMP_HUM_ADDR | TWI_READ,
            temp_hum_data,
            sizeof(temp_hum_data),
            TWI_ISSUE_STOP
    )) {for (int i = 0; i < 10000; ++i) {}}

    float humidity = -6.0 + ((125.0 / (1 << 16)) * (((uint32_t) temp_hum_data[0] << 8) | ((uint32_t) temp_hum_data[1] & 0xf0)));
    int int_humidity = (int) humidity;
    m_sensor_info.humidity = (uint8_t) int_humidity;

    //Pressure
    uint8_t pressure_write[] = {0xA8};
    twi_master_transfer(
            PRESSURE_ADDR | TWI_WRITE,
            pressure_write,
            sizeof(pressure_write),
            TWI_DONT_ISSUE_STOP
    );
    uint8_t pressure_data[3];
    twi_master_transfer(
            PRESSURE_ADDR | TWI_READ,
            pressure_data,
            sizeof(pressure_data),
            TWI_ISSUE_STOP
    );
   
    float pressure =    (0x00FFFFFF & (((uint32_t)pressure_data[2] << 16) |
                        ((uint32_t) pressure_data[1] << 8) |
                        ((uint32_t) pressure_data[0]))) / 40960.0;
//    m_sensor_info.temp = pressure_data[2];
//    m_sensor_info.humidity = pressure_data[1];
//    m_sensor_info.light = pressure_data[0];
    int int_pressure = (int) pressure;

    m_sensor_info.pressure = (uint8_t) int_pressure;

    // LIGHT
    uint8_t chan0_low_byte[] = {0b10101100};
    uint8_t chan0_output[] = {0x00, 0x00};
    uint8_t chan0_output_high[] = {0x00};
    uint8_t chan0_output_low[] = {0x00};
    
    twi_master_transfer
    (
        LUX_ADDR | TWI_WRITE,
        chan0_low_byte,
        sizeof(chan0_low_byte),
        TWI_DONT_ISSUE_STOP
    );
    
    twi_master_transfer
    (
        LUX_ADDR | TWI_READ,
        chan0_output_low,
        sizeof(chan0_output_low),
        TWI_ISSUE_STOP
    );
    
    uint8_t chan0_high_byte[] = {0b10101101};
    
    twi_master_transfer
    (
        LUX_ADDR | TWI_WRITE,
        chan0_high_byte,
        sizeof(chan0_high_byte),
        TWI_DONT_ISSUE_STOP
    );
    
    twi_master_transfer
    (
        LUX_ADDR | TWI_READ,
        chan0_output_high,
        sizeof(chan0_output_high),
        TWI_ISSUE_STOP
    );

    chan0_output[1] = chan0_output_low[0];
    chan0_output[0] = chan0_output_high[0];

    uint8_t chan1_low_byte[] = {0b10101110};
    uint8_t chan1_output[] = {0x00, 0x00};
    uint8_t chan1_output_high[] = {0x00};
    uint8_t chan1_output_low[] = {0x00};
    
    twi_master_transfer
    (
        LUX_ADDR | TWI_WRITE,
        chan1_low_byte,
        sizeof(chan1_low_byte),
        TWI_DONT_ISSUE_STOP
    );
    
    twi_master_transfer
    (
        LUX_ADDR | TWI_READ,
        chan1_output_low,
        sizeof(chan1_output_low),
        TWI_ISSUE_STOP
    );
    
    uint8_t chan1_high_byte[] = {0b10101111};
    
    twi_master_transfer
    (
        LUX_ADDR | TWI_WRITE,
        chan1_high_byte,
        sizeof(chan1_high_byte),
        TWI_DONT_ISSUE_STOP
    );
    
    twi_master_transfer
    (
        LUX_ADDR | TWI_READ,
        chan1_output_high,
        sizeof(chan1_output_high),
        TWI_ISSUE_STOP
    );

    chan1_output[1] = chan1_output_low[0];
    chan1_output[0] = chan1_output_high[0];

    // Conversion

    //m_sensor_info.temp = chan1_output[1];
    //m_sensor_info.humidity = chan1_output[0];
    //m_sensor_info.pressure = chan0_output[1];
    //m_sensor_info.light = chan0_output[0];

    uint16_t chan1 = ((((uint16_t) chan1_output[0]) << 8) | (0x00FF & (uint16_t) chan1_output[1]))/0.034;
    uint16_t chan0 = ((((uint16_t) chan0_output[0]) << 8) | (0x00FF & (uint16_t) chan0_output[1]))/0.034;

    float ratio = ((float) chan1) / chan0;

    float lux = 0.0;

    if (ratio <= 0.52) {
        lux = (0.0315 * chan0) - (0.0593 * chan0 * (pow(ratio, 1.4))); 
    } else if (ratio <= 0.65) {
        lux = (0.0229 * chan0) - (0.0291 * chan1); 
    } else if (ratio <= 0.80) {
        lux = (0.0157 * chan0) - (0.0180 * chan1); 
    } else if (ratio <= 1.3) {
        lux = (0.00338 * chan0) - (0.00260 * chan1); 
    } 

    m_sensor_info.light = (uint8_t) (lux / 10.0);
}

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    // Initialize.
    //platform_init();
    
    // Init timers
    APP_TIMER_INIT(0, 1, 1, 0);
    
    sensors_init();

    ble_stack_init();

    gap_params_init();

    advertising_init();

    // Start execution.
    advertising_start();

    //10-Sec Timer
    app_timer_id_t timer;
    app_timer_create(&timer, APP_TIMER_MODE_REPEATED, run_after_timer);
    app_timer_start(timer, APP_TIMER_TICKS(2000, 0), NULL);

    while (1) {
        m_sensor_info.temp += 1;
        get_sensor_data();
        
        // Update information sent with beacon
        update_beacon_info();

        // Reset beacon

        power_manage(); //sleep
    }
}

