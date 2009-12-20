/* editor_gps_data.c - record GPS data
 *
 * LICENSE:
 *
 *   Copyright 2007 Ehud Shabtai
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
 * NOTE:
 * This file implements all the "dynamic" editor functionality.
 * The code here is experimental and needs to be reorganized.
 * 
 * SYNOPSYS:
 *
 *   See editor_gps_data.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_config.h"
#include "roadmap_path.h"
#include "roadmap_file.h"
#include "roadmap_gps.h"

#include "editor_gps_data.h"

#define DATA_ACTIVE_FILE "current.nmea"

static RoadMapConfigDescriptor ConfigSaveGpsData =
                        ROADMAP_CONFIG_ITEM("FreeMap", "Save GPS data");

static RoadMapFile GpsDataFile = ROADMAP_INVALID_FILE;
static int GpsRegistered;
static int GpsDataActive;

static char *get_data_dir (void) {
   char *dir = roadmap_path_join (roadmap_path_user (), "GPS");
   return dir;
}

static char *get_active_file_name (void) {
   char *dir = get_data_dir ();
   char *file_name = roadmap_path_join (dir, DATA_ACTIVE_FILE);

   roadmap_path_free (dir);

   return file_name;
}

static void gps_data_dir_check (void) {
   char *dir = get_data_dir ();

   roadmap_path_create (dir);
   roadmap_path_free (dir);
}

static void gps_data_update (const char *data) {
   char line[100];
   if  (!GpsDataActive) return;

   snprintf(line, sizeof(line), "%s\r\n", data);
   roadmap_file_write (GpsDataFile, line, strlen(line));
}


static void gps_data_status (void) {

   GpsDataActive = roadmap_config_match(&ConfigSaveGpsData, "yes");

   if (!GpsDataActive && !ROADMAP_FILE_IS_VALID(GpsDataFile)) return;
   if (GpsDataActive && ROADMAP_FILE_IS_VALID(GpsDataFile)) return;

   if (!GpsDataActive) {
      char *dir;

      roadmap_file_close (GpsDataFile);
      GpsDataFile = ROADMAP_INVALID_FILE;

      dir = get_data_dir ();
      roadmap_file_remove (dir, DATA_ACTIVE_FILE);

      roadmap_path_free (dir);
   } else {
      char *file_name = get_active_file_name ();
      gps_data_dir_check ();
      
      GpsDataFile = roadmap_file_open (file_name, "a");

      if (ROADMAP_FILE_IS_VALID(GpsDataFile)) {
         if (!GpsRegistered) {
            roadmap_gps_register_logger (gps_data_update);
            GpsRegistered = 1;
         }
      } else {
         roadmap_log (ROADMAP_ERROR,
            "Can't create GPS data logger at: %s", file_name);
         GpsDataActive = 0;
      }

      roadmap_path_free (file_name);
   }
}


void editor_gps_data_initialize (void) {

   roadmap_config_declare_enumeration
      ("preferences", &ConfigSaveGpsData, gps_data_status, "no", "yes", NULL);

   gps_data_status ();
}


void editor_gps_data_shutdown (void) {

   if (GpsDataActive) {
      GpsDataActive = 0;
      roadmap_file_close (GpsDataFile);
      GpsDataFile = ROADMAP_INVALID_FILE;
   }
}


void editor_gps_data_export (const char *name) {

   char *dir;
   char *old_name;
   char *new_name;
   char export_file_name[255];

   if (!ROADMAP_FILE_IS_VALID(GpsDataFile)) return;

   GpsDataActive = 0;
   roadmap_file_close (GpsDataFile);
   GpsDataFile = ROADMAP_INVALID_FILE;

   old_name = get_active_file_name ();
   dir = get_data_dir ();
   snprintf (export_file_name, sizeof(export_file_name), "%s.nmea", name);
   new_name = roadmap_path_join (dir, export_file_name);

   if (roadmap_file_rename (old_name, new_name) == -1) {
      roadmap_log (ROADMAP_ERROR, "Can't rename GPS data file from:%s to %s",
                   old_name, new_name);
   }

   roadmap_path_free (dir);
   roadmap_path_free (old_name);
   roadmap_path_free (new_name);

   /* This should turn the logging back on */
   gps_data_status ();
}

