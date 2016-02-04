#ifndef __BLE_CONFIG_H
#define __BLE_CONFIG_H

#include "app_timer.h"

//cannot change server database
#define IS_SRVC_CHANGED_CHARACT_PRESENT 0

//advertising interval = 1000ms
#define APP_ADV_INTERVAL                MSEC_TO_UNITS(1000, UNIT_0_625_MS)
#define APP_COMPANY_IDENTIFIER			0xB1EE
#define MANUFACTURER_NAME 				"Lab11UMich"
#define MODEL_NUMBER 					DEVICE_NAME
#define HARDWARE_REVISION 				"A"
#define FIRMWARE_REVISION 				"0.1"

//advertising timeout sec
#define APP_ADV_TIMEOUT_IN_SECONDS      0


//RTC1_Prescale
#define APP_TIMER_PRESCALER             0

//size of op queues
#define APP_TIMER_OP_QUEUE_SIZE         8


//500ms
#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(500, UNIT_1_25_MS)

//1s
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(1000, UNIT_1_25_MS)

#define SLAVE_LATENCY                   0

//supervisory timeout 4s
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)

//time from initiating event(conn or notify start) to first time param_update c
//called
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)


#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER)

//attempts before giving up parameter negotiation
#define MAX_CONN_PARAMS_UPDATE_COUNT    3

//timeout for pairing or sec requests secs
#define SEC_PARAM_TIMEOUT               30

//perform bonding
#define SEC_PARAM_BOND                  1

//no man in the middle
#define SEC_PARAM_MITM                  0

//no i/o capability
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE

//no out of bound data
#define SEC_PARAM_OOB                   0

#define SEC_PARAM_MIN_KEY_SIZE          7

#define SEC_PARAM_MAX_KEY_SIZE          16


//max scheduler event size
//#define SCHED_MAX_EVENT_DATA_SIZE       sizeof(app_timer_event_t)
//need to fix this
#define SCHED_MAX_EVENT_DATA_SIZE       50

//max num in scheduler queue
#define SCHED_QUEUE_SIZE                10

#define MAX_PKT_LEN                     20


typedef struct ble_app_s
{
    uint16_t                     conn_handle;           /**< Handle of the current connection (as provided by the S110 SoftDevice). This will be BLE_CONN_HANDLE_INVALID when not in a connection. */
    // uint16_t                     revision;           /**< Handle of DFU Service (as provided by the S110 SoftDevice). */
    uint16_t                     service_handle;        /**< Handle of DFU Service (as provided by the S110 SoftDevice). */
    uint8_t                      uuid_type;             /**< UUID type assigned for DFU Service by the S110 SoftDevice. */
    ble_gatts_char_handles_t     char_location_handles; /**< Handles related to the DFU Packet characteristic. */
    ble_srv_error_handler_t      error_handler;         /**< Function to be called in case of an error. */
    uint8_t                      current_location[6];    /** Value of num characteristic */
} ble_app_t;


// Physical Web
#define PHYSWEB_SERVICE_ID  0xFEAA
#define PHYSWEB_URL_TYPE    0x10    // Denotes URLs (vs URIs or TLM data)
#define PHYSWEB_TX_POWER    0xBA    // Tx Power. Measured at 1 m plus 41 dBm. (who cares)

#define PHYSWEB_URLSCHEME_HTTPWWW   0x00    // http://www.
#define PHYSWEB_URLSCHEME_HTTPSWWW  0x01    // https://www.
#define PHYSWEB_URLSCHEME_HTTP      0x02    // http://
#define PHYSWEB_URLSCHEME_HTTPS     0x03    // https://

#define PHYSWEB_URLEND_COMSLASH 0x00    // .com/
#define PHYSWEB_URLEND_ORGSLASH 0x01    // .org/
#define PHYSWEB_URLEND_EDUSLASH 0x02    // .edu/
#define PHYSWEB_URLEND_COM      0x07    // .com
#define PHYSWEB_URLEND_ORG      0x08    // .org
#define PHYSWEB_URLEND_EDU      0x09    // .edu

ble_uuid_t PHYSWEB_SERVICE_UUID[] = {{PHYSWEB_SERVICE_ID, BLE_UUID_TYPE_BLE}};
ble_advdata_uuid_list_t PHYSWEB_SERVICE_LIST = {1, PHYSWEB_SERVICE_UUID};


#endif
