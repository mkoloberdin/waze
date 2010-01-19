/* roadmap_ticker.h
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
 *
 *   This file is part of RoadMap.
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
 *
 *
 */

#ifndef INCLUDE__ROADMAP_TICKER__H
#define INCLUDE__ROADMAP_TICKER__H

#define TICKER_OPEN_IMAGE       ("ticker_left_open")
#define TICKER_CLOSED_IMAGE     ("ticker_handle_closed")
#define TICKER_MIDDLE_IMAGE     ("ticker_middle")
#define TICKER_DIVIDER			  ("ticker_divider")

#define TICKER_FOREGROUND       ("#343434")

void roadmap_ticker_initialize(void);
void roadmap_ticker_display();

void roadmap_ticker_hide(void);
void roadmap_ticker_show(void);
int roadmap_ticker_height();
int roadmap_ticker_state(void);
int roadmap_ticker_top_bar_offset(void);
typedef enum
{
	default_event,
	report_event,
	confirm_event,
	road_munching_event,
	comment_event,
	user_contribution_event,
	bonus_points
} lastPointsUpdateEvent;
void roadmap_ticker_set_last_event(int event); // set the last event the updated the points
void roadmap_ticker_set_suppress_hide(BOOL myBool);
void roadmap_ticker_supress_hide(void);
#endif //INCLUDE__ROADMAP_TICKER__H
