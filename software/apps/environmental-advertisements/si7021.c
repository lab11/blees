#include <stdint.h>
#include "twi_master.h"
#include <stdbool.h>
#include <math.h>
#include "si7021.h"
#include <stdio.h>
#include <string.h>
#include "time.h"

#define Meas_RH_Hold_Master 0xE5
//Measure Relative Humidity, Hold Master Mode

#define Meas_RH_No_Hold_Master 0xF5
//Measure Relative Humidity, No Hold Master Mode

#define Meas_Temp_Hold_Master 0xE3
//Measure Temperature, Hold Master Mode

#define Meas_Temp_No_Hold_Master 0xF3
//Measure Temperature, No Hold Master Mode

#define Read_Temp_From_Prev_RH 0xE0
//Read Temperature Value from Previous RH Measurement

#define Reset 0xFE
//Reset

#define Write_User_Reg_1 0xE6
//Write RH/T User Register 1

#define Read_User_Reg_1 0xE7
//Read RH/T User Register 1

#define	Read_ID_Byte_1	0xFA 0x0F
//Read Electronic ID 1st

#define Read_ID_Byte_2	0xFC 0xC9
//Read Electronic ID 2nd

#define Read_Firm_Rev	0x84 0xB8
//Read Firmware Revision

#define HEATER_ON 	0x04
#define HEATER_OFF  ~(HEATER_ON)

#define TEMP_HUM_ADDR	0x80

#define TWI_READ 0b1
#define TWI_WRITE 0b0


void si7021_reset(){

	uint8_t data = Reset;
    twi_master_transfer(
            TEMP_HUM_ADDR | TWI_WRITE,
            &data,
            sizeof(data),
            TWI_ISSUE_STOP
    );

}

void si7021_read_temp_hold(float * temp){

	uint8_t command = Meas_Temp_Hold_Master;
	twi_master_transfer(
        TEMP_HUM_ADDR | TWI_WRITE,
        &command,
        sizeof(command),
        TWI_DONT_ISSUE_STOP
    );
	uint8_t temp_hum_data[3] = {0, 0, 0};
    twi_master_transfer(
        TEMP_HUM_ADDR | TWI_READ,
        temp_hum_data,
        sizeof(temp_hum_data),
        TWI_ISSUE_STOP
    );

    float temperature = -46.85 + (175.72 * (((uint32_t) temp_hum_data[0] << 8) | ((uint32_t) temp_hum_data[1] & 0xfc)) / (1 << 16));
    *temp = temperature;

}


void si7021_read_temp(float *temp){

	uint8_t command = Meas_Temp_No_Hold_Master;
	twi_master_transfer(
        TEMP_HUM_ADDR | TWI_WRITE,
        &command,
        sizeof(command),
        TWI_DONT_ISSUE_STOP
    );
	uint8_t temp_hum_data[3] = {0, 0, 0};
    while (
        !twi_master_transfer(
                TEMP_HUM_ADDR | TWI_READ,
                temp_hum_data,
                sizeof(temp_hum_data),
                TWI_ISSUE_STOP
        )) {for(int i = 0; i < 10000; ++i) {}}

    float temperature = -46.85 + (175.72 * (((uint32_t) temp_hum_data[0] << 8) | ((uint32_t) temp_hum_data[1] & 0xfc)) / (1 << 16));
    *temp = temperature;

}

//is this correct for hold ?
void si7021_read_RH_hold(float *hum){

	uint8_t command = Meas_RH_Hold_Master;
    twi_master_transfer(
        TEMP_HUM_ADDR | TWI_WRITE,
        &command,
        sizeof(command),
        TWI_DONT_ISSUE_STOP
    );
    uint8_t temp_hum_data[3] = {0, 0, 0};
    twi_master_transfer(
        TEMP_HUM_ADDR | TWI_READ,
        temp_hum_data,
        sizeof(temp_hum_data),
        TWI_ISSUE_STOP
    );

    float humidity = -6.0 + ((125.0 / (1 << 16)) * (((uint32_t) temp_hum_data[0] << 8) | ((uint32_t) temp_hum_data[1] & 0xf0)));
    *hum = humidity;

}


void si7021_read_RH(float *hum){

	uint8_t command = Meas_RH_No_Hold_Master;
    twi_master_transfer(
            TEMP_HUM_ADDR | TWI_WRITE,
            &command,
            sizeof(command),
            TWI_DONT_ISSUE_STOP
    );
    uint8_t temp_hum_data[3] = {0, 0, 0};
    while (!twi_master_transfer(
            TEMP_HUM_ADDR | TWI_READ,
            temp_hum_data,
            sizeof(temp_hum_data),
            TWI_ISSUE_STOP
    )) {for (int i = 0; i < 10000; ++i) {}}

    float humidity = -6.0 + ((125.0 / (1 << 16)) * (((uint32_t) temp_hum_data[0] << 8) | ((uint32_t) temp_hum_data[1] & 0xf0)));
    *hum = humidity;

}


void si7021_read_temp_after_RH(float *temp){

	uint8_t command = Read_Temp_From_Prev_RH;
	twi_master_transfer(
        TEMP_HUM_ADDR | TWI_WRITE,
        &command,
        sizeof(command),
        TWI_DONT_ISSUE_STOP
    );
	uint8_t temp_hum_data[3] = {0, 0, 0};
    twi_master_transfer(
        TEMP_HUM_ADDR | TWI_READ,
        temp_hum_data,
        sizeof(temp_hum_data),
        TWI_ISSUE_STOP
    );

    float temperature = -46.85 + (175.72 * (((uint32_t) temp_hum_data[0] << 8) | ((uint32_t) temp_hum_data[1] & 0xfc)) / (1 << 16));
    *temp = temperature;

}

void si7021_read_temp_and_RH(float *temp, float *hum){

	si7021_read_RH(hum);
	//employ read temp after RH shortcut
	si7021_read_temp_after_RH(temp);

}

void si7021_read_user_reg(uint8_t *reg_status){

	uint8_t command = Read_User_Reg_1;
	twi_master_transfer(
        TEMP_HUM_ADDR | TWI_WRITE,
        &command,
        sizeof(command),
        TWI_DONT_ISSUE_STOP
    );
    twi_master_transfer(
        TEMP_HUM_ADDR | TWI_READ,
        reg_status,
        sizeof(uint8_t),
        TWI_ISSUE_STOP
    );


}

void si7021_config(si7021_meas_res_t res_mode){

	uint8_t res1, res0;
	uint8_t command;
	uint8_t reg_status;


	res1 = (res_mode & 0x2) << 6;
	res0 = res_mode & 0x1;

	si7021_read_user_reg(&reg_status);

    uint8_t data = reg_status | res1 | res0;

    command = Write_User_Reg_1;
	twi_master_transfer(
        TEMP_HUM_ADDR | TWI_WRITE,
        &command,
        sizeof(command),
        TWI_DONT_ISSUE_STOP
    );
	twi_master_transfer(
        TEMP_HUM_ADDR | TWI_WRITE,
        &data,
        sizeof(data),
        TWI_ISSUE_STOP
    );

}

void si7021_heater_on(){

	uint8_t reg_status;

	si7021_read_user_reg(&reg_status);

	uint8_t data = reg_status | HEATER_ON;

	uint8_t command = Write_User_Reg_1;
	twi_master_transfer(
        TEMP_HUM_ADDR | TWI_WRITE,
        &command,
        sizeof(command),
        TWI_DONT_ISSUE_STOP
    );
	twi_master_transfer(
        TEMP_HUM_ADDR | TWI_WRITE,
        &data,
        sizeof(data),
        TWI_ISSUE_STOP
    );

}

void si7021_heater_off(){

	uint8_t reg_status;

	si7021_read_user_reg(&reg_status);

	uint8_t data = reg_status & HEATER_OFF;

	uint8_t command = Write_User_Reg_1;
	twi_master_transfer(
        TEMP_HUM_ADDR | TWI_WRITE,
        &command,
        sizeof(command),
        TWI_DONT_ISSUE_STOP
    );
	twi_master_transfer(
        TEMP_HUM_ADDR | TWI_WRITE,
        &data,
        sizeof(data),
        TWI_ISSUE_STOP
    );


}