/*
 * protocol.h
 *
 * Created: 07/05/2024 15:16:48
 *  Author: micro
 */ 

// Protocol is 1 tx every 100ms or when a change occurs

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <stdint.h>

#include "op_codes.h"

#ifdef __cplusplus
extern "C" {
#endif

void protocol_init(void);

/** @brief Reactor handler */
void protocol_handle_traffic(void *);

#ifdef __cplusplus
}
#endif


#endif /* PROTOCOL_H_ */