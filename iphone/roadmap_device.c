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


#include "roadmap_main.h"
#include "roadmap_config.h"
#include "roadmap_device.h"


static RoadMapConfigDescriptor RoadMapConfigBackLight =
      ROADMAP_CONFIG_ITEM("Display", "BackLight");



/***********************************************************
 * Name   : roadmap_device_initialize
 * Purpose: Loads the backlight parameter from the configuration
 *          and updates the application. Returns the loaded value
 */
static void roadmap_device_periodic( void )
{
   roadmap_main_refresh_backlight();
}

/***********************************************************
 * Name   : roadmap_device_initialize
 * Purpose: Loads the backlight parameter from the configuration
 *          and updates the application. Returns the loaded value
 */
int roadmap_device_initialize( void )
{
   int isAlwaysOn;
   
	// Load the configuration
   roadmap_config_declare
   ("user", &RoadMapConfigBackLight, "yes", NULL);
   isAlwaysOn = roadmap_config_match( &RoadMapConfigBackLight, "yes" );
   
	// Update the UI object
	roadmap_main_set_backlight( isAlwaysOn );
   
	// Log the operation
	roadmap_log( ROADMAP_DEBUG, "roadmap_backlight_initialize() - Current setting : %s",
               roadmap_config_get( &RoadMapConfigBackLight ) );
   
   roadmap_main_set_periodic(10*1000, roadmap_device_periodic);
   
	return isAlwaysOn;
   
}

/***********************************************************
 * Name   : roadmap_device_set_backlight
 * Purpose: Sets the backlight of the display to be always on
 *          if the parameter is zero the device system defaults are used
 */
void roadmap_device_set_backlight( int alwaysOn )
{
   const char * alwaysOnStr = alwaysOn ? "yes" : "no";
   
	// Update the UI object
	roadmap_main_set_backlight( alwaysOn );
   
	// Update the configuration
	roadmap_config_set( &RoadMapConfigBackLight, alwaysOnStr );
   
	// Log the operation
	roadmap_log( ROADMAP_DEBUG, "roadmap_set_backlight() - Current setting : %s", alwaysOnStr );
   
	return;
   
}

/***********************************************************/
/*	Name 		: roadmap_device_get_battery_level
/*	Purpose 	: Returns the battery level ( in percents )
 *
 */
int roadmap_device_get_battery_level( void )
{
	return -1;
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

#include "roadmap_camera.h"


/***********************************************************
 *  Name        : roadmap_camera_take_picture
 *  Purpose     : Shows the camera preview, passes target image attributes
 *                 and defines the target capture file location
 *
 */
BOOL roadmap_camera_take_picture( CameraImageFile* image_file, CameraImageBuf* image_thumbnail )
{
	    return ( -1 );
}

