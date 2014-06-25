#include "ch.h"
#include "hal.h"
#include "i2c.h"
#include "kbi2c.h"

static i2cflags_t errors = 0;

i2cflags_t kb_i2c_request(uint8_t address, void *buf, int buflen)
{
	msg_t status = RDY_OK;
	systime_t tmo = MS2ST(100);

	address &= 0x03;
	address <<= 5;

	//i2cAcquireBus(&I2CD1);
	status = i2cMasterReceiveTimeout(&I2CD1, address, buf, buflen, tmo);
	//i2cReleaseBus(&I2CD1);

	if (status == RDY_OK)
		return 0;
	else
		return 1;

	if (status == RDY_RESET) {
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

i2cflags_t kb_i2c_set_output(uint8_t address, uint8_t mask, uint8_t data)
{
	uint8_t buf[2];
	systime_t tmo = MS2ST(500);
	msg_t status = RDY_OK;

	address &= 0x03;
	address <<= 5;

	buf[0] = mask;
	buf[1] = data;

	//i2cAcquireBus(&I2CD1);
	//status = i2cMasterTransmitTimeout(&I2CD1, address | I2C_CMD_SET_OUTPUT, buf, sizeof(buf), NULL, 0, tmo);
	//i2cReleaseBus(&I2CD1);

	if (status == RDY_OK)
		return 0;
	else
		return 1;

	switch (status) {
	case RDY_RESET:
		kb_i2c_reset();
		return i2cGetErrors(&I2CD1);
	case RDY_TIMEOUT:
		return 12345;
	default:
		return 0;
	}
}

/* I2C1 */
static const I2CConfig i2cfg1 = {
	OPMODE_I2C,
	100000,
	FAST_DUTY_CYCLE_2,
};

void kb_i2c_reset(void)
{
	i2cAcquireBus(&I2CD1);
	i2cStop(&I2CD1);
	chThdSleepMilliseconds(100);
	i2cStart(&I2CD1, &i2cfg1);
	i2cReleaseBus(&I2CD1);
}

void kb_i2c_init(void)
{
	i2cInit();

	/* tune ports for I2C1*/
	palSetPadMode(IOPORT2, 6, PAL_MODE_STM32_ALTERNATE_OPENDRAIN);
	palSetPadMode(IOPORT2, 7, PAL_MODE_STM32_ALTERNATE_OPENDRAIN);

	i2cStop(&I2CD1); /* Required to make I2C start without power reset */
	i2cStart(&I2CD1, &i2cfg1);

	chThdSleepMilliseconds(100);  /* Just to be safe. */

	kb_i2c_set_output(1, 0xFF, 0x00);
}
