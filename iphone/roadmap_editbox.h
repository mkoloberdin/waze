/* roadmap_editbox.h - RoadMap editbox view
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
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

#ifndef __ROADMAP_EDITBOX__H
#define __ROADMAP_EDITBOX__H

#include "roadmap_factory.h"
#include "ssd/ssd_keyboard.h"

typedef BOOL (*SsdKeyboardCallback) (int exit_code, const char *value, void *context);

typedef enum
	{
		// Standard edit box
		EEditBoxStandard = 0,
		// Empty forbidden string edit box
		EEditBoxEmptyForbidden,
		// Secured edit box for password
	EEditBoxPassword,
	EEditBoxAlphaNumeric,
	EEditBoxNumeric} TEditBoxType;

void ShowEditbox(const char* aTitleUtf8, const char* aTextUtf8, SsdKeyboardCallback callback, void *context, TEditBoxType aBoxType );


#endif // __ROADMAP_EDITBOX__H
