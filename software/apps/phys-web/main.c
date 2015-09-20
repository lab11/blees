/*
 * Eddystone URL advertisement
 */

// Standard Libraries
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// Nordic Libraries
#include "ble.h"
#include "ble_advdata.h"
#include "app_timer.h"

// Platform, Peripherals, Devices, Services
#include "blees.h"
#include "led.h"
#include "simple_ble.h"
#include "eddystone.h"

// Intervals for advertising and connections
static const simple_ble_config_t ble_config = {
    .platform_id       = PLATFORM_ID_BYTE,  // used as 4th octect in device BLE address
    .adv_name          = DEVICE_NAME,       // used in advertisements if there is room
    .adv_interval      = MSEC_TO_UNITS(1000, UNIT_0_625_MS),
    .min_conn_interval = MSEC_TO_UNITS(500, UNIT_1_25_MS),
    .max_conn_interval = MSEC_TO_UNITS(1000, UNIT_1_25_MS),
};

// Maximum size is 17 characters
#define PHYSWEB_URL     "goo.gl/XMRl3M"

void ble_error(uint32_t error_code) {
    led_on(SQUALL_LED_PIN);
}

void ble_evt_connected(ble_evt_t* p_ble_evt) {
    led_on(BLEES_LED_PIN);
}

void ble_evt_disconnected(ble_evt_t* p_ble_evt) {
    led_off(BLEES_LED_PIN);
}

int main(void) {

    // Initialization
    led_init(SQUALL_LED_PIN);
    led_init(BLEES_LED_PIN);
    led_on(SQUALL_LED_PIN);
    led_off(BLEES_LED_PIN);

    simple_ble_init(&ble_config);
    eddystone_adv(PHYSWEB_URL, NULL);

    // Initialization complete
    led_off(SQUALL_LED_PIN);

    while (1) {
        power_manage();
    }
}

