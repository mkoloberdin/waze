/*
 * LICENSE:
 *
 *   Copyright 2009 Maxim Kalaev
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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>
#ifndef WIN32
#include "../roadmap_string.h"
#endif

#include "../roadmap_net.h"
#include "socket_async_receive.h"

#include "websvc_trans.h"

#define MAX_RETRIES 3

static   void  on_socket_connected  ( RoadMapSocket Socket, void* context, roadmap_result res);
static   void  on_data_received     ( void* data, int size, void* context);
static   int   wst_Send             ( RoadMapSocket socket, const char* szData);
static   BOOL  wst_Receive          ( wst_context_ptr session);

static transaction_result OnHTTPAck( cyclic_buffer_ptr CB, http_parsing_state* parser_state);
static   transaction_result
               OnHTTPHeader         ( cyclic_buffer_ptr CB, http_parsing_state* parser_state);
static   transaction_result
               OnCustomResponse     ( wst_context_ptr session);
static LastNetConnectRes ELastNetConnectRes = LastNetConnect_Success;
static   int   gNextTypeId = 0;


static BOOL wst_start_trans__int(wst_context_ptr      session,
                                 int                  flags,
                                 const char*          action,
                                 int                  type,
                                 const wst_parser_ptr parsers,
                                 int                  parsers_count,
                                 CB_OnWSTCompleted    cbOnCompleted,
                                 void*                context,
                                 char*                packet,
                                 BOOL                 retry);

static void wst_stop_trans__int( wst_handle h, BOOL bStopNow, BOOL bNotifyCb);


void wst_context_init( wst_context_ptr this)
{
  memset( this, 0, sizeof(wst_context));

/* General:
   1  this->service              = NULL;
   2  this->content_type         = NULL;
   3  this->port                 = 0;              */
/* 4*/this->Socket               = ROADMAP_INVALID_SOCKET;/*
   5  this->state                = trans_idle;
   6  this->async_receive_started= FALSE;
   7  this->last_receive_time        = 0;              */
/* 8*/wstq_init(                 &(this->queue));  /*
   9  this->context              = NULL;           */
      this->retries              = 0;

/* Send:                                           */
/*10*/ebuffer_init(              &(this->packet));

/* Receive:                                        */
/*11*/cyclic_buffer_init(        &(this->CB));     /*
  12 this->http_parser_state     = http_not_parsed;
  13 this->parsers               = NULL;
  14 this->parsers_count         = 0;

  Completion:
  15 this->cbOnWSTCompleted      = NULL;
  16 this->result                = trans_succeeded;
  17 this->rc                    = succeeded;
  18 this->delete_on_idle        = FALSE;          */
   this->connect_context         = FALSE;
}

void wst_context_free( wst_context_ptr this)
{
   if( this->async_receive_started)
   {
      socket_async_receive_end( this->Socket);
      this->async_receive_started = FALSE;
   }

   if( ROADMAP_INVALID_SOCKET != this->Socket)
   {
      roadmap_net_close( this->Socket);
      this->Socket = ROADMAP_INVALID_SOCKET;
   }

   ebuffer_free( &(this->packet));
   wstq_clear  ( &(this->queue));

   wst_context_init( this);
}

// Refresh object between transactions:
static void wst_context_reset( wst_context_ptr this, int use_ack)
{
// Don't loose these:
/* 1*/   const char*    service     = this->service;
/* 2*/   const char*    content_type= this->content_type;
/* 3*/   int            port        = this->port;
/* 4*/   RoadMapSocket  Socket      = this->Socket;

// Reset other transaction variables:

/* General:       */
/* 5*/   this->state                = trans_idle;
/* 6*/   this->async_receive_started= FALSE;
/* 7*/   this->last_receive_time        = 0;
// 8     this->queue    !NOT MODIFYING QUEUE!
/* 9*/   //this->context              = NULL;
   this->retries = 0;

/* Send:          */
/*10*/   ebuffer_free(              &(this->packet));

/* Receive:       */
/*11*/   cyclic_buffer_init(        &(this->CB));
/*12*/
   if (use_ack)
      this->http_parser_state    = http_not_acked;
   else
      this->http_parser_state    = http_not_parsed;
/*13*/   //this->parsers              = NULL;
/*14*/   //this->parsers_count        = 0;

/* Completion:    */
/*15*/   //this->cbOnWSTCompleted     = NULL;
/*16*/   this->result               = trans_succeeded;
/*17*/   this->rc                   = succeeded;
   
   this->active_item.type = WEBSVC_NO_TYPE;
   this->active_item.packet = NULL;

// Restore:
/* 1*/   this->service     = service;
/* 2*/   this->content_type= content_type;
/* 3*/   this->port        = port;
/* 4*/   this->Socket      = Socket;
}

static BOOL wst_Receive( wst_context_ptr session)
{
   cyclic_buffer_ptr CB = &(session->CB);

   if( ROADMAP_INVALID_SOCKET == session->Socket)
   {
      roadmap_log( ROADMAP_ERROR, "wst_Receive() - Socket is invalid");
      return FALSE;
   }

   //   Read next data
   if( !socket_async_receive( session->Socket,
                              CB->next_read,
                              CB->free_size,
                              on_data_received,
                              session))
   {
      roadmap_log( ROADMAP_ERROR, "wst_Receive( SOCKET: %d) - 'socket_async_receive()' had failed", roadmap_net_get_fd(session->Socket));
      return FALSE;
   }

   session->async_receive_started = TRUE;
   return TRUE;
}

static void  wst_context_load(wst_context_ptr         this,
                              int                     flags,
                              const char*             action,
                              int                     type,
                              const wst_parser_ptr    parsers,
                              int                     parsers_count,
                              CB_OnWSTCompleted       cbOnCompleted,
                              void*                   context,
                              char*                   packet)
{
   wst_context_reset( this, (flags & WEBSVC_FLAG_V2));

//   this->action            = action;
//   this->parsers           = parsers;
//   this->parsers_count     = parsers_count;
//   this->cbOnWSTCompleted  = cbOnCompleted;
//   this->context           = context;
   this->state             = trans_active;
   
   //Save current item for retries
   wstq_item_init( &this->active_item);
   
   this->active_item.action        = action;
   this->active_item.type          = type;
   this->active_item.parsers       = parsers;
   this->active_item.parsers_count = parsers_count;
   this->active_item.cbOnCompleted = cbOnCompleted;
   this->active_item.context       = context;
   this->active_item.packet        = strdup(packet);
   this->active_item.flags         = flags;
}

wst_handle wst_init( const char* service_name,  // e.g. - rtserver
                     const char* secured_service_name,
                     const char* secured_service_name_resolved,
                     const char* service_v2_suffix,
                     const char* content_type)  // e.g. - binary/octet-stream
{
   int               server_port;
   int               secured_server_port;
   wst_context_ptr   session = NULL;

   if( !service_name || !(*service_name))
   {
      assert(0);  // Please supply service name
      return session;
   }

   if( !WSA_ExtractParams( service_name,  //   IN        -   Web service full address (http://...)
                           NULL,          //   OUT,OPT   -   Server URL[:Port]
                           &server_port,  //   OUT,OPT   -   Server Port
                           NULL))         //   OUT,OPT   -   Web service name
      return session;

   session = malloc( sizeof(wst_context));
   wst_context_init( session);
   session->service     = service_name;
   session->content_type= content_type;
   session->port        = server_port;
   
   if (secured_service_name && *secured_service_name) {
      if( !WSA_ExtractParams( secured_service_name,  //   IN        -   Web service full address (https://...)
                             NULL,          //   OUT,OPT   -   Server URL[:Port]
                             &secured_server_port,  //   OUT,OPT   -   Secured Server Port
                             NULL))         //   OUT,OPT   -   Web service name
         return session;
      
      session->secured_service = secured_service_name;
      session->secured_port = secured_server_port;
   }
   
   if (secured_service_name_resolved && *secured_service_name_resolved) {      
      session->secured_service_resolved = secured_service_name_resolved;
   }
   
   if (service_v2_suffix) {
      session->service_v2_suffix = service_v2_suffix;
   }
   
   return session;
}

void wst_term( wst_handle h)
{
   wst_context_ptr session = (wst_context_ptr)h;

   if( session)
   {
      if( trans_idle != session->state)
      {
         //assert(0);  // THIS IS NOT AN ERROR
                     // Just left this assert in order to see if this case happens...

         session->delete_on_idle = TRUE;
         wst_stop_trans( h, FALSE);
      }
      else
      {
         wst_context_free( session);
         free( session);
      }

      session  = NULL;
      h        = NULL;
   }
}

void wst_queue_clear( wst_handle h)
{
   wst_context_ptr session = (wst_context_ptr)h;

   assert(session);

   wstq_clear( &(session->queue));
}

BOOL wst_queue_is_empty( wst_handle h)
{
   wst_context_ptr session = (wst_context_ptr)h;

   assert(session);

   return wstq_is_empty( &(session->queue));
}

BOOL wst_process_queue_item( wst_handle h, BOOL* transaction_started)
{
   wst_context_ptr   session = (wst_context_ptr)h;
   wstq_item      Item;
   BOOL                    bRes;

   (*transaction_started) = FALSE;

   if( wst_queue_is_empty( session))
      return TRUE;

   if( !wstq_dequeue( &(session->queue), &Item))
   {
      roadmap_log( ROADMAP_ERROR, "wst_process_queue_item() - 'wstq_dequeue()' had failed!");
      return FALSE;
   }

   bRes = wst_start_trans( session,
                           Item.flags,
                           Item.action,
                           Item.type,
                           Item.parsers,
                           Item.parsers_count,
                           Item.cbOnCompleted,
                           Item.context,
                           Item.packet);

   wstq_item_release( &Item);

   if( bRes)
      (*transaction_started) = TRUE;

   return bRes;
}

// This method terminates the a-sync transaction:
// 1. Call callback
// 2. Reset context
void wst_transaction_completed( wst_handle h, roadmap_result res)
{
   wst_context_ptr   session  = (wst_context_ptr)h;
   CB_OnWSTCompleted cb       = session->active_item.cbOnCompleted;
   void*             context  = session->active_item.context;

   if( session->delete_on_idle)
   {
      wst_context_free( session);
      free( session);

      return;  // Do we want to call callback? (no)
   }

   if (ROADMAP_INVALID_SOCKET == session->Socket)
   {
      roadmap_log(ROADMAP_DEBUG, "wst_transaction_completed() - stopping active session WITHOUT socket");
      if (session->connect_context) {//should never be NULL !
         roadmap_net_cancel_connect(session->connect_context);
         session->connect_context = NULL;
         session->state = trans_idle;
      }
   }

   if( session->async_receive_started)
   {
      socket_async_receive_end( session->Socket);
      session->async_receive_started = FALSE;
   }

   if( ROADMAP_INVALID_SOCKET != session->Socket)
   {
      roadmap_net_close( session->Socket);
      session->Socket = ROADMAP_INVALID_SOCKET;
   }

   if (res == err_timed_out &&
       time(NULL) - session->last_receive_time < WST_SESSION_TIMEOUT && //means we did not get any response
       session->retries++ < MAX_RETRIES) {
      BOOL bRes = wst_start_trans__int(session,                            // Session object
                                       session->active_item.flags,         // Session flags
                                       session->active_item.action,        // /<service_name>/<action>
                                       session->active_item.type,          // Type identifier
                                       session->active_item.parsers,       // Array of 1..n data parsers
                                       session->active_item.parsers_count, // Parsers count
                                       session->active_item.cbOnCompleted, // Callback for transaction completion
                                       session->active_item.context,       // Caller context
                                       session->active_item.packet,        // Custom data for the HTTP request
                                       TRUE);                              // Retry
      if (!bRes) {
         wst_context_reset( session, 0);
         
         if(cb)
            cb( context, err_net_failed);
      }
   } else {
      if (session->active_item.packet && session->active_item.packet[0]){
         free(session->active_item.packet);
         session->active_item.packet = NULL;
      }
      
      wst_context_reset( session, 0);
      
      if(cb)
         cb( context, res);
   }
}

void wst_watchdog( wst_handle h)
{
   wst_context_ptr   session        = (wst_context_ptr)h;
   time_t            now            = time(NULL);
   int               seconds_passed = (int)(now - session->last_receive_time);

   if (!session->last_receive_time)
      return;
   
   if (session->Socket != ROADMAP_INVALID_SOCKET &&
              session->http_parser_state == http_not_acked &&
              seconds_passed >= WST_ACK_TIMEOUT) {
      //connect to ack timeout
      roadmap_log( ROADMAP_ERROR, "wst_watchdog() - TERMINATING SESSION !!! - Ack not received in %d seconds (action '%s')",
                  seconds_passed, session->active_item.action);
      wst_transaction_completed( session, err_timed_out);
   } else if (session->Socket != ROADMAP_INVALID_SOCKET &&
              session->http_parser_state == http_not_parsed &&
              !(session->active_item.flags & WEBSVC_FLAG_V2) &&
              seconds_passed >= WST_ACK_TIMEOUT * 2) {
      //connect to header timeout (non-ack request)
      roadmap_log( ROADMAP_ERROR, "wst_watchdog() - TERMINATING SESSION !!! - Header not received in %d seconds (action '%s')",
                  seconds_passed, session->active_item.action);
      wst_transaction_completed( session, err_timed_out);
   } else if (session->Socket != ROADMAP_INVALID_SOCKET &&
              session->http_parser_state == http_not_parsed &&
              session->active_item.flags & WEBSVC_FLAG_V2 &&
              seconds_passed >= WST_ACK_HEADER_TIMEOUT) {
      //ack to header timeout
      roadmap_log( ROADMAP_ERROR, "wst_watchdog() - TERMINATING SESSION !!! - Ack to header not received in %d seconds (action '%s')",
                  seconds_passed, session->active_item.action);
      wst_transaction_completed( session, err_timed_out);
   } else if (session->Socket != ROADMAP_INVALID_SOCKET &&
              session->http_parser_state != http_not_acked && session->http_parser_state != http_not_parsed &&
//              session->active_item.flags & WEBSVC_FLAG_V2 &&
              seconds_passed >= WST_RECEIVE_TIMEOUT) {
      //between receives timeout
      roadmap_log( ROADMAP_ERROR, "wst_watchdog() - TERMINATING SESSION !!! - No Receive in %d seconds (action '%s')",
                  seconds_passed, session->active_item.action);
      wst_transaction_completed( session, err_timed_out);
   } else if (seconds_passed >= WST_SESSION_TIMEOUT) {
      roadmap_log( ROADMAP_ERROR, "wst_watchdog() - TERMINATING SESSION !!! - Session is running already %d seconds (action '%s')",
                  seconds_passed, session->active_item.action);
      wst_transaction_completed( session, err_timed_out);
   }     
}

static void wstq_RemoveType ( wst_context_ptr   session,
                             int        type) {
   wstq_remove_type (&(session->queue), type);
   
   if (trans_idle != session->state &&
       session->active_item.type != WEBSVC_NO_TYPE &&
       session->active_item.type == type) {
      roadmap_log(ROADMAP_WARNING, "wstq_RemoveType() - stopping active session with type %d", type);
      wst_stop_trans__int(session, TRUE, FALSE);
   }
}

static BOOL wstq_Add( wst_context_ptr      session,
                      int                  flags,
                      const char*          action,
                      int                  type,
                      const wst_parser_ptr parsers,
                      int                  parsers_count,
                      CB_OnWSTCompleted    cbOnCompleted,
                      void*                context,
                      char*                packet)
{
   wstq_item  TQI;

   if( !parsers || !parsers_count || !cbOnCompleted || !packet || !packet[0])
   {
      roadmap_log( ROADMAP_ERROR, "wstq_Add() - Invalid argument");
      return FALSE;  // Invalid argument
   }


   wstq_item_init( &TQI);

   TQI.action        = action;
   TQI.type          = type;
   TQI.parsers       = parsers;
   TQI.parsers_count = parsers_count;
   TQI.cbOnCompleted = cbOnCompleted;
   TQI.context       = context;
   TQI.packet        = strdup(packet); //AviR: Do we free this?
   TQI.flags         = flags;

   if( wstq_enqueue( &(session->queue), &TQI))
      return TRUE;

   // Else (queue will log on error)
   wstq_item_release( &TQI);
   return FALSE;
}

// Assumptions and implementation notes:
// 1. This method is called from a single thread, thus no MT locks are used.
// 2. This method supports only one request at a time.
//       Until the full transaction is completed (including all callbacks), any new
//       requests will be inserted into queue.
//       If queue is full, the method will return an error.
// 3. Signal on a-sync method termination:
//       If method returned TRUE it means that the a-sync operation had begun.
//       When the a-sync operation will terminate the callback 'cbOnCompleted'
//       will be called.
//       If method returned FALSE - the a-sync operation did not begin and the callback
//       'cbOnCompleted' will not be called.
static BOOL wst_start_trans__int(wst_context_ptr      session,
                                 int                  flags,
                                 const char*          action,
                                 int                  type,
                                 const wst_parser_ptr parsers,
                                 int                  parsers_count,
                                 CB_OnWSTCompleted    cbOnCompleted,
                                 void*                context,
                                 char*                packet,
                                 BOOL                 retry)
{
   char* AsyncPacket    = NULL;
   int   AsyncPacketSize= 0;
   char  WebServiceMethod[WST_WEBSERVICE_METHOD_MAX_SIZE];
   char  WebServiceMethodResolved[WST_WEBSERVICE_METHOD_MAX_SIZE];
   int   port;
   
   if (!retry) {
      if(!session || !action        || !(*action)     ||
         !parsers || !parsers_count || !cbOnCompleted ||
         !packet  || !(*packet))
      {
         assert(0);  // Invalid arguments
         return FALSE;
      }
      
      if( trans_idle != session->state) {
         if (type != WEBSVC_NO_TYPE)
            wstq_RemoveType( session, type);
      }
      
      if( trans_idle != session->state) {
         roadmap_log(ROADMAP_DEBUG, "wst_start_trans() - queue not idle, adding to queue item of type: %d", type);
         return wstq_Add(  session, flags, action, type, parsers, parsers_count,
                         cbOnCompleted, context, packet);
      }
      
      if( ROADMAP_INVALID_SOCKET != session->Socket)
      {
         assert(0);  // No session is active - Socket should be invalid..
         roadmap_log( ROADMAP_ERROR, "wst_start_trans() - socket should be invalid");
         return FALSE;
      }
      
      //    Inital transaction context:
      wst_context_load( session, flags, action, type, parsers, parsers_count, cbOnCompleted, context, packet);
   } else {
      roadmap_log( ROADMAP_ERROR, "wst_start_trans() - retry # %d", session->retries); //TODO: reduce log level
      session->state             = trans_active;
      
      if (flags & WEBSVC_FLAG_V2)
         session->http_parser_state    = http_not_acked;
      else
         session->http_parser_state    = http_not_parsed;
   }
   

   // Allocate buffer for the async-info object:
   AsyncPacketSize= HTTP_HEADER_MAX_SIZE + strlen(packet) + 10;
   AsyncPacket    = ebuffer_alloc( &(session->packet), AsyncPacketSize);

   if ((flags & WEBSVC_FLAG_SECURED) && !(flags & WEBSVC_FLAG_V2)) {
      snprintf(WebServiceMethod,
               WST_WEBSERVICE_METHOD_MAX_SIZE,
               "%s/%s", session->secured_service, action);
      snprintf(WebServiceMethodResolved,
               WST_WEBSERVICE_METHOD_MAX_SIZE,
               "%s/%s", session->secured_service_resolved, action);
      port = session->secured_port;
   } else if (!(flags & WEBSVC_FLAG_SECURED) && (flags & WEBSVC_FLAG_V2)) {
      snprintf(WebServiceMethod,
               WST_WEBSERVICE_METHOD_MAX_SIZE,
               "%s%s/%s", session->service, session->service_v2_suffix, action);
      snprintf(WebServiceMethodResolved,
               WST_WEBSERVICE_METHOD_MAX_SIZE,
               "%s%s/%s", session->service, session->service_v2_suffix, action);
      port = session->port;
   } else if ((flags & WEBSVC_FLAG_SECURED) && (flags & WEBSVC_FLAG_V2)) {
      snprintf(WebServiceMethod,
               WST_WEBSERVICE_METHOD_MAX_SIZE,
               "%s%s/%s", session->secured_service, session->service_v2_suffix, action);
      snprintf(WebServiceMethodResolved,
               WST_WEBSERVICE_METHOD_MAX_SIZE,
               "%s%s/%s", session->secured_service_resolved, session->service_v2_suffix, action);
      port = session->secured_port;
   } else {
      snprintf(WebServiceMethod,
               WST_WEBSERVICE_METHOD_MAX_SIZE,
               "%s/%s", session->service, action);
      snprintf(WebServiceMethodResolved,
               WST_WEBSERVICE_METHOD_MAX_SIZE,
               "%s/%s", session->service, action);
      port = session->port;
   }


   snprintf(AsyncPacket,
            AsyncPacketSize,
            "Content-type: %s\r\n"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s",
            session->content_type,
            (unsigned int)strlen(packet),
            packet);

   // Start the async-connect process:
   session->connect_context = roadmap_net_connect_async("http_post",
                                                        WebServiceMethod,
                                                        WebServiceMethodResolved,
                                                        0,
                                                        port,
                                                        NET_COMPRESS,
                                                        on_socket_connected,
                                                        session);
   if( NULL == session->connect_context)
   {
      ELastNetConnectRes = LastNetConnect_Failure;
      roadmap_log( ROADMAP_ERROR, "wst_start_trans() - 'roadmap_net_connect_async' had failed (Invalid params or queue is full?)");
      wst_context_reset( session, (flags & WEBSVC_FLAG_V2));
      return FALSE;
   }
   ELastNetConnectRes = LastNetConnect_Success;
   // Mark starting time:
   session->last_receive_time = time(NULL);

   return TRUE;
}

transaction_state wst_get_trans_state( wst_handle h)
{
   wst_context_ptr   session = (wst_context_ptr)h;

   assert(session);

   return session->state;
}

static void wst_stop_trans__int( wst_handle h, BOOL bStopNow, BOOL bNotifyCb)
{
   wst_context_ptr   session = (wst_context_ptr)h;

   assert(session);
   
   if (trans_idle == session->state)
      return;

   if (bStopNow) {
      if (!bNotifyCb) {
         session->active_item.cbOnCompleted = NULL;
      }
      wst_transaction_completed( session, err_aborted);
   } else {
      session->state = trans_stopping;
   }
}

void wst_stop_trans( wst_handle h, BOOL bStopNow) {
   wst_stop_trans__int (h, bStopNow, TRUE);
}

BOOL wst_start_trans(wst_handle           h,             // Session object
                     int                  flags,         // Session flags
                     const char*          action,        // (/<service_name>/)<ACTION>
                     int                  type,          // Type identifier
                     const wst_parser_ptr parsers,       // Array of 1..n data parsers
                     int                  parsers_count, // Parsers count
                     CB_OnWSTCompleted    cbOnCompleted, // Callback for transaction completion
                     void*                context,       // Caller context
                     const char*          szFormat,      // Custom data for the HTTP request
                     ...)                                // Parameters
{
   wst_context_ptr   session = (wst_context_ptr)h;
   va_list           vl;
   int               j;
   ebuffer           Packet;
   char*             Data;
   int               SizeNeeded;
   BOOL              bRes;

   if( !h || !action || !(*action) || !parsers || !parsers_count || !cbOnCompleted || !szFormat || !(*szFormat))
   {
      assert(0);  // Invalid arguments
      return FALSE;
   }

   if( (parsers_count < WST_MIN_PARSERS_COUNT) || (WST_MAX_PARSERS_COUNT < parsers_count))
   {
      assert(0);  // Invalid parsers count
      return FALSE;
   }

   // Only a single 'default parser' can be supplied
   //
   // Default parser is a parser with no 'tag name'.
   // This parser will handle all data, which was not matched with
   // the supplied parsers-tags.
   if( 1 != parsers_count)
   {
      // No 'default parsers' are allowed:
      int   i;
      BOOL  default_parser_found = FALSE;

      for( i=0; i<parsers_count; i++)
      {
         if( !parsers[i].tag || !parsers[i].tag[0])
         {
            if( default_parser_found)
            {
               assert(0);  // More then one 'default parser'
               return FALSE;
            }

            default_parser_found = TRUE;
         }
      }
   }

   ebuffer_init( &Packet);

   SizeNeeded  = HTTP_HEADER_MAX_SIZE + strlen(szFormat) + 0xFF;
   Data        = ebuffer_alloc( &Packet, SizeNeeded);

   va_start(vl, szFormat);
   j = vsnprintf( Data, SizeNeeded, szFormat, vl);
   va_end(vl);

   if( j < 0)
   {
      roadmap_log( ROADMAP_ERROR, "wst_start_trans() - Failed to format command '%s' (buffer size too small?)", szFormat);
      ebuffer_free( &Packet);
      return FALSE;
   }

   bRes = wst_start_trans__int(  session,       // Session object
                                 flags,         // Session flags
                                 action,        // /<service_name>/<action>
                                 type,          // Type identifier
                                 parsers,       // Array of 1..n data parsers
                                 parsers_count, // Parsers count
                                 cbOnCompleted, // Callback for transaction completion
                                 context,       // Caller context
                                 Data,          // Custom data for the HTTP request
                                 FALSE);        // Retry

   ebuffer_free( &Packet);

   return bRes;
}

transaction_result on_socket_connected_(  RoadMapSocket     Socket,
                                          wst_context_ptr   session,
                                          roadmap_result    res)
{
   const char* packet;

   session->rc = res;


   // Verify Socket is valid:
   if( ROADMAP_INVALID_SOCKET == Socket)
   {
      assert( succeeded != res);
      roadmap_log( ROADMAP_ERROR, "on_socket_connected() - INVALID SOCKET - Failed to create Socket; Error '%s'", roadmap_result_string( res));
      /*
       * In this case the context is deallocated in the net layer.
       */
      session->connect_context = NULL;
      return trans_failed;
   }

   assert( succeeded == res);

   packet         = ebuffer_get_buffer( &(session->packet));
   session->Socket= Socket;
   
   // Were we asked to abort transaction?
   if( trans_stopping == session->state)
   {
      session->rc = err_aborted;
      roadmap_log( ROADMAP_ERROR, "on_socket_connected() - Was asked to abort session");
      return trans_failed;
   }

   // Try to send packet:
   if( !wst_Send( Socket, packet))
   {
      session->rc = err_net_failed;

      roadmap_log( ROADMAP_ERROR, "on_socket_connected() - 'wst_Send()' had failed");
      return trans_failed;
   }

   if( !wst_Receive( session))
   {
      roadmap_log( ROADMAP_ERROR, "on_socket_connected() - 'wst_Receive()' had failed");
      return trans_failed;
   }
   
   session->last_receive_time = time(NULL);

   return trans_in_progress;
}

void on_socket_connected( RoadMapSocket Socket, void* context, roadmap_result res)
{
   wst_context_ptr session = (wst_context_ptr)context;

   session->result = on_socket_connected_( Socket, context, res);
   switch( session->result)
   {
      case trans_succeeded:
         wst_transaction_completed( session, succeeded);
         break;

      case trans_failed:
         wst_transaction_completed( session, session->rc);
         break;

      case trans_in_progress:
         break;
   }
}

transaction_result on_data_received_( void* data, int size, wst_context_ptr session)
{
   cyclic_buffer_ptr    CB;
   http_parsing_state   http_parser_state;
   transaction_result   res;

   assert(session);

   // Were we asked to abort transaction?
   if( trans_stopping == session->state)
   {
      session->rc = err_aborted;
      roadmap_log( ROADMAP_ERROR, "on_data_received() - Was asked to abort session");
      return trans_failed;
   }

   if( size < 0)
   {
      roadmap_log( ROADMAP_ERROR, "on_data_received( SOCKET: %d) - 'roadmap_net_receive()' returned %d bytes", roadmap_net_get_fd(session->Socket), size);
      return trans_failed;
   }

   if( size > 0 && data == NULL )
   {
      roadmap_log( ROADMAP_ERROR, "on_data_received( SOCKET: %d) - Invalid data (NULL)", roadmap_net_get_fd(session->Socket));
      return trans_failed;
   }

   roadmap_log( ROADMAP_DEBUG, "on_data_received( SOCKET: %d) - Received %d bytes", roadmap_net_get_fd(session->Socket), size);
   session->last_receive_time = time(NULL);
   
   CB                = &(session->CB);
   http_parser_state = session->http_parser_state;
   res               = trans_failed;   //   Default

   //   Terminate data with NULL, so it can be processed as string:
   CB->read_size += size;
   CB->buffer[ CB->read_size] = '\0';

   //   Handle data:
   //   1.   Handle ACK response
   if( http_not_acked == http_parser_state)
   {
      //   Http data was not processed yet; Use HTTP handler:
      res = OnHTTPAck( CB, &http_parser_state);
      
      //   OK?
      if( res == trans_failed ) {
         roadmap_log( ROADMAP_DEBUG,
                     "on_data_received() - Read 'ack' Status: %s",
                     "Failed" );
         
         if( succeeded == session->rc )
            session->rc = err_failed;
         
         return trans_failed;
      }
      
      session->http_parser_state = http_parser_state;
   }
   
   //   2.   Handle HTTP headers:
   if( http_parse_completed != http_parser_state &&
      http_not_acked != http_parser_state)
   {
      //   Http data was not processed yet; Use HTTP handler:
      res = OnHTTPHeader( CB, &http_parser_state);

      //   Done?
      if( trans_succeeded == res )
         session->http_parser_state = http_parse_completed;
   }

   //   3.   Handle custom data:
   if( http_parse_completed == http_parser_state)
      res = OnCustomResponse( session);
   
   if (res == trans_canceled)
      return res;

   if( res == trans_failed ) {
      roadmap_log( ROADMAP_DEBUG,
                   "on_data_received() - Finish to process all data; Status: %s",
                   "Failed" );

      if( succeeded == session->rc )
         session->rc = err_failed;

      return trans_failed;
   }

   // If no more data is expected to be received because either
   //  the last packet has arrived or amount specified by content-length
   //  has been received:
   if(size == 0 ||
       (res != trans_in_progress && CB->data_processed + CB->read_size >= CB->data_size )) {
	   // Check that no data has left unprocessed: 
	   if( CB->read_size != CB->read_processed ) {
		   return trans_failed;
	   }

	   roadmap_log( ROADMAP_DEBUG,
	                "on_data_received() - Finish to process all data; Status: %s", "Succeeded" );
	  return trans_succeeded;
   }
   
   //   Recycle buffer:
   cyclic_buffer_recycle( CB);

   if( 0 == CB->free_size)
   {
      // Need to make CYCLIC_BUFFER_SIZE bigger
      roadmap_log( ROADMAP_ERROR, "on_data_received() - Buffer size is smaller then a single response-command");
      return trans_failed;
   }

#ifdef   _DEBUG
   //   Sanity:
   if( (CB->free_size < 0) || (WST_RESPONSE_BUFFER_SIZE < CB->free_size))
   {
      roadmap_log( ROADMAP_ERROR, "on_data_received() - [BUG] Invalid value in 'free_size': %d", CB->free_size);
      return trans_failed;
   }
#endif   // _DEBUG

   //   Read next data
   if( !socket_async_receive( session->Socket,
                              CB->next_read,
                              CB->free_size,
                              on_data_received,
                              session))
   {
      roadmap_log( ROADMAP_ERROR, "on_data_received( SOCKET: %d) - 'socket_async_receive()' had failed", roadmap_net_get_fd(session->Socket));
      return trans_failed;
   }

   session->async_receive_started = TRUE;
   
   return trans_in_progress;
}

static void on_data_received( void* data, int size, void* context)
{
   wst_context_ptr session = (wst_context_ptr)context;

   if( -1 == session->CB.data_size)
      session->CB.data_size = size;

   session->result = on_data_received_( data, size, session);

   switch( session->result)
   {
      case trans_succeeded:
         wst_transaction_completed( session, session->rc);
         break;

      case trans_failed:
         if( succeeded == session->rc)
            session->rc = err_failed;

         wst_transaction_completed( session, session->rc);
         break;

      case trans_in_progress:
      case trans_canceled:
         break;
   }
}

static int wst_Send( RoadMapSocket socket, const char* szData)
{
   int iRes;

   if( ROADMAP_INVALID_SOCKET == socket)
   {
      roadmap_log( ROADMAP_ERROR, "wst_Send() - Socket is invalid");
      return -1;
   }

   if( NULL == szData)
   {
         roadmap_log( ROADMAP_ERROR, "wst_Send() - szData is NULL");
         return -1;
   }
   
   iRes = roadmap_net_send( socket, szData, (int)strlen(szData), 1 /* Wait */);

   if( -1 == iRes)
      roadmap_log( ROADMAP_ERROR, "wst_Send( SOCKET: %d) - 'roadmap_net_send()' returned -1", roadmap_net_get_fd(socket));
   else
      roadmap_log( ROADMAP_DEBUG, "wst_Send( SOCKET: %d) - Sent %d bytes", roadmap_net_get_fd(socket), strlen(szData));

   return iRes;
}

//   General HTTP packet parser
//   Used prior to any response by all response-cases
static transaction_result OnHTTPAck( cyclic_buffer_ptr CB, http_parsing_state* parser_state)
{
   if( http_not_acked == *parser_state)
   {
      const char* buffer   = cyclic_buffer_get_unprocessed_data( CB);
      char* szDelimiter = strstr( buffer, "\r\n");
      if( !szDelimiter)
         return trans_in_progress;   //   Continue reading...
      
      // Lower case:
      ToLowerN( (char*)buffer, (size_t)(szDelimiter - (char*)buffer));
      
      //   Verify we have the 'ACK' status:
      if( !strstr( buffer, "ack"))
      {
         roadmap_log( ROADMAP_ERROR, "WST::OnHTTPAck() - Response is not 'ack' (%s)", buffer);
         return trans_failed;         //   Quit reading loop
      }
      
      roadmap_log(ROADMAP_WARNING, "WST::OnHTTPAck() - Succeeded"); //TODO: reduce log level
      
      //   Found the ACK:
      (*parser_state) = http_not_parsed;
      CB->read_processed += strlen("ack\r\n");
   }
      
   return trans_succeeded;
}

//   General HTTP packet parser
//   Used prior to any response by all response-cases
static transaction_result OnHTTPHeader( cyclic_buffer_ptr CB, http_parsing_state* parser_state)
{
   const char* pDataSize;
   const char* pHeaderEnd;
   const char* buffer   = cyclic_buffer_get_unprocessed_data( CB);
   int         data_size= 0;

   //   Did we find the 'OK' status already?
   if( http_not_parsed == *parser_state)
   {
      char* szDelimiter = strstr( buffer, "\r\n");
      if( !szDelimiter)
         return trans_in_progress;   //   Continue reading...

      // Lower case:
      ToLowerN( (char*)buffer, (size_t)(szDelimiter - (char*)buffer));

      //   Verify we have the '200 OK' status:
      if( !strstr( buffer, "200 ok"))
      {
         roadmap_log( ROADMAP_ERROR, "WST::OnHTTPHeader() - Response not successfull (%s)", buffer);
         return trans_failed;         //   Quit reading loop
      }

      //   Found the OK status:
      (*parser_state) = http_status_verified;
   }

   //   Search for the data sign:
   pHeaderEnd = strstr( buffer, "\r\n\r\n");
   if( !pHeaderEnd)
      return trans_in_progress;   //   Continue reading...

   // Initialize buffer processed-size:
   CB->read_processed   += (int)(((size_t)pHeaderEnd  + strlen("\r\n\r\n")) - (size_t)(buffer));
   CB->data_processed   = 0;        // Out of 'data_size', how much was processed
   CB->data_size        = INT_MAX;  // Data size is not known, unless content-length header is set:

   // Lower case:
   ToLowerN( (char*)buffer, (size_t)(pHeaderEnd - (char*)buffer) );

   //   Search for content length:
   pDataSize = strstr( buffer, "content-length:");
   if( pDataSize != NULL ) {
	   //   Move pointer:
	   pDataSize += strlen("content-length:");
	   //   Read size:
	   pDataSize  = ReadIntFromString(
						 pDataSize,           //   [in]     Source string
						 "\r\n",              //   [in,opt] Value termination
						 " ",                 //   [in,opt] Allowed padding
						 &data_size,    	  //   [out]    Put it here
						 DO_NOT_TRIM);        //   [in]     Remove additional termination chars
	
	   if( !pDataSize || !(*pDataSize)|| !data_size)
	   {
		  roadmap_log( ROADMAP_ERROR, "WST::OnHTTPHeader() - Did not find custom-data size value in the response (%s)", buffer);
		  return trans_failed;      //   Quit reading loop
	   }
	
	   // Update overall expected size of transaction data
	   CB->data_size = CB->read_processed + data_size; 

	   // Log on findings:
	   roadmap_log( ROADMAP_DEBUG, "WST::OnHTTPHeader() - Custom data size: %d; Packet: '%s'", data_size, buffer);
   }else{
	   roadmap_log( ROADMAP_DEBUG, "WST::OnHTTPHeader() - Did not find 'Content-Length:' in response (%s)", buffer);
   }

   (*parser_state) = http_parse_completed;

   return trans_succeeded;      //   Quit loop
}

static transaction_result OnCustomResponse( wst_context_ptr session)
{
   char                 tag[WST_RESPONSE_TAG_MAXSIZE+1];
   cyclic_buffer_ptr    CB                = &(session->CB);
   wst_parser_ptr       parsers           = session->active_item.parsers;
   int                  parsers_count     = session->active_item.parsers_count;
   const char*          next              = NULL;
   const char*          last              = NULL;   //   For logging
   CB_OnWSTResponse     parser            = NULL;
   CB_OnWSTResponse     def_parser        = NULL;
   BOOL                 have_tags         = FALSE;
   BOOL                 more_data_needed  = FALSE;
   int                  buffer_size;
   int                  i;
   roadmap_result		rc						= succeeded;

   assert(session);
   assert(CB);
   assert(parsers);
   assert(parsers_count);

   // Select default parser:
   for( i=0; i<parsers_count; i++)
   {
      if( parsers[i].tag && parsers[i].tag[0])
         have_tags = TRUE;
      else
      {
         def_parser= parsers[i].parser;
         break;
      }
   }
   
   //   As long as we have data - keep on parsing:
   while( CB->read_size > CB->read_processed )
   {
      parser            = NULL;
      more_data_needed  = FALSE;

      //   Set pointer:
      next = cyclic_buffer_get_unprocessed_data( CB);

      //   Test against end-of-string (error in packet)
      if( !(*next))
      {
         session->rc = err_parser_unexpected_data;

         roadmap_log(ROADMAP_ERROR,
                     "WST::OnCustomResponse() - CORRUPTED PACKET: Although remaining custom data size is %d, a NULL was found in the data...",
                     (CB->data_size - (CB->data_processed + CB->read_processed)));
         return trans_failed;   //   Quit the 'receive' loop
      }

      // Save last position:
      last = next;

      //   In order to parse a full statement we must have a full line:
      ///[BOOKMARK]:[NOTE]:[PAZ] - WEBSVC_TRANS - Assuming each command is terminated with '\n'
      if( NULL == strchr( next, '\n'))
         return trans_in_progress;   //   Continue reading...

      if( have_tags)
      {
         //   Read next tag:
         buffer_size = WST_RESPONSE_TAG_MAXSIZE;
         next        = ExtractNetworkString(
                           next,          // [in]     Source string
                           tag,           // [out,opt]Output buffer
                           &buffer_size,  // [in,out] Buffer size / Size of extracted string
                           ",\r\n",       // [in]     Array of chars to terminate the copy operation
                           1);            // [in]     Remove additional termination chars

         next = EatChars( next, "\r\n", TRIM_ALL_CHARS);
         if( !next || !(*next))
         {
            roadmap_log( ROADMAP_ERROR, "WST::OnCustomResponse() - Failed to read server-response tag from packet location '%s'", last);
            return trans_failed;   //   Quit the 'receive' loop
         }

         //   Find parser:
         parser = NULL;
         for( i=0; i<parsers_count; i++)
         {
            #ifdef _WIN32
               if( parsers[i].tag && (0 ==   _stricmp( tag, parsers[i].tag)))
            #else
               if( parsers[i].tag && (0 ==   roadmap_string_compare_ignore_case( tag, parsers[i].tag)))
            #endif
            {
               parser = parsers[i].parser;
               break;
            }
         }
      }

      if( parser)
         // Parser can 'jump' over "tag"
         cyclic_buffer_update_processed_data( CB, next, NULL);
      else
      {
         if( def_parser)
         {
            // "tag" was not found, thus the string "tag" was not used.
            //    Go-back on the stream, and send "tag" as well:
            cyclic_buffer_update_processed_data( CB, last, NULL);
            parser = def_parser;
         }
         else
         {
            session->rc = err_parser_missing_tag_handler;

            roadmap_log( ROADMAP_ERROR, "WST::OnCustomResponse() - Did not find parser for tag '%s'", tag);
            return trans_failed;   //   Quit the 'receive' loop
         }
      }

      //   Activate the appropriate server-request handler function:
      next = cyclic_buffer_get_unprocessed_data( CB);
      next = parser( next, session->active_item.context, &more_data_needed, &rc);
      
      if (session->http_parser_state != http_parse_completed) {
         //current request was removed from queue and replaced by a new request
         return trans_canceled;
      }
      
      if( !next)
      {
         if( succeeded == rc)
         {
            assert(0);  // Client parser failed but did not set 'last error'
            rc = err_failed;
         }
         
         roadmap_log( ROADMAP_ERROR, "WST::OnCustomResponse() - Failed to process server request '%s'; Error: '%s'", tag, roadmap_result_string( rc));
         //[SRUL] Instead of failing the transaction, move on to next line
         next = SkipChars( last, "\r\n", TRIM_ALL_CHARS);
         //return trans_failed;
      }

      if( succeeded == session->rc)
      {
      	session->rc = rc;
      }

      if( more_data_needed)
      {
         roadmap_log( ROADMAP_DEBUG, "WST::OnCustomResponse() - Tag '%s' is asking for more data. Exiting method", tag);
         return trans_in_progress;  // User is asking for more data...
      }

      // Jump over stuff read:
      cyclic_buffer_update_processed_data( CB, next, " ,\t\r\n");
   }


	assert( CB->read_size == CB->read_processed );
    return ((succeeded == session->rc) ? trans_succeeded : trans_failed); 
}

void http_response_status_init( http_response_status* this)
{ memset( this, 0, sizeof(http_response_status));}

transaction_result http_response_status_load(
                              http_response_status*   this,
                              const char*             response,
                              BOOL                    verify_tag,
                              int*                    bytes_read)
{
   int         rc = 0;
   const char* p  = response;
   int         buffer_size;

   http_response_status_init( this);

   //   Expected string:               RC,<int>,<string>\r\n
   //   Minimum size for Expected string:   6

   if( !response || !bytes_read)
      return trans_failed; // Invalid argument

   (*bytes_read) = 0;

   if( !(*p) || (NULL == strchr( p, '\n')))
      return trans_in_progress;   //   Continue reading...

   if( verify_tag)
   {
      if( strncmp( p, "RC,", 3))
         return trans_failed;
      p+=3;   //   Jump over the RC
   }

   //   Get status code:
   p = ReadIntFromString(  p,                //   [in]      Source string
                           ",",              //   [in,opt]  Value termination
                           NULL,             //   [in,opt]  Allowed padding
                           &rc,              //   [out]     Put it here
                           TRIM_ALL_CHARS);  //   [in]      Remove additional termination CHARS

   if( !p || !(*p))
      return trans_failed;

   //   Copy exracted value:
   this->rc = rc;

   //   Copy status string:
   buffer_size = WST_HTTP_STATUS_STRING_MAXSIZE;
   p           = ExtractNetworkString(
                  p,                // [in]     Source string
                  this->text,       // [out,opt]Output buffer
                  &buffer_size,     // [in,out] Buffer size / Size of extracted string
                  "\r\n",           // [in]     Array of chars to terminate the copy operation
                  TRIM_ALL_CHARS);  // [in]     Remove additional termination chars
   if( !p)
      return trans_failed;

   //   Update [out] parameter 'bytes_read':
   if( !(*p))
      (*bytes_read) = (int)strlen( response);
   else
      (*bytes_read) = (int)((size_t)p - (size_t)response);

   return trans_succeeded;
}

LastNetConnectRes websvc_trans_getLastNetConnectRes(void){
	return ELastNetConnectRes;
}

int wst_get_unique_type (void) {
   return (gNextTypeId++);
}
