#include <stdbool.h>
#include <stdint.h>
#include "led.h"
#include "nordic_common.h"
#include "softdevice_handler.h"
#include "app_gpiote.h"

// nrf5x-base libraries
#include "simple_ble.h"
#include "eddystone.h"
#include "simple_adv.h"
#include "multi_adv.h"

// Need pin number for LED
#define LED 13
#define USE_LED 0

// Interrupt pin number
#define INTERRUPT_PIN 25

// How many milliseconds between switching advertisements
#define ADV_SWITCH_MS 1000

#define DEVICE_NAME "squall+PIR"

// Manufacturer specific data setup
#define UMICH_COMPANY_IDENTIFIER 0x02E0
#define PIR_MOTION_SERVICE 0x13

// https://rawgit.com/lab11/blees/master/summon/squall-pir/index.html
#define PHYSWEB_URL "j2x.us/sbMMHT"

// Need this for one minute timer
#define ONE_MINUTE APP_TIMER_TICKS(60000, APP_TIMER_PRESCALER)

// Need this for the app_gpiote library
app_gpiote_user_id_t gpiote_user;

// Define our one-minute timeout timer
APP_TIMER_DEF(one_minute_timer);


typedef struct {
    uint8_t current_motion;         // 1 if the PIR is currently detecting motion, 0 otherwise
    uint8_t motion_since_last_adv;  // 1 if the PIR detected motion at any point since the last adv was transmitted
    uint8_t motion_last_minute;     // 1 if the PIR detected motion at any point in the last minute
} __attribute__((packed)) pir_data_t;

// Keep track of motion from the PIR. This will get put in the PIR packet
pir_data_t pir_data;

// Buffer for the manufacturer specific data
uint8_t mdata[1 + sizeof(pir_data_t)];

// Intervals for advertising and connections
static simple_ble_config_t ble_config = {
    .platform_id       = 0x90,              // used as 4th octect in device BLE address
    .device_id         = DEVICE_ID_DEFAULT,
    .adv_name          = DEVICE_NAME,       // used in advertisements if there is room
    .adv_interval      = MSEC_TO_UNITS(500, UNIT_0_625_MS),
    .min_conn_interval = MSEC_TO_UNITS(500, UNIT_1_25_MS),
    .max_conn_interval = MSEC_TO_UNITS(1000, UNIT_1_25_MS)
};


static void adv_config_eddystone () {
    eddystone_adv(PHYSWEB_URL, NULL);
}

static void adv_config_data () {
    ble_advdata_manuf_data_t mandata;

    // Put in service byte
    mdata[0] = PIR_MOTION_SERVICE;

    // Copy in latest PIR data
    memcpy(mdata+1, (uint8_t*) &pir_data, sizeof(pir_data_t));

    // Reset that we got the last motion since the last advertisement, but
    // only if the interrupt is not still high
    if (pir_data.current_motion == 0) {
        pir_data.motion_since_last_adv = 0;
    }

    // Fill out nordic struct
    mandata.company_identifier = UMICH_COMPANY_IDENTIFIER;
    mandata.data.p_data = mdata;
    mandata.data.size   = 1 + sizeof(pir_data_t);

    simple_adv_manuf_data(&mandata);
}

void interrupt_handler (uint32_t pins_l2h, uint32_t pins_h2l) {

    if (pins_l2h & (1 << INTERRUPT_PIN)) {
        // The PIR interrupt pin when high.
#if USE_LED
		led_on(LED);
#endif

        // Mark that the PIN is currently high
        pir_data.current_motion = 1;

        // We also know that we have motion since the last advertisement
        // and in the last minute.
        pir_data.motion_since_last_adv = 1;
        pir_data.motion_last_minute = 1;

        // Stop the one minute timer if it is started. We wait to set our
        // one minute timeout when motion stops.
        app_timer_stop(one_minute_timer);

    } else if (pins_h2l & (1 << INTERRUPT_PIN)) {
        // Motion detection stopped
#if USE_LED
		led_off(LED);
#endif

        // No more detected motion
        pir_data.current_motion = 0;

        // Call a one-shot timer here to to set motion to low if it ever fires.
        app_timer_start(one_minute_timer, ONE_MINUTE, NULL);
    }
}

// Called one minute after motion stopped.
void one_minute_timer_handler (void* p_context) {
    pir_data.motion_last_minute = 0;
}

int main(void) {

    // Initialize.
    led_init(LED);
    led_off(LED);

    // Enable internal DC-DC converter to save power
    // sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);

    // Setup BLE
    simple_ble_init(&ble_config);

    // Need to init multi adv
    multi_adv_init(ADV_SWITCH_MS);

    // Now register our advertisement configure functions
    multi_adv_register_config(adv_config_eddystone);
    multi_adv_register_config(adv_config_data);


    // For 3 users of GPIOTE
    APP_GPIOTE_INIT(3);

    // Register us as one user
    app_gpiote_user_register(&gpiote_user,
                             1<<INTERRUPT_PIN,   // Which pins we want the interrupt for low to high
                             1<<INTERRUPT_PIN,   // Which pins we want the interrupt for high to low
                             interrupt_handler);

    // Setup a timer to keep track of a minute
    app_timer_create(&one_minute_timer,
                     APP_TIMER_MODE_SINGLE_SHOT,
                     one_minute_timer_handler);

    // PIR pin needs a pull down
    nrf_gpio_cfg_input(INTERRUPT_PIN, NRF_GPIO_PIN_PULLDOWN);

    // Ready to go!
    app_gpiote_user_enable(gpiote_user);

    // Start rotating
    multi_adv_start();

    // Enter main loop.
    while (1) {
        sd_app_evt_wait();
    }
}
