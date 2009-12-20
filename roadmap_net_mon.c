/* roadmap_net_mon.c - network activity monitor
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
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
 *
 * DESCRIPTION:
 *
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_lang.h"
#include "roadmap_message.h"
#include "roadmap_start.h"
#include "roadmap_main.h"
#include "roadmap_start.h"
#include "roadmap_screen.h"

#include "roadmap_net_mon.h"

#define ACTIVITY_TIMEOUT_SEC  3

static size_t RecvBytesCount;
static size_t SendBytesCount;
static int NumConnections;
static time_t LastActivityTime = 0;
static const char *LastErrorText = "";
static ROADMAP_NET_MON_STATE CurrentState = NET_MON_DISABLED;

static void periodic_callack (void) {

   if(
#ifndef   _WIN32
      (CurrentState == NET_MON_IDLE) &&
#endif   // _WIN32
         ((time(NULL) - LastActivityTime) > ACTIVITY_TIMEOUT_SEC)) {

      roadmap_message_unset('!');
      LastActivityTime = 0;
      roadmap_main_remove_periodic(periodic_callack);
      roadmap_screen_mark_redraw();
   }
}


static void update_activity (void) {
   if (!LastActivityTime) roadmap_main_set_periodic (1000, periodic_callack);
   LastActivityTime = time(NULL);   
   roadmap_screen_mark_redraw ();
}


/* Called when a connection initialization is about to be attempted.
 * For mobile phones this should be called when a connection
 * module is started.
 */
void roadmap_net_mon_start (void) {
   assert (CurrentState == NET_MON_DISABLED || CurrentState == NET_MON_OFFLINE);
   CurrentState = NET_MON_START;
   roadmap_message_set('!', roadmap_lang_get("Accessing network..."));
   roadmap_screen_mark_redraw ();
   update_activity();
}


/* Called after the connection module is shutdown
 */
void roadmap_net_mon_destroy (void) {
   assert (CurrentState != NET_MON_DISABLED);
   CurrentState = NET_MON_DISABLED;
   roadmap_screen_mark_redraw ();
}


/* Called when a new connection is about to be opened
 */
void roadmap_net_mon_connect (void) {
   assert (CurrentState != NET_MON_DISABLED);
   if ( CurrentState == NET_MON_OFFLINE ) { return; } //  connect means nothing in this case
   CurrentState = NET_MON_CONNECT;
   NumConnections++;
   roadmap_message_set('!', roadmap_lang_get("Connecting..."));
   update_activity();
}


/* Called after a connection is closed
 */
void roadmap_net_mon_disconnect (void) {
   assert (CurrentState != NET_MON_DISABLED);
   if ( CurrentState == NET_MON_OFFLINE ) { return; } //  disconnect means nothing in this case
   assert (NumConnections);
   NumConnections--;
   if (NumConnections == 0) CurrentState = NET_MON_IDLE;
}


/* Called after data is sent over the net
 */
void roadmap_net_mon_send (size_t size) {
   assert (CurrentState != NET_MON_DISABLED);
   CurrentState = NET_MON_DATA;
   SendBytesCount += size;
   roadmap_message_set('!', "%d KB", roadmap_net_mon_get_count() / 1024);
   update_activity();
}


/* Called after data is received from the net
 */
void roadmap_net_mon_recv (size_t size) {

#ifndef _WIN32
   assert (CurrentState != NET_MON_DISABLED);
#endif

   CurrentState = NET_MON_DATA;
   RecvBytesCount += size;
   roadmap_message_set('!', "%d KB", roadmap_net_mon_get_count() / 1024);
   update_activity();
}


/* Called if an error occures
*/
void roadmap_net_mon_error (const char *text) {
   LastErrorText = text;
   CurrentState = NET_MON_ERROR;
   roadmap_message_set('!', text);
   update_activity();
}


/* Returns the current status of the net activity
 */
ROADMAP_NET_MON_STATE roadmap_net_mon_get_status (void) {

   return CurrentState;
}


/* Returns the error message. Should be called only if we're in error state
 */
const char *roadmap_net_mon_get_error_text (void) {
   return LastErrorText;
}


/* Returns the total amount of bytes recvd+sent on the net
 */
size_t roadmap_net_mon_get_count (void) {
   return RecvBytesCount + SendBytesCount;
}

/* Called when user chose not to connect 
 */
void roadmap_net_mon_offline (void) {
   CurrentState = NET_MON_OFFLINE;
   roadmap_message_set('!', roadmap_lang_get("Offline"));
   roadmap_start_exit ();
}

