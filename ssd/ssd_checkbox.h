/* ssd_checkbox.h - Combo box widget
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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
 */

#ifndef __SSD_CHECKBOX_H_
#define __SSD_CHECKBOX_H_

#include "ssd_widget.h"

#define CHECKBOX_STYLE_ON_OFF  0
#define CHECKBOX_STYLE_ROUNDED 1
#define CHECKBOX_STYLE_DEFAULT 2

SsdWidget ssd_checkbox_new (const char *name,
							BOOL Selected,
                          	int flags,
                          	SsdCallback callback,
 	                        const char *checked_icon,
                            const char *unchecked_icon,
                          	int style);


#endif // __SSD_CHECKBOX_H_
