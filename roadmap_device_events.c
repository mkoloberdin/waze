
/* roadmap_device_array.h
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

#include <assert.h>
#include <string.h>
#include "roadmap_cyclic_array.h"
#include "roadmap_device_events.h"
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
// Platform dependant:
extern BOOL  roadmap_device_events_os_init();
extern void  roadmap_device_events_os_term();
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct tag_device_event_info
{
   device_event_handler handler;
   void*                context;

}  device_event_info, *device_event_info_ptr;

void device_event_info_init( device_event_info_ptr this)
{ memset( this, 0, sizeof(device_event_info));}

void device_event_info_free( device_event_info_ptr this)
{ device_event_info_init( this);}

void device_event_info_copy( device_event_info_ptr a, device_event_info_ptr b)
{ (*a) = (*b);}

BOOL device_event_info_are_identical( device_event_info_ptr a, device_event_info_ptr b)
{ return (a->handler == b->handler);}

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#define  DEVICEEVENTS_QUEUE_MAXSIZE       (20)
static   device_event_info    events_queue[DEVICEEVENTS_QUEUE_MAXSIZE];
static   cyclic_array_context EQC;

BOOL  roadmap_device_events_init()
{
   cyclic_array_init(&EQC,                         // Context
                     events_queue,                 // The static array
                     sizeof(device_event_info),    // Size of a single item in the array
                     DEVICEEVENTS_QUEUE_MAXSIZE,   // Elements count
                     "roadmap_device_events",      // Module name (used for logging)
                                                   //    Callbacks:
                     (init_array_item)device_event_info_init,
                     (free_array_item)device_event_info_free,
                     (copy_array_item)device_event_info_copy,
                     (are_same_items) device_event_info_are_identical);
                     
   if( roadmap_device_events_os_init())
      return TRUE;
   
   // Else:   
   cyclic_array_free(&EQC);
   return FALSE;
}

void  roadmap_device_events_term()
{
   cyclic_array_free(&EQC);
   roadmap_device_events_os_term();
}

BOOL roadmap_device_events_register(device_event_handler handler,
                                    void*                context)
{
   device_event_info new_handler;

   new_handler.handler = handler;
   new_handler.context = context;
   
   return cyclic_array_push_last( &EQC, &new_handler);
}

void roadmap_device_events_unregister( device_event_handler handler)
{
   device_event_info handler_to_remove;
   
   handler_to_remove.handler = handler;
   cyclic_array_remove_same_item( &EQC, &handler_to_remove);
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_device_event_notification( device_event event)
{
   int i=0;
   for( i=0; i<cyclic_array_size(&EQC); i++)
   {
      device_event_info_ptr handler = (device_event_info_ptr)cyclic_array_get_item( &EQC, i);
      
      assert(handler);
      handler->handler( event, handler->context);
   }
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
const char* get_device_event_name( device_event event)
{
   switch(event)
   {
      case device_event_hardware_change             : return "hardware_change";
      //se device_event_internet_proxy_changed      : return "internet_proxy_changed";
      case device_event_infrared_server             : return "infrared_server";
      case device_event_network_connected           : return "network_connected";
      case device_event_network_disconnected        : return "network_disconnected";
      case device_event_AC_power_on                 : return "AC_power_on";
      case device_event_AC_power_off                : return "AC_power_off";
      //se device_event_RNDISFN_instantiated        : return "RNDISFN_instantiated";
      case device_event_RS232_connection_established: return "RS232_connection_established";
      case device_event_system_time_change          : return "system_time_change";
      case device_event_system_time_zone_change     : return "system_time_zone_change";
      case device_event_device_woke_up              : return "device_woke_up";
      case device_event_window_orientation_changed  : return "window_orientation_changed";
      case device_event_application_shutdown        : return "application_shutdown";


      default: assert(0); return "<unknown>";
   }
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
