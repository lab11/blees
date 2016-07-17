#pragma once

// Libraries
#include <stdint.h>
#include "app_twi.h"

// Functions
void si7021_init(app_twi_t* twi_instance, void(*callback)(void));

void si7021_reset(void (*callback)(void));

void si7021_read_temp(void (*callback)(int16_t temperature));
void si7021_read_humidity_and_temp(void (*callback)(uint16_t humidity, int16_t temperature));

