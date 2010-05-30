/*  GSConvert.cpp - Implementation of conversion utilities
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
 *   See roadmap_library.h
 */

#include "GSConvert.h"
#include <string.h>
#include <stdlib.h>
#include <e32base.h>

void GSConvert::CharPtrToTDes16(const char* p, TDes16& des16, int len)
{
  if ( len == 0 )
  {// get length from string
    len = User::StringLength((const unsigned char*)p);
  }
  HBufC8 *pBuf8 = HBufC8::NewL(len); //TODO TRAPD
  pBuf8->Des().Append((const unsigned char *)p, len);
  des16.Copy(pBuf8->Des());
  delete pBuf8; //  can also use NewLC and CleanupStack::PopAndDestroy(pBuf8);
}

void GSConvert::CharPtrToTDes8(const char* p, TDes8& des8)
{
  int len = User::StringLength((const unsigned char*)p);
  des8.Zero();
  des8.Append((unsigned char *)p, len);
}

void GSConvert::TDes16ToCharPtr(const TDes16& des16, char** p, bool shouldAlloc)
{
  int len = des16.Length(); 
  HBufC8 *pBuf8 = HBufC8::NewL(len); //TODO TRAPD
  pBuf8->Des().Copy(des16);
  if ( shouldAlloc == true )
  {
    *p = (char*)calloc(len + 1, 1);
  }
  strncpy(*p, (const char*)pBuf8->Ptr(), len);
  delete pBuf8; //  can also use NewLC and CleanupStack::PopAndDestroy(pBuf8)
}
