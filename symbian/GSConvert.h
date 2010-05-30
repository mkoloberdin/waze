/* GSConvert.h
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
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
 */

#ifndef __GS_CONVERT_H__
#define __GS_CONVERT_H__

#include <e32std.h>

//  Generic namespace for conversion methods
class GSConvert
{
public:   
  //  if len > 0 then we use it;
  //  if it's 0 (default) then get length from the ptr 
  static void CharPtrToTDes16(const char* p, TDes16& des16, int len = 0);
  static void CharPtrToTDes8(const char* p, TDes8& des8);
  //  we usually want to alloc the buffer ourselves
  static void TDes16ToCharPtr(const TDes16& des16, char** p, bool shouldAlloc = true);
};

#endif  //  __GS_CONVERT_H__
