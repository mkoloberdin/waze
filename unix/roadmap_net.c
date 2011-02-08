/* roadmap_net.c - Network interface for the RoadMap application.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 * SYNOPSYS:
 *
 *   See roadmap_net.h
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "roadmap.h"
#include "roadmap_start.h"
#include "roadmap_config.h"
#include "md5.h"
#include "roadmap_net_mon.h"
#include "roadmap_http_comp.h"
#include "roadmap_net.h"
#include "../websvc_trans/websvc_address.h"
#include "../websvc_trans/web_date_format.h"
#include "roadmap_main.h"

typedef struct roadmap_net_data_t {
   RoadMapNetConnectCallback callback;
   void *context;
   char packet[255];
} RoadMapNetData;

struct roadmap_socket_t {
   int s;
   int is_compressed;
   RoadMapHttpCompCtx compress_ctx;
};

static RoadMapConfigDescriptor RoadMapConfigNetCompressEnabled =
                        ROADMAP_CONFIG_ITEM( "Network", "Compression Enabled");

static int  RoadMapNetNumConnects;
static BOOL RoadMapNetCompressEnabled = FALSE;

static const char* GetProxyAddress() {
#ifdef IPHONE
   return (roadmap_main_get_proxy ("http://www.waze.com"));
#endif //IPHONE
   return NULL;
}

#define CONNECT_TIMEOUT_SEC 5

static void connect_callback (RoadMapSocket s, RoadMapNetData *data);
static void io_connect_callback (RoadMapIO *io);

static struct servent* roadmap_net_getservbyname( const char *name, const char *protocol );
static RoadMapSocket roadmap_net_connect_internal (const char *protocol, const char *name,
                                                   time_t update_time,
                                                   int default_port,
                                                   int async,
                                                   int flags,
                                                   RoadMapNetConnectCallback callback,
                                                   void *context,
                                                   RoadMapSocket reuse_socket,
                                                   int num_retries);

static void check_connect_timeout (void) {
   RoadMapIO *io;

   time_t timeout = time(NULL) - CONNECT_TIMEOUT_SEC;

   while ((io = roadmap_main_output_timedout(timeout))) {
      RoadMapNetData *data = io->context;
      RoadMapSocket s = io->os.socket;
      RoadMapIO retry_io = *io;

      roadmap_log(ROADMAP_ERROR, "Connect time out");
      roadmap_main_remove_input(io);
      
      RoadMapNetNumConnects--;
      if (RoadMapNetNumConnects == 0) {
         roadmap_main_remove_periodic(check_connect_timeout);
      }
      

      if (retry_io.retry_params.num_retries < 2) {
         roadmap_net_mon_disconnect();
         close (s->s);
         if (s->compress_ctx) roadmap_http_comp_close(s->compress_ctx);
         
         retry_io.retry_params.num_retries++;
         roadmap_log(ROADMAP_ERROR, "Retrying connection (retry # %d)", retry_io.retry_params.num_retries);
         if (ROADMAP_INVALID_SOCKET != roadmap_net_connect_internal (retry_io.retry_params.protocol, retry_io.retry_params.name,
                                                                     retry_io.retry_params.update_time,
                                                                     retry_io.retry_params.default_port,
                                                                     TRUE,
                                                                     retry_io.retry_params.flags,
                                                                     retry_io.retry_params.callback,
                                                                     retry_io.retry_params.context,
                                                                     retry_io.os.socket,
                                                                     retry_io.retry_params.num_retries))
            continue;
      } else {
         roadmap_net_close(s);
      }
      
      connect_callback(ROADMAP_INVALID_SOCKET, data);
   }
}

static void connect_callback (RoadMapSocket s, RoadMapNetData *data) {

   RoadMapNetNumConnects--;

   if (RoadMapNetNumConnects == 0) {
      roadmap_main_remove_periodic(check_connect_timeout);
   }

   if ((s != ROADMAP_INVALID_SOCKET) && *data->packet) {
      if( -1 == roadmap_net_send(s, data->packet,
                                    (int)strlen(data->packet), 1)) {
         roadmap_log( ROADMAP_ERROR, "roadmap_net callback (HTTP) - Failed to send the 'POST' packet");
         roadmap_net_close(s);
         s = ROADMAP_INVALID_SOCKET;
      }
   }

   if (s == ROADMAP_INVALID_SOCKET)
       (*data->callback) (s, data->context, err_net_failed);
   else
      (*data->callback) (s, data->context, succeeded);
   free(data);
}

static void io_connect_callback (RoadMapIO *io) {

   RoadMapNetData *data = io->context;
   RoadMapSocket s;

   s = io->os.socket;
   if (s != ROADMAP_INVALID_SOCKET) {
      roadmap_main_remove_input(io);
   }

   connect_callback(s, data);
}

static RoadMapSocket create_socket (const char *protocol,
                                    const char *name,
                                    int default_port,
                                    struct sockaddr *saddr) {
   int   s;
   char *hostname;
   char *separator = strchr (name, ':');

   struct hostent *host = NULL;
   struct sockaddr_in *addr = (struct sockaddr_in *)saddr;

   RoadMapSocket sock_struct;

   roadmap_net_mon_connect ();

   addr->sin_family = AF_INET;

   hostname = strdup(name);
   roadmap_check_allocated(hostname);

   if (separator != NULL) {

      struct servent *service = NULL;

#ifndef IPHONE
      service = roadmap_net_getservbyname( separator+1, "tcp" );
#endif

      if (service == NULL) {

         if (isdigit(separator[1])) {

            addr->sin_port = htons(atoi(separator+1));

            if (addr->sin_port == 0) {
               roadmap_log (ROADMAP_ERROR, "invalid port in '%s'", name);
               goto connection_failure;
            }

         } else {
            roadmap_log (ROADMAP_ERROR, "invalid service in '%s'", name);
            goto connection_failure;
         }

      } else {
         addr->sin_port = service->s_port;
      }

      *(strchr(hostname, ':')) = 0;


   } else {
      addr->sin_port = htons(default_port);
   }

   if (!isdigit(hostname[0])) {
      roadmap_log(ROADMAP_INFO, "Calling gethostbyname:%s", hostname);
      host = gethostbyname(hostname);
   }

   if (host == NULL) {

      if (isdigit(hostname[0])) {
         addr->sin_addr.s_addr = inet_addr(hostname);
         if (addr->sin_addr.s_addr == INADDR_NONE) {
            roadmap_log (ROADMAP_ERROR, "invalid IP address '%s'", hostname);
            goto connection_failure;
         }
      } else {
         roadmap_log (ROADMAP_ERROR, "invalid host name '%s'", hostname);
         goto connection_failure;
      }

   } else {
      memcpy (&addr->sin_addr, host->h_addr, host->h_length);
   }

   if (strcmp (protocol, "udp") == 0) {
      s = socket (PF_INET, SOCK_DGRAM, 0);
   } else if (strcmp (protocol, "tcp") == 0) {
      s = socket (PF_INET, SOCK_STREAM, 0);
   } else {
      roadmap_log (ROADMAP_ERROR, "unknown protocol %s", protocol);
      goto connection_failure;
   }

   if (s < 0) {
      roadmap_log (ROADMAP_ERROR, "cannot create socket, errno = %d", errno);
      goto connection_failure;
   }

   sock_struct = (RoadMapSocket) calloc(sizeof(struct roadmap_socket_t), 1);
   sock_struct->s = s;

   free(hostname);

   return sock_struct;

connection_failure:

   free(hostname);
   roadmap_net_mon_disconnect();
   roadmap_net_mon_error("Error connecting.");
   return ROADMAP_INVALID_SOCKET;
}


static int create_connection (RoadMapSocket s, struct sockaddr *addr) {

   if (connect (s->s, addr, sizeof(*addr)) < 0) {

      return -1;
   }

   return 0;
}

#ifndef IPHONE
static int create_async_connection (RoadMapIO *io, struct sockaddr *addr) {

   int s_flags;
   int res;

   RoadMapSocket s = io->os.socket;
   s_flags = fcntl(s->s, F_GETFL, 0);
   if (fcntl(s->s, F_SETFL, s_flags|O_NONBLOCK) == -1) {
      roadmap_log (ROADMAP_ERROR, "Can't set socket nonblocking, errno = %d", errno);
   }

   if ((res = connect (s->s, addr, sizeof(*addr))) < 0) {

      if (errno != EINPROGRESS) {
         return -1;
      }

      if (fcntl(s->s, F_SETFL, s_flags) == -1) {
         roadmap_log (ROADMAP_ERROR, "Can't reset socket to blocking, errno = %d", errno);
         return -1;
      }
   }

   roadmap_main_set_output(io, io_connect_callback);
   RoadMapNetNumConnects++;

   if (res == 0) {
      /* Probably not realistic */
      io_connect_callback(io);
      return 0;
   }

   if (RoadMapNetNumConnects == 1) {
      roadmap_main_set_periodic(CONNECT_TIMEOUT_SEC * 1000 /2, check_connect_timeout);
   }

   return 0;
}
#endif


static RoadMapSocket roadmap_net_connect_internal (const char *protocol, const char *name,
                                         time_t update_time,
                                         int default_port,
                                         int async,
                                         int flags,
                                         RoadMapNetConnectCallback callback,
                                         void *context,
                                         RoadMapSocket reuse_socket,
                                         int          num_retries) {

   char            server_url  [ WSA_SERVER_URL_MAXSIZE   + 1];
   char            service_name[ WSA_SERVICE_NAME_MAXSIZE + 1];
   int             server_port;
   const char*     proxy_address = GetProxyAddress();
   char            packet[512];
   char            update_since[WDF_MODIFIED_HEADER_SIZE + 1];
   RoadMapSocket   res_socket, temp_socket;
   const char *   req_type = "GET";
   struct sockaddr addr;
   RoadMapNetData *data = NULL;

   if( strncmp( protocol, "http", 4) != 0) {
      temp_socket = create_socket(protocol, name, default_port, &addr);

      if(ROADMAP_INVALID_SOCKET == temp_socket) return ROADMAP_INVALID_SOCKET;
      
      if (reuse_socket != ROADMAP_INVALID_SOCKET) {
         res_socket = reuse_socket;
         res_socket->s = temp_socket->s;
         free(temp_socket);
      } else {
         res_socket = temp_socket;
      }

      if (async) {
         data = (RoadMapNetData *)malloc(sizeof(RoadMapNetData));
         data->packet[0] = '\0';
      }
   } else {
      // HTTP Connection, using system configuration for Proxy

      WDF_FormatHttpIfModifiedSince (update_time, update_since);
      if (!strcmp(protocol, "http_post")) req_type = "POST";

      if( !WSA_ExtractParams( name,          //   IN        -   Web service full address (http://...)
                              server_url,    //   OUT,OPT   -   Server URL[:Port]
                              &server_port,  //   OUT,OPT   -   Server Port
                             service_name)) //   OUT,OPT   -   Web service name
      {
         roadmap_log( ROADMAP_ERROR, "roadmap_net_connect(HTTP) - Failed to extract information from '%s'", name);
         return ROADMAP_INVALID_SOCKET;
      }

      if (proxy_address) {
         int   proxy_port  = server_port;
         char* port_offset = strchr(proxy_address, ':');
         if (port_offset) proxy_port = atoi(port_offset + 1);

         temp_socket = create_socket("tcp", proxy_address, proxy_port, &addr);

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

         temp_socket = create_socket("tcp", server_url, server_port, &addr);

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

      if(ROADMAP_INVALID_SOCKET == temp_socket) return ROADMAP_INVALID_SOCKET;
      
      if (reuse_socket != ROADMAP_INVALID_SOCKET) {
         res_socket = reuse_socket;
         res_socket->s = temp_socket->s;
         free(temp_socket);
      } else {
         res_socket = temp_socket;
      }
      
      res_socket->is_compressed = TEST_NET_COMPRESS( flags );

      if (async) {
         data = (RoadMapNetData *)malloc(sizeof(RoadMapNetData));
         strncpy_safe(data->packet, packet, sizeof(data->packet));
      }
   }

   if (async) {
      RoadMapIO io;

      data->callback = callback;
      data->context = context;

      io.subsystem = ROADMAP_IO_NET;
      io.context = data;
      io.os.socket = res_socket;

      io.retry_params.num_retries = num_retries;
      io.retry_params.protocol = strdup(protocol);
      io.retry_params.name = strdup(name);
      io.retry_params.update_time = update_time;
      io.retry_params.default_port = default_port;
      io.retry_params.flags = flags;
      io.retry_params.callback = callback;
      io.retry_params.context = context;

#ifdef IPHONE
      if (roadmap_main_async_connect(&io, &addr, io_connect_callback) == -1) {
#else
      if (create_async_connection(&io, &addr) == -1) {
#endif
         free(data);
         roadmap_net_close(res_socket);
         return ROADMAP_INVALID_SOCKET;
      }

#ifdef IPHONE
      RoadMapNetNumConnects++;

      if (RoadMapNetNumConnects == 1) {
         roadmap_main_set_periodic(CONNECT_TIMEOUT_SEC * 1000 /2, check_connect_timeout);
      }
#endif

   } else {

      /* Blocking connect */
      if (create_connection(res_socket, &addr) == -1) {
         roadmap_net_close(res_socket);
         return ROADMAP_INVALID_SOCKET;
      }

      if( strncmp(protocol, "http", 4) == 0) {
         if(-1 == roadmap_net_send(res_socket, packet, (int)strlen(packet), 1)) {
            roadmap_log( ROADMAP_ERROR, "roadmap_net_connect(HTTP) - Failed to send the 'POST' packet");
            roadmap_net_close(res_socket);
            return ROADMAP_INVALID_SOCKET;
         }
      }
   }

   return res_socket;
}


int roadmap_net_get_fd(RoadMapSocket s) {
   return s->s;
}

RoadMapSocket roadmap_net_connect (const char *protocol, const char *name,
                                   time_t update_time,
                                   int default_port,
                                   int flags,
                                   roadmap_result* err) {

   if (err != NULL)
   (*err) = succeeded;

   RoadMapSocket s = roadmap_net_connect_internal(protocol, name, update_time,
                                                   default_port, 0, flags, NULL, NULL, ROADMAP_INVALID_SOCKET, 0);


   if ((s == ROADMAP_INVALID_SOCKET) && (err != NULL))
      (*err) = err_net_failed;
   return s;
}


int roadmap_net_connect_async (const char *protocol, const char *name,
                               time_t update_time,
                               int default_port,
                               int flags,
                               RoadMapNetConnectCallback callback,
                               void *context) {

   RoadMapSocket s = roadmap_net_connect_internal
                        (protocol, name, update_time, default_port, 1, flags, callback, context, ROADMAP_INVALID_SOCKET, 0);

   if (ROADMAP_NET_IS_VALID(s)) return 0;
   else return -1;
}


int roadmap_net_send (RoadMapSocket s, const void *data, int length, int wait) {

   int total = length;
   fd_set fds;
   struct timeval recv_timeout = {0, 0};

   FD_ZERO(&fds);
   FD_SET(s->s, &fds);

   if (wait) {
      recv_timeout.tv_sec = 60;
   }

   while (length > 0) {
      int res;

      res = select(s->s + 1, NULL, &fds, NULL, &recv_timeout);

      if(!res) {
         roadmap_log (ROADMAP_ERROR,
               "Timeout waiting for select in roadmap_net_send");

         roadmap_net_mon_error("Error in send - timeout.");

         if (!wait) return 0;
         else return -1;
      }

      if(res < 0) {
         roadmap_log (ROADMAP_ERROR,
               "Error waiting on select in roadmap_net_send");

         roadmap_net_mon_error("Error in send - select.");
         return -1;
      }

      res = send(s->s, data, length, 0);

      if (res < 0) {
         roadmap_log (ROADMAP_ERROR, "Error sending data: (%d) %s", errno, strerror(errno));

         roadmap_net_mon_error("Error in send - data.");
         return -1;
      }

      length -= res;
      data = (char *)data + res;

      roadmap_net_mon_send(res);
   }

   return total;
}


int roadmap_net_receive (RoadMapSocket s, void *data, int size) {

   int received;
   
   if (s->is_compressed) {

      if (!s->compress_ctx) {
         s->compress_ctx = roadmap_http_comp_init();
         if (s->compress_ctx == NULL) return -1;
      }

      if ((received = roadmap_http_comp_read(s->compress_ctx, data, size))
            == 0) {

         void *ctx_buffer;
         int ctx_buffer_size;

         roadmap_http_comp_get_buffer(s->compress_ctx, &ctx_buffer, &ctx_buffer_size);

         received = read(s->s, ctx_buffer, ctx_buffer_size);

         roadmap_http_comp_add_data(s->compress_ctx, received);
         
         received = roadmap_http_comp_read(s->compress_ctx, data, size);
      }


   } else {

      received = read (s->s, data, size);
   }

   if (received < 0) {
      roadmap_net_mon_error("Error in recv.");
      roadmap_log (ROADMAP_DEBUG, "Error in recv., errno = %d", errno);
      return -1; /* On UNIX, this is sign of an error. */
   }

   roadmap_net_mon_recv(received);

   return received;
}


RoadMapSocket roadmap_net_listen(int port) {

   return ROADMAP_INVALID_SOCKET; // Not yet implemented.
}


RoadMapSocket roadmap_net_accept(RoadMapSocket server_socket) {

   return ROADMAP_INVALID_SOCKET; // Not yet implemented.
}


void roadmap_net_close (RoadMapSocket s) {
   roadmap_net_mon_disconnect();
   close (s->s);
   if (s->compress_ctx) roadmap_http_comp_close(s->compress_ctx);
   free(s);
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


static struct servent* roadmap_net_getservbyname( const char *name, const char *protocol )
{
   static int has_bug = -1;   /* Android bug overriding (returns the port in the wrong order */
   struct servent* service = NULL;

#ifndef IPHONE
   if ( has_bug < 0 )
   {
      service = getservbyname( "http", NULL );
        has_bug = ( service == NULL || service->s_port == 80 );
   }

   service = getservbyname( name, protocol );
    if ( service && has_bug )
    {
        service->s_port = htons( service->s_port );
    }
#endif

   return service;
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

