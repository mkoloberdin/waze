/* roadmap_power.h - The interface for the client power management functionality
 *
 * LICENSE:
 *
 *   Copyright 2008 Alex Agranovich
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



// Configuration entries
#define CFG_CATEGORY_BATTERY         	( "Battery" )
#define CFG_ENTRY_BATTERY_CHK_PERIOD    ( "CheckPeriod" )
#define CFG_ENTRY_BATTERY_WARN_THR_1    ( "Threshold 1" )
#define CFG_ENTRY_BATTERY_WARN_THR_2    ( "Threshold 2" )

#define CFG_BATTERY_WARN_THR_1_DEFAULT		("30")		// The threshold in percents for the first warning
#define CFG_BATTERY_WARN_THR_2_DEFAULT		("20")		// The threshold in percents for the first warning

#define CFG_BATTERY_CHK_PERIOD_DEFAULT	("30000")	// The battery level check period (in milliseconds ). This value used as
													// a default, if there is no configuration value in preferences

#define BATTERY_WARN_STATE_NORMAL_THR		(60)	// The value (in percents) indicating when the engine will
													// start checking the battery level again, after warning


#define RESP_WAIT_PERIOD_WARN_1			(10*1000)	// The amount of time to wait for response for the warning 1
#define RESP_WAIT_PERIOD_WARN_2			(5*60*1000)	// The amount of time to wait for response for the warning 2


void roadmap_power_initialize( void );
