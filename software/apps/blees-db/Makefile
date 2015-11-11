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
APPLICATION_SRCS += adxl362.c
APPLICATION_SRCS += lps331ap.c
APPLICATION_SRCS += si7021.c
APPLICATION_SRCS += tsl2561.c
APPLICATION_SRCS += spi_driver.c

# In RAM filesystem
APPLICATION_SRCS += mbramfs.c

# LittleD
APPLICATION_SRCS += relation.c
APPLICATION_SRCS += tuple.c
APPLICATION_SRCS += db_query_mm.c
APPLICATION_SRCS += dbstorage.c
APPLICATION_SRCS += eet.c
APPLICATION_SRCS += scan.c
APPLICATION_SRCS += aggregate.c
APPLICATION_SRCS += compare_tuple.c
APPLICATION_SRCS += select.c
APPLICATION_SRCS += project.c
APPLICATION_SRCS += ntjoin.c
APPLICATION_SRCS += osijoin.c
APPLICATION_SRCS += sort.c
APPLICATION_SRCS += db_ops.c
APPLICATION_SRCS += dbindex.c
APPLICATION_SRCS += query_output.c
APPLICATION_SRCS += dblexer.c
APPLICATION_SRCS += dbparseexpr.c
APPLICATION_SRCS += dbcreate.c
APPLICATION_SRCS += dbinsert.c
APPLICATION_SRCS += dbparser.c

DEVICE = NRF51
RAM_KB = 32

ifndef TARGET
	TARGET = BLEES
endif

TARGET_UPPER = $(shell echo $(TARGET) | tr a-z A-Z)
TARGET_LOWER = $(shell echo $(TARGET) | tr A-Z a-z)
BOARD = BOARD_$(TARGET_UPPER)

APPLICATION_SRCS += $(TARGET_LOWER).c

LIBRARY_PATHS += ./ ../../platform/ ../../chips
SOURCE_PATHS += ../../platform/ ../../chips/

# For LittleD
LIBRARY_PATHS += ../../LittleD/src
LIBRARY_PATHS += ../../LittleD/src/dbobjects
LIBRARY_PATHS += ../../LittleD/src/dbmm
LIBRARY_PATHS += ../../LittleD/src/dblogic
LIBRARY_PATHS += ../../LittleD/src/dbops
LIBRARY_PATHS += ../../LittleD/src/dbindex
LIBRARY_PATHS += ../../LittleD/src/dboutput
LIBRARY_PATHS += ../../LittleD/src/dbparser
LIBRARY_PATHS += ../../LittleD/src/dbstorage

SOURCE_PATHS += ../../LittleD/src/dbobjects
SOURCE_PATHS += ../../LittleD/src/dbmm
SOURCE_PATHS += ../../LittleD/src/dblogic
SOURCE_PATHS += ../../LittleD/src/dbops
SOURCE_PATHS += ../../LittleD/src/dbindex
SOURCE_PATHS += ../../LittleD/src/dboutput
SOURCE_PATHS += ../../LittleD/src/dbparser
SOURCE_PATHS += ../../LittleD/src/dbstorage

NRF_BASE_PATH ?= ../../nrf5x-base/
TEMPLATE_PATH ?= $(NRF_BASE_PATH)/nrf51-pure-gcc-setup/template/

DEVICE = NRF51
USE_SOFTDEVICE = s110
SOFTDEVICE ?= $(NRF_BASE_PATH)/softdevice/s110_nrf51_8.0.0_softdevice.hex

SDK_VERSION = 9
SDK_PATH ?= $(NRF_BASE_PATH)/sdk/nrf51_sdk_9.0.0/

CFLAGS = -Os
GDB_PORT_NUMBER = 2331

include $(NRF_BASE_PATH)/Makefile.include
include $(TEMPLATE_PATH)/Makefile
