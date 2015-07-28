#ifndef SI7021_H

#define SI7021_H

#define FIRMWARE_REV 1.0 // A10

typedef enum
{
    si7021_MODE0, // RH = 12 bit, Temp = 14 bit
    si7021_MODE1, // RH = 8 bit, Temp = 12 bit
    si7021_MODE2, // RH = 10 bit, Temp = 13 bit
    so7021_MODE3  // RH = 11 bit, Temp = 11 bit
} si7021_meas_res_t; //Measurement Resolution

void si7021_reset();

void si7021_read_temp_hold(float * temp);

void si7021_read_temp(float *temp);

void si7021_read_RH_hold(float *hum);

void si7021_read_RH(float *hum);

void si7021_read_temp_after_RH(float *temp);

void si7021_read_temp_and_RH(float *temp, float *hum);

//not sure if this is working...
void si7021_config(si7021_meas_res_t res_mode);

//heater currently not working..
void si7021_heater_on();

void si7021_heater_off();


#endif //SI7021_H