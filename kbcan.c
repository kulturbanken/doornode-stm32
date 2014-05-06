#include <ch.h>
#include <hal.h>
#include <string.h>

#include "kbcan.h"

static int node_id;
static bool can_is_ready = false;

#define KB_CAN_MSG_QUEUE_LENGTH 16

static struct {
	kb_can_msg_t     msgs[KB_CAN_MSG_QUEUE_LENGTH];
	int              start;
	int              end;
	Mutex            mtx;
	BinarySemaphore	 sem;
} msgq;

static inline bool msgq_is_empty(void)
{
	return msgq.start == msgq.end;
}

static inline bool msgq_is_full(void)
{
	return (msgq.end + 1) % KB_CAN_MSG_QUEUE_LENGTH == msgq.start;
}

int kb_can_msg_new(int priority, int modid, int endpoint, char *data, int datalen)
{
	chMtxLock(&msgq.mtx);

	if (msgq_is_full()) {
		chMtxUnlock();
		return 1;
	}

	msgq.msgs[msgq.end].priority = priority & 0x03;
	msgq.msgs[msgq.end].modid = modid & 0x0F;
	msgq.msgs[msgq.end].endpoint = endpoint & 0x3F;
	msgq.msgs[msgq.end].datalen = datalen;
	memcpy(&msgq.msgs[msgq.end].data, data, datalen);

	msgq.end = (msgq.end + 1) % KB_CAN_MSG_QUEUE_LENGTH;
	if (msgq.end == msgq.start)
		msgq.start = (msgq.start + 1) % KB_CAN_MSG_QUEUE_LENGTH;

	chMtxUnlock();

	chBSemSignal(&msgq.sem);

	return 0;
}

static kb_can_msg_t msgq_get_next_msg(void)
{
	kb_can_msg_t msg;

	msg = msgq.msgs[msgq.start];
	msgq.start = (msgq.start + 1) % KB_CAN_MSG_QUEUE_LENGTH;

	return msg;
}

/*
 * Receiver thread.
 */
 static WORKING_AREA(can_rx_wa, 256);
 static msg_t can_rx(void *p) {
 	EventListener el;
 	CANRxFrame rxmsg;

 	(void)p;
 	chRegSetThreadName("receiver");
 	chEvtRegister(&CAND1.rxfull_event, &el, 0);
 	while(!chThdShouldTerminate()) {
 		if (chEvtWaitAnyTimeout(ALL_EVENTS, MS2ST(100)) == 0)
 			continue;
 		while (canReceive(&CAND1, CAN_ANY_MAILBOX, &rxmsg, TIME_IMMEDIATE) == RDY_OK) {
      /* Process message.*/
      //palTogglePad(IOPORT3, GPIOC_LED);
 			palTogglePad(GPIOA, GPIOA_YELLOW_LED);
 		}
 	}
 	chEvtUnregister(&CAND1.rxfull_event, &el);
 	return 0;
}

static CANTxFrame generate_can_msg(kb_can_msg_t msg)
{
	CANTxFrame txmsg;

	int id =   (msg.priority << 27)
		  	 | (node_id << 21)
			 | (msg.modid << 17)
			 | (msg.endpoint << 11);

 	txmsg.IDE = CAN_IDE_EXT;
 	txmsg.EID = id;
 	txmsg.RTR = CAN_RTR_DATA;
 	txmsg.DLC = msg.datalen;
 	memcpy(&txmsg.data8, &msg.data, msg.datalen);

 	return txmsg;
}

/*
 * Transmitter thread.
 */
static WORKING_AREA(can_tx_wa, 256);
static msg_t can_tx(void * p) {
 	CANTxFrame txmsg;
 	kb_can_msg_t msg;
 	bool do_send = false;

 	(void)p;
 	chRegSetThreadName("transmitter");

 	while (!chThdShouldTerminate()) {
 		chMtxLock(&msgq.mtx);
 		if (!msgq_is_empty()) {
 			msg = msgq_get_next_msg();
 			txmsg = generate_can_msg(msg);
 			do_send = true;
 		}
 		chMtxUnlock();

 		if (do_send) {
 			do_send = false;
 			palTogglePad(GPIOA, GPIOA_GREEN_LED);
	 		canTransmit(&CAND1, CAN_ANY_MAILBOX, &txmsg, MS2ST(100));
 		} else {
 			chBSemWait(&msgq.sem);
 		}

 		chThdSleepMilliseconds(10);
 	}
 	return 0;
}

/*
 * 500KBaud, automatic wakeup, automatic recover from abort mode.
 */

#define brp ((36000000/18)/500000)

static const CANConfig cancfg = {
 	CAN_MCR_ABOM | 
 	CAN_MCR_AWUM |
 	CAN_MCR_TXFP | 
//    CAN_MCR_NART |
 	0
 	,
	//CAN_BTR_LBKM |
 	//CAN_BTR_SJW(1) | 
  	//CAN_BTR_TS2(6) |
  	//CAN_BTR_TS1(9) | 
  	//CAN_BTR_BRP(6)
  	//0x001c0003
 	((((4-1) & 0x03) << 24) | (((5-1) & 0x07) << 20) | (((12-1) & 0x0F) << 16) | ((brp-1) & 0x1FF))
 };

 /*

 	CAN 29bit ID:

                     2                   1
     8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 	|Pri|  Node ID  | ModID |  Endpoint |         ---         |
 	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
              |               |               |               |

	Priority  =  2 bits  (0-4)
	Node ID   =  6 bits (0-63)
	Module ID =  4 bits (0-15)
	Endpoint  =  6 bits (0-63)
	reserved  = 11 bits 
	            -------
	Total     = 29 bits

*/ 

static void setup_can_filter(void)
{
	CANFilter cfp = {
		/* We only use Filter Bank 0 at this time */
		.filter = 0,

		/* Use Mask mode */
		.mode = 0,

		/* Use 32 bits mode */
		.scale = 1,

		/* Assign the Filter Bank to FIFO0 is the only supported option */
		.assignment = 0,

		/* Set the Identifier part to Node ID (3 LSB is not ID, see ref.man. 24.7.4) */
		.register1 = (node_id << 21) << 3,

		/* And set the Mask part for Node IDs */
		.register2 = (0x3F << 21) << 3
	};

	/* The number 10 for first filter bank for CAN2 is completely made up! */
	canSTM32SetFilters(10, 1, &cfp);
}

bool kb_can_is_ready(void)
{
	return can_is_ready;
}

void kb_can_init(int _node_id)
{
 	rccDisableUSB(0);

 	node_id = _node_id & 0x3F;

 	canInit();
 	setup_can_filter();
	canStart(&CAND1, &cancfg);

	chMtxInit(&msgq.mtx);
	chBSemInit(&msgq.sem, 0);

    chThdCreateStatic(can_rx_wa, sizeof(can_rx_wa), NORMALPRIO + 7, can_rx, NULL);
    chThdCreateStatic(can_tx_wa, sizeof(can_tx_wa), NORMALPRIO + 7, can_tx, NULL);

    can_is_ready = true;
}
