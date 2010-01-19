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


#include <string.h>
#include "roadmap_config.h"
#include "roadmap_device.h"
#include "JNI/FreeMapJNI.h"
#include "auto_hide_dlg.h"
#include "roadmap_gps.h"
#include "roadmap_math.h"


/* TODO:: Add these two params to the configuration *** AGA *** */
#define RM_BACKLIGHT_AUTO_OFF_TIMEOUT		300	/* Time in seconds. Staying this time in the radius of
												 * RM_BACKLIGHT_AUTO_OFF_MOVEMENT meters indicates
												 * "Staying in one position" state
												 */
#define RM_BACKLIGHT_AUTO_OFF_MOVEMENT		10	/* Distance in meters of the movement threshold.
												 * Staying in this radius for RM_BACKLIGHT_AUTO_OFF_TIMEOUT seconds
												 * indicates "Staying in one position" state
												 */

#define RM_BACKLIGHT_ON 					1
#define RM_BACKLIGHT_OFF 					0

RoadMapConfigDescriptor RoadMapConfigBackLight =
                        ROADMAP_CONFIG_ITEM("Display", "BackLight");

static time_t gsLastMovementTime = 0;										/* Time of the last detected movement */
static RoadMapGpsPosition gsLastMovementPos = ROADMAP_GPS_NULL_POSITION;	/* Position of the last detected movement */
static int gsBacklightOnActive = 0;

void roadmap_device_backlight_monitor_reset( void );
static void roadmap_device_backlight_monitor( time_t gps_time, const RoadMapGpsPrecision *dilution, const RoadMapGpsPosition *position);
static void roadmap_device_set_backlight_( int value );

/***********************************************************
 *	Name 		: roadmap_device_initialize
 *	Purpose 	: Loads the backlight parameter from the configuration
 * 					and updates the application. Returns the loaded value
 */
int roadmap_device_initialize( void )
{
	// Load the configuration
    roadmap_config_declare
       ("user", &RoadMapConfigBackLight, "no", NULL);

	// Update the UI object
    roadmap_device_backlight_monitor_reset();

	// Register as gps listener
	roadmap_gps_register_listener( roadmap_device_backlight_monitor );

	// Log the operation
	roadmap_log( ROADMAP_DEBUG, "roadmap_backlight_initialize() - Current setting : %s",
													roadmap_config_get( &RoadMapConfigBackLight ) );

	return 1;
}


/***********************************************************
 *	Name 		: roadmap_device_set_backlight
 *	Purpose 	: Sets the backlight of the display to be always on
 * 					if the parameter is zero the device system defaults are used
 */
void roadmap_device_set_backlight( int alwaysOn )
{
	const char * alwaysOnStr = alwaysOn ? "yes" : "no";

	// Update the UI object
	roadmap_device_set_backlight_( alwaysOn );

	// Update the configuration
	roadmap_config_set( &RoadMapConfigBackLight, alwaysOnStr );

	// Log the operation
	roadmap_log( ROADMAP_DEBUG, "roadmap_set_backlight() - Current setting : %s", alwaysOnStr );

	return;
}


/***********************************************************
 *	Name 		: roadmap_device_set_backlight_
 *	Purpose 	: Sets the current backlight value in the android layer
 *
 *
 */
static void roadmap_device_set_backlight_( int value )
{
	gsBacklightOnActive = value;
	FreeMapNativeManager_SetBackLightOn( gsBacklightOnActive );
}


/***********************************************************
 *	Name 		: roadmap_device_backlight_monitor_reset
 *	Purpose 	: Resets the backlight monitor
 *
 *
 */
void roadmap_device_backlight_monitor_reset( void )
{
	roadmap_device_set_backlight_( RM_BACKLIGHT_ON );
	gsLastMovementTime = 0;
}

/***********************************************************
 *	Name 		: roadmap_device_backlight_monitor
 *	Purpose 	: Monitors the movement of the device according to gps positions. If staying in the same point and
 *					backlight is off - use the device settings. Otherwise keep the screen bright
 *
 */
static void roadmap_device_backlight_monitor( time_t gps_time, const RoadMapGpsPrecision *dilution, const RoadMapGpsPosition *position)
{
	RoadMapPosition last_pos;
	RoadMapPosition cur_pos;
	int cur_distance;

	/*
	 * If ALWAYS ON - Nothing to do
	 */
	if ( roadmap_config_match( &RoadMapConfigBackLight, "yes" ) )
	{
		gsLastMovementTime = 0;
		return;
	}

	/* First point */
	if ( gsLastMovementTime == 0 )
	{
		gsLastMovementPos = *position;
		gsLastMovementTime = gps_time;
		return;
	}

	memcpy( &last_pos, &gsLastMovementPos, sizeof( RoadMapPosition ) );
	memcpy( &cur_pos, position, sizeof( RoadMapPosition ) );
	cur_distance = roadmap_math_distance( &last_pos, &cur_pos );

	if ( gsBacklightOnActive ) /* Currently set on - check if still moving */
	{
		if ( cur_distance < RM_BACKLIGHT_AUTO_OFF_MOVEMENT &&
				( ( gps_time - gsLastMovementTime ) > RM_BACKLIGHT_AUTO_OFF_TIMEOUT ) )
		{
			/* Switch the state to off (system settings) */
			roadmap_device_set_backlight_( RM_BACKLIGHT_OFF );
		}
	}
	else	/* Currently backlight is off - check if started to move */
	{
		if ( cur_distance > RM_BACKLIGHT_AUTO_OFF_MOVEMENT )
		{
			/* Switch the state to on */
			roadmap_device_set_backlight_( RM_BACKLIGHT_ON );
		}
	}

	/* Set the last movement point and time */
	if ( cur_distance > RM_BACKLIGHT_AUTO_OFF_MOVEMENT )
	{
		gsLastMovementPos = *position;
		gsLastMovementTime = gps_time;
	}
	return;
}



/***********************************************************
 *	Name 		: roadmap_device_get_battery_level
 *	Purpose 	: Returns the battery level ( in percents )
 *
 */
int roadmap_device_get_battery_level( void )
{

	int batteryCurrentLevel = FreeMapNativeManager_getBatteryLevel();

	return batteryCurrentLevel;
}


/***********************************************************/
/*  Name        : on_minimize_dialog_close
 *  Purpose     : Called when the "auto hide" dialog closing
 *              :
 *              :
 */
static void on_minimize_dialog_close( int exit_code, void* context )
{
    // Show the dialer
    FreeMapNativeManager_ShowDilerWindow();
}


/***********************************************************/
/*  Name        : roadmap_device_call_start_callback
 *  Purpose     : Starts the start call procedure with the automatic minimizing and
 *              : and maximizing of the application
 *              :
 */
void roadmap_device_call_start_callback( void )
{
    // Auto minimization
    auto_hide_dlg(on_minimize_dialog_close );
}
