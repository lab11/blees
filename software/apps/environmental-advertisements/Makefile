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
APPLICATION_SRCS += nrf_drv_gpiote.c
APPLICATION_SRCS += nrf_drv_twi.c
APPLICATION_SRCS += led.c
APPLICATION_SRCS += app_scheduler.c



DEVICE = NRF51

ifndef TARGET
	TARGET = BLEES
endif

TARGET_UPPER = $(shell echo $(TARGET) | tr a-z A-Z)
TARGET_LOWER = $(shell echo $(TARGET) | tr A-Z a-z)
BOARD = BOARD_$(TARGET_UPPER)

APPLICATION_SRCS += $(TARGET_LOWER).c

USE_SOFTDEVICE = s110

SDK_VERSION = 9

TEMPLATE_PATH ?= ../../nrf5x-base/nrf51-pure-gcc-setup/template/

LIBRARY_PATHS += ./ ../../platform/
SOURCE_PATHS += ../../platform/

CFLAGS = -O0
GDB_PORT_NUMBER = 2331

include ../../nrf5x-base/Makefile.include
include ../../Makefile.include
include $(TEMPLATE_PATH)/Makefile
