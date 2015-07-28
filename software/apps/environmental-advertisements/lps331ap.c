#include <stdint.h>
#include "twi_master.h"
#include <stdbool.h>
#include <math.h>
#include "lps331ap.h"
#include <stdio.h>
#include <string.h>
#include "time.h"


#define REF_P_XL 0x08
#define REF_P_L 0x09
#define REF_P_H 0x0A

#define RES_CONF 0x10

#define CTRL_REG1 0x20 
#define POWER_ON   0x80
#define POWER_OFF   0x00
#define BLOCK_UPDATE_ENABLE 0x04


#define CTRL_REG2 0x21 
#define ONE_SHOT_ENABLE 0x01

#define CTRL_REG3 0x22 

#define INT_CFG_REG 0x23 
#define INT_SOURCE_REG 0x24 

#define THS_P_LOW_REG 0x25
#define THS_P_HIGH_REG 0x26

#define STATUS_REG 0x27 

#define PRESS_OUT_XL 0x28
#define PRESS_OUT_L 0x29 
#define PRESS_OUT_H 0x2A 

#define TEMP_OUT_L 0x2B
#define TEMP_OUT_H 0x2C 

#define AMP_CTRL 0x30 

// Read/Write def
#define TWI_READ                        0b1
#define TWI_WRITE                       0b0

#define SENSOR_ADDR	0xB8

#define AUTO_INCR	0x80



static lps331ap_data_rate_t data_rate_g;


void lps331ap_readPressure(float *pres){

	bool error = false;

    uint8_t command = PRESS_OUT_XL | AUTO_INCR;
    error = 
    twi_master_transfer(
            SENSOR_ADDR | TWI_WRITE,
            &command,
            sizeof(command),
            TWI_DONT_ISSUE_STOP
    );
    uint8_t pressure_data[3] = {0x00, 0x00, 0x00};
    error = 
    twi_master_transfer(
            SENSOR_ADDR | TWI_READ,
            pressure_data,
            sizeof(pressure_data),
            TWI_ISSUE_STOP
    );

    float pressure = (pressure_data[2] << 16) | (pressure_data[1] << 8) | (pressure_data[0]);

    pressure = pressure / 4096.0;


    pressure =    (0x00FFFFFF & (((uint32_t)pressure_data[2] << 16) |
                        ((uint32_t) pressure_data[1] << 8) |
                        ((uint32_t) pressure_data[0]))) / 4096.0;
    *pres = pressure;

}

void lps331ap_readTemp (float *temp){

	bool error = false;

    uint8_t command = TEMP_OUT_L | AUTO_INCR;
    error =
    twi_master_transfer(
            SENSOR_ADDR | TWI_WRITE,
            &command,
            sizeof(command),
            TWI_DONT_ISSUE_STOP
    );
    uint8_t temp_data[2] = {0x00, 0x00};

    error =
    twi_master_transfer(
            SENSOR_ADDR | TWI_READ,
            temp_data,
            sizeof(temp_data),
            TWI_ISSUE_STOP
    );

    float temperature =   ( (0x0000FFFF & (
                        ((uint32_t) temp_data[1] << 8) |
                        ((uint32_t) temp_data[0]))) / 480.0 ) + 42.5;
    *temp = temperature;

}

//resets everything
void lps331ap_power_off(){

	bool error = false;

	uint8_t command = CTRL_REG1;
	error =
    twi_master_transfer(
            SENSOR_ADDR | TWI_WRITE,
            &command,
            sizeof(command),
            TWI_DONT_ISSUE_STOP
    );

    uint8_t data = POWER_OFF;

    error =
    twi_master_transfer(
            SENSOR_ADDR | TWI_WRITE,
            &data,
            sizeof(data),
            TWI_ISSUE_STOP
    );

}


/*
void lps331ap_power_on_config(lps331ap_data_rate_t data_rate){


	lps331ap_power_off(); // it is necessary to turn off sensor before updating data rate

	uint8_t command = CTRL_REG1;
    twi_master_transfer(
            SENSOR_ADDR | TWI_WRITE,
            &command,
            sizeof(command),
            TWI_DONT_ISSUE_STOP
    );

	uint8_t data = (data_rate << 4) | BLOCK_UPDATE_ENABLE | POWER_ON;

    twi_master_transfer(
            SENSOR_ADDR | TWI_WRITE,
            &data,
            sizeof(data),
            TWI_ISSUE_STOP
    );


}
*/


void lps331ap_init(lps331ap_data_rate_t data_rate){


	lps331ap_power_off(); // it is necessary to turn off sensor before updating data rate

	data_rate_g = data_rate; 


}

// must only be called after init
void lps331ap_power_on(){

	bool error = false;

	uint8_t command = CTRL_REG1;
	error =
    twi_master_transfer(
            SENSOR_ADDR | TWI_WRITE,
            &command,
            sizeof(command),
            TWI_DONT_ISSUE_STOP
    );

	uint8_t data = (data_rate_g << 4) | BLOCK_UPDATE_ENABLE | POWER_ON;

	error =
    twi_master_transfer(
            SENSOR_ADDR | TWI_WRITE,
            &data,
            sizeof(data),
            TWI_ISSUE_STOP
    );


}


void lps331ap_one_shot(){

	bool error = false;

	uint8_t command = CTRL_REG2;
	error =
	twi_master_transfer(
            SENSOR_ADDR | TWI_WRITE,
            &command,
            sizeof(command),
            TWI_DONT_ISSUE_STOP
    );

	uint8_t data = ONE_SHOT_ENABLE;
	error =
    twi_master_transfer(
            SENSOR_ADDR | TWI_WRITE,
            &data,
            sizeof(data),
            TWI_ISSUE_STOP
    );


}