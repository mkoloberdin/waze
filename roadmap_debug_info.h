/* roadmap_debug_info.h - Submit debug files
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
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

#ifndef INCLUDE__ROADMAP_DEBUG_INFO__H
#define INCLUDE__ROADMAP_DEBUG_INFO__H


#define CFG_CATEGORY_DEBUG_INFO         ( "Debug Info" )
#define CFG_ENTRY_DEBUG_INFO_SERVER     ( "Web-Service Address" )
#define CFG_DEBUG_INFO_SERVER_DEFAULT   ""


void roadmap_debug_info_submit (void);
void roadmap_debug_info_submit_confirmed (void);


#endif //INCLUDE__ROADMAP_DEBUG_INFO__H
