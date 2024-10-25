/*
 * i2c_slave.h
 *
 * Created: 07/05/2024 15:41:34
 *  Author: micro
 */ 


#ifndef I2C_SLAVE_H_
#define I2C_SLAVE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "reactor.h"

/** @brief Initialise the i2c slave device */
void i2c_slave_init(reactor_handle_t);

#ifdef __cplusplus
}
#endif


#endif /* I2C_SLAVE_H_ */