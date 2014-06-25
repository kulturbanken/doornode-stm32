#include "ch.h"
#include "hal.h"
#include "test.h"
#include "shell.h"
#include "chprintf.h"
//#include "iwdg.h"

#include "kbi2c.h"
#include "iocard.h"
#include "kbcan.h"
#include "kbiocard.h"
#include "kbkeypad.h"
#include "kbshell.h"

static BaseSequentialStream *chp;

/* To make backtrace work on unhandled exceptions */
/* http://koti.kapsi.fi/jpa/stuff/other/stm32-hardfault-backtrace.html */
void **HARDFAULT_PSP;
register void *stack_pointer asm("sp");

void HardFaultVector(void)
{
    // Hijack the process stack pointer to make backtrace work
    asm("mrs %0, psp" : "=r"(HARDFAULT_PSP) : :);
    stack_pointer = HARDFAULT_PSP;
    while(1);
}

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
/*
	IWDGConfig wdg_cfg = {
		.counter = IWDG_COUNTER_MAX,
		.div     = IWDG_DIV_256
	};
	iwdgInit();
	iwdgStart(&IWDGD, &wdg_cfg);
	iwdgReset(&IWDGD);
*/
	//palSetPadMode(GPIOA, GPIOA_YELLOW_LED, PAL_MODE_OUTPUT_PUSHPULL);
	//palSetPadMode(GPIOA, GPIOA_GREEN_LED, PAL_MODE_OUTPUT_PUSHPULL);

	kb_shell_init();
	chp = kb_shell_get_stream();
	chprintf(chp, "kb_shell_init\r\n");

	kb_i2c_init();
	chprintf(chp, "kb_i2c_init\r\n");

	kb_keypad_init();
	chprintf(chp, "kb_keypad_init\r\n");

	kb_iocard_init();
	chprintf(chp, "kb_iocard_init\r\n");

	iocard_t *first_card = kb_iocard_get_card(0);
	while (!first_card->has_data) {
		chprintf(chp, "Waiting first IO card to settle.\r\n");
		chThdSleepMilliseconds(100);
	}

	int node_id = first_card->data.dip_switch & 0x3F;
	chprintf(chp, " * Node ID: %d\r\n", node_id);
	chprintf(chp, " CAN ID: 0x%08x\r\n", node_id << 21);

	kb_can_init(node_id);
	chprintf(chp, "kb_can_init\r\n");

	/* Report my existance */
	kb_can_msg_new(0, 0, 0, NULL, 0);

	while (TRUE) {
		kb_shell_check_running();

		chThdSleepMilliseconds(100);

		if (kb_can_ok_flag && kb_keypad_ok_flag && kb_iocard_ok_flag) {
			kb_can_ok_flag = kb_keypad_ok_flag = kb_iocard_ok_flag = false;
			//iwdgReset(&IWDGD);
		}
	}
}
