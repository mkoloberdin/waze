/*
 * LICENSE:
 *
 *   Copyright 2009 PazO
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "efficient_buffer.h"
#include <string.h>
#include <stdlib.h>

static int s_StaticAllocationsCount   = 0;
static int s_DynamicAllocationsCount  = 0;

void  ebuffer_init( ebuffer_ptr this)
{ memset( this, 0, sizeof(ebuffer));}

char* ebuffer_alloc( ebuffer_ptr this, int size)
{
   ebuffer_free( this);

   size++; // For the string terminating-NULL char
   
   if( size <= EBUFFER_STATIC_SIZE)
   {
      s_StaticAllocationsCount++;
      this->size = size;
      return this->static_buffer;
   }
   
   // Else
   s_DynamicAllocationsCount++;
   this->dynamic_buffer = malloc( size);
   
   if(this->dynamic_buffer)
      this->size = size;
   
   return this->dynamic_buffer;
}

void ebuffer_free( ebuffer_ptr this)
{
   if( !this->size)
      return;

   if( this->dynamic_buffer)
   {
      free( this->dynamic_buffer);
      
      this->dynamic_buffer = NULL;
      this->size           = 0;
   }
   else
      ebuffer_init( this);
}

char* ebuffer_get_buffer( ebuffer_ptr this)
{
   if( !this->size)
      return NULL;
      
   if( this->dynamic_buffer)
      return this->dynamic_buffer;
   
   return this->static_buffer;
}

int ebuffer_get_string_size( ebuffer_ptr this)
{
   char* p = ebuffer_get_buffer( this);
   
   if( !p || !(*p))
      return 0;
   
   return strlen( p);
}

int ebuffer_get_buffer_size( ebuffer_ptr this)
{ return this->size;}




void  ebuffer_get_statistics( int*  StaticAllocationsCount,
                              int*  DynamicAllocationsCount)
{
   (*StaticAllocationsCount)  = s_StaticAllocationsCount;
   (*DynamicAllocationsCount) = s_DynamicAllocationsCount;
}




