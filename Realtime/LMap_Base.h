/*
 * LICENSE:
 *
 *   Copyright 2008 PazO
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

#ifndef	__LMAP_BASE_H__
#define	__LMAP_BASE_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef	BYTE
	typedef unsigned char       BYTE;
#endif	//	BYTE
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#if defined (__SYMBIAN32__)
#ifndef TOUCH_SCREEN
#define  RT_DEVICE_ID                     (10)
#else
#define  RT_DEVICE_ID                     (11)
#endif

#elif defined (IPHONE)
#define	RT_DEVICE_ID							(20)

#elif defined (UNDER_CE)
#ifdef TOUCH_SCREEN
#define	RT_DEVICE_ID							(30)
#else
#define	RT_DEVICE_ID							(31)
#endif

#elif defined (_WIN32)
#define	RT_DEVICE_ID							(40)

#elif defined (ANDROID)
#define	RT_DEVICE_ID							(50)

#elif defined (__linux__)
#define	RT_DEVICE_ID							(90)

#else
#error UNKNOWN DEVICE
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#include "../roadmap.h" // logger
#include <assert.h>
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#define  FREE_MEM(_p_)     \
   if( NULL != (_p_))      \
   {                       \
      free(_p_);           \
      (_p_)=NULL;          \
   }
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif	//	__LMAP_BASE_H__
