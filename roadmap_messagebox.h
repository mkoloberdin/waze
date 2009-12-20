/* roadmap_messagebox.h - Display a small message window for RoadMap.
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

#ifndef INCLUDE__ROADMAP_MESSAGEBOX__H
#define INCLUDE__ROADMAP_MESSAGEBOX__H

#define  SSD_MESSAGEBOX_DEFBUTTON_YES           (0x01)
#define  SSD_MESSAGEBOX_DEFBUTTON_NO            (0x02)

typedef void(*messagebox_closed)( int exit_code );

void roadmap_messagebox (const char *title, const char *message);

void roadmap_messagebox_custom( const char *title, const char *text,
		int title_font_size, char* title_color, int text_font_size, char* text_color );

void roadmap_messagebox_cb(const char *title, const char *message,
         messagebox_closed on_messagebox_closed);
void roadmap_messagebox_timeout (const char *title, const char *text, int seconds);

#endif // INCLUDE__ROADMAP_MESSAGEBOX__H
