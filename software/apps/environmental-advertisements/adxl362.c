#define LED_PIN 25
#define SPI_SCK_PIN 9
#define SPI_MISO_PIN 10
#define SPI_MOSI_PIN 11
#define SPI_SS_PIN 4
#define NRF_SPI NRF_SPI0


//Addresses
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
#define RESET_CODE 0x52

#define THRESH_ACT_L 0x20
#define THRESH_ACT_H 0x21
#define TIME_ACT 0x22
#define THRESH_INACT_L 0x23
#define THRESH_INACT_H 0x24
#define TIME_INACT_L 0x25
#define TIME_INACT_H 0x26
#define ACT_INACT_CTL 0x27
#define FIFO_CONTROL 0x28
#define FIFO_SAMPLES 0x29
#define INTMAP1 0x2A
#define INTMAP2 0x2B
#define FILTER_CTL 0x2C
#define POWER_CTL 0x2D
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


static void spi_multi_read(uint8_t * buff, uint8_t num_bytes){

	int i = 0;

	do{

		spi_read(buff[i])

		i++;

	} while (i < num_bytes);

}


static void spi_multi_write(uint8_t * buff, uint8_t num_bytes){

	int i = 0;

	do{
		spi_write(buff[i])

		i++;

	}while (i < num_bytes);


}

static void spi_read_reg(uint8_t reg_addr, uint8_t * data, uint8_t num_bytes){

	nrf_gpio_pin_clear(SPI_SS_PIN);
    spi_write(READ_REG);
    spi_write(reg_addr); // STATUS

    int i = 0;
    do{
    	spi_read(data[i]);
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


static void sample_accel_byte(uint8_t * x_data, uint8_t * y_data, uint8_t * z_data){

	spi_read_reg(XDATA, x_data, 1);

	spi_read_reg(YDATA, y_data, 1);

	spi_read_reg(ZDATA, z_data, 1);

}

/*
static void sample_accel() {
    nrf_gpio_pin_clear(SPI_SS_PIN);
    spi_write(0x0B);
    //spi_write(0x0E); // XDATA_L
    //spi_write(0x2D); // POWER_CTL
    spi_write(0x40); // STATUS
    //spi_write(0x27); // ACTIVITY/INACTIVITY
    //spi_write(0x29);
    uint8_t device_id = 42;
    spi_read(&device_id);
    nrf_gpio_pin_set(SPI_SS_PIN);
    //m_sensor_info.pressure = device_id;
}
*/

static void spi_init(){
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

    // send a soft reset to the accelerometer
    uint8_t data[1] = {RESET_CODE};
    spi_write_reg(SOFT_RESET, data, 1);

    for(volatile int j=0; j<1000000; j++);
    accel_cmd[1] = 0x2D;
    accel_cmd[2] = 0x02;
    nrf_gpio_pin_clear(SPI_SS_PIN);
    for(int i=0; i<3; i++) {
        spi_write(accel_cmd[i]);
    }
    nrf_gpio_pin_set(SPI_SS_PIN);

}