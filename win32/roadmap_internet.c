/*
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
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


#include "../roadmap_internet.h"
#include <ShellAPI.h>

BOOL roadmap_internet_open_browser( const char* url)
{
   SHELLEXECUTEINFO  EI;
   WCHAR             URL[GENERAL_URL_MAXSIZE+1];
   size_t            chars_count;
   
   if( !url || !(*url))
   {
      roadmap_log(ROADMAP_ERROR, "roadmap_internet_open_browser() - Invalid (empty) input");
      return FALSE;
   }
   
   memset( &EI, 0, sizeof(SHELLEXECUTEINFO));
   
   EI.cbSize   = sizeof(SHELLEXECUTEINFO);
   EI.lpVerb   = L"open";
   EI.lpFile   = URL;
   EI.nShow    = SW_SHOW;
   
   chars_count = mbstowcs( URL, url, GENERAL_URL_MAXSIZE);
   if( (chars_count < 1) || (GENERAL_URL_MAXSIZE <= chars_count))
   {
      roadmap_log(ROADMAP_ERROR, 
                  "roadmap_internet_open_browser() - Failed to convert URL to unicode (%s)", url);
      return FALSE;
   }
   URL[chars_count] = '\0';
   
   if( ShellExecuteEx( &EI))
      return TRUE;
   
   roadmap_log(ROADMAP_ERROR, 
               "roadmap_internet_open_browser() - 'ShellExecuteEx()' failed with error %d, for URL '%s'",
               GetLastError(), url);               

   return FALSE;
}
