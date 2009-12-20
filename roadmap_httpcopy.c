/* roadmap_httpcopy.c - Download RoadMap maps using the HTTP protocol.
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
 * SYNOPSYS:
 *
 *   See roadmap_httpcopy.h
 *
 *
 * DESCRIPTION:
 *
 *   This module handles the "http://[...]" URLs for the map download
 *   feature of RoadMap. This module is designed to be linked to RoadMap
 *   either statically or dynamically. In the later case, the following
 *   linker option should be used when creating the shared library on Linux:
 *
 *       --defsym roadmap_plugin_init=roadmap_httpcopy_init
 *
 * BUGS:
 *
 *   This module make explicit references to the roadmap_file module,
 *   which prevents it from being built as a plugin.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "roadmap.h"
#include "roadmap_net.h"
#include "roadmap_file.h"

#include "editor/editor_main.h"
#include "roadmap_httpcopy.h"


#if !defined (_WIN32) && !defined (__SYMBIAN32__)
#define ROADMAP_HTTP_MAX_CHUNK 32768
#else
#define ROADMAP_HTTP_MAX_CHUNK 4096
#endif


static int roadmap_http_decode_header (RoadMapSocket   fd,
                                       char *buffer,
                                       int  *sizeof_buffer,
                                       RoadMapDownloadCallbackError error) {

   int  received_status = 0;
   int  shift;

   int  size;
   int  total;
   int  received;

   char *p;
   char *next;

   size = 0;
   total = 0;

   for (;;) {

      buffer[total] = 0;
      if (strchr (buffer, '\n') == NULL) {

         /* We do not have a full line: we need more data. */

         received =
            roadmap_net_receive
               (fd, buffer + total, *sizeof_buffer - total - 1);

         if (received <= 0) {
            error ("Receive error");
            return 0;
         }

         total += received;
         buffer[total] = 0;
      }

      shift = 2;
      next = strstr (buffer, "\r\n");
      if (next == NULL) {
         shift = 1;
         next = strchr (buffer, '\n');
      }

      if (next != NULL) {

         *next = 0;

         if (! received_status) {

            if (next != buffer) {
               if (strstr (buffer, " 200 ") == NULL) {
                  error ("received bad status: %s", buffer);
                  return 0;
               }
               received_status = 1;
            }

         } else {

            if (next == buffer) {

               /* An empty line signals the end of the header. Any
                * reminder data is part of the download: save it.
                */
               next += shift;
               received = (buffer + total) - next;
               if (received) memcpy (buffer, next, received);
               *sizeof_buffer = received;

               return size;
            }

            if (strncasecmp (buffer,
                        "Content-Length", sizeof("Content-Length")-1) == 0) {

               p = strchr (buffer, ':');
               if (p == NULL) {
                  error ("bad formed header: %s", buffer);
                  return 0;
               }

               while (*(++p) == ' ') ;
               size = atoi(p);
               if (size <= 0) {
                  error ("bad formed header: %s", buffer);
                  return 0;
               }
            }
         }

         /* Move the remaining data to the beginning of the buffer
          * and wait for more.
          */
         next += shift;
         received = (buffer + total) - next;
         if (received) memcpy (buffer, next, received);
         total = received;
      }
   }

   error ("No valid header received");
   return 0;
}


static int roadmap_httpcopy (RoadMapDownloadCallbacks *callbacks,
                             const char *source,
                             const char *destination) {

   RoadMapSocket fd;
   RoadMapFile file;
   int size;
   int loaded;
   int received;

   char buffer[ROADMAP_HTTP_MAX_CHUNK];


   fd = roadmap_net_connect("http_get", source, 0, 80, NULL);
   if (!ROADMAP_NET_IS_VALID(fd)) return 0;
   if (roadmap_net_send(fd, "\r\n", 2, 0) == -1) return 0;

   received = sizeof(buffer);
   size = roadmap_http_decode_header
             (fd, buffer, &received, callbacks->error);
   if (size <= 0) {
      roadmap_net_close (fd);
      return 0; /* We did not get the size. */
   }

   if (! callbacks->size (size)) {
      roadmap_net_close (fd);
      return 0;
   }

   callbacks->progress (received);
   roadmap_file_remove (NULL, destination);
   file = roadmap_file_open(destination, "w");
   if (!ROADMAP_FILE_IS_VALID(file)) {
      roadmap_net_close (fd);
      return 0;
   }

   if (received > 0) {
      if (roadmap_file_write(file, buffer, received) != received) {
         callbacks->error ("Error writing data");
         goto cancel_download;
      }
   }
   loaded = received;

   while (loaded < size) {

      received = roadmap_net_receive (fd, buffer, sizeof(buffer));

      if (received <= 0) {
         callbacks->error ("Receive error after %d data bytes", loaded);
         goto cancel_download;
      }
      
      if (roadmap_file_write(file, buffer, received) != received) {
         callbacks->error ("Error writing data");
         goto cancel_download;
      }

      loaded += received;

      callbacks->progress (loaded);
   }

   if (loaded != size) {
      callbacks->error ("Receive error after %d data bytes", loaded);
      goto cancel_download;   
   }
   roadmap_net_close (fd);
   roadmap_file_close(file);

   return 1;

cancel_download:

   roadmap_file_close(file);
   roadmap_file_remove (NULL, destination);
   roadmap_net_close (fd);

   return 0;
}


void roadmap_httpcopy_init (RoadMapDownloadSubscribe subscribe) {

   subscribe ("http://", roadmap_httpcopy);
}

