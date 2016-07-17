PROJECT_NAME = $(shell basename "$(realpath ./)")

APPLICATION_SRCS = $(notdir $(wildcard ./*.c))
APPLICATION_SRCS += softdevice_handler.c
APPLICATION_SRCS += ble_advdata.c
APPLICATION_SRCS += ble_conn_params.c
APPLICATION_SRCS += ble_debug_assert_handler.c
APPLICATION_SRCS += ble_srv_common.c

APPLICATION_SRCS += nrf_delay.c
APPLICATION_SRCS += nrf_drv_common.c
APPLICATION_SRCS += nrf_drv_twi.c
APPLICATION_SRCS += nrf_drv_gpiote.c

APPLICATION_SRCS += app_error.c
APPLICATION_SRCS += app_timer.c
APPLICATION_SRCS += app_util_platform.c
APPLICATION_SRCS += app_gpiote.c

APPLICATION_SRCS += led.c

APPLICATION_SRCS += simple_ble.c
APPLICATION_SRCS += eddystone.c
APPLICATION_SRCS += simple_adv.c
APPLICATION_SRCS += multi_adv.c

APPLICATION_SRCS += app_twi.c

#APPLICATION_SRCS += adxl362.c
#APPLICATION_SRCS += lps331ap.c
APPLICATION_SRCS += si7021.c
#APPLICATION_SRCS += tsl2561.c
#APPLICATION_SRCS += spi_driver.c

#LIBRARY_PATHS += ./ ../../platform/ ../../chips
#SOURCE_PATHS += ../../platform/ ../../chips/
LIBRARY_PATHS += ./ ../../platform/ ../../revised_chips
SOURCE_PATHS += ../../platform/ ../../revised_chips/

ENABLE_WIRELESS_DFU = 1

SDK_VERSION = 11
SOFTDEVICE_MODEL = s130
RAM_KB = 32
FLASH_KB = 256

HW_REVISION = 2.1.0
FW_REVISION = 2.0.0

NRF_BASE_PATH ?= ../../nrf5x-base/
include $(NRF_BASE_PATH)/make/Makefile
