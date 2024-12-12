#pragma once

#define sleep_enable()

#ifdef __cplusplus
extern "C" {
#endif

// Use this to inject into the reactor
void sleep_cpu();

#ifdef __cplusplus
}
#endif
