#include "nrf_gpio.h"
#include <stdio.h>
#include "adxl362.h"
#include "app_error.h"
#include <stdint.h>


//Addresses of Registers
#define DEVID_AD 0x00

#define DEVID_MST 0x01

#define PARTID 0x02

#define REVID 0x03

#define XDATA 0x08

#define YDATA 0x09

#define ZDATA 0x0A

#define STATUS 0x0B

#define FIFO_ENTRIES_L 0x0C
#define FIFO_ENTRIES_H 0x0D

#define XDATA_L 0x0E
#define XDATA_H 0x0F

#define YDATA_L 0x10
#define YDATA_H 0x11

#define ZDATA_L 0x12
#define ZDATA_H 0x13

#define TEMP_L 0x14
#define TEMP_H 0x15

#define SOFT_RESET 0x1F
#define RESET_CODE 0x52 // for soft_reset reg

#define THRESH_ACT_L 0x20
#define THRESH_ACT_H 0x21

#define TIME_ACT 0x22

#define THRESH_INACT_L 0x23
#define THRESH_INACT_H 0x24

#define TIME_INACT_L 0x25
#define TIME_INACT_H 0x26

#define ACT_INACT_CTL 0x27
#define DEFAULT_MODE 0x00 // for act_inact_ctl
#define LINKED_MODE 0x10 // for act_inact_ctl
#define LOOP_MODE 0x30 // for act_inact_ctl
#define INACT_EN 0x04 // for act_inact_ctl
#define ACT_EN 0x01 // for act_inact_ctl
#define INACT_REF_EN 0x08 // for act_inact_ctl
#define ACT_REF_EN 0x02 // for act_inact_ctl

#define FIFO_CONTROL 0x28
#define STORE_TEMP_MODE 0x04 // for fifo_control
#define GREATER_THAN_255 0x08 // for fifo control

#define FIFO_SAMPLES 0x29

#define INTMAP1 0x2A

#define INTMAP2 0x2B

#define FILTER_CTL 0x2C

#define POWER_CTL 0x2D
#define STANDBY_MODE 0x00 // for power_ctl reg
#define MEASUREMENT_MODE 0x02 // for power_ctl_reg
#define AUTOSLEEP_MODE_EN 0x03
#define WAKEUP_MODE_EN 0x08

#define SELF_TEST 0x2E

//Commands
#define WRITE_REG 0x0A
#define READ_REG 0x0B
#define READ_FIFO 0x0D


static void spi_write(uint8_t buf) {
    //clear the ready event
    NRF_SPI->EVENTS_READY = 0;

    NRF_SPI->TXD = buf;

    //wait until byte has transmitted
    while(NRF_SPI->EVENTS_READY == 0);

    uint8_t throw = NRF_SPI->RXD;

    NRF_SPI->EVENTS_READY = 0;
}

static void spi_read(uint8_t* buf) {

    //clear ready event
    NRF_SPI->EVENTS_READY = 0;

    NRF_SPI->TXD = 0x00;

    //wait until byte has been received
    while(NRF_SPI->EVENTS_READY == 0);

    buf[0] = NRF_SPI->RXD;

    NRF_SPI->EVENTS_READY = 0;
}

static void spi_read_reg(uint8_t reg_addr, uint8_t * data, uint8_t num_bytes){

	nrf_gpio_pin_clear(SPI_SS_PIN);
    spi_write(READ_REG);
    spi_write(reg_addr); // STATUS

    int i = 0;
    do{
    	spi_read(data + i);
    	i++;
    } while (i < num_bytes);

    nrf_gpio_pin_set(SPI_SS_PIN);

}

static void spi_write_reg(uint8_t reg_addr, uint8_t * data, uint8_t num_bytes){

	nrf_gpio_pin_clear(SPI_SS_PIN);
    spi_write(WRITE_REG);
    spi_write(reg_addr); // STATUS

    int i = 0;
    do{
    	spi_write(data[i]);
    	i++;
    } while (i < num_bytes);

    nrf_gpio_pin_set(SPI_SS_PIN);

}

void config_interrupt_mode(interrupt_mode i_mode, bool use_referenced_activity, bool use_referenced_inactivity){

	uint8_t data[1] = {0x00};

	if (i_mode == LINKED){
		data[0] = LINKED_MODE;
	}
	else if (i_mode == LOOP){
		data[0] = LOOP_MODE;
	}
	//else default;

	if (use_referenced_inactivity){
		data[0] = data[0] | INACT_REF_EN; 
	}
	if (use_referenced_activity){
		data[0] = data[0] | ACT_REF_EN;
	}

	spi_write_reg(ACT_INACT_CTL, data, 1);

}

//if intmap_1 = 1, config for intpin 1
// if intmap_1 = 0, config for intpin 2
void config_INTMAP(interrupt_map_t * int_map, bool intmap_1){

	uint8_t data[1] = {0x00};

	if (int_map->DATA_READY){
		data[0] = 0x01;
	} 
	if (int_map->FIFO_READY){
		data[0] = data[0] | 0x02;
	}
	if (int_map->FIFO_WATERMARK){
		data[0] = data[0] | 0x04;
	}
	if (int_map->FIFO_OVERRUN){
		data[0] = data[0] | 0x08;
	}
	if (int_map->ACT){
		data[0] = data[0] | 0x10;
	}
	if (int_map->INACT){
		data[0] = data[0] | 0x20;
	}
	if (int_map->AWAKE){
		data[0] = data[0] | 0x40;
	}
	if (int_map->INT_LOW){
		data[0] = data[0] | 0x80;
	}


	if (intmap_1){
		spi_write_reg(INTMAP1, data, 1);
	}
	else {
		spi_write_reg(INTMAP2, data, 1);
	}

}



//only 11 bits are used
void set_activity_threshold(uint16_t act_threshold){

	uint8_t data[1] = { 0x0F & act_threshold};
	spi_write_reg(THRESH_ACT_L, data, 1);

	data[0] = 0xF0 & act_threshold;
	spi_write_reg(THRESH_ACT_H, data, 1);

}

void set_inactivity_threshold(uint16_t inact_threshold){

	uint8_t data[1] = { 0x0F & inact_threshold};
	spi_write_reg(THRESH_INACT_L, data, 1);
	
	data[0] = 0xF0 & inact_threshold;
	spi_write_reg(THRESH_INACT_H, data, 1);

}

//ignored if device is on wake-up mode
void set_inactivity_time(uint16_t inact_time){

	uint8_t data[2];
	data[0] = 0x00FF & inact_time;
	data[1] = (0xFF00 & inact_time) >> 15;

	spi_write_reg(TIME_INACT_L, data, 1);
	spi_write_reg(TIME_INACT_H, data + 1, 1);


}

void set_activity_time(uint8_t act_time){

	uint8_t data[1] = {act_time};

	spi_write_reg(TIME_ACT, data, 1);

}

void activity_interrupt_enable(){

	uint8_t data[1] = {0x00};

	spi_read_reg(ACT_INACT_CTL, data, 1);

	data[0] = data[0] | ACT_EN;

	spi_write_reg(ACT_INACT_CTL, data, 1);

}

void inactivity_interrupt_enable(){

	uint8_t data[1] = {0x00};

	spi_read_reg(ACT_INACT_CTL, data, 1);

	data[0] = data[0] | INACT_EN;

	spi_write_reg(ACT_INACT_CTL, data, 1);

}

void activity_inactivity_interrupt_enable(){

	uint8_t data[1] = {0x00};

	spi_read_reg(ACT_INACT_CTL, data, 1);

	data[0] = data[0] | INACT_EN | ACT_EN;

	spi_write_reg(ACT_INACT_CTL, data, 1);

}

void config_FIFO(fifo_mode f_mode, bool store_temp, uint16_t num_samples){

	uint8_t data[1] = { num_samples & 0x00FF};

	spi_write_reg(FIFO_SAMPLES, data, 1);

	data[0] = f_mode;

	if (store_temp){

		data[0] = data[0] | STORE_TEMP_MODE;

	}

	if ( num_samples > 255){

		data[0] = data[0] | GREATER_THAN_255; //AH bit set
	}

	spi_write_reg(FIFO_CONTROL, data, 1);

}

/**********SAMPLE 8 MSB OF DATA***********/

void sample_accel_byte_x(uint8_t * x_data){

	spi_read_reg(XDATA, x_data, 1);

}

void sample_accel_byte_y(uint8_t * y_data){

	spi_read_reg(YDATA, y_data, 1);

}

void sample_accel_byte_z(uint8_t * z_data){

	spi_read_reg(ZDATA, z_data, 1);

}


/**********SAMPLE 16 BITS OF DATA***********/

void sample_accel_word_x(uint8_t * x_data){

	spi_read_reg(XDATA_L, x_data, 1);

	spi_read_reg(XDATA_H, x_data + 1, 1);

}

void sample_accel_word_y(uint8_t * y_data){

	spi_read_reg(YDATA_L, y_data, 1);

	spi_read_reg(YDATA_H, y_data + 1, 1);

}

void sample_accel_word_z(uint8_t * z_data){

	spi_read_reg(ZDATA_L, z_data, 1);

	spi_read_reg(ZDATA_H, z_data + 1, 1);

}


uint8_t read_status_reg(){

	uint8_t status_reg_data[1] = {0};

	spi_read_reg(STATUS, status_reg_data, 1);

	return status_reg_data[0];


}

void sample_accel_word(uint8_t * x_data, uint8_t * y_data, uint8_t * z_data){

	sample_accel_word_x(x_data);

	sample_accel_word_y(y_data);

	sample_accel_word_z(z_data);

}


void sample_accel_byte(uint8_t * x_data, uint8_t * y_data, uint8_t * z_data){

	sample_accel_byte_x(x_data);

	sample_accel_byte_y(y_data);

	sample_accel_byte_z(z_data);

}

//if measure = 0 standby mode, if measure = 1, measurement mode

void accelerometer_init(noise_mode n_mode, bool measure, bool autosleep_en, bool wakeup_en){

    // send a soft reset to the accelerometer
    uint8_t data[1] = {RESET_CODE};
    spi_write_reg(SOFT_RESET, data, 1);

    if (measure){
    	data[0] = MEASUREMENT_MODE;
    }
    if (autosleep_en){
    	data[0] = data[0] | AUTOSLEEP_MODE_EN;
    }
    if (wakeup_en){
    	data[0] = data[0] | WAKEUP_MODE_EN;
    }
    data[0] = data[0] | (n_mode << 4);

    spi_write_reg(POWER_CTL, data, 1);

}

void config_measurement_range(measurement_range m_range){

	uint8_t data[1] = {0x00};

	spi_read_reg(FILTER_CTL, data, 1);	

	data[0] = data[0] & 0x3F;

	data[0] =  data[0] | (m_range << 6);

	spi_write_reg(FILTER_CTL, data, 1);


}


void spi_init(){
    // initialize spi
    nrf_gpio_cfg_output(SPI_SS_PIN);
    nrf_gpio_pin_set(SPI_SS_PIN);
	NRF_SPI->PSELSCK 	= 	SPI_SCK_PIN;
	NRF_SPI->PSELMOSI 	= 	SPI_MOSI_PIN;
	NRF_SPI->PSELMISO 	= 	SPI_MISO_PIN;
	NRF_SPI->FREQUENCY	= 	SPI_FREQUENCY_FREQUENCY_M1;
	NRF_SPI->CONFIG	= 	(uint32_t)(SPI_CONFIG_CPHA_Leading << SPI_CONFIG_CPHA_Pos) |
						(SPI_CONFIG_CPOL_ActiveHigh << SPI_CONFIG_CPOL_Pos) |
						(SPI_CONFIG_ORDER_MsbFirst << SPI_CONFIG_ORDER_Pos);
	NRF_SPI->ENABLE = (SPI_ENABLE_ENABLE_Enabled << SPI_ENABLE_ENABLE_Pos);
	NRF_SPI->EVENTS_READY = 0;

}