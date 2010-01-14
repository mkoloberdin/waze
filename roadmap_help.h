/* roadmap_help.h - Manage access to some help.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 * DESCRIPTION:
 *
 *   This module provides a simple interface for launching a help tool.
 *
 *   The basic concept is that the help tool supports a short list of topics
 *   represented each by an index string and a label. The typical help tool
 *   is a web browser.
 *
 *   The functions roadmap_help_topic_first and roadmap_help_topic_next
 *   help build the user interface (for example a list of menu entries).
 */

#ifndef INCLUDE__ROADMAP_HELP__H
#define INCLUDE__ROADMAP_HELP__H

#include "roadmap.h"

int roadmap_help_first_topic (const char **label, RoadMapCallback *callback);
int roadmap_help_next_topic  (const char **label, RoadMapCallback *callback);

void roadmap_help_initialize (void);
void roadmap_help_menu(void);
void roadmap_open_help(void);

#endif // INCLUDE__ROADMAP_HELP__H

