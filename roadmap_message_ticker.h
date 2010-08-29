/* roadmap_message_ticker.h
 *
 * LICENSE:
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
#ifndef ROAADMAP_MESSAGE_TICKER_H_
#define ROAADMAP_MESSAGE_TICKER_H_

int roadmap_message_ticker_height(void);
void roadmap_message_ticker_show(const char *title, const char* text, const char* icon, int timer);
void roadmap_message_ticker_show_cb(const char *title, const char* text, const char* icon, int timer, RoadMapCallback callback);
void roadmap_message_ticker_initialize(void);
BOOL roadmap_message_ticker_is_on(void);
#endif /* ROAADMAP_MESSAGE_TICKER_H_ */
