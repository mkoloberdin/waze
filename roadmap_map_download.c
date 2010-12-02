/* roadmap_map_download.c - download compressed map files.
 *
 * LICENSE:
 *
 *   Copyright 2009 Israel Disatnik
 *
 *   This file is part of Waze.
 *
 *   Waze is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   Waze is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Waze; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "roadmap.h"
#include "roadmap_map_download.h"
#include "roadmap_main.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
#include "roadmap_config.h"
#include "roadmap_httpcopy_async.h"
#include "roadmap_screen.h"
#include "roadmap_messagebox.h"
#include "roadmap_warning.h"
#include "roadmap_lang.h"
#include "roadmap_locator.h"
#include "roadmap_tile_manager.h"
#include "ssd/ssd_confirm_dialog.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_progress_msg_dialog.h"

#ifdef IPHONE
#include "roadmap_main.h"
#endif //IPHONE

static RoadMapConfigDescriptor DlMapSourceConf =
                    ROADMAP_CONFIG_ITEM("Download", "Source");

static RoadMapConfigDescriptor DlMapNameConf =
                    ROADMAP_CONFIG_ITEM("Download", "Map name");

static RoadMapConfigDescriptor DlMapEnabledConf =
                    ROADMAP_CONFIG_ITEM("Download", "Enabled");

static int DlMapProgress = -1;
static int DlMapTotalSize = -1;
static char DlMapFileName[256];
static char DlMapFileFullName[256];
static char DlMapTempFullName[256];
static void *DlContext = NULL;
static int DlFips = 0;
static RoadMapFile DlFile = ROADMAP_INVALID_FILE;



BOOL dlmap_warning_fn ( char* dest_string ) {

	if (DlMapProgress < 0) {
		return FALSE;
	}
	
	snprintf (dest_string, ROADMAP_WARNING_MAX_LEN, "%s %s: %d%%%%", roadmap_lang_get ("Downloading"),
	               roadmap_lang_get(DlMapFileName), 
					DlMapProgress * 100 / DlMapTotalSize);
	return TRUE;	
}


static void dlmap_initialize (void) {
	
	static int first_time = 1;
	
	if (first_time) {
		roadmap_config_declare
	      ("preferences", &DlMapSourceConf,  "", NULL);

      roadmap_config_declare
         ("preferences", &DlMapEnabledConf,  "0", NULL);

      roadmap_config_declare
         ("preferences", &DlMapNameConf,  "", NULL);

      first_time = 0;
	}
      
   roadmap_warning_register (dlmap_warning_fn, "dlmap");
}


static void dlmap_finalize (void) {

	DlMapProgress = -1;
	roadmap_warning_unregister (dlmap_warning_fn);
}

void dl_map_cb(int exit_code ){

   
   roadmap_main_exit();
}

static void dlmap_update_tiles (void) {

	roadmap_main_remove_periodic (dlmap_update_tiles);	
	roadmap_locator_close (DlFips);
	roadmap_file_rename (DlMapTempFullName, DlMapFileFullName);
	roadmap_locator_refresh (DlFips);
	roadmap_tile_reset_session ();

	ssd_progress_msg_dialog_hide ();
	
	roadmap_messagebox_cb ("Map Download Complete", "Please restart Waze", dl_map_cb);
}


static void dlmap_close (int success) {

	if (ROADMAP_FILE_IS_VALID (DlFile)) {
		roadmap_file_close (DlFile);
		DlFile = ROADMAP_INVALID_FILE;
		if (success) {
			ssd_progress_msg_dialog_show (roadmap_lang_get ("Updating Map Tiles..."));
			if (!roadmap_screen_refresh ()) {
				roadmap_screen_redraw();
			}
			roadmap_main_set_periodic (100, dlmap_update_tiles);
		} else {
			roadmap_file_remove (NULL, DlMapTempFullName);
		}
	}	
	dlmap_finalize ();
	DlContext = NULL;
}

static int  DlMapCallbackSize	(void *context, size_t size) {

	DlMapTotalSize = size;
	
	//TODO: check free file space
	
	DlFile = roadmap_file_open (DlMapTempFullName, "w");
	if (!ROADMAP_FILE_IS_VALID (DlFile)) {
		roadmap_log (ROADMAP_ERROR, "Cannot open file %s for writing", DlMapFileFullName);
		roadmap_messagebox ("Error", "Map Download Failed");
		return 0;
	}

	DlMapProgress = 0;
	return 1;	 	
}

void DlMapCallbackProgress (void *context, char *data, size_t size) {

	if (ROADMAP_FILE_IS_VALID (DlFile)) {
		size_t written = roadmap_file_write (DlFile, data, size);
		if (written != size) {
			roadmap_log (ROADMAP_ERROR, "Writing failed to file %s", DlMapFileFullName);
			roadmap_messagebox ("Error", "Map Download Failed -- Not Enough Space");
			dlmap_close (0);
		}
		DlMapProgress += written;
		if (DlMapProgress * 10 / DlMapTotalSize > (DlMapProgress - (int)written) * 10 / DlMapTotalSize) {
			roadmap_screen_refresh ();
		}
	}
}

void DlMapCallbackError (void *context, int connection_failure, const char *format, ...) {

   va_list ap;
   char err_string[1024];

   va_start (ap, format);
   vsnprintf (err_string, 1024, format, ap);
   va_end (ap);
	roadmap_log (ROADMAP_ERROR, err_string);

	dlmap_close (0);
	roadmap_messagebox ("Error", "Map Download Failed");
}


void DlMapCallbackDone (void *context, char *last_modified) {

	dlmap_close (1);	
}

static void DlMapConfirmCallback(int exit_code, void *context){
    if (exit_code != dec_yes)
         return;
    ssd_dialog_hide_all( dec_close);

    if (!roadmap_screen_refresh ())
       roadmap_screen_redraw();
    
    roadmap_map_download_region( roadmap_config_get (&DlMapNameConf),
                                 roadmap_locator_static_county() );
}

BOOL roamdmap_map_download_enabled(void){
   if (0 == strcmp (roadmap_config_get (&DlMapEnabledConf), "yes"))
      return TRUE;
   return FALSE; 

}

int roadmap_map_download(SsdWidget widget, const char *new_value){
#ifdef IPHONE
   roadmap_main_show_root(0);
#endif //IPHONE
   ssd_confirm_dialog ("Warning", "Download requires large amount of data, continue?", TRUE, DlMapConfirmCallback , NULL);
   return 1;
}

const char* roadmap_map_download_build_file_name( int fips )
{
   snprintf( DlMapFileName, sizeof (DlMapFileName), "map%05d.wzm", fips );
#ifndef IPHONE
	roadmap_path_format (DlMapFileFullName, sizeof (DlMapFileFullName), 
								roadmap_db_map_path (), DlMapFileName);
#else
     roadmap_path_format (DlMapFileFullName, sizeof (DlMapFileFullName), 
								roadmap_path_preferred("maps"), DlMapFileName);
#endif //IPHONE
   return DlMapFileFullName;
}

void roadmap_map_download_region (const char *region_code, int fips) {

	char url[512];
	static RoadMapHttpAsyncCallbacks cbs = { DlMapCallbackSize, DlMapCallbackProgress, DlMapCallbackError, DlMapCallbackDone };
	
	if (DlContext != NULL) {
		roadmap_log (ROADMAP_ERROR, "Trying to open a second map download");
		return;	
	}

	DlFips = fips;
	roadmap_map_download_build_file_name( fips );
	snprintf (DlMapTempFullName, sizeof (DlMapTempFullName), "%s_", DlMapFileFullName);
	
	snprintf (url, sizeof(url), "%s/%s/%s", 
				 roadmap_config_get (&DlMapSourceConf),
				 region_code, DlMapFileName);
				 
	dlmap_initialize ();
	DlContext = roadmap_http_async_copy (&cbs, NULL, url, 0);
}
