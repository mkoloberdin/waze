/* roadmap_gzm.h - Open and read a gzm compressed map file.
 *
 * LICENSE:
 *
 *   Copyright 2008 Israel Disatnik
 *   Copyright 2008 Ehud Shabtai
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

#ifndef _ROADMAP_GZM__H_
#define _ROADMAP_GZM__H_

#define ROADMAP_GZM_TYPE   ".wzm"

int roadmap_gzm_open (const char *name);
void roadmap_gzm_close (int gzm_id);

int roadmap_map_get_num_tiles (int gzm_id);
int roadmap_map_get_tile_id (int gzm_id, int tile_no);

int roadmap_gzm_get_section (int gzm_id, int tile_id,
									  void **section, int *length);
void roadmap_gzm_free_section (int gzm_id, void *section);
 
#endif // _ROADMAP_GZM__H_
