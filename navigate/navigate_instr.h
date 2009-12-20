/* navigate_instr.h - Navigation instructions
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
 */

#ifndef INCLUDE__NAVIGATE_INSTR__H
#define INCLUDE__NAVIGATE_INSTR__H

#include "navigate/navigate_main.h"

#define LINE_START 0
#define LINE_END   1

typedef NavigateSegment * (*SegmentIterator) (int i);

int navigate_instr_calc_length (const RoadMapPosition *position,
                                const NavigateSegment *segment,
                                int type);

void navigate_instr_fix_line_end (RoadMapPosition *position,
                                   NavigateSegment *segment,
                                   int type);

int navigate_instr_prepare_segments (SegmentIterator get_segment,
                                     int count,
                                     int count_new,
                                     RoadMapPosition *src_pos,
                                     RoadMapPosition *dst_pos);

void navigate_instr_calc_cross_time (NavigateSegment *segments,
                                     int count);
                                     
#endif // INCLUDE__NAVIGATE_INSTR__H

