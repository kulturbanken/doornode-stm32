#ifndef KBI2C_H
#define KBI2C_H

#include "ch.h"
#include "hal.h"

void kb_i2c_init(void);
i2cflags_t kb_i2c_request(uint8_t address, void *buf, int buflen);
i2cflags_t kb_i2c_set_output(uint8_t address, uint8_t mask, uint8_t data);
void kb_i2c_reset(void);

#endif
