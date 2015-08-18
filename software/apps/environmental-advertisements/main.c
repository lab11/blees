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
#include "app_trace.h"
#include "app_scheduler.h"
#include "app_util_platform.h"
#include "app_timer.h"
#include "ble_advdata_parser.h"
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
#include "device_manager.h"
#include "device_manager_cnfg.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_sdm.h"
#include "nrf_gpio.h"
#include "nrf_soc.h"
#include "nrf_gpiote.h"
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
#include "led.h"
#include "tsl2561.h"
#include "si7021.h"
#include "lps331ap.h"
#include "adxl362.h"

/*******************************************************************************
 *   DEFINES
 ******************************************************************************/

#define APP_BEACON_INFO_LENGTH      11

#define PIN_IN                      5

#define APP_GPIOTE_MAX_USERS        1                                              /**< Maximum number of users of the GPIOTE handler. */

#define DEAD_BEEF                   0xDEADBEEF                                     /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define UPDATE_RATE                 APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER)

//Initial Pressure Parameters
#define INIT_PRES_DATA              456
#define PRES_TRIGGER_CONDITION      TRIG_FIXED_INTERVAL
#define PRES_TRIGGER_VAL_OPERAND    470
#define PRES_TRIGGER_VAL_TIME       APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER)

//Initial Humidity Parameters
#define INIT_HUM_DATA               789
#define HUM_TRIGGER_CONDITION       TRIG_FIXED_INTERVAL
#define HUM_TRIGGER_VAL_OPERAND     799
#define HUM_TRIGGER_VAL_TIME        APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER)

//Initial Temperature Parameters
#define INIT_TEMP_DATA              123
#define TEMP_TRIGGER_CONDITION      TRIG_FIXED_INTERVAL
#define TEMP_TRIGGER_VAL_OPERAND    156
#define TEMP_TRIGGER_VAL_TIME       APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER)

//Initial Lux Parameters
#define INIT_LUX_DATA               789
#define LUX_TRIGGER_CONDITION       TRIG_FIXED_INTERVAL
#define LUX_TRIGGER_VAL_OPERAND     799
#define LUX_TRIGGER_VAL_TIME        APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER)

//Initial Acceleration Parameters
#define INIT_ACC_DATA               789
#define ACC_TRIGGER_CONDITION       TRIG_FIXED_INTERVAL
#define ACC_TRIGGER_VAL_OPERAND     799
#define ACC_TRIGGER_VAL_TIME        APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER)


/*******************************************************************************
 *   STATIC AND GLOBAL VARIABLES
 ******************************************************************************/

uint8_t MAC_ADDR[6] = {0x00, 0x00, 0x30, 0xe5, 0x98, 0xc0};
nrf_drv_twi_t twi_instance = NRF_DRV_TWI_INSTANCE(1);

static ble_app_t                    app;
static ble_gap_adv_params_t         m_adv_params;
static ble_advdata_t                advdata;
static ble_ess_t                    m_ess;
static uint8_t                      m_beacon_info[APP_BEACON_INFO_LENGTH];
static uint16_t                     m_conn_handle = BLE_CONN_HANDLE_INVALID;     /**< Handle of the current connection. */

static app_timer_id_t               m_pres_timer_id;                             /**< ESS Pressure timer. */
static app_timer_id_t               m_hum_timer_id;                              /**< ESS Humidity timer. */
static app_timer_id_t               m_temp_timer_id;                             /**< ESS Temperature timer. */
static app_timer_id_t               m_lux_timer_id;                              /**< ESS Lux timer. */
static app_timer_id_t               m_acc_timer_id;                              /**< ESS Accelerometer timer. */

static bool                         m_ess_updating_advdata = false;
static bool                         switch_acc = false;

// Security requirements for this application.
static ble_gap_sec_params_t m_sec_params = 
{
    SEC_PARAM_BOND,
    SEC_PARAM_MITM,
    SEC_PARAM_IO_CAPABILITIES,
    SEC_PARAM_OOB,
    SEC_PARAM_MIN_KEY_SIZE,
    SEC_PARAM_MAX_KEY_SIZE,
};

static struct {
    uint32_t pressure;
    uint16_t humidity;
    int16_t temp;
    uint16_t light;
    uint8_t acceleration;
} m_sensor_info = {1.0f, 2.0f, 3.0f, 4.0f, 0.0f};

/*******************************************************************************
 *   FUNCTION PROTOTYPES
 ******************************************************************************/
static void advertising_start(void);
static bool update_advdata(void);
static void update_timers( ble_evt_t * p_ble_evt );

/*******************************************************************************
 *   HANDLERS AND CALLBACKS
 ******************************************************************************/

/**@brief Function for error handling, which is called when an error has occurred.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of error.
 *
 * @param[in] error_code  Error code supplied to the handler.
 * @param[in] line_num    Line number where the handler is called.
 * @param[in] p_file_name Pointer to the file name.
 */
void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name) {
    // APPL_LOG("[APPL]: ASSERT: %s, %d, error 0x%08x\r\n", p_file_name, line_num, error_code);
    // nrf_gpio_pin_set(ASSERT_LED_PIN_NO);

    // This call can be used for debug purposes during development of an application.
    // @note CAUTION: Activating this code will write the stack to flash on an error.
    //                This function should NOT be used in a final product.
    //                It is intended STRICTLY for development/debugging purposes.
    //                The flash write will happen EVEN if the radio is active, thus interrupting
    //                any communication.
    //                Use with care. Un-comment the line below to use.
    // ble_debug_assert_handler(error_code, line_num, p_file_name);

    // On assert, the system can only recover with a reset.
    //NVIC_SystemReset();

    led_off(BLEES_LED_PIN);
    //while(1);
    ble_debug_assert_handler(error_code, line_num, p_file_name);

}

/**@brief Function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num     Line number of the failing ASSERT call.
 * @param[in] p_file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name) {
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}

// service error callback
static void service_error_handler(uint32_t nrf_error) {
    APP_ERROR_HANDLER(nrf_error);
}

// connection parameters event handler callback
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt) {
    uint32_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED) {
        err_code = sd_ble_gap_disconnect(app.conn_handle,
                                         BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}

// connection parameters error callback
static void conn_params_error_handler(uint32_t nrf_error) {
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt) {
    uint32_t                         err_code;

    switch (p_ble_evt->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED:
            app.conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            //advertising_stop();
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            app.conn_handle = BLE_CONN_HANDLE_INVALID;
            advertising_start();
            break;

        case BLE_GATTS_EVT_WRITE:
            update_timers(p_ble_evt);
            break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            err_code = sd_ble_gap_sec_params_reply(app.conn_handle,
                    BLE_GAP_SEC_STATUS_SUCCESS, &m_sec_params, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            err_code = sd_ble_gatts_sys_attr_set(app.conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_AUTH_STATUS:
            break;

        case BLE_GAP_EVT_SEC_INFO_REQUEST:
            // No keys found for this device.
            err_code = sd_ble_gap_sec_info_reply(app.conn_handle, NULL, NULL, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_TIMEOUT:
            if (p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_ADVERTISING) {
                err_code = sd_power_system_off();
                APP_ERROR_CHECK(err_code);
            }
            break;
        case BLE_GATTS_EVT_TIMEOUT:
            if (p_ble_evt->evt.gatts_evt.params.timeout.src == BLE_GATT_TIMEOUT_SRC_PROTOCOL)
            {
                err_code = sd_ble_gap_disconnect(m_conn_handle,
                                                 BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
                APP_ERROR_CHECK(err_code);
            }
            break;    
        default:
            break;
    }
}

void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{

    //If high to low transition
    if (nrf_gpio_pin_read(PIN_IN) == 0){

        m_sensor_info.acceleration = 0x11;

        led_on(BLEES_LED_PIN);

        while( !(update_advdata()) ){
            m_sensor_info.acceleration = 0x11;
        };

        adxl362_read_status_reg();

        nrf_gpio_pin_clear(PIN_IN);
        switch_acc = true;

    }
    //If low to high transition
    else {

        m_sensor_info.acceleration = 0x01;

        while( !(update_advdata()) ){
            m_sensor_info.acceleration = 0x01;
        };

        led_off(BLEES_LED_PIN);

    }


}

static void gpio_init(void)
{
    led_off(BLEES_LED_PIN);

    ret_code_t err_code;
    err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);

    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);

    err_code = nrf_drv_gpiote_in_init(PIN_IN, &in_config, in_pin_handler);
    APP_ERROR_CHECK(err_code);

    nrf_gpio_pin_clear(PIN_IN);
    nrf_drv_gpiote_in_event_enable(PIN_IN, true);
    nrf_gpio_cfg_input(PIN_IN, NRF_GPIO_PIN_NOPULL);

    sd_nvic_SetPriority(GPIOTE_IRQn, 3); 

    NVIC_EnableIRQ(GPIOTE_IRQn);

    NRF_GPIOTE->CONFIG[0] =  (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos)
              | (PIN_IN << GPIOTE_CONFIG_PSEL_Pos)
              | (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos);
    NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_IN0_Set << GPIOTE_INTENSET_IN0_Pos;

    led_on(BLEES_LED_PIN);

}

/**@brief Function for handling the ESS events.
 *
 * @details This function will be called for all ESS events which are passed to
 *          the application.
 *
 * @param[in]   p_ess   Environmental Sensing Service structure.
 * @param[in]   p_evt   Event received from the ESS.
 */
static void on_ess_evt(ble_ess_t * p_ess, ble_ess_evt_t * p_evt)
{
    /*
    switch (p_evt->evt_type)
    {
        case BLE_ESS_EVT_NOTIFICATION_ENABLED:
            break;
            
        case BLE_ESS_EVT_NOTIFICATION_CONFIRMED:
            break;
            
        default:
            // No implementation needed.
            break;
    }
    */
}

/**@brief Function for handling the Application's system events.
 *
 * @param[in]   sys_evt   system event.
 */
 
static void on_sys_evt(uint32_t sys_evt)
{
    /*
    switch(sys_evt)
    {
        case NRF_EVT_FLASH_OPERATION_SUCCESS:
        case NRF_EVT_FLASH_OPERATION_ERROR:
            
            if (m_memory_access_in_progress)
            {
                m_memory_access_in_progress = false;
                advertising_start();
            }
            break;
            
        default:
            // No implementation needed.
            break;
    }
    */
}

/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the scheduler in the main loop after a BLE stack event has
 *  been received.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    ble_ess_on_ble_evt(&m_ess, p_ble_evt);
    on_ble_evt(p_ble_evt);
    ble_conn_params_on_ble_evt(p_ble_evt);
}


/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in]   sys_evt   System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt) {
    on_sys_evt(sys_evt);

}

static void pres_take_measurement(void * p_context)
{
    lps331ap_one_shot_enable();

    UNUSED_PARAMETER(p_context);
        
    uint8_t  pres_meas_val[4];

    uint32_t meas;
    memset(&meas, 0, sizeof(meas));
    
    float pres;
    memset(&pres, 0, sizeof(pres));

    lps331ap_readPressure(&pres);

    meas = (uint32_t)(pres * 1000);
    m_sensor_info.pressure = meas;

    update_advdata();

    memcpy(pres_meas_val, &meas, 4);
    
    uint32_t err_code = ble_ess_char_value_update(&m_ess, &(m_ess.pressure), pres_meas_val, 
        MAX_PRES_LEN, false, &(m_ess.pres_char_handles) );
    
    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != BLE_ERROR_NO_TX_BUFFERS) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
        )
    {
        APP_ERROR_HANDLER(err_code);
    }
}

static void hum_take_measurement(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    
    uint8_t  hum_meas_val[2]; 

    uint32_t meas;
    memset(&meas, 0, sizeof(meas));

    float hum;
    memset(&hum, 0, sizeof(hum));

    (si7021_read_RH_hold(&hum));

    meas = (uint16_t)(hum * 100);
    m_sensor_info.humidity = meas;

    update_advdata();

    memcpy(hum_meas_val, &meas, 2);

    uint32_t err_code = ble_ess_char_value_update(&m_ess, &(m_ess.humidity), hum_meas_val, 
        MAX_HUM_LEN, false, &(m_ess.hum_char_handles) );
    
    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != BLE_ERROR_NO_TX_BUFFERS) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
        )
    {
        APP_ERROR_HANDLER(err_code);
    }
}

static void temp_take_measurement(void * p_context)
{
    UNUSED_PARAMETER(p_context);

    uint8_t  temp_meas_val[2]; 

    uint32_t meas;
    memset(&meas, 0, sizeof(meas));

    float temp;
    memset(&temp, 0, sizeof(temp));

    si7021_read_temp_hold(&temp);

    meas = (int16_t)(temp * 100);
    m_sensor_info.temp = meas;
    
    update_advdata();

    memcpy(temp_meas_val, &meas, 2);

    uint32_t err_code = ble_ess_char_value_update(&m_ess, &(m_ess.temperature), temp_meas_val, 
        MAX_TEMP_LEN, false, &(m_ess.temp_char_handles) );

    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != BLE_ERROR_NO_TX_BUFFERS) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
        )
    {
        APP_ERROR_HANDLER(err_code);
    }
}

static void lux_take_measurement(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    
    uint8_t  lux_meas_val[2]; 

    uint32_t meas;
    memset(&meas, 0, sizeof(meas));

    float lux;
    memset(&lux, 0, sizeof(lux));

    lux = tsl2561_readLux(tsl2561_MODE0);

    meas = (uint16_t)(lux);
    m_sensor_info.light = meas;

    update_advdata();

    memcpy(lux_meas_val, &meas, 2);

    uint32_t err_code = ble_ess_char_value_update(&m_ess, &(m_ess.lux), lux_meas_val, 
        MAX_LUX_LEN, false, &(m_ess.lux_char_handles) );
    
    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != BLE_ERROR_NO_TX_BUFFERS) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
        )
    {
        APP_ERROR_HANDLER(err_code);
    }
}

static void acc_take_measurement(void * p_context)
{
    
    UNUSED_PARAMETER(p_context);
    
    uint8_t  acc_meas_val[1]; 

    uint32_t meas;
    memset(&meas, 0, sizeof(meas));

    if (switch_acc){
        meas = m_sensor_info.acceleration & 0x11;
        switch_acc = false;
    }
    else {
        meas = m_sensor_info.acceleration & 0x10;
    }
    m_sensor_info.acceleration = meas;

    while( !update_advdata() );

    memcpy(acc_meas_val, &meas, 1);

    uint32_t err_code = ble_ess_char_value_update(&m_ess, &(m_ess.acceleration), acc_meas_val, 
        MAX_ACC_LEN, false, &(m_ess.acc_char_handles) );
    
    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != BLE_ERROR_NO_TX_BUFFERS) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
        )
    {
        APP_ERROR_HANDLER(err_code);
    }
}

/*******************************************************************************
 *   INIT FUNCTIONS
 ******************************************************************************/

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init (void) {
    uint32_t err_code;

    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_RC_250_PPM_8000MS_CALIBRATION,
            false);

    // Enable BLE stack
    ble_enable_params_t ble_enable_params;
    memset(&ble_enable_params, 0, sizeof(ble_enable_params));
    ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
    err_code = sd_ble_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);

    //Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);

    // Set the MAC address of the device
    {
        ble_gap_addr_t gap_addr;

        // Get the current original address
        sd_ble_gap_address_get(&gap_addr);

        // Set the new BLE address with the Michigan OUI
        gap_addr.addr_type = BLE_GAP_ADDR_TYPE_PUBLIC;
        memcpy(gap_addr.addr+2, MAC_ADDR+2, sizeof(gap_addr.addr)-2);
        err_code = sd_ble_gap_address_set(BLE_GAP_ADDR_CYCLE_MODE_NONE,
                &gap_addr);
        APP_ERROR_CHECK(err_code);
    }
}

// gap name/appearance/connection parameters
static void gap_params_init (void) {
    uint32_t                err_code;
    ble_gap_conn_sec_mode_t sec_mode;
    ble_gap_conn_params_t   gap_conn_params;

    // Full strength signal
    sd_ble_gap_tx_power_set(4);

    // Let anyone connect and set the name given the platform
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
    err_code = sd_ble_gap_device_name_set(&sec_mode,
            (const uint8_t *)DEVICE_NAME, strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    // Not sure what this is useful for, but why not set it
    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_GENERIC_COMPUTER);
    APP_ERROR_CHECK(err_code);

    // Specify parameters for a connection
    memset(&gap_conn_params, 0, sizeof(gap_conn_params));
    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

// initialize advertising
static void advertising_init(void) {
    memset(&m_adv_params, 0, sizeof(m_adv_params));
    m_adv_params.type               = BLE_GAP_ADV_TYPE_ADV_IND;
    m_adv_params.p_peer_addr        = NULL;
    m_adv_params.fp                 = BLE_GAP_ADV_FP_ANY;
    m_adv_params.interval           = APP_ADV_INTERVAL;
    m_adv_params.timeout            = APP_ADV_TIMEOUT_IN_SECONDS;

    volatile uint32_t      err_code;
    ble_advdata_t advdata;
    uint8_t flags = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;
    
    // Build and set advertising data
    memset(&advdata, 0, sizeof(advdata));
    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = true;
    advdata.flags = flags;
    
    err_code = ble_advdata_set(&advdata, NULL);
    APP_ERROR_CHECK(err_code);
}

// Initialize connection parameters
static void conn_params_init(void) {
    uint32_t                err_code;
    ble_conn_params_init_t  cp_init;
    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;
    
    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

//Note: No timer for acceleration for now. Setting trigger condition 1 or 2 for acceleration will do the same as trigger_inactive
static void timers_init(void) {

    uint32_t err_code;
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, APP_TIMER_OP_QUEUE_SIZE, false);
    
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
                    APP_TIMER_MODE_REPEATED,
                    acc_take_measurement);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the pressure.
 */
static void pres_char_init(ble_ess_init_t * p_ess_init)
{
    p_ess_init->pres_trigger_data.condition = PRES_TRIGGER_CONDITION;
    p_ess_init->init_pres_data = (uint32_t)(INIT_PRES_DATA);
    p_ess_init->pres_trigger_val_var = (uint32_t)(PRES_TRIGGER_VAL_OPERAND);
    p_ess_init->pres_trigger_data.time_interval = (uint32_t)(PRES_TRIGGER_VAL_TIME);
}

/**@brief Function for initializing the humidity.
 */
static void hum_char_init(ble_ess_init_t * p_ess_init)
{
    p_ess_init->hum_trigger_data.condition = HUM_TRIGGER_CONDITION;
    p_ess_init->init_hum_data = (uint16_t)(INIT_HUM_DATA);
    p_ess_init->hum_trigger_val_var = (uint16_t)(HUM_TRIGGER_VAL_OPERAND);
    p_ess_init->hum_trigger_data.time_interval = (uint32_t)(HUM_TRIGGER_VAL_TIME);
}

/**@brief Function for initializing the temperature.
 */
static void temp_char_init(ble_ess_init_t * p_ess_init)
{
    p_ess_init->temp_trigger_data.condition = TEMP_TRIGGER_CONDITION;
    p_ess_init->init_temp_data = (int16_t)(INIT_TEMP_DATA);
    p_ess_init->temp_trigger_val_var = (int16_t)(TEMP_TRIGGER_VAL_OPERAND);
    p_ess_init->temp_trigger_data.time_interval = (uint32_t)(TEMP_TRIGGER_VAL_TIME);
}

/**@brief Function for initializing lux.
 */
static void lux_char_init(ble_ess_init_t * p_ess_init)
{
    p_ess_init->lux_trigger_data.condition = LUX_TRIGGER_CONDITION;
    p_ess_init->init_lux_data = (uint16_t)(INIT_LUX_DATA);
    p_ess_init->lux_trigger_val_var = (uint16_t)(LUX_TRIGGER_VAL_OPERAND);
    p_ess_init->lux_trigger_data.time_interval = (uint32_t)(LUX_TRIGGER_VAL_TIME);
}

/**@brief Function for initializing the acceleration.
 */
static void acc_char_init(ble_ess_init_t * p_ess_init)
{
    p_ess_init->acc_trigger_data.condition = ACC_TRIGGER_CONDITION;
    p_ess_init->init_acc_data = (uint8_t)(INIT_ACC_DATA);
    p_ess_init->acc_trigger_val_var = (uint16_t)(ACC_TRIGGER_VAL_OPERAND);
    p_ess_init->acc_trigger_data.time_interval = (uint32_t)(ACC_TRIGGER_VAL_TIME);
}


/**@brief Function for initializing the sensor simulators.
 */
static void sensors_init(void)
{

    nrf_drv_twi_config_t twi_config;

    // Initialize the I2C module
    twi_config.sda                = I2C_SDA_PIN;
    twi_config.scl                = I2C_SCL_PIN;
    twi_config.frequency          = NRF_TWI_FREQ_400K;
    twi_config.interrupt_priority = APP_IRQ_PRIORITY_HIGH;

    nrf_drv_twi_init(&twi_instance, &twi_config, NULL);
    nrf_drv_twi_enable(&twi_instance);

    //initialize pressure
    lps331ap_init(&twi_instance);
    lps331ap_sw_reset();
    lps331ap_sw_reset_disable();
    lps331ap_power_off();
    lps331ap_config(lps331ap_MODE1, lps331ap_P_RES_10, lps331ap_T_RES_7);
    lps331ap_one_shot_config();
    lps331ap_power_on();

    //initialize humidity and temperature
    si7021_init(&twi_instance);
    si7021_reset();
    si7021_heater_off();

    //initialize lux
    tsl2561_init(&twi_instance);
    tsl2561_on();

    //initialize accelerometer
    adxl362_accelerometer_init(adxl362_NOISE_NORMAL, true, false, false);
    uint16_t act_thresh = 0x000C;
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

}

// init services
static void services_init (void) {

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

/**@brief Function for the Event Scheduler initialization.
 */
static void scheduler_init(void)
{
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}


/*******************************************************************************
 *   HELPER FUNCTIONS
 ******************************************************************************/

static void advertising_start(void) {
    uint32_t err_code = sd_ble_gap_adv_start(&m_adv_params);
    APP_ERROR_CHECK(err_code);
}

static void advertising_stop(void) {
    uint32_t err_code = sd_ble_gap_adv_stop();
    APP_ERROR_CHECK(err_code);
}

static void timers_start(void) {

 
    uint32_t err_code = app_timer_start(m_pres_timer_id, (uint32_t)(PRES_TRIGGER_VAL_TIME), NULL);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_start(m_hum_timer_id, (uint32_t)(HUM_TRIGGER_VAL_TIME), NULL);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_start(m_temp_timer_id, (uint32_t)(TEMP_TRIGGER_VAL_TIME), NULL);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_start(m_lux_timer_id, (uint32_t)(LUX_TRIGGER_VAL_TIME), NULL);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_start(m_acc_timer_id, (uint32_t)(ACC_TRIGGER_VAL_TIME), NULL);
    APP_ERROR_CHECK(err_code);
}

static void update_timers( ble_evt_t * p_ble_evt ){

    ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    uint32_t meas_interval;
    memset(&meas_interval, 0 , sizeof(meas_interval));
    meas_interval = 0;

    uint32_t err_code;

    if (p_evt_write->handle == m_ess.pressure.trigger_handle)
    {
        if ( (m_ess.pressure.trigger_val_cond == 0x01) || (m_ess.pressure.trigger_val_cond == 0x02) ){   
            memcpy(&meas_interval, m_ess.pressure.trigger_val_buff + 1, 3);
            meas_interval = (uint32_t)(meas_interval);
            meas_interval = (uint32_t)APP_TIMER_TICKS(meas_interval, APP_TIMER_PRESCALER);
            app_timer_stop(m_pres_timer_id);
            err_code = app_timer_start(m_pres_timer_id, meas_interval, NULL);
            APP_ERROR_CHECK(err_code);
        } 
    }
    else if (p_evt_write->handle == m_ess.humidity.trigger_handle)
    {
        if ( (m_ess.humidity.trigger_val_cond == 0x01) || (m_ess.humidity.trigger_val_cond == 0x02) ){   
            memcpy(&meas_interval, m_ess.humidity.trigger_val_buff + 1, 3);
            meas_interval = (uint32_t)(meas_interval);
            meas_interval = (uint32_t)APP_TIMER_TICKS(meas_interval, APP_TIMER_PRESCALER);
            app_timer_stop(m_hum_timer_id);
            err_code = app_timer_start(m_hum_timer_id, meas_interval, NULL);
            APP_ERROR_CHECK(err_code);
        }
    }
    else if (p_evt_write->handle == m_ess.temperature.trigger_handle)
    {
        if ( (m_ess.temperature.trigger_val_cond == 0x01) || (m_ess.temperature.trigger_val_cond == 0x02) ){   
            memcpy(&meas_interval, m_ess.temperature.trigger_val_buff + 1, 3);
            meas_interval = (uint32_t)(meas_interval);
            meas_interval = (uint32_t)APP_TIMER_TICKS(meas_interval, APP_TIMER_PRESCALER);
            app_timer_stop(m_temp_timer_id);
            err_code = app_timer_start(m_temp_timer_id, meas_interval, NULL);
            APP_ERROR_CHECK(err_code);
        }
    }
    else if (p_evt_write->handle == m_ess.lux.trigger_handle)
    {
        if ( (m_ess.lux.trigger_val_cond == 0x01) || (m_ess.lux.trigger_val_cond == 0x02) ){
            memcpy(&meas_interval, m_ess.lux.trigger_val_buff + 1, 3);
            meas_interval = (uint32_t)(meas_interval);
            meas_interval = (uint32_t)APP_TIMER_TICKS(meas_interval, APP_TIMER_PRESCALER);
            app_timer_stop(m_lux_timer_id);
            err_code = app_timer_start(m_lux_timer_id, meas_interval, NULL);
            APP_ERROR_CHECK(err_code);
        }
    }
    
    else if (p_evt_write->handle == m_ess.acceleration.trigger_handle)
    {
        if ( (m_ess.acceleration.trigger_val_cond == 0x01) || (m_ess.acceleration.trigger_val_cond == 0x02) ){
            memcpy(&meas_interval, m_ess.acceleration.trigger_val_buff + 1, 3);
            meas_interval = (uint32_t)(meas_interval);
            meas_interval = (uint32_t)APP_TIMER_TICKS(meas_interval, APP_TIMER_PRESCALER);
            app_timer_stop(m_acc_timer_id);
            err_code = app_timer_start(m_acc_timer_id, meas_interval, NULL);
            APP_ERROR_CHECK(err_code);
        }
    }

}

/** @brief Function for the Power manager.
 */
static void power_manage(void) {
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}

static bool update_advdata(void) {

    if (m_ess_updating_advdata == false){

        m_ess_updating_advdata = true;
        uint32_t err_code;
        uint8_t flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

        ble_advdata_service_data_t service_data;

        memcpy(&m_beacon_info[0],  &m_sensor_info.pressure, 4);
        memcpy(&m_beacon_info[4],  &m_sensor_info.humidity, 2);
        memcpy(&m_beacon_info[6], &m_sensor_info.temp, 2);
        memcpy(&m_beacon_info[8],  &m_sensor_info.light, 2);
        memcpy(&m_beacon_info[10], &m_sensor_info.acceleration, 1);

        memset(&service_data, 0, sizeof(service_data));
        service_data.data.p_data = (uint8_t *)m_beacon_info;
        service_data.data.size = APP_BEACON_INFO_LENGTH;
        service_data.service_uuid = ESS_UUID_SERVICE;

        // Build and set advertising data.
        memset(&advdata, 0, sizeof(advdata));

        advdata.name_type               = BLE_ADVDATA_FULL_NAME;
        advdata.flags                   = flags;
        advdata.p_service_data_array = &service_data;
        advdata.service_data_count = 1;

        err_code = ble_advdata_set(&advdata, NULL);
        APP_ERROR_CHECK(err_code);
        m_ess_updating_advdata = false;

        return true;

    }

    return false;
}

/*******************************************************************************
 *   MAIN LOOP
 ******************************************************************************/

int main(void) {

    // Initialization
    led_init(SQUALL_LED_PIN);
    led_init(BLEES_LED_PIN);
    led_on(SQUALL_LED_PIN);
    led_on(BLEES_LED_PIN);


    timers_init();

    // Setup BLE and services
    ble_stack_init();
    scheduler_init();
    gap_params_init();
    services_init();
    advertising_init();
    sensors_init();
    conn_params_init();

    // Advertise data
    update_advdata();

    // Start execution
    timers_start();
    advertising_start();

    gpio_init();

    // Initialization complete
    led_off(SQUALL_LED_PIN);

    while (1) {
        app_sched_execute();
        power_manage();
    }
}
