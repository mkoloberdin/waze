/* roadmap_main.h - The interface for the RoadMap main window module.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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

#ifndef INCLUDE__ROADMAP_MAIN__H
#define INCLUDE__ROADMAP_MAIN__H

#include "roadmap.h"
#include "roadmap_gui.h"

#include "roadmap_io.h"
#include "roadmap_spawn.h"

#define ROADMAP_CURSOR_NORMAL 1
#define ROADMAP_CURSOR_WAIT   2

struct RoadMapFactoryKeyMap;

typedef void (* RoadMapKeyInput) (char *key);
typedef void (* RoadMapInput)    (RoadMapIO *io);

typedef void *RoadMapMenu;

void roadmap_main_new (const char *title, int width, int height);

void roadmap_main_set_keyboard (struct RoadMapFactoryKeyMap *bindings,
                                RoadMapKeyInput callback);

RoadMapMenu roadmap_main_new_menu (void);
void roadmap_main_free_menu       (RoadMapMenu menu);
void roadmap_main_add_menu        (RoadMapMenu menu, const char *label);
void roadmap_main_add_menu_item   (RoadMapMenu menu,
                                   const char *label,
                                   const char *tip,
                                   RoadMapCallback callback);
void roadmap_main_add_separator   (RoadMapMenu menu);
void roadmap_main_popup_menu      (RoadMapMenu menu, int x, int y);

void roadmap_main_add_tool       (const char *label,
                                  const char *icon,
                                  const char *tip,
                                  RoadMapCallback callback);
void roadmap_main_add_tool_space (void);

void roadmap_main_add_canvas     (void);
void roadmap_main_add_status     (void);

void roadmap_main_show (void);

void roadmap_main_set_input    (RoadMapIO *io, RoadMapInput callback);
void roadmap_main_set_output   (RoadMapIO *io, RoadMapInput callback);
void roadmap_main_remove_input (RoadMapIO *io);

RoadMapIO *roadmap_main_output_timedout(time_t timeout);

void roadmap_main_set_periodic (int interval, RoadMapCallback callback);
void roadmap_main_remove_periodic (RoadMapCallback callback);

void roadmap_main_set_status (const char *text);

void roadmap_main_toggle_full_screen (void);

void roadmap_main_flush (void);

void roadmap_main_exit (void);


void roadmap_main_set_cursor (int cursor);

void roadmap_gui_minimize();
void roadmap_gui_maximize();
void roadmap_main_minimize (void);

#ifdef IPHONE

int roadmap_main_should_mute ();
const char* roadmap_main_get_proxy (const char* url);

int roadmap_main_get_platform ();
int roadmap_main_should_save_nav ();

void roadmap_main_adjust_skin (int state);

#define ROADMAP_MAIN_PLATFORM_NA       0
#define ROADMAP_MAIN_PLATFORM_IPOD     1
#define ROADMAP_MAIN_PLATFORM_IPHONE   2
#define ROADMAP_MAIN_PLATFORM_IPHONE3G 3

#endif //IPHONE

#endif /* INCLUDE__ROADMAP_MAIN__H */

