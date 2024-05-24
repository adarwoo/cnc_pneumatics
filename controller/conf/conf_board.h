/*
 * board.h
 *
 * Created: 07/05/2024 11:20:17
 *  Author: micro
 */ 
#ifndef BOARD_H_
#define BOARD_H_

// Tracing
#define TRACE_INFO IOPORT_CREATE_PIN(PORTA, 0)

// Share the trace pin
#define ALERT_OUTPUT_PIN LED_FAULT

/************************************************************************/
/* Functional I/Os                                                      */
/************************************************************************/
#define OC_DOOR_CLOSED IOPORT_CREATE_PIN(PORTA, 2)
#define OC_CHUCH_RELEASED IOPORT_CREATE_PIN(PORTA, 1)

#define PIEZZO_DRIVE_PIN IOPORT_CREATE_PIN(PORTA, 3)

#define IN_CHUCK_OPEN IOPORT_CREATE_PIN(PORTA, 4)
#define IN_SPINDLE_AIR_BLAST IOPORT_CREATE_PIN(PORTA, 5)
#define IN_TOOLSET_AIR_BLAST IOPORT_CREATE_PIN(PORTA, 6)
#define IN_SOUNDER IOPORT_CREATE_PIN(PORTA, 7)
#define IN_BEEP IOPORT_CREATE_PIN(PORTB, 4)
#define IN_DOOR_OPEN_CLOSE IOPORT_CREATE_PIN(PORTB, 5)

#define IN_DOOR_UP IOPORT_CREATE_PIN(PORTB, 2)
#define IN_DOOR_DOWN IOPORT_CREATE_PIN(PORTB, 3)

#define LED_CHUCK IOPORT_CREATE_PIN(PORTC, 0)
#define LED_DOOR_CLOSING IOPORT_CREATE_PIN(PORTC, 1)
#define LED_DOOR_OPENING IOPORT_CREATE_PIN(PORTC, 2)
#define LED_FAULT IOPORT_CREATE_PIN(PORTC, 3)

#endif /* BOARD_H_ */