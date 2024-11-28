#pragma once

extern char sim_registers[];
#define SIM_OFFSET(offset) (&sim_registers[offset])

#define _MMIO_BYTE(mem_addr) (*(volatile uint8_t *)(SIM_OFFSET(mem_addr)))
#define _MMIO_WORD(mem_addr) (*(volatile uint16_t *)(SIM_OFFSET(mem_addr)))
#define _MMIO_DWORD(mem_addr) (*(volatile uint32_t *)(SIM_OFFSET(mem_addr)))

#define __SFR_OFFSET 0x20

#define _SFR_MEM8(mem_addr) _MMIO_BYTE(mem_addr)
#define _SFR_MEM16(mem_addr) _MMIO_WORD(mem_addr)
#define _SFR_MEM32(mem_addr) _MMIO_DWORD(mem_addr)
#define _SFR_IO8(io_addr) _MMIO_BYTE((io_addr) + __SFR_OFFSET)
#define _SFR_IO16(io_addr) _MMIO_WORD((io_addr) + __SFR_OFFSET)

#define _SFR_MEM_ADDR(sfr) ((uint16_t) (&(sfr))
#define _SFR_IO_ADDR(sfr) (_SFR_MEM_ADDR(sfr) - __SFR_OFFSET)
#define _SFR_IO_REG_P(sfr) (_SFR_MEM_ADDR(sfr) < 0x40 + __SFR_OFFSET)

#define _SFR_ADDR(sfr) _SFR_MEM_ADDR(sfr)

#define _SFR_BYTE(sfr) _MMIO_BYTE(_SFR_ADDR(sfr))
#define _SFR_WORD(sfr) _MMIO_WORD(_SFR_ADDR(sfr))
#define _SFR_DWORD(sfr) _MMIO_DWORD(_SFR_ADDR(sfr))
    
#define _BV(bit) (1 << (bit))

#ifndef _VECTOR
#define _VECTOR(N) __vector_ ## N
#endif

#define bit_is_set(sfr, bit) (_SFR_BYTE(sfr) & _BV(bit))

#define bit_is_clear(sfr, bit) (!(_SFR_BYTE(sfr) & _BV(bit)))

#define loop_until_bit_is_set(sfr, bit) do { } while (bit_is_clear(sfr, bit))

#define loop_until_bit_is_clear(sfr, bit) do { } while (bit_is_set(sfr, bit))

