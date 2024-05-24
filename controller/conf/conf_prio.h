/*
 * conf_prio.h
 *
 * Created: 17/05/2024 15:11:34
 *  Author: micro
 */ 


#ifndef CONF_PRIO_H_
#define CONF_PRIO_H_

#define TIMER_PRIO           reactor_prio_very_high        
#define PIEZZO_PRIO          reactor_prio_high
#define TWI_PRIO             reactor_prio_high_plus
#define DIGITAL_OUTPUT_PRIO  reactor_prio_medium
#define DIGITAL_INPUT PRIO   reactor_prio_medium_plus
#define PROTOCOL_CMD_PRIO    reactor_prio_medium_plus_plus


#endif /* CONF_PRIO_H_ */