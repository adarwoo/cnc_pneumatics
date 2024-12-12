#ifndef BOARD_H_
#define BOARD_H_

/************************************************************************/
/* Functional I/Os                                                      */
/************************************************************************/
#define IN_MASSO0  IOPORT_CREATE_PIN(PORTA, 2)
#define IN_MASSO1  IOPORT_CREATE_PIN(PORTA, 3)
#define IN_MASSO2  IOPORT_CREATE_PIN(PORTA, 5)
#define IN_MASSO3  IOPORT_CREATE_PIN(PORTA, 6)
#define IN_MASSO4  IOPORT_CREATE_PIN(PORTA, 7)
#define IN_MASSO5  IOPORT_CREATE_PIN(PORTB, 2)
#define IN_MASSO6  IOPORT_CREATE_PIN(PORTB, 3)
#define IN_MASSO7  IOPORT_CREATE_PIN(PORTB, 4)
#define IN_MASSO8  IOPORT_CREATE_PIN(PORTB, 7)
#define IN_MASSO9  IOPORT_CREATE_PIN(PORTC, 0)
#define IN_MASSO10 IOPORT_CREATE_PIN(PORTC, 1)
#define IN_MASSO11 IOPORT_CREATE_PIN(PORTC, 2)
#define IN_MASSO12 IOPORT_CREATE_PIN(PORTC, 3)

/************************************************************************/
/* UART                                                                 */
/************************************************************************/
#define RS485_UART UART0
#define RS485_XDIR_PIN IOPORT_CREATE_PIN(PORTA, 4)

/************************************************************************/
/* I2C                                                                  */
/************************************************************************/


#endif /* BOARD_H_ */