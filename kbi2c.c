#include "ch.h"
#include "hal.h"
#include "i2c.h"
#include "chprintf.h"
#include "iocard.h"
#include "kbi2c.h"

/* input buffer */
static uint8_t rx_data[64];

/* temperature value */
static uint16_t temperature = 0;

static i2cflags_t errors = 0;

static iocard_data_t iocard_data;

#define addr 1

/* This is main function. */
i2cflags_t kb_i2c_request_fake(void)
{
	msg_t status = RDY_OK;
	systime_t tmo = MS2ST(100);

	i2cAcquireBus(&I2CD1);
	status = i2cMasterReceiveTimeout(&I2CD1, addr, (uint8_t *)&iocard_data, sizeof(iocard_data), tmo);
	i2cReleaseBus(&I2CD1);

	if (status == RDY_RESET){
		errors = i2cGetErrors(&I2CD1);
		return errors;
		
		//if (errors == I2CD_ACK_FAILURE){
			/* there is no slave with given address on the bus, or it was die */
		//	return;
		//}
	} else {
		iocard_data_t *iodata = (iocard_data_t *)rx_data;
		//temperature = rx_data[0]; //(rx_data[0] << 8) + rx_data[1];
		temperature = iodata->analog_in[2];
	}

	return 0;
}

uint16_t kb_i2c_get_data(void)
{
	uint16_t tmp = temperature;
	temperature = 0;

	return iocard_data.analog_in[2];
	return tmp;
}

/* I2C1 */
static const I2CConfig i2cfg1 = {
	OPMODE_I2C,
	100000,
	FAST_DUTY_CYCLE_2,
};

void kb_i2c_init(void)
{
	i2cInit();

	i2cStart(&I2CD1, &i2cfg1);

	/* tune ports for I2C1*/
	palSetPadMode(IOPORT2, 6, PAL_MODE_STM32_ALTERNATE_OPENDRAIN);
	palSetPadMode(IOPORT2, 7, PAL_MODE_STM32_ALTERNATE_OPENDRAIN);

	chThdSleepMilliseconds(100);  /* Just to be safe. */
}
