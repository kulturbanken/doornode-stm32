#include "ch.h"
#include "hal.h"
#include "i2c.h"
#include "chprintf.h"
#include "iocard.h"
#include "kbi2c.h"

static i2cflags_t errors = 0;

static iocard_data_t iocard_data;


/* This is main function. */
i2cflags_t kb_i2c_request_fake(uint8_t address)
{
	msg_t status = RDY_OK;
	systime_t tmo = MS2ST(100);

	address &= 0x03;
	address <<= 5;

	i2cAcquireBus(&I2CD1);
	status = i2cMasterReceiveTimeout(&I2CD1, address, (uint8_t *)&iocard_data, sizeof(iocard_data), tmo);
	i2cReleaseBus(&I2CD1);

	if (status == RDY_OK)
		return 0;
	else
		return 1;

	if (status == RDY_RESET){
		return -1;
		errors = i2cGetErrors(&I2CD1);
		return errors;
		
		//if (errors == I2CD_ACK_FAILURE){
			/* there is no slave with given address on the bus, or it was die */
		//	return;
		//}
	} else {
	}

	return 0;
}

iocard_data_t *kb_i2c_get_iocard_data(void)
{
	return &iocard_data;
}

i2cflags_t kb_i2c_set_output(uint8_t address)
{
	static uint8_t out = 0, mask = 0;
	static uint8_t pin = 0;
	uint8_t data[2];
	systime_t tmo = MS2ST(100);
	msg_t status = RDY_OK;

	address &= 0x03;
	address <<= 5;

	mask = (1<<pin);

	pin++;
	if (pin > 7) {
		out ^= 0xFF;
		pin = 0;
	}

	data[0] = mask;
	data[1] = out;

	i2cAcquireBus(&I2CD1);
	status = i2cMasterTransmitTimeout(&I2CD1, address | 0x01, (uint8_t *) data, 2, NULL, 0, tmo);
	i2cReleaseBus(&I2CD1);

	if (status == RDY_RESET){
		errors = i2cGetErrors(&I2CD1);
		return errors;
		
		//if (errors == I2CD_ACK_FAILURE){
			/* there is no slave with given address on the bus, or it was die */
		//	return;
		//}
	} else {
		
	}

	return 0;
}

uint16_t kb_i2c_get_data(void)
{
	return iocard_data.analog_in.sirene1;
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
