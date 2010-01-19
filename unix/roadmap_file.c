/* roadmap_file.c - a module to open/read/close a roadmap database file.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 *   See roadmap_file.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_file.h"


struct RoadMapFileContextStructure {

   int   fd;
   void *base;
   int   size;
};


FILE *roadmap_file_fopen (const char *path,
                          const char *name,
                          const char *mode) {

   int   silent;
   FILE *file;
   const char *full_name = roadmap_path_join (path, name);

   if (mode[0] == 's') {
      /* This special mode is a "lenient" read: do not complain
       * if the file does not exist.
       */
      silent = 1;
      ++mode;
   } else {
      silent = 0;
   }

   file = fopen (full_name, mode);

   if ((file == NULL) && (! silent)) {
      roadmap_log (ROADMAP_ERROR, "cannot open file %s", full_name);
   }

   roadmap_path_free (full_name);
   return file;
}


void roadmap_file_remove (const char *path, const char *name) {

   const char *full_name = roadmap_path_join (path, name);

   remove(full_name);
   roadmap_path_free (full_name);
}


int roadmap_file_exists (const char *path, const char *name) {

   int   status;
   const char *full_name = roadmap_path_join (path, name);
   struct stat stat_buffer;

   status = stat (full_name, &stat_buffer);

   roadmap_path_free (full_name);

   return (status == 0);
}


int roadmap_file_length (const char *path, const char *name) {

   int   status;
   const char *full_name = roadmap_path_join (path, name);
   struct stat stat_buffer;

   status = stat (full_name, &stat_buffer);
   roadmap_path_free (full_name);

   if (status == 0) {
      return stat_buffer.st_size;
   }
   return -1;
}


void roadmap_file_save (const char *path, const char *name,
                        void *data, int length) {

   int   fd;
   const char *full_name = roadmap_path_join (path, name);

   fd = open (full_name, O_CREAT+O_WRONLY+O_TRUNC, 0666);
   roadmap_path_free (full_name);

   if (fd >= 0) {

      write (fd, data, length);
      close(fd);
   }
}


int roadmap_file_truncate (const char *path, const char *name,
                           int length) {

   int   res;
   const char *full_name = roadmap_path_join (path, name);

   res = truncate (full_name, length);
   roadmap_path_free (full_name);

   return res;
}

int roadmap_file_rename (const char *old_name, const char *new_name) {

   return rename (old_name, new_name);
}

void roadmap_file_append (const char *path, const char *name,
                          void *data, int length) {

   int   fd;
   const char *full_name = roadmap_path_join (path, name);

   fd = open (full_name, O_CREAT+O_WRONLY+O_APPEND, 0666);
   roadmap_path_free (full_name);

   if (fd >= 0) {

      write (fd, data, length);
      close(fd);
   }
}


const char *roadmap_file_unique (const char *base) {

    static int   UniqueNameCounter = 0;
    static char *UniqueNameBuffer = NULL;
    static int   UniqueNameBufferLength = 0;

    int length;
    
    length = strlen(base + 16);
    
    if (length > UniqueNameBufferLength) {

        if (UniqueNameBuffer != NULL) {
            free(UniqueNameBuffer);
        }
        UniqueNameBuffer = malloc (length);
        
        roadmap_check_allocated(UniqueNameBuffer);
        
        UniqueNameBufferLength = length;
    }
    
    sprintf (UniqueNameBuffer,
             "%s%d_%d", base, getpid(), UniqueNameCounter);

    UniqueNameCounter += 1;

    return UniqueNameBuffer;
}


const char *roadmap_file_map (const char *set,
                              const char *name,
                              const char *sequence,
                              const char *mode,
                              RoadMapFileContext *file) {

   RoadMapFileContext context;

   struct stat state_result;
   int open_mode;
   int map_mode;


   context = malloc (sizeof(*context));
   roadmap_check_allocated(context);

   context->fd = -1;
   context->base = NULL;
   context->size = 0;

   if (strcmp(mode, "r") == 0) {
      open_mode = O_RDONLY;
      map_mode = PROT_READ;
   } else if (strchr (mode, 'w') != NULL) {
      open_mode = O_RDWR;
      map_mode = PROT_READ|PROT_WRITE;
   } else {
      roadmap_log (ROADMAP_ERROR,
                   "%s: invalid file access mode %s", name, mode);
      free (context);
      return NULL;
   }

   if (name[0] == '/') {

      context->fd = open (name, open_mode, 0666);
      sequence = ""; /* Whatever, but NULL. */

   } else {

      char *full_name;

      int full_name_size;
      int name_size = strlen(name);
      int size;


      if (sequence == NULL) {
         sequence = roadmap_path_first(set);
      } else {
         sequence = roadmap_path_next(set, sequence);
      }
      if (sequence == NULL) {
         free (context);
         return NULL;
      }

      full_name_size = 512;
      full_name = malloc (full_name_size);
      roadmap_check_allocated(full_name);

      do {
         size = strlen(sequence) + name_size + 2;

         if (size >= full_name_size) {
            full_name = realloc (full_name, size);
            roadmap_check_allocated(full_name);
            full_name_size = size;
         }

         strcpy (full_name, sequence);
         strcat (full_name, "/");
         strcat (full_name, name);

         context->fd = open (full_name, open_mode, 0666);

         if (context->fd >= 0) break;

         sequence = roadmap_path_next(set, sequence);

      } while (sequence != NULL);

      free (full_name);
   }

   if (context->fd < 0) {
      if (sequence == 0) {
         roadmap_log (ROADMAP_INFO, "cannot open file %s", name);
      }
      roadmap_file_unmap (&context);
      return NULL;
   }

   if (fstat (context->fd, &state_result) != 0) {
      if (sequence == 0) {
         roadmap_log (ROADMAP_ERROR, "cannot stat file %s", name);
      }
      roadmap_file_unmap (&context);
      return NULL;
   }
   context->size = state_result.st_size;

   context->base =
      mmap (NULL, state_result.st_size, map_mode, MAP_SHARED, context->fd, 0);

   if (context->base == NULL) {
      roadmap_log (ROADMAP_ERROR, "cannot map file %s", name);
      roadmap_file_unmap (&context);
      return NULL;
   }

   *file = context;

   return sequence; /* Indicate the next directory in the path. */
}


void *roadmap_file_base (RoadMapFileContext file){

   if (file == NULL) {
      return NULL;
   }
   return file->base;
}


int roadmap_file_size (RoadMapFileContext file){

   if (file == NULL) {
      return 0;
   }
   return file->size;
}


int roadmap_file_sync (RoadMapFileContext file) {

   if (file->base != NULL) {
      return msync (file->base, file->size, MS_SYNC);
   }

   return -1;
}


void roadmap_file_unmap (RoadMapFileContext *file) {

   RoadMapFileContext context = *file;

   if (context->base != NULL) {
      munmap (context->base, context->size);
   }

   if (context->fd >= 0) {
      close (context->fd);
   }
   free(context);
   *file = NULL;
}


RoadMapFile roadmap_file_open  (const char *name, const char *mode) {

   int unix_mode = 0;

   if (strcmp(mode, "r") == 0) {
      unix_mode = O_RDONLY;
   } else if (strcmp (mode, "rw") == 0) {
      unix_mode = O_RDWR|O_CREAT;
   } else if (strchr (mode, 'w') != NULL) {
      unix_mode = O_RDWR|O_CREAT|O_TRUNC;
   } else if (strchr (mode, 'a') != NULL) {
      unix_mode = O_RDWR|O_CREAT|O_APPEND;
   } else {
      roadmap_log (ROADMAP_ERROR,
                   "%s: invalid file access mode %s", name, mode);
      return -1;
   }

   return (RoadMapFile) open (name, unix_mode, 0644);
}


int roadmap_file_read  (RoadMapFile file, void *data, int size) {
   return read ((int)file, data, size);
}

int roadmap_file_write (RoadMapFile file, const void *data, int length) {
   return write ((int)file, data, length);
}

int roadmap_file_seek (RoadMapFile file, int offset, RoadMapSeekWhence whence) {

	int unix_whence;
	
	switch (whence) {
		case ROADMAP_SEEK_START:
			unix_whence = SEEK_SET;
			break;
		case ROADMAP_SEEK_CURR:
			unix_whence = SEEK_CUR;
			break;
		case ROADMAP_SEEK_END:
			unix_whence = SEEK_END;
			break;
		default:
	      roadmap_log (ROADMAP_ERROR,
	                   "invalid file seek whence %d", (int)whence);
	      return -1;
	}
	
	return lseek ((int)file, offset, unix_whence);
} 

void  roadmap_file_close (RoadMapFile file) {
   close ((int)file);
}

int roadmap_file_free_space (const char *path) {
   return -1;
}

