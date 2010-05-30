/* roadmap_path.h - a module to handle file path in an OS independent way.
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
 * DESCRIPTION:
 *
 *   This module provides a simple to use API for handling directory search
 *   lists, similar to the one defined for the UNIX PATH environment variable.
 *   In addition this API is OS independent.
 *
 *   The roadmap_path_user() function returns a constant that represents
 *   the path of the RoadMap context for the current user (on UNIX, this
 *   is typically $HOME/,roadmap). The roadmap_path_trips() function
 *   returns a constant that represents the path of the directory where
 *   the current user stores his/her trip files (this is usually a
 *   subdirectory of the user context, named "trips"). These functions
 *   return the full path name of a single directory (i.e. not a list).
 *
 *   The roadmap_path_set() function sets a current RoadMap directory search
 *   list. If the search list was never set, the default path is used (see
 *   above). When analyzing the path string, roadmap_path_set() expands
 *   the following characters, but only if the character is the first
 *   character of the path:
 *
 *      ~   user's home directory path (same as the UNIX shell convention)
 *      &   user's roadmap directory (usually $HOME/.roadmap on UNIX)
 *
 *
 *   The functions roadmap_path_first()/roadmap_path_next() and
 *   roadmap_path_last()/roadmap_path_previous() can be used to
 *   parse a directory search list. Each set is used to parse the
 *   list in a specific order. For example:
 *
 *      for (path = roadmap_path_first("maps");
 *           path != NULL;
 *           path = roadmap_path_next("maps", path)) {
 *         // do something with path.
 *      }
 *
 *   The function roadmap_path_preferred() returns the recommended path
 *   for the given path list. This value will be used as a default when
 *   the user did not specify a path explicitely. This preferred path
 *   cannot be changed.
 *
 *   The function roadmap_path_join() returns a new string that concatenates
 *   path and name in a way that is compatible with the OS conventions. The
 *   function roadmap_path_parent() works in a similar fashion, except that
 *   it returns the path of the parent directory of the concatenation result.
 *
 *   For example, on UNIX, if path is "/usr/include" and name is "sys/errno.h"
 *   then roadmap_path_join() will return "/usr/include/sys/errno.h" while
 *   roadmap_path_parent() will return "/usr/include/sys".
 *
 *   The function roadmap_path_create() creates the directory which name
 *   is specified by its argument. No error is reported if the directory
 *   already exists.
 *
 *   The path returned by roadmap_path_join(), roadmap_path_parent() and
 *   roadmap_path_expand() must be deallocated using roadmap_path_free().
 *
 *   The function roadmap_path_list() returns a list of file names, one
 *   for each file found in the directory provided. Only file names that
 *   match the provided extension are returned. The file names do not
 *   include the path of the directory. The list is NULL terminated. If no
 *   file was found, or if the path is invalid, an empty list is returned
 *   (i.e. a list made of a NULL: the pointer returned by roadmap_path_list()
 *   is never NULL itself).
 *
 *   The result returned by roadmap_path_list() must be deallocated using
 *   roadmap_path_list_free(). For example:
 *
 *      char **cursor;
 *      char **list = roadmap_path_list ("/usr/include", ".h");
 *
 *      for (cursor = list; *cursor != NULL; ++cursor) {
 *          // process this file..
 *      }
 *      roadmap_path_list_free (list);
 *
 * BUGS:
 *
 *   The result returned by roadmap_path_user(), roadmap_path_trips() and
 *   roadmap_path_default() must NOT be free()'d!
 */

#ifndef INCLUDE__ROADMAP_PATH__H
#define INCLUDE__ROADMAP_PATH__H

void roadmap_path_set  (const char *name, const char *path);

const char *roadmap_path_first (const char *name);
const char *roadmap_path_next  (const char *name, const char *current);

const char *roadmap_path_last     (const char *name);
const char *roadmap_path_previous (const char *name, const char *current);

const char *roadmap_path_preferred (const char *name);

const char *roadmap_path_user    (void);
const char *roadmap_path_trips   (void);

char *roadmap_path_join (const char *path, const char *name);
char *roadmap_path_parent (const char *path, const char *name);
void roadmap_path_format (char *buffer, int buffer_size, const char *path, const char *name);

void roadmap_path_create (const char *path);

char **roadmap_path_list (const char *path, const char *extension);
void   roadmap_path_list_free (char **list);

void roadmap_path_free (const char *path);

const char *roadmap_path_search_icon (const char *name);

char *roadmap_path_skip_directories (const char *name);
char *roadmap_path_remove_extension (const char *name);

int roadmap_path_is_full_path (const char *name);

const char *roadmap_path_temporary (void);

int roadmap_path_is_directory (const char *name);

const char *roadmap_path_gps( void );

const char *roadmap_path_images( void );

const char *roadmap_path_downloads( void );

const char *roadmap_path_config( void );

#ifdef IPHONE
const char *roadmap_path_bundle (void);
#else
const char *roadmap_path_sdcard (void);
#endif //IPHONE

const char *roadmap_path_debug( void );
#endif // INCLUDE__ROADMAP_PATH__H

