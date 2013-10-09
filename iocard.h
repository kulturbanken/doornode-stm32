#ifndef IOCARD_H
#define IOCARD_H

typedef struct {
	uint8_t   sirene1:1;
	uint8_t   flash1:1;
	uint8_t   sirene2:1;
	uint8_t   flash2:1;
	uint8_t   door_lock1:1;
	uint8_t   door_lock2:1;
	uint8_t   keypad:1;
	uint8_t   ir_detector:1;
} __attribute__ ((__packed__)) iocard_output_t;

typedef struct {
	uint8_t   version;          /* Version of this struct, for future use */
	uint8_t   dip_switch;       /* DIP switch value */
	union {
		struct {
			uint8_t   door1:1;
			uint8_t   door2:1;
			uint8_t   balcony:1;
			uint8_t   _unused:5;
		} digital_in;
		uint8_t  digital_in_byte;
	};
	union {
		iocard_output_t digital_out;
		uint8_t  digital_out_byte;
	};
	union {
		iocard_output_t over_current;
		uint8_t over_current_byte;
	};
	union {
		struct {
			uint16_t  sirene1;
			uint16_t  flash1;
			uint16_t  sirene2;
			uint16_t  flash2;
			uint16_t  door_lock2;
			uint16_t  door_lock1;
			uint16_t  keypad_pwr;
			uint16_t  ir_detector_pwr;
			uint16_t  supply_12v;
			uint16_t  tamper_loop;
			uint16_t  ir_loop1;
			uint16_t  ir_loop2;
			uint16_t  int_temp;         /* Internal CPU temperature */
			uint16_t  int_voltage;      /* Internal VCC voltage */
			uint16_t  int_bandgap;      /* Internal bandgap voltage */
		} analog_in;
		uint16_t analog_in_array[12];
	};
} __attribute__ ((__packed__)) iocard_data_t;

#endif/*IOCARD_H*/

