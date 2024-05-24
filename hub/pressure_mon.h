/*
 * pressure_mon.h
 *
 * Created: 07/05/2024 17:08:37
 *  Author: micro
 */ 


#ifndef PRESSURE_MON_H_
#define PRESSURE_MON_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void pressure_mon_init(void);
bool pressure_mon_get_status(void);


#ifdef __cplusplus
}
#endif

#endif /* PRESSURE_MON_H_ */