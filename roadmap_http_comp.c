/* roadmap_http_comp.c - HTTP compression support
 *
 * LICENSE:
 *
 *   Copyright 2010 Ehud Shabtai
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

#include "stdlib.h"
#include "string.h"
#include "zlib.h"
#include "roadmap_http_comp.h"

/* states */
#define STATE_READ_HEADERS        1
#define STATE_SEND_HEADERS        2
#define STATE_NO_COMPRESSION_FLOW 3
#define STATE_DEFLATE_READ_HEADER 4
#define STATE_COMPRESSION_FLOW    5

static const char deflate_magic[2] = {'\037', '\213' };

/* flags */
static const int FLAG_COMPRESSED = 0x1;
static const int FLAG_ERROR      = 0x10;

/* Buffer must be big enough to fit a complete http header */
#define READ_BUFFER_SIZE 1024

struct roadmap_http_comp_t {
   int state;
   int flags;

   char buffer[READ_BUFFER_SIZE+1];
   int buffer_ptr;
   int read_ptr;
   int read_size;

   z_stream stream;
   unsigned int crc;
};

static const int ZLIB_WIN_SIZE = 15;

/* process http headers and see if we got a compressed response.
 * NOTE: we assume that the whole header can fit our buffer
 */
static void process_headers (RoadMapHttpCompCtx ctx) {
   char *ptr;
   if ((ptr = strstr(ctx->buffer, "\r\n\r\n")) != NULL) {

      /* We got all the headers so we can now send to the consumer */
      ctx->state = STATE_SEND_HEADERS;
      ctx->read_size = ptr - ctx->buffer + 4;

      if (strstr(ctx->buffer, "Content-Encoding: gzip") != NULL) {
         ctx->flags |= FLAG_COMPRESSED;
      }
   }
}

RoadMapHttpCompCtx roadmap_http_comp_init (void) {

   int res;
   RoadMapHttpCompCtx ctx = (RoadMapHttpCompCtx) calloc(1, sizeof(struct roadmap_http_comp_t));

   roadmap_check_allocated(ctx);

   if ((res = inflateInit2( &ctx->stream, -1 * ZLIB_WIN_SIZE )) != Z_OK) {
      roadmap_log(ROADMAP_ERROR, "Error initializing deflate - %d\n", res);
      inflateEnd(&ctx->stream);
      free(ctx);
      return NULL;
   }

   ctx->state = STATE_READ_HEADERS;

   return ctx;
}

void roadmap_http_comp_get_buffer (RoadMapHttpCompCtx ctx,
      void **ctx_buffer, int *ctx_buffer_size) {

   *ctx_buffer = ctx->buffer + ctx->buffer_ptr;
   *ctx_buffer_size = READ_BUFFER_SIZE - ctx->buffer_ptr;
}

/* Called by the network layer when new data is read from the socket */
void roadmap_http_comp_add_data (RoadMapHttpCompCtx ctx, int received) {

   if (received == -1) {
      ctx->flags |= FLAG_ERROR;
      return;
   } else {
      ctx->buffer_ptr += received;
      assert(ctx->buffer_ptr <= READ_BUFFER_SIZE);
      ctx->buffer[ctx->buffer_ptr] = '\0';
   }

   if (ctx->state == STATE_READ_HEADERS) {
      process_headers(ctx);
   }
}

int roadmap_http_comp_read (RoadMapHttpCompCtx ctx, void *data, int size) {

   int res;

   if (ctx->flags & FLAG_ERROR) return -1;

   switch (ctx->state) {

      case STATE_READ_HEADERS:

         /* we need more data */
         return 0;

      case STATE_SEND_HEADERS:
         if (size > (ctx->read_size - ctx->read_ptr)) {
            size = ctx->read_size - ctx->read_ptr;
         }
            
         memcpy(data, ctx->buffer + ctx->read_ptr, size);
         ctx->read_ptr += size;
         if (ctx->read_ptr == ctx->read_size) {
            ctx->read_size = ctx->buffer_ptr;

            if (! (ctx->flags & FLAG_COMPRESSED)) {
               ctx->state = STATE_NO_COMPRESSION_FLOW;
            } else {
               ctx->state = STATE_DEFLATE_READ_HEADER;
            }
         }
         return size;

      case STATE_NO_COMPRESSION_FLOW:
         if (ctx->buffer_ptr != ctx->read_ptr) {

            if (size > (ctx->buffer_ptr - ctx->read_ptr)) {
               size = ctx->buffer_ptr - ctx->read_ptr;
            }

            memcpy(data, ctx->buffer + ctx->read_ptr, size);
            ctx->read_ptr += size;
            if (ctx->read_ptr == ctx->buffer_ptr) {
               ctx->read_ptr = ctx->buffer_ptr = 0;
            }
            return size;

         } else {
            /* need more data */
            return 0;
         }

      case STATE_DEFLATE_READ_HEADER:
         if ((ctx->buffer_ptr - ctx->read_ptr) < 10) {
            /* Gzip Header size is 10 bytes. */
            return 0;
         }

         /* Check Gzip header. We don't support also ext. header. */
         if ((memcmp(ctx->buffer + ctx->read_ptr, deflate_magic, 2) != 0) ||
              (ctx->buffer[ctx->read_ptr + 3] != 0 )) {

            roadmap_log(ROADMAP_ERROR, "Unsupported gzip header.");
            ctx->flags |= FLAG_ERROR;
            return -1;
         }
            
         /* Ok, eat the header! */
         ctx->read_ptr += 10;
         ctx->state = STATE_COMPRESSION_FLOW;

         /* fall through to STATE_COMPRESSION_FLOW */

      case STATE_COMPRESSION_FLOW:

         if (ctx->read_ptr == ctx->buffer_ptr) {
            /* need more data */
            ctx->read_ptr = ctx->buffer_ptr = 0;
            return 0;
         }

         /* Decompress received data */
         ctx->stream.next_in   = (unsigned char *) (ctx->buffer + ctx->read_ptr);
         ctx->stream.avail_in  = ctx->buffer_ptr - ctx->read_ptr;
         ctx->stream.next_out  = data;
         ctx->stream.avail_out = size;

         res = inflate( &ctx->stream, Z_SYNC_FLUSH );

         if ((res == Z_STREAM_END) || (res == Z_OK)) {
            /* We are not verifying CRC, ignoring data leftovers, etc.
             * Being lazy saves power :)
             */
            ctx->read_ptr = ctx->buffer_ptr - ctx->stream.avail_in;
            size = size - ctx->stream.avail_out;
         } else {
            roadmap_log(ROADMAP_ERROR, "Error in inflate - %d", res);
            ctx->flags |= FLAG_ERROR;
            return -1;
         }

         /* Ignore compressed stream trailer */
         if (res == Z_STREAM_END) {
            ctx->read_ptr = ctx->buffer_ptr;
         }

         return size;
   }

   return -1;
}

int roadmap_http_comp_avail (RoadMapHttpCompCtx ctx) {
   return ctx->stream.avail_out;
}

void roadmap_http_comp_close (RoadMapHttpCompCtx ctx) {
	if (ctx) free(ctx);
}

