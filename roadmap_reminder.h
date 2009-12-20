/* roadmap_reminder.h
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi Ben-Shoshan
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


#ifndef ROADMAP_REMINDER_H_
#define ROADMAP_REMINDER_H_

typedef enum tag_reminder_history_item
{
   reminder_hi_house_number,
   reminder_hi_street,
   reminder_hi_city,
   reminder_state,
   reminder_hi_title,
   reminder_hi_latitude,
   reminder_hi_longtitude,
   reminder_hi_add_reminder,
   reminder_hi_distance,
   reminder_hi_description,
   reminder_hi_repeat,
   
   reminder_hi__count,
   reminder_hi__invalid

}  reminder_history_item;


BOOL roadmap_reminder_feature_enabled (void); 
void roadmap_reminder_init(void);
void roadmap_reminder_term(void);
void roadmap_reminder_add(void);
void roadmap_reminder_save_location(void);
void roadmap_reminder_delete(void *history);
void roadmap_reminder_add_at_position(RoadMapPosition *position,  BOOL isReminder, BOOL default_reminder);
void roadmap_reminder_add_entry(const char **argv, BOOL add_reminder);
#endif /* ROADMAP_REMINDER_H_ */
