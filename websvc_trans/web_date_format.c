/*
 * LICENSE:
 *
 *   Copyright 2009 Israel Disatnik
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


#include <stdio.h>
#include <string.h>

#include "web_date_format.h"
#include "mkgmtime.h"
#include "../roadmap.h"
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void	WDF_FormatHttpDate (
				time_t		tStamp,		// IN		-	result of time()
				char *		pDate)		// OUT	-	HTTP-Date format (must be at least WDF_DATE_SIZE + 1 long)
{
	static char *Days[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	static char *Months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	struct tm	*pTime;

	pTime = gmtime (&tStamp);

	sprintf (pDate, "%s, %02d %s %04d %02d:%02d:%02d GMT",
				Days[pTime->tm_wday],
				pTime->tm_mday,
				Months[pTime->tm_mon],
				1900 + pTime->tm_year,
				pTime->tm_hour,
				pTime->tm_min,
				pTime->tm_sec);
}


void	WDF_FormatHttpIfModifiedSince (
				time_t		tStamp,		// IN		-	result of time()
				char *		pHeader)		// OUT	-	If-Modified-Since header (must be at least WDF_MODIFIED_HEADER_SIZE + 1 long)
{
	char	Date[WDF_DATE_SIZE + 1];

	if (tStamp > 0)
	{
		WDF_FormatHttpDate (tStamp, Date);
		sprintf (pHeader, "%s%s%s", WDF_MODIFIED_PREFIX, Date, WDF_MODIFIED_SUFFIX);
	}
	else
	{
		pHeader[0] = '\0';
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////



time_t WDF_TimeFromModifiedSince(const char *modified_since){
   struct tm tms;
   time_t result;
   static char *Months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
   char wkday[4], mon[4];
   int n;

   char test[100];

   // Format is Sun, 06 Nov 2009 02:34:14 GMT
   n = sscanf(modified_since, "%3s, %02d %3s %4d %02d:%02d:%02d GMT",
              wkday, &tms.tm_mday, mon, &tms.tm_year, &tms.tm_hour,
              &tms.tm_min, &tms.tm_sec);
    
   tms.tm_year -= 1900;
   for (n=0; n<12; n++)
      if (strcmp(mon, Months[n]) == 0)
         break;
   tms.tm_mon = n;
   result = mkgmtime(&tms);
   
   WDF_FormatHttpDate(result, &test[0]);
   
   return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
