#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "nrf_drv_twi.h"
#include "time.h"

#include "lps331ap.h"

//Register defines
#define REF_P_XL 0x08
#define REF_P_L 0x09
#define REF_P_H 0x0A
#define RES_CONF 0x10
#define CTRL_REG1 0x20 
#define CTRL_REG2 0x21 
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

//Ctrl_Reg1 defines
#define POWER_ON   0x80
#define POWER_OFF   0x00
#define BLOCK_UPDATE_ENABLE 0x04

//Ctrl_Reg2 defines
#define ONE_SHOT_ENABLE 0x01
#define SW_RESET 0x04

#define SENSOR_ADDR 0x5C
#define AUTO_INCR   0x80

static nrf_drv_twi_t * m_instance;


// must only be called after lps331ap_on has been called
void lps331ap_readPressure(float *pres){

    uint8_t command[1] = {PRESS_OUT_XL | AUTO_INCR};
    
    nrf_drv_twi_tx(
            m_instance, 
            SENSOR_ADDR,
           	command,
            sizeof(command),
            true
    );


    uint8_t pressure_data[3];

    nrf_drv_twi_rx(
            m_instance, 
            SENSOR_ADDR,
            pressure_data,
            sizeof(pressure_data),
            false
    );

    float pressure =    (0x00FFFFFF & (((uint32_t)pressure_data[2] << 16) |
                        ((uint32_t) pressure_data[1] << 8) |
                        ((uint32_t) pressure_data[0]))) / 4096.0;

    *pres = pressure;

}

//does not return correct temperature
void lps331ap_readTemp (float *temp){


    uint8_t command[1] = {TEMP_OUT_L | AUTO_INCR};

    nrf_drv_twi_tx(
            m_instance, 
            SENSOR_ADDR,
            command,
            sizeof(command),
            true
    );

    uint8_t temp_data[2];

    nrf_drv_twi_rx(
            m_instance, 
            SENSOR_ADDR,
            temp_data,
            sizeof(temp_data),
            false
    );


    float temperature = (((int16_t)( 0x0000FFFF & (  ( (uint32_t) temp_data[1] << 8) |  ((uint32_t) temp_data[0]) ) )) / 480.0 ) + 42.5;


    *temp = temperature;

}


//resets everything
void lps331ap_power_off(){

	
	uint8_t command[2] = {
		CTRL_REG1, 
		POWER_OFF
	};

    nrf_drv_twi_tx(
        m_instance, 
        SENSOR_ADDR,
        command,
        sizeof(command),
        false
    );

}

void lps331ap_sw_reset(){

    uint8_t command[2] = 
    {
        CTRL_REG2,
        SW_RESET

    };

    nrf_drv_twi_tx(
        m_instance, 
        SENSOR_ADDR,
        command,
        sizeof(command),
        false
    );

}

void lps331ap_sw_reset_disable(){

    uint8_t command[2] = 
    {
        CTRL_REG2,
        0x00
    };

    nrf_drv_twi_tx(
        m_instance, 
        SENSOR_ADDR,
        command,
        sizeof(command),
        false
    );

}


void lps331ap_config_interrupt(interrupt_config interrupt_c_1, interrupt_config interrupt_c_2 , bool active_low){

    uint8_t command[2] = 
    {
        CTRL_REG3, 
        interrupt_c_1 | (interrupt_c_2 << 3)
    };


    if(active_low){

        command[1] = command[1] | 0x80;

    }

    nrf_drv_twi_tx(
        m_instance, 
        SENSOR_ADDR,
        command,
        sizeof(command),
        false
    );


    uint8_t data[1] = {0x00};

    lps331ap_read_controlreg3(data);

}


void lps331ap_interrupt_disable_all(){

    uint8_t command[2] = {INT_CFG_REG, 0x00};

    nrf_drv_twi_tx(
        m_instance, 
        SENSOR_ADDR,
        command,
        sizeof(command),
        false
    );

}


void lps331ap_interrupt_enable_manual(bool high, bool low, bool latch_request){

    uint8_t command[2] = {INT_CFG_REG, 0x00};

    if (high){
        //enable high
        command[1] = 0x01;
    }
    if (low){
        //enable low
        command[1] = command[1] | 0x02;
    }
    if (latch_request){

        command[1] = command[1] | 0x04;
    }

    nrf_drv_twi_tx(
        m_instance, 
        SENSOR_ADDR,
        command,
        sizeof(command),
        false
    );

}


void lps331ap_interrupt_enable(bool latch_request){

    uint8_t data[1] = {0x00};

    uint8_t command[2] = {INT_CFG_REG, 0x00};

    lps331ap_read_controlreg3(data);

    if (data[0] & 0x09){
        //enable high
        command[1] = 0x01;
    }
    if (data[0] & 0x12){
        //enable low
        command[1] = command[1] | 0x02;
    }
    if (latch_request){

        command[1] = command[1] | 0x04;
    }

    nrf_drv_twi_tx(
        m_instance, 
        SENSOR_ADDR,
        command,
        sizeof(command),
        false
    );


}


void lps331ap_config(lps331ap_data_rate data_rate, lps331ap_p_res p_res, lps331ap_t_res t_res){


    lps331ap_config_data_rate(data_rate);

    lps331ap_config_res(p_res, t_res);

}


void lps331ap_config_data_rate(lps331ap_data_rate data_rate){

    uint8_t command[2] = 
    {
        CTRL_REG1,
        (data_rate << 4)
    };

    nrf_drv_twi_tx(
        m_instance, 
        SENSOR_ADDR,
        command,
        sizeof(command),
        false
    );

}

void lps331ap_set_pressure_threshold(uint16_t p_thresh){

    uint8_t command[3] = 
    {
        THS_P_LOW_REG | AUTO_INCR,
        p_thresh & 0x00FF,
        (p_thresh & 0xFF00) >> 8
    };

    nrf_drv_twi_tx(
        m_instance, 
        SENSOR_ADDR,
        command,
        sizeof(command),
        false
    );

}

void lps331ap_init(nrf_drv_twi_t * p_instance){

    //lps331ap_sw_reset();

    m_instance = p_instance;


}

void lps331ap_read_controlreg1(uint8_t * data){

    uint8_t command[1] = {CTRL_REG1};

    nrf_drv_twi_tx(
            m_instance, 
            SENSOR_ADDR,
            command,
            sizeof(command),
            true
    );

    nrf_drv_twi_rx(
            m_instance, 
            SENSOR_ADDR,
            data,
            sizeof(uint8_t),
            false
    );

}


void lps331ap_read_controlreg2(uint8_t * data){

    uint8_t command[1] = {CTRL_REG2};

    nrf_drv_twi_tx(
            m_instance, 
            SENSOR_ADDR,
            command,
            sizeof(command),
            true
    );

    nrf_drv_twi_rx(
            m_instance, 
            SENSOR_ADDR,
            data,
            sizeof(uint8_t),
            false
    );

}

void lps331ap_read_controlreg3(uint8_t * data){

    uint8_t command[1] = {CTRL_REG3};


    
    nrf_drv_twi_tx(
            m_instance, 
            SENSOR_ADDR,
            command,
            sizeof(command),
            true
    );

    nrf_drv_twi_rx(
            m_instance, 
            SENSOR_ADDR,
            data,
            sizeof(uint8_t),
            false
    );
    
    /*
    nrf_drv_twi_rx(
        m_instance, 
        SENSOR_ADDR,
        command,
        sizeof(command),
        false
    );
    */

}

void lps331ap_read_status_reg(uint8_t * buf){

    uint8_t command[1] = {STATUS_REG};

    nrf_drv_twi_tx(
            m_instance, 
            SENSOR_ADDR,
            command,
            sizeof(command),
            true
    );

    nrf_drv_twi_rx(
            m_instance, 
            SENSOR_ADDR,
            buf,
            sizeof(uint8_t),
            false
    );

}

void lps331ap_read_interrupt_source_reg(uint8_t * buf){

    uint8_t command[1] = {INT_SOURCE_REG};

    nrf_drv_twi_tx(
            m_instance, 
            SENSOR_ADDR,
            command,
            sizeof(command),
            true
    );

    nrf_drv_twi_rx(
            m_instance, 
            SENSOR_ADDR,
            buf,
            sizeof(uint8_t),
            false
    );

}

void lps331ap_config_res(lps331ap_p_res p_res, lps331ap_t_res t_res){

    uint8_t data = (p_res | (t_res << 4));

    uint8_t command[2] = 
    {
        RES_CONF,
        data

    };

     nrf_drv_twi_tx(
            m_instance, 
            SENSOR_ADDR,
            command,
            sizeof(command),
            false
    );

}


// must only be called after lps331ap_init has been called
void lps331ap_power_on(){

    uint8_t command[1] = {CTRL_REG1};

    nrf_drv_twi_tx(
        m_instance, 
        SENSOR_ADDR,
        command,
        sizeof(command),
        true
    );

    uint8_t data[1] = {0x00};

    nrf_drv_twi_rx(
        m_instance, 
        SENSOR_ADDR,
        data,
        sizeof(uint8_t),
        false
    );


	uint8_t command2[2] = {
		CTRL_REG1, 
		data[0] | POWER_ON | BLOCK_UPDATE_ENABLE
	};

    nrf_drv_twi_tx(
        m_instance, 
        SENSOR_ADDR,
        command2,
        sizeof(command2),
        false
    );

}

//need to test this
void lps331ap_one_shot_config(){


    uint8_t command[2] = 
    {
        CTRL_REG1,
        BLOCK_UPDATE_ENABLE
    };

    nrf_drv_twi_tx(
        m_instance, 
        SENSOR_ADDR,
        command,
        sizeof(command),
        false
    ); 

}

void lps331ap_one_shot_enable(){

    uint8_t command2[2] = 
    {
        CTRL_REG2,
        ONE_SHOT_ENABLE

    };

     nrf_drv_twi_tx(
            m_instance, 
            SENSOR_ADDR,
            command2,
            sizeof(command2),
            false
    );

}