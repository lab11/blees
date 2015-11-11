/*
 * Send an advertisement periodically
 */

#include <stdbool.h>
#include <stdint.h>
#include "nrf_gpio.h"
#include "ble_advdata.h"
#include "nordic_common.h"
#include "softdevice_handler.h"
#include "ble_debug_assert_handler.h"
#include "blees.h"
#include "led.h"

#include "dbparser.h"

#include "simple_ble.h"
#include "simple_adv.h"


// Intervals for advertising and connections
static simple_ble_config_t ble_config = {
    .platform_id       = 0x40,              // used as 4th octect in device BLE address
    .device_id         = DEVICE_ID_DEFAULT,
    .adv_name          = DEVICE_NAME,       // used in advertisements if there is room
    .adv_interval      = MSEC_TO_UNITS(500, UNIT_0_625_MS),
    .min_conn_interval = MSEC_TO_UNITS(500, UNIT_1_25_MS),
    .max_conn_interval = MSEC_TO_UNITS(1000, UNIT_1_25_MS)
};



int main(void) {
    uint32_t err_code;

    led_init(SQUALL_LED_PIN);

    // Setup BLE
    simple_ble_init(&ble_config);

    // Advertise because why not
    simple_adv_only_name();

    led_on(SQUALL_LED_PIN);


    {
        char          memseg[1000];
        db_query_mm_t mm;
        db_op_base_t* root;
        db_tuple_t    tuple;
        init_query_mm(&mm, memseg, 1000);

        // DML commands don't produce operators, they just execute.
        parse("CREATE TABLE sensors (temp int);", &mm);
        parse("INSERT INTO sensors VALUES (1);", &mm);

        root = parse("SELECT * FROM sensors", &mm);
        init_tuple(&tuple, root->header->tuple_size, root->header->num_attr, &mm);

        while (next(root, &tuple, &mm) == 1) {
        // Do whatever. Check src/dbobjects/tuple.h for how to extract tuple data.
            led_toggle(SQUALL_LED_PIN);
        }

    }









    while (1) {
        power_manage();
    }
}
