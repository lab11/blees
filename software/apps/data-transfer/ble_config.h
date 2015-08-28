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

#define APP_TIMER_MAX_TIMERS            6

//size of op queues
#define APP_TIMER_OP_QUEUE_SIZE         5


//10ms
#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(100, UNIT_1_25_MS)

//1s
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(1000, UNIT_1_25_MS)

#define SLAVE_LATENCY                   0

//supervisory timeout 4s
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)

//time from initiating event(conn or notify start) to first time param_update c
//called
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(500, APP_TIMER_PRESCALER)


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
#define SCHED_MAX_EVENT_DATA_SIZE       sizeof(app_timer_event_t)

//max num in scheduler queue
#define SCHED_QUEUE_SIZE                10

#define MAX_PKT_LEN                     20


typedef struct ble_app_s
{
    uint16_t                     conn_handle;           // Handle of the current connection. This will be BLE_CONN_HANDLE_INVALID when not in a connection.
    uint16_t                     service_handle;
    uint8_t                      uuid_type;
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


// Data Transfer Service

// Full uuid. Bytes 12 and 13 are ignored
const ble_uuid128_t data_transfer_uuid128 = {{
    0x2a, 0x39, 0x43, 0x51, 0x55, 0x41, 0x4a, 0xae,
    0x8a, 0x42, 0x25, 0xa4, 0x00, 0x00, 0x0f, 0x99
}};

// Short uuid for the service. Placed in bytes 12 and 13
const uint16_t data_transfer_srvc_uuid16 = 0x7e40;

// Short uuids for characteristics
const uint16_t test_char_uuid16 = 0x7e41;

typedef struct data_transfer_struct {
    ble_gatts_char_handles_t    test_char_handles;
    uint8_t test_char;
    //uint8_t test_char[500];
} data_transfer_t;

#endif
