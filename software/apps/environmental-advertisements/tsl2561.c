#include <stdint.h>
#include "nrf_drv_twi.h"
#include <stdbool.h>
#include <math.h>
#include "tsl2561.h"
#include <stdio.h>
#include <string.h>
#include "time.h"
#include "app_error.h"


void sleep(unsigned int mseconds)
{
    clock_t goal = mseconds + clock();
    while (goal > clock());
}

//#define SENSOR_GND_ADDR 	0x52

#define SENSOR_GND_ADDR 0b00101001

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

static nrf_drv_twi_t * m_instance;

static int check_error(ret_code_t error){

	if (error == NRF_SUCCESS){
		return 0;
	}

	else if (error == NRF_ERROR_BUSY){
		return 1;
	}
	else if (error == NRF_ERROR_INTERNAL){
		return 2;
	}
	else return 3;

}

// Read ADC Channels Using Read Word Protocol
void tsl2561_readADC(uint16_t * channel0_data, uint16_t * channel1_data){

	ret_code_t error = NRF_SUCCESS;
	int e = 4;
	uint8_t data_low[1] = {0};
	uint8_t data_high[1] = {0};

	//uint8_t data[2] = {0x00, 0x00};

	//read byte from Ch0 lower data register
	uint8_t command[1] = {COMMAND_REG | CH0_LOW};

	error =
	nrf_drv_twi_tx(
        m_instance, 
        SENSOR_GND_ADDR,
		command, 
		sizeof(command), 
		true
	);

	e = check_error(error);

	printf("hey");

	error =
	nrf_drv_twi_rx(
        m_instance, 
        SENSOR_GND_ADDR,
		data_low,
		sizeof(data_low),
		false
	);

	e = check_error(error);

	//read byte from Ch0 higher data register
	command[0] = COMMAND_REG | CH0_HIGH;

	error =
	nrf_drv_twi_tx(
        m_instance, 
        SENSOR_GND_ADDR, 
		command, 
		sizeof(command), 
		true
	);

	e = check_error(error);

	error =
	nrf_drv_twi_rx(
        m_instance, 
        SENSOR_GND_ADDR,
		data_high,
		sizeof(data_high),
		false
	);

	e = check_error(error);

	//Shift data high to upper byte and set channel 0 data
	*channel0_data = 256 * data_high[0] + data_low[0];

	//read byte from Ch1 lower data register
	command[0] = COMMAND_REG | CH1_LOW;

	error =
	nrf_drv_twi_tx(
        m_instance, 
        SENSOR_GND_ADDR, 
		command, 
		sizeof(command), 
		true
	);

	e = check_error(error);

	error =
	nrf_drv_twi_rx(
        m_instance, 
        SENSOR_GND_ADDR,
		data_low,
		sizeof(data_low),
		false
	);

	e = check_error(error);

	//read byte from Ch1 higher data register
	command[0] = COMMAND_REG | CH1_HIGH;

	error =
	nrf_drv_twi_tx(
        m_instance, 
        SENSOR_GND_ADDR, 
		command, 
		sizeof(command), 
		true
	);

	e = check_error(error);


	error =
	nrf_drv_twi_rx(
        m_instance, 
        SENSOR_GND_ADDR,
		data_high,
		sizeof(data_high),
		false
	);

	e = check_error(error);


	//Shift data high to upper byte and set channel 1 data
	*channel1_data = 256 * data_high[0] + data_low[0];

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

	ret_code_t error = NRF_SUCCESS;

	//uint8_t command[] = {0xC0};

	uint8_t command[2] = 
	{ 
		COMMAND_REG | CONTROL_REG_ADDR,
		POWER_ON
	};


	//uint8_t lux_data[] = {0b11000000, 0b00000011};

    printf("hi");

    error =
    nrf_drv_twi_tx(
        m_instance, 
        SENSOR_GND_ADDR,
        command,
        sizeof(command),
        false
	);
    
	uint8_t command2[1] = {COMMAND_REG | CLEAR_INTERRUPT};

    error =
    nrf_drv_twi_tx(
        m_instance, 
        SENSOR_GND_ADDR,
        command2,
        sizeof(command2),
        false
	);

	//int a = 5;

   	
    printf("hey");

}

void tsl2561_off(void){

	ret_code_t error = NRF_SUCCESS;

	//uint8_t command[] = {0xC0};

	uint8_t command[2] = 
	{
		COMMAND_REG | CLEAR_INTERRUPT | CONTROL_REG_ADDR,
		POWER_OFF
	};

	//uint8_t lux_data[] = {0b11000000, 0b00000011};

    printf("hi");

    error =
    nrf_drv_twi_tx(
        m_instance, 
        SENSOR_GND_ADDR,
        command,
        sizeof(command),
        false
    );
    

	//int a = 5;

   	
    printf("hey");


}

void tsl2561_interrupt_test(){

	uint8_t command[2] = {0x00, 0x00};

	uint8_t data = 0x30; 
	command[0] = COMMAND_REG | INTERRUPT_CONTROL_REG_ADDR;

	command[1] = data;

	nrf_drv_twi_tx(
        m_instance, 
        SENSOR_GND_ADDR,
		command, 
		sizeof(command), 
		false
	);



}

void tsl2561_interrupt_enable(uint16_t * threshold_low, uint16_t * threshold_high){

	uint8_t command3[3] = {0x00, 0x00, 0x00};
	uint8_t command2[2] = {0x00, 0x00};
	uint8_t data1[1] = {0x00};
	uint8_t data2[2] = {0x00, 0x00};
	ret_code_t error = NRF_SUCCESS;

	if (threshold_low){

		//memcpy(data, threshold_low, 1);
		memcpy(data2, threshold_low, 2);

		command3[0] = COMMAND_REG| THRESHLOWLOW_REG_ADDR;
		command3[1] = data2[0];
		command3[2] = data2[1];

		error = 
        nrf_drv_twi_tx(
            m_instance, 
            SENSOR_GND_ADDR,
			command3, 
			sizeof(command3), 
			false
		);

        /*
		memcpy(&data, threshold_low+1, 1);
		command[0] = COMMAND_REG| THRESHLOWHIGH_REG_ADDR;
		command[1] = data;

		error = 
        nrf_drv_twi_tx(
            m_instance, 
            SENSOR_GND_ADDR,
			command, 
			sizeof(command), 
			false
		);
		*/

	}

	if (threshold_high){

		memcpy(data2, threshold_high, 2);
		command3[0] = COMMAND_REG| THRESHHIGHLOW_REG_ADDR;
		command3[1] = data2[0];
		command3[2] = data2[1];

		//error = 
		nrf_drv_twi_tx(
            m_instance, 
            SENSOR_GND_ADDR,
			command3, 
			sizeof(command3), 
			false
		); 

		/*
		memcpy(data, threshold_high+1, 1);
		command[0] = COMMAND_REG| THRESHHIGHHIGH_REG_ADDR;
		command[1] = data[0];

		error = 
        nrf_drv_twi_tx(
            m_instance, 
            SENSOR_GND_ADDR,
			command, 
			sizeof(command), 
			false
		);
		*/

	}

	if ((threshold_low) || (threshold_high)){
		
		data1[0] = INTERRUPT_LEVEL_INTERRUPT;
		command2[0] = COMMAND_REG| INTERRUPT_CONTROL_REG_ADDR;
		command2[1] = data1[0];

		error = 
        nrf_drv_twi_tx(
            m_instance, 
            SENSOR_GND_ADDR,
			command2, 
			sizeof(command2), 
			false
		);

	}

}

void tsl2561_interrupt_disable(void){

	ret_code_t error = NRF_SUCCESS;

	uint8_t data = INTERRUPT_OUTPUT_DISABLED;
	uint8_t command[2] = 
	{
		COMMAND_REG | CLEAR_INTERRUPT | INTERRUPT_CONTROL_REG_ADDR,
		data
	};


	error = 
    nrf_drv_twi_tx(
        m_instance, 
        SENSOR_GND_ADDR,
		command, 
		sizeof(command), 
		false
	);

	printf("end");

}

void tsl2561_init(nrf_drv_twi_t * p_instance){

	m_instance = p_instance;

}


void tsl2561_config(tsl2561_gain_mode_t mode_gain, tsl2561_integration_time_mode_t mode_time){

	ret_code_t error = NRF_SUCCESS;

	uint8_t data;

	if (mode_gain == tsl2561_HIGH){

		data = HIGH_GAIN_MODE; // high gain mode (16x)

	}

	else {

		data = LOW_GAIN_MODE; // low gain mode (1x)

	}
	
	data = data | mode_time;

	uint8_t command[2] = 
	{
		COMMAND_REG | TIMING_REG_ADDR,
		data
	};

	error = 
    nrf_drv_twi_tx(
        m_instance, 
        SENSOR_GND_ADDR, 
		command, 
		sizeof(command), 
		false
	);

}


void tsl2561_config_manual(tsl2561_gain_mode_t mode_gain, uint32_t man_time){

	uint8_t data;
	ret_code_t error = NRF_SUCCESS;

	if (mode_gain == tsl2561_HIGH){

		data = HIGH_GAIN_MODE; // high gain mode (16x)

	}

	else {

		data = LOW_GAIN_MODE; // low gain mode (1x)

	}
	
	data = data | MANUAL_INTEGRATION | BEGIN_INTEGRATION;

	uint8_t command[2] =
	{
		COMMAND_REG | TIMING_REG_ADDR,
		data
	};

	error = 
    nrf_drv_twi_tx(
        m_instance, 
        SENSOR_GND_ADDR, 
		command, 
		sizeof(command), 
		false
	);

	sleep(man_time);

	data = data & (STOP_INTEGRATION);

	command[1] = data;

	//is this one necessary??
	error = 
    nrf_drv_twi_tx(
        m_instance, 
        SENSOR_GND_ADDR, 
		command, 
		sizeof(command), 
		false
	);

}