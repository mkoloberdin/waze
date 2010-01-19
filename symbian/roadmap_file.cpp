/* roadmap_file.cpp - a module to open/read/close a roadmap database file.
 *
 * LICENSE:
 *
 *   Copyright 2008 Giant Steps Ltd.
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
 *   See roadmap_file.h.
 */

extern "C" {
#include "roadmap.h"
#include "roadmap_path.h"
#include "roadmap_file.h"
}

#include <string.h>
#include <stdlib.h>
#include <f32file.h>
#include <eikfutil.h>
#include <coeutils.h>

#include "GSConvert.h"
#include "Roadmap_NativeFile.h"

struct RoadMapFileContextStructure 
{
  CRoadMapNativeFile *fptr;
  TPtr8              *map;
};

//  Helper methods for filesession handling
/*
RFs& OpenFileSession()
{
  static CCoeEnv *env = CEikonEnv::Static();
  if ( env != NULL )
  {
    return env->FsSession();
  }
  else
  {
    RFs *fsSession = new RFs();
    User::LeaveIfError(fsSession->Connect());
    return *fsSession;
  }
}

void CloseFileSession(RFs& fs)
{
  if ( CEikonEnv::Static() == NULL )
  {
    fs.Close();
    delete &fs;
  }
}
*/

RoadMapFile roadmap_file_open  (const char *name, const char *mode)
{
  TFileName fileName;
  CRoadMapNativeFile::EFileOp file_op;
   
  if (strcmp(mode, "r") == 0) {
     file_op = CRoadMapNativeFile::EFileOpRead;
  } else if (strcmp (mode, "rw") == 0) {
     file_op = CRoadMapNativeFile::EFileOpReadWrite;
  } else if (strchr (mode, 'w') != NULL) {
     file_op = CRoadMapNativeFile::EFileOpWrite;
  } else if (strchr (mode, 'a') != NULL) {
     file_op = CRoadMapNativeFile::EFileOpAppend;
  } else {
      roadmap_log (ROADMAP_ERROR,
          "%s: invalid file access mode %s", name, mode);
     return ROADMAP_INVALID_FILE;
  }
  
  GSConvert::CharPtrToTDes16(name, fileName);
  
  // Create the file
  TInt err = KErrNone;
  CRoadMapNativeFile* pFile = CRoadMapNativeFile::NewL( fileName, file_op, err ); //TODO TRAPD
  if ( err != KErrNone ) // || errVal != KErrNone )
  {
    //TODO roadmap_log?
    delete pFile;
    pFile = NULL;
  }
  return pFile;
}

int   roadmap_file_read  (RoadMapFile file, void *data, int size)
{
  if ( file == NULL || size == 0 || data == NULL ) {return 0;}
  CRoadMapNativeFile* pFile = (CRoadMapNativeFile*)file;
  return pFile->Read(data, size);
}

int   roadmap_file_write (RoadMapFile file, const void *data, int length)
{
  if ( file == NULL || length == 0 || data == NULL ) {return 0;}
  CRoadMapNativeFile* pFile = (CRoadMapNativeFile*)file;
  return pFile->Write(data, length);
}

void  roadmap_file_close (RoadMapFile file)
{
  if ( file == NULL ) {return;}
  CRoadMapNativeFile* pFile = (CRoadMapNativeFile*)file;
  delete pFile; //  this flushes the file buffer and closes the fs session
}

void  roadmap_file_remove (const char *path, const char *name)
{
  const char *full_name = roadmap_path_join (path, name);
  if ( full_name == NULL ) {return;}
  
  TFileName fileName;
  GSConvert::CharPtrToTDes16(full_name, fileName);
  roadmap_path_free (full_name);
  
  TInt res = EikFileUtils::DeleteFile(fileName);
}

int   roadmap_file_exists (const char *path, const char *name)
{
  const char *full_name = roadmap_path_join (path, name);
  if ( full_name == NULL ) {return 0;}
  
  TFileName fileName;
  GSConvert::CharPtrToTDes16(full_name, fileName);
  roadmap_path_free (full_name);
  
  TBool res = ConeUtils::FileExists(fileName);
  return ( res == TRUE );
}

int roadmap_file_length (const char *path, const char *name)
{
  const char *full_name = roadmap_path_join (path, name);
  if ( full_name == NULL ) {return 0;}

  TFileName fileName;
  GSConvert::CharPtrToTDes16(full_name, fileName);
  roadmap_path_free (full_name);
  
  TEntry entry;
  TInt res = CEikonEnv::Static()->FsSession().Entry(fileName, entry);
  return entry.iSize;
}

void  roadmap_file_save (const char *path, const char *name, void *data, int length)
{
  const char *full_name = roadmap_path_join (path, name);
  if ( full_name == NULL ){ return; }
  
  RoadMapFile file = roadmap_file_open(full_name, "w");
  roadmap_path_free (full_name);

  if (file != ROADMAP_INVALID_FILE) 
  {
    roadmap_file_write(file, data, length);
    roadmap_file_close(file);
  }
}

void roadmap_file_append (const char *path, const char *name, void *data, int length)
{
  const char *full_name = roadmap_path_join (path, name);
  if ( full_name == NULL ){ return; }
  
  RoadMapFile file = roadmap_file_open(full_name, "a");
  roadmap_path_free (full_name);

  if (file != ROADMAP_INVALID_FILE) 
  {
    roadmap_file_write(file, data, length);
    roadmap_file_close(file);
  }
}

int roadmap_file_rename (const char *old_name, const char *new_name)
{
  TFileName oldName, newName;
  GSConvert::CharPtrToTDes16(old_name, oldName);
  GSConvert::CharPtrToTDes16(new_name, newName);
  
  TInt res = EikFileUtils::RenameFile(oldName, newName);

  return ( res != KErrNone );
}

int roadmap_file_truncate (const char *path, const char *name, int length)
{
  const char *full_name = roadmap_path_join (path, name);
  if ( full_name == NULL ){ return 0; }
  
  RoadMapFile file = roadmap_file_open(full_name, "a");
  roadmap_path_free (full_name);

  TInt err = KErrNone;
  if (file != ROADMAP_INVALID_FILE) 
  {
    err = ((CRoadMapNativeFile*)(file))->SetSize(length);
    roadmap_file_close(file);
  }  
  return (err != KErrNone);
}

FILE *roadmap_file_fopen (const char *path, const char *name, const char *mode)
{
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

const char *roadmap_file_map (const char *set,
                              const char *name,
                              const char *sequence,
                              const char *mode,
                              RoadMapFileContext *file)
{
  CRoadMapNativeFile::EFileOp open_mode;
  RoadMapFileContext context = new RoadMapFileContextStructure();
  if ( context == NULL ){ return NULL;  }
  
  context->fptr = ROADMAP_INVALID_FILE;
  context->map = NULL;

  if (strcmp(mode, "r") == 0) {
    open_mode = CRoadMapNativeFile::EFileOpRead;
  } else if (strchr (mode, 'w') != NULL) {
    open_mode = CRoadMapNativeFile::EFileOpWrite;
  } else {
     roadmap_log (ROADMAP_ERROR,
                  "%s: invalid file access mode %s", name, mode);
     delete context;
     return NULL;
  }

  if (name[0] == '\\' || (name[1] == ':')) 
  {//TODO use TParse
     context->fptr = (CRoadMapNativeFile*)roadmap_file_open(name, mode);
     sequence = ""; // Whatever, but NULL.
  } 
  else 
  {
     int size;

     if (sequence == NULL) {
        sequence = roadmap_path_first(set);
     } else {
        sequence = roadmap_path_next(set, sequence);
     }
     if (sequence == NULL) {
        delete context;
        return NULL;
     }

     RBuf8 full_name;
     full_name.Create(KMaxFileName);
     full_name = (const unsigned char*)sequence;
     int name_len = User::StringLength((const unsigned char*)name);
     
     // Find the first file
     do 
     {
        size =  User::StringLength((const unsigned char*)sequence)
              + name_len
              + 2;

        if (size >= full_name.MaxLength())
        {
          roadmap_log(ROADMAP_ERROR, "filename %s is too long", (const char*)(full_name.Ptr()));
          full_name.ReAlloc(size);
        }

        full_name = (const unsigned char*)sequence;
        full_name.Append(KPathDelimiter);
        full_name.Append((const unsigned char*)name, name_len);
        full_name.ZeroTerminate();

        context->fptr = (CRoadMapNativeFile*)roadmap_file_open (((const char *)full_name.Ptr()), mode);

        if (context->fptr != ROADMAP_INVALID_FILE) {  break;  }

        sequence = roadmap_path_next(set, sequence);
        
     } while (sequence != NULL);
     
     full_name.Close(); //  although we don't strictly need this
  }

  if (context->fptr == ROADMAP_INVALID_FILE) 
  {
     if (sequence == NULL) {
        roadmap_log (ROADMAP_INFO, "cannot open file %s", name);
     }
     roadmap_file_unmap (&context);
     return NULL;
  }

  int fileSize = 0;
  int retVal = context->fptr->GetSize(fileSize);
  //TODO make this one use the ported roadmap_file_length?
  if (retVal != KErrNone || fileSize == 0)
  {
    if (sequence == 0)
    {
      roadmap_log(ROADMAP_ERROR, "cannot get size of file %s", name);
    }
    roadmap_file_unmap(&context);
    return NULL;
  }
  
  TUint8* mapBuf = new TUint8[fileSize];
  context->map = new TPtr8(mapBuf, fileSize);
  int numRead = context->fptr->Read(mapBuf, fileSize);
  if ( numRead == 0 )
  {
    //some kind of problem loading the file to memory
    roadmap_log(ROADMAP_ERROR, "cannot read/map file %s", name);
    delete[] mapBuf;
    delete context->map;
  }
  
  if (mapBuf == NULL) {
     roadmap_log (ROADMAP_ERROR, "cannot map file %s", name);
     roadmap_file_unmap (&context);
     return NULL;
  }
  //  managed to read and everything is fine, set the valid descriptor length
  context->map->SetLength(numRead);

  *file = (RoadMapFileContext)context;

  return sequence; // Indicate the next directory in the path. 
}

void *roadmap_file_base (RoadMapFileContext file)
{
  if (file == NULL || file->map == NULL) {
     return NULL;
  }
  return (void*)(file->map->Ptr());
}

int   roadmap_file_size (RoadMapFileContext file)
{
  if (file == NULL || file->map == NULL) {
     return 0;
  }
  return file->map->Length();
}

int  roadmap_file_sync (RoadMapFileContext file)
{
  int written;
  if ((file->fptr != NULL) && (file->map != NULL) && (file->map->Length() != 0))
  {
    written = roadmap_file_write((CRoadMapNativeFile*)(file->fptr) , 
                                  file->map->Ptr(), 
                                  file->map->Length());
  }
  
  return ( written == 0 ? -1 : 0 );
}

void roadmap_file_unmap (RoadMapFileContext *file)
{
  RoadMapFileContext context = *file;

  if (context->map != NULL) 
  {// free all memory allocated in the mapping function
    delete[] context->map->Ptr();
    delete context->map;  
  }

  if (context->fptr != NULL) {
     delete (CRoadMapNativeFile*)(context->fptr);
  }
  
  delete context;
  *file = NULL;
}

const char *roadmap_file_unique (const char *base)
{//TODO what do we need this for exactly? 
  TFileName fileName;
  TFileName basePath;
  GSConvert::CharPtrToTDes16(base, basePath);
  
  RFile file;
  TInt err = file.Temp(CEikonEnv::Static()->FsSession(), basePath, fileName, EFileShareAny);

  return NULL;
}

int roadmap_file_free_space (const char *path)
{
  if ( path == NULL ){  return 0; }
  
  int freeSpace = 0;
  TInt driveNum;
  TFileName pathDesc;
  GSConvert::CharPtrToTDes16(path, pathDesc);
  
  TInt err;
  TParse pathParse;
  pathParse.Set(pathDesc, NULL, NULL);
  if ( pathParse.DrivePresent() == false )
  {
    driveNum = KDefaultDrive;
  }
  else
  {
    TPtrC drivePath = pathParse.Drive();
    err = CEikonEnv::Static()->FsSession().CharToDrive((TChar)drivePath[0], driveNum);
    if (err != KErrNone) return freeSpace;
  }
  
  TVolumeInfo volumeInfo; 
  err = CEikonEnv::Static()->FsSession().Volume(volumeInfo, driveNum);
  if (err == KErrNone)
  {
    freeSpace = volumeInfo.iFree;
  }

  return freeSpace;
}

int roadmap_file_seek(RoadMapFile file, int offset, RoadMapSeekWhence whence)
{
	TSeek method;

	switch (whence) {
		case ROADMAP_SEEK_START:
			method = ESeekStart;
			break;
		case ROADMAP_SEEK_CURR:
			method = ESeekCurrent;
			break;
		case ROADMAP_SEEK_END:
			method = ESeekEnd;
			break;
		default:
			roadmap_log (ROADMAP_ERROR, "illegal whence param %d", (int)whence);
			return -1;
	}

  CRoadMapNativeFile* pFile = (CRoadMapNativeFile*)file;
  return (int)pFile->Seek(method, offset );
}
