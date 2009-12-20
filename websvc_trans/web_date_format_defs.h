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


#ifndef  __FREEMAP_WDFDEFS_H__
#define  __FREEMAP_WDFDEFS_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#define  WDF_MODIFIED_PREFIX              ("If-Modified-Since: ")
#define  WDF_MODIFIED_PREFIX_SIZE         (sizeof(WDF_MODIFIED_PREFIX)-1)
#define  WDF_MODIFIED_SUFFIX              ("\r\n")
#define  WDF_MODIFIED_SUFFIX_SIZE         (sizeof(WDF_MODIFIED_SUFFIX)-1)

#define	WDF_DATE_SIZE							(29) // Example: Sun, 25 Jan 2009 12:00:00 GMT

#define	WDF_MODIFIED_HEADER_SIZE			(WDF_MODIFIED_PREFIX_SIZE + WDF_DATE_SIZE + WDF_MODIFIED_SUFFIX_SIZE)
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif   //   __FREEMAP_WDFDEFS_H__

