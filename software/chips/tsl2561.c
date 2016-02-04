#include <stdint.h>
#include <stdbool.h>

#include "nrf_drv_twi.h"
#include "app_error.h"

#include "tsl2561.h"

// Register locations
#define CONTROL_REG_ADDR			0x0
#define TIMING_REG_ADDR				0x1
#define THRESHLOWLOW_REG_ADDR		0x2
#define THRESHLOWHIGH_REG_ADDR		0x3
#define	THRESHHIGHLOW_REG_ADDR		0x4
#define THRESHHIGHHIGH_REG_ADDR		0x5
#define INTERRUPT_CONTROL_REG_ADDR	0x6
#define CH0_LOW 	  				0xC
#define CH0_HIGH 					0xD
#define	CH1_LOW						0xE
#define CH1_HIGH					0xF

// Command register defines
#define COMMAND_REG					0x80
#define CLEAR_INTERRUPT				0x40
#define	WORD_PROTOCOL 				0x20
#define	BLOCK_PROTOCOL				0x10

//Control_Reg defines
#define POWER_ON					0x03
#define	POWER_OFF					0x00

//Timing_Reg defines
#define HIGH_GAIN_MODE				0x10
#define LOW_GAIN_MODE				0x00
#define	MANUAL_INTEGRATION			0x03
#define	BEGIN_INTEGRATION			0x08
#define	STOP_INTEGRATION			~BEGIN_INTEGRATION

//Interrupt_Control_Reg defines
#define	INTERRUPT_OUTPUT_DISABLED	(0x0 << 4)
#define INTERRUPT_LEVEL_INTERRUPT	(0x1 << 4)
#define INTERRUPT_ON_ADC_DONE       (0x0)


/******************************************************************************
 * Global State
 ******************************************************************************/

// Pointer to I2C Hardware
static nrf_drv_twi_t* m_instance;
static uint8_t m_i2c_addr;

// Device configuration
static tsl2561_gain_mode_t m_mode_gain = tsl2561_GAIN_LOW;
static tsl2561_integration_time_mode_t m_mode_time = tsl2561_INTEGRATION_402MS;
static tsl2561_package_type_t m_package_type = tsl2561_PACKAGE_TFNCL;


/******************************************************************************
 * Internal functions for this driver
 ******************************************************************************/

// Read a single byte from a register on the sensor.
static uint8_t read_byte (uint8_t reg) {
    ret_code_t error;
    uint8_t command[1];
    uint8_t ret;

    // Setup which register we want to read from.
    command[0] = COMMAND_REG | reg;
    error = nrf_drv_twi_tx(m_instance, m_i2c_addr, command, 1, true);

    // Now do the RX
    error = nrf_drv_twi_rx(m_instance, m_i2c_addr, &ret, 1, false);
    return ret;
}

// Read ADC Channels Using Read Word Protocol
static void tsl2561_readADC(uint16_t* channel0_data, uint16_t* channel1_data) {
	uint8_t data_low, data_high;

	// Read from channel 0
	data_low = read_byte(CH0_LOW);
	data_high = read_byte(CH0_HIGH);
	*channel0_data = (((uint16_t) data_high) << 8) | data_low;

	// Read from channel 1
	data_low = read_byte(CH1_LOW);
	data_high = read_byte(CH1_HIGH);
	*channel1_data = (((uint16_t) data_high) << 8) | data_low;
}


/******************************************************************************
 * Public Functions
 ******************************************************************************/

// Pass in a pointer to the I2C hardware we can use. Call this first before
// any other ts12561 functions.
// The i2c address should be the 7 bit address NOT shifted.
void tsl2561_driver_init(nrf_drv_twi_t* p_instance, uint8_t i2c_addr) {
	m_instance = p_instance;
	m_i2c_addr = i2c_addr;
}

// Enable the light sensor. This will cause it to begin sampling.
void tsl2561_on(void) {
	ret_code_t error;

    // Setup commands to say that we want to write the control register
    // and we want to turn the light sensor on.
	uint8_t command[2] = {
		COMMAND_REG | CONTROL_REG_ADDR,
		POWER_ON
	};

    error = nrf_drv_twi_tx(m_instance, m_i2c_addr, command, 2, false);

    // Now clear interrupts in case any were set
	command[0] = COMMAND_REG | CLEAR_INTERRUPT;
	error = nrf_drv_twi_tx(m_instance, m_i2c_addr, command, 1, false);
}

// Disable the light sensor and put it back into low power mode.
void tsl2561_off(void) {
	ret_code_t error;

    // Setup command to write control register and turn sensor off.
	uint8_t command[2] = {
		COMMAND_REG | CLEAR_INTERRUPT | CONTROL_REG_ADDR,
		POWER_OFF
	};
	error = nrf_drv_twi_tx(m_instance, m_i2c_addr, command, 2, false);
}

// Configure the sample integration time and gain.
void tsl2561_config(tsl2561_gain_mode_t mode_gain, tsl2561_integration_time_mode_t mode_time) {
	ret_code_t error;

	// save configuration settings
	m_mode_gain = mode_gain;
	m_mode_time = mode_time;

	uint8_t command[2] = {
		COMMAND_REG | TIMING_REG_ADDR,
		mode_gain | mode_time
	};

	error = nrf_drv_twi_tx(m_instance, m_i2c_addr, command, 2, false);
}

// enable level interrupt on an ADC read
void tsl2561_interrupt_enable (void) {
	ret_code_t error;

    // set interrupt register
    uint8_t command[2] = {
        COMMAND_REG | INTERRUPT_CONTROL_REG_ADDR,
        (INTERRUPT_LEVEL_INTERRUPT) | (INTERRUPT_ON_ADC_DONE)
    };
    error = nrf_drv_twi_tx(m_instance, m_i2c_addr, command, 2, false);

    // clear interrupts
    command[0] = COMMAND_REG | CLEAR_INTERRUPT;
    error = nrf_drv_twi_tx(m_instance, m_i2c_addr, command, 1, false);
}

// disable interrupts
void tsl2561_interrupt_disable (void) {
	ret_code_t error;

    // clear interrupts and disable them
    uint8_t command[2] = {
        COMMAND_REG | INTERRUPT_CONTROL_REG_ADDR,
        INTERRUPT_OUTPUT_DISABLED
    };
    error = nrf_drv_twi_tx(m_instance, m_i2c_addr, command, 2, false);
}



// ADC counts to Lux value conversion copied from TSL2561 manual

//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
// Value scaling factors
//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
#define LUX_SCALE 14 // scale by 2^14
#define RATIO_SCALE 9 // scale ratio by 2^9

//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
// Integration time scaling factors
//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
#define CH_SCALE 10 // scale channel values by 2^10
#define CHSCALE_TINT0 0x7517 // 322/11 * 2^CH_SCALE
#define CHSCALE_TINT1 0x0fe7 // 322/81 * 2^CH_SCALE

//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
// T, FN, and CL Package coefficients
//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
// For Ch1/Ch0=0.00 to 0.50
// Lux/Ch0=0.0304−0.062*((Ch1/Ch0)^1.4)
// piecewise approximation
// For Ch1/Ch0=0.00 to 0.125:
// Lux/Ch0=0.0304−0.0272*(Ch1/Ch0)
//
// For Ch1/Ch0=0.125 to 0.250:
// Lux/Ch0=0.0325−0.0440*(Ch1/Ch0)
//
// For Ch1/Ch0=0.250 to 0.375:
// Lux/Ch0=0.0351−0.0544*(Ch1/Ch0)
//
// For Ch1/Ch0=0.375 to 0.50:
// Lux/Ch0=0.0381−0.0624*(Ch1/Ch0)
//
// For Ch1/Ch0=0.50 to 0.61:
// Lux/Ch0=0.0224−0.031*(Ch1/Ch0)
//
// For Ch1/Ch0=0.61 to 0.80:
// Lux/Ch0=0.0128−0.0153*(Ch1/Ch0)
//
// For Ch1/Ch0=0.80 to 1.30:
// Lux/Ch0=0.00146−0.00112*(Ch1/Ch0)
//
// For Ch1/Ch0>1.3:
// Lux/Ch0=0
//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
#define K1T 0x0040 // 0.125 * 2^RATIO_SCALE
#define B1T 0x01f2 // 0.0304 * 2^LUX_SCALE
#define M1T 0x01be // 0.0272 * 2^LUX_SCALE
#define K2T 0x0080 // 0.250 * 2^RATIO_SCALE
#define B2T 0x0214 // 0.0325 * 2^LUX_SCALE
#define M2T 0x02d1 // 0.0440 * 2^LUX_SCALE
#define K3T 0x00c0 // 0.375 * 2^RATIO_SCALE
#define B3T 0x023f // 0.0351 * 2^LUX_SCALE
#define M3T 0x037b // 0.0544 * 2^LUX_SCALE
#define K4T 0x0100 // 0.50 * 2^RATIO_SCALE
#define B4T 0x0270 // 0.0381 * 2^LUX_SCALE
#define M4T 0x03fe // 0.0624 * 2^LUX_SCALE
#define K5T 0x0138 // 0.61 * 2^RATIO_SCALE
#define B5T 0x016f // 0.0224 * 2^LUX_SCALE
#define M5T 0x01fc // 0.0310 * 2^LUX_SCALE
#define K6T 0x019a // 0.80 * 2^RATIO_SCALE
#define B6T 0x00d2 // 0.0128 * 2^LUX_SCALE
#define M6T 0x00fb // 0.0153 * 2^LUX_SCALE
#define K7T 0x029a // 1.3 * 2^RATIO_SCALE
#define B7T 0x0018 // 0.00146 * 2^LUX_SCALE
#define M7T 0x0012 // 0.00112 * 2^LUX_SCALE
#define K8T 0x029a // 1.3 * 2^RATIO_SCALE
#define B8T 0x0000 // 0.000 * 2^LUX_SCALE
#define M8T 0x0000 // 0.000 * 2^LUX_SCALE

//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
// CS package coefficients
//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
// For 0 <= Ch1/Ch0 <= 0.52
// Lux/Ch0 = 0.0315−0.0593*((Ch1/Ch0)^1.4)
// piecewise approximation
// For 0 <= Ch1/Ch0 <= 0.13
// Lux/Ch0 = 0.0315−0.0262*(Ch1/Ch0)
// For 0.13 <= Ch1/Ch0 <= 0.26
// Lux/Ch0 = 0.0337−0.0430*(Ch1/Ch0)
// For 0.26 <= Ch1/Ch0 <= 0.39
// Lux/Ch0 = 0.0363−0.0529*(Ch1/Ch0)
// For 0.39 <= Ch1/Ch0 <= 0.52
// Lux/Ch0 = 0.0392−0.0605*(Ch1/Ch0)
// For 0.52 < Ch1/Ch0 <= 0.65
// Lux/Ch0 = 0.0229−0.0291*(Ch1/Ch0)
// For 0.65 < Ch1/Ch0 <= 0.80
// Lux/Ch0 = 0.00157−0.00180*(Ch1/Ch0)
// For 0.80 < Ch1/Ch0 <= 1.30
// Lux/Ch0 = 0.00338−0.00260*(Ch1/Ch0)
// For Ch1/Ch0 > 1.30
// Lux = 0
//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
#define K1C 0x0043 // 0.130 * 2^RATIO_SCALE
#define B1C 0x0204 // 0.0315 * 2^LUX_SCALE
#define M1C 0x01ad // 0.0262 * 2^LUX_SCALE
#define K2C 0x0085 // 0.260 * 2^RATIO_SCALE
#define B2C 0x0228 // 0.0337 * 2^LUX_SCALE
#define M2C 0x02c1 // 0.0430 * 2^LUX_SCALE
#define K3C 0x00c8 // 0.390 * 2^RATIO_SCALE
#define B3C 0x0253 // 0.0363 * 2^LUX_SCALE
#define M3C 0x0363 // 0.0529 * 2^LUX_SCALE
#define K4C 0x010a // 0.520 * 2^RATIO_SCALE
#define B4C 0x0282 // 0.0392 * 2^LUX_SCALE
#define M4C 0x03df // 0.0605 * 2^LUX_SCALE
#define K5C 0x014d // 0.65 * 2^RATIO_SCALE
#define B5C 0x0177 // 0.0229 * 2^LUX_SCALE
#define M5C 0x01dd // 0.0291 * 2^LUX_SCALE
#define K6C 0x019a // 0.80 * 2^RATIO_SCALE
#define B6C 0x0101 // 0.0157 * 2^LUX_SCALE
#define M6C 0x0127 // 0.0180 * 2^LUX_SCALE
#define K7C 0x029a // 1.3 * 2^RATIO_SCALE
#define B7C 0x0037 // 0.00338 * 2^LUX_SCALE
#define M7C 0x002b // 0.00260 * 2^LUX_SCALE
#define K8C 0x029a // 1.3 * 2^RATIO_SCALE
#define B8C 0x0000 // 0.000 * 2^LUX_SCALE
#define M8C 0x0000 // 0.000 * 2^LUX_SCALE

//////////////////////////////////////////////////////////////////////////////
//
// Calculate the approximate illuminance (lux) given the raw channel values of
//  the TSL2560. The equation is implemented as a piece−wise linear
//  approximation without floating point calculations
//
//////////////////////////////////////////////////////////////////////////////
uint32_t tsl2561_read_lux (void) {

    // read ADC values from sensor
    uint16_t ch0 = 0;
    uint16_t ch1 = 0;
    tsl2561_readADC(&ch0, &ch1);

    // first, scale the channel values depending on the gain and integration time
    // 16X, 402mS is nominal.
    // scale if integration time is NOT 402 msec
    unsigned long chScale;
    unsigned long channel1;
    unsigned long channel0;
    switch (m_mode_time) {
        case tsl2561_INTEGRATION_13p7MS: // 13.7 msec
            chScale = CHSCALE_TINT0;
            break;
        case tsl2561_INTEGRATION_101MS: // 101 msec
            chScale = CHSCALE_TINT1;
            break;
        default: // assume no scaling
            chScale = (1 << CH_SCALE);
            break;
    }

    // scale if gain is NOT 16X
    if (m_mode_gain == tsl2561_GAIN_LOW) {
        chScale = chScale << 4; // scale 1X to 16X
    }

    // scale the channel values
    channel0 = (ch0 * chScale) >> CH_SCALE;
    channel1 = (ch1 * chScale) >> CH_SCALE;

    // find the ratio of the channel values (Channel1/Channel0)
    // protect against divide by zero
    unsigned long ratio1 = 0;
    if (channel0 != 0) {
        ratio1 = (channel1 << (RATIO_SCALE+1)) / channel0;
    }

    // round the ratio value
    unsigned long ratio = (ratio1 + 1) >> 1;

    // is ratio <= eachBreak ?
    unsigned int b, m;
    switch (m_package_type) {
        case tsl2561_PACKAGE_TFNCL: // T, FN, and CL package
            if ((ratio >= 0) && (ratio <= K1T)) {
                b=B1T; m=M1T;
            } else if (ratio <= K2T) {
                b=B2T; m=M2T;
            } else if (ratio <= K3T) {
                b=B3T; m=M3T;
            } else if (ratio <= K4T) {
                b=B4T; m=M4T;
            } else if (ratio <= K5T) {
                b=B5T; m=M5T;
            } else if (ratio <= K6T) {
                b=B6T; m=M6T;
            } else if (ratio <= K7T) {
                b=B7T; m=M7T;
            } else if (ratio > K8T) {
                b=B8T; m=M8T;
            }
            break;
        case tsl2561_PACKAGE_CS:// CS package
            if ((ratio >= 0) && (ratio <= K1C)) {
                b=B1C; m=M1C;
            } else if (ratio <= K2C) {
                b=B2C; m=M2C;
            } else if (ratio <= K3C) {
                b=B3C; m=M3C;
            } else if (ratio <= K4C) {
                b=B4C; m=M4C;
            } else if (ratio <= K5C) {
                b=B5C; m=M5C;
            } else if (ratio <= K6C) {
                b=B6C; m=M6C;
            } else if (ratio <= K7C) {
                b=B7C; m=M7C;
            } else if (ratio > K8C) {
                b=B8C; m=M8C;
            }
            break;
    }

    // calculate actual lux value
    unsigned long val;
    val = ((channel0 * b) - (channel1 * m));

    // do not allow negative lux value
    if (val < 0) {
        val = 0;
    }

    // round lsb (2^(LUX_SCALE−1))
    // val += (1 << (LUX_SCALE−1));
    val += (1 << (LUX_SCALE-1));

    // strip off fractional portion
    uint32_t lux = (unsigned int)(val >> LUX_SCALE);

    return lux;
}
