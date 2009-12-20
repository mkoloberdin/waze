/* roadmap_warning.h - The interface for the system warnings functionality
 *
 * LICENSE:
 *
 *   Copyright 2009, Waze Ltd
 *                   Alex Agranovich
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

#ifndef INCLUDE__ROADMAP_WARNING__H
#define INCLUDE__ROADMAP_WARNING__H

#ifdef __cplusplus
extern "C" {
#endif
#include "roadmap.h"


#define ROADMAP_WARNING_MAX_LEN		128

typedef BOOL (*RoadMapWarningFn) ( char* dest_string );

BOOL roadmap_warning_register( RoadMapWarningFn warning_fn, const char* warning_name );
BOOL roadmap_warning_unregister( RoadMapWarningFn warning_fn );

void roadmap_warning_monitor( void );

void roadmap_warning_initialize( void );

void roadmap_warning_shutdown( void );

#ifdef __cplusplus
}
#endif
#endif /* INCLUDE__ROADMAP_WARNING__H */

