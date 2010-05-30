/* roadmap_analytics.c
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi R.
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


#include "roadmap_analytics.h"

#ifdef FLURRY
extern void roadmap_flurry_log_event (const char *event_name, const char *info_name, const char *info_val);
#endif //FLURRY


//////////////////////////////////////////////////////////////////
void roadmap_analytics_log_event (const char *event_name, const char *info_name, const char *info_val) {
#ifdef FLURRY
   roadmap_flurry_log_event (event_name, info_name, info_val);
#endif //FLURRY
}