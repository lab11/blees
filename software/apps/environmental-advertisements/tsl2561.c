#include <stdint.h>
#include "twi_master.h"
#include <stdbool.h>
#include <math.h>
#include "tsl2561.h"
#include <stdio.h>
#include <string.h>
#include "time.h"

void sleep(unsigned int mseconds)
{
    clock_t goal = mseconds + clock();
    while (goal > clock());
}

//#define SENSOR_GND_ADDR 	0x52

#define SENSOR_GND_ADDR 0b01010010

#define COMMAND_REG			0x80
#define CLEAR_INTERRUPT		0x40
#define	WORD_PROTOCOL 		0x20
#define	BLOCK_PROTOCOL		0x10
#define CH0_LOW 	  		0xC
#define CH0_HIGH 			0xD
#define	CH1_LOW				0xE
#define CH1_HIGH			0xF

#define CONTROL_REG_ADDR	0x0
#define POWER_ON			0x03
#define	POWER_OFF			0x00

#define TIMING_REG_ADDR		0x1
#define HIGH_GAIN_MODE		0x10
#define LOW_GAIN_MODE		0x00
#define	MANUAL_INTEGRATION	0x03
#define	BEGIN_INTEGRATION	0x08
#define	STOP_INTEGRATION	~BEGIN_INTEGRATION

#define THRESHLOWLOW_REG_ADDR	0x2
#define THRESHLOWHIGH_REG_ADDR	0x3
#define	THRESHHIGHLOW_REG_ADDR	0x4
#define THRESHHIGHHIGH_REG_ADDR	0x5	

#define INTERRUPT_CONTROL_REG_ADDR	0x6
#define	INTERRUPT_OUTPUT_DISABLED	0x00
#define INTERRUPT_LEVEL_INTERRUPT	0x10

#define TWI_READ 0b1
#define TWI_WRITE 0b0

	command = COMMAND_REG | BLOCK_PROTOCOL | TIMING_REG_ADDR;


// Read ADC Channels Using Read Word Protocol
void tsl2561_readADC(uint16_t * channel0_data, uint16_t * channel1_data){

	bool error = false;
	uint8_t data_low = 0;
	uint8_t data_high = 0;
	uint8_t command;

	uint8_t address;

	//read byte from Ch0 lower data register
	command = COMMAND_REG| BLOCK_PROTOCOL | CH0_LOW;

	address = SENSOR_GND_ADDR | TWI_WRITE;

	error = 
	twi_master_transfer(
		address, 
		&command, 
		sizeof(command), 
		TWI_DONT_ISSUE_STOP
	);

	printf("hey");

	error = 
	twi_master_transfer(
		SENSOR_GND_ADDR | TWI_READ,
		&data_low,
		sizeof(data_low),
		TWI_ISSUE_STOP
	);

	//read byte from Ch0 higher data register
	command = COMMAND_REG | WORD_PROTOCOL | CH0_HIGH;

	error = 
	twi_master_transfer(
		SENSOR_GND_ADDR | TWI_WRITE, 
		&command, 
		sizeof(command), 
		TWI_ISSUE_STOP
	);

	error = 
	twi_master_transfer(
		SENSOR_GND_ADDR | TWI_READ,
		&data_high,
		sizeof(data_high),
		TWI_ISSUE_STOP
	);

	//Shift data high to upper byte and set channel 0 data
	*channel0_data = 256 * data_high + data_low;

	//read byte from Ch1 lower data register
	command = COMMAND_REG | WORD_PROTOCOL | CH1_LOW;

	error = 
	twi_master_transfer(
		SENSOR_GND_ADDR | TWI_WRITE, 
		&command, 
		sizeof(command), 
		TWI_ISSUE_STOP
	);

	error = 
	twi_master_transfer(
		SENSOR_GND_ADDR | TWI_READ,
		&data_low,
		sizeof(data_low),
		TWI_ISSUE_STOP
	);

	//read byte from Ch1 higher data register
	command = COMMAND_REG | WORD_PROTOCOL | CH1_HIGH;

	error = 
	twi_master_transfer(
		SENSOR_GND_ADDR | TWI_WRITE, 
		&command, 
		sizeof(command), 
		TWI_ISSUE_STOP
	);

	error = 
	twi_master_transfer(
		SENSOR_GND_ADDR | TWI_READ,
		&data_high,
		sizeof(data_high),
		TWI_ISSUE_STOP
	);

	//Shift data high to upper byte and set channel 1 data
	*channel1_data = 256 * data_high + data_low;

	printf("hi");

}

//currently doesn't work for manual integration
float tsl2561_readLux(tsl2561_integration_time_mode_t int_mode){

	uint16_t chan0, chan1;
	float lux = 0.0;

	tsl2561_readADC(&chan0, &chan1);

	if (int_mode == tsl2561_MODE0){
		chan0 = chan0 / 0.034;
		chan1 = chan1 / 0.034;
	}
	else if (int_mode == tsl2561_MODE1){
		chan0 = chan0 / 0.252;
		chan1 = chan1 / 0.252;
	}

	float ratio = ((float) chan1) / chan0;

   	if (ratio <= 0.50) {
        lux = (0.0304 * chan0) - (0.062 * chan0 * (pow(ratio, 1.4)));
    } else if (ratio <= 0.61) {
        lux = (0.0224 * chan0) - (0.031 * chan1);
    } else if (ratio <= 0.80) {
        lux = (0.0128 * chan0) - (0.0153 * chan1);
    } else if (ratio <= 1.3) {
        lux = (0.00146 * chan0) - (0.00112 * chan1);
    }

    return lux;

}

void tsl2561_on(void){

	bool error = false;

	//uint8_t command[] = {0xC0};

	uint8_t command = COMMAND_REG | CLEAR_INTERRUPT | CONTROL_REG_ADDR;

	//uint8_t lux_data[] = {0b11000000, 0b00000011};

    printf("hi");

    
   	error = twi_master_transfer(
            SENSOR_GND_ADDR | TWI_WRITE,
            &command,
            sizeof(command),
            TWI_DONT_ISSUE_STOP
    );
    

	//int a = 5;

   	
    printf("hey");

    
    uint8_t data = POWER_ON;

   	error = twi_master_transfer(
            SENSOR_GND_ADDR | TWI_WRITE,
            &data,
            sizeof(data),
            TWI_ISSUE_STOP
    );
    

}

void tsl2561_off(void){

	bool error = false;

	//uint8_t command[] = {0xC0};

	uint8_t command = COMMAND_REG | CLEAR_INTERRUPT | CONTROL_REG_ADDR;

	//uint8_t lux_data[] = {0b11000000, 0b00000011};

    printf("hi");

    
   	error = twi_master_transfer(
            SENSOR_GND_ADDR | TWI_WRITE,
            &command,
            sizeof(command),
            TWI_DONT_ISSUE_STOP
    );
    

	//int a = 5;

   	
    printf("hey");

    
    uint8_t data = POWER_OFF;

   	error = twi_master_transfer(
            SENSOR_GND_ADDR | TWI_WRITE,
            &data,
            sizeof(data),
            TWI_ISSUE_STOP
    );

}

void tsl2561_interrupt_enable(uint16_t * threshold_low, uint16_t * threshold_high){

	uint8_t command;
	uint8_t data;

	if (threshold_low){

		memcpy(&data, threshold_low, 1);
		command = COMMAND_REG| BLOCK_PROTOCOL | THRESHLOWLOW_REG_ADDR;

		//error = 
		twi_master_transfer(
			SENSOR_GND_ADDR | TWI_WRITE, 
			&command, 
			sizeof(command), 
			TWI_ISSUE_STOP
		);

		//error = 
		twi_master_transfer(
			SENSOR_GND_ADDR | TWI_WRITE,
			&data,
			sizeof(data),
			TWI_ISSUE_STOP
		); 

		memcpy(&data, threshold_low+1, 1);
		command = COMMAND_REG| BLOCK_PROTOCOL | THRESHLOWHIGH_REG_ADDR;

		//error = 
		twi_master_transfer(
			SENSOR_GND_ADDR | TWI_WRITE, 
			&command, 
			sizeof(command), 
			TWI_ISSUE_STOP
		);

		//error = 
		twi_master_transfer(
			SENSOR_GND_ADDR | TWI_WRITE,
			&data,
			sizeof(data),
			TWI_ISSUE_STOP
		);

	}

	if (threshold_high){

		memcpy(&data, threshold_high, 1);
		command = COMMAND_REG| BLOCK_PROTOCOL | THRESHHIGHLOW_REG_ADDR;

		//error = 
		twi_master_transfer(
			SENSOR_GND_ADDR | TWI_WRITE, 
			&command, 
			sizeof(command), 
			TWI_ISSUE_STOP
		);

		//error = 
		twi_master_transfer(
			SENSOR_GND_ADDR | TWI_WRITE,
			&data,
			sizeof(data),
			TWI_ISSUE_STOP
		); 

		memcpy(&data, threshold_high+1, 1);
		command = COMMAND_REG| BLOCK_PROTOCOL | THRESHHIGHHIGH_REG_ADDR;

		//error = 
		twi_master_transfer(
			SENSOR_GND_ADDR | TWI_WRITE, 
			&command, 
			sizeof(command), 
			TWI_ISSUE_STOP
		);

		//error = 
		twi_master_transfer(
			SENSOR_GND_ADDR | TWI_WRITE,
			&data,
			sizeof(data),
			TWI_ISSUE_STOP
		);

	}

	if ((threshold_low) || (threshold_high)){
		
		data = INTERRUPT_LEVEL_INTERRUPT;
		command = COMMAND_REG| BLOCK_PROTOCOL | INTERRUPT_CONTROL_REG_ADDR;

		//error = 
		twi_master_transfer(
			SENSOR_GND_ADDR | TWI_WRITE, 
			&command, 
			sizeof(command), 
			TWI_ISSUE_STOP
		);

		//error = 
		twi_master_transfer(
			SENSOR_GND_ADDR | TWI_WRITE,
			&data,
			sizeof(data),
			TWI_ISSUE_STOP
		);

	}

}

void tsl2561_interrupt_disable(void){

	uint8_t data = INTERRUPT_OUTPUT_DISABLED;
	uint8_t command = COMMAND_REG | CLEAR_INTERRUPT | BLOCK_PROTOCOL | INTERRUPT_CONTROL_REG_ADDR;

	//error = 
	twi_master_transfer(
		SENSOR_GND_ADDR | TWI_WRITE, 
		&command, 
		sizeof(command), 
		TWI_ISSUE_STOP
	);

	//error = 
	twi_master_transfer(
		SENSOR_GND_ADDR | TWI_WRITE,
		&data,
		sizeof(data),
		TWI_ISSUE_STOP
	);

}

void tsl2561_config(tsl2561_gain_mode_t mode_gain, tsl2561_integration_time_mode_t mode_time){

	uint8_t data;
	uint8_t command;

	if (mode_gain == tsl2561_HIGH){

		data = HIGH_GAIN_MODE; // high gain mode (16x)

	}

	else {

		data = LOW_GAIN_MODE; // low gain mode (1x)

	}
	
	data = data | mode_time;

	command = COMMAND_REG | BLOCK_PROTOCOL | TIMING_REG_ADDR;

	//error = 
	twi_master_transfer(
		SENSOR_GND_ADDR | TWI_WRITE, 
		&command, 
		sizeof(command), 
		TWI_ISSUE_STOP
	);

	//error = 
	twi_master_transfer(
		SENSOR_GND_ADDR | TWI_WRITE,
		&data,
		sizeof(data),
		TWI_ISSUE_STOP
	);

}


void tsl2561_config_manual(tsl2561_gain_mode_t mode_gain, uint32_t man_time){

	uint8_t data;
	uint8_t command;

	if (mode_gain == tsl2561_HIGH){

		data = HIGH_GAIN_MODE; // high gain mode (16x)

	}

	else {

		data = LOW_GAIN_MODE; // low gain mode (1x)

	}
	
	data = data | MANUAL_INTEGRATION | BEGIN_INTEGRATION;

	command = COMMAND_REG | BLOCK_PROTOCOL | TIMING_REG_ADDR;

	//error = 
	twi_master_transfer(
		SENSOR_GND_ADDR | TWI_WRITE, 
		&command, 
		sizeof(command), 
		TWI_ISSUE_STOP
	);

	//error = 
	twi_master_transfer(
		SENSOR_GND_ADDR | TWI_WRITE,
		&data,
		sizeof(data),
		TWI_ISSUE_STOP
	);

	sleep(man_time);

	data = data & (STOP_INTEGRATION);

	//is this one necessary??
	//error = 
	twi_master_transfer(
		SENSOR_GND_ADDR | TWI_WRITE, 
		&command, 
		sizeof(command), 
		TWI_ISSUE_STOP
	);

	//error = 
	twi_master_transfer(
		SENSOR_GND_ADDR | TWI_WRITE,
		&data,
		sizeof(data),
		TWI_ISSUE_STOP
	);

}