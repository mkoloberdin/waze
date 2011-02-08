/* RealtimePopUp.h - Manage Idle popups
 *
 * LICENSE:
 *
 *
 *   Copyright 2010 Avi B.S
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


#ifndef REALTIMEPOPUP_H_
#define REALTIMEPOPUP_H_

typedef BOOL (*StartScrolling)(void);
typedef void (*StopScrolling)(void);
typedef BOOL (*IsScrolling)(void);

#define SCROLLER_DEFAULT 0
#define SCROLLER_NORMAL  1
#define SCROLLER_HIGH    2
#define SCROLLER_HIGHEST 3



typedef struct {
   StartScrolling                   start_scrolling;
   StopScrolling                    stop_scrolling;
   IsScrolling                      is_scrolling;
} RealtimeIdleScroller;


void  RealtimePopUp_Register(RealtimeIdleScroller *scroller, int priority);
void RealtimePopUp_set_Stoped_Popups(void);
void  RealtimePopUp_Init(void);

#endif /* REALTIMEPOPUP_H_ */
