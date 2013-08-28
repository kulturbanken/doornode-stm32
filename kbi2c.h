#ifndef KBI2C_H
#define KBI2C_H

#include "i2c.h"

void kb_i2c_init(void);
i2cflags_t kb_i2c_request_fake(void);
int16_t kb_i2c_get_data();

#endif
