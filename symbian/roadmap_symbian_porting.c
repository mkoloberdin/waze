#include "roadmap_symbian_porting.h"

#include <stdio.h>
#include <string.h>

#define KMaxTUint32 0xffffffffu

int snprintf(char * __restrict str, size_t n, char const * __restrict fmt, ...)
{
  size_t on;
  int ret;
  va_list ap;
  FILE f;

  on = n;
  if (n != 0)
    n--;
  if (n > KMaxTUint32)
    n = KMaxTUint32;
  memset(&f, 0, sizeof(f));
  va_start(ap, fmt);
  f._file = -1;
  f._flags = __SWR | __SSTR;
  f._bf._base = f._p = (unsigned char *)str;
  f._bf._size = f._w = n;
  ret = vfprintf(&f, fmt, ap);
  if (on > 0)
    *f._p = '\0';
  va_end(ap);
  return (ret);
}

int vsnprintf(char * __restrict str, size_t n, const char * __restrict fmt, va_list ap)
{
  size_t on;
  int ret;
  char dummy[2];
  FILE f;

  on = n;
  if (n != 0)
    n--;
  if (n > KMaxTUint32)
    n = KMaxTUint32;
  /* Stdio internals do not deal correctly with zero length buffer */
  if (n == 0) {
    if (on > 0)
        *str = '\0';
    str = dummy;
    n = 1;
  }
  memset(&f, 0, sizeof(f));
  f._file = -1;
  f._flags = __SWR | __SSTR;
  f._bf._base = f._p = (unsigned char *)str;
  f._bf._size = f._w = n;
  ret = vfprintf(&f, fmt, ap);
  if (on > 0)
    *f._p = '\0';
  return (ret);
}
