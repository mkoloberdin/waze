/* RoadmapNativeFile.cpp - Symbian file implementation for Roadmap
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
 *   See roadmap_file.h
 */

#include "Roadmap_NativeFile.h"
#include "GSConvert.h"
#include <f32file.h>
#include <e32std.h>

CRoadMapNativeFile::CRoadMapNativeFile()
{
  m_IsFileValid = false;
}

CRoadMapNativeFile::~CRoadMapNativeFile()
{
  if ( m_IsFileValid == true )
  {
    /*TInt res = */iFile.Flush(); //  we don't check if it went ok...
  }
  iSession.Close();
}

CRoadMapNativeFile* CRoadMapNativeFile::NewL( TFileName &fileName, 
                                              EFileOp fileOp, 
                                              TInt &aErrorCode)
{
  CRoadMapNativeFile* self = NewLC(fileName, fileOp, aErrorCode);
  CleanupStack::Pop(self);
  return self;
}

CRoadMapNativeFile* CRoadMapNativeFile::NewLC( TFileName &fileName, 
                                              EFileOp fileOp, 
                                              TInt &aErrorCode)
{
  CRoadMapNativeFile* self = new ( ELeave ) CRoadMapNativeFile();
  CleanupStack::PushL(self);
  self->ConstructL(fileName, fileOp, aErrorCode);
  return self;
}

void CRoadMapNativeFile::ConstructL(TFileName &fileName, 
                                    EFileOp fileOp,
                                    TInt &aErrorCode)
{
  User::LeaveIfError(iSession.Connect());
  aErrorCode = KErrNone;
  TInt offsetFromEnd = 0;
  
  //  Open the file according to the name and op/mode
  switch (fileOp)
  {
  case EFileOpRead:
    aErrorCode = iFile.Open(iSession, fileName, EFileRead );
    break;
  case EFileOpWrite:
    //  Assume the path exists
    aErrorCode = iFile.Replace(iSession, fileName, EFileWrite );
    break;
  case EFileOpReadWrite:
  case EFileOpAppend:
    //  Assume the path exists
    aErrorCode = iFile.Open(iSession, fileName, EFileWrite );
    if (( aErrorCode == KErrNotFound ) && (fileOp == EFileOpAppend))
    {// Create the file please
      aErrorCode = iFile.Create(iSession, fileName, EFileWrite );
    }
    if (KErrNone != aErrorCode)
    {
      return;
    }
    
    if (fileOp == EFileOpAppend) {
       aErrorCode = iFile.Seek(ESeekEnd, offsetFromEnd);
    }

    m_IsFileValid = true; //  Seek can fail, file is still valid
    break;
  default:
    break;
  }
  
  if ( aErrorCode == KErrNone )
  {
    m_IsFileValid = true;
  }
}

int CRoadMapNativeFile::Read(void *data, int length)
{
  TPtr8 ptr((unsigned char *)data, length);
  TInt err = iFile.Read(ptr, length);
  return (err == KErrNone? ptr.Length() : 0);
}

int CRoadMapNativeFile::Write(const void *data, int length)
{
  TPtrC8 ptr((const unsigned char *)data, length);
  TInt err = iFile.Write(ptr, length);
  return (err == KErrNone? ptr.Length() : 0);
}

int CRoadMapNativeFile::GetSize(int& fileSize)
{
  fileSize = 0;
  TInt err = iFile.Size(fileSize);
  return err;
}

int CRoadMapNativeFile::SetSize(int fileSize)
{
  return iFile.SetSize(fileSize);
}

int CRoadMapNativeFile::Seek(TSeek method, int pos)
{
  TInt err = iFile.Seek(method, pos);
  return (err == KErrNone? pos : -1);
}

void CRoadMapNativeFile::WriteAsync( const void *data, int length, RoadMapCallback callback )
{
	CFileAsync *fileAsync = CFileAsync::NewL( callback );
	TPtrC8 ptr( (const unsigned char *)data, length );
	iFile.Write( ptr, length, fileAsync->iStatus );
	fileAsync->Start();
}


/******************** CFileAsync class ********************************************/
inline CFileAsync* CFileAsync::NewL( RoadMapCallback aCallback )
{
	CFileAsync* self = new (ELeave) CFileAsync( aCallback );
	self->ConstructL();
	return self;
}
inline void CFileAsync::Start()
{
	SetActive();
}
inline CFileAsync::CFileAsync( RoadMapCallback aCallback ) : CActive( EPriorityIdle )
{
	iCallback = aCallback;
	CActiveScheduler::Add( this );
}
inline CFileAsync::~CFileAsync()
{
	Cancel();
}
inline void CFileAsync::ConstructL()
{
}


inline void CFileAsync::RunL()
{
	User::LeaveIfError( iStatus.Int() );
	
	if ( iCallback )
	{
		iCallback();
	}
	// Must be the last statement
	delete this;
}

inline void CFileAsync::DoCancel()
{	
}

inline TInt CFileAsync::RunError( TInt aError )
{
	roadmap_log( ROADMAP_WARNING, "Error handling async request: %d", aError );
	return KErrNone;
}

/*************************************************************************************************/
