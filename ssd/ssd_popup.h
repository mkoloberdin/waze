/* ssd_popup.h - PopUp widget
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi Ben-Shoshan
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

#ifndef __SSD_POP_UP_H_
#define __SSD_POP_UP_H_
  
#include "ssd_widget.h" 

SsdWidget ssd_popup_new (const char *name, 
						 const char *title,
						 PFN_ON_DIALOG_CLOSED on_popup_closed,
						 int width, 
						 int height, 
						 const RoadMapPosition *position,
                         int flags);
void ssd_popup_update_location(SsdWidget popup, const RoadMapPosition *position);
#endif // __SSD_POP_UP_H_
