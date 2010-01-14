/* roadmap_download.c - Download RoadMap maps.
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
 * SYNOPSYS:
 *
 *   See roadmap_download.h
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#ifndef J2ME
#include <zlib.h>
#endif

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_dbread.h"
#include "roadmap_hash.h"
#include "roadmap_path.h"
#include "roadmap_file.h"
#include "roadmap_config.h"
#include "roadmap_gui.h"
#include "roadmap_county.h"
#include "roadmap_screen.h"
#include "roadmap_locator.h"
#include "roadmap_messagebox.h"
#include "roadmap_dialog.h"
#include "roadmap_start.h"
#include "roadmap_square.h"
#include "roadmap_main.h"
#include "roadmap_preferences.h"
#include "roadmap_spawn.h"
#include "roadmap_db.h"

#include "navigate/navigate_main.h"
#include "Realtime/RealtimeTrafficInfo.h"

#include "roadmap_download.h"


#define FREEMAP_FILE_NAME_FORMAT "map%05d.gzm"


static RoadMapConfigDescriptor RoadMapConfigSource =
                                  ROADMAP_CONFIG_ITEM("Download", "Source");

static RoadMapConfigDescriptor RoadMapConfigDestination =
                                  ROADMAP_CONFIG_ITEM("Download", "Destination");

struct roadmap_download_tool {
   char *suffix;
   char *name;
};

#if 0
static struct roadmap_download_tool RoadMapDownloadCompressTools[] = {
   {".gz",  "gunzip -f"},
   {".bz2", "bunzip2 -f"},
   {".lzo", "lzop -d -Uf"},
   {NULL, NULL}
};
#endif

static RoadMapHash *RoadMapDownloadBlock = NULL;
static int *RoadMapDownloadBlockList = NULL;
static int  RoadMapDownloadBlockCount = 0;

static int *RoadMapDownloadQueue = NULL;
static int  RoadMapDownloadQueueConsumer = 0;
static int  RoadMapDownloadQueueProducer = 0;
static int  RoadMapDownloadQueueSize = 0;

static int  RoadMapDownloadRefresh = 0;

static int  RoadMapDownloadCurrentFileSize = 0;
static int  RoadMapDownloadDownloaded = -1;

static const char *RoadMapDownloadFrom;
static const char *RoadMapDownloadTo;


struct roadmap_download_protocol {

   struct roadmap_download_protocol *next;

   const char *prefix;
   RoadMapDownloadProtocol handler;
};

static struct roadmap_download_protocol *RoadMapDownloadProtocolMap = NULL;


static int roadmap_download_next_county (RoadMapDownloadCallbacks *callbacks);

static void roadmap_download_no_handler (void) {}

static RoadMapDownloadEvent RoadMapDownloadWhenDone =
                               roadmap_download_no_handler;



static int roadmap_download_request (int size) {

   /* TBD: for the time being, answer everything is fine. */

   RoadMapDownloadCurrentFileSize = size;
   RoadMapDownloadDownloaded = -1;
   return 1;
}


static void roadmap_download_format_size (char *image, int value) {

   if (value > (10 * 1024 * 1024)) {
      sprintf (image, "%dMB", value / (1024 * 1024));
   } else {
      sprintf (image, "%dKB", value / 1024);
   }
}


static void roadmap_download_error (const char *format, ...) {

   va_list ap;
   char message[2048];

   va_start(ap, format);
   vsnprintf (message, sizeof(message), format, ap);
   va_end(ap);

   roadmap_messagebox ("Download Error", message);
}


static int roadmap_download_increment (int cursor) {

   if (++cursor >= RoadMapDownloadQueueSize) {
      cursor = 0;
   }
   return cursor;
}


static int roadmap_download_end (RoadMapDownloadCallbacks *callbacks) {


   RoadMapDownloadQueueConsumer =
      roadmap_download_increment (RoadMapDownloadQueueConsumer);

   if (RoadMapDownloadQueueConsumer != RoadMapDownloadQueueProducer) {

      /* The queue is not yet empty: start the next download. */
      if (roadmap_download_next_county (callbacks) != 0) {
         RoadMapDownloadWhenDone ();
         return -1;
      }

   } else if (RoadMapDownloadRefresh) {

      /* The queue is empty: tell the final consumer, but only
       * if there was at least one successful download.
       */
      RoadMapDownloadRefresh = 0;
      roadmap_dialog_hide ("Downloading");
      RoadMapDownloadWhenDone ();
   } else {
      roadmap_dialog_hide ("Downloading");
      RoadMapDownloadWhenDone ();
   }

   return 0;
}


static void roadmap_download_progress (int loaded) {

   int  fips;
   char image[32];

   int progress = (100 * loaded) / RoadMapDownloadCurrentFileSize;


   /* Avoid updating the dialog too often: this may slowdown the download. */

   if (progress == RoadMapDownloadDownloaded) return;

   RoadMapDownloadDownloaded = progress;


   fips = RoadMapDownloadQueue[RoadMapDownloadQueueConsumer];

   if (roadmap_dialog_activate ("Downloading", NULL, 1)) {

      roadmap_dialog_new_label  (".file", "County");
      roadmap_dialog_new_label  (".file", "State");
      roadmap_dialog_new_label  (".file", "Size");
      roadmap_dialog_new_label  (".file", "Download");
      roadmap_dialog_new_label  (".file", "Status");

      roadmap_dialog_complete (0);
   }

   if (fips != -1) {
      roadmap_dialog_set_data (".file", "County",
            roadmap_county_get_name (fips));
      roadmap_dialog_set_data (".file", "State",
            roadmap_county_get_state (fips));
   } else {
      roadmap_dialog_set_data (".file", "County", "usdir");
      roadmap_dialog_set_data (".file", "State", "");
   }

   roadmap_dialog_set_data (".file", "Status", "Downloading...");
   roadmap_download_format_size (image, RoadMapDownloadCurrentFileSize);
   roadmap_dialog_set_data (".file", "Size", image);

   if (loaded == RoadMapDownloadCurrentFileSize) {
      roadmap_dialog_set_data (".file", "Status", "Completed.");
   } else {
      roadmap_download_format_size (image, loaded);
      roadmap_dialog_set_data (".file", "Download", image);
   }

   roadmap_main_flush ();
}


static RoadMapDownloadCallbacks RoadMapDownloadCallbackFunctions = {
   roadmap_download_request,
   roadmap_download_progress,
   roadmap_download_error
};


static void roadmap_download_allocate (void) {

   if (RoadMapDownloadQueue == NULL) {

      RoadMapDownloadQueueSize = roadmap_county_count() + 5;

      RoadMapDownloadQueue =
         calloc (RoadMapDownloadQueueSize, sizeof(int));
      roadmap_check_allocated(RoadMapDownloadQueue);

      RoadMapDownloadBlockList = calloc (RoadMapDownloadQueueSize, sizeof(int));
      roadmap_check_allocated(RoadMapDownloadBlockList);

      RoadMapDownloadBlock =
         roadmap_hash_new ("download", RoadMapDownloadQueueSize);
   }
}


void roadmap_download_block (int fips) {

   int i;
   int candidate = -1;

   roadmap_download_allocate ();

   /* Check if the county was not already locked. While doing this,
    * detect any unused slot in that hash list that we might reuse.
    */
   for (i = roadmap_hash_get_first (RoadMapDownloadBlock, fips);
        i >= 0;
        i = roadmap_hash_get_next (RoadMapDownloadBlock, i)) {

      if (RoadMapDownloadBlockList[i] == fips) {
         return; /* Already done. */
      }
      if (RoadMapDownloadBlockList[i] == 0) {
         candidate = i;
      }
   }

   if (candidate >= 0) {
      /* This county is not yet blocked, and there is a free slot
       * for us to reuse in the right hash list.
       */
      RoadMapDownloadBlockList[candidate] = fips;
      return;
   }

   /* This county was not locked and there was no free slot: we must create
    * a new one.
    */
   if (RoadMapDownloadBlockCount < RoadMapDownloadQueueSize) {

      roadmap_hash_add (RoadMapDownloadBlock,
                        fips,
                        RoadMapDownloadBlockCount);

      RoadMapDownloadBlockList[RoadMapDownloadBlockCount] = fips;
      RoadMapDownloadBlockCount += 1;
   }
}


void roadmap_download_unblock (int fips) {

   int i;

   if (RoadMapDownloadQueue == NULL)  return;

   for (i = roadmap_hash_get_first (RoadMapDownloadBlock, fips);
        i >= 0;
        i = roadmap_hash_get_next (RoadMapDownloadBlock, i)) {

      if (RoadMapDownloadBlockList[i] == fips) {
         RoadMapDownloadBlockList[i] = 0;
      }
   }
}


void roadmap_download_unblock_all (void) {

   int i;

   if (RoadMapDownloadQueue == NULL)  return;

   for (i = RoadMapDownloadQueueSize - 1; i >= 0; --i) {
      RoadMapDownloadBlockList[i] = 0;
   }
}


int roadmap_download_blocked (int fips) {

   int i;

   roadmap_download_allocate ();

   for (i = roadmap_hash_get_first (RoadMapDownloadBlock, fips);
        i >= 0;
        i = roadmap_hash_get_next (RoadMapDownloadBlock, i)) {

      if (RoadMapDownloadBlockList[i] == fips) {
         return 1;
      }
   }
   return 0;
}


static int roadmap_download_uncompress (const char *tmp_file,
                                        const char *destination) {
#ifdef J2ME
	return 0;
#else
   char *p;
   RoadMapFile source_file = 0;
   gzFile      source_file_gz = NULL;
   RoadMapFile dest_file;
   char *dest_name = strdup(destination);
   int is_gz_file = 0;
   int res = 0;
   char buffer[1024];
   int count;

   p = strstr (dest_name, ".gz");

   if ((p != NULL) && (strcmp (p, ".gz") == 0)) {
      is_gz_file = 1;
      *p = '\0';
      source_file_gz = gzopen(tmp_file, "rb");
      if (!source_file_gz) res = -1;

   } else {

      source_file = roadmap_file_open (tmp_file, "r");
      if (!ROADMAP_FILE_IS_VALID (source_file)) res = -1;
   }

   if (res != 0) {
      free(dest_name);
      roadmap_file_remove (NULL, tmp_file);
      return res;
   }

   roadmap_file_remove (NULL, dest_name);
   dest_file = roadmap_file_open (dest_name, "w");
   if (!ROADMAP_FILE_IS_VALID (dest_file)) {
      res = -1;
      goto end;
   }

   while (1) {

      if (is_gz_file) {
         count = gzread (source_file_gz, buffer, sizeof(buffer));
      } else {
         count = roadmap_file_read (source_file, buffer, sizeof(buffer));
      }

      if (count <= 0) {
         res = count;
         break;
      }

      if (roadmap_file_write (dest_file, buffer, count) != count) {
         res = -1;
         break;
      }
   }

end:
   free(dest_name);

   if (is_gz_file) {
      gzclose (source_file_gz);
   } else {
      roadmap_file_close (source_file);
   }

   if (ROADMAP_FILE_IS_VALID (dest_file)) {
      roadmap_file_close (dest_file);
   }

   return res;
#endif
}


static int roadmap_download_start (const char *name,
                                   RoadMapDownloadCallbacks *callbacks) {

   int  fips = RoadMapDownloadQueue[RoadMapDownloadQueueConsumer];
   struct roadmap_download_protocol *protocol;
   int error = 0;

   char source[256];
   char destination[256];
   const char *tmp_file;

   const char *format;
   const char *directory;

   format = RoadMapDownloadFrom;
   roadmap_config_set (&RoadMapConfigSource, format);
   snprintf (source, sizeof(source), format, fips);

   format = RoadMapDownloadTo;
   snprintf (destination, sizeof(destination), format, fips);

   if (!callbacks) {
      roadmap_dialog_hide (name);
      callbacks = &RoadMapDownloadCallbackFunctions;
   }


   directory = roadmap_path_parent (NULL, destination);
   roadmap_path_create (directory);
   tmp_file = roadmap_path_join (directory, "dl_tmp.dat");
   roadmap_path_free (directory);

   roadmap_file_remove (NULL, tmp_file);

   /* Search for the correct protocol handler to call this time. */

   for (protocol = RoadMapDownloadProtocolMap;
        protocol != NULL;
        protocol = protocol->next) {

      if (strncmp (source, protocol->prefix, strlen(protocol->prefix)) == 0) {

         if (protocol->handler (callbacks, source, tmp_file)) {

            navigate_main_stop_navigation();
            roadmap_locator_close (fips);
            roadmap_file_remove (NULL, destination);
            roadmap_file_rename (tmp_file, destination);
            if (roadmap_locator_activate (fips) == ROADMAP_US_OK) {
               roadmap_square_rebuild_index();
               RTTrafficInfo_RecalculateSegments();
               RoadMapDownloadRefresh = 1;
            } else {
               error = 1;
            }
         } else {
            error = 1;

            /* empty queue */
            RoadMapDownloadQueueConsumer = RoadMapDownloadQueueProducer - 1;
         }
         roadmap_download_unblock (fips);
         roadmap_download_end (callbacks);
         break;
      }
   }

   roadmap_file_remove (NULL, tmp_file);
   roadmap_path_free (tmp_file);

   if (protocol == NULL) {

      roadmap_messagebox ("Download Error", "invalid download protocol");
      roadmap_log (ROADMAP_WARNING, "invalid download source %s", source);
   }

   if (RoadMapDownloadCurrentFileSize > 0) {
      roadmap_download_progress (RoadMapDownloadCurrentFileSize);
   }

   return error;
}


static void roadmap_download_ok (const char *name, void *context) {

   RoadMapDownloadFrom = roadmap_dialog_get_data (".file", "From");
   roadmap_config_set (&RoadMapConfigSource, RoadMapDownloadFrom);

   RoadMapDownloadTo = roadmap_dialog_get_data (".file", "To");

   roadmap_download_start (name, NULL);
}


static void roadmap_download_cancel (const char *name, void *context) {

   roadmap_dialog_hide (name);

   roadmap_download_block (RoadMapDownloadQueue[RoadMapDownloadQueueConsumer]);

   /* empty queue */
   RoadMapDownloadQueueConsumer = RoadMapDownloadQueueProducer - 1;

   roadmap_download_end (0);
}


static int roadmap_download_usdir (RoadMapDownloadCallbacks *callbacks) {

   struct roadmap_download_protocol *protocol;
   int error = 0;

   char source[256];
   char destination[256];

   char *format;
   const char *directory;
   const char *tmp_file;

   if (!callbacks) callbacks = &RoadMapDownloadCallbackFunctions;

   strncpy_safe (source, roadmap_config_get (&RoadMapConfigSource), sizeof(source));

   format = strrchr (source, '/');
   if (!format) {
      roadmap_messagebox ("Download Error", "Can't download usdir.");
      roadmap_log (ROADMAP_WARNING, "invalid download source %s", source);
      roadmap_download_end (callbacks);
      return -1;
   }

   strncpy_safe (format+1, "usdir.rdm", sizeof(source) - (format - source + 1));

#ifndef _WIN32
   snprintf (destination, sizeof(destination), "%s/usdir.rdm",
         roadmap_config_get (&RoadMapConfigDestination));
#else
   snprintf (destination, sizeof(destination), "%s\\usdir.rdm",
         roadmap_config_get (&RoadMapConfigDestination));
#endif

   directory = roadmap_path_parent (NULL, destination);
   roadmap_path_create (directory);

   tmp_file = roadmap_path_join (directory, "dl_tmp.dat");
   roadmap_path_free (directory);

   /* Search for the correct protocol handler to call this time. */

   for (protocol = RoadMapDownloadProtocolMap;
        protocol != NULL;
        protocol = protocol->next) {

      if (strncmp (source, protocol->prefix, strlen(protocol->prefix)) == 0) {

         roadmap_start_freeze ();
#if 0
         roadmap_locator_close_dir();
#endif

         if (protocol->handler (callbacks, source, tmp_file)) {

            roadmap_download_uncompress (tmp_file, destination);
            RoadMapDownloadRefresh = 1;
         } else {
            error = -1;
         }
         roadmap_start_unfreeze ();
         roadmap_download_end (callbacks);
         break;
      }
   }

   roadmap_path_free (tmp_file);

   if (protocol == NULL) {

      roadmap_messagebox ("Download Error", "invalid download protocol");
      roadmap_log (ROADMAP_WARNING, "invalid download source %s", source);
      roadmap_download_end (callbacks);
      return -1;
   }

   if (RoadMapDownloadCurrentFileSize > 0) {
      roadmap_download_progress (RoadMapDownloadCurrentFileSize);
   }

   return error;
}


int roadmap_download_next_county (RoadMapDownloadCallbacks *callbacks) {

   int fips = RoadMapDownloadQueue[RoadMapDownloadQueueConsumer];

   const char *source;
   const char *basename;
   char *dest;

   int res;

   if (fips == -1) {
      return roadmap_download_usdir (callbacks);
   }

   source = roadmap_config_get (&RoadMapConfigSource);
   basename = strrchr (source, '/');
   if (basename == NULL) {
      roadmap_messagebox (source, "Bad source file name (no path)");
      roadmap_download_end (callbacks);
      return -1;
   }

   if (!callbacks) {
      if (roadmap_dialog_activate ("Download a Map", NULL, 1)) {

         roadmap_dialog_new_label  (".file", "County");
         roadmap_dialog_new_label  (".file", "State");
         roadmap_dialog_new_entry  (".file", "From", NULL);
         roadmap_dialog_new_entry  (".file", "To", NULL);

         roadmap_dialog_add_button ("OK", roadmap_download_ok);
         roadmap_dialog_add_button ("Cancel", roadmap_download_cancel);

         roadmap_dialog_complete (roadmap_preferences_use_keyboard());
      }
      roadmap_dialog_set_data (".file", "County", roadmap_county_get_name (fips));
      roadmap_dialog_set_data (".file", "State", roadmap_county_get_state (fips));
   }

   RoadMapDownloadFrom = source;
   roadmap_dialog_set_data (".file", "From", source);

   dest = roadmap_path_join(roadmap_config_get (&RoadMapConfigDestination), basename + 1);

   RoadMapDownloadTo = dest;

#ifdef __SYMBIAN32__
   res = roadmap_download_start ("Download a Map", callbacks);
#else
   if (!callbacks) {
      roadmap_dialog_set_data (".file", "To", dest);
      res = 0;
   } else {
      res = roadmap_download_start ("Download a Map", callbacks);
   }
#endif

   roadmap_path_free(dest);
   return res;
}


int roadmap_download_get_county (int fips, int download_usdir,
                                 RoadMapDownloadCallbacks *callbacks) {

   int next;

   if (RoadMapDownloadWhenDone == roadmap_download_no_handler) return 0;


   /* Check that we did not refuse to download that county already.
    * If not, set a block, which we will release when done. This is to
    * avoid requesting a download while we are downloading this county.
    */
   if (roadmap_download_blocked (fips)) return -1;

   roadmap_download_block (fips);


   /* Add this county to the download request queue. */

   next = roadmap_download_increment(RoadMapDownloadQueueProducer);

   if (next == RoadMapDownloadQueueConsumer) {
      /* The queue is full: stop downloading more. */
      return -1;
   }

   RoadMapDownloadQueue[RoadMapDownloadQueueProducer] = fips;

   if (download_usdir) {

      next = roadmap_download_increment(next);

      if (next != RoadMapDownloadQueueConsumer) {
         RoadMapDownloadQueue[next-1] = -1;
      }
   }

   if (RoadMapDownloadQueueProducer == RoadMapDownloadQueueConsumer) {

      /* The queue was empty: start downloading now. */

      RoadMapDownloadQueueProducer = next;
      return roadmap_download_next_county(callbacks);
   }

   RoadMapDownloadQueueProducer = next;
   return 0;
}


/* -------------------------------------------------------------------------
 * Show map statistics: number of files, disk space occupied.
 */

static void roadmap_download_compute_space (int *count, int *size) {

   char **files;
   char **cursor;
   const char *directory = roadmap_config_get (&RoadMapConfigDestination);

   files = roadmap_path_list (directory, ".rdm");

   for (cursor = files, *size = 0, *count = 0; *cursor != NULL; ++cursor) {
      if (strcmp (*cursor, "usdir.rdm")) {
         *size  += roadmap_file_length (directory, *cursor);
         *count += 1;
      }
   }

   roadmap_path_list_free (files);
}


static void roadmap_download_show_space_ok (const char *name, void *context) {
   roadmap_dialog_hide (name);
}


void roadmap_download_show_space (void) {

   int  size;
   int  count;
   char image[32];


   if (roadmap_dialog_activate ("Disk Usage", NULL, 1)) {

      roadmap_dialog_new_label  (".file", "Files");
      roadmap_dialog_new_label  (".file", "Space");

      roadmap_dialog_add_button ("OK", roadmap_download_show_space_ok);

      roadmap_dialog_complete (0);
   }

   roadmap_dialog_set_data   (".file", ".title", "Map statistics:");

   roadmap_download_compute_space (&count, &size);

   sprintf (image, "%d", count);
   roadmap_dialog_set_data (".file", "Files", image);

   roadmap_download_format_size (image, size);
   roadmap_dialog_set_data (".file", "Space", image);
}


/* -------------------------------------------------------------------------
 * Select & delete map files.
 */
static int   *RoadMapDownloadDeleteFips = NULL;

static char **RoadMapDownloadDeleteNames = NULL;
static int    RoadMapDownloadDeleteCount = 0;
static int    RoadMapDownloadDeleteSelected = 0;


static void roadmap_download_delete_free (void) {

   int i;

   if (RoadMapDownloadDeleteNames != NULL) {

      for (i = RoadMapDownloadDeleteCount-1; i >= 0; --i) {
         free (RoadMapDownloadDeleteNames[i]);
      }
      free (RoadMapDownloadDeleteNames);
   }

   RoadMapDownloadDeleteNames = NULL;
   RoadMapDownloadDeleteCount = 0;
   RoadMapDownloadDeleteSelected = -1;
}


static void roadmap_download_delete_done (const char *name, void *context) {

   roadmap_dialog_hide (name);
   roadmap_download_delete_free ();
}


static void roadmap_download_delete_selected (const char *name, void *data) {

   int i;

   char *map_name = (char *) roadmap_dialog_get_data (".delete", ".maps");

   for (i = RoadMapDownloadDeleteCount-1; i >= 0; --i) {
      if (! strcmp (map_name, RoadMapDownloadDeleteNames[i])) {
         RoadMapDownloadDeleteSelected = RoadMapDownloadDeleteFips[i];
      }
   }
}


static void roadmap_download_delete_populate (void) {

   int i;
   int size;
   int count;
   char name[1024];
   RoadMapPosition center;


   roadmap_download_delete_free ();

   roadmap_download_compute_space (&count, &size);

   sprintf (name, "%d", count);
   roadmap_dialog_set_data (".delete", "Files", name);

   roadmap_download_format_size (name, size);
   roadmap_dialog_set_data (".delete", "Space", name);

   roadmap_main_flush();


   roadmap_screen_get_center (&center);
   RoadMapDownloadDeleteCount =
      roadmap_locator_by_position (&center, &RoadMapDownloadDeleteFips);

    RoadMapDownloadDeleteNames =
       calloc (RoadMapDownloadDeleteCount, sizeof(char *));
    roadmap_check_allocated(RoadMapDownloadDeleteNames);

    /* - List each candidate county: */

    for (i = 0; i < RoadMapDownloadDeleteCount; ++i) {

#ifndef _WIN32
       snprintf (name, sizeof(name), "%s/" FREEMAP_FILE_NAME_FORMAT,
                 roadmap_config_get (&RoadMapConfigDestination),
                 RoadMapDownloadDeleteFips[i]);
#else
       snprintf (name, sizeof(name), "%s\\" FREEMAP_FILE_NAME_FORMAT,
                 roadmap_config_get (&RoadMapConfigDestination),
                 RoadMapDownloadDeleteFips[i]);
#endif
       if (! roadmap_file_exists (NULL, name)) {

          /* The map for this county is not available: remove
           * the county from the list.
           */
          RoadMapDownloadDeleteFips[i] =
             RoadMapDownloadDeleteFips[--RoadMapDownloadDeleteCount];

          if (RoadMapDownloadDeleteCount <= 0) break; /* Empty list. */

          --i;
          continue;
       }

       snprintf (name, sizeof(name), "%s, %s",
                 roadmap_county_get_name (RoadMapDownloadDeleteFips[i]),
                 roadmap_county_get_state (RoadMapDownloadDeleteFips[i]));

       RoadMapDownloadDeleteNames[i] = strdup (name);
       roadmap_check_allocated(RoadMapDownloadDeleteNames[i]);
    }

    roadmap_dialog_show_list
        (".delete", ".maps",
         RoadMapDownloadDeleteCount,
         RoadMapDownloadDeleteNames, (void **)RoadMapDownloadDeleteNames,
         roadmap_download_delete_selected);
}


static void roadmap_download_delete_doit (const char *name, void *context) {

   char path[1024];

   if (RoadMapDownloadDeleteSelected > 0) {

      roadmap_download_block (RoadMapDownloadDeleteSelected);

#ifndef _WIN32
      snprintf (path, sizeof(path), "%s/" FREEMAP_FILE_NAME_FORMAT,
                roadmap_config_get (&RoadMapConfigDestination),
                RoadMapDownloadDeleteSelected);
#else
      snprintf (path, sizeof(path), "%s\\" FREEMAP_FILE_NAME_FORMAT,
                roadmap_config_get (&RoadMapConfigDestination),
                RoadMapDownloadDeleteSelected);
#endif
      roadmap_locator_close (RoadMapDownloadDeleteSelected);
      roadmap_file_remove (NULL, path);

      roadmap_screen_redraw ();

      roadmap_download_delete_populate ();
   }
}


void roadmap_download_delete (void) {

   if (roadmap_dialog_activate ("Delete Maps", NULL, 1)) {

      roadmap_dialog_new_label  (".delete", "Files");
      roadmap_dialog_new_label  (".delete", "Space");

      roadmap_dialog_new_list   (".delete", ".maps");

      roadmap_dialog_add_button ("Delete", roadmap_download_delete_doit);
      roadmap_dialog_add_button ("Done", roadmap_download_delete_done);

      roadmap_dialog_complete (0); /* No need for a keyboard. */
   }

   roadmap_download_delete_populate ();
}


/* -------------------------------------------------------------------------
 * Configure this download module.
 */

void roadmap_download_subscribe_protocol  (const char *prefix,
                                           RoadMapDownloadProtocol handler) {

   struct roadmap_download_protocol *protocol;

   protocol = malloc (sizeof(*protocol));
   roadmap_check_allocated(protocol);

   protocol->prefix = strdup(prefix);
   protocol->handler = handler;

   protocol->next = RoadMapDownloadProtocolMap;
   RoadMapDownloadProtocolMap = protocol;
}


void roadmap_download_subscribe_when_done (RoadMapDownloadEvent handler) {

   if (handler == NULL) {
      RoadMapDownloadWhenDone = roadmap_download_no_handler;
   } else {
      RoadMapDownloadWhenDone = handler;
   }
}


int  roadmap_download_enabled (void) {
   return (RoadMapDownloadWhenDone != roadmap_download_no_handler);
}


void roadmap_download_initialize (void) {

	char default_destination[256];
	
#ifdef __SYMBIAN32__
	strncpy_safe (default_destination, roadmap_db_map_path(), sizeof (default_destination));
#else
   roadmap_path_format (default_destination, sizeof (default_destination), roadmap_path_user(), "maps");
#endif

   roadmap_config_declare
      ("preferences",
      &RoadMapConfigSource,
      "" FREEMAP_FILE_NAME_FORMAT,
      NULL);

   roadmap_config_declare
      ("preferences",
      &RoadMapConfigDestination, default_destination, NULL);
}

