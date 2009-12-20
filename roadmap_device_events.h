
/* roadmap_device_events.h
 *
 * LICENSE:
 *
 *   Copyright 2008 PazO
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
 */

#ifndef	__ROADMAP_DEVICE_EVENTS_H__
#define	__ROADMAP_DEVICE_EVENTS_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
#include "roadmap.h"
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
// Types of events:
typedef enum tag_device_event
{
   device_event_hardware_change,                // A PC Card device changed. 
   //vice_event_internet_proxy_changed,         // The Internet Proxy used by the device has changed. 
   device_event_infrared_server,                // The device discovered a server by using infrared communications. 
   device_event_network_connected,              // The device connected to a network. 
   device_event_network_disconnected,           // The device disconnected from a network. 
   device_event_AC_power_on,                    // The user turned the AC power on. 
   device_event_AC_power_off,                   // The user turned the alternating current (AC) power off. 
   //vice_event_RNDISFN_instantiated,           // RNDISFN interface is instantiated. 
   device_event_RS232_connection_established,   // An RS232 connection was made. 
   device_event_system_time_change,             // The system time changed. 
   device_event_system_time_zone_change,        // The time zone changed. 
   device_event_device_woke_up,                 // The device woke up. 
   device_event_window_orientation_changed,     // Window orientation changed
   device_event_application_shutdown,           // Application is shutting down
   
   // These are last:
   device_event__count,
   device_event__invalid

}  device_event;

const char* get_device_event_name( device_event event);

// Callback for event:
typedef void(*device_event_handler)( device_event event, void* context);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void  roadmap_device_event_notification( device_event event);

BOOL  roadmap_device_events_init();
void  roadmap_device_events_term();

// Start / Stop listenning for events:
BOOL  roadmap_device_events_register   (  device_event_handler handler,
                                          void*                context);
void  roadmap_device_events_unregister (  device_event_handler handler);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif	//	__ROADMAP_DEVICE_EVENTS_H__
