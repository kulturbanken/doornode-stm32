#ifndef KBIOCARD_H
#define KBIOCARD_H

#include "iocard.h"

typedef struct {
	int is_enabled;
	int has_data;
	int timed_out;
	iocard_data_t data;
} iocard_t;

extern bool kb_iocard_ok_flag;

iocard_t *kb_iocard_get_card(int address);
void kb_iocard_init(void);

#endif/*KBIOCARD_H*/
