#ifndef __BLE_CONFIG_H
#define __BLE_CONFIG_H



//advertising interval = 1000ms
#define APP_ADV_INTERVAL                MSEC_TO_UNITS(1000, UNIT_0_625_MS)
#define APP_COMPANY_IDENTIFIER			0xB1EE
#define MANUFACTURER_NAME 				"Lab11UMich"
#define MODEL_NUMBER 					DEVICE_NAME
#define HARDWARE_REVISION 				"A"
#define FIRMWARE_REVISION 				"0.1"

//advertising timeout sec
#define APP_ADV_TIMEOUT_IN_SECONDS      0



//500ms
#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(500, UNIT_1_25_MS)

//1s
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(1000, UNIT_1_25_MS)


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



#endif
