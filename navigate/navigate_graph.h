/* navigate_graph.h - generic navigate functions
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

#ifndef _NAVIGATE_GRAPH_H_
#define _NAVIGATE_GRAPH_H_

#define REVERSED (1 << 31)

struct successor {
	int square_id;
   int line_id;
   unsigned char reversed;
   int to_point;
};

int get_connected_segments (int square,
									 int seg_line_id, int is_seg_reversed,
                            int node_id, struct successor *successors,
                            int max, int use_restrictions, int use_directions);

int navigate_graph_get_line (int node, int line_no);
void navigate_graph_clear (int square);

#endif /* _NAVIGATE_GRAPH_H_ */

