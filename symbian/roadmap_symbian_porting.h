#ifndef _ROADMAP_SYMBIAN_PORTING__H_
#define _ROADMAP_SYMBIAN_PORTING__H_

#ifdef __SYMBIAN32__

#include <stdarg.h>
#include <stddef.h>
int snprintf(char * __restrict str, size_t n, char const * __restrict fmt, ...);
int vsnprintf(char * __restrict str, size_t n, const char * __restrict fmt, va_list ap);

extern int global_FreeMapLock();
extern int global_FreeMapUnlock();

typedef enum 
{
	// Standard edit box ( empty allowed, inherits input type from the UI )
	EEditBoxStandard = 0x0,
	// Empty forbidden string edit box
	EEditBoxEmptyForbidden = 0x1,
	// Secured edit box for password
	EEditBoxPassword = 0x2,
	// Numeric edit box
	EEditBoxAlphabetic = 0x4,	// The parent input type will be restored after the editbox is closed 
	// Numeric edit box
	EEditBoxNumeric = 0x8,		// The parent input type will be restored after the editbox is closed
	// Alphanumeric edit box
	EEditBoxAlphaNumeric = 0x10 // The parent input type will be restored after the editbox is closed
	
} TEditBoxType;

extern void ShowEditbox( const char* aTitleUtf8, const char* aTextUtf8, signed char(*aCallback)(int, const char*, void*), void *context, int aBoxType );

#endif

#endif  //   _ROADMAP_SYMBIAN_PORTING__H_
