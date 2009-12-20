
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "roadmap.h"
#include "roadmap_utf8.h"

#include "roadmap_strings.h"


//////////////////////////////////////////////////////////////////////////////////////////////////
void dynstr_reset( char** this_ptr)
{ FREE( *this_ptr)}

void dynstr_copy( char** this_ptr, const char* new_str, size_t max_size)
{
	dynstr_reset( this_ptr);
	
	if( !new_str || !(*new_str) || (max_size < strlen(new_str)))
		return;
	
	(*this_ptr) = calloc( 1, strlen(new_str)+1);
	strcpy( (*this_ptr), new_str);
}

void dynstr_append_string( char** this_ptr, const char* new_str, size_t max_size)
{
	char*	temp = NULL;
	size_t	new_size;
	
	if( NULL == (*this_ptr))
	{
		dynstr_copy( this_ptr, new_str, max_size);
		return;
	}

	if( !new_str || !(*new_str))
		return;

	new_size = strlen(*this_ptr) + strlen(new_str);
	if( max_size < new_size)
		return;
		
	temp = calloc( 1, new_size+1);
	strcpy( temp, (*this_ptr));
	strcat( temp, new_str);
	
	free( *this_ptr);
	(*this_ptr) = temp;
	temp        = NULL;
}

void dynstr_append_char( char** this_ptr, char new_char, size_t max_size)
{
	char*	temp = NULL;
	size_t	new_size;
	
	if( NULL == (*this_ptr))
	{
		(*this_ptr) = calloc( 1, 2);
		sprintf( (*this_ptr), "%c", new_char);
		return;
	}

	new_size = strlen(*this_ptr) + 1;
	if( max_size < new_size)
		return;
		
	temp = calloc( 1, new_size+1);
	sprintf( temp, "%s%c", (*this_ptr), new_char);
	
	free( *this_ptr);
	(*this_ptr) = temp;
	temp		   = NULL;
}

void dynstr_trim_last_char( char* this)
{
	utf8_remove_last_char( this);
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void sttstr_reset( char* this)
{
	if( NULL == this)
		return;
	
	(*this) = '\0';
}

void sttstr_copy( char* this, const char* new_str, size_t buffer_size)
{
	if( NULL == this)
		return;
	
	sttstr_reset( this);
	if( !new_str || !(*new_str) || (buffer_size < strlen(new_str)))
		return;
	
	strcpy( this, new_str);
}

void sttstr_append_string( char* this, const char* new_str, size_t buffer_size)
{
	size_t	new_size;
	
	if( NULL == this)
		return;

	if( !(*this))
	{
		sttstr_copy( this, new_str, buffer_size);
		return;
	}

	if( !new_str || !(*new_str))
		return;

	new_size = strlen(this) + strlen(new_str);
	if( buffer_size < new_size)
		return;
		
	strcat( this, new_str);
}

void sttstr_append_char( char* this, char new_char, size_t buffer_size)
{
	size_t	cur_size;
	size_t	new_size;
	
	if( NULL == this)
		return;

	if( !(*this))
	{
		sprintf( this, "%c", new_char);
		return;
	}

	cur_size = strlen(this);
	new_size = cur_size + 1;
	if( buffer_size < new_size)
		return;
		
	this[cur_size] = new_char;
	this[new_size] = '\0';
}

void sttstr_trim_last_char( char* this)
{
	utf8_remove_last_char( this);
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
