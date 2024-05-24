/*
 * i2c_slave.c
 *
 * Created: 07/05/2024 14:43:19
 *  Author: micro
 */ 
#include "timer.h"
#include "protocol.h"
#include "twis.h"
#include "op_codes.h"
#include "pressure_mon.h"

#define TWI_SLAVE_ADDR   0x54
#define DATA_LENGTH 2

// The slave driver instance
TWI_Slave_t slave; 
uint8_t data[DATA_LENGTH] = {0};
   
// Last command received remains valid for 1 sec no matter what
protocol_send_receive_t last_good_command = cmd_idle_e;
bool allow_change = true;


ISR(TWI0_TWIS_vect)
{
   TWI_SlaveInterruptHandler(&slave);
}
   

/**
 * Called from within the interrupt of the twi to handle the data 
 * Note: The reactor is not used to avoid any delay
 */
static void slave_process(void) 
{
   uint8_t received = slave.receivedData[0];
   
   if ( protocol_process(received) )
   {
      // Ready the data to send (slave write for a master read)
      slave.sendData[0] = opcodes_encode_reply( received, pressure_mon_get_status() );
   }
   else
   {
      slave.sendData[0] = opcodes_cmd_error;
   }
}


void i2c_slave_init(void)
{
   TWI_SlaveInitializeDriver(&slave, &TWI0, slave_process);
   TWI_SlaveInitializeModule(&slave, TWI_SLAVE_ADDR);
}   