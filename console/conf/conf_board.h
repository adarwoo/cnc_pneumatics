/*
 * board.h
 *
 * Created: 07/05/2024 11:20:17
 *  Author: micro
 */ 
#ifndef BOARD_H_
#define BOARD_H_

// Tracing
#define TRACE_INFO IOPORT_CREATE_PIN(PORTA, 1)
#define TRACE_WARN IOPORT_CREATE_PIN(PORTA, 2)
#define TRACE_ERR  IOPORT_CREATE_PIN(PORTA, 3)

// Share the trace pin
#define ALERT_OUTPUT_PIN TRACE_ERR

/************************************************************************/
/* Functional I/Os                                                      */
/************************************************************************/
#define IOPORT_TOOL_SETTER_AIR_BLAST IOPORT_CREATE_PIN(PORTA, 4)

#define IOPORT_CHUCK_CLAMP IOPORT_CREATE_PIN(PORTA, 5)

#define IOPORT_SPINDLE_CLEAN IOPORT_CREATE_PIN(PORTA, 6)

#define IOPORT_DOOR_PUSH IOPORT_CREATE_PIN(PORTA, 7)
#define IOPORT_DOOR_PULL IOPORT_CREATE_PIN(PORTB, 3)

// Input
#define IOPORT_PRESSURE_READOUT IOPORT_CREATE_PIN(PORTB, 2)


#endif /* BOARD_H_ */