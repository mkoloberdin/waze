/* roadmap_library.c - a low level module to manage plugins for RoadMap.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 *   See roadmap_library.h
 */

#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_library.h"

struct roadmap_library_context {
   const char *name;
   void *handle;
};


static char *roadmap_library_name (const char *pathname) {

   char *filename = malloc (strlen(pathname) + 4);

   roadmap_check_allocated(filename);

   strcpy (filename, pathname);
   strcat (filename, ".so");

   return filename;
}


RoadMapLibrary roadmap_library_load (const char *pathname) {

   RoadMapLibrary library;
   char *filename = roadmap_library_name (pathname);

   void *handle = dlopen (filename, RTLD_NOW);

   if (handle == NULL) {
      roadmap_log (ROADMAP_ERROR,
                   "cannot load library %s: %s", filename, dlerror());
      return NULL;
   }

   library = (RoadMapLibrary) malloc (sizeof(struct roadmap_library_context));
   roadmap_check_allocated(library);

   library->name = filename;
   library->handle = handle;

   return library;
}


int roadmap_library_exists (const char *pathname) {

   int   result;
   char *filename = roadmap_library_name (pathname);

   result = roadmap_file_exists (NULL, filename);
   free (filename);

   return result;
}


void *roadmap_library_symbol (RoadMapLibrary library, const char *symbol) {

   void *address = dlsym (library->handle, symbol);

   if (address == NULL) {

       roadmap_log (ROADMAP_ERROR,
                    "cannot find %s in library %s: %s",
                    symbol, library->name, dlerror());
       dlclose (library->handle);

       return NULL;
   }

   return address;
}

