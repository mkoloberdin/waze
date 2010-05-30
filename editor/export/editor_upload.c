/* editor_upload.c - Upload a track file to waze
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
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
 *   See editor_upload.h
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "roadmap.h"
#include "roadmap_net.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_httpcopy.h"
#include "roadmap_messagebox.h"
#include "roadmap_config.h"
#include "roadmap_main.h"
#include "roadmap_fileselection.h"
#include "md5.h"

#include "../editor_main.h"
#include "editor_upload.h"

#define ROADMAP_HTTP_MAX_CHUNK      (4096)
#define DEFAULT_CONTENT_TYPE        ("application/octet-stream")

static RoadMapConfigDescriptor RoadMapConfigTarget =
                                  ROADMAP_CONFIG_ITEM("Realtime", "OfflineTarget");

static RoadMapConfigDescriptor RoadMapConfigUser =
                                  ROADMAP_CONFIG_ITEM("Realtime", "Name");

static RoadMapConfigDescriptor RoadMapConfigPassword =
                                  ROADMAP_CONFIG_ITEM("Realtime", "Password");

static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char *RoadMapUploadCurrentName = "";
static int  RoadMapUploadCurrentFileSize = 0;
static int  RoadMapUploadUploaded = -1;


static void encodeblock (unsigned char in[3], unsigned char out[4], int len) {
    out[0] = cb64[ in[0] >> 2 ];
    out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
    out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
    out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
}


static int editor_upload_request (int size) {

   RoadMapUploadCurrentFileSize = size;
   RoadMapUploadUploaded = -1;
   return 1;
}


static void editor_upload_format_size (char *image, int value) {

   if (value > (10 * 1024 * 1024)) {
      sprintf (image, "%dMB", value / (1024 * 1024));
   } else {
      sprintf (image, "%dKB", value / 1024);
   }
}


static void editor_upload_error (const char *format, ...) {

   va_list ap;
   char message[2048];

   va_start(ap, format);
   vsnprintf (message, sizeof(message), format, ap);
   va_end(ap);

   roadmap_messagebox ("Upload Error", message);
}


static void editor_upload_progress (int loaded) {

   char image[32];

   int progress = (100 * loaded) / RoadMapUploadCurrentFileSize;

   /* Avoid updating the dialog too often: this may slowdown the upload. */

   if (progress == RoadMapUploadUploaded) return;

   RoadMapUploadUploaded = progress;

   editor_upload_format_size (image, RoadMapUploadCurrentFileSize);
   editor_upload_format_size (image, loaded);

   roadmap_main_flush ();
}


static RoadMapDownloadCallbacks EditorUploadCallbackFunctions = {
   editor_upload_request,
   editor_upload_progress,
   editor_upload_error
};


static int editor_http_send (RoadMapSocket socket,const char *format, ...)

{
   va_list ap;
   int     length;
   char    buffer[ROADMAP_HTTP_MAX_CHUNK];

   va_start(ap, format);
   vsnprintf (buffer, sizeof(buffer), format, ap);
   va_end(ap);
   length = strlen(buffer);
   if (roadmap_net_send (socket, buffer, length, 1) != length) {
      roadmap_log(ROADMAP_ERROR,"send error on: %s", buffer);
      return -1;
   }
   return length;
}


static int send_auth(const char *user, const char *pw, RoadMapSocket fd) {
    unsigned char in[3], out[4];
    int i, len;
    char auth_string[255];
    char auth_encoded[sizeof(auth_string) * 2];
    char *itr = auth_string;
    char *encoded_itr = auth_encoded;
    snprintf (auth_string, sizeof(auth_string), "%s:%s", user, pw);
    auth_string[sizeof(auth_string) - 1] = 0;
    while(*itr) {
        len = 0;
        for( i = 0; i < 3; i++ ) {
            in[i] = (unsigned char) *itr;
            if (*itr) {
               itr++;
               len++;
            }
        }

        if( len ) {
            encodeblock( in, out, len );
            for( i = 0; i < 4; i++ ) {
                *encoded_itr++ = out[i];
            }
        }
    }
    *encoded_itr = 0;
    if(editor_http_send (fd, "Authorization: Basic %s\r\n", auth_encoded)==-1){
		return -1;
    }
    return 1;
}


static int editor_http_send_header (RoadMapSocket socket,
                                     const char *content_type,
                                     const char *full_name,
                                     size_t size,
                                     const char *user,
                                     const char *pw) {

  const char *filename = roadmap_path_skip_directories (full_name);

  if(send_auth (user, pw, socket)==-1)
	 return -1;

  if (editor_http_send (socket,
		   "Content-Type: multipart/form-data; boundary=---------------------------10424402741337131014341297293\r\n") == -1) {
	 return -1;
  }

  if (editor_http_send (socket,"Content-Length: %d\r\n",
		   size + 237 + strlen(filename) + strlen(content_type) ) == -1) {
	  return -1;
  }

  if (editor_http_send (socket,"\r\n") == -1)
	  return -1;

  if (editor_http_send (socket,
		   "-----------------------------10424402741337131014341297293\r\n") == -1) {
	  return -1;
  }

  if (editor_http_send (socket,
		   "Content-disposition: form-data; name=\"file_0\"; filename=\"%s\"\r\n",
		   filename) == -1) {
	 return -1;
  }

  if (editor_http_send (socket,
		   "Content-type: %s\r\n", content_type ) == -1) {
	 return -1;
  }

  if (editor_http_send (socket,
		   "Content-Transfer-Encoding: binary\r\n\r\n") == -1) {
	 return -1;
  }

  return 1;

}

static void editor_close_socket_file(RoadMapSocket socket, RoadMapFile file){
   if(socket)
	   roadmap_net_close (socket);
   if(file)
	   roadmap_file_close (file);
}

static int editor_http_decode_response (RoadMapSocket fd,
                                        char *buffer,
                                        int  *sizeof_buffer) {

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

         received = roadmap_net_receive
               (fd, buffer + total, *sizeof_buffer - total - 1);

         if (received <= 0) {
            roadmap_log(ROADMAP_ERROR,"Receive error");
            return -1;
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
                  roadmap_log(ROADMAP_ERROR,"Received bad status: %s", buffer);
                  return -1;
               }
               received_status = 1;
            }

         } else {

            if (next == buffer) {

               /* An empty line signals the end of the header. Any
                * reminder data is part of the upload: save it.
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
            	  roadmap_log(ROADMAP_ERROR,"bad formed header: %s", buffer);
                  return -1;
               }

               while (*(++p) == ' ') ;
               size = atoi(p);
               if (size < 0) {
            	  roadmap_log(ROADMAP_ERROR,"bad formed header: %s", buffer);
                  return -1;
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

   roadmap_log(ROADMAP_ERROR,"No valid header received");
   return -1;
}




static void editor_post_file (RoadMapSocket socket,
                             const char *content_type,
                             const char *file_name,
                             const char *user_name,
                             const char *password,
                             EditorUploadCallbacks *callbacks,
                             void * context) {

   int size;
   int loaded;
   int uploaded;
   char buffer[ROADMAP_HTTP_MAX_CHUNK];
   RoadMapFile file;
   char * message;
#ifndef J2ME
   char user_digest_hex[100];
   char pw_digest_hex[100];
#endif


   if (!callbacks){
	   editor_close_socket_file(socket,0);
	   roadmap_log(ROADMAP_ERROR,"NULL callbacks in editor_post_file");
	   return;
   }

   file = roadmap_file_open (file_name, "r");

   if (!ROADMAP_FILE_IS_VALID(file)) {
	  editor_close_socket_file(socket,file);
	  roadmap_log(ROADMAP_ERROR,"Could not open file %s for upload",file_name);
      (*callbacks->error) (context);
      return;
   }

   size = roadmap_file_length (NULL, file_name);
   if ( !(*callbacks->size) (context,size)) {
	  editor_close_socket_file(socket,file);
	  roadmap_log(ROADMAP_ERROR,"File %s did not pass size check ( too big for sending? )",file_name);
	  (*callbacks->error)(context);
	  return;
   }

   RoadMapUploadCurrentName = file_name;

#ifndef J2ME
   if (!user_name[0]) {
      struct MD5Context md5context;
      unsigned char digest[16];
      if (roadmap_net_unique_id (digest, sizeof(digest)) != sizeof(digest)) {
    	 editor_close_socket_file(socket,file);
    	 roadmap_log(ROADMAP_ERROR,"could not find unique md5 id for file %s\n",file_name);
         (*callbacks->error)(context);
         return;
      }

      strcpy(user_digest_hex, "anon_");
      MD5Hex (digest, user_digest_hex + strlen(user_digest_hex));

      user_name = user_digest_hex;

      MD5Init (&md5context);
      MD5Update (&md5context, (unsigned char *)user_name, strlen(user_name));
      MD5Final (digest, &md5context);
      MD5Hex (digest, pw_digest_hex);

      password = pw_digest_hex;
   }
#endif

   if (-1 == editor_http_send_header(socket,content_type, file_name, size,user_name, password)){
	   editor_close_socket_file(socket,file);
	   (*callbacks->error)(context);
	   return;
   }

   uploaded = 0;
   (*callbacks->progress) (context,uploaded);

   loaded = uploaded;
   while (loaded < size) {
      uploaded = roadmap_file_read (file, buffer, sizeof(buffer));
      uploaded = roadmap_net_send (socket, buffer, uploaded, 1);
      if (uploaded <= 0) {
    	 editor_close_socket_file(socket,file);
    	 roadmap_log(ROADMAP_ERROR,"Send error after %d data bytes", loaded);
         (*callbacks->error) (context);
         return;
      }
      loaded += uploaded;
      (*callbacks->progress) (context,loaded);
   }


   if(editor_http_send (socket,"\r\n-----------------------------10424402741337131014341297293--\r\n")==-1){
	   editor_close_socket_file(socket,file);
	   (*callbacks->error) (context);
	   return;
   }

   loaded = sizeof(buffer);

   size = editor_http_decode_response (socket, buffer, &loaded);

   if (size < 0) {
	  editor_close_socket_file(socket,file);
	  roadmap_log(ROADMAP_ERROR,"Upload file error after decoding response", loaded);
	  (*callbacks->error) (context);
	  return;
   }

   if (size > 1) {
      message = malloc(size+1);
      memcpy (message, buffer, loaded);

      while (loaded < size) {
         int r;
         r = roadmap_net_receive (socket, message + loaded, size - loaded);

         if (r <= 0) break;

         loaded += r;
      }

      message[loaded] = '\0';
      editor_close_socket_file(socket,file);
      (*callbacks->done)(context,message);
      free(message);
   }else{
	  editor_close_socket_file(socket,file);
	  buffer[loaded] = 0;
	  (*callbacks->done)(context,NULL);
   }
   return;
}



void editor_upload_initialize (void) {

   roadmap_config_declare
      ("preferences",
      &RoadMapConfigTarget,
      "", NULL);

   roadmap_config_declare ("user", &RoadMapConfigUser, "", NULL);
   roadmap_config_declare_password ("user", &RoadMapConfigPassword, "");
}


/*
 * This function gets called after async socket connect - D.F.
 */
void editor_upload_on_socket_connected(RoadMapSocket socket, void *context, roadmap_result err){

   const char * user = roadmap_config_get (&RoadMapConfigUser);
   const char * password = roadmap_config_get (&RoadMapConfigPassword);
   editor_upload_context * upload_context = (editor_upload_context *)context;
	// Content type

   if (!ROADMAP_NET_IS_VALID(socket)) {
	   roadmap_log( ROADMAP_ERROR, "Couldn't open socket for file %s\n", upload_context->file_name);
	   (*upload_context->callbacks->error) (upload_context->caller_specific_context);
	   free(upload_context->custom_content_type);
	   free(upload_context->file_name);
	   free(upload_context);
	   return;
   }

   editor_post_file (socket,
					 upload_context->custom_content_type,
					 upload_context->file_name,
					 user,
					 password,
					 upload_context->callbacks,
					 upload_context->caller_specific_context);

   free(upload_context->custom_content_type);
   free(upload_context->file_name);
   free(upload_context);

}

/*
 * Starts an async transaction - Returns false if could not even start the transaction.
 * Flow continues after socket connection to editor_upload_on_socket_connected.
 * The caller is able to respond to the progress of the file upload, through the input param 'callbacks' - D.F.
 */
int editor_upload_auto (const char *filename,
                        EditorUploadCallbacks *callbacks,
                        const char* custom_target, const char* custom_content_type, void * context ) {

	const char *url;
	editor_upload_context * upload_context = malloc(sizeof(editor_upload_context));
	upload_context->callbacks = callbacks;
	upload_context->file_name = strdup(filename);
	upload_context->caller_specific_context  = context;

	// Content type
	if ( custom_content_type )
		upload_context->custom_content_type = strdup(custom_content_type);
	else
		upload_context->custom_content_type = strdup(DEFAULT_CONTENT_TYPE);

	// Custom target
	if ( custom_target )
		url = custom_target;
	else
		url = roadmap_config_get (&RoadMapConfigTarget);

	// Start the async-connect process:
	if( -1 == roadmap_net_connect_async("http_post",url,0,80,0,editor_upload_on_socket_connected, upload_context))
	{
		free(upload_context->custom_content_type);
		free(upload_context->file_name);
		free(upload_context);
		roadmap_log( ROADMAP_ERROR, "editor_upload_auto() - 'roadmap_net_connect_async' had failed, file : %s, url : %s", filename,url);
		return -1;
    }
    return 0;
}


