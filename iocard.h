#ifndef IOCARD_H
#define IOCARD_H

typedef struct {
	uint8_t   version;         /* Version of this struct, for future use */
	uint8_t   dip_switch;      /* DIP switch value */
	uint8_t   digital_in;      /* Digital inputs */
	uint8_t   digital_out;     /* Feedback from digital outputs */
	uint8_t   over_current;    /* Triggered over current protections */
	uint16_t  analog_in[12];   /* Twelve analog inputs */
	uint16_t  int_temp;        /* Internal CPU temperature */
	uint16_t  int_voltage;     /* Internal VCC voltage */
	uint16_t  int_bandgap;     /* Internal bandgap voltage */
} __attribute__ ((__packed__)) iocard_data_t;

#endif/*IOCARD_H*/

