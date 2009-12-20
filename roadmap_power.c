/* roadmap_power.c - Power management modle for the roadmapapplication
 *
 * LICENSE:
 *
 *   Copyright 2009, Alex Agranovich
 *
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
 *   See roadmap_power.h, roadmap_device.h
 */

#include <string.h> 
#include "roadmap_config.h"
#include "roadmap_main.h"
#include "roadmap_lang.h"
#include "roadmap_device.h"
#include "roadmap_power.h"
#include "ssd/ssd_confirm_dialog.h"
#include "ssd/ssd_dialog.h"

static RoadMapConfigDescriptor RMCfgBatteryChkPeriod =
                        ROADMAP_CONFIG_ITEM( CFG_CATEGORY_BATTERY, CFG_ENTRY_BATTERY_CHK_PERIOD );
static RoadMapConfigDescriptor RMCfgBatteryWarnThr_1 =
                        ROADMAP_CONFIG_ITEM( CFG_CATEGORY_BATTERY, CFG_ENTRY_BATTERY_WARN_THR_1 );
static RoadMapConfigDescriptor RMCfgBatteryWarnThr_2 =
                        ROADMAP_CONFIG_ITEM( CFG_CATEGORY_BATTERY, CFG_ENTRY_BATTERY_WARN_THR_2 );

static int gBatteryWarnThr_1;
static int gBatteryWarnThr_2;

static void roadmap_power_monitor( void );
static void roadmap_power_confirm_warn_1( int aResCode, void * aContext );
static void roadmap_power_confirm_warn_2( int aResCode, void * aContext );
//static void roadmap_power_response_waiter_warn_1( void );
static void roadmap_power_response_waiter_warn_2( void );
static void roadmap_power_state_handler_warn_1( void );
static void roadmap_power_state_handler_warn_2( void );

typedef enum
{
	pwrmon_normal,
	pwrmon_below_threshold_1,
	pwrmon_below_threshold_2,
} PowerMonState;

static BOOL gConfirmDialogShown = FALSE;
static PowerMonState gPowerMonState = pwrmon_normal;

/***********************************************************/
/*	Name 		: roadmap_power_initialize
 *	Purpose 	: Initializes the power manager
 *
 */
void roadmap_power_initialize( void )
{
	int batteryCheckPeriod;
	// Load the configuration
    roadmap_config_declare( "preferences", &RMCfgBatteryWarnThr_1, CFG_BATTERY_WARN_THR_1_DEFAULT, NULL );

    roadmap_config_declare( "preferences", &RMCfgBatteryWarnThr_2, CFG_BATTERY_WARN_THR_2_DEFAULT, NULL );

	roadmap_config_declare( "preferences", &RMCfgBatteryChkPeriod, CFG_BATTERY_CHK_PERIOD_DEFAULT, NULL);

    // Get the config values
    gBatteryWarnThr_1 = roadmap_config_get_integer( &RMCfgBatteryWarnThr_1 );
    gBatteryWarnThr_2 = roadmap_config_get_integer( &RMCfgBatteryWarnThr_2 );
    batteryCheckPeriod = roadmap_config_get_integer( &RMCfgBatteryChkPeriod );

    // Start the timer if the argument is OK
    if ( batteryCheckPeriod <= 0 )
    {
        roadmap_log( ROADMAP_WARNING, "Failed to start the periodic for power due to illegal period. Period: %d. Threshold 1: %d. Threshold 2: %d", batteryCheckPeriod, gBatteryWarnThr_1, gBatteryWarnThr_2 );
    }
    else
    {
        roadmap_main_set_periodic( batteryCheckPeriod, roadmap_power_monitor );
        roadmap_log( ROADMAP_DEBUG, "Starting the periodic for power. Period: %d.", batteryCheckPeriod );
    }

	return;
}

/***********************************************************
 *	Name 		: roadmap_power_monitor
 *	Purpose 	: Checks if the battery low - if low ask if exit
 *
 */
static void roadmap_power_monitor( void )
{

	int batteryCurrentLevel = roadmap_device_get_battery_level();

	roadmap_log( ROADMAP_INFO, "Current battery level : %d. Threshold 1: %d. Threshold 2: %d", batteryCurrentLevel, gBatteryWarnThr_1, gBatteryWarnThr_2 );

	if ( batteryCurrentLevel < 0 )
		return;

	if ( gConfirmDialogShown )
		return;




	// State machine for the power monitor
	switch ( gPowerMonState )
	{
		case pwrmon_normal:	// In normal state check the thresholds
		{
			if ( batteryCurrentLevel < gBatteryWarnThr_2 )
			{
				roadmap_power_state_handler_warn_2();
				gPowerMonState = pwrmon_below_threshold_2;
				break;
			}
			if ( batteryCurrentLevel < gBatteryWarnThr_1 )
			{
				roadmap_power_state_handler_warn_1();
				gPowerMonState = pwrmon_below_threshold_1;
				break;
			}
		}
		case pwrmon_below_threshold_1:
		{
			if ( batteryCurrentLevel > BATTERY_WARN_STATE_NORMAL_THR )
			{
				gPowerMonState = pwrmon_normal;
			}

			if ( batteryCurrentLevel < gBatteryWarnThr_2 )
			{
				roadmap_power_state_handler_warn_2();
				gPowerMonState = pwrmon_below_threshold_2;
			}

			break;
		}
		case pwrmon_below_threshold_2:
		{
			if ( batteryCurrentLevel > BATTERY_WARN_STATE_NORMAL_THR )
			{
				gPowerMonState = pwrmon_normal;
			}
			break;
		}
	}
}

/***********************************************************
 *	Name 		: roadmap_power_confirm_warn_1
 *	Purpose 	: Result of confirmation dialog query for the warning 1
 *
 */
static void roadmap_power_confirm_warn_1( int aResCode, void * aContext )
{
	gConfirmDialogShown = FALSE;
	// roadmap_main_remove_periodic( roadmap_power_response_waiter_warn_1 );
	if ( aResCode != dec_yes )
	{
		return;
	}
	else
	{
		// Remove the periodic
		roadmap_main_remove_periodic( roadmap_power_monitor );
		// Exit the application
		roadmap_main_exit();
	}
}

/***********************************************************
 *	Name 		: roadmap_power_confirm_warn_2
 *	Purpose 	: Result of confirmation dialog query for the warning 2
 *
 */
static void roadmap_power_confirm_warn_2( int aResCode, void * aContext )
{
	roadmap_main_remove_periodic( roadmap_power_response_waiter_warn_2 );
	gConfirmDialogShown = FALSE;
	if ( aResCode != dec_yes )
	{
		return;
	}
	else
	{
		// Remove the periodic
		roadmap_main_remove_periodic( roadmap_power_monitor );
		// Exit the application
		roadmap_main_exit();
	}
}


/***********************************************************
 *	Name 		: roadmap_power_response_waiter_warn_1
 *	Purpose 	: Timer waiting for the dialog of the first warning
 *
 */
static void roadmap_power_response_waiter_warn_1( void )
{
	// If we are here - there is no response for the
	roadmap_log( ROADMAP_DEBUG, "Warning 1 has no response - removing the dialog" );

	roadmap_main_remove_periodic( roadmap_power_response_waiter_warn_1 );

	ssd_confirm_dialog_close();

	gConfirmDialogShown = FALSE;
}

/***********************************************************
 *	Name 		: roadmap_power_response_waiter_warn_2
 *	Purpose 	: Timer waiting for the dialog of the second warning
 *
 */
static void roadmap_power_response_waiter_warn_2( void )
{
	// If we are here - there is no response for the
	roadmap_log( ROADMAP_DEBUG, "Warning 2 has no response - shutting down the application" );

	roadmap_main_remove_periodic( roadmap_power_response_waiter_warn_2 );

	ssd_confirm_dialog_close();

	gConfirmDialogShown = FALSE;

	roadmap_main_remove_periodic( roadmap_power_monitor );

	roadmap_main_exit();

}

/***********************************************************
 *	Name 		: roadmap_power_handler_warn_1
 *	Purpose 	: State handler for warning 1
 *
 */
static void roadmap_power_state_handler_warn_1( void )
{
	char fmtText[256];
	// Set the timer for the no-response handling
	// roadmap_main_set_periodic( RESP_WAIT_PERIOD_WARN_1, roadmap_power_response_waiter_warn_1 );
	// Show the dialog and handle the input
	snprintf( fmtText, 256, roadmap_lang_get( "Less than %d%% battery remaining. Would you like to exit waze?" ), gBatteryWarnThr_1 );

	ssd_confirm_dialog_timeout( "", fmtText, FALSE, roadmap_power_confirm_warn_1, NULL, RESP_WAIT_PERIOD_WARN_1/1000 );
	gConfirmDialogShown = TRUE;
}

/***********************************************************
 *	Name 		: roadmap_power_handler_warn_2
 *	Purpose 	: State handler for warning 2
 *
 */
static void roadmap_power_state_handler_warn_2( void )
{
	char fmtText[256];
	// Set the timer for the no-response handling
	roadmap_main_set_periodic( RESP_WAIT_PERIOD_WARN_2, roadmap_power_response_waiter_warn_2 );
	// Show the dialog and handle the input
	snprintf( fmtText, 256, roadmap_lang_get( "Less than %d%% battery remaining. Would you like to exit waze?" ), gBatteryWarnThr_2 );

	ssd_confirm_dialog ( "", fmtText, TRUE, roadmap_power_confirm_warn_2, NULL);
	gConfirmDialogShown = TRUE;
}
