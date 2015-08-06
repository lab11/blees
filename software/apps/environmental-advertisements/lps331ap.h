#ifndef LPS331AP_H

#define LP331AP_H


typedef enum
{
    lps331ap_MODE0, // Pressure = One Shot, 	Temperature = One Shot
    lps331ap_MODE1, // Pressure = 1 Hz, 		Temperature = 1 Hz
    lps331ap_MODE2, // Pressure = 7 Hz, 		Temperature = 1 Hz
    lps331ap_MODE3, // Pressure = 12.5 Hz, 		Temperature = 1 Hz
    lps331ap_MODE4, // Pressure = 25 Hz, 		Temperature = 1 Hz
    lps331ap_MODE5, // Pressure = 7 Hz, 		Temperature = 7 Hz
    lps331ap_MODE6, // Pressure = 12.5 Hz, 		Temperature = 12.5 Hz
    lps331ap_MODE7, // Pressure = 25 Hz, 		Temperature = 25 Hz

} lps331ap_data_rate_t; //Measurement Resolution, (aka ODR)


typedef enum
{
    lps331ap_P_RES_0, // Nr. Internal Average = 1
    lps331ap_P_RES_1, // Nr. Internal Average = 2
    lps331ap_P_RES_2, // Nr. Internal Average = 4
    lps331ap_P_RES_3, // Nr. Internal Average = 8
    lps331ap_P_RES_4, // Nr. Internal Average = 16
    lps331ap_P_RES_5, // Nr. Internal Average = 32
    lps331ap_P_RES_6, // Nr. Internal Average = 64
    lps331ap_P_RES_7, // Nr. Internal Average = 128
    lps331ap_P_RES_8, // Nr. Internal Average = 256
    lps331ap_P_RES_9, // Nr. Internal Average = 384
    lps331ap_P_RES_10, // Nr. Internal Average = 512, not allowed with ODR = 25Hz/25Hz

} lps331ap_p_res_t; //Pressure Resolution

typedef enum
{
    lps331ap_T_RES_0, // Nr. Internal Average = 1
    lps331ap_T_RES_1, // Nr. Internal Average = 2
    lps331ap_T_RES_2, // Nr. Internal Average = 4
    lps331ap_T_RES_3, // Nr. Internal Average = 8
    lps331ap_T_RES_4, // Nr. Internal Average = 16
    lps331ap_T_RES_5, // Nr. Internal Average = 32
    lps331ap_T_RES_6, // Nr. Internal Average = 64
    lps331ap_T_RES_7, // Nr. Internal Average = 128, not allowed with ODR = 25Hz/25Hz

} lps331ap_t_res_t; //Temperature Resolution

//call in this order to start:
//sw_reset, init, off, config, one_shot_config?, on, one_shot_enable

void lps331ap_sw_reset();

void lps331ap_init(nrf_drv_twi_t * p_instance);

void lps331ap_power_off();

void lps331ap_config(lps331ap_data_rate_t data_rate, lps331ap_p_res_t p_res, lps331ap_t_res_t t_res);


void lps331ap_config_data_rate(lps331ap_data_rate_t data_rate);

void lps331ap_config_res(lps331ap_p_res_t p_res, lps331ap_t_res_t t_res);


void lps331ap_one_shot_config();

void lps331ap_power_on();

void lps331ap_one_shot_enable();


void lps331ap_readPressure(float *pres);

void lps331ap_readTemp (float *temp);

void lps331ap_read_controlreg1(uint8_t * data);

void lps331ap_read_controlreg2(uint8_t * data);

void lps331ap_read_status_reg(uint8_t * buf);



#endif //LPS331AP_H