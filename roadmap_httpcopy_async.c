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
#include "roadmap_main.h"

#include "editor/editor_main.h"
#include "roadmap_httpcopy_async.h"


#if !defined (__SYMBIAN32__)
#define ROADMAP_HTTP_MAX_CHUNK 32768
#else
#define ROADMAP_HTTP_MAX_CHUNK 4096
#endif

struct HttpAsyncContext_st {
	RoadMapHttpAsyncCallbacks *callbacks;
	void *cb_context;
	int download_size_current;
	int received_status;
	int content_length;
	int is_parsing_headers;
	char header_buffer[256];
	char last_modified_buffer[256];
	RoadMapIO io;
};

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
               next += shift;
               received = (buffer + total) - next;
               if (received) memmove (buffer, next, received);

               return received;
            }

            if (strncasecmp (buffer,
                        "Content-Length", sizeof("Content-Length")-1) == 0) {

               p = strchr (buffer, ':');
               if (p == NULL) {
                  roadmap_log (ROADMAP_ERROR, "roadmap_http_async_decode_header() : bad formed header: %s", buffer);
                  return -1;
               }

               while (*(++p) == ' ') ;
               context->content_length = atoi(p);
               if (context->content_length <= 0) {
                  roadmap_log (ROADMAP_ERROR, "roadmap_http_async_decode_header() : bad formed header: %s", buffer);
                  return -1;
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

static void roadmap_http_async_has_data_cb (RoadMapIO *io) {

	HttpAsyncContext *context = (HttpAsyncContext *)io->context;
   RoadMapHttpAsyncCallbacks *callbacks = context->callbacks;
   char buffer[ROADMAP_HTTP_MAX_CHUNK + 1];

   int res;

   if (!context->is_parsing_headers) {
		int read_size = context->content_length - context->download_size_current;
		if (read_size > ROADMAP_HTTP_MAX_CHUNK) {
			read_size = ROADMAP_HTTP_MAX_CHUNK;
		}
		res = roadmap_io_read(io, buffer, read_size);
		if (res < 0) {
         roadmap_log(ROADMAP_ERROR, "roadmap_http_async_has_data_cb(): error receiving http data (%d/%d)", context->download_size_current,
            context->content_length);
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
         res = roadmap_http_async_decode_header(context, buffer, res + leftover_size);
         if ( context->is_parsing_headers ) {
            if ( res == 0 ) { /* Need more header data - keep listening.
                               * Other res values are handled further (actually can't be positive in this state ... */
               if (strlen(buffer) && (strlen(buffer) < sizeof(context->header_buffer)))
                              strcpy(context->header_buffer, buffer);
               return;
            }
         } else {
            /* we just finished parsing the headers - check size callback for the received content length */
            if (!callbacks->size(context->cb_context, context->content_length)) {
               res = -1;
            }
            if ( res == 0 ) { /* Size is Ok and there is no download data to process after checking the headers - keep listening
                               * Other res values will be handled further */
               return;
            }
         }
      } else if (res < 0) {
         roadmap_log (ROADMAP_ERROR, "roadmap_http_async_has_data_cb(): error receiving http header (%d)", leftover_size);
		} else  /* res == 0 */ {
         roadmap_log (ROADMAP_ERROR, "roadmap_http_async_has_data_cb(): error receiving http header. Read zero bytes. (%d)", leftover_size );
         res = -1;
      }
   }

   if (res > 0) {
      context->download_size_current += res;
      callbacks->progress (context->cb_context, buffer, res);
   }

   if ((res <= 0) || (context->download_size_current >= context->content_length)) {
      roadmap_main_remove_input(io);
      roadmap_io_close(&context->io);

      if ( ( res >= 0 ) && ( context->content_length > 0 ) && ( context->download_size_current == context->content_length ) ) {
         callbacks->done(context->cb_context, context->last_modified_buffer);
      }
      else {
         roadmap_log (ROADMAP_WARNING, "Http download error. Res: %d. Content length: %d. Downloaded: %d",
                                             res, context->content_length, context->download_size_current );
         if ( context->content_length > 0 && context->download_size_current > context->content_length ) {
            roadmap_log (ROADMAP_ERROR, "Too many bytes for http download (%d/%d)", context->download_size_current, context->content_length);
         }
         callbacks->error (context->cb_context, 0, "roadmap_http_async_has_data_cb() failed" );
      }
      /*
       * Context deallocation. Be aware to NULL references in "error" and "done" callbacks
       */
      free (context);
   }
}


static void roadmap_http_async_connect_cb (RoadMapSocket socket, void *context, roadmap_result err) {

	HttpAsyncContext *hcontext = (HttpAsyncContext *)context;
   RoadMapHttpAsyncCallbacks *callbacks = hcontext->callbacks;

   if (!ROADMAP_NET_IS_VALID(socket)) {
      callbacks->error(hcontext->cb_context, 1, "Can't connect to server.");
      free (hcontext);
      return;
   }

   hcontext->io.subsystem = ROADMAP_IO_NET;
   hcontext->io.context = context;
   hcontext->io.os.socket = socket;

   if (roadmap_io_write(&hcontext->io, "\r\n", 2, 0) == -1) {
      roadmap_io_close(&hcontext->io);
      callbacks->error(hcontext->cb_context, 1, "Error sending request.");
      free (hcontext);
      return;
   }

   hcontext->header_buffer[0] = '\0';
   hcontext->is_parsing_headers = 1;
   hcontext->download_size_current = 0;
   hcontext->received_status = 0;
   hcontext->content_length = -1;

   roadmap_main_set_input(&hcontext->io, roadmap_http_async_has_data_cb);
   callbacks->progress (hcontext->cb_context, NULL, 0);
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
	
   if (roadmap_net_connect_async("http_get", source, update_time, 80, 0,
            roadmap_http_async_connect_cb, hcontext) == -1) {
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

