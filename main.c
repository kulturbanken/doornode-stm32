/*
  ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,2011 Giovanni Di Sirio.

  This file is part of ChibiOS/RT.

  ChibiOS/RT is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  ChibiOS/RT is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.

  ---

  A special exception to the GPL can be applied should you wish to distribute
  a combined work that includes ChibiOS/RT, without being obliged to provide
  the source code for any proprietary components. See the file exception.txt
  for full details of how and when the exception can be applied.
*/

#include "ch.h"
#include "hal.h"
#include "test.h"
#include "shell.h"
#include "chprintf.h"

#include "kbi2c.h"

extern void shellCheckRunning(void);

/*
 * Green LED blinker thread, times are in milliseconds.
 * GPIOA,5 is the green LED on the Olimexino STM32.
 * GPIOA_GREEN_LED is defined as 5 in the board header.
 */
static WORKING_AREA(waGreenLED, 128);
static msg_t GreenLED(void *arg) {
	(void)arg;
	while (TRUE) {
		palClearPad(GPIOA, GPIOA_GREEN_LED);
		chThdSleepMilliseconds(500);
		palSetPad(GPIOA, GPIOA_GREEN_LED);
		chThdSleepMilliseconds(500);
	}

	return 0;
}

/*
 * Yellow LED blinker thread, times are in milliseconds.
 * GPIOA,1 is the yellow LED on the Olimexino STM32.
 * GPIOA_YELLOW_LED is defined as 1 in the board header.
 */
static WORKING_AREA(waYellowLED, 128);
static msg_t YellowLED(void *arg) {
	(void)arg;
	while (TRUE) {
		palClearPad(GPIOA, GPIOA_YELLOW_LED);
		chThdSleepMilliseconds(100);
		palSetPad(GPIOA, GPIOA_YELLOW_LED);
		chThdSleepMilliseconds(100);
	}

	return 0;
}


static WORKING_AREA(waI2C, 128);
static msg_t I2C(void *arg) {
	(void)arg;
	while (TRUE) {
		if(!kb_i2c_request_fake())
			chprintf(&SD1, "result: %d\r\n", kb_i2c_get_data());
		chThdSleepMilliseconds(500);
	}

	return 0;
}

#define SOH 0x01
#define STX 0x02
#define ETX 0x03

static WORKING_AREA(waKeyPad, 128);
static msg_t KeyPad(void *arg) {
	(void)arg;
	uint8_t cmd[64], bcc, n;
	int cmdlen, stop, read_data;

	while (TRUE) {
		cmdlen = 0;
		cmd[cmdlen++] = SOH;
		cmd[cmdlen++] = 'S';
		/* ID */
		cmd[cmdlen++] = '0';
		cmd[cmdlen++] = '0';
		/* Function */
		cmd[cmdlen++] = 'A';
		cmd[cmdlen++] = '5';
		cmd[cmdlen++] = STX;
		/* Data */
		cmd[cmdlen++] = ETX;

		bcc = 0;
		for (n = 0; n < cmdlen; n++)
			bcc ^= cmd[n];
		bcc |= 0x20;

		cmd[cmdlen++] = bcc;

		chSequentialStreamWrite(&SD2, cmd, cmdlen);

		stop = read_data = cmdlen = 0;
		while (!stop) {
			n = chSequentialStreamGet(&SD2);
			if (n == STX) {
				read_data = 1;
			} else if (n == ETX) {
				n = chSequentialStreamGet(&SD2);
				stop = 1;
			} else if (read_data) {
				cmd[cmdlen++] = n;
			}
		}

		if (cmdlen > 4) {
			chSequentialStreamWrite(&SD1, cmd, cmdlen);
			chSequentialStreamWrite(&SD1, (uint8_t *)"\r\n", 2);
		}
		
		chThdSleepMilliseconds(100);
	}

	return 0;
}

/*
 * Application entry point.
 */
int main(void)
{
	/*
	 * System initializations.
	 * - HAL initialization, this also initializes the configured device drivers
	 *   and performs the board-specific initializations.
	 * - Kernel initialization, the main() function becomes a thread and the
	 *   RTOS is active.
	 */
	halInit();
	chSysInit();

	/* TX+RX on USART1 */
	palSetPadMode(IOPORT1, 9, PAL_MODE_STM32_ALTERNATE_PUSHPULL);
	palSetPadMode(IOPORT1, 10, PAL_MODE_INPUT);

	/* TX+RX on USART2 */
	palSetPadMode(IOPORT1, 2, PAL_MODE_STM32_ALTERNATE_PUSHPULL);
	palSetPadMode(IOPORT1, 3, PAL_MODE_INPUT);

	static const SerialConfig keypad_config = {
		9600,
		0,
		USART_CR2_STOP1_BITS | USART_CR2_LINEN,
		0
	};

	sdStart(&SD1, NULL); /* Shell */
	sdStart(&SD2, &keypad_config); /* Keypad */

	shellInit();

	kb_i2c_init();

	/*
	 * Creates the blinker threads.
	 */
	chThdCreateStatic(waGreenLED, sizeof(waGreenLED), NORMALPRIO, GreenLED, NULL);
	chThdCreateStatic(waYellowLED, sizeof(waYellowLED), NORMALPRIO, YellowLED, NULL);
	chThdCreateStatic(waI2C, sizeof(waI2C), NORMALPRIO, I2C, NULL);
	chThdCreateStatic(waKeyPad, sizeof(waKeyPad), NORMALPRIO, KeyPad, NULL);

	/*
	 * Normal main() thread activity, in this demo it does nothing.
	 */
	while (TRUE) {
		shellCheckRunning();

		chThdSleepMilliseconds(500);
		//sdPut(&SD1, 'A');
		//chprintf((BaseSequentialStream *)&SD1, "Tjosan\r\n");

	}
}
