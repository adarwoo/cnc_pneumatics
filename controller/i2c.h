/*
 * i2c.h
 *
 * Created: 20/05/2024 22:01:06
 *  Author: micro
 */ 


#ifndef I2C_H_
#define I2C_H_



/*
 * i2c.c
 *
 * Created: 14/05/2024 22:17:01
 *  Author: micro
 */ 
#include "twim.h"
#include "reactor.h"
#include "op_codes.h"

#ifdef __cplusplus
extern "C" {
#endif

void i2c_init(reactor_handle_t data_received, reactor_handle_t error_detected);
void i2c_master_send(opcodes_cmd_t code);

/** Check the status of the peripheral */
static inline bool i2c_is_busy(void)
{
   return twim_idle(&TWI0);
}


#ifdef __cplusplus
}
#endif

#endif /* I2C_H_ */