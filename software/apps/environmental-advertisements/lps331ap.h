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

} lps331ap_data_rate_t; //Measurement Resolution

void lps331ap_readPressure(float *pres);

void lps331ap_readTemp (float *temp);

void lps331ap_power_off();

void lps331ap_init(lps331ap_data_rate_t data_rate, nrf_drv_twi_t * p_instance);

void lps331ap_power_on();

void lps331ap_one_shot();


#endif //SI7021_H