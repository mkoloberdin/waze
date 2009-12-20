/*
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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#ifndef WIN32
#include "../roadmap_string.h"
#endif

#include "../roadmap_net.h"
#include "socket_async_receive.h"

#include "websvc_trans.h"

static   void  on_socket_connected  ( RoadMapSocket Socket, void* context, roadmap_result res);
static   void  on_data_received     ( void* data, int size, void* context);
static   int   wst_Send             ( RoadMapSocket socket, const char* szData);
static   BOOL  wst_Receive          ( wst_context_ptr session);

static   transaction_result
               OnHTTPHeader         ( cyclic_buffer_ptr CB, http_parsing_state* parser_state);
static   transaction_result
               OnCustomResponse     ( wst_context_ptr session);

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
   7  this->starting_time        = 0;              */
/* 8*/wstq_init(                 &(this->queue));  /*
   9  this->context              = NULL;           */

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
void wst_context_reset( wst_context_ptr this)
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
/* 7*/   this->starting_time        = 0;
// 8     this->queue    !NOT MODIFYING QUEUE!
/* 9*/   this->context              = NULL;

/* Send:          */
/*10*/   ebuffer_free(              &(this->packet));

/* Receive:       */
/*11*/   cyclic_buffer_init(        &(this->CB));
/*12*/   this->http_parser_state    = http_not_parsed;
/*13*/   this->parsers              = NULL;
/*14*/   this->parsers_count        = 0;

/* Completion:    */
/*15*/   this->cbOnWSTCompleted     = NULL;
/*16*/   this->result               = trans_succeeded;
/*17*/   this->rc                   = succeeded;

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
      roadmap_log( ROADMAP_ERROR, "wst_Receive( SOCKET: %d) - 'socket_async_receive()' had failed", session->Socket);
      return FALSE;
   }

   session->async_receive_started = TRUE;
   return TRUE;
}

void  wst_context_load( wst_context_ptr         this,
                        const wst_parser_ptr    parsers,
                        int                     parsers_count,
                        CB_OnWSTCompleted       cbOnCompleted,
                        void*                   context)
{
   wst_context_reset( this);

   this->parsers           = parsers;
   this->parsers_count     = parsers_count;
   this->cbOnWSTCompleted  = cbOnCompleted;
   this->context           = context;
   this->state             = trans_active;
}

wst_handle wst_init( const char* service_name,  // e.g. - rtserver
                     const char* content_type)  // e.g. - binary/octet-stream
{
   int               server_port;
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

         wst_stop_trans( h);
         session->delete_on_idle = TRUE;
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
                           Item.action,
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
   CB_OnWSTCompleted cb       = session->cbOnWSTCompleted;
   void*             context  = session->context;

   if( session->delete_on_idle)
   {
      wst_context_free( session);
      free( session);

      return;  // Do we want to call callback? (no)
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

   wst_context_reset( session);

   if(cb)
      cb( context, res);
}

void wst_watchdog( wst_handle h)
{
   wst_context_ptr   session        = (wst_context_ptr)h;
   time_t            now            = time(NULL);
   int               seconds_passed = (int)(now - session->starting_time);

   if( !session->starting_time || (seconds_passed < WST_SESSION_TIMEOUT))
      return;

   roadmap_log( ROADMAP_ERROR, "wst_watchdog() - TERMINATING SESSION !!! - Session is running already %d seconds", seconds_passed);

   wst_transaction_completed( session, err_timed_out);
}

BOOL wstq_Add( wst_context_ptr      session,
               const char*          action,
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
   TQI.parsers       = parsers;
   TQI.parsers_count = parsers_count;
   TQI.cbOnCompleted = cbOnCompleted;
   TQI.context       = context;
   TQI.packet        = strdup(packet);

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
BOOL wst_start_trans__int(
            wst_context_ptr      session,
            const char*          action,
            const wst_parser_ptr parsers,
            int                  parsers_count,
            CB_OnWSTCompleted    cbOnCompleted,
            void*                context,
            char*                packet)
{
   char* AsyncPacket    = NULL;
   int   AsyncPacketSize= 0;
   char  WebServiceMethod[WST_WEBSERVICE_METHOD_MAX_SIZE];

   if(!session || !action        || !(*action)     ||
      !parsers || !parsers_count || !cbOnCompleted ||
      !packet  || !(*packet))
   {
      assert(0);  // Invalid arguments
      return FALSE;
   }

   if( trans_idle != session->state)
      return wstq_Add(  session,       action,  parsers, parsers_count,
                        cbOnCompleted, context, packet);

   if( ROADMAP_INVALID_SOCKET != session->Socket)
   {
      assert(0);  // No session is active - Socket should be invalid..
      return FALSE;
   }

   //    Inital transaction context:
   wst_context_load( session, parsers, parsers_count, cbOnCompleted, context);

   // Allocate buffer for the async-info object:
   AsyncPacketSize= HTTP_HEADER_MAX_SIZE + strlen(packet) + 10;
   AsyncPacket    = ebuffer_alloc( &(session->packet), AsyncPacketSize);

   snprintf(WebServiceMethod,
            WST_WEBSERVICE_METHOD_MAX_SIZE,
            "%s/%s", session->service, action);

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
   if( -1 == roadmap_net_connect_async("http_post",
                                       WebServiceMethod,
                                       0,
                                       session->port,
                                       on_socket_connected,
                                       session))
   {
      roadmap_log( ROADMAP_ERROR, "wst_start_trans() - 'roadmap_net_connect_async' had failed (Invalid params or queue is full?)");
      wst_context_reset( session);
      return FALSE;
   }

   // Mark starting time:
   session->starting_time = time(NULL);

   return TRUE;
}

transaction_state wst_get_trans_state( wst_handle h)
{
   wst_context_ptr   session = (wst_context_ptr)h;

   assert(session);

   return session->state;
}

void wst_stop_trans( wst_handle h)
{
   wst_context_ptr   session = (wst_context_ptr)h;

   assert(session);

   if( trans_active == session->state)
      session->state = trans_stopping;
}

BOOL wst_start_trans(wst_handle           h,             // Session object
                     const char*          action,        // (/<service_name>/)<ACTION>
                     const wst_parser_ptr parsers,       // Array of 1..n data parsers
                     int                  parsers_count, // Parsers count
                     CB_OnWSTCompleted    cbOnCompleted, // Callback for transaction completion
                     void*                context,       // Caller context
                     const char*          szFormat,      // Custom data for the HTTP request
                     ...)                                // Parameters
{
   wst_context_ptr   session = (wst_context_ptr)h;
   va_list           vl;
   int               i;
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
   i = vsnprintf( Data, SizeNeeded, szFormat, vl);
   va_end(vl);

   if( i < 0)
   {
      roadmap_log( ROADMAP_ERROR, "wst_start_trans() - Failed to format command '%s' (buffer size too small?)", szFormat);
      ebuffer_free( &Packet);
      return FALSE;
   }

   bRes = wst_start_trans__int(  session,       // Session object
                                 action,        // /<service_name>/<action>
                                 parsers,       // Array of 1..n data parsers
                                 parsers_count, // Parsers count
                                 cbOnCompleted, // Callback for transaction completion
                                 context,       // Caller context
                                 Data);         // Custom data for the HTTP request

   ebuffer_free( &Packet);

   return bRes;
}

transaction_result on_socket_connected_(  RoadMapSocket     Socket,
                                          wst_context_ptr   session,
                                          roadmap_result    res)
{
   const char* packet;

   session->rc = res;

   // Where we asked to abort transaction?
   if( trans_stopping == session->state)
   {
      session->rc = err_aborted;
      roadmap_log( ROADMAP_ERROR, "on_socket_connected() - Was asked to abort session");
      return trans_failed;
   }

   // Verify Socket is valid:
   if( ROADMAP_INVALID_SOCKET == Socket)
   {
      assert( succeeded != res);

      roadmap_log( ROADMAP_ERROR, "on_socket_connected() - INVALID SOCKET - Failed to create Socket; Error '%s'", roadmap_result_string( res));
      return trans_failed;
   }

   assert( succeeded == res);

   packet         = ebuffer_get_buffer( &(session->packet));
   session->Socket= Socket;

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

   // Where we asked to abort transaction?
   if( trans_stopping == session->state)
   {
      session->rc = err_aborted;
      roadmap_log( ROADMAP_ERROR, "on_data_received() - Was asked to abort session");
      return trans_failed;
   }

   if( size < 1)
   {
      roadmap_log( ROADMAP_ERROR, "on_data_received( SOCKET: %d) - 'roadmap_net_receive()' returned %d bytes", session->Socket, size);
      return trans_failed;
   }

   if( !data)
   {
      roadmap_log( ROADMAP_ERROR, "on_data_received( SOCKET: %d) - Invalid data (NULL)", session->Socket);
      return trans_failed;
   }

   roadmap_log( ROADMAP_DEBUG, "on_data_received( SOCKET: %d) - Received %d bytes", session->Socket, size);

   CB                = &(session->CB);
   http_parser_state = session->http_parser_state;
   res               = trans_failed;   //   Default


   //   Terminate data with NULL, so it can be processed as string:
   CB->read_size += size;
   CB->buffer[ CB->read_size] = '\0';

   //   Handle data:
   //   1.   Handle HTTP headers:
   if( http_parse_completed != http_parser_state)
   {
      //   Http data was not processed yet; Use HTTP handler:
      res = OnHTTPHeader( CB, &http_parser_state);

      //   Done?
      if( trans_succeeded == res)
         session->http_parser_state = http_parse_completed;
   }

   //   2.   Handle custom data:
   if( http_parse_completed == http_parser_state)
      res = OnCustomResponse( session);

   if( trans_in_progress != res)
   {
      roadmap_log(ROADMAP_DEBUG,
                  "on_data_received() - Finish to process all data; Status: %s",
                  ((trans_succeeded==res)? "Succeeded": "Failed"));

      if( (trans_succeeded != res) && (succeeded == session->rc))
         session->rc = err_failed;

      return res;
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
      roadmap_log( ROADMAP_ERROR, "on_data_received( SOCKET: %d) - 'socket_async_receive()' had failed", session->Socket);
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
      roadmap_log( ROADMAP_ERROR, "wst_Send( SOCKET: %d) - 'roadmap_net_send()' returned -1", socket);
   else
      roadmap_log( ROADMAP_DEBUG, "wst_Send( SOCKET: %d) - Sent %d bytes", socket, strlen(szData));

   return iRes;
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

   // Lower case:
   ToLowerN( (char*)buffer, (size_t)(pHeaderEnd - (char*)buffer));

   //   Search for content length:
   pDataSize = strstr( buffer, "content-length:");
   if( !pDataSize)
   {
      roadmap_log( ROADMAP_ERROR, "WST::OnHTTPHeader() - Did not find 'Content-Length:' in response (%s)", buffer);
      return trans_failed;         //   Quit reading loop
   }

   //   Move pointer:
   pDataSize += strlen("Content-Length:");
   //   Read size:
   pDataSize  = ReadIntFromString(
                     pDataSize,           //   [in]     Source string
                     "\r\n",              //   [in,opt] Value termination
                     " ",                 //   [in,opt] Allowed padding
                     &data_size,    //   [out]    Put it here
                     DO_NOT_TRIM);        //   [in]     Remove additional termination chars

   if( !pDataSize || !(*pDataSize)|| !data_size)
   {
      roadmap_log( ROADMAP_ERROR, "WST::OnHTTPHeader() - Did not find custom-data size value in the response (%s)", buffer);
      return trans_failed;      //   Quit reading loop
   }

   // Log on findings:
   roadmap_log( ROADMAP_DEBUG, "WST::OnHTTPHeader() - Custom data size: %d; Packet: '%s'", data_size, buffer);

   // Update buffer processed-size:
   CB->read_processed   = (int)(((size_t)pHeaderEnd  + strlen("\r\n\r\n")) - (size_t)(buffer));

   // Set boundreis for the following data processing:
   CB->data_size        = CB->read_processed + data_size;// Overall expected size of all data
   CB->data_processed   = 0;                             // Out of 'data_size', how much was processed

   (*parser_state) = http_parse_completed;

   return trans_succeeded;      //   Quit loop
}

static transaction_result OnCustomResponse( wst_context_ptr session)
{
   char                 tag[WST_RESPONSE_TAG_MAXSIZE+1];
   cyclic_buffer_ptr    CB                = &(session->CB);
   wst_parser_ptr       parsers           = session->parsers;
   int                  parsers_count     = session->parsers_count;
   const char*          next              = NULL;
   const char*          last              = NULL;   //   For logging
   CB_OnWSTResponse     parser            = NULL;
   CB_OnWSTResponse     def_parser        = NULL;
   BOOL                 have_tags         = FALSE;
   BOOL                 more_data_needed  = FALSE;
   int                  buffer_size;
   int                  i;
   roadmap_result			rc						= succeeded;

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
   while( (CB->data_processed + CB->read_processed) < CB->data_size)
   {
      parser            = NULL;
      more_data_needed  = FALSE;

      //   Do we need to read more bytes?
      if( CB->read_size <= CB->read_processed)
      {
         assert(CB->read_size == CB->read_processed);
         return trans_in_progress;   //   Continue reading...
      }

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
      next = parser( next, session->context, &more_data_needed, &rc);

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

	if( succeeded != session->rc)
		return trans_failed;
		 
   return trans_succeeded;
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

