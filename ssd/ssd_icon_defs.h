/* ssd_bitmap.h - Bitmap widget
 *
 * LICENSE:
 *
 *   Copyright 2006 PazO
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

#ifndef __SSD_WIDGET_ICON_DEFS_H__
#define __SSD_WIDGET_ICON_DEFS_H__

#define  LOAD_WIDE_ICON(_part_)                                                        \
   this->_part_ = roadmap_res_get( RES_BITMAP, RES_SKIN|RES_LOCK, filenames->_part_);  \
   if( !this->_part_)                                                                  \
   {                                                                                   \
      assert(0);                                                                       \
      return FALSE;                                                                    \
   }                                                                                   \
   /* Verify sizes:  */                                                                \
   if( -1 == s_height)                                                                 \
      s_height = roadmap_canvas_image_height( this->_part_);                           \
   else                                                                                \
   {                                                                                   \
      if( s_height != roadmap_canvas_image_height( this->_part_))                      \
      {                                                                                \
         assert(0);                                                                    \
         return FALSE;                                                                 \
      }                                                                                \
   }


#endif // __SSD_WIDGET_ICON_DEFS_H__

