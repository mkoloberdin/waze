/* roadmap_sprite.h - manage the roadmap sprites that represent moving objects.
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
 *   This module draws predefined graphical objects on the RoadMap canvas.
 */

#ifndef INCLUDE__ROADMAP_SPRITE__H
#define INCLUDE__ROADMAP_SPRITE__H

#include "roadmap_gui.h"

#define SPRITE_FAST 0x1
#define SPRITE_NO_ROTATE 0x2

void roadmap_sprite_initialize (void);

void roadmap_sprite_draw
        (const char *name, RoadMapGuiPoint *location, int orientation);

#endif // INCLUDE__ROADMAP_CANVAS__H

