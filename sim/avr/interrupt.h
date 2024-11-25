#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define sei()
#define cli()

typedef uint8_t irqflags_t;

static inline bool cpu_irq_is_enabled_flags(irqflags_t flags)
{
    return false;
}

#define cpu_irq_is_enabled()             cpu_irq_is_enabled_flags(SREG)

inline uint8_t cpu_irq_save() { return 0; }
inline void cpu_irq_restore(uint8_t x) { (void)x; }


#ifdef __cplusplus
#define ISR(x) extern "C" void interrupt_##x()
#else
#define ISR(x) void interrupt_##x()
#endif

#ifdef __cplusplus
}
#endif
