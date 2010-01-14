/* roadmap_device.c - Device specific funcionality for the roadmap application
 *
 * LICENSE:
 *
 *   Copyright 2008 Alex Agranovich
 *   Copyright 2008 Ehud Shabtai
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
 * SYNOPSYS:
 *
 *   See roadmap_device.h
 */

#include "../roadmap_device.h"

/***********************************************************/
/*	Name 		: roadmap_device_initialize
/*	Purpose 	: Loads the backlight parameter from the configuration
 * 					and updates the application. Returns the loaded value
 */
int roadmap_device_initialize( void )
{
	return 0;
}

/***********************************************************/
/*	Name 		: roadmap_device_set_backlight
/*	Purpose 	: Sets the backlight of the display to be always on
 * 					if the parameter is zero the device system defaults are used
 */
void roadmap_device_set_backlight( int alwaysOn )
{
}

/***********************************************************/
/*  Name        : roadmap_device_call_start_callback
/*  Purpose     : Starts the start call procedure with the automatic minimizing and
 *              : and maximizing of the application
 *              :
 *
 */
void roadmap_device_call_start_callback( void )
{
}
