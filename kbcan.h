#ifndef INC_KBCAN_H
#define INC_KBCAN_H

typedef struct {
	int priority;
	int modid;
	int endpoint;
	int datalen;
	uint8_t data[8];
} kb_can_msg_t;

extern bool kb_can_ok_flag;

void kb_can_init(int can_addr);
int kb_can_msg_new(int priority, int modid, int endpoint, char *data, int datalen);
bool kb_can_is_ready(void);

#endif