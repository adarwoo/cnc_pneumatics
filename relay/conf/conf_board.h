#ifndef BOARD_H_
#define BOARD_H_

// Tracing
#define TRACE_INFO IOPORT_CREATE_PIN(PORTA, 3)
#define TRACE_WARN IOPORT_CREATE_PIN(PORTA, 5)
#define TRACE_ERR  IOPORT_CREATE_PIN(PORTB, 2)

// Leds
#define LED_A IOPORT_CREATE_PIN(PORTB, 1)
#define LED_B IOPORT_CREATE_PIN(PORTB, 0)
#define LED_C IOPORT_CREATE_PIN(PORTA, 2)

// Relay
#define RELAY_A IOPORT_CREATE_PIN(PORTB, 3)
#define RELAY_B IOPORT_CREATE_PIN(PORTA, 7)
#define RELAY_C IOPORT_CREATE_PIN(PORTA, 6)


/************************************************************************/
/* UART                                                                 */
/************************************************************************/
#define RS485_UART UART0
#define RS485_XDIR_PIN IOPORT_CREATE_PIN(PORTA, 4)


#endif /* BOARD_H_ */