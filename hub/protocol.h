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

#ifdef __cplusplus
extern "C" {
#endif

/** Values for the various actions with largest hamming distance */
typedef enum protocol_enum {
   cmd_idle_e = 0x5A,
   cmd_door_push_e = 0x6B,
   cmd_door_pull_e = 0x05,
   cmd_toolsetter_air_blast = 0x34,
   cmd_spindle_chuck_open = 0xD0,
   cmd_spindle_air_clean = 0xE1,
   cmd_pressure_on = 0x8F,
   cmd_pressure_off = 0xBC
} protocol_send_receive_t;

void protocol_init(void);
bool protocol_process(uint8_t raw_data);

#ifdef __cplusplus
}
#endif


#endif /* PROTOCOL_H_ */