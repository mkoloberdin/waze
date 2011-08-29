/* roadmap_httpcopy_async.c - Download RoadMap maps using the HTTP protocol asynchornously.
 *
 * LICENSE:
 *
 *   Copyright 2003 Pascal Martin.
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
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "roadmap.h"
#include "roadmap_io.h"
#include "roadmap_net.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_main.h"
#include "roadmap_base64.h"

#include "editor/editor_main.h"
#include "roadmap_httpcopy_async.h"


#if !defined (__SYMBIAN32__)
#define ROADMAP_HTTP_MAX_CHUNK 32768
#else
#define ROADMAP_HTTP_MAX_CHUNK 4096
#endif

#define ROADMAP_HTTP_MAX_UPLOAD_CHUNK 4096
#define ROADMAP_HTTP_MP_BOUNDARY   "---------------------------10424402741337131014341297293" 

typedef enum
{
   _http_async_method__get,
   _http_async_method__post,
   _http_async_method__post_file
} HttpAsyncMethod;

struct HttpAsyncContext_st {
	RoadMapHttpAsyncCallbacks *callbacks;
	void *cb_context;
	int download_size_current;
	int received_status;
	int content_length;
	int is_parsing_headers;
	char header_buffer[512];
	char error_buffer[512];
	char last_modified_buffer[256];
	RoadMapIO io;
	char* header;
	const void *data;
	int data_len;
   RoadMapFile file;
	HttpAsyncMethod method;
   char write_buf[ROADMAP_HTTP_MAX_UPLOAD_CHUNK];
   int write_buf_read;
   int write_buf_sent;
   int flags;
};


static void roadmap_http_async_prepare_input (HttpAsyncContext *hcontext);


static int roadmap_http_async_decode_header (HttpAsyncContext *context,
															char *buffer,
                              		         int  sizeof_buffer) {

   int  shift;

   int  total;
   int  received;

   char *p;
   char *next;

   total = sizeof_buffer;

   for (;;) {

      buffer[sizeof_buffer] = 0;
      if (strchr (buffer, '\n') == NULL) {

         /* We do not have a full line: we need more data. */

         return 0;
      }

      shift = 2;
      next = strstr (buffer, "\r\n");
      if (next == NULL) {
         shift = 1;
         next = strchr (buffer, '\n');
      }

      if (next != NULL) {

         *next = 0;

			//printf ("Line is %s\n", buffer);

         if (! context->received_status) {

            if (next != buffer) {
               if (strstr (buffer, " 200 ") == NULL) {
                  roadmap_log (ROADMAP_DEBUG, "roadmap_http_async_decode_header() : received bad status: %s", buffer);
                  return -1;
               }
               context->received_status = 1;
            }

         } else {

            if (next == buffer) {

               /* An empty line signals the end of the header. Any
                * reminder data is part of the download: save it.
                */
               context->is_parsing_headers = 0;
               context->header_buffer[0] = '\0';
               next += shift;
               received = (buffer + total) - next;
               if (received) memmove (buffer, next, received);

               return received;
            }

            if ( ( context->flags & HTTPCOPY_FLAG_IGNORE_CONTENT_LEN ) == 0 ) /* If no ignore */
            {
               if (strncasecmp (buffer,
                           "Content-Length", sizeof("Content-Length")-1) == 0) {

                  p = strchr (buffer, ':');
                  if (p == NULL) {
                     roadmap_log (ROADMAP_ERROR, "roadmap_http_async_decode_header() : bad formed header: %s", buffer);
                     return -1;
                  }

                  while (*(++p) == ' ') ;
                  context->content_length = atoi(p);
                  if (context->content_length < 0) {
                     roadmap_log (ROADMAP_ERROR, "roadmap_http_async_decode_header() : bad formed header: %s", buffer);
                     return -1;
                  }
                  else if (context->content_length == 0){
                     return -2;
                  }
               }
            }
            if (strncasecmp (buffer,
                        "Last-Modified", sizeof("Last-Modified")-1) == 0) {

               p = strchr (buffer, ':');
               if (p != NULL) {
                  while (*(++p) == ' ') ;
                  context->last_modified_buffer[0] = '\0';
                  strncpy(context->last_modified_buffer, p, sizeof(context->last_modified_buffer));
               }
               
            }
         }

         /* Move the remaining data to the beginning of the buffer
          * and wait for more.
          */
         next += shift;
         received = (buffer + total) - next;
         if (received) memmove (buffer, next, received);
         total = received;
      }
   }

   roadmap_log (ROADMAP_ERROR, "roadmap_http_async_decode_header() : No valid header received");
   return -1;
}

/*
 * Examines the content length value
 */
static BOOL _check_content_length( HttpAsyncContext *context ){

   BOOL result = TRUE;

   if ( ( context->flags & HTTPCOPY_FLAG_IGNORE_CONTENT_LEN ) == 0 )
   {
      result = ( context->content_length > 0 ) && ( context->download_size_current == context->content_length );
   }

   return result;
}

static void roadmap_http_async_has_data_cb (RoadMapIO *io) {

	HttpAsyncContext *context = (HttpAsyncContext *)io->context;
   RoadMapHttpAsyncCallbacks *callbacks = context->callbacks;
   char buffer[ROADMAP_HTTP_MAX_CHUNK + 1];
   BOOL ignore_content_len = ( context->flags & HTTPCOPY_FLAG_IGNORE_CONTENT_LEN );
   int res;
   
   if (!context->is_parsing_headers) {
      int read_size = context->content_length - context->download_size_current;
      if (read_size > ROADMAP_HTTP_MAX_CHUNK || ignore_content_len ) {
         read_size = ROADMAP_HTTP_MAX_CHUNK;
      }
		res = roadmap_io_read(io, buffer, read_size);
		if (res < 0) {
         roadmap_log(ROADMAP_ERROR, "roadmap_http_async_has_data_cb(): error receiving http data (%d/%d). Requested: %d", context->download_size_current,
            context->content_length, read_size );
		}
   } else {
      /* we're still parsing headers and may have left overs to append */
      int leftover_size = strlen(context->header_buffer);
      res = roadmap_io_read(io, buffer + leftover_size, ROADMAP_HTTP_MAX_CHUNK - leftover_size);
      if (res > 0) {
         if (leftover_size) {
         	memcpy(buffer, context->header_buffer, leftover_size);
         	context->header_buffer[0] = '\0';
         }
         strncpy_safe( context->error_buffer, buffer, sizeof( context->error_buffer ) );
         res = roadmap_http_async_decode_header(context, buffer, res + leftover_size);
         if (res == -2){
            roadmap_main_remove_input(io);
            roadmap_io_close(&context->io);
            
            if (context->method == _http_async_method__post_file)
               callbacks->done(context->cb_context, context->last_modified_buffer, context->header_buffer);
            else
               callbacks->done(context->cb_context, context->last_modified_buffer, NULL);
            
            return;
         }

         if ( context->is_parsing_headers ) {
            if ( res == 0 ) { /* Need more header data - keep listening.
                               * Other res values are handled further (actually can't be positive in this state ... */
               if (strlen(buffer) && (strlen(buffer) < sizeof(context->header_buffer)))
                              strcpy(context->header_buffer, buffer);
               return;
            }
         } else {
            /* we just finished parsing the headers - check size callback for the received content length */
            if (context->method != _http_async_method__post_file) {
               if (!callbacks->size(context->cb_context, context->content_length)) {
                  res = -1;
               }
            }
            if ( res == 0 ) { /* Size is Ok and there is no download data to process after checking the headers - keep listening
                               * Other res values will be handled further */
               return;
            }
         }
      } else if (res < 0) {
         roadmap_log (ROADMAP_DEBUG, "roadmap_http_async_has_data_cb(): error receiving http header (%d)", leftover_size);//AviR: lower log level until we remove false alerts
		} else  /* res == 0 */ {
         roadmap_log (ROADMAP_DEBUG, "roadmap_http_async_has_data_cb(): error receiving http header. Read zero bytes. (%d)", leftover_size );//AviR: lower log level until we remove false alerts
         res = -1;
      }
   }

   if (res > 0) {
      context->download_size_current += res;
      if (context->method != _http_async_method__post_file) {
         callbacks->progress (context->cb_context, buffer, res);
      } else {
         if (res > sizeof(context->header_buffer) - strlen(context->header_buffer) - 1) {
            roadmap_log(ROADMAP_ERROR, "roadmap_http_async_has_data_cb(): error receiving http data - not enough buffer to save response (%d/%d)", context->download_size_current,
                        context->content_length);
            res = -1;
         } else {
            int cur_size = strlen(context->header_buffer);
            memcpy (context->header_buffer + cur_size, buffer, res);
            context->header_buffer[cur_size + res] = 0;
         }
      }

   }

   if ((res <= 0) || (context->download_size_current >= context->content_length && !ignore_content_len)) {
      roadmap_main_remove_input(io);
      roadmap_io_close(&context->io);

//      if ( context->error_buffer[0] )
//         roadmap_log (ROADMAP_WARNING, "Http error buffer: %s", context->error_buffer );

      if ( ( res >= 0 ) && _check_content_length( context ) ) {
         if (context->method == _http_async_method__post_file)
            callbacks->done(context->cb_context, context->last_modified_buffer, context->header_buffer);
         else
            callbacks->done(context->cb_context, context->last_modified_buffer, NULL);
      }
      else {
         roadmap_log (ROADMAP_DEBUG, "Http download error. Res: %d. Content length: %d. Downloaded: %d",
                                             res, context->content_length, context->download_size_current ); //AviR: Reduced log level
         if ( context->content_length > 0 && context->download_size_current > context->content_length ) {
            roadmap_log (ROADMAP_ERROR, "Too many bytes for http download (%d/%d)", context->download_size_current, context->content_length);
         }
         if ( context->error_buffer[0] )
            callbacks->error ( context->cb_context, 0, "HTTP Error: %s", context->error_buffer );
         else
            callbacks->error (context->cb_context, 0, "roadmap_http_async_has_data_cb() failed" );
      }
      /*
       * Context deallocation. Be aware to NULL references in "error" and "done" callbacks
       */
      free (context);
   }
}

static void roadmap_http_async_write_cb (RoadMapIO *io) {
   int sent = 0;
   HttpAsyncContext *hcontext = (HttpAsyncContext *)io->context;
   RoadMapHttpAsyncCallbacks *callbacks = hcontext->callbacks;
   
   if (hcontext->download_size_current == hcontext->data_len) {
      roadmap_file_close (hcontext->file);
      snprintf(hcontext->write_buf, ROADMAP_HTTP_MAX_UPLOAD_CHUNK, "\r\n--%s--\r\n", ROADMAP_HTTP_MP_BOUNDARY);
      //This is not async write, but sending small buffer which should not block
      sent = roadmap_io_write( &hcontext->io, hcontext->write_buf, strlen(hcontext->write_buf), 0 );
      if (sent == -1) {
         roadmap_io_close(&hcontext->io);
         callbacks->error(hcontext->cb_context, 1, "Error sending request.");
         free (hcontext);
         return;
      }
      
      roadmap_main_remove_input(io);
      roadmap_http_async_prepare_input(hcontext);
      
      return;
   }
   
   if (hcontext->write_buf_read == 0 ||
       hcontext->write_buf_read == hcontext->write_buf_sent) {
      
      hcontext->write_buf_read = roadmap_file_read (hcontext->file, hcontext->write_buf, ROADMAP_HTTP_MAX_UPLOAD_CHUNK);
      
      if (hcontext->write_buf_read <= 0) {
         roadmap_file_close (hcontext->file);
         roadmap_io_close(&hcontext->io);
         callbacks->error(hcontext->cb_context, 1, "Error uploading file - invalid file read size: %d.", sent);
         free (hcontext);
         return;
      }
      
      hcontext->write_buf_sent = 0;
   }
   
   sent = roadmap_io_write_async( &hcontext->io, hcontext->write_buf + hcontext->write_buf_sent, hcontext->write_buf_read - hcontext->write_buf_sent);
   if (sent == -1) {
      roadmap_file_close (hcontext->file);
      roadmap_io_close(&hcontext->io);
      callbacks->error(hcontext->cb_context, 1, "Error sending request.");
      free (hcontext);
      return;
   }
   
   hcontext->write_buf_sent += sent;
   hcontext->download_size_current += sent;
   
   if (hcontext->callbacks->progress)
      hcontext->callbacks->progress(hcontext->cb_context, NULL, sent);
}

static void roadmap_http_async_prepare_input (HttpAsyncContext *hcontext) {
   hcontext->header_buffer[0] = '\0';  // Prepare the buffer for the response header
   hcontext->error_buffer[0] = '\0';
   hcontext->is_parsing_headers = 1;
   hcontext->download_size_current = 0;
   hcontext->received_status = 0;
   hcontext->content_length = -1;
   
   roadmap_main_set_input(&hcontext->io, roadmap_http_async_has_data_cb);
}

static void roadmap_http_async_prepare_output (HttpAsyncContext *hcontext) {
   hcontext->download_size_current = 0;
   hcontext->content_length = -1;
   
   roadmap_main_set_output(&hcontext->io, roadmap_http_async_write_cb, FALSE);
}


static void roadmap_http_async_connect_cb (RoadMapSocket socket, void *context, roadmap_result err) {

	HttpAsyncContext *hcontext = (HttpAsyncContext *)context;
   RoadMapHttpAsyncCallbacks *callbacks = hcontext->callbacks;
   char* header = hcontext->header_buffer;

   if (!ROADMAP_NET_IS_VALID(socket)) {
      callbacks->error(hcontext->cb_context, 1, "Can't connect to server.");
      free (hcontext);
      return;
   }

   hcontext->io.subsystem = ROADMAP_IO_NET;
   hcontext->io.context = context;
   hcontext->io.os.socket = socket;

   if ( ( strlen( header ) + 3 ) /* [header"\r\n\0"]*/ > sizeof( hcontext->header_buffer ) )
   {
      callbacks->error( hcontext->cb_context, 1, "Error sending request. Header text is too long." );
      roadmap_io_close( &hcontext->io );
      free (hcontext);
      return;
   }

   strcat( header, "\r\n" );

   if ( roadmap_io_write( &hcontext->io, header, strlen( header ), 0 ) == -1) {
      roadmap_io_close(&hcontext->io);
      callbacks->error(hcontext->cb_context, 1, "Error sending request.");
      free (hcontext);
      return;
   }

   if ( hcontext->data != NULL )
   {
      if ( roadmap_io_write( &hcontext->io, hcontext->data, hcontext->data_len, 0 ) == -1) {
         roadmap_io_close(&hcontext->io);
         callbacks->error(hcontext->cb_context, 1, "Error sending request.");
         free (hcontext);
         return;
      }
   if (roadmap_io_write(&hcontext->io, "\r\n", 2, 0) == -1) {
      roadmap_io_close(&hcontext->io);
      callbacks->error(hcontext->cb_context, 1, "Error sending request.");
      free (hcontext);
      return;
   }

   }
   
   if (hcontext->method != _http_async_method__post_file)
      roadmap_http_async_prepare_input(hcontext);
   else
      roadmap_http_async_prepare_output(hcontext);
   
   callbacks->progress (hcontext->cb_context, NULL, 0);
}

HttpAsyncContext * roadmap_http_async_post_file( RoadMapHttpAsyncCallbacks *callbacks, void *context,
                                           const char *source, const char* header, const char *full_name, int size )
{
   HttpAsyncContext *hcontext = malloc (sizeof (HttpAsyncContext));
   hcontext->callbacks = callbacks;
   hcontext->cb_context = context;
   hcontext->io.os.socket = ROADMAP_INVALID_SOCKET;
   hcontext->last_modified_buffer[0] = '\0';
   hcontext->header_buffer[0] = '\0';
   hcontext->error_buffer[0] = '\0';
   hcontext->data = NULL;
   hcontext->data_len = size;
   hcontext->write_buf[0] = '\0';
   hcontext->write_buf_read = 0;
   hcontext->write_buf_sent = 0;
   hcontext->method = _http_async_method__post_file;
   hcontext->flags = HTTPCOPY_FLAG_NONE;
   
   if ( header != NULL )
      strncpy( hcontext->header_buffer, header, sizeof( hcontext->header_buffer ) );
   
   hcontext->file = roadmap_file_open (full_name, "r");
   
   if (!ROADMAP_FILE_IS_VALID(hcontext->file)) {
      roadmap_log(ROADMAP_ERROR,"Could not open file '%s' for upload", full_name);
      callbacks->error(context, 1, "Could not open file");
      free (hcontext);
      return NULL;
   }
   
   if (!callbacks->size(hcontext->cb_context, hcontext->data_len)) {
      roadmap_log(ROADMAP_ERROR,"Size check failed for file '%s' upload", full_name);
      callbacks->error(context, 1, "Size check failed");
      free (hcontext);
      return NULL;
   }
   
   if ( roadmap_net_connect_async( "http_post", source, source, 0, 80, 0,
                                  roadmap_http_async_connect_cb, hcontext ) == NULL )
   {
      callbacks->error(context, 1, "Can't create http connection.");
      free (hcontext);
      return NULL;
   }
   
   return hcontext;
}

static char* get_encoded_auth (const char *user, const char *pw) {
   char auth_string[255];
   char *base64Text;
   int text_length;
   int size;
   
   snprintf (auth_string, sizeof(auth_string), "%s:%s", user, pw);
   
   size = strlen(auth_string);   
   text_length = roadmap_base64_get_buffer_size(size);
   base64Text = (char*)malloc( text_length );
   
   if (!roadmap_base64_encode(auth_string, size, &base64Text, text_length))
      return NULL;
   
   return base64Text;
}

const char* roadmap_http_async_get_upload_header( const char* content_type, const char *full_name, int size,
                                                 const char *user, const char *pw)
{
   static char s_header[512];
   char *encoded_auth;
   
   const char *filename = roadmap_path_skip_directories (full_name);
   
   s_header[0] = '\0';
   
   if (user && user[0] && pw && pw[0]) {
      encoded_auth = get_encoded_auth(user, pw);
      if (!encoded_auth) {
         return s_header;
      }
      snprintf( s_header, sizeof( s_header ),
               "Authorization: Basic %s\r\n",
               encoded_auth);

      free(encoded_auth);

   }

   
   snprintf( s_header + strlen(s_header), sizeof( s_header ) - strlen(s_header),
            "Content-Type: multipart/form-data; boundary=%s\r\n"
            "Content-Length: %d\r\n\r\n"
            "--%s\r\n"
            "Content-disposition: form-data; name=\"file_0\"; filename=\"%s\"\r\n"
            "Content-type: %s\r\n"
            "Content-Transfer-Encoding: binary\r\n",
            ROADMAP_HTTP_MP_BOUNDARY,
            (int)(size + 237 + strlen(filename) + strlen(content_type)),
            ROADMAP_HTTP_MP_BOUNDARY,
            filename,
            content_type);
   
   return s_header;
}

HttpAsyncContext * roadmap_http_async_post( RoadMapHttpAsyncCallbacks *callbacks, void *context,
                                                 const char *source, const char* header, const void* data, int data_length, int flags )
{
   HttpAsyncContext *hcontext = malloc (sizeof (HttpAsyncContext));
   hcontext->callbacks = callbacks;
   hcontext->cb_context = context;
   hcontext->io.os.socket = ROADMAP_INVALID_SOCKET;
   hcontext->last_modified_buffer[0] = '\0';
   hcontext->header_buffer[0] = '\0';
   hcontext->error_buffer[0] = '\0';
   hcontext->data = data;
   hcontext->data_len = data_length;
   hcontext->method = _http_async_method__post;
   hcontext->flags = flags;

   if ( header != NULL )
      strncpy( hcontext->header_buffer, header, sizeof( hcontext->header_buffer ) );

   if ( roadmap_net_connect_async( "http_post", source, source, 0, 80, 0,
               roadmap_http_async_connect_cb, hcontext ) == NULL )
      {
         callbacks->error(context, 1, "Can't create http connection.");
         free (hcontext);
         return NULL;
      }

      return hcontext;
}

const char* roadmap_http_async_get_simple_header( const char* content_type, int content_len )
{
   static char s_header[256];

   snprintf( s_header, sizeof( s_header ),
               "Content-type: %s\r\n"
               "Content-Length: %d\r\n",
               content_type, content_len );

   return s_header;
}


HttpAsyncContext * roadmap_http_async_copy (RoadMapHttpAsyncCallbacks *callbacks,
									  void *context,
                             const char *source,
                             time_t update_time) {

	HttpAsyncContext *hcontext = malloc (sizeof (HttpAsyncContext));
	hcontext->callbacks = callbacks;
	hcontext->cb_context = context;
	hcontext->io.os.socket = ROADMAP_INVALID_SOCKET;
	hcontext->last_modified_buffer[0] = '\0';
	hcontext->header_buffer[0] = '\0';
	hcontext->error_buffer[0] = '\0';
	hcontext->data = NULL;
   hcontext->data_len = 0;
	hcontext->method = _http_async_method__get;
	hcontext->flags = HTTPCOPY_FLAG_NONE;
	
   if (roadmap_net_connect_async("http_get", source, source, update_time, 80, 0,
            roadmap_http_async_connect_cb, hcontext) == NULL) {
      callbacks->error(context, 1, "Can't create http connection.");
      free (hcontext);
      return NULL;
   }

   return hcontext;
}


void roadmap_http_async_copy_abort (HttpAsyncContext *context) {
   /*
    * The context can be deallocated earlier by callback, however this function
    * can be called with no consideration on the callback execution.
    * The responsibility of the API callbacks to null the pointer on error
    */
   if ( context != NULL )
   {
      if (ROADMAP_NET_IS_VALID(context->io.os.socket)) {
	   roadmap_main_remove_input(&context->io);
	   roadmap_io_close (&context->io);
	  }
      free (context);
   }
}

