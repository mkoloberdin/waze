/* roadmap_net.m - Network interface for iPhone
 *
 * LICENSE:
 *
 *   Copyright 2011, Avi R.
 *   Copyright 2011, Waze Ltd
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
 */


#include <stdlib.h>

#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "roadmap_start.h"
#include "roadmap_net.h"
#include "roadmap_http_comp.h"
#include "roadmap_net_mon.h"
#include "roadmap_config.h"
#include "md5.h"
#include "websvc_address.h"
#include "web_date_format.h"
#include "roadmap_ssl.h"

#import <CFNetwork/CFProxySupport.h>

static RoadMapConfigDescriptor RoadMapConfigNetCompressEnabled =
ROADMAP_CONFIG_ITEM( "Network", "Compression Enabled");

static int  RoadMapNetNumConnects;
static BOOL RoadMapNetCompressEnabled = FALSE;


typedef struct roadmap_net_data_t {
   RoadMapNetConnectCallback callback;
   void *context;
   char packet[255];
} RoadMapNetData;

struct roadmap_socket_t {
   CFReadStreamRef readStream;
   CFWriteStreamRef writeStream;
   int is_compressed;
   int is_secured;
   RoadMapHttpCompCtx compress_ctx;
   RoadMapMainIoCb read_callback;
   RoadMapMainIoCb write_callback;
   RoadMapNetData *connect_data;
   
   struct {
      int                        num_retries;
      char                       *protocol;
      char                       *name;
      char                       *resolved_name;
      time_t                     update_time;
      int                        default_port;
      int                        flags;
      RoadMapNetConnectCallback  callback;
      void                       *context;
   } retry_params;
};

#define CONNECT_TIMEOUT_SEC 5


static void check_connect_timeout (void);
static void connect_callback (RoadMapSocket s, roadmap_result res);
static void *roadmap_net_connect_async_internal (const char *protocol, const char *name, const char *resolved_name,
                                                 time_t update_time,
                                                 int default_port,
                                                 int flags,
                                                 RoadMapNetConnectCallback callback,
                                                 void *context,
                                                 int num_retries);





//////////////////////////////////////////////////////////////////////////////////////////////////
static void increment_connections (void) {
   RoadMapNetNumConnects++;
   if (RoadMapNetNumConnects == 1) {
      roadmap_log(ROADMAP_DEBUG, "roadmap_net_connect_async_internal() - first connection, enabling timeout");
      roadmap_main_set_periodic(CONNECT_TIMEOUT_SEC * 1000 /2, check_connect_timeout);
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void decrement_connections (void) {
   RoadMapNetNumConnects--;
   
   if (RoadMapNetNumConnects == 0) {
      roadmap_log(ROADMAP_DEBUG, "connect_callback() - no more connections, cancelling timeout");
      roadmap_main_remove_periodic(check_connect_timeout);
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void check_connect_timeout (void) {
   RoadMapIO *io;
   
   time_t timeout = time(NULL) - CONNECT_TIMEOUT_SEC;
   
   while ((io = roadmap_main_output_timedout(timeout))) {
      RoadMapNetData *data = io->context;
      RoadMapSocket s = io->os.socket;
      RoadMapIO retry_io = *io;
      char *protocol = strdup(retry_io.retry_params.protocol);
      char *name = strdup(retry_io.retry_params.name);
      char *resolved_name = strdup(retry_io.retry_params.resolved_name);
      
      if (retry_io.retry_params.protocol && retry_io.retry_params.protocol[0]) {
         free(retry_io.retry_params.protocol);
      }
      if (retry_io.retry_params.name && retry_io.retry_params.name[0]) {
         free(retry_io.retry_params.name);
      }
      if (retry_io.retry_params.resolved_name && retry_io.retry_params.resolved_name[0]) {
         free(retry_io.retry_params.resolved_name);
      }
      
      roadmap_log(ROADMAP_ERROR, "Connect time out");
      roadmap_main_remove_input(io);
      roadmap_net_close(s);      
      
      if (retry_io.retry_params.num_retries < 2) {
         decrement_connections();
         retry_io.retry_params.num_retries++;
         roadmap_log(ROADMAP_ERROR, "Retrying connection (retry # %d)", retry_io.retry_params.num_retries);
         if (NULL == roadmap_net_connect_async_internal (protocol, name, resolved_name,
                                                       retry_io.retry_params.update_time,
                                                       retry_io.retry_params.default_port,
                                                       retry_io.retry_params.flags,
                                                       retry_io.retry_params.callback,
                                                       retry_io.retry_params.context,
                                                       retry_io.retry_params.num_retries)) {
            free(protocol);
            free(name);
            free(resolved_name);
            free(data);
            continue;
         }
      }
      
      free(protocol);
      free(name);
      free(resolved_name);
      connect_callback(s, err_net_failed);
   }
}



//////////////////////////////////////////////////////////////////////////////////////////////////
RoadMapSocket roadmap_net_connect (const char *protocol, const char *name,
                                   time_t update_time,
                                   int default_port,
                                   int flags,
                                   roadmap_result* err) {
   roadmap_log(ROADMAP_ERROR, "roadmap_net_connect() sync connection not supported !!!");
   return NULL;
}
   

//////////////////////////////////////////////////////////////////////////////////////////////////
int roadmap_net_receive (RoadMapSocket s, void *data, int size) {
   int total_received = 0, received;
   void *ctx_buffer;
   int ctx_buffer_size;
   
   if (s->is_compressed) {
      
      if (!s->compress_ctx) {
         s->compress_ctx = roadmap_http_comp_init();
         if (s->compress_ctx == NULL) return -1;
      }
      
      roadmap_http_comp_get_buffer(s->compress_ctx, &ctx_buffer, &ctx_buffer_size);
      
      received = CFReadStreamRead (s->readStream, ctx_buffer, ctx_buffer_size);
      
      roadmap_http_comp_add_data(s->compress_ctx, received);
      
      while ((received = roadmap_http_comp_read(s->compress_ctx, data + total_received, size - total_received))
             != 0) {
         if (received < 0) {
            roadmap_net_mon_error("Error in recv.");
            roadmap_log (ROADMAP_DEBUG, "Error in recv. - comp returned %d", received);
            return -1;
         }
         
         total_received += received;
      }
   } else {
      total_received = CFReadStreamRead (s->readStream, data, size);
   }
   
   if (total_received < 0) {
      roadmap_net_mon_error("Error in recv.");
      roadmap_log (ROADMAP_DEBUG, "Error in recv., errno = %d", errno);
      return -1;
   }
   
   roadmap_net_mon_recv(total_received);
   
   return total_received;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
int roadmap_net_send (RoadMapSocket s, const void *data, int length, int wait) {
   int total = length;
   
   while (length > 0) {
      int res = CFWriteStreamWrite(s->writeStream, data, length);
      
      if (res < 0) {
         CFErrorRef err = CFWriteStreamCopyError(s->writeStream);
         if (err) {
            CFStringRef str = CFErrorCopyDescription(err);
            char buf[1024];
            CFStringGetCString(str, buf, 1024, kCFStringEncodingUTF8);
            roadmap_log(ROADMAP_ERROR, "Error writing to stream: '%s'", buf);
            CFRelease(str);
            CFRelease(err);
            
            roadmap_net_mon_error("Error in send - data.");
            return -1;
         }
      }
      
      length -= res;
      data = (char *)data + res;
      
      roadmap_net_mon_send(res);
   }

   return total;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
int roadmap_net_send_async (RoadMapSocket s, const void *data, int length) {
   int res = CFWriteStreamWrite(s->writeStream, data, length);
   
   if (res < 0) {
      CFErrorRef err = CFWriteStreamCopyError(s->writeStream);
      if (err) {
         CFStringRef str = CFErrorCopyDescription(err);
         char buf[1024];
         CFStringGetCString(str, buf, 1024, kCFStringEncodingUTF8);
         roadmap_log(ROADMAP_ERROR, "Error writing to stream: '%s'", buf);
         CFRelease(str);
         CFRelease(err);
         
         roadmap_net_mon_error("Error in send - data.");
         return -1;
      }
   }
   
   roadmap_net_mon_send(res);
   
   return res;
}

static void connect_callback (RoadMapSocket s, roadmap_result res) {
   RoadMapNetData *data = s->connect_data;
   RoadMapNetConnectCallback callback = data->callback;
   void *context = data->context;
   
   if (!s->connect_data)
      return;
   
   decrement_connections();
   
   if ((res == succeeded) && *data->packet) {
         if( -1 == roadmap_net_send(s, data->packet,
                                    (int)strlen(data->packet), 1)) {
            roadmap_log( ROADMAP_ERROR, "roadmap_net callback (HTTP) - Failed to send the 'POST' packet");
            roadmap_net_close(s);
            res = err_net_failed;
         }
   }
   
   free(data);
   s->connect_data = NULL;
   
   if (res != succeeded)
      (*callback) (ROADMAP_INVALID_SOCKET, context, res);
   else
      (*callback) (s, context, res);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void read_stream_client_cb (CFReadStreamRef stream,
                                   CFStreamEventType eventType,
                                   void *clientCallBackInfo)
{
   RoadMapSocket socket = clientCallBackInfo;
   
   if (eventType & kCFStreamEventOpenCompleted) {
      //NSLog(@"read_stream_client_cb: kCFStreamEventOpenCompleted");
   }
   if (eventType & kCFStreamEventErrorOccurred) {
      CFErrorRef err = CFReadStreamCopyError(stream);
      if (err) {
         CFStringRef str = CFErrorCopyDescription(err);
         char buf[1024];
         
         if (str) {
            CFStringGetCString(str, buf, 1024, kCFStringEncodingUTF8);
            roadmap_log(ROADMAP_ERROR, "read_stream_client_cb: kCFStreamEventErrorOccurred description: '%s'", buf);
            CFRelease(str);
         }
         
         str = CFErrorCopyFailureReason (err);
         if (str) {
            CFStringGetCString(str, buf, 1024, kCFStringEncodingUTF8);
            roadmap_log(ROADMAP_ERROR, "read_stream_client_cb: kCFStreamEventErrorOccurred reason: '%s'", buf);
            CFRelease(str);
         }
         
         str = CFErrorCopyRecoverySuggestion (err);
         if (str) {
            CFStringGetCString(str, buf, 1024, kCFStringEncodingUTF8);
            roadmap_log(ROADMAP_ERROR, "read_stream_client_cb: kCFStreamEventErrorOccurred recovery: '%s'", buf);
            CFRelease(str);
         }
         
         CFRelease(err);
      } else {
         roadmap_log(ROADMAP_ERROR, "read_stream_client_cb: kCFStreamEventErrorOccurred but NO ERROR CODE");
      }


      if (socket->connect_data)
         connect_callback(socket, err_net_failed);
      else if (socket->read_callback)
         socket->read_callback(socket);
      
      return;
   }
   
   if (eventType & kCFStreamEventEndEncountered) {
      //NSLog(@"read_stream_client_cb: kCFStreamEventEndEncountered");
      roadmap_log(ROADMAP_WARNING, "read_stream_client_cb: kCFStreamEventEndEncountered");
      return;
   }
   
   if (eventType & kCFStreamEventHasBytesAvailable) {
      //NSLog(@"read_stream_client_cb: kCFStreamEventHasBytesAvailable");

      
      if (!socket->read_callback) {
         roadmap_log(ROADMAP_WARNING, "Stream received bytes but no receive cb is set");
      } else {
         if (socket->read_callback)
            socket->read_callback(socket);
      }
      
      return;
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void write_stream_client_cb (CFWriteStreamRef stream,
                                    CFStreamEventType eventType,
                                    void *clientCallBackInfo)
{
    RoadMapSocket socket = clientCallBackInfo;
   
   //printf("write_stream_client_cb status: %d\n", CFReadStreamGetStatus(stream));
   if (eventType & kCFStreamEventOpenCompleted) {
      //NSLog(@"write_stream_client_cb: kCFStreamEventOpenCompleted");
      return;
   }
   if (eventType & kCFStreamEventErrorOccurred) {
      CFErrorRef err = CFWriteStreamCopyError(stream);
      if (err) {
         CFStringRef str = CFErrorCopyDescription(err);
         char buf[1024];
         
         if (str) {
            CFStringGetCString(str, buf, 1024, kCFStringEncodingUTF8);
            roadmap_log(ROADMAP_ERROR, "write_stream_client_cb: kCFStreamEventErrorOccurred description: '%s'", buf);
            CFRelease(str);
         }
         
         str = CFErrorCopyFailureReason (err);
         if (str) {
            CFStringGetCString(str, buf, 1024, kCFStringEncodingUTF8);
            roadmap_log(ROADMAP_ERROR, "write_stream_client_cb: kCFStreamEventErrorOccurred reason: '%s'", buf);
            CFRelease(str);
         }
         
         str = CFErrorCopyRecoverySuggestion (err);
         if (str) {
            CFStringGetCString(str, buf, 1024, kCFStringEncodingUTF8);
            roadmap_log(ROADMAP_ERROR, "write_stream_client_cb: kCFStreamEventErrorOccurred recovery: '%s'", buf);
            CFRelease(str);
         }
         
         CFRelease(err);
      } else {
         roadmap_log(ROADMAP_ERROR, "write_stream_client_cb: kCFStreamEventErrorOccurred but NO ERROR CODE");
      }
      
      if (socket->connect_data)
         connect_callback(socket, err_net_failed);
      
      return;
   }
   
   if (eventType & kCFStreamEventEndEncountered) {
      //NSLog(@"write_stream_client_cb: kCFStreamEventEndEncountered");
      roadmap_log(ROADMAP_WARNING, "write_stream_client_cb: kCFStreamEventEndEncountered");
      return;
   }
   
   if (eventType & kCFStreamEventCanAcceptBytes) {
      //NSLog(@"write_stream_client_cb: kCFStreamEventCanAcceptBytes");
      
      if (!socket->write_callback) {
         CFWriteStreamSetClient(stream, kCFStreamEventNone, NULL, NULL);
         CFWriteStreamUnscheduleFromRunLoop(socket->writeStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
         connect_callback(socket, succeeded);
      } else {
         socket->write_callback(socket);
      }
      
      return;
   }
}


//////////////////////////////////////////////////////////////////////////////////////////////////

void roadmap_net_close (RoadMapSocket s) {
   roadmap_net_mon_disconnect();
   CFWriteStreamSetClient(s->writeStream, kCFStreamEventNone, NULL, NULL);
   CFWriteStreamUnscheduleFromRunLoop(s->writeStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
   CFWriteStreamClose (s->writeStream);
   CFRelease(s->writeStream);
   
   CFReadStreamSetClient(s->readStream, kCFStreamEventNone, NULL, NULL);
   CFReadStreamUnscheduleFromRunLoop(s->readStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
   CFReadStreamClose (s->readStream);
   CFRelease(s->readStream);
   
   if (s->compress_ctx) roadmap_http_comp_close(s->compress_ctx);
   
   free(s);
}


//////////////////////////////////////////////////////////////////////////////////////////////////
static int open_stream (RoadMapSocket socket) {
   CFReadStreamRef readStream = socket->readStream;
   CFWriteStreamRef writeStream = socket->writeStream;
      
   //Open read stream
   NSInputStream *inputStream = (NSInputStream *)readStream;
   if (socket->is_secured)
      [inputStream setProperty:NSStreamSocketSecurityLevelNegotiatedSSL forKey:NSStreamSocketSecurityLevelKey];
   
   if (!CFReadStreamOpen(readStream)) {
      roadmap_log(ROADMAP_ERROR, "CFReadStreamOpen() failed");
      return 0;
   }
   
   //Open write stream
   NSOutputStream *outputStream = (NSOutputStream *)writeStream;
   if (socket->is_secured)
      [outputStream setProperty:NSStreamSocketSecurityLevelNegotiatedSSL forKey:NSStreamSocketSecurityLevelKey];
   
   if (!CFWriteStreamOpen(writeStream)) {
      roadmap_log(ROADMAP_ERROR, "CFWriteStreamOpen() failed");
      return 0;
   }
   
   return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static RoadMapSocket create_stream (const char *name,
                                    int default_port) {
   RoadMapSocket socket;
   CFReadStreamRef readStream;
   CFWriteStreamRef writeStream;
   char *hostname;
   char *separator = strchr (name, ':');
   UInt32 port = 0;
   CFStringRef cf_hostname = NULL;
   
   
   hostname = strdup(name);
   roadmap_check_allocated(hostname);
   
   if (separator != NULL) {
      if (isdigit(separator[1])) {
         
         port = atoi(separator+1);
         
         if (port == 0) {
            roadmap_log (ROADMAP_ERROR, "invalid port in '%s', will use default port (%d)", name, default_port);
         }
      }
      
      *(strchr(hostname, ':')) = 0;
   }
   
   if (port == 0 && default_port <= 0) {
      goto connection_failure;
   } else if (port == 0) {
      port = default_port;
   }

   cf_hostname = CFStringCreateWithCString(NULL, hostname, kCFStringEncodingUTF8);
   
   CFStreamCreatePairWithSocketToHost(NULL, cf_hostname, port, &readStream, &writeStream);
   CFRelease(cf_hostname);
   
   if (!readStream || !writeStream) {
      roadmap_log(ROADMAP_ERROR, "CFStreamCreatePairWithSocket() failed");
      goto connection_failure;
   }
   
   
   
   socket = malloc(sizeof (struct roadmap_socket_t));
   socket->readStream = readStream;
   socket->writeStream = writeStream;
   socket->read_callback = NULL;
   socket->write_callback = NULL;
   CFStreamClientContext context = {0, socket, NULL, NULL, NULL};
   CFReadStreamSetClient(readStream,
                         /*kCFStreamEventHasBytesAvailable | */kCFStreamEventErrorOccurred | kCFStreamEventEndEncountered
                         |kCFStreamEventOpenCompleted,
                         read_stream_client_cb,
                         &context);
   CFReadStreamScheduleWithRunLoop(readStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
   CFWriteStreamSetClient(writeStream,
                          kCFStreamEventCanAcceptBytes | kCFStreamEventErrorOccurred | kCFStreamEventEndEncountered
                          |kCFStreamEventOpenCompleted,
                          write_stream_client_cb,
                          &context);
   CFWriteStreamScheduleWithRunLoop(writeStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
   
   free(hostname);
      
   return socket;
   
   
connection_failure:
   
   free(hostname);
   return ROADMAP_INVALID_SOCKET;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void *roadmap_net_connect_async_internal (const char *protocol, const char *name, const char *resolved_name,
                                        time_t update_time,
                                        int default_port,
                                        int flags,
                                        RoadMapNetConnectCallback callback,
                                        void *context,
                                        int num_retries) {
   
   char           packet[512];
   RoadMapNetData *data = NULL;
   const char     *req_type = "GET";
   BOOL           is_using_proxy = FALSE;
   char           update_since[WDF_MODIFIED_HEADER_SIZE + 1];
   char            server_url  [ WSA_SERVER_URL_MAXSIZE   + 1];
   char            resolved_server_url  [ WSA_SERVER_URL_MAXSIZE   + 1];
   char            service_name[ WSA_SERVICE_NAME_MAXSIZE + 1];
   int             server_port;
   RoadMapSocket  socket;
   
   
   if( strncmp( protocol, "http", 4) != 0) {
      roadmap_log(ROADMAP_ERROR, "Protocol not supported '%s'", protocol);
      return NULL;
   }
   
   data = (RoadMapNetData *)malloc(sizeof(RoadMapNetData)); //TODO: free in case of errors
   

   WDF_FormatHttpIfModifiedSince (update_time, update_since);
   if (!strcmp(protocol, "http_post")) req_type = "POST";
   
   if( !WSA_ExtractParams( resolved_name,          //   IN        -   Web service full address (http://...)
                          resolved_server_url,    //   OUT,OPT   -   Server URL[:Port]
                          &server_port,  //   OUT,OPT   -   Server Port
                          service_name)) //   OUT,OPT   -   Web service name
   {
      roadmap_log( ROADMAP_ERROR, "roadmap_net_connect_async(HTTP) - Failed to extract information from '%s'", resolved_name);
      return ROADMAP_INVALID_SOCKET;
   }
   
   if( !WSA_ExtractParams( name,          //   IN        -   Web service full address (http://...)
                          server_url,    //   OUT,OPT   -   Server URL[:Port]
                          &server_port,  //   OUT,OPT   -   Server Port
                          service_name)) //   OUT,OPT   -   Web service name
   {
      roadmap_log( ROADMAP_ERROR, "roadmap_net_connect_async(HTTP) - Failed to extract information from '%s'", name);
      return ROADMAP_INVALID_SOCKET;
   }
   
   if (is_using_proxy) { //TODO:
      sprintf(packet,
              "%s %s HTTP/1.0\r\n"
              "Host: %s\r\n"
              "User-Agent: FreeMap/%s\r\n"
              "%s"
              "%s",
              req_type, name, server_url, roadmap_start_version(),
              TEST_NET_COMPRESS( flags ) ? "Accept-Encoding: gzip, deflate\r\n" : "",
              update_since); 
   } else {
      sprintf(packet,
              "%s %s HTTP/1.0\r\n"
              "Host: %s\r\n"
              "User-Agent: FreeMap/%s\r\n"
              "%s"
              "%s",
              req_type, service_name, server_url, roadmap_start_version(),
              TEST_NET_COMPRESS( flags ) ? "Accept-Encoding: gzip, deflate\r\n" : "",
              update_since);
      
   }
   
   socket = create_stream(server_url, default_port);
   if (socket == ROADMAP_INVALID_SOCKET) {
      return ROADMAP_INVALID_SOCKET;
   }
   
   socket->is_compressed = TEST_NET_COMPRESS( flags );
   socket->compress_ctx = NULL;
   socket->is_secured = (server_port == 443); //TODO: consider different check
   
   strncpy_safe(data->packet, packet, sizeof(data->packet));
   data->callback = callback;
   data->context = context;
   socket->connect_data = data;
   
   socket->retry_params.num_retries = num_retries;
   socket->retry_params.protocol = strdup(protocol);
   socket->retry_params.name = strdup(name);
   socket->retry_params.resolved_name = strdup(resolved_name);
   socket->retry_params.update_time = update_time;
   socket->retry_params.default_port = default_port;
   socket->retry_params.flags = flags;
   socket->retry_params.callback = callback;
   socket->retry_params.context = context;
 
   
   if (open_stream(socket)) {
      roadmap_net_mon_connect ();
      increment_connections();
      
      return socket;
   }
   
   roadmap_net_mon_error("Error connecting.");
   return ROADMAP_INVALID_SOCKET;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void *roadmap_net_connect_async (const char *protocol, const char *name, const char *resolved_name,
                               time_t update_time,
                               int default_port,
                               int flags,
                               RoadMapNetConnectCallback callback,
                               void *context) {
   return roadmap_net_connect_async_internal (protocol, name, resolved_name, update_time, default_port, flags, callback, context, 0);
}

void roadmap_net_cancel_connect (RoadMapSocket socket) {
   RoadMapNetData *data = socket->connect_data;
   
   if (socket->retry_params.protocol && socket->retry_params.protocol[0]) {
      free(socket->retry_params.protocol);
   }
   if (socket->retry_params.name && socket->retry_params.name[0]) {
      free(socket->retry_params.name);
   }
   if (socket->retry_params.resolved_name && socket->retry_params.resolved_name[0]) {
      free(socket->retry_params.resolved_name);
   }
   
   roadmap_log(ROADMAP_DEBUG, "Cancelling async connect request");
   roadmap_net_close(socket);
   free(data);
   
   decrement_connections();
}


void roadmap_net_set_compress_enabled( BOOL value )
{
   RoadMapNetCompressEnabled = value;
}

BOOL roadmap_net_get_compress_enabled( void )
{
   return RoadMapNetCompressEnabled;
}


void roadmap_net_shutdown (void) {
   
   const char* netcompress_cfg_value = RoadMapNetCompressEnabled ? "yes" : "no";
   roadmap_config_set( &RoadMapConfigNetCompressEnabled, netcompress_cfg_value );
   roadmap_net_mon_destroy();
}

void roadmap_net_initialize (void) {
   roadmap_config_declare
   ( "user", &RoadMapConfigNetCompressEnabled, "no", NULL );
   RoadMapNetCompressEnabled = roadmap_config_match( &RoadMapConfigNetCompressEnabled, "yes" );
   
   roadmap_net_mon_start ();
}

int roadmap_net_socket_secured (RoadMapSocket s) {
   return s->is_secured;
}

int roadmap_net_unique_id (unsigned char *buffer, unsigned int size) {
   struct MD5Context context;
   unsigned char digest[16];
   time_t tm;
   
   time(&tm);
   
   MD5Init (&context);
   MD5Update (&context, (unsigned char *)&tm, sizeof(tm));
   MD5Final (digest, &context);
   
   if (size > sizeof(digest)) size = sizeof(digest);
   memcpy(buffer, digest, size);
   
   return size;
}

RoadMapSocket roadmap_net_listen(int port) {
   
   return ROADMAP_INVALID_SOCKET; // Not yet implemented.
}


RoadMapSocket roadmap_net_accept(RoadMapSocket server_socket) {
   
   return ROADMAP_INVALID_SOCKET; // Not yet implemented.
}

int roadmap_net_get_fd(RoadMapSocket s) {
   return -1;
}

void roadmap_net_register_output (RoadMapSocket socket, RoadMapMainIoCb callback) {
   CFStreamClientContext context = {0, socket, NULL, NULL, NULL};
   
   socket->write_callback = callback;
   CFWriteStreamSetClient(socket->writeStream,
                          kCFStreamEventCanAcceptBytes | kCFStreamEventErrorOccurred | kCFStreamEventEndEncountered
                          |kCFStreamEventOpenCompleted,
                          write_stream_client_cb,
                          &context);
   CFWriteStreamScheduleWithRunLoop(socket->writeStream, CFRunLoopGetCurrent(), kCFRunLoopCommonModes);
}

void roadmap_net_register_input (RoadMapSocket socket, RoadMapMainIoCb callback) {
   CFStreamClientContext context = {0, socket, NULL, NULL, NULL};

   socket->read_callback = callback;
   CFReadStreamSetClient(socket->readStream,
                         kCFStreamEventHasBytesAvailable | kCFStreamEventErrorOccurred | kCFStreamEventEndEncountered
                         |kCFStreamEventOpenCompleted,
                         read_stream_client_cb,
                         &context);
}

void roadmap_net_unregister_input (RoadMapSocket socket) {
   if (socket->read_callback) {
      CFStreamClientContext context = {0, socket, NULL, NULL, NULL};
      
      socket->read_callback = NULL;
      CFReadStreamSetClient(socket->readStream,
                            kCFStreamEventErrorOccurred | kCFStreamEventEndEncountered
                            |kCFStreamEventOpenCompleted,
                            read_stream_client_cb,
                            &context);
   }
}

void roadmap_net_unregister_output (RoadMapSocket socket) {
   if (socket->write_callback) {
      CFStreamClientContext context = {0, socket, NULL, NULL, NULL};
      
      socket->write_callback = NULL;
      CFWriteStreamSetClient(socket->writeStream,
                             kCFStreamEventErrorOccurred | kCFStreamEventEndEncountered
                             |kCFStreamEventOpenCompleted,
                             write_stream_client_cb,
                             &context);
   }
}
