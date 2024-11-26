#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define sei()
#define cli()

#ifdef __cplusplus
#define ISR(x) extern "C" void interrupt_##x()
#else
#define ISR(x) void interrupt_##x()
#endif

#ifdef __cplusplus
}
#endif
