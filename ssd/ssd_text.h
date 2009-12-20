/* ssd_text.h - Static text widget
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
 */

#ifndef __SSD_WIDGET_TEXT_H_
#define __SSD_WIDGET_TEXT_H_

#include "ssd_widget.h"

#define	SSD_TEXT_MAXIMUM_TEXT_LENGTH			(255)

SsdWidget	ssd_text_new(	const char*	name,
									const char*	value,
									int			size,
									int			flags);

void			ssd_text_set_input_type	( SsdWidget this, roadmap_input_type input_type);
void			ssd_text_reset_text		( SsdWidget this);
const char*	ssd_text_get_text			( SsdWidget this);
void			ssd_text_set_text			( SsdWidget this, const char* new_value);
void        ssd_text_set_text_size  ( SsdWidget this, int size);
void        ssd_text_set_font_size  ( SsdWidget this, int size);
int         ssd_text_get_char_height( int size);
int         ssd_text_get_char_width ();
void			ssd_text_set_readonly   ( SsdWidget this, BOOL read_only);
void        ssd_text_set_auto_size  ( SsdWidget this);
void        ssd_text_set_auto_trim  ( SsdWidget this);
void        ssd_text_set_color      ( SsdWidget this, const char* color);
void ssd_text_set_lines_space_padding( SsdWidget this, int space );
#endif // __SSD_WIDGET_TEXT_H_
