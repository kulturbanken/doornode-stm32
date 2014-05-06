
#include "string.h"
#include "ch.h"
#include "kbiocard.h"
#include "kbi2c.h"
#include "kbshell.h"
#include "kbcan.h"
#include "chprintf.h"

static iocard_t iocard[4];
static BaseSequentialStream *chp;

static bool compare_iocard_data(int modid, iocard_data_t *o, iocard_data_t *n)
{
	bool ret = false;
	int b;

	for (b = 0; b < 8; b++) {
		if ((o->digital_in_byte & (1 << b)) != (n->digital_in_byte & (1 << b))) {
			kb_can_msg_new(0, modid, b, n->digital_in_byte & (1 << b) ? "\x01" : "\x00", 1);
			ret = true;
		}
	}

	return ret;
}

static WORKING_AREA(waPullThread, 128);
static msg_t pull_thread(void *arg) {
	chRegSetThreadName("iocard_poller");
	(void)arg;
	uint8_t address;
	iocard_data_t iodata;

	while (TRUE) {
		for (address = 0; address < 4; address++) {
			chThdSleepMilliseconds(10);
			if (!iocard[address].is_enabled)
				continue;

			if (!kb_i2c_request(address, &iodata, sizeof(iodata))) {
				if (kb_can_is_ready())
					compare_iocard_data(address, &iocard[address].data, &iodata);
				memcpy(&iocard[address].data, &iodata, sizeof(iodata));
				iocard[address].has_data = 1;
			} else {
				iocard[address].timed_out++;
				if (iocard[address].timed_out > 50 && iocard[address].has_data == 0) {
					iocard[address].is_enabled = false;
					chprintf(chp, "iocard %d disabled.\r\n", address);
				}
			}
		}
	}

	return 0;
}

iocard_t *kb_iocard_get_card(int address)
{
	return &iocard[address];
}

void kb_iocard_init(void)
{
	int n;

	for (n = 0; n < 4; n++) {
		memset(&iocard[n], 0, sizeof(iocard[n]));
		iocard[n].is_enabled = true;
	}

	chp = kb_shell_get_stream();

	chThdCreateStatic(waPullThread, sizeof(waPullThread), NORMALPRIO, pull_thread, NULL);
}
