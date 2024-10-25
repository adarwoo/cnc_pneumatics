/*
 * i2c_slave.c
 *
 * Created: 07/05/2024 14:43:19
 *  Author: micro
 */ 
#include "timer.h"
#include "protocol.h"
#include "twis.h"
#include "pressure_mon.h"
#include "conf_twi.h"


/** The slave driver instance */
TWI_Slave_t slave;
   
/** Reactor handler to call when data is received */
reactor_handle_t _react_i2c_handler = REACTOR_NULL_HANDLE;


/**
 * Called from within the interrupt of the twi to handle the data 
 * Note: The reactor is not used to avoid any delay
 */
static void slave_process(void) 
{
   uint8_t received = slave.receivedData[0];
   
   // Ready the data to send (slave write for a master read)
   slave.sendData[0] = opcodes_encode_reply(
      pressure_mon_reply(),
      received
   );
   
   reactor_notify(_react_i2c_handler, (void *)(uint16_t)received);
}


void i2c_slave_init(reactor_handle_t react_i2c_handler)
{
   // Store the reactor handler
   _react_i2c_handler = react_i2c_handler;
   
   TWI_SlaveInitializeDriver(&slave, &TWI0, slave_process);
   TWI_SlaveInitializeModule(&slave, TWI_SLAVE_ADDR);
}   

ISR(TWI0_TWIS_vect)
{
   TWI_SlaveInterruptHandler(&slave);
}
