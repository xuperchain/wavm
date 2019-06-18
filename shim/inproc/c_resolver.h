#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef void* PTR;
PTR init();
void set_ctxid(PTR ptr, int64_t id);
void release(PTR);
#ifdef __cplusplus
}
#endif
