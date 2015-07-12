/* Send an advertisement periodically
 */

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "app_timer.h"
#include "app_util_platform.h"
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

#define APP_CFG_NON_CONN_ADV_TIMEOUT    0                                   /**< Time for which the device must be advertising in non-connectable mode (in seconds). 0 disables timeout. */
#define NON_CONNECTABLE_ADV_INTERVAL    MSEC_TO_UNITS(1600, UNIT_0_625_MS)  /**< The advertising interval is 1s. This value can vary between 100ms to 10.24s). */



// Insert manufacturing things into the advertisement
#define APP_COMPANY_IDENTIFIER          0xB1EE                            /**< Company identifier I made up that looks like BLEES */

#define APP_BEACON_INFO_LENGTH          16                                /**< Five sensors, four bytes each. */

#define TEMP_FLOAT                      1.0f
#define HUMIDITY_FLOAT                  2.0f
#define LIGHT_FLOAT                     3.0f
#define PRESSURE_FLOAT                  4.0f
#define ACCELRATION_FLOAT               5

#define TEMP_DATA                       1,   2,   3,   4
#define HUMIDITY_DATA                   5,   6,   7,   8
#define LIGHT_DATA                      9,   0xA, 0xB, 0xC
#define PRESSURE_DATA                   0xD, 0xE, 0xF, 0x10
#define ACCELERATION_DATA               0x11, 0x12

#define THIS_DEVICE_NAME                "BLEES"

// Sensor defines
#define LUX_ADDR                        0b01010010
#define TEMP_HUM_ADDR                   0b10000000
#define PRESSURE_ADDR                   0b10111000

// Read/Write def
#define TWI_READ                        0b1
#define TWI_WRITE                       0b0

// Pin selections
#define LED_PIN 25
#define SPI_SCK_PIN 9
#define SPI_MISO_PIN 10
#define SPI_MOSI_PIN 11
#define SPI_SS_PIN 4
#define NRF_SPI NRF_SPI0

app_timer_id_t timer;
uint32_t test_data = 0xDEADBEEF;

// Address for this node
uint8_t BLEES_ADDR[6] = {0x00, 0x00, 0x30, 0xe5, 0x98, 0xc0};

// information about the advertisement
ble_advdata_t                           advdata;
//ble_advdata_t                           srdata;
static ble_gap_adv_params_t             m_adv_params;                     /**< Parameters to be passed to the stack when starting advertising. */

static struct {
    float temp;
    float humidity;
    float light;
    float pressure;
    float acceleration;
} m_sensor_info = {1.0f, 2.0f, 3.0f, 4.0f, 5};

static uint8_t m_beacon_info[APP_BEACON_INFO_LENGTH] =                   /**< Information advertised by the Beacon. */
{
    TEMP_DATA,
    HUMIDITY_DATA,
    LIGHT_DATA,
    PRESSURE_DATA,
   // ACCELERATION_DATA,
};

// Moves data from the m_sensor_info struct
// into the m_beacon_info array
// to set it up to send
void update_beacon_info()
{
    uint32_t                  err_code;

    // Use the simplest send adv packets only mode
    uint8_t                   flags = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;

    // More manufacturing only stuff that might get added back in
    ble_advdata_manuf_data_t  manuf_specific_data;

    memcpy(&m_beacon_info[0],  &m_sensor_info.temp, 4);
    memcpy(&m_beacon_info[4],  &m_sensor_info.humidity, 4);
    memcpy(&m_beacon_info[8],  &m_sensor_info.light, 4);
    memcpy(&m_beacon_info[12], &m_sensor_info.pressure, 4);
    //memcpy(&m_beacon_info[16], &m_sensor_info.acceleration, 2);

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
}


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
                                          (const uint8_t *) THIS_DEVICE_NAME,
                                          strlen(THIS_DEVICE_NAME));
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
    uint32_t err_code;

    // Initialize advertising parameters (used when starting advertising).
    memset(&m_adv_params, 0, sizeof(m_adv_params));

    // m_adv_params.type        = BLE_GAP_ADV_TYPE_ADV_SCAN_IND;
    m_adv_params.type        = BLE_GAP_ADV_TYPE_ADV_NONCONN_IND;
    m_adv_params.p_peer_addr = NULL;                             // Undirected advertisement.
    m_adv_params.fp          = BLE_GAP_ADV_FP_ANY;
    m_adv_params.interval    = NON_CONNECTABLE_ADV_INTERVAL;
    m_adv_params.timeout     = APP_CFG_NON_CONN_ADV_TIMEOUT;

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

    // Set the address of this BLEES / squall
    {
        ble_gap_addr_t gap_addr;

        // Get the current original address
        sd_ble_gap_address_get(&gap_addr);

        // Set the new BLE address with the Michigan OUI
        gap_addr.addr_type = BLE_GAP_ADDR_TYPE_PUBLIC;
        memcpy(gap_addr.addr+2, BLEES_ADDR+2, sizeof(gap_addr.addr)-2);
        err_code = sd_ble_gap_address_set(BLE_GAP_ADDR_CYCLE_MODE_NONE, &gap_addr);
        APP_ERROR_CHECK(err_code);
    }
}

/**@brief Function for doing power management.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}

static void spi_write(uint8_t buf) {
    //clear the ready event
    NRF_SPI->EVENTS_READY = 0;

    NRF_SPI->TXD = buf;

    //wait until byte has transmitted
    while(NRF_SPI->EVENTS_READY == 0);

    uint8_t throw = NRF_SPI->RXD;

    NRF_SPI->EVENTS_READY = 0;
}

static void spi_read(uint8_t* buf) {

    //clear ready event
    NRF_SPI->EVENTS_READY = 0;

    NRF_SPI->TXD = 0x00;

    //wait until byte has been received
    while(NRF_SPI->EVENTS_READY == 0);

    buf[0] = NRF_SPI->RXD;

    NRF_SPI->EVENTS_READY = 0;
}

/**@brief init sensor data structures and sensors
 */
static void sensors_init(void) {
    m_sensor_info.temp = 0;
    m_sensor_info.humidity = 0;
    m_sensor_info.pressure = 0;
    m_sensor_info.light = 0;
    m_sensor_info.acceleration = 0;

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

    // initialize spi
    nrf_gpio_cfg_output(SPI_SS_PIN);
    nrf_gpio_pin_set(SPI_SS_PIN);
	NRF_SPI->PSELSCK 	= 	SPI_SCK_PIN;
	NRF_SPI->PSELMOSI 	= 	SPI_MOSI_PIN;
	NRF_SPI->PSELMISO 	= 	SPI_MISO_PIN;
	NRF_SPI->FREQUENCY	= 	SPI_FREQUENCY_FREQUENCY_M1;
	NRF_SPI->CONFIG	= 	(uint32_t)(SPI_CONFIG_CPHA_Leading << SPI_CONFIG_CPHA_Pos) |
						(SPI_CONFIG_CPOL_ActiveHigh << SPI_CONFIG_CPOL_Pos) |
						(SPI_CONFIG_ORDER_MsbFirst << SPI_CONFIG_ORDER_Pos);
	NRF_SPI->ENABLE = (SPI_ENABLE_ENABLE_Enabled << SPI_ENABLE_ENABLE_Pos);
	NRF_SPI->EVENTS_READY = 0;

    // send a soft reset to the accelerometer
    uint8_t accel_cmd[3] = {0x0A, 0x1F, 0x52};
    nrf_gpio_pin_clear(SPI_SS_PIN);
    for(int i=0; i<3; i++) {
        spi_write(accel_cmd[i]);
    }
    nrf_gpio_pin_set(SPI_SS_PIN);
    accel_cmd[1] = 0x2D;
    accel_cmd[2] = 0x02;
    nrf_gpio_pin_clear(SPI_SS_PIN);
    for(int i=0; i<3; i++) {
        spi_write(accel_cmd[i]);
    }
    nrf_gpio_pin_set(SPI_SS_PIN);
}

static void sample_accel() {
    nrf_gpio_pin_clear(SPI_SS_PIN);
    spi_write(0x0B);
    spi_write(0x0E);
    uint8_t device_id = 42;
    spi_read(&device_id);
    nrf_gpio_pin_set(SPI_SS_PIN);
    m_sensor_info.pressure = device_id;
}

/**@brief get sensor data and update m_sensor_info
 */
static void get_sensor_data() {
    //*** TURN ON SLEEPING SENSORS ***//
    // Init pressure sensor
    uint8_t pressure_config_data[] = {0x20, 0b10010100};
    twi_master_transfer(
            PRESSURE_ADDR | TWI_WRITE,
            pressure_config_data,
            sizeof(pressure_config_data),
            TWI_ISSUE_STOP
    );

    //*** TAKE MEASUREMENTS ***//
    //Temperature
    uint8_t temp_hum_write[] = {0xF3};
    twi_master_transfer(
            TEMP_HUM_ADDR | TWI_WRITE,
            temp_hum_write,
            sizeof(temp_hum_write),
            TWI_DONT_ISSUE_STOP
    );
    uint8_t temp_hum_data[3] = {0, 0, 0};
    while (
        !twi_master_transfer(
                TEMP_HUM_ADDR | TWI_READ,
                temp_hum_data,
                sizeof(temp_hum_data),
                TWI_ISSUE_STOP
        )) {for(int i = 0; i < 10000; ++i) {}}

    float temperature = -46.85 + (175.72 * (((uint32_t) temp_hum_data[0] << 8) | ((uint32_t) temp_hum_data[1] & 0xfc)) / (1 << 16));
    m_sensor_info.temp = temperature;

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
    m_sensor_info.humidity = humidity;

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
                        ((uint32_t) pressure_data[0]))) / 4096.0;
    m_sensor_info.pressure = pressure;

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

    //*** TURN OFF SLEEPABLE SENSORS ***//
    // Shut off pressure sensor
    pressure_config_data[0] = 0x20;
    pressure_config_data[1] = 0b00010100;
    twi_master_transfer(
            PRESSURE_ADDR | TWI_WRITE,
            pressure_config_data,
            sizeof(pressure_config_data),
            TWI_ISSUE_STOP
    );

    //*** Conversion ***//
    uint16_t chan1 = ((((uint16_t) chan1_output[0]) << 8) | (0x00FF & (uint16_t) chan1_output[1]))/0.034;
    uint16_t chan0 = ((((uint16_t) chan0_output[0]) << 8) | (0x00FF & (uint16_t) chan0_output[1]))/0.034;

    float ratio = ((float) chan1) / chan0;

    float lux = 0.0;

    if (ratio <= 0.50) {
        lux = (0.0304 * chan0) - (0.062 * chan0 * (pow(ratio, 1.4)));
    } else if (ratio <= 0.61) {
        lux = (0.0224 * chan0) - (0.031 * chan1);
    } else if (ratio <= 0.80) {
        lux = (0.0128 * chan0) - (0.0153 * chan1);
    } else if (ratio <= 1.3) {
        lux = (0.00146 * chan0) - (0.00112 * chan1);
    }

    m_sensor_info.light = lux;
}

void run_after_timer() {
    get_sensor_data();
    sample_accel();
    nrf_gpio_pin_toggle(LED_PIN);

    // Update information sent with beacon
    update_beacon_info();
    app_timer_start(timer, APP_TIMER_TICKS(1000, 0), NULL);
}

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    // Init timers
    APP_TIMER_INIT(0, 1, 1, 0);

    nrf_gpio_pin_clear(LED_PIN);
    nrf_gpio_pin_set(SPI_SS_PIN);
    nrf_gpio_cfg_output(LED_PIN);
    nrf_gpio_cfg_output(SPI_SS_PIN);

    sensors_init();

    ble_stack_init();

    gap_params_init();

    // Initialize this
    get_sensor_data();
    sample_accel();
    update_beacon_info();

    // Start execution.
    advertising_start();

    //10-Sec Timer
    app_timer_create(&timer, APP_TIMER_MODE_SINGLE_SHOT, run_after_timer);
    app_timer_start(timer, APP_TIMER_TICKS(1000, 0), NULL);


    while (1) {
        power_manage(); //sleep
    }
}

