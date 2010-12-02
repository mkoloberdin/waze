/* roadmap_zlib.c - Compress / Decompress files
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
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
 *   See roadmap_zlib.h
 */

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_zlib.h"


#ifdef __SYMBIAN32__
#define CHUNK 2048
#else
#define CHUNK 16384
#endif


///////////////////////////////////////////////////////
// Compress (gzip) in_file to out_file
int roadmap_zlib_compress (const char* in_path, const char* in_file, const char* out_path, const char* out_file, int level, BOOL isLogFile) {

   int ret, flush;
   unsigned have;
   z_stream strm;
   unsigned char in[CHUNK];
   unsigned char out[CHUNK];
   FILE *source = NULL;
   FILE *dest = NULL;

   /* allocate deflate state */
   strm.zalloc = Z_NULL;
   strm.zfree = Z_NULL;
   strm.opaque = Z_NULL;
   //ret = deflateInit(&strm, level);
   ret = deflateInit2(&strm, level, Z_DEFLATED, 15+16, 8, Z_DEFAULT_STRATEGY);
   if (ret != Z_OK)
      return ret;

#ifdef __SYMBIAN32__
   if (isLogFile)
   // symbian does not support double opening of file, so get the already open log file
   		source = roadmap_log_get_log_file();
   else
   		source = roadmap_file_fopen (in_path, in_file, "r");
#else
   source = roadmap_file_fopen (in_path, in_file, "r");

#endif
   if (!source) {
         roadmap_log(ROADMAP_ERROR, "Error openning file for read: %s%s", in_path, in_file);
         return 0;
   }

   if (isLogFile){
#ifndef __SYMBIAN32__
      if ( roadmap_verbosity() <= ROADMAP_MESSAGE_DEBUG )
         fseek(source,-5000000,SEEK_END); //In debug send only 5000K
      else
#endif
         fseek(source,-200000,SEEK_END); //Send only 200K
   }

#ifdef WIN32
   dest = roadmap_file_fopen (out_path, out_file, "wb");
#else
   dest = roadmap_file_fopen (out_path, out_file, "w");
#endif //WIN32
   if (!dest) {
      roadmap_log(ROADMAP_ERROR, "Error openning file for write: %s%s", out_path, out_file);
      return 0;
   }

   /* compress until end of file */
   do {
      strm.avail_in = fread(in, 1, CHUNK, source);
      if (ferror(source)) {
         (void)deflateEnd(&strm);
         return Z_ERRNO;
      }
      flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
      strm.next_in = in;

      /* run deflate() on input until output buffer not full, finish
       compression if all of source has been read in */
      do {
         strm.avail_out = CHUNK;
         strm.next_out = out;
         ret = deflate(&strm, flush);    /* no bad return value */
         assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
         have = CHUNK - strm.avail_out;
         if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
         }
      } while (strm.avail_out == 0);
      assert(strm.avail_in == 0);     /* all input will be used */

      /* done when last data in file processed */
   } while (flush != Z_FINISH);
   assert(ret == Z_STREAM_END);        /* stream will be complete */

   /* clean up and return */
   (void)deflateEnd(&strm);
#ifndef __SYMBIAN32__
   fclose(source); // in symbian it's still the original
#endif
   fclose(dest);

   return Z_OK;
}


///////////////////////////////////////////////////////
// Decompress using zlib
// not implemented
