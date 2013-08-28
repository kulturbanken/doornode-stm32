#include "ch.h"
#include "hal.h"
#include "i2c.h"
#include "chprintf.h"

/* input buffer */
static uint8_t rx_data[2];

/* temperature value */
static int16_t temperature = 0;

static i2cflags_t errors = 0;

#define addr 18

/* This is main function. */
i2cflags_t kb_i2c_request_fake(void)
{
	msg_t status = RDY_OK;
	systime_t tmo = MS2ST(100);

	i2cAcquireBus(&I2CD1);
	status = i2cMasterReceiveTimeout(&I2CD1, addr, rx_data, 2, tmo);
	i2cReleaseBus(&I2CD1);

	if (status == RDY_RESET){
		errors = i2cGetErrors(&I2CD1);
		return errors;
		
		//if (errors == I2CD_ACK_FAILURE){
			/* there is no slave with given address on the bus, or it was die */
		//	return;
		//}
	} else {
		temperature = rx_data[0]; //(rx_data[0] << 8) + rx_data[1];
	}

	return 0;
}

int16_t kb_i2c_get_data()
{
	int16_t tmp = temperature;
	temperature = 0;
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
