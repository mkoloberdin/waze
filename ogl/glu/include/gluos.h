/*
** gluos.h - operating system dependencies for GLU
**
** $Header: //depot/main/gfx/lib/glu/include/gluos.h#4 $
*/

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOIME
#include <windows.h>

/* Disable warnings */
#pragma warning(disable : 4101)
#pragma warning(disable : 4244)
#pragma warning(disable : 4761)

#else

/* Disable Microsoft-specific keywords */
#define GLAPI
#define WINGDIAPI

#endif
