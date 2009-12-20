/* roadmap_range.c - RoadMap street address operations and attributes.
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

#include <stdlib.h>
#include "roadmap_db_range.h"
#include "roadmap.h"
#include "roadmap_dbread.h"
#include "roadmap_tile_model.h"

#include "roadmap_range.h"

static char *RoadMapRangeType  = "RoadMapRangeContext";

typedef struct {

   char *type;

   RoadMapRange        *RoadMapRanges;
   int                  RoadMapRangesCount;
} RoadMapRangeContext;

static RoadMapRangeContext *RoadMapRangeActive = NULL;


static void *roadmap_range_map (const roadmap_db_data_file *file) {

   RoadMapRangeContext *context;

   context = malloc (sizeof(RoadMapRangeContext));
   roadmap_check_allocated(context);

   context->type = RoadMapRangeType;
   context->RoadMapRanges = NULL;

	if (!roadmap_db_get_data (file,
									  model__tile_range_addr,
									  sizeof(RoadMapRange),
									  (void **)&(context->RoadMapRanges),
									  &(context->RoadMapRangesCount))) {
      roadmap_log (ROADMAP_FATAL,
                   "invalid range structure");
   }

   return context;
}

static void roadmap_range_activate (void *context) {

   RoadMapRangeContext *this = (RoadMapRangeContext *) context;

   if ((this != NULL) && (this->type != RoadMapRangeType)) {
      roadmap_log (ROADMAP_FATAL, "cannot activate (bad context type)");
   }
   RoadMapRangeActive = this;
}

static void roadmap_range_unmap (void *context) {

   RoadMapRangeContext *this = (RoadMapRangeContext *) context;

   if (this->type != RoadMapRangeType) {
      roadmap_log (ROADMAP_FATAL, "cannot unmap (bad context type)");
   }
   if (RoadMapRangeActive == this) {
      RoadMapRangeActive = NULL;
   }
   free (this);
}

roadmap_db_handler RoadMapRangeHandler = {
   "range",
   roadmap_range_map,
   roadmap_range_activate,
   roadmap_range_unmap
};


static void roadmap_range_extract_compact_range 
		(const RoadMapRange *rec,
		 RoadMapStreetRange *left,
		 RoadMapStreetRange *right) {

	left->fradd = (rec->fradd & 0xFF00) >> 8;
	if (left->fradd == 255) left->fradd = -1;
	left->toadd = (rec->toadd & 0xFF00) >> 8;
	if (left->toadd == 255) left->toadd = -1;
	right->fradd = (rec->fradd & 0x00FF);
	if (right->fradd == 255) right->fradd = -1;
	right->toadd = (rec->toadd & 0x00FF);			 	
	if (right->toadd == 255) right->toadd = -1;
}
	 

static void roadmap_range_extract_side
		(const RoadMapRange **rec,
		 RoadMapStreetRange *range) {

	if ((*rec)->fradd == (unsigned short)-1 &&
		 (*rec)->toadd == (unsigned short)-1) {
		
		(*rec)++;
		range->fradd = (((*rec)->street & 0xFF00) << 8) + (*rec)->fradd;
		range->toadd = (((*rec)->street & 0x00FF) << 16) + (*rec)->toadd;
	} else {
	
		range->fradd = (*rec)->fradd;
		range->toadd = (*rec)->toadd;	
	}
	
	(*rec)++;	 	
}


int roadmap_range_get_street
       (int range) {

	if (range == -1) return -1;
	
	if (range & RANGE_FLAG_STREET_ONLY) return range & ~RANGE_FLAG_STREET_ONLY;
	
	if (!RoadMapRangeActive) return -1;
	
	if (range < 0 ||
		 range >= RoadMapRangeActive->RoadMapRangesCount) {
		
		return -1;
	}
	
	return RoadMapRangeActive->RoadMapRanges[range].street & ~RANGE_FLAG_MASK;      	
}


void roadmap_range_get_address
		(int range,
		 RoadMapStreetRange *left,
		 RoadMapStreetRange *right) {

	const RoadMapRange *rec;
	
	if ((!RoadMapRangeActive) ||
		 (range & RANGE_FLAG_STREET_ONLY) ||
		 range < 0 ||
		 range >= RoadMapRangeActive->RoadMapRangesCount) {
		
		left->fradd = -1;
		left->toadd = -1;
		right->fradd = -1;
		right->toadd = -1;
		return;
	}

	rec = RoadMapRangeActive->RoadMapRanges + range;
	
	switch (rec->street & RANGE_FLAG_MASK) {
		
		case RANGE_FLAG_COMPACT:
		
			roadmap_range_extract_compact_range (rec, left, right);
			break;

		case RANGE_FLAG_LEFT_ONLY:
		
			roadmap_range_extract_side (&rec, left);
			right->fradd = -1;
			right->toadd = -1;
			break;
			
		case RANGE_FLAG_RIGHT_ONLY:
		
			roadmap_range_extract_side (&rec, right);
			left->fradd = -1;
			left->toadd = -1;
			break;
			
		case RANGE_FLAG_BOTH:
			roadmap_range_extract_side (&rec, left);
			roadmap_range_extract_side (&rec, right);
			break;

		default:
			roadmap_log (ROADMAP_FATAL, "inconsistency in range flags");
	}
}

