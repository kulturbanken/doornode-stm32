#ifndef KBI2C_H
#define KBI2C_H

#include "i2c.h"
#include "iocard.h"

void kb_i2c_init(void);
i2cflags_t kb_i2c_request_fake(uint8_t address);
i2cflags_t kb_i2c_set_output(uint8_t address);
uint16_t kb_i2c_get_data(void);
iocard_data_t *kb_i2c_get_iocard_data(void);

#endif
