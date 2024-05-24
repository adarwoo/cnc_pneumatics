/*
 * memfence.c
 *
 * Created: 19/05/2024 20:32:49
 *  Author: micro
 */ 
#include "alert.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <avr/io.h>

extern char __heap_start;
extern char __heap_end;

#define HEAP_START ((uint16_t)&__heap_start)
#define HEAP_SIZE (RAMEND - (uint16_t)&__heap_start - 32)

/** Store the address to return for the next element */
static char *_heap_allocation_next_block = &__heap_start;


static void
   __attribute__ ((naked))
   __attribute__ ((section (".init5")))    /* run this right before main */
   __attribute__ ((unused))    /* Kill the unused function warning */
   _mem_stack_init(void)
{
   memset(&__heap_start, 0xaa, HEAP_SIZE);
}

/**
 * @return A pointer to the allocated block of memory initialized to 0
 */
void *mem_calloc(size_t __nele, size_t __size)
{
   size_t block_size = __nele * __size;
   
   char *retval = _heap_allocation_next_block;
   char *next = _heap_allocation_next_block + block_size;
   
   // Make sure it contains 0xaa - else too close to the stack
   alert_and_stop_if(*next != 0xaa);
   
   memset(retval, 0, block_size);
   
   _heap_allocation_next_block += block_size;
   
   return retval;
}