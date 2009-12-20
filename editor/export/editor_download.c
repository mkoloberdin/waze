/* editor_download.c - Download map updates.
 *
 * LICENSE:
 *
 *   (c) Copyright 2006 Ehud Shabtai
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
 */

#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_httpcopy.h"
#include "roadmap_download.h"
#include "roadmap_screen.h"
#include "roadmap_locator.h"
#include "roadmap_messagebox.h"
#include "roadmap_display.h"

#include "../editor_main.h"
#include "navigate/navigate_main.h"
#include "../db/editor_db.h"
#include "editor_download.h"

static void download_map_done (void) {

   editor_main_set (1);
   navigate_main_reload_data ();
   roadmap_download_subscribe_when_done (NULL);
   roadmap_screen_unfreeze ();
   roadmap_screen_redraw ();
}

static int editor_download_map (RoadMapDownloadCallbacks *callbacks) {

   static int *fips = NULL;
   static int ProtocolInitialized = 0;
   RoadMapPosition center;
   int count;
   int i;

   if (! ProtocolInitialized) {

      /* PLUGINS NOT SUPPORTED YET.
       * roadmap_plugin_load_all
       *      ("download", roadmap_download_subscribe_protocol);
       */

      roadmap_httpcopy_init (roadmap_download_subscribe_protocol);

      ProtocolInitialized = 1;
   }

   roadmap_screen_get_center (&center);
   count = roadmap_locator_by_position (&center, &fips);

#ifdef __SYMBIAN32__
   fips[0] = 77001;
   count = 1;
#else
   if (count == 0) {

      if (callbacks) {
         fips[0] = 77001;
         count = 1;
      } else {
         roadmap_display_text("Info", "No map available");
         return -1;
      }
   }
#endif
   
#if 0
   for (i = count-1; i >= 0; --i) {

      if (!editor_export_empty (fips[i])) {

         if (!callbacks) {
            roadmap_messagebox("Info", "You must first export your data.");
         }

         return -1;
      }
   }
#endif

   roadmap_screen_freeze ();
   editor_main_set (0);

   roadmap_download_subscribe_when_done (download_map_done);
   roadmap_download_unblock_all ();

   for (i = count-1; i >= 0; --i) {

      int res;

      editor_db_close (fips[i]);
      editor_db_delete (fips[i]);
      res = roadmap_download_get_county (fips[i], 0, callbacks);

      if (res != 0) return -1;
   }

   return 0;
}


int editor_download_update_map (RoadMapDownloadCallbacks *callbacks) {

   return editor_download_map (callbacks);
}

