#ifndef LPS331AP_H

#define LP331AP_H

typedef enum
{
    lps331ap_MODE0, 	// Pressure = One Shot, Temperature = One Shot
    lps331ap_MODE1, 	// Pressure = 1 Hz, 	Temperature = 1 Hz
    lps331ap_MODE2, 	// Pressure = 7 Hz, 	Temperature = 1 Hz
    lps331ap_MODE3, 	// Pressure = 12.5 Hz, 	Temperature = 1 Hz
    lps331ap_MODE4, 	// Pressure = 25 Hz, 	Temperature = 1 Hz
    lps331ap_MODE5, 	// Pressure = 7 Hz, 	Temperature = 7 Hz
    lps331ap_MODE6, 	// Pressure = 12.5 Hz, 	Temperature = 12.5 Hz
    lps331ap_MODE7, 	// Pressure = 25 Hz, 	Temperature = 25 Hz

} lps331ap_data_rate; //Measurement Resolution, (aka ODR)


typedef enum
{
    lps331ap_P_RES_0, 	// Nr. Internal Average = 1
    lps331ap_P_RES_1, 	// Nr. Internal Average = 2
    lps331ap_P_RES_2, 	// Nr. Internal Average = 4
    lps331ap_P_RES_3, 	// Nr. Internal Average = 8
    lps331ap_P_RES_4, 	// Nr. Internal Average = 16
    lps331ap_P_RES_5, 	// Nr. Internal Average = 32
    lps331ap_P_RES_6, 	// Nr. Internal Average = 64
    lps331ap_P_RES_7, 	// Nr. Internal Average = 128
    lps331ap_P_RES_8, 	// Nr. Internal Average = 256
    lps331ap_P_RES_9, 	// Nr. Internal Average = 384
    lps331ap_P_RES_10, 	// Nr. Internal Average = 512, not allowed with ODR = 25Hz/25Hz

} lps331ap_p_res; 	//Pressure Resolution


typedef enum
{
    lps331ap_T_RES_0, 	// Nr. Internal Average = 1
    lps331ap_T_RES_1, 	// Nr. Internal Average = 2
    lps331ap_T_RES_2, 	// Nr. Internal Average = 4
    lps331ap_T_RES_3, 	// Nr. Internal Average = 8
    lps331ap_T_RES_4, 	// Nr. Internal Average = 16
    lps331ap_T_RES_5, 	// Nr. Internal Average = 32
    lps331ap_T_RES_6, 	// Nr. Internal Average = 64
    lps331ap_T_RES_7, 	// Nr. Internal Average = 128, not allowed with ODR = 25Hz/25Hz

} lps331ap_t_res; 	//Temperature Resolution


typedef enum
{
	lps331ap_intrerrupt_mode_NONE,
	lps331ap_interrupt_mode_P_HIGH,
	lps331ap_interrupt_mode_P_LOW,
	lps331ap_interrupt_mode_P_LOW_OR_P_HIGH,
	lps331ap_interrupt_mode_DATA_READY,

} interrupt_config;


//call in this order to start:
//sw_reset, init, off, config, one_shot_config?, on, one_shot_enable

void lps331ap_sw_reset();
void lps331ap_sw_reset_disable();

void lps331ap_init(nrf_drv_twi_t * p_instance);

void lps331ap_set_pressure_threshold(uint16_t p_thresh);

void lps331ap_config(lps331ap_data_rate data_rate, lps331ap_p_res p_res, lps331ap_t_res t_res);
void lps331ap_config_interrupt(interrupt_config interrupt_c_1, interrupt_config interrupt_c_2 , bool active_low);
void lps331ap_config_data_rate(lps331ap_data_rate data_rate);
void lps331ap_config_res(lps331ap_p_res p_res, lps331ap_t_res t_res);
void lps331ap_one_shot_config();

void lps331ap_power_on();
void lps331ap_power_off();

//if latch_request = true, interrupt latched into interrupt source reg
//interrupt source reg is cleared by reading INT_ACT register
void lps331ap_interrupt_enable(bool latch_request);
void lps331ap_interrupt_enable_manual(bool high, bool low, bool latch_request);
void lps331ap_one_shot_enable();
void lps331ap_interrupt_disable_all();

void lps331ap_readPressure(float *pres);
void lps331ap_readTemp (float *temp);

void lps331ap_read_controlreg1(uint8_t * data);
void lps331ap_read_controlreg2(uint8_t * data);
void lps331ap_read_controlreg3(uint8_t * data);
void lps331ap_read_status_reg(uint8_t * buf);
void lps331ap_read_interrupt_source_reg(uint8_t * buf);


#endif //LPS331AP_H