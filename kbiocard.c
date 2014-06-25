#include "string.h"
#include "ch.h"
#include "kbiocard.h"
#include "kbi2c.h"
#include "kbshell.h"
#include "kbcan.h"
#include "chprintf.h"

#define ANALOG_THRESHOLD 20
#define ANALOG_SUB (ANALOG_THRESHOLD - 5)

bool kb_iocard_ok_flag = false;

static iocard_t iocard[4];
static BaseSequentialStream *chp;

/**
 * Compare cached IO card values with new updated values
 *
 * @param address  The address of the IO card from where the values originate
 * @param cached   The locally cached data values
 * @param updated  New updated data values from last poll
 */
static bool compare_iocard_data(int address, iocard_data_t *cached, iocard_data_t *updated)
{
	bool ret = false;
	int b;

	for (b = 0; b < 8; b++) {
		if ((cached->digital_in_byte & (1 << b)) != (updated->digital_in_byte & (1 << b))) {
			kb_can_msg_new(0, address + 1, b, updated->digital_in_byte & (1 << b) ? "\x01" : "\x00", 1);
			ret = true;
		}
	}

	if (ret) {
		cached->digital_in_byte = updated->digital_in_byte;
	}

	for (b = 0; b < 15; b++) {
		ret = false;

		if (cached->analog_in_array[b] < updated->analog_in_array[b] - ANALOG_THRESHOLD) {
			cached->analog_in_array[b] = updated->analog_in_array[b] - ANALOG_SUB;
			ret = true;
		} else if (cached->analog_in_array[b] > updated->analog_in_array[b] + ANALOG_THRESHOLD) {
			cached->analog_in_array[b] = updated->analog_in_array[b] + ANALOG_SUB;
			ret = true;
		}

		if (ret) {
			kb_can_msg_new(0, address + 1, 8 + b, (char *)&updated->analog_in_array[b], sizeof(uint16_t));
		}
	}

	/* Set the current output pins according to the cached output pin data */
	uint8_t data = 0x00, mask = 0x00;
	for (b = 0; b < 8; b++) {
		if ((cached->digital_out_byte & (1 << b)) != (updated->digital_out_byte & (1 << b))) {
			data |= cached->digital_out_byte & (1 << b);
			mask |= (1 << b);
		}
	}

	if (mask) {
		kb_i2c_set_output(address, mask, data);
	}

	return ret;
}

static WORKING_AREA(waPullThread, 128);
static msg_t pull_thread(void *arg) {
	chRegSetThreadName("iocard_poller");
	(void)arg;
	int address;
	iocard_data_t iodata;

	while (TRUE) {
		for (address = 0; address < 4; address++) {
			chThdSleepMilliseconds(10);
			if (!iocard[address].is_enabled)
				continue;

			if (kb_i2c_request(address, &iodata, sizeof(iodata)) == 0) {
				//chprintf(chp, "Got DIP %d from %d\r\n", iodata.dip_switch, address);
				if (kb_can_is_ready()) {
					kb_iocard_ok_flag = true;
					compare_iocard_data(address, &iocard[address].data, &iodata);
				} else {
					/* While waiting for CAN to start, make sure all cached iocard
					   data is set with initial data */
					memcpy(&iocard[address].data, &iodata, sizeof(iodata));
				}
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
		iocard[n].has_data = 0;
	}

	chp = kb_shell_get_stream();

	chThdCreateStatic(waPullThread, sizeof(waPullThread), NORMALPRIO, pull_thread, NULL);
}
