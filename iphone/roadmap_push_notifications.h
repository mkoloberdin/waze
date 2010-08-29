/* roadmap_push_notifications.h - iPhone push notifications
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi R.
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

#ifndef __ROADMAP_PUSH_NOTIFICATIONS__H
#define __ROADMAP_PUSH_NOTIFICATIONS__H

void roadmap_push_notifications_settings(void);

const char* roadmap_push_notifications_token(void);
void roadmap_push_notifications_set_token(const char* token);
BOOL roadmap_push_notifications_score(void);
BOOL roadmap_push_notifications_updates(void);
BOOL roadmap_push_notifications_friends(void);

BOOL roadmap_push_notifications_pending(void);
void roadmap_push_notifications_set_pending(BOOL value);


#endif // __ROADMAP_PUSH_NOTIFICATIONS__H