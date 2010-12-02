#ifndef _ROADMAP_SYMBIAN_PORTING__H_
#define _ROADMAP_SYMBIAN_PORTING__H_

#ifdef __SYMBIAN32__

#include <stdarg.h>
#include <stddef.h>
int snprintf(char * __restrict str, size_t n, char const * __restrict fmt, ...);
int vsnprintf(char * __restrict str, size_t n, const char * __restrict fmt, va_list ap);

extern int global_FreeMapLock();
extern int global_FreeMapUnlock();

#endif

#endif  //   _ROADMAP_SYMBIAN_PORTING__H_
