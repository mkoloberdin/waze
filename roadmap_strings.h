
#ifndef	__STRINGS_H__
#define	__STRINGS_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
//	Manipulations on dynamicly-allocated string:
void	dynstr_reset			( char**	ptr_this);
void	dynstr_copy				( char**	ptr_this,	const char*	new_str,	size_t max_size);
void	dynstr_append_string	( char**	ptr_this,	const char*	new_str,	size_t max_size);
void	dynstr_append_char	( char**	ptr_this,	char			new_char,size_t max_size);
void	dynstr_trim_last_char( char*	this);

//	Manipulations on statically pre-allocated string:
void	sttstr_reset			( char* this);
void	sttstr_copy				( char* this,	const char*	new_str,	size_t buffer_size);
void	sttstr_append_string	( char* this,	const char*	new_str,	size_t buffer_size);
void	sttstr_append_char	( char* this,	char			new_char,size_t buffer_size);
void	sttstr_trim_last_char( char* this);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif	//	__STRINGS_H__

