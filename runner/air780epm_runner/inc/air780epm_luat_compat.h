#pragma once

#ifndef LUA_VERSION_NUM
typedef struct lua_State lua_State;
#endif

#ifdef __cplusplus
#ifndef __FORCEINLINE
#define __FORCEINLINE __attribute__((always_inline))
#endif
#ifdef sprintf
#undef sprintf
#endif
#ifdef snprintf
#undef snprintf
#endif
#ifdef vsnprintf
#undef vsnprintf
#endif
#include <errno.h>
#ifndef ENOTSUP
#define ENOTSUP 134
#endif
#ifndef ECANCELED
#define ECANCELED 140
#endif
#ifndef ENOTRECOVERABLE
#define ENOTRECOVERABLE 141
#endif
#ifndef EOWNERDEAD
#define EOWNERDEAD 142
#endif
#endif
