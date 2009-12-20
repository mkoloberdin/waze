/* navigate_instr.c - calculate navigation instructions
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
 * SYNOPSYS:
 *
 *   See navigate_instr.h
 */

#include <stdlib.h>
#include <stdio.h>
#include "roadmap.h"
#include "roadmap_line.h"
#include "roadmap_line_route.h"
#include "roadmap_math.h"
#include "roadmap_point.h"
#include "roadmap_street.h"
#include "roadmap_turns.h"
#include "roadmap_layer.h"
#include "roadmap_square.h"
#include "roadmap_locator.h"

#include "navigate_main.h"
#include "navigate_cost.h"
#include "navigate_graph.h"
#include "navigate_instr.h"

static int navigate_instr_azymuth_delta (int az1, int az2) {
   
   int delta;

   delta = az1 - az2;

   while (delta > 180)  delta -= 360;
   while (delta < -180) delta += 360;

   return delta;
}

static int navigate_instr_calc_azymuth (NavigateSegment *seg, int type) {

   RoadMapPosition start;
   RoadMapPosition end;
   RoadMapPosition *shape_pos;
   int shape;

   start = seg->from_pos;
   end   = seg->to_pos;

	roadmap_square_set_current (seg->square);
   if (seg->first_shape > -1) {

      int last_shape;

      if (type == LINE_START) {

         last_shape = seg->first_shape;
         shape_pos  = &end;
         *shape_pos = seg->shape_initial_pos;
      } else {

         last_shape = seg->last_shape;
         shape_pos  = &start;
         *shape_pos = seg->shape_initial_pos;
      }

      for (shape = seg->first_shape; shape <= last_shape; shape++) {

         roadmap_shape_get_position (shape, shape_pos);
      }
   }

   return roadmap_math_azymuth (&start, &end);
}


void navigate_instr_fix_line_end (RoadMapPosition *position,
                                   NavigateSegment *segment,
                                   int type) {

   RoadMapPosition from;
   RoadMapPosition to;
   RoadMapPosition intersection;
   int smallest_distance = 0x7fffffff;
   int distance;
   int seg_shape_end = -1;
   RoadMapPosition seg_end_pos = {0, 0};
   RoadMapPosition seg_shape_initial = {0, 0};
   int i;

	roadmap_square_set_current (segment->square);
   if (segment->first_shape <= -1) {
      
      from = segment->from_pos;
      to = segment->to_pos;
   } else {

      to = from = segment->from_pos;

      for (i = segment->first_shape; i <= segment->last_shape; i++) {

         roadmap_shape_get_position (i, &to);

         distance =
            roadmap_math_get_distance_from_segment
            (position, &from, &to, &intersection, NULL);

         if (distance < smallest_distance) {

            smallest_distance = distance;

            if (type == LINE_START) {

               seg_shape_end = i;
               seg_end_pos = intersection;
               seg_shape_initial = from;
            } else {

               seg_shape_end = i-1;
               seg_end_pos = intersection;
            }
         }

         from = to;
      }

      to = segment->to_pos;
   }

   distance =
      roadmap_math_get_distance_from_segment
      (position, &from, &to, &intersection, NULL);

   if (distance < smallest_distance) {

      if (type == LINE_START) {

         seg_shape_end = -1;
         seg_end_pos = intersection;
         seg_shape_initial = from;
      } else {

         seg_shape_end = segment->last_shape;
         seg_end_pos = intersection;
      }
   }

   if (type == LINE_START) {
      
      segment->from_pos = seg_end_pos;
      segment->shape_initial_pos = seg_shape_initial;
      if ((seg_shape_end < 0) || (seg_shape_end > segment->last_shape)) {
         segment->first_shape = segment->last_shape = -1;
      } else {
         segment->first_shape = seg_shape_end;
      }

   } else {

      segment->to_pos = seg_end_pos;
      if ((seg_shape_end < 0) || (seg_shape_end < segment->first_shape)) {
         segment->first_shape = segment->last_shape = -1;
      } else {
         segment->last_shape = seg_shape_end;
      }
   }
}


static void navigate_instr_fill_segment (int square, int line, int reversed,
													  NavigateSegment *segment) {
	
	int first_shape;
	int last_shape;
													  	
	roadmap_square_set_current (square);
   segment->square = square;
   segment->line = line;
   segment->line_direction = reversed ?
   	ROUTE_DIRECTION_AGAINST_LINE :
   	ROUTE_DIRECTION_WITH_LINE;
   segment->cfcc = roadmap_line_cfcc (line);
   	
   roadmap_line_shapes (line, &first_shape, &last_shape);
   segment->first_shape = first_shape;
   segment->last_shape = last_shape;
   roadmap_line_from (line, &segment->from_pos);
   roadmap_line_to (line, &segment->to_pos);
   segment->shape_initial_pos.longitude = segment->from_pos.longitude;
   segment->shape_initial_pos.latitude = segment->from_pos.latitude;
}


enum RoadSimilarityLevel {
	
	NOT_SIMILAR,
	SAME_CFCC,
	SAME_NAME,
	SAME_NAME_AND_CFCC
};

static int navigate_instr_compare_lines (int street1, int cfcc1,
                                         int street2, int cfcc2,
                                         int delta_azymuth) {

	if (street1 != street2) {
		 
		if (cfcc1 != cfcc2) {
		 	return NOT_SIMILAR;
		} else {
			return SAME_CFCC;
		}
	}
	
	if (cfcc1 != 
		 cfcc2) { 
		return SAME_NAME;
	}
		
	return SAME_NAME_AND_CFCC;
}


static int navigate_instr_has_more_connections (NavigateSegment *seg1,
                                             	NavigateSegment *seg2) {

	int i;
	int junction_node_id;
	struct successor successors[16];
	int count;
	 
	roadmap_square_set_current (seg1->square);                                            	
   if (seg1->line_direction == ROUTE_DIRECTION_WITH_LINE) {
      
      roadmap_line_points
         (seg1->line,
          &i, &junction_node_id);
   } else {
      roadmap_line_points
         (seg1->line,
          &junction_node_id, &i);
   }

   count = get_connected_segments
           (seg1->square,
            seg1->line,
            seg1->line_direction != ROUTE_DIRECTION_WITH_LINE,
            junction_node_id,
            successors,
            sizeof(successors) / sizeof(successors[0]),
            0, 0);

   for (i = 0; i < count; ++i) {
   
   	if ((successors[i].square_id != seg1->square ||
   		  successors[i].line_id != seg1->line) &&
   		 (successors[i].square_id != seg2->square ||
   		  successors[i].line_id != seg2->line)) {
   	
   		return 1;	  	
   	} 	
   }

	return 0;
}


static void navigate_instr_check_neighbours (NavigateSegment *seg1,
                                             NavigateSegment *seg2) {

   RoadMapPosition *junction;
   int junction_node_id;
	struct successor successors[16];
   int i;
   int count;
   int seg1_azymuth;
   int seg2_azymuth;
   int delta;
   int left_delta;
   int right_delta;
   int delta_similarity;
   NavigateSegment next_segment;

   if (seg1->line_direction == ROUTE_DIRECTION_WITH_LINE) {
      
      seg1_azymuth  = navigate_instr_calc_azymuth (seg1, LINE_END);
      /* TODO no plugin support */
      roadmap_line_points
         (seg1->line,
          &i, &junction_node_id);
      junction = &seg1->to_pos;
   } else {
      seg1_azymuth  = 180 + navigate_instr_calc_azymuth (seg1, LINE_START);
      /* TODO no plugin support */
      roadmap_line_points
         (seg1->line,
          &junction_node_id, &i);
      junction = &seg1->from_pos;
   }

   if (seg2->line_direction == ROUTE_DIRECTION_WITH_LINE) {
      
      seg2_azymuth  = navigate_instr_calc_azymuth (seg2, LINE_START);
   } else {
      seg2_azymuth  = 180 + navigate_instr_calc_azymuth (seg2, LINE_END);
   }

   delta = navigate_instr_azymuth_delta (seg1_azymuth, seg2_azymuth);
	delta_similarity = navigate_instr_compare_lines (seg1->street, seg1->cfcc,
																	 seg2->street, seg2->cfcc,
																	 delta);
   left_delta = right_delta = delta;

   /* TODO no plugin support */
   count = get_connected_segments
           (seg1->square,
            seg1->line,
            seg1->line_direction != ROUTE_DIRECTION_WITH_LINE,
            junction_node_id,
            successors,
            sizeof(successors) / sizeof(successors[0]),
            1, 1);

   for (i = 0; i < count; ++i) {

      int line = successors[i].line_id;
      int square = successors[i].square_id;
      int reversed = successors[i].reversed;
      int line_delta;
      int line_similarity;
      int next_azymuth;
      
      if (square != seg2->square) {
      	/* this is not supposed to happen */
      	roadmap_log (ROADMAP_ERROR, "Invalid navigation route check from %d/%d to %d/%d",
      					 seg1->square,
      					 seg1->line,
      					 seg2->square,
      					 seg2->line);
      	seg1->instruction = CONTINUE;
      	return;				 
      }
      
      roadmap_square_set_current (square);
      
      if (line == seg1->line || 
      	 line == seg2->line) {
         continue;
      }

		navigate_instr_fill_segment (square,
											  line,
											  reversed,
											  &next_segment);
		if (next_segment.line_direction == ROUTE_DIRECTION_WITH_LINE) {
			next_azymuth = navigate_instr_calc_azymuth (&next_segment, LINE_START);
		} else {
			next_azymuth = 180 + navigate_instr_calc_azymuth (&next_segment, LINE_END);
		} 
      line_delta = navigate_instr_azymuth_delta
                     (seg1_azymuth, next_azymuth);
                     
      if (line_delta < -45 || line_delta > 45) continue;
      
		line_similarity = navigate_instr_compare_lines (seg1->street, seg1->cfcc,
																		roadmap_line_get_street (line),
																		roadmap_line_cfcc (line),
																		line_delta);
																		
      if (line_similarity > delta_similarity) {
      	left_delta = right_delta = line_delta;
      	delta_similarity = line_similarity;
      } else if (line_similarity == delta_similarity) {
      	if (line_delta > left_delta) left_delta = line_delta;
      	if (line_delta < right_delta) right_delta = line_delta;
      }
   }

	if ((delta < left_delta &&
		  delta > right_delta) ||
		 (delta == left_delta &&
		  delta == right_delta)) {
	
		seg1->instruction = CONTINUE;	 	
	} else if (delta > right_delta) {
		
		seg1->instruction = KEEP_LEFT;
	} else {
		
		seg1->instruction = KEEP_RIGHT; 
	}
}


static void navigate_instr_set_road_instr (NavigateSegment *seg1,
                                          NavigateSegment *seg2) {

   int seg1_azymuth;
   int seg2_azymuth;
   int delta;
   int minimum_turn_degree = 45; //SRUL - optimizing (was 15);

	if (seg1->square != seg2->square) {
		/* must be line crossing tile */
		seg1->instruction = CONTINUE;
		return;
	}
	
	if (seg1->context == SEG_ROUNDABOUT) {
		if (seg2->context == SEG_ROUNDABOUT) seg1->instruction = CONTINUE;
		else											 seg1->instruction = ROUNDABOUT_EXIT;
		return;
	}
	if (seg2->context == SEG_ROUNDABOUT) {
		seg1->instruction = ROUNDABOUT_ENTER;
		return;
	}
	
	if (!navigate_instr_has_more_connections (seg1, seg2)) {
		seg1->instruction = CONTINUE;
		return;
	}

   if (seg1->line_direction == ROUTE_DIRECTION_WITH_LINE) {
      
      seg1_azymuth  = navigate_instr_calc_azymuth (seg1, LINE_END);
   } else {
      seg1_azymuth  = 180 + navigate_instr_calc_azymuth (seg1, LINE_START);
   }


   if (seg2->line_direction == ROUTE_DIRECTION_WITH_LINE) {
      
      seg2_azymuth  = navigate_instr_calc_azymuth (seg2, LINE_START);
   } else {
      seg2_azymuth  = 180 + navigate_instr_calc_azymuth (seg2, LINE_END);
   }

   delta = navigate_instr_azymuth_delta (seg1_azymuth, seg2_azymuth);
/*SRUL - optimizing
   if (roadmap_plugin_same_street (&seg1->street, &seg2->street)) {
      minimum_turn_degree = 45;
   }
*/
   if (delta < -minimum_turn_degree) {

      if (delta > -45) {
         seg1->instruction = KEEP_RIGHT;
      } else {
         seg1->instruction = TURN_RIGHT;
      }

   } else if (delta > minimum_turn_degree) {

      if (delta < 45) {
         seg1->instruction = KEEP_LEFT;
      } else {
         seg1->instruction = TURN_LEFT;
      }
      
   } else {

      navigate_instr_check_neighbours (seg1, seg2);
   }
}


int navigate_instr_calc_length (const RoadMapPosition *position,
                                const NavigateSegment *segment,
                                int type) {

   int total_length = 0;
   int result = 0;

	roadmap_square_set_current (segment->square);
   result =
      roadmap_math_calc_line_length (position,
                                     &segment->from_pos, &segment->to_pos,
                                     segment->first_shape, segment->last_shape,
                                     NULL,
                                     &total_length);

   if (type == LINE_START) {
      
      return result;
   } else {

      return total_length - result;
   }
}


static int navigate_instr_classify_roundabout_azymuth 
					(int entry_azymuth,
					 int exit_azymuth,
					 int tolerance_straight,
					 int tolerance_uturn) {


	int turn_degree = navigate_instr_azymuth_delta (entry_azymuth, exit_azymuth);
	
	if (turn_degree > -180 + tolerance_uturn &&
		 turn_degree <= -tolerance_straight) {
		 return 1; // right
	}
	if (turn_degree > -tolerance_straight &&
		 turn_degree < tolerance_straight) {
		 return 2; // straight ahead
	}	
	if (turn_degree >= tolerance_straight &&
		 turn_degree < 180 - tolerance_uturn) {
		 return 3; // left
	}
	return 0; // going back
}


static void navigate_instr_analyze_roundabout (NavigateSegment *segment_before, 
														 NavigateSegment *segment_after,
														 int *roundabout_is_standard,
														 NavSmallValue *exit_number) {

	int direction_count[4];
	int direction_exit;
	int direction;
	NavigateSegment *start_segment;
	NavigateSegment *next_roundabout_segment;
	struct successor next_segments[3];
	int num_next;
	int next_index;
	NavigateSegment temp_segment;
	NavigateSegment *exit_segment;
	int entry_azymuth;
	int exit_azymuth;
	int dir;
	int curr_square;
	int curr_line;
	int curr_rev;
	int curr_node;
	int found_exit;
	int found_next;
	int roundabout_count;
	int exit_count;
	int calc_direction;
	int continue_loop;
	
	if (segment_before->context != SEG_ROUNDABOUT) {
		calc_direction = 1;
		start_segment = segment_before + 1;
	} else {
		calc_direction = 0;
		start_segment = segment_before;
	}
	
	*roundabout_is_standard = 1;
	*exit_number = -1;
	
	/* trying to figure if this is a standard roundabout, with at most
	 * one exit to each of the four major directions
	 * and try to match the exit in the navigation route
	 * to set a more accurate navigation instruction
	 */
	 
	for (dir = 0; dir < 4; dir++) {
		direction_count[dir] = 0;
	}
	direction_exit = -1;
	
	if (segment_before->line_direction == ROUTE_DIRECTION_WITH_LINE) {
		entry_azymuth = navigate_instr_calc_azymuth (segment_before, LINE_END);
	} else {
		entry_azymuth = 180 + navigate_instr_calc_azymuth (segment_before, LINE_START);
	} 
	
	curr_square = start_segment->square;
	curr_line = start_segment->line;
	curr_rev = start_segment->line_direction != ROUTE_DIRECTION_WITH_LINE;
	next_roundabout_segment = start_segment + 1;
	roundabout_count = 0;
	exit_count = 0;
	do {
		continue_loop = 1;
		
		roadmap_square_set_current (curr_square);
		if (curr_rev) {
			roadmap_line_from_point (curr_line, &curr_node);
		} else {
			roadmap_line_to_point (curr_line, &curr_node);
		}		
		
		/* expecting one or no exit, and one continuing roundabout segment */
		num_next = get_connected_segments (curr_square,
													  curr_line,
													  curr_rev,
													  curr_node,
													  next_segments,
													  3, 1, 1);
		if (num_next < 1 || num_next > 2) {
			/* there is some problem with this roundabout - abort */
			*roundabout_is_standard = 0;
			roadmap_log (ROADMAP_WARNING, "roundabout problem - square %d line %d has %d successors", curr_square, curr_line, num_next);
			if (next_roundabout_segment < segment_after) {
				return;
			}
			continue_loop = 0;
			break;
		}
		
		found_exit = 0;
		found_next = 0;
		for (next_index = 0; next_index < num_next; next_index++) {
			struct successor *next = next_segments + next_index;
			
			roadmap_square_set_current (next->square_id);
			if (roadmap_line_context (next->line_id) == SEG_ROUNDABOUT) {
				if (found_next++) {
					*roundabout_is_standard = 0;
					roadmap_log (ROADMAP_WARNING, "found two roundabout segments originating in same node - square %d line %d", curr_square, curr_line); 
					if (next_roundabout_segment < segment_after) {
						return;
					}
					continue_loop = 0;
					break;
				}
				curr_square = next->square_id;
				curr_line = next->line_id;
				curr_rev = next->reversed;
				curr_node = next->to_point;
				
				/* match route segments with roundabout segments */
				if (next_roundabout_segment < segment_after) {
					if (curr_square != next_roundabout_segment->square ||
						 curr_line != next_roundabout_segment->line) {
						
						/* route/roundabout mismatch */
						*roundabout_is_standard = 0;
						roadmap_log (ROADMAP_WARNING, "found wrong route in roundabout - square %d line %d", curr_square, curr_line); 
						return;
					}
				}
				next_roundabout_segment++;
			} else {
				if (found_exit++) {
					*roundabout_is_standard = 0;
					roadmap_log (ROADMAP_WARNING, "found a roundabout node with no connected roundabout segments - square %d line %d", curr_square, curr_line); 
					if (next_roundabout_segment < segment_after) {
						return;
					}
					continue_loop = 0;
					break;
				}
				exit_count++;
				
				if (next->square_id == segment_after->square &&
					 next->line_id == segment_after->line &&
					 (next->reversed ? 
					 	segment_after->line_direction == ROUTE_DIRECTION_AGAINST_LINE :
					 	segment_after->line_direction == ROUTE_DIRECTION_WITH_LINE)) {
					 		
					 /* this is the current route's exit */
					 exit_segment = segment_after;
				} else {
				
					navigate_instr_fill_segment (next->square_id,
														  next->line_id,
														  next->reversed,
														  &temp_segment);
			      exit_segment = &temp_segment;
				}
				/* find exit direction */
				if (calc_direction) {
					if (exit_segment->line_direction == ROUTE_DIRECTION_WITH_LINE) {
						exit_azymuth = navigate_instr_calc_azymuth (exit_segment, LINE_START);
					} else {
						exit_azymuth = 180 + navigate_instr_calc_azymuth (exit_segment, LINE_END);
					} 
					direction = navigate_instr_classify_roundabout_azymuth (entry_azymuth, exit_azymuth, 30, 15);
					if (direction < 0 || direction >= 4) {
							roadmap_log (ROADMAP_ERROR, "illegal exit direction %d", direction); 
						*roundabout_is_standard = 0;
					} else if (direction_count[direction]++) {
						/* two exits pointing the same way */
						*roundabout_is_standard = 0;
					}
				} else {
					direction = -1;
				}
				if (exit_segment == segment_after) {
					direction_exit = direction;
					*exit_number = exit_count;
				}
			}
		}
		
		if (roundabout_count++ > 16) {
			/* just precaution from infinite loops */
			roadmap_log (ROADMAP_WARNING, "roundabout has too many segments - square %d line %d", curr_square, curr_line); 
			*roundabout_is_standard = 0;
			return;
		}
	} while (continue_loop &&
				(curr_square != start_segment->square ||
				 curr_line != start_segment->line));
	
	if (*roundabout_is_standard) {			
		switch (direction_exit)	{
			case 0:
				segment_before->instruction = ROUNDABOUT_U;
				(segment_after - 1)->instruction = ROUNDABOUT_EXIT_U;
				break;
			case 1:
				segment_before->instruction = ROUNDABOUT_RIGHT;
				(segment_after - 1)->instruction = ROUNDABOUT_EXIT_RIGHT;
				break;
			case 2:
				segment_before->instruction = ROUNDABOUT_STRAIGHT;
				(segment_after - 1)->instruction = ROUNDABOUT_EXIT_STRAIGHT;
				break;
			case 3:
				segment_before->instruction = ROUNDABOUT_LEFT;
				(segment_after - 1)->instruction = ROUNDABOUT_EXIT_LEFT;
				break;
		}	
	}
}


void navigate_instr_calc_cross_time (NavigateSegment *segments,
                                     int count) {

	NavigateSegment *last_segment = segments + count - 1;
	NavigateSegment *segment;
   int prev_line_id = -1;
   int is_prev_reversed = 0;
   int cur_cost = 0;
	
   navigate_cost_reset ();
	for (segment = segments; segment <= last_segment; segment++) {
	
		roadmap_square_set_current (segment->square);
		segment->cross_time = 	
         navigate_cost_time (segment->line,
                 segment->line_direction != ROUTE_DIRECTION_WITH_LINE,
                 cur_cost,
                 prev_line_id,
                 is_prev_reversed);
                 
      prev_line_id = segment->line;
      is_prev_reversed = segment->line_direction != ROUTE_DIRECTION_WITH_LINE;

		if (segment == segments || segment == last_segment) {
         segment->cross_time =
            (int) (1.0 * segment->cross_time * (segment->distance + 1) /
               (roadmap_line_length (segment->line)+1));
		}

		cur_cost += segment->cross_time;
	}
}

int navigate_instr_prepare_segments (SegmentIterator get_segment,
                                     int count,
                                     int count_new,
                                     RoadMapPosition *src_pos,
                                     RoadMapPosition *dst_pos) {

   int i;
   int group_id;
   int group_count;
   NavigateSegment *segment;
   int first_shape;
   int last_shape;

   for (i=0; i < count_new; i++) {

		segment = get_segment (i);
		roadmap_square_set_current (segment->square);
	   roadmap_line_shapes (segment->line, &first_shape, &last_shape);
	   segment->first_shape = first_shape;
	   segment->last_shape = last_shape;
	   roadmap_line_from (segment->line, &segment->from_pos);
	   roadmap_line_to (segment->line, &segment->to_pos);
	   segment->shape_initial_pos.longitude = segment->from_pos.longitude;
	   segment->shape_initial_pos.latitude = segment->from_pos.latitude;

      segment->street = roadmap_line_get_street (segment->line);
      
      segment->context = roadmap_line_context (segment->line);
   }

   for (i=0; i < count - 1 && i < count_new; i++) {

      navigate_instr_set_road_instr (get_segment (i), get_segment (i+1));
   }

	if (count == count_new) {
   	get_segment (count - 1) ->instruction = APPROACHING_DESTINATION;
	}

	if (count_new > 0) {
		segment = get_segment (0);
	   if (segment->line_direction == ROUTE_DIRECTION_WITH_LINE) {
	      navigate_instr_fix_line_end (src_pos, segment, LINE_START);
	   } else {
	      navigate_instr_fix_line_end (src_pos, segment, LINE_END);
	   }
	}

	if (count == count_new) {
		segment = get_segment (count - 1);
	   if (segment->line_direction == ROUTE_DIRECTION_WITH_LINE) {
	      navigate_instr_fix_line_end (dst_pos, segment, LINE_END);
	   } else {
	      navigate_instr_fix_line_end (dst_pos, segment, LINE_START);
	   }
	}

   /* assign group ids */
   for (i = 0, group_id = 0, group_count = 0; i < count; i++) {

		segment = get_segment (i);
		
		segment->group_id = group_id;
      
		if (segment->instruction == ROUNDABOUT_EXIT &&
			 i < count_new - 1) {
			
			int standard; // future use
			NavigateSegment *seg_before = get_segment (i - group_count);
			NavigateSegment *seg_after = get_segment (i + 1);
			//TODO: handle case where roundabout ends in old segments
			navigate_instr_analyze_roundabout (seg_before, seg_after, &standard, 
														  &(seg_before->exit_no));
		}
		
      if (segment->instruction != CONTINUE) {
      	
      	group_id++;
      	group_count = 0;
      }
		group_count++;
   }

   /* Calculate lengths and ETA for each segment */
   for (i = 0; i < count_new; i++) {

		segment = get_segment (i);
		
      if ((i == 0) || (i == (count -1))) {

         if (segment->line_direction == ROUTE_DIRECTION_WITH_LINE) {
            segment->distance =
               navigate_instr_calc_length
                  (&segment->from_pos, segment, LINE_END);

         } else {
            segment->distance =
               navigate_instr_calc_length
                  (&segment->to_pos, segment, LINE_START);
         }
      } else {

			roadmap_square_set_current (segment->square);
         segment->distance = roadmap_line_length (segment->line);
      }
      
      segment->is_instrumented = 1;
   }

	segment = get_segment (0);
	navigate_instr_calc_cross_time (segment, count_new);
	
   return 0;
}


