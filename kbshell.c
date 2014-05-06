#include "ch.h"
#include "hal.h"
#include "test.h"
#include "shell.h"
#include "chprintf.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "kbiocard.h"
#include "kbi2c.h"
#include "iocard.h"
#include "kbcan.h"

static BaseSequentialStream *chp;

#define SHELL_WA_SIZE   THD_WA_SIZE(2048)

static void cmd_cantest(BaseSequentialStream *chp, int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	kb_can_msg_new(0, 0, 13, "\x01", 1);
	kb_can_msg_new(0, 0, 13, "\x02", 1);
	kb_can_msg_new(1, 0, 13, "\x03", 1);
	kb_can_msg_new(2, 0, 13, "\x04", 1);
	kb_can_msg_new(3, 0, 13, "\x05", 1);

	chprintf(chp, "CAN msg queued\r\n");
}

static void cmd_iocard(BaseSequentialStream *chp, int argc, char *argv[])
{
	if (argc == 0) {
		chprintf(chp, 
			 "Usage:\r\n"
			 "  iocard cardnr\r\n");
		return;
	}

	int address = atoi(argv[0]);
	iocard_t *card = kb_iocard_get_card(0);
	iocard_data_t *iodata = &card->data;
	int n;

	char *chtxt[] = {
			"SIR1", "FLA1", "SIR2", "FLA2",
			"LCK2", "LCK1", "KPAD", "IR",
			"VOLT", "TAMP", "IR1",  "IR2",
			"DIGI", "OVRC"
	};

	chprintf(chp, "\r\n\r\n");
	for (n = 0; n < (int)(sizeof(chtxt)/sizeof(chtxt[0])); n++) {
		chprintf(chp, " %4s | ", chtxt[n]);
	}
	chprintf(chp, "\r\n");

	for (n = 0; n < 12; n++) {
		chprintf(chp, "%5d | ", iodata->analog_in_array[n]);
	}
	chprintf(chp, " 0x%02x | ", iodata->digital_in_byte);
	chprintf(chp, " 0x%02x | ", iodata->over_current_byte);

	chprintf(chp, "\r\n\r\n");
}

static void cmd_regs(BaseSequentialStream *chp, int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	chprintf(chp, "AFIO->MAPR: 0x%08x\r\n", AFIO->MAPR);
	chprintf(chp, "RCC->APB1ENR: 0x%08x\r\n", RCC->APB1ENR);
	chprintf(chp, "RCC->APB2ENR: 0x%08x\r\n", RCC->APB2ENR);
	chprintf(chp, "CAN1->MCR: 0x%08x\r\n", CAN1->MCR);
	chprintf(chp, "CAN1->MSR: 0x%08x\r\n", CAN1->MSR);
	chprintf(chp, "CAN1->TSR: 0x%08x\r\n", CAN1->TSR);
	chprintf(chp, "CAN1->RF0R: 0x%08x\r\n", CAN1->RF0R);
	chprintf(chp, "CAN1->RF1R: 0x%08x\r\n", CAN1->RF1R);
	chprintf(chp, "CAN1->IER: 0x%08x\r\n", CAN1->IER);
	chprintf(chp, "CAN1->ESR: 0x%08x\r\n", CAN1->ESR);
	chprintf(chp, "CAN1->BTR: 0x%08x\r\n", CAN1->BTR);
}

static void cmd_mem(BaseSequentialStream *chp, int argc, char *argv[])
{
	size_t n, size;

	(void)argv;
	if (argc > 0) {
		chprintf(chp, "Usage: mem\r\n");
		return;
	}
	n = chHeapStatus(NULL, &size);
	chprintf(chp, "core free memory : %u bytes\r\n", chCoreStatus());
	chprintf(chp, "heap fragments   : %u\r\n", n);
	chprintf(chp, "heap free total  : %u bytes\r\n", size);
}

static void cmd_threads(BaseSequentialStream *chp, int argc, char *argv[])
{
	static const char *states[] = {THD_STATE_NAMES};
	Thread *tp;

	(void)argv;
	if (argc > 0) {
		chprintf(chp, "Usage: threads\r\n");
		return;
	}
	chprintf(chp, "    addr    stack prio refs     state time\r\n");
	tp = chRegFirstThread();
	do {
		chprintf(chp, "%.8lx %.8lx %4lu %4lu %9s %lu\r\n",
			 (uint32_t)tp, (uint32_t)tp->p_ctx.r13,
			 (uint32_t)tp->p_prio, (uint32_t)(tp->p_refs - 1),
			 states[tp->p_state], (uint32_t)tp->p_time);
		tp = chRegNextThread(tp);
	} while (tp != NULL);
}

static void cmd_i2c(BaseSequentialStream *chp, int argc, char *argv[])
{
	i2cflags_t errors = 0;

	if (argc == 0) {
		chprintf(chp, 
			 "Usage:\r\n"
			 "  i2c test\r\n");
		return;
	}
	iocard_data_t iodata;

	if (!strcmp(argv[0], "test")) {
		if ((errors = kb_i2c_request(1, &iodata, sizeof(iodata)))) {
			chprintf(chp, "i2c message sent successfully\r\n");
		} else {
			chprintf(chp, "i2c message transmission failed with error %d\r\n", errors);
		}
	} else if (!strcmp(argv[0], "set") && argc >= 2) {
		if (!strcmp(argv[1], "all"))
			errors = kb_i2c_set_output(1, 0xFF, 0xfF);
		else
			errors = kb_i2c_set_output(1, (1 << atoi(argv[1])), 0xFF);
	} else if (!strcmp(argv[0], "clr") && argc >= 2) {
		if (!strcmp(argv[1], "all"))
			errors = kb_i2c_set_output(1, 0xFF, 0x00);
		else
			errors = kb_i2c_set_output(1, (1 << atoi(argv[1])), 0x00);
	} else if (!strcmp(argv[0], "reset")) {
		kb_i2c_reset();
	} else {
		chprintf(chp, "Unknown i2c command %s\r\n", argv[0]);
	}

	if (errors)
		chprintf(chp, "I2C transmission failed with error %d\r\n", errors);
}

static const ShellCommand commands[] = {
	{"mem", cmd_mem},
	{"threads", cmd_threads},
	{"i2c", cmd_i2c},
	{"regs", cmd_regs},
	{"iocard", cmd_iocard},
	{"cantest", cmd_cantest},
	{NULL, NULL}
};

static const ShellConfig shell_cfg = {
	(BaseSequentialStream *)&SD1,
	commands
};

static Thread *shelltp = NULL;

void kb_shell_check_running(void)
{
	if (!shelltp)
		shelltp = shellCreate(&shell_cfg, SHELL_WA_SIZE, NORMALPRIO);
	else if (chThdTerminated(shelltp)) {
		chThdRelease(shelltp);
		shelltp = NULL;
	}
}

BaseSequentialStream *kb_shell_get_stream(void)
{
	return chp;
}

void kb_shell_init(void)
{
	/* TX+RX on USART1 */
	palSetPadMode(GPIOA, 9, PAL_MODE_STM32_ALTERNATE_PUSHPULL);
	palSetPadMode(GPIOA, 10, PAL_MODE_INPUT);

	sdStart(&SD1, NULL); /* Shell */
	chp = (BaseSequentialStream *)&SD1;

	shellInit();
}
