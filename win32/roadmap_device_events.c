/*
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
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


#include <windows.h>
#include <assert.h>

#include "..\roadmap_device_events.h"

#ifndef  UNDER_CE
BOOL roadmap_device_events_os_init()
{ return TRUE;}

void roadmap_device_events_os_term()
{}

#else

#include <notify.h>
static   HANDLE   MonitorThread  = NULL;
static   HANDLE   eventPleaseQuit= NULL;

extern   HANDLE   eventShuttingDown;
extern   HWND     RoadMapMainWindow;

#define  FIRST_CORE_DEVICE_EVENT_FOR_WINDOWS    (device_event_hardware_change)
#define  LAST_CORE_DEVICE_EVENT_FOR_WINDOWS     (device_event_device_woke_up)
#define  CORE_DEVICE_EVENTS_COUNT               (LAST_CORE_DEVICE_EVENT_FOR_WINDOWS+1)

#if(FIRST_CORE_DEVICE_EVENT_FOR_WINDOWS != 0)
#error In this implementation 'device_event_hardware_change' must be first...
#endif   // device_event_hardware_change is 0

DWORD WINAPI DeviceEventsMonitorThread( void* params);

DWORD get_win32_device_event_id( device_event event);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL roadmap_device_events_os_init()
{
   DWORD dwThreadID;

   if( MonitorThread || eventPleaseQuit)
   {
      assert(0);
      return TRUE;
   }
   
   eventPleaseQuit = CreateEvent( NULL, FALSE /* Manual? */, FALSE, NULL);
   if( !eventPleaseQuit)
   {
      roadmap_log( ROADMAP_ERROR, "roadmap_device_events_os_init() - Failed to create event");
      return FALSE;
   }
   
   MonitorThread = CreateThread( NULL, // SD,
                                 0,    // Stack size
                                 DeviceEventsMonitorThread,
                                       // Worker thread
                                 NULL, // Params
                                 0,    // Flags
                                 &dwThreadID);

   if( !MonitorThread)
   {
      roadmap_log( ROADMAP_ERROR, "roadmap_device_events_os_init() - Failed to create thread");
      
      CloseHandle( eventPleaseQuit);
      eventPleaseQuit = NULL;
      
      return FALSE;
   }
   
   return TRUE;
}

void  roadmap_device_events_os_term()
{
   if( MonitorThread)
   {
      if( eventPleaseQuit)
         SetEvent( eventPleaseQuit);
      
      if( WAIT_OBJECT_0 != WaitForSingleObject( MonitorThread, 3000))
      {
         roadmap_log( ROADMAP_ERROR, "roadmap_device_events_os_term() - Monitor thread failed to shutdown within 3 seconds - KILLING IT!");
         TerminateThread( MonitorThread, -1);
      }
      
      CloseHandle( MonitorThread);
      MonitorThread = NULL;
   }

   if( eventPleaseQuit)
   {
      CloseHandle( eventPleaseQuit);
      eventPleaseQuit = NULL;
   }
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
DWORD WINAPI DeviceEventsMonitorThread( void* params)
{
   int                     i;
   HANDLE                  Events   [CORE_DEVICE_EVENTS_COUNT+2];
   HANDLE                  Triggers [CORE_DEVICE_EVENTS_COUNT];
   CE_NOTIFICATION_TRIGGER NT;
   DWORD                   dwEventsCount = sizeof(Events)/sizeof(HANDLE);
   DWORD                   dwWaitRes;
   BOOL                    bQuit = FALSE;
   
   memset( &NT, 0, sizeof(CE_NOTIFICATION_TRIGGER));
   
   NT.dwSize = sizeof(CE_NOTIFICATION_TRIGGER); 
   NT.dwType = CNT_EVENT; 

   Events[CORE_DEVICE_EVENTS_COUNT + 0] = eventShuttingDown;
   Events[CORE_DEVICE_EVENTS_COUNT + 1] = eventPleaseQuit;
   
   // Create all events and triggers:
   for( i=0; i<CORE_DEVICE_EVENTS_COUNT; i++)
   {
      WCHAR EventName      [ 64];
      WCHAR EventFullName  [112];

      swprintf( EventName,    L"%S", get_device_event_name(i));
      swprintf( EventFullName,L"\\\\.\\Notifications\\NamedEvents\\%s", EventName);
      
      NT.dwEvent        = get_win32_device_event_id(i); 
      NT.lpszApplication= EventFullName;
      Events   [i]      = CreateEvent( NULL, FALSE /* Manual? */, FALSE, EventName);
      Triggers [i]      = CeSetUserNotificationEx( 0, &NT, NULL);
      
      if( !Events[i] || !Triggers[i])
      {
         while(i--)
         {
            if( Triggers[i])
            {
               CeClearUserNotification( Triggers[i]);
               Triggers[i] = NULL;
            }
         
            if( Events[i])
            {
               CloseHandle( Events[i]);
               Events[i] = NULL;
            }
         }
         
         roadmap_log( ROADMAP_ERROR, "DeviceEventsMonitorThread() - Failed to create event or trigger %d; Error: %d", i, GetLastError());
         return -1;  
      }
   }
   
   do
   {
      dwWaitRes = WaitForMultipleObjects( dwEventsCount, Events, FALSE, INFINITE);

      if( (dwWaitRes < WAIT_OBJECT_0) || ((WAIT_OBJECT_0+dwEventsCount)<=dwWaitRes))
      {
         roadmap_log( ROADMAP_ERROR, "DeviceEventsMonitorThread() - Unexpected value (%d) returned from 'WaitFor...'", dwWaitRes);
         break;
      }
      
      dwWaitRes -= WAIT_OBJECT_0;
      roadmap_log( ROADMAP_DEBUG, "DeviceEventsMonitorThread() - Got event %d (%s)", dwWaitRes, get_device_event_name(dwWaitRes));

      switch( dwWaitRes)
      {
         case device_event_hardware_change:
         //se device_event_internet_proxy_changed:
         case device_event_infrared_server:
         //se device_event_network_connected:
         //se device_event_network_disconnected:
         case device_event_AC_power_on:
         case device_event_AC_power_off:
         //se device_event_RNDISFN_instantiated:
         case device_event_RS232_connection_established:
         case device_event_system_time_change:
         case device_event_system_time_zone_change:
         case device_event_device_woke_up:
            PostMessage( RoadMapMainWindow, WM_FREEMAP_DEVICE_EVENT, dwWaitRes, 0);
            break;

         case CORE_DEVICE_EVENTS_COUNT+0:
         case CORE_DEVICE_EVENTS_COUNT+1:
            bQuit = TRUE;
            break;
         
//         default:
         //   assert(0);
      }
      
   }  while( !bQuit);   
   
   for( i=0; i<CORE_DEVICE_EVENTS_COUNT; i++)
   {
      if( Triggers[i])
      {
         CeClearUserNotification( Triggers[i]);
         Triggers[i] = NULL;
      }

      if( Events[i])
      {
         CloseHandle( Events[i]);
         Events[i] = NULL;
      }
   }

   ///"DeviceEventsMonitorThread() - Exiting..."

   return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
DWORD get_win32_device_event_id( device_event event)
{
   switch(event)
   {
      case device_event_hardware_change             : return NOTIFICATION_EVENT_DEVICE_CHANGE;
      //se device_event_internet_proxy_changed      : return NOTIFICATION_EVENT_INTERNET_PROXY_CHANGE;
      case device_event_infrared_server             : return NOTIFICATION_EVENT_IR_DISCOVERED;
      case device_event_network_connected           : return NOTIFICATION_EVENT_NET_CONNECT;
      case device_event_network_disconnected        : return NOTIFICATION_EVENT_NET_DISCONNECT;
      case device_event_AC_power_on                 : return NOTIFICATION_EVENT_ON_AC_POWER;
      case device_event_AC_power_off                : return NOTIFICATION_EVENT_OFF_AC_POWER;
      //se device_event_RNDISFN_instantiated        : return NOTIFICATION_EVENT_RNDIS_FN_DETECTED;
      case device_event_RS232_connection_established: return NOTIFICATION_EVENT_RS232_DETECTED;
      case device_event_system_time_change          : return NOTIFICATION_EVENT_TIME_CHANGE;
      case device_event_system_time_zone_change     : return NOTIFICATION_EVENT_TZ_CHANGE;
      case device_event_device_woke_up              : return NOTIFICATION_EVENT_WAKEUP;

      default: assert(0); return NOTIFICATION_EVENT_NONE;
   }
}
#endif   // UNDER_CE
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
