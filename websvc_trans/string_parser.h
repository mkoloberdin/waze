/*
 * LICENSE:
 *
 *   Copyright 2008 PazO
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


#ifndef   __FREEMAP_STRING_H__
#define   __FREEMAP_STRING_H__
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <string.h>

#include "../roadmap.h"

#define  ESCAPE_SEQUENCE_TAG                 ('\\')
#define  STRING_DOUBLE_MAXSIZE               (31)
#define  TRIM_ALL_CHARS                      (-1)
#define  DO_NOT_TRIM                         ( 0)
#define  HAVE_STRING(_p_)                    ((_p_) && (*(_p_)))
#define  STRING_IS_EMPTY(_p_)                (!(_p_) || !(*(_p_)))
#define  NULLSTRING_TO_EMPTYSTRING(_str_)    if( !(_str_)) (_str_) = "";
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
//   Method:   ReadIntFromString
//
//   Abstract: Extract integer value from a string
//
//   Return:   If succeeds, return the end of string processed.
//             If fails, return NULL
//
//   Parameters:
//
//      o   szStr                - [in]      Source string
//      o   szValueTermination   - [in,opt]  Characters that terminate the integer value
//      o   szAllowedPadding     - [in,opt]  Characters that are allowed within the integer value
//      o   pValue               - [out]     Output value
//      o   bTrimEnd             - [in]      Remove additional termination chars from 'szStr'
//
//   Remarks:   The integer value does not have to be NULL terminated
//
const char*   ReadIntFromString(   
            const char* szStr,               //   [in]     Source string
            const char* szValueTermination,  //   [in,opt] Value termination
            const char* szAllowedPadding,    //   [in,opt] Allowed padding
            int*        pValue,              //   [out]    Output value
            int         iTrimCount);         //   [in]     TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

//   Same as above, but for 64-bit int
const char*   ReadInt64FromString(   
            const char* szStr,               //   [in]     Source string
            const char* szValueTermination,  //   [in,opt] Value termination
            const char* szAllowedPadding,    //   [in,opt] Allowed padding
            long long*  pValue,              //   [out]    Output value
            int         iTrimCount);         //   [in]     TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

////////////////////////////////////
//   Method:   ReadDoubleFromString
//
//   Abstract: Extract a double value from a string
//
//   Return:   If succeeds, return the end of string processed.
//             If fails, return NULL
//
//   Parameters:
//
//      o   szStr             - [in]      Source string
//      o   szValueTermination- [in,opt]  Characters that terminate the double value
//      o   szAllowedPadding  - [in,opt]  Characters that are allowed within the double value
//      o   pValue            - [out]     Output value
//      o   bTrimEnd          - [in]      Remove additional termination chars from 'szStr'
//
//   Remarks:   The double value does not have to be NULL terminated
//
const char*   ReadDoubleFromString(   
               const char* szStr,               //   [in]      Source string
               const char* szValueTermination,  //   [in,opt]  Value termination
               const char* szAllowedPadding,    //   [in,opt]  Allowed padding
               double*     pValue,              //   [out]     Output value
               int         iTrimCount);         //   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'


////////////////////////////
//   Method:   ExtractString
//
//   Abstract: Copy string, until end of string, or until one termination-characters is reached
//
//   Return:   If succeeds, return the end of string processed.
//             If fails, return NULL
//
//   Parameters:
//
//      o   szSrc    - [in]   Source string
//      o   szDst    - [out,opt] Output buffer
//      o   pDstSize - [in,out]  Buffer size / Size of extracted string
//      o   szChars  - [out]  Array of one or more chars to terminate the copy operation
//      o   bTrimEnd - [in]   Remove additional termination chars from 'szSrc'
//
//   Remarks:   
//       If the string to be copied exceeds the size of the output buffer then NULL
//       is returned.
//       Output buffer (szDst) can be NULL, if only the size is needed.
//       Note about 'pDstSize':
//          On entry, pDstSize will hold the size of the buffer. Note that this size must
//          be big enough to contain the terminating null.
//          On exit, pDstSize will hold the size of the extracted string. Note that this 
//          size DOES NOT include the terminating null.
//
const char*   ExtractString(   
               const char* szSrc,      // [in]     Source string
               char*       szDst,      // [out,opt]Output buffer
               int*        pDstSize,   // [in,out] Buffer size / Size of extracted string
               const char* szChars,    // [in]     Array of chars to terminate the copy operation
               int         iTrimCount);// [in]     TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

///////////////////////////////////
// Method:     ExtractNetworkString
//
// Abstract:   Same as 'ExtractString' (see documentation above), with a small difference:
//             Supporting escape sequences chars ('\,', '\r', '\n', '\t'...)
//
// Details:    If an escape-sequence tag ('\') will be encountered then the current 
//             char will be ignored, and the following char will be treated as the 
//             escape-squence value (e.g. - 'r' will be treated as '\r', etc).
//             Note that a delimiter character will not be used as a delimiter if it is 
//             proceeded with an escape sequence tag.
const char* ExtractNetworkString(   
            const char* szSrc,      // [in]     Source string
            char*       szDst,      // [out,opt]Output buffer
            int*        pDstSize,   // [in,out] Buffer size / Size of extracted string
            const char* szChars,    // [in]     Array of chars to terminate the copy operation
            int         iTrimCount);// [in]     TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

////////////////////////////////
//   Method:   PackNetworkString
//
//   Abstract: Convert string to roadmap-network string, by replaceing special characters
//             such as '\r' to a combination of ESCAPE-TAG/ESCAPE-VALUE pairs.
//
//   Return:   TRUE on success
//             FALSE on failure
//
//   Parameters:
//
//      o   szSrc    - [in]   Source string
//      o   szDst    - [opt]  Output buffer
//      o   iDstSize - [in]   Size of output buffer
//
BOOL PackNetworkString( 
            const char* szSrc,      //   [in]   Source string
            char*       szDst,      //   [out]  Output buffer
            int         iDstSize);  //   [in]   Output buffer size
            

////////////////////////
//   Method:   EatChars
//
//   Abstract: Consume chars 'szChars' from the beginning of source string 'szStr'.
//             For any character in the beginnig of the string 'szStr', which also apears 
//             in the set 'szChars', the string (szStr) is prgressed one character forward.
//
//   Return:   Return the first character in 'szStr', which does not apear in 'szChars'.
//             If end-of-string is reached, the terminating NULL ('\0') is returned.
//
//   Parameters:
//
//      o   szStr    - [in]   Source string
//      o   szChars  - [out]  Array of one or more chars to be removed from the beginning of 'szStr'
//
const char*   EatChars(
               const char* szStr,      // [in]  Source string
               const char* szChars,    // [in]  Array of chars to skip
               int         count);     // [in]  Number of chars to eat, or TRIM_ALL_CHARS
            

////////////////////////
//   Method:   SkipChars
//
//   Abstract: Consume chars 'szChars' from the beginning of source string 'szStr'.
//             For any character in the beginnig of the string 'szStr', which does not apears 
//             in the set 'szChars', the string (szStr) is prgressed one character forward.
//
//   Return:   Return the first character in 'szStr', which also appears in 'szChars'.
//             If end-of-string is reached, the terminating NULL ('\0') is returned.
//
//   Parameters:
//
//      o   szStr    - [in]   Source string
//      o   szChars  - [out]  Array of one or more chars to terminate char removal from the beginning of 'szStr'
//
const char*   SkipChars(
               const char* szStr,      // [in]  Source string
               const char* szChars,    // [in]  Array of chars to terminate skipping
               int         count);     // [in]  Number of chars to eat, or TRIM_ALL_CHARS
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
// Link two strings by prefixing 'szOriginal' with 'szPrefix'
// 
// Link method:   
//    Shift original string n chars to the right, and copy the prefix to the original offset
//    location.
//
// Return value:
//    New string
char* AppendPrefix_ShiftOriginalRight( const char* szPrefix, char* szOriginal);

// Link two strings by prefixing 'szOriginal' with 'szPrefix'
// 
// Link method:   
//    Copy prefix to the left of the original string, at offset MINUS n (szOriginal[-n]), where
//    'n' is the size of 'szPrefix'.
//    Original string is not moved.
//
// Return value:
//    New string
char* AppendPrefix_CopyToTheLeft( const char* szPrefix, char* szOriginal);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL  InsertString_AfterTag(  const char* szSrc,      // [in]  Source string
                              const char* szTag,      // [in]  Insert new string AFTER this tag
                              const char* szStr,      // [in]  New string to insert
                              char*       szDst,      // [out] Put it here
                              int         iDstSize);  // [in]  Size of buffer

BOOL  InsertString_BeforeTag( const char* szSrc,      // [in]  Source string
                              const char* szTag,      // [in]  Insert new string BEFORE this tag
                              const char* szStr,      // [in]  New string to insert
                              char*       szDst,      // [out] Put it here
                              int         iDstSize);  // [in]  Size of buffer
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
char  LowerChar( char ch);
void  ToLower  ( char* szStr);
void  ToLowerN ( char* szStr, size_t iCount);
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
#endif   //   __FREEMAP_STRING_H__
