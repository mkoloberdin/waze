/* roadmap_iphoneimage.m - iphone image load.
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
 *   Copyright 2009, Waze Ltd
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
 *
 * SYNOPSYS:
 *
 *   See roadmap_iphoneimage.h
 */



/*
#include <stdlib.h>
#include "roadmap.h"
#include "roadmap_lang.h"
#include "roadmap_main.h"
#include "roadmap_path.h"

#include "roadmap_iphonemain.h"
#include "roadmap_messagebox.h"
#include "roadmap_iphonemessagebox.h"

static RoadMapMessageBoxView *gMessageBox = NULL;

#define MESSAGE_BOX_IMAGE "message_box"
#define TITLE_HEIGHT 20.0f
#define BUTTON_HEIGHT 30.0f
#define BUTTON_WIDTH 100.0f

static RoadMapCallback MessageBoxCallback = NULL;
*/

#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_iphoneimage.h"


UIImage *roadmap_iphoneimage_load (const char *name) {
	const char *cursor;
	UIImage *img;
	NSString *fileName;
	
	for (cursor = roadmap_path_first ("skin");
		 cursor != NULL;
		 cursor = roadmap_path_next ("skin", cursor)){
		
		fileName = [NSString stringWithFormat:@"%s/%s.png", cursor, name];
		
		img = [[UIImage alloc] initWithContentsOfFile: fileName];
		
		if (img) break; 
	}
	
	return (img);
}