/* roadmap_version.c - RoadMap version file handler
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R
 *   Copyright 2009, Waze Ltd
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
 */

#include <string.h>
#include <stdio.h>

#include "roadmap_path.h"
#include "roadmap_file.h"
#include "roadmap_version.h"


const char* roadmap_version_read() {
   FILE *ver_file = NULL;
   char *filename = "version";
   static char ver[100];
   static int done = 0;
   
   if (!done) {
      ver[0] = 0;
      if (roadmap_file_exists(roadmap_path_user(), filename)) {
         ver_file = roadmap_file_fopen(roadmap_path_user(), filename, "r");
         if (ver_file) {
            fscanf(ver_file, "%s",ver);
            fclose(ver_file);
         }
      }
      done = 1;
   }
   
   return ver;
}

void roadmap_version_write(const char* ver) {
   FILE *ver_file = NULL;
   char *filename = "version";
   
   ver_file = roadmap_file_fopen(roadmap_path_user(), filename, "w");
   fprintf(ver_file, ver);
   fclose(ver_file);
}