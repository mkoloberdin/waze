/*
============================================================================
 Name		: FreeMap.pan
 Author	  : Ehud Shabtai
 Copyright   : GPL
 Description : This file contains panic codes.
============================================================================
*/

#ifndef __FREEMAP_PAN__
#define __FREEMAP_PAN__

/** FreeMap application panic codes */
enum TFreeMapPanics
	{
	EFreeMapUi = 1
	// add further panics here
	};

inline void Panic(TFreeMapPanics aReason)
	{
	_LIT(applicationName,"FreeMap");
	User::Panic(applicationName, aReason);
	}

#endif // __FREEMAP_PAN__
