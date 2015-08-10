/*
 * Advertises Sensor Data
 */

// Standard Libraries
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Nordic Libraries
#include "nordic_common.h"
#include "softdevice_handler.h"
#include "nrf.h"
#include "nrf_sdm.h"
#include "ble.h"
#include "ble_db_discovery.h"
#include "app_util.h"
#include "app_error.h"
#include "ble_advdata_parser.h"
#include "ble_conn_params.h"
#include "ble_hci.h"
#include "nrf_gpio.h"
#include "pstorage.h"
#include "app_trace.h"
#include "ble_hrs_c.h"
#include "ble_bas_c.h"
#include "app_timer.h"

// Platform, Peripherals, Devices, Services
#include "blees.h"
#include "led.h"

#include "ble_ess.h"

#include "ble_advdata.h"
#include "ble_debug_assert_handler.h"
#include "app_util_platform.h"
#include "device_manager.h"
#include "ble_gap.h"
#include "ble_gatts.h"
#include "device_manager_cnfg.h"
#include "app_scheduler.h"
#include "nrf_soc.h"
 #include "pstorage_platform.h"


#include "nrf_error.h"
#include "nrf_assert.h"


/**********************************frombefore****************************/

#define APP_ADV_TIMEOUT_IN_SECONDS      200                                         /**< The advertising timeout (in units of seconds). */

#define APP_GPIOTE_MAX_USERS            1                                           /**< Maximum number of users of the GPIOTE handler. */


#define BUTTON_DETECTION_DELAY          APP_TIMER_TICKS(50, APP_TIMER_PRESCALER)    /**< Delay from a GPIOTE event until a button is reported as pushed (in number of timer ticks). */
#define ESS_MEAS_INTERVAL   APP_TIMER_TICKS(500, APP_TIMER_PRESCALER)


#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */


#define MIN_TEMP_LEVEL              123
#define MAX_TEMP_LEVEL              100000000
#define TEMP_LEVEL_INCREMENT        1
#define INIT_TEMP_DATA              123
#define TEMP_TRIGGER_CONDITION      TRIG_WHILE_NE
#define TEMP_TRIGGER_VAL_OPERAND    156
#define TEMP_TRIGGER_VAL_TIME       50000

#define MIN_PRES_LEVEL              456
#define MAX_PRES_LEVEL              100000000
#define PRES_LEVEL_INCREMENT        1
#define INIT_PRES_DATA              456
#define PRES_TRIGGER_CONDITION      TRIG_INACTIVE
#define PRES_TRIGGER_VAL_OPERAND    470
#define PRES_TRIGGER_VAL_TIME       100000

#define MIN_HUM_LEVEL               789
#define MAX_HUM_LEVEL               100000000
#define HUM_LEVEL_INCREMENT         100
#define INIT_HUM_DATA               789
#define HUM_TRIGGER_CONDITION       TRIG_INACTIVE
#define HUM_TRIGGER_VAL_OPERAND     799
#define HUM_TRIGGER_VAL_TIME        50000


#define BOND_DELETE_ALL_BUTTON_ID            1                                          /**< Button used for deleting all bonded centrals during startup. */


static ble_gap_sec_params_t                  m_sec_params;                               /**< Security requirements for this application. */

static uint16_t                              m_conn_handle = BLE_CONN_HANDLE_INVALID;    /**< Handle of the current connection. */

static ble_ess_t                             m_ess;

//static ble_sensorsim_cfg_t                   m_temp_sim_cfg;                         /**< Temperature sensor simulator configuration. */
//static ble_sensorsim_state_t                 m_temp_sim_state;                       /**< Temperature sensor simulator state. */

//static ble_sensorsim_cfg_t                   m_pres_sim_cfg;                         /**< Pressure sensor simulator configuration. */
//static ble_sensorsim_state_t                 m_pres_sim_state;                       /**< Pressure sensor simulator state. */

//static ble_sensorsim_cfg_t                   m_hum_sim_cfg;                         /**< Humidity sensor simulator configuration. */
//static ble_sensorsim_state_t                 m_hum_sim_state;                       /**< Humidity sensor simulator state. */

static app_timer_id_t                        m_ess_timer_id;                        /**< ESS timer. */

static app_timer_id_t                        m_temp_timer_id;                        /**< ESS timer. */
static app_timer_id_t                        m_pres_timer_id;                        /**< ESS timer. */
static app_timer_id_t                        m_hum_timer_id;                        /**< ESS timer. */


static bool                                  m_ess_meas_not_conf_pending = false; /** Flag to keep track of when a notification confirmation is pending */

static bool                                  m_memory_access_in_progress = false;       /**< Flag to keep track of ongoing operations on persistent memory. */

static dm_application_instance_t             m_app_handle;                              /**< Application identifier allocated by device manager */

static dm_security_status_t                  m_security_status;

void ess_update(void);


#include "nrf_drv_twi.h"

#include "tsl2561.h"
#include "si7021.h"
#include "lps331ap.h"
#include "adxl362.h"

nrf_drv_twi_t twi_instance = NRF_DRV_TWI_INSTANCE(1);

/*******************************************************************************
 *   DEFINES
 ******************************************************************************/
#include "ble_config.h"
#include "nrf_drv_config.h"

#define UPDATE_RATE     APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER)

#define APP_BEACON_INFO_LENGTH  16


/*******************************************************************************
 *   STATIC AND GLOBAL VARIABLES
 ******************************************************************************/

uint8_t MAC_ADDR[6] = {0x00, 0x00, 0x30, 0xe5, 0x98, 0xc0};

static ble_app_t            app;
static ble_gap_adv_params_t m_adv_params;
static ble_advdata_t        advdata;

// Security requirements for this application.
/*
static ble_gap_sec_params_t m_sec_params = {
    SEC_PARAM_BOND,
    SEC_PARAM_MITM,
    SEC_PARAM_IO_CAPABILITIES,
    SEC_PARAM_OOB,
    SEC_PARAM_MIN_KEY_SIZE,
    SEC_PARAM_MAX_KEY_SIZE,
};
*/

static app_timer_id_t sample_timer;

static struct {
    float temp;
    float humidity;
    float light;
    float pressure;
    float acceleration;
} m_sensor_info = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};

static uint8_t m_beacon_info[APP_BEACON_INFO_LENGTH];


/*******************************************************************************
 *   FUNCTION PROTOTYPES
 ******************************************************************************/
static void advertising_start(void);
static void update_advdata(void);


/*******************************************************************************
 *   HANDLERS AND CALLBACKS
 ******************************************************************************/

// Persistent storage system event handler
void pstorage_sys_event_handler (uint32_t p_evt);


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

    //led_on(SQUALL_LED_PIN);
    //while(1);
    ble_debug_assert_handler(error_code, line_num, p_file_name);
    //NVIC_SystemReset();


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


            m_ess_meas_not_conf_pending = false;

            app.conn_handle = BLE_CONN_HANDLE_INVALID;
            advertising_start();
            break;

        case BLE_GATTS_EVT_WRITE:
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
            //dm_device_delete_all(m_app_handle);

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
    switch (p_evt->evt_type)
    {
        case BLE_ESS_EVT_NOTIFICATION_ENABLED:
            // Notification has been enabled, send a single ESS measurement.
            //ess_meas_send(p_ess);
            break;
            
        case BLE_ESS_EVT_NOTIFICATION_CONFIRMED:
            m_ess_meas_not_conf_pending = false;
            break;
            
        default:
            // No implementation needed.
            break;
    }
}

/**@brief Function for handling the Application's system events.
 *
 * @param[in]   sys_evt   system event.
 */
static void on_sys_evt(uint32_t sys_evt)
{
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

    //dm_ble_evt_handler(p_ble_evt); // what does this do?
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
    //pstorage_sys_event_handler(sys_evt);
    on_sys_evt(sys_evt);
}

// timer callback
static void timer_handler (void* p_context) {
    led_toggle(BLEES_LED_PIN);

    update_advdata();
}

/**@brief Function for handling the ESS meas timer timeout.
 * *
 * @param[in]   p_context   Pointer used for passing some arbitrary information (context) from the
 *                          app_start_timer() call to the timeout handler.
 */
static void ess_meas_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    //if (m_ess_meas_not_conf_pending) ess_update();
    ess_update();
}

static void temp_meas_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);

    //if (!m_ess_meas_not_conf_pending) {
        uint32_t err_code;

        uint8_t  temp_meas_val[4]; 
        uint32_t meas;

        float temp;
        memset(&temp, 0, sizeof(temp));

        si7021_read_temp_hold(&temp);


        meas = (uint32_t)(temp);
        
        memcpy(temp_meas_val, &meas, 2);

        err_code = ble_ess_char_value_update(&m_ess, &(m_ess.temperature), temp_meas_val, MAX_TEMP_LEN, false, &(m_ess.temp_char_handles) );
        
        if (err_code == NRF_SUCCESS) {
            m_ess_meas_not_conf_pending = true;
        }

        else if ( //(err_code != NRF_SUCCESS) &&
            (err_code != NRF_ERROR_INVALID_STATE) &&
            (err_code != BLE_ERROR_NO_TX_BUFFERS) &&
            (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
            )
        {
            APP_ERROR_HANDLER(err_code);
        }
    //}

}

static void pres_meas_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);

    //if (!m_ess_meas_not_conf_pending) {
        uint32_t err_code;
        
        uint8_t  pres_meas_val[4]; 
        uint32_t meas;

        float pres;
        memset(&pres, 0, sizeof(pres));

        lps331ap_readPressure(&pres);

        meas = (uint32_t)(pres);
        
        memcpy(pres_meas_val, &meas, 4);

        err_code = ble_ess_char_value_update(&m_ess, &(m_ess.pressure), pres_meas_val, MAX_PRES_LEN, false, &(m_ess.pres_char_handles) );
        
        if (err_code == NRF_SUCCESS) {
            m_ess_meas_not_conf_pending = true;
        }

        else if ( //(err_code != NRF_SUCCESS) &&
            (err_code != NRF_ERROR_INVALID_STATE) &&
            (err_code != BLE_ERROR_NO_TX_BUFFERS) &&
            (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
            )
        {
            APP_ERROR_HANDLER(err_code);
        }
    //}

}

static void hum_meas_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);

    //if (!m_ess_meas_not_conf_pending) {
        uint32_t err_code;
        
        uint8_t  hum_meas_val[4]; 
        uint32_t meas;
        float hum;
        memset(&hum, 0, sizeof(hum));

        (si7021_read_RH_hold(&hum));

        meas = (uint32_t)(hum);
        
        memcpy(hum_meas_val, &meas, 2);

        err_code = ble_ess_char_value_update(&m_ess, &(m_ess.humidity), hum_meas_val, MAX_HUM_LEN, false, &(m_ess.hum_char_handles) );
        
        if (err_code == NRF_SUCCESS) {
            m_ess_meas_not_conf_pending = true;
        }

        if ( //(err_code != NRF_SUCCESS) &&
            (err_code != NRF_ERROR_INVALID_STATE) &&
            (err_code != BLE_ERROR_NO_TX_BUFFERS) &&
            (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
            )
        {
            APP_ERROR_HANDLER(err_code);
        }
    //}

}

/**@brief Function for handling the Device Manager events.
 *
 * @param[in]   p_evt   Data associated to the device manager event.
 */
static uint32_t device_manager_evt_handler(dm_handle_t const * p_handle,
                                           dm_event_t const  * p_event,
                                           ret_code_t        event_result)
{
    
    APP_ERROR_CHECK(event_result);
    
    switch(p_event->event_id)
    {
        case DM_EVT_DEVICE_CONTEXT_STORED:
            //dm_security_status_req(p_handle, &m_security_status);
            break;
        case DM_EVT_LINK_SECURED:
            //dm_security_status_req(p_handle, &m_security_status);
            break;
        case DM_EVT_DISCONNECTION:
            //dm_device_delete(p_handle);
            break;
        default:
            break;
    }
    
    return NRF_SUCCESS;
    
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
    //m_adv_params.type               = BLE_GAP_ADV_TYPE_ADV_NONCONN_IND; // use this to disable connection abilit
    m_adv_params.type               = BLE_GAP_ADV_TYPE_ADV_IND;

    m_adv_params.p_peer_addr        = NULL;
    m_adv_params.fp                 = BLE_GAP_ADV_FP_ANY;
    m_adv_params.interval           = APP_ADV_INTERVAL;
    m_adv_params.timeout            = APP_ADV_TIMEOUT_IN_SECONDS;

    /*Newly added*/
    volatile uint32_t      err_code;
    ble_advdata_t advdata;
    ble_advdata_t scanrsp;
    uint8_t flags = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;
    ble_uuid_t adv_uuids[] = {
        {ESS_UUID_SERVICE, m_ess.uuid_type}
    };
    
    // Build and set advertising data
    memset(&advdata, 0, sizeof(advdata));
    
    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = true;
    //advdata.flags.size              = sizeof(flags);
    //advdata.flags.p_data            = &flags;
    advdata.flags = flags;
    
    memset(&scanrsp, 0, sizeof(scanrsp));
    scanrsp.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
    scanrsp.uuids_complete.p_uuids  = adv_uuids;
    
    err_code = ble_advdata_set(&advdata, &scanrsp);
    
    APP_ERROR_CHECK(err_code);



}

// Initialize connection parameters
static void conn_params_init(void) {

    uint32_t               err_code;
    ble_conn_params_init_t cp_init;
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

static void timers_init(void) {
    uint32_t err_code;

    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS,
            APP_TIMER_OP_QUEUE_SIZE, false);

    err_code = app_timer_create(&sample_timer, APP_TIMER_MODE_REPEATED,
            timer_handler);
    APP_ERROR_CHECK(err_code);

    // Create timers.
    // APP_TIMER_MODE_REPEATED means the timer will resart every time it expires
    // ess_meas_timeout_handler called when the timer expires
    err_code = app_timer_create(&m_ess_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                ess_meas_timeout_handler);
    APP_ERROR_CHECK(err_code);
        
    // Initialize timer for Temperature trigger
    err_code = app_timer_create(&m_temp_timer_id,
    APP_TIMER_MODE_REPEATED,
    temp_meas_timeout_handler);
    APP_ERROR_CHECK(err_code);
    
    // Initialize timer for Pressure Trigger
    err_code = app_timer_create(&m_pres_timer_id,
    APP_TIMER_MODE_REPEATED,
    pres_meas_timeout_handler);
    APP_ERROR_CHECK(err_code);
    
    // Initialize timer for Humidity Trigger
    err_code = app_timer_create(&m_hum_timer_id,
    APP_TIMER_MODE_REPEATED,
    hum_meas_timeout_handler);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the temperature.
 */
static void temp_char_init(ble_ess_init_t * p_ess_init)
{
    int16_t init_data = INIT_TEMP_DATA;
    p_ess_init->temp_trigger_data.condition = TEMP_TRIGGER_CONDITION;
    int16_t operand = TEMP_TRIGGER_VAL_OPERAND; // used if trigger doesn't require a timer
    uint32_t temp_time = TEMP_TRIGGER_VAL_TIME; // used if trigger requires a timer

    p_ess_init->init_temp_data = init_data;
    p_ess_init->temp_trigger_val_var = (operand);
    p_ess_init->temp_trigger_data.time_interval = (temp_time);


}

/**@brief Function for initializing the pressure.
 */
static void pres_char_init(ble_ess_init_t * p_ess_init)
{
    uint32_t init_data = INIT_PRES_DATA;
    p_ess_init->pres_trigger_data.condition = PRES_TRIGGER_CONDITION;
    uint32_t operand = PRES_TRIGGER_VAL_OPERAND; // used if trigger doesn't require a timer
    uint32_t pres_time = PRES_TRIGGER_VAL_TIME; // used if trigger requires a timer

    p_ess_init->init_pres_data = init_data;
    p_ess_init->pres_trigger_val_var = (operand);
    p_ess_init->pres_trigger_data.time_interval = (pres_time);

}

/**@brief Function for initializing the humidity.
 */
static void hum_char_init(ble_ess_init_t * p_ess_init)
{
    uint16_t init_data = INIT_HUM_DATA;
    p_ess_init->hum_trigger_data.condition = HUM_TRIGGER_CONDITION;
    uint16_t operand = HUM_TRIGGER_VAL_OPERAND; // used if trigger doesn't require a timer
    uint32_t hum_time = HUM_TRIGGER_VAL_TIME; // used if trigger requires a timer

    p_ess_init->init_hum_data = init_data;
    p_ess_init->hum_trigger_val_var = (operand);
    p_ess_init->hum_trigger_data.time_interval = (hum_time);

}

/**@brief Function for initializing the sensor simulators.
 */
static void sensor_sim_init(void)
{

    nrf_drv_twi_config_t twi_config;

    // Initialize the I2C module
    twi_config.sda                = I2C_SDA_PIN;
    twi_config.scl                = I2C_SCL_PIN;
    twi_config.frequency          = NRF_TWI_FREQ_400K;
    twi_config.interrupt_priority = APP_IRQ_PRIORITY_HIGH;

    nrf_drv_twi_init(&twi_instance, &twi_config, NULL);
    nrf_drv_twi_enable(&twi_instance);

    //initialize temperature and humidity
    si7021_init(&twi_instance);
    si7021_reset();
    si7021_heater_off();


    //initialize pressure
    lps331ap_init(&twi_instance);
    lps331ap_sw_reset();
    lps331ap_sw_reset_disable();
    lps331ap_power_off();
    lps331ap_config(lps331ap_MODE1, lps331ap_P_RES_10, lps331ap_T_RES_7);
    lps331ap_power_on();

    
}

// init services
static void services_init (void) {
    uint32_t err_code;
    ble_ess_init_t ess_init;
    
    //Initialize the Environmental Sensing Service
    memset(&ess_init, 0 , sizeof(ess_init));
    
    ess_init.evt_handler = on_ess_evt;
    ess_init.is_notify_supported = true;
    
    
    temp_char_init(&ess_init); //initialize temp
    pres_char_init(&ess_init); //initialize pres
    hum_char_init(&ess_init); //initialize hum
    
    err_code = ble_ess_init(&m_ess, &ess_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the Device Manager initialization.
 */
static void device_manager_init(void)
{

    /*
    
    uint32_t               err_code;
    dm_init_param_t        init_data;
    dm_application_param_t register_param;

    // Initialize persistent storage module.
    //err_code = pstorage_init();
    //APP_ERROR_CHECK(err_code);

    memset(&init_data, 0, sizeof(init_data));

    init_data.clear_persistent_data = true;

    // Clear all bonded centrals if the Bonds Delete button is pushed.
    //err_code = bsp_button_is_pressed(BOND_DELETE_ALL_BUTTON_ID,&(init_data.clear_persistent_data));
    //APP_ERROR_CHECK(err_code);

    err_code = dm_init(&init_data);
    APP_ERROR_CHECK(err_code);

    memset(&register_param.sec_param, 0, sizeof(ble_gap_sec_params_t));
    
    //register_param.sec_param.timeout      = SEC_PARAM_TIMEOUT;
    register_param.sec_param.bond         = SEC_PARAM_BOND;
    register_param.sec_param.mitm         = SEC_PARAM_MITM;
    register_param.sec_param.io_caps      = SEC_PARAM_IO_CAPABILITIES;
    register_param.sec_param.oob          = SEC_PARAM_OOB;
    register_param.sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    register_param.sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
    register_param.evt_handler            = device_manager_evt_handler;
    register_param.service_type           = DM_PROTOCOL_CNTXT_GATT_SRVR_ID;

    err_code = dm_register(&m_app_handle, &register_param);
    APP_ERROR_CHECK(err_code);
    */
    
}

/**@brief Function for initializing security parameters.
 */
static void sec_params_init(void)
{
    m_sec_params.bond         = SEC_PARAM_BOND;
    m_sec_params.mitm         = SEC_PARAM_MITM;
    m_sec_params.io_caps      = SEC_PARAM_IO_CAPABILITIES;
    m_sec_params.oob          = SEC_PARAM_OOB;
    m_sec_params.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    m_sec_params.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
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

    /*
    uint32_t count, err_code;

    // Verify if there is any flash access pending, if yes delay starting advertising until
    // it's complete.
    err_code = pstorage_access_status_get(&count);
    APP_ERROR_CHECK(err_code);

    if (count != 0)
    {
        m_memory_access_in_progress = true;
        return;
    }
    
    */
    /***from before***/
    uint32_t err_code = sd_ble_gap_adv_start(&m_adv_params);
    APP_ERROR_CHECK(err_code);
}

static void advertising_stop(void) {
    uint32_t err_code = sd_ble_gap_adv_stop();
    APP_ERROR_CHECK(err_code);
}

static void timers_start(void) {
    uint32_t err_code = app_timer_start(sample_timer, UPDATE_RATE, NULL);
    APP_ERROR_CHECK(err_code);

    /**added***/
    // Start application timers.
    err_code = app_timer_start(m_ess_timer_id, ESS_MEAS_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);

    
    uint32_t meas_interval;


    if ( (m_ess.temperature.trigger_val_cond == 0x01) || (m_ess.temperature.trigger_val_cond == 0x02) ){   
        memcpy(&meas_interval, m_ess.temperature.trigger_val_buff + 1, 3);
        err_code = app_timer_start(m_temp_timer_id, meas_interval, NULL);
        APP_ERROR_CHECK(err_code);
    }
    else{
        err_code = app_timer_stop(m_temp_timer_id);
        APP_ERROR_CHECK(err_code);
    }
    
    
    if ( (m_ess.pressure.trigger_val_cond == 0x01) || (m_ess.pressure.trigger_val_cond == 0x02) ){   
        memcpy(&meas_interval, m_ess.pressure.trigger_val_buff + 1, 3);
        err_code = app_timer_start(m_pres_timer_id, meas_interval, NULL);
        APP_ERROR_CHECK(err_code);
    } 
    else{
        err_code = app_timer_stop(m_pres_timer_id);
        APP_ERROR_CHECK(err_code);
    }
    
    
    if ( (m_ess.humidity.trigger_val_cond == 0x01) || (m_ess.humidity.trigger_val_cond == 0x02) ){   
        memcpy(&meas_interval, m_ess.humidity.trigger_val_buff + 1, 3);
        err_code = app_timer_start(m_hum_timer_id, meas_interval, NULL);
        APP_ERROR_CHECK(err_code);
    }
    else{
        err_code = app_timer_stop(m_hum_timer_id);
        APP_ERROR_CHECK(err_code);
    }
}

/** @brief Function for the Power manager.
 */
static void power_manage(void) {
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}

static void update_advdata(void) {
    uint32_t err_code;
    uint8_t flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    ble_advdata_manuf_data_t manuf_specific_data;

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
    advdata.flags                   = flags;
    advdata.p_manuf_specific_data   = &manuf_specific_data;

    err_code = ble_advdata_set(&advdata, NULL);
    APP_ERROR_CHECK(err_code);
}


/*******************************************************************************
 *   Need to be moved
 ******************************************************************************/
/**@brief Function for updating the Sensors for the ESS.
 */
void ess_update(void)
{
    uint32_t err_code;

    uint8_t  temp_meas_val[4];
    uint8_t  pres_meas_val[4];
    uint8_t  hum_meas_val[4];
    
    uint32_t meas;

    if (m_ess.temperature.trigger_val_cond >= 0x03){
        float temp;
        memset(&temp, 0, sizeof(temp));

        si7021_read_temp_hold(&temp);


        meas = (uint32_t)(temp);
        
        memcpy(temp_meas_val, &meas, 2);

        err_code = ble_ess_char_value_update(&m_ess, &(m_ess.temperature), temp_meas_val, MAX_TEMP_LEN, false, &(m_ess.temp_char_handles) );
        
        if (err_code == NRF_SUCCESS) {
            m_ess_meas_not_conf_pending = true;
        }

        else if ( //(err_code != NRF_SUCCESS) &&
            (err_code != NRF_ERROR_INVALID_STATE) &&
            (err_code != BLE_ERROR_NO_TX_BUFFERS) &&
            (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
            )
        {
            APP_ERROR_HANDLER(err_code);
        }
        
        
    }

    if (m_ess.pressure.trigger_val_cond >= 0x03){

        float pres;
        memset(&pres, 0, sizeof(pres));

        lps331ap_readPressure(&pres);

        meas = (uint32_t)(pres);

        memcpy(pres_meas_val, &meas, 4);
        
        err_code = ble_ess_char_value_update(&m_ess, &(m_ess.pressure), pres_meas_val, MAX_PRES_LEN, false, &(m_ess.pres_char_handles) );
        
        if (err_code == NRF_SUCCESS) {
            m_ess_meas_not_conf_pending = true;
        }

        else if (//(err_code != NRF_SUCCESS) &&
            (err_code != NRF_ERROR_INVALID_STATE) &&
            (err_code != BLE_ERROR_NO_TX_BUFFERS) &&
            (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
            )
        {
            APP_ERROR_HANDLER(err_code);
        }
    
    }

    if (m_ess.humidity.trigger_val_cond >= 0x03){

        float hum;
        memset(&hum, 0, sizeof(hum));

        (si7021_read_RH_hold(&hum));

        meas = (uint32_t)(hum);

        memcpy(hum_meas_val, &meas, 2);

        err_code = ble_ess_char_value_update(&m_ess, &(m_ess.humidity), hum_meas_val, MAX_HUM_LEN, false, &(m_ess.hum_char_handles) );
        
        if (err_code == NRF_SUCCESS) {
            m_ess_meas_not_conf_pending = true;
        }

        else if (//(err_code != NRF_SUCCESS) &&
            (err_code != NRF_ERROR_INVALID_STATE) &&
            (err_code != BLE_ERROR_NO_TX_BUFFERS) &&
            (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
            )
        {
            APP_ERROR_HANDLER(err_code);
        }
    
   }
    
}

/*******************************************************************************
 *   MAIN LOOP
 ******************************************************************************/

int main(void) {

    //printf("hi");



    // Initialization
    //led_init(SQUALL_LED_PIN);
    //led_init(BLEES_LED_PIN);
    //led_on(SQUALL_LED_PIN);
    //led_on(BLEES_LED_PIN);

    timers_init();

    // Setup BLE and services
    ble_stack_init();
    scheduler_init(); //new
    device_manager_init(); //new
    gap_params_init();
    services_init();
    advertising_init();
        sensor_sim_init();//new
    conn_params_init();
    sec_params_init();

    // Advertise data
    update_advdata();

    // Start execution
    timers_start();
    advertising_start();

    // Initialization complete
    //led_off(SQUALL_LED_PIN);
    //printf("hi");

    //printf("hi2");

    //printf("hi3");


    while (1) {
        app_sched_execute();

        //printf("hey");

        power_manage();
    }
}
