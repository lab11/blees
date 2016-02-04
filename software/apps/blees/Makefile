PROJECT_NAME = $(shell basename "$(realpath ./)")

APPLICATION_SRCS = $(notdir $(wildcard ./*.c))
APPLICATION_SRCS += softdevice_handler.c
APPLICATION_SRCS += ble_advdata.c
APPLICATION_SRCS += ble_conn_params.c
APPLICATION_SRCS += ble_debug_assert_handler.c
APPLICATION_SRCS += ble_srv_common.c

APPLICATION_SRCS += app_timer.c
APPLICATION_SRCS += app_util_platform.c

APPLICATION_SRCS += nrf_delay.c
APPLICATION_SRCS += nrf_drv_common.c
APPLICATION_SRCS += nrf_drv_twi.c

APPLICATION_SRCS += led.c

APPLICATION_SRCS += adxl362.c
APPLICATION_SRCS += lps331ap.c
APPLICATION_SRCS += si7021.c
APPLICATION_SRCS += tsl2561.c
APPLICATION_SRCS += spi_driver.c


LIBRARY_PATHS += ./ ../../platform/ ../../chips
SOURCE_PATHS += ../../platform/ ../../chips/


SOFTDEVICE_MODEL = s110

NRF_BASE_PATH ?= ../../nrf5x-base/
include $(NRF_BASE_PATH)/make/Makefile
