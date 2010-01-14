/* roadmap_warning.c - System warnings management
 *
 *
 * LICENSE:
 *   Copyright 2009, Waze Ltd
 *   Alex Agranovich
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
 *   See roadmap_warning.h
 */


#include "roadmap_warning.h"
#include "roadmap_screen.h"
#include "roadmap_main.h"
#include "roadmap_message.h"
#include "roadmap_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

///// Defines ///////
#define MAX_WARNINGS_NUM 	5
#define MAX_WARN_ENTRY_NAME_LEN 	32

#define RM_WARNING_MONITOR_PERIOD_DFLT   2000 /* Milli sec */

typedef struct
{
	BOOL is_active;
	BOOL last_status;
	RoadMapWarningFn warning_fn;
	char entry_name[MAX_WARN_ENTRY_NAME_LEN];
} WarningEntry;


///// Globals ///////
static WarningEntry  gsSysWarnings[MAX_WARNINGS_NUM];
static int gsCurrentWarning = 0;

////// Configuration section ///////


///// Local interface ///////
static RoadMapConfigDescriptor RoadMapConfigWarningMonitorPeriod =
                  ROADMAP_CONFIG_ITEM("Warning", "TimeOut");


/***********************************************************/
/*  Name        : roadmap_warning_register()
 *  Purpose     : Registers the warning function for the system warning indication and string formatting
 *
 *  Params		: [in] warning_fn - the function returning if the warning has to be displayed and the target string
 *				:
 *  Returns 	: TRUE - successfully registered
 */
BOOL roadmap_warning_register( RoadMapWarningFn warning_fn, const char* warning_name )
{
	int i;
	BOOL res = FALSE;
	for( i = 0; i < MAX_WARNINGS_NUM; ++i )
	{
		if ( gsSysWarnings[i].is_active == FALSE )
		{
			gsSysWarnings[i].is_active = TRUE;
			gsSysWarnings[i].warning_fn = warning_fn;
			if ( warning_name != NULL )
				strncpy( gsSysWarnings[i].entry_name, warning_name, MAX_WARN_ENTRY_NAME_LEN );
			res = TRUE;
			break;
		}
	}
	return res;
}

/***********************************************************/
/*  Name        : roadmap_warning_monitor
 *  Purpose     : Updates the system message structure with  the next warning
 *
 *  Params		:
 *				:
 *  Returns 	:
 */
void roadmap_warning_monitor(void)
{
	int next_idx, i, warn_status;
	char warning_str[ROADMAP_WARNING_MAX_LEN] = {0};
	BOOL refresh_needed = FALSE;
	for( i = 0; i < MAX_WARNINGS_NUM; ++i )
	{
		next_idx = ( gsCurrentWarning + i + 1 ) % MAX_WARNINGS_NUM;
		if ( gsSysWarnings[next_idx].is_active )
		{
			// If warning has to be shown and was not shown before (or  vice versa) - refresh is necessary
			warn_status = gsSysWarnings[next_idx].warning_fn( warning_str );
			refresh_needed = ( gsSysWarnings[next_idx].last_status != warn_status );
			gsSysWarnings[next_idx].last_status = warn_status;
			if ( warn_status )
			{
				roadmap_message_set( 'w', warning_str );
				// If another warning has to be shown - refresh is necessary
				refresh_needed = refresh_needed || ( gsCurrentWarning != next_idx  );
				gsCurrentWarning = next_idx;
				break;
			}
			else
			{
				roadmap_message_unset( 'w' );
			}
		}
	}
	if ( refresh_needed && !roadmap_screen_refresh() )
		roadmap_screen_redraw();
}

/***********************************************************/
/*  Name        : roadmap_warning_unregister()
 *  Purpose     : Removes the warning from the monitoring
 *
 *  Params		: [in] warning_fn - the function returning if the warning has to be displayed and the target string
 *				:
 *  Returns 	: TRUE - successfully unregistered
 */
BOOL roadmap_warning_unregister( RoadMapWarningFn warning_fn )
{
	int i;
	BOOL res = FALSE;
	for( i = 0; i < MAX_WARNINGS_NUM; ++i )
	{
		if ( gsSysWarnings[i].is_active == TRUE && gsSysWarnings[i].warning_fn == warning_fn )
		{
			gsSysWarnings[i].is_active = FALSE;
			res = TRUE;
			break;
		}
	}
	return res;
}


/***********************************************************/
/*  Name        : roadmap_warning_initialize()
 *  Purpose     : Initialization of the warnings engine
 *
 *  Params		:
 *				:
 *  Returns 	:
 */
void roadmap_warning_initialize()
{
   int i, period;

   roadmap_config_declare
	   ("preferences", &RoadMapConfigWarningMonitorPeriod, OBJ2STR(RM_WARNING_MONITOR_PERIOD_DFLT), NULL );

	for( i = 0; i < MAX_WARNINGS_NUM; ++i )
	{
		gsSysWarnings[i].is_active = FALSE;
		gsSysWarnings[i].last_status = FALSE;
		gsSysWarnings[i].warning_fn = NULL;
		gsSysWarnings[i].entry_name[0] = 0;
	}

	/* Starting the monitor */
	period = roadmap_config_get_integer( &RoadMapConfigWarningMonitorPeriod );
	roadmap_main_set_periodic( period, roadmap_warning_monitor );
}

/***********************************************************/
/*  Name        : roadmap_warning_shutdown()
 *  Purpose     : Stops the warnings monitoring
 *
 *  Params		:
 *				:
 *  Returns 	:
 */
void roadmap_warning_shutdown()
{
	roadmap_main_remove_periodic( roadmap_warning_monitor );
}
