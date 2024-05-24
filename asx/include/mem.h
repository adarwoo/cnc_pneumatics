/*
 * mem.h
 *
 * Created: 19/05/2024 21:04:23
 *  Author: micro
 */ 


#ifndef MEM_H_
#define MEM_H_

#include <stdlib.h>

/** Same as calloc - but way smaller and simpler. No free possible */
void *mem_calloc(size_t __nele, size_t __size) __attribute__((__malloc__));


#endif /* MEM_H_ */