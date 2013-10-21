#include "ch.h"
#include "hal.h"
#include "test.h"
#include "shell.h"
#include "chprintf.h"

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "kbi2c.h"

#define SHELL_WA_SIZE   THD_WA_SIZE(2048)

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

	if (!strcmp(argv[0], "test")) {
		if ((errors = kb_i2c_request_fake(1))) {
			chprintf(chp, "i2c message sent successfully\r\n");
			chprintf(chp, "result: %d\r\n", kb_i2c_get_data());
		} else {
			chprintf(chp, "i2c message transmission failed with error %d\r\n", errors);
		}
	} else if (!strcmp(argv[0], "set") && argc >= 2) {
		if ((errors = kb_i2c_set_output(1, (1 << atoi(argv[1])), 0xFF)))
			chprintf(chp, "I2C transmission failed with error %d\r\n", errors);
	} else if (!strcmp(argv[0], "clr") && argc >= 2) {
		if ((errors = kb_i2c_set_output(1, (1 << atoi(argv[1])), 0x00)))
			chprintf(chp, "I2C transmission failed with error %d\r\n", errors);
	} else if (!strcmp(argv[0], "reset")) {
		kb_i2c_reset();
	} else {
		chprintf(chp, "Unknown i2c command %s\r\n", argv[0]);
	}
}

static const ShellCommand commands[] = {
	{"mem", cmd_mem},
	{"threads", cmd_threads},
	{"i2c", cmd_i2c},
	{NULL, NULL}
};

static const ShellConfig shell_cfg = {
	(BaseSequentialStream *)&SD1,
	commands
};

static Thread *shelltp = NULL;

void shellCheckRunning(void)
{
	if (!shelltp)
		shelltp = shellCreate(&shell_cfg, SHELL_WA_SIZE, NORMALPRIO);
	else if (chThdTerminated(shelltp)) {
		chThdRelease(shelltp);
		shelltp = NULL;
	}
}
