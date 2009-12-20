/* roadmap_metadata.h - Interface to the map metadata information.
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
 */

#ifndef INCLUDED__ROADMAP_METADATA__H
#define INCLUDED__ROADMAP_METADATA__H

#include "roadmap_types.h"
#include "roadmap_dbread.h"


const char *roadmap_metadata_get_attribute (const char *category,
                                            const char *name);

extern roadmap_db_handler RoadMapMetadataHandler;

#endif // INCLUDED__ROADMAP_METADATA__H

