#include <string.h>
#include "ch.h"
#include "hal.h"
#include "kbkeypad.h"
#include "kbshell.h"
#include "chprintf.h"

#include "kbcan.h"

bool kb_keypad_ok_flag = false;

static BaseAsynchronousChannel *kPadCh;
static BaseSequentialStream *chp;

#define SOH 0x01
#define STX 0x02
#define ETX 0x03

static WORKING_AREA(waKeyPad, 256);
static msg_t KeyPad(void *arg) {
	chRegSetThreadName("KeyPad");
	(void)arg;
	uint8_t cmd[64], bcc, n;
	int cmdlen, read_data;

	chThdSleepMilliseconds(500);

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

		chnWrite(kPadCh, cmd, cmdlen);

		read_data = cmdlen = 0;
		while (TRUE) {
			if (!chnReadTimeout(kPadCh, &n, 1, MS2ST(500)))
				break;

			if (n == STX) {
				read_data = 1;
			} else if (n == ETX) {
				chnReadTimeout(kPadCh, &n, 1, MS2ST(500));
				break;
			} else if (read_data) {
				cmd[cmdlen++] = n;
			}
		}

		// TODO: Check BBC!!
#if 0
		if (cmdlen > 4) {
			cmd[cmdlen] = '\0';
			chprintf(chp, "\r\nKEYPAD cmdlen = %d cmd = ", cmdlen);
			for (n = 0; n < cmdlen; n++)
				chprintf(chp, "%c", cmd[n]);
			chprintf(chp, "\r\n");
		} else {
			//chprintf(chp, "\r\nKEYPAD ERROR\r\n");
		}
#endif

		if (cmdlen > 4) {
			cmd[cmdlen] = '\0';
			char *tok = (char *)cmd;
			char *tagid = strsep(&tok, ":");
			char *code = strsep(&tok, ":");
			char *inputs = strsep(&tok, ":");

			chprintf(chp, "tagid='%s', code='%s', inputs='%s'\r\n", tagid, code, inputs);
			
			if (strlen(tagid) >= 8) {
				int s = strlen(tagid);
				char *data_start = tagid + (s - 8); // Only grab the last 8 bytes
				kb_can_msg_new(0, 0, 10, data_start, 8);
			}

			if (strlen(code) > 0 && strlen(code) <= 8) {
				kb_can_msg_new(0, 0, 11, code, strlen(code));
			}

			/* Just ignore the input pins on the keypad, not used */
		}

		chThdSleepMilliseconds(100);
	}

	return 0;
}

void kb_keypad_init(void)
{
	/* TX+RX on USART2 */
	palSetPadMode(GPIOA, 2, PAL_MODE_STM32_ALTERNATE_PUSHPULL);
	palSetPadMode(GPIOA, 3, PAL_MODE_INPUT);

	chp = kb_shell_get_stream();

	static const SerialConfig keypad_config = {
		9600,
		0,
		USART_CR2_STOP1_BITS | USART_CR2_LINEN,
		0
	};

	sdStart(&SD2, &keypad_config); /* Keypad */
	kPadCh = (BaseAsynchronousChannel *)&SD2;

	chThdCreateStatic(waKeyPad, sizeof(waKeyPad), NORMALPRIO, KeyPad, NULL);
}
