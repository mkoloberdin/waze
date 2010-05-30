/* roadmap_path.cpp - a module to handle file path in an OS independent way.
 *
 * LICENSE:
 *
 *   Copyright 2008 Giant Steps Ltd
 *   Copyright 2008 Ehud Shabtai
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
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
 *   See roadmap_path.h.
 */

#include <stdlib.h>
#include <string.h>
#include <f32file.h>
#include <eikenv.h>
#include <aknutils.h>
#include <eikfutil.h>
#include <utf.h>

#include "GSConvert.h"

extern "C"{
#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_path.h"
}

typedef struct RoadMapPathRecord *RoadMapPathList;

struct RoadMapPathRecord {

   RoadMapPathList next;

   char  *name;
   int    count;
   char **items;
   char  *preferred;
};

static RoadMapPathList RoadMapPaths = NULL;

/* The hardcoded path for configuration files (the "config" path).
*/
static const char *RoadMapPathConfig[] = {
   "&",
   "C:\\private\\2001EB29",
   "E:\\private\\2001EB29",
   "Z:\\private\\2001EB29",
   "C:\\Data\\Others",
   NULL
};

static const char *RoadMapPathGps= "E:\\waze\\gps";

static const char *RoadMapPathConfigPreferred = "E:\\private\\2001EB29";

/* Skins directories */
static const char *RoadMapPathSkin[] = {
   "&\\skins\\default\\day",
   "&\\skins\\default",
   "&", //fallback
   NULL
};

static const char *RoadMapPathSkinPreferred = "&\\skins";

/* The default path for the map files (the "maps" path): */
static const char *RoadMapPathMaps[] = {
   "E:\\Data\\Others\\maps",
   "C:\\Data\\Others\\maps",
   "&\\maps",
   "C:\\private\\2001EB29\\maps",
   "E:\\private\\2001EB29\\maps",
   "Z:\\private\\2001EB29\\maps",
   "&", //fallback
   NULL
};

static const char *RoadMapPathMapsPreferred = "E:\\private\\2001EB29\\maps";

/* We don't have a user directory in wince so we'll leave this one empty */
static const char *RoadMapPathUser[] = {
   NULL
};


static char *roadmap_path_expand (const char *item, size_t length);

static void roadmap_path_list_create(const char *name,
                            const char *items[],
                            const char *preferred)
{
   int i;
   int count;
   RoadMapPathList new_path;
   for (count = 0; items[count] != NULL; ++count) ;

   new_path = (RoadMapPathRecord*)malloc (sizeof(struct RoadMapPathRecord));
   roadmap_check_allocated(new_path);

   new_path->next  = RoadMapPaths;
   new_path->name  = strdup(name);
   new_path->count = count;

   new_path->items = (char**)calloc (count, sizeof(char *));
   roadmap_check_allocated(new_path->items);

   for (i = 0; i < count; ++i) {
      new_path->items[i] = roadmap_path_expand (items[i], strlen(items[i]));
   }
   new_path->preferred  = roadmap_path_expand (preferred, strlen(preferred));

   RoadMapPaths = new_path;
}

static RoadMapPathList roadmap_path_find (const char *name)
{
   RoadMapPathList cursor;

   if (RoadMapPaths == NULL) {

      /* Add the hardcoded configuration. */
      roadmap_path_list_create ("user", RoadMapPathUser, roadmap_path_user());
      roadmap_path_list_create ("config", RoadMapPathConfig,
                                 RoadMapPathConfigPreferred);
      roadmap_path_list_create ("skin", RoadMapPathSkin,
                                 RoadMapPathSkinPreferred);
      roadmap_path_list_create ("maps", RoadMapPathMaps,
                                RoadMapPathMapsPreferred);
   }

   for (cursor = RoadMapPaths; cursor != NULL; cursor = cursor->next) {
      if (strcasecmp(cursor->name, name) == 0) break;
   }
   return cursor;
}


/* Directory path strings operations. -------------------------------------- */

static char *roadmap_path_cat (const char *s1, const char *s2)
{
   char *result = (char*)malloc (strlen(s1) + strlen(s2) + 4);

   roadmap_check_allocated (result);

   strcpy (result, s1);
   strcat (result, "\\");
   strcat (result, s2);

   return result;
}


char *roadmap_path_join (const char *path, const char *name)
{
   if (path == NULL) {
      return strdup (name);
   }
   if ( name == NULL ) {
	  return strdup( path );
   }
   return roadmap_path_cat (path, name);
}


char *roadmap_path_parent (const char *path, const char *name)
{
   char *separator;
   char *full_name = roadmap_path_join (path, name);

   separator = strrchr (full_name, '\\');
   if (separator == NULL) {
      return ".";
   }

   *separator = 0;

   return full_name;
}


void roadmap_path_format (char *buffer, int buffer_size, const char *path, const char *name) {

	int len1 = path ? strlen (path) + 1 : 0;
	int len2 = name ? strlen (name) : 0;

	if (len1 >= buffer_size) {
		len1 = buffer_size - 1;
	}
	if (len1 + len2 >= buffer_size) {
		len2 = buffer_size - 1 - len1;
	}

	// first copy file name, for the case where buffer and name are the same pointer
	if (len2) {
		memmove (buffer + len1, name, len2);
	}
	if (len1) {
		memmove (buffer, path, len1 - 1);
		buffer[len1 - 1] = '\\';
	}
	buffer[len1 + len2] = '\0';
}


char *roadmap_path_skip_directories (const char *name)
{
   char *result = strrchr (name, '\\');

   if (result == NULL) return (char *)name;

   return result + 1;
}


char *roadmap_path_remove_extension (const char *name)
{
   char *result;
   char *p;


   result = strdup(name);
   roadmap_check_allocated(result);

   p = roadmap_path_skip_directories (result);
   p = strrchr (p, '.');
   if (p != NULL) *p = 0;

   return result;
}


const char *roadmap_path_user (void)
{
   static char *RoadMapUser = NULL;

   if (RoadMapUser == NULL)
   {
     TFileName privatePath;
     CEikonEnv::Static()->FsSession().PrivatePath(privatePath);
     // CompleteWithAppPath works even without a filename
     CompleteWithAppPath(privatePath);
     privatePath.Delete(privatePath.Length()-1,1);

     GSConvert::TDes16ToCharPtr(privatePath, &RoadMapUser);


   }
   return RoadMapUser;
}

const char *roadmap_path_gps (void)
{
   static const char *RoadMapGps = NULL;

   if (RoadMapGps == NULL)
   {
	  RoadMapGps = RoadMapPathGps;
      roadmap_path_create( RoadMapGps );
   }
   return RoadMapGps;
}

const char *roadmap_path_images( void )
{
   static char *RoadMapPathImages = NULL;

   if ( RoadMapPathImages == NULL )
   {
	  RoadMapPathImages = roadmap_path_cat( roadmap_path_user(), "images" );
	  roadmap_path_create( RoadMapPathImages );
   }
   return RoadMapPathImages;
}

const char *roadmap_path_downloads( void )
{
   return roadmap_path_user();
}

const char *roadmap_path_debug( void )
{
   static char *RoadMapPathDebug = NULL;

   if ( RoadMapPathDebug == NULL )
   {
      RoadMapPathDebug = roadmap_path_join( roadmap_path_user(), "debug" );
      roadmap_path_create( RoadMapPathDebug );
   }
   return RoadMapPathDebug;
}

const char *roadmap_path_trips (void)
{
   static char  RoadMapDefaultTrips[] = "trips";
   static char *RoadMapTrips = NULL;

   if (RoadMapTrips == NULL) {

      RoadMapTrips =
         roadmap_path_cat (roadmap_path_user(), RoadMapDefaultTrips);

      roadmap_path_create(RoadMapTrips);
   }
   return RoadMapTrips;
}


static char *roadmap_path_expand (const char *item, size_t length) {

   const char *expansion;
   size_t expansion_length;
   char *expanded;

   switch (item[0]) {
      case '&': expansion = roadmap_path_user(); item++; length--; break;
      default:  expansion = "";
   }
   expansion_length = strlen(expansion);

   expanded = (char*)malloc (length + expansion_length + 1);
   roadmap_check_allocated(expanded);

   strcpy (expanded, expansion);
   strncat (expanded, item, length);

   expanded[length+expansion_length] = 0;

   return expanded;
}

/* Path lists operations. -------------------------------------------------- */

void roadmap_path_set (const char *name, const char *path)
{
   int i;
   int count;
   const char *item;
   const char *next_item;

   RoadMapPathList path_list = roadmap_path_find (name);


   if (path_list == NULL) {
      roadmap_log(ROADMAP_FATAL, "unknown path set '%s'", name);
   }

   while (*path == ',') path += 1;
   if (*path == 0) return; /* Ignore empty path: current is better. */


   if (path_list->items != NULL) {

      /* This replaces a path that was already set. */

      for (i = path_list->count-1; i >= 0; --i) {
         free (path_list->items[i]);
      }
      free (path_list->items);
   }


   /* Count the number of items in this path string. */

   count = 0;
   for (item = path-1; item != NULL; item = strchr (item+1, ',')) {
      count += 1;
   }

   path_list->items = (char**)calloc (count, sizeof(char *));
   roadmap_check_allocated(path_list->items);


   /* Extract and expand each item of the path.
   * Ignore directories that do not exist yet.
   */
   for (i = 0, item = path-1; item != NULL; item = next_item) {

      item += 1;
      next_item = strchr (item, ',');

      if (next_item == NULL) {
         path_list->items[i] = roadmap_path_expand (item, strlen(item));
      } else {
         path_list->items[i] =
            roadmap_path_expand (item, (size_t)(next_item - item));
      }

      if (roadmap_path_is_directory(path_list->items[i])) {
         ++i;
      } else {
         free (path_list->items[i]);
         path_list->items[i] = NULL;
      }
   }
   path_list->count = i;
}


const char *roadmap_path_first (const char *name)
{
   RoadMapPathList path_list = roadmap_path_find (name);

   if (path_list == NULL) {
      roadmap_log (ROADMAP_FATAL, "invalid path set '%s'", name);
   }

   if (path_list->count > 0) {
      return path_list->items[0];
   }

   return NULL;
}


const char *roadmap_path_next  (const char *name, const char *current)
{
   int i;
   RoadMapPathList path_list = roadmap_path_find (name);


   for (i = 0; i < path_list->count-1; ++i) {

      if (path_list->items[i] == current) {
         return path_list->items[i+1];
      }
   }

   return NULL;
}


const char *roadmap_path_last (const char *name)
{
   RoadMapPathList path_list = roadmap_path_find (name);

   if (path_list == NULL) {
      roadmap_log (ROADMAP_FATAL, "invalid path set '%s'", name);
   }

   if (path_list->count > 0) {
      return path_list->items[path_list->count-1];
   }
   return NULL;
}


const char *roadmap_path_previous (const char *name, const char *current)
{
   int i;
   RoadMapPathList path_list = roadmap_path_find (name);

   for (i = path_list->count-1; i > 0; --i) {

      if (path_list->items[i] == current) {
         return path_list->items[i-1];
      }
   }
   return NULL;
}

/* This function always return a hardcoded default location,
 * which is the recommended location for these objects.
 */
const char *roadmap_path_preferred (const char *name)
{
   RoadMapPathList path_list = roadmap_path_find (name);

   if (path_list == NULL) {
      roadmap_log (ROADMAP_FATAL, "invalid path set '%s'", name);
   }

   return path_list->preferred;
}

/*
 * Thread safe implementation
 */
void roadmap_path_create (const char *path)
{
  RFs fs;
  char pathSlash[256];
  if ( path == NULL ) {return;}

  fs.Connect();

  strncpy_safe (pathSlash, path, sizeof (pathSlash) - 1);
  if (pathSlash[strlen (pathSlash) - 1] != '\\')
    strcat (pathSlash, "\\");

  TFileName pathName;
  GSConvert::CharPtrToTDes16(pathSlash, pathName);

  if ( fs.Handle() )
  {
	  fs.MkDirAll( pathName );
  }
  fs.Close();
}


static char *RoadMapPathEmptyList = NULL;

char **roadmap_path_list (const char *path, const char *extension)
{
  TFileName strPath;
  TPtrC8 ptrPath((const unsigned char*)path, User::StringLength((const TUint8*)path));
  CnvUtfConverter::ConvertToUnicodeFromUtf8(strPath, ptrPath);
  TFileName strExt;
  if (extension != NULL){
	  TPtrC8 ptrExt((const unsigned char*)extension, User::StringLength((const TUint8*)extension));
	  CnvUtfConverter::ConvertToUnicodeFromUtf8(strExt, ptrExt);
  }

  TFileName strFullname;
  _LIT(KDelims, "\\*");
  strFullname = strPath;
  strFullname.Append(KDelims);
  if (extension != NULL)
	  strFullname.Append(strExt);

  char **result;
  char **cursor;
  CDir* fileList;
  TInt err = CEikonEnv::Static()->FsSession().GetDir( strFullname,
                                                      KEntryAttNormal,
                                                      ESortNone,
                                                      fileList);
  if ( err != KErrNone )
  {
    return &RoadMapPathEmptyList;
  }

  int numOfFiles = fileList->Count();
  if (numOfFiles == 0)
  {
    return &RoadMapPathEmptyList;
  }

  cursor = result = (char**)calloc (numOfFiles+1, sizeof(char *));
  roadmap_check_allocated (result);

  for (TInt i=0;i<numOfFiles;i++)
  {
    int bits = ((*fileList)[i].iName.Length()+1)* 4;
    unsigned char* textBuf = (unsigned char*)calloc(bits,1);
    TPtr8 textBufPtr(textBuf, bits+1);
    CnvUtfConverter::ConvertFromUnicodeToUtf8(textBufPtr, (*fileList)[i].iName);
    *(cursor++) = (char*)textBuf;
  }
  delete fileList;

  *cursor = NULL;
  return result;
}


void roadmap_path_list_free (char **list)
{
   char **cursor;

   if ((list == NULL) || (list == &RoadMapPathEmptyList)) return;

   for (cursor = list; *cursor != NULL; ++cursor) {
      free (*cursor);
   }
   free (list);
}


void roadmap_path_free (const char *path)
{
   free ((char *) path);
}


const char *roadmap_path_search_icon (const char *name)
{
   static char result[256];

   sprintf (result, "%s\\icons\\rm_%s.png", roadmap_path_user(), name);
   if (roadmap_file_exists(NULL, result)) return result;

   sprintf (result, "z:\\private\\2001EB29\\roadmap\\icons\\rm_%s.png", name);
   if (roadmap_file_exists(NULL, result)) return result;

   return NULL; /* Not found. */
}


int roadmap_path_is_full_path (const char *name)
{
#ifdef UNDER_CE
   return name[0] == '\\';
#else
   return name[1] == ':';
#endif
}


const char *roadmap_path_temporary (void) {

   return roadmap_path_user();
}

int roadmap_path_is_directory (const char *name)
{
  if ( name == NULL ) {return 0;}

  TFileName pathName;
  GSConvert::CharPtrToTDes16(name, pathName);

  TBool isFolder = false;
  TInt err = EikFileUtils::CheckWhetherFullNameRefersToFolder(pathName, isFolder);
  if ( err != KErrNone )
  {// we couldn't know, assume false
    return 0;
  }
  return true; //AVI
}


const char *roadmap_path_config( void )
{
	return roadmap_path_user();
}
