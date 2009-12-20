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

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <locale.h>
#include "string_parser.h"

#ifndef   NULL
   #define NULL   (0)
#endif   //   NULL
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
//      o   szStr             - [in]      Source string
//      o   szValueTermination- [in,opt]  Characters that terminate the integer value
//      o   szAllowedPadding  - [in,opt]  Characters that are allowed within the integer value
//      o   pValue            - [out]     Output value
//      o   bTrimEnd          - [in]      Remove additional termination chars from 'szStr'
//
//   Remarks:   The integer value does not have to be NULL terminated
//
const char*   ReadInt64FromString(   
      const char* szStr,               //   [in]      Source string
      const char* szValueTermination,  //   [in,opt]  Characters that terminate the integer value
      const char* szAllowedPadding,    //   [in,opt]  Characters allowed within the integer value
      long  long* pValue,              //   [out]     Output value
      int         iTrimCount)          //   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'
{
   BOOL  bMinus = FALSE;

   (*pValue) = 0;
   
   while( (*szStr) && (!szValueTermination || (NULL == strchr( szValueTermination, (*szStr)))))
   {
      if( ('0' <= (*szStr)) && ((*szStr) <= '9'))
      {
         (*pValue) *= 10;
         (*pValue) += ((*szStr) - '0');
      }
      else if( '-' == (*szStr))
         bMinus = TRUE;
      else
      {
         if( !szAllowedPadding || (NULL == strchr( szAllowedPadding, (*szStr))))
            return NULL;
      }
      
      szStr++;
   }
   
   if( bMinus)
      (*pValue) *= -1;
   
   if( szValueTermination && (DO_NOT_TRIM != iTrimCount))
      return EatChars( szStr, szValueTermination, iTrimCount);
   
   return szStr;
}

const char*   ReadIntFromString(   
   const char* szStr,               //   [in]      Source string
   const char* szValueTermination,  //   [in,opt]  Characters that terminate the integer value
   const char* szAllowedPadding,    //   [in,opt]  Characters allowed within the integer value
   int*        pValue,              //   [out]     Output value
   int         iTrimCount)          //   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'
{
   long long i64Val;
   const char*   pRes = ReadInt64FromString(   
               szStr,               //   [in]      Source string
               szValueTermination,  //   [in,opt]  Characters that terminate the integer value
               szAllowedPadding,    //   [in,opt]  Characters allowed within the integer value
               &i64Val,             //   [out]     Output value
               iTrimCount);         //   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   (*pValue) = (int)i64Val;
   return pRes;                           
}                           

double m_atof(char *s)
{
        double a = 0.0;
        int e = 0;
        int c;
        int negative = 1;
        
        if (*s == '-'){
           negative = -1;
           s += 1;
        }
        while ((c = *s++) != '\0' && isdigit(c)) {
                a = a*10.0 + (c - '0');
        }
        if (c == '.') {
                while ((c = *s++) != '\0' && isdigit(c)) {
                        a = a*10.0 + (c - '0');
                        e = e-1;
                }
        }
        if (c == 'e' || c == 'E') {
                int sign = 1;
                int i = 0;
                c = *s++;
                if (c == '+')
                        c = *s++;
                else if (c == '-') {
                        c = *s++;
                        sign = -1;
                }
                while (isdigit(c)) {
                        i = i*10 + (c - '0');
                        c = *s++;
                }
                e += i*sign;
        }
        while (e > 0) {
                a *= 10.0;
                e--;
        }
        while (e < 0) {
                a *= 0.1;
                e++;
        }
        return a * negative;
}

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
                  int         iTrimCount)          //   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'
{
   int   i     = 0;
   char  sDouble[STRING_DOUBLE_MAXSIZE+1];

   (*pValue) = 0.F;
   
   memset( sDouble, 0, sizeof(sDouble));
   
   while( szStr[i] && (!szValueTermination || (NULL == strchr( szValueTermination, szStr[i]))))
   {
      char ch = szStr[i];
   
      if( STRING_DOUBLE_MAXSIZE < i)
         return NULL;

      if( (('0' <= ch) && (ch <= '9')) || ('.'==ch) || ('-' == ch))
         sDouble[i] = ch;
      else
      {
         if( !szAllowedPadding || (NULL == strchr( szAllowedPadding, ch)))
            return NULL;
      }
      
      i++;
   }
   
   if( !i)
      return NULL;
      
#ifdef __SYMBIAN32__      
   (*pValue) = m_atof(sDouble);
#else
  (*pValue) = atof(sDouble);
#endif   

   
   if( szValueTermination && (DO_NOT_TRIM != iTrimCount))
      return EatChars( szStr + i, szValueTermination, iTrimCount);
   
   return szStr + i;
}


//   Method:   ExtractString
//
//   Abstract: Copy string, until end of string, or until one termination-characters is reached
//
//   Return:   If succeeds, return the end of string processed.
//             If fails, return NULL
//
//   Parameters:
//
//      o   szSrc    - [in]      Source string
//      o   szDst    - [out,opt] Output buffer
//      o   pDstSize - [in,out]  Buffer size / Size of extracted string
//      o   szChars  - [out]     Array of one or more chars to terminate the copy operation
//      o   bTrimEnd - [in]      Remove additional termination chars from 'szSrc'
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
            const char* szSrc,      // [in]      Source string
            char*       szDst,      // [out,opt] Output buffer
            int*        pDstSize,   // [in,out]  Buffer size / Size of extracted string
            const char* szChars,    // [in]      Array of chars to terminate the copy operation
            int         iTrimCount) // [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'
{
   int   i = 0;

   if( !szSrc || !pDstSize || !(*pDstSize) || !szChars || !(*szChars))
      return NULL;
   
   if(szDst)
      szDst[i] = '\0';
   while( szSrc[i] && (NULL == strchr( szChars, szSrc[i])))
   {
      if( (*pDstSize) <= (i+1))
         return NULL;
      
      if(szDst)
      {
         szDst[i] = szSrc[i];
         i++;
         szDst[i] = '\0';
      }
      else
         i++;
   }
   
   (*pDstSize) = i;
   
   if( DO_NOT_TRIM != iTrimCount)
      return EatChars( szSrc + i, szChars, iTrimCount);

   return szSrc + i;
}


// Method:     ExtractNetworkString
//
// Abstract:   Same as 'ExtractString' (see documentation above), with a single difference:
//             Supporting escape sequences chars ('\,', '\r', '\n', '\t'...)
//
// Details:    If the escape sequence tag '\' will be encountered then current char will be 
//             ignored, and the following char will be treated as the escape-squence 
//             value ('\r' for 'r', etc).
//             Note that a delimiter character will not be used as a delimiter if it is 
//             proceeded with the escape sequence tag.
const char* ExtractNetworkString(   
            const char* szSrc,      // [in]     Source string
            char*       szDst,      // [out,opt]Output buffer
            int*        pDstSize,   // [in,out] Buffer size / Size of extracted string
            const char* szChars,    // [in]     Array of chars to terminate the copy operation
            int         iTrimCount) // [in]     TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'
{
   int   iSrc     = 0;
   int   iDst     = 0;
   BOOL  bFoundES = FALSE;

   if( !szSrc || !pDstSize || !(*pDstSize) || !szChars || !(*szChars))
      return NULL;
   
   if(szDst)
      szDst[0] = '\0';
      
   while( szSrc[iSrc])
   {
      char  ch = szSrc[iSrc++];

      if( bFoundES)
      {
         switch(ch)
         {
            case 'r':
            case 'R':
               ch = '\r';
               break;

            case 'n':
            case 'N':
               ch = '\n';
               break;

            case 't':
            case 'T':
               ch = '\t';
               break;
         }

         bFoundES = FALSE;
      }
      else
      {
         if( ESCAPE_SEQUENCE_TAG == ch)
         {
            bFoundES = TRUE;
            continue;
         }

         if( strchr( szChars, ch))
         {
            iSrc--;
            break;
         }
      }

      if( (*pDstSize) <= (iDst+1))
         return NULL;
      
      if(szDst)
      {
         szDst[iDst++]  = ch;
         szDst[iDst]    = '\0';
      }
      else
         iDst++;
   }
   
   (*pDstSize) = iDst;
   
   if( DO_NOT_TRIM != iTrimCount)
      return EatChars( szSrc + iSrc, szChars, iTrimCount);

   return szSrc + iSrc;
}

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
            int         iDstSize)   //   [in]   Output buffer size
{
   int   iSrc = 0;
   int   iDst = 0;
   char  ESC_VAL;

   if( !szSrc || !szDst || !iDstSize || (szSrc == szDst))
      return FALSE;
   
   if( iDstSize)
      (*szDst) = '\0';

   while( szSrc[iSrc])
   {
      char chSrc = szSrc[iSrc++];
      
      switch( chSrc)
      {
         case '\r':
            ESC_VAL = 'r';
            break;

         case '\n':
            ESC_VAL = 'n';
            break;

         case '\t':
            ESC_VAL = 't';
            break;

         case ',':
         case '\\':
            ESC_VAL = chSrc;
            break;

         default:
            ESC_VAL = '\0';
      }

      if( ESC_VAL)
      {
         if( iDstSize <= (iDst+2))
            return FALSE;

         szDst[iDst++] = '\\';
         szDst[iDst++] = ESC_VAL;
         szDst[iDst  ] = '\0';
      }
      else
      {
         if( iDstSize <= (iDst+1))
            return FALSE;

         szDst[iDst++] = chSrc;
         szDst[iDst  ] = '\0';
      }
   }

   return TRUE;
}

////////////////////////
//   Method:      EatChars
//
//   Abstract:   Consume chars 'szChars' from the beginning of source string 'szStr'.
//            For any character in the beginnig of the string 'szStr', which also apears 
//            in the set 'szChars', the string (szStr) is prgressed one character forward.
//
//   Return:      Return the first character in 'szStr', which does not apear in 'szChars'.
//            If end-of-string is reached, the terminating NULL ('\0') is returned.
//
//   Parameters:
//
//      o   szStr    - [in]   Source string
//      o   szChars  - [out]  Array of one or more chars to be removed from the beginning of 'szStr'
//
const char*   EatChars(
            const char* szStr,   // [in]  Source string
            const char* szChars, // [in]  Array of chars to skip
            int         count)   // [in]  Number of chars to eat, or TRIM_ALL_CHARS
{
   if( !szStr)
      return NULL;
   
   if( !(*szStr) || !szChars || !(*szChars) || (DO_NOT_TRIM == count))
      return szStr;

   while( (*szStr) && (NULL != strchr( szChars, (*szStr))) && (count != 0))
   {
      szStr++;

      if( TRIM_ALL_CHARS != count)
         count--;
   }
         
   
   return szStr;
}

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
               int         count)      // [in]  Number of chars to eat, or TRIM_ALL_CHARS
{
   if( !szStr)
      return NULL;
   
   if( !(*szStr) || !szChars || !(*szChars) || (DO_NOT_TRIM == count))
      return szStr;

   while( (*szStr) && (NULL == strchr( szChars, (*szStr))) && (count != 0))
   {
      szStr++;

      if( TRIM_ALL_CHARS != count)
         count--;
   }
         
   
   return szStr;
}
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
char* AppendPrefix_ShiftOriginalRight( const char* szPrefix, char* szOriginal)
{
   int   iPreStrSize = strlen( szPrefix);
   int   iOrgStrSize = strlen( szOriginal);

   memmove( (void*)(szOriginal + iPreStrSize), (void*)szOriginal, (iOrgStrSize+1));
   memcpy ( (void*)szOriginal, szPrefix, iPreStrSize);

   return szOriginal;
}

// Link two strings by prefixing 'szOriginal' with 'szPrefix'
// 
// Link method:   
//    Copy prefix to the left of the original string, at offset MINUS n (szOriginal[-n]), where
//    'n' is the size of 'szPrefix'.
//    Original string is not moved.
//
// Return value:
//    New string
char* AppendPrefix_CopyToTheLeft( const char* szPrefix, char* szOriginal)
{
   int   iPreStrSize = strlen( szPrefix);
   char* szDest      = szOriginal - iPreStrSize;

   memcpy ( (void*)szDest, szPrefix, iPreStrSize);
   return szDest;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL  InsertString_AfterTag(  const char* szSrc,      // [in]  Source string
                              const char* szTag,      // [in]  Insert new string AFTER this tag
                              const char* szStr,      // [in]  New string to insert
                              char*       szDst,      // [out] Put it here
                              int         iDstSize)   // [in]  Size of buffer
{
   const char* szFound = NULL;
   int         iFoundOffset;
   int         iSrcSize;
   int         iTagSize;
   int         iStrSize;
   int         iNewSize;

   if(STRING_IS_EMPTY(szSrc)   ||
      STRING_IS_EMPTY(szTag)   ||
      STRING_IS_EMPTY(szStr)   ||
      !szDst || !iDstSize)
      return FALSE;   //   Invalid argument

   iSrcSize = strlen(szSrc);
   iTagSize = strlen(szTag);
   iStrSize = strlen(szStr);
   iNewSize = (iSrcSize + iStrSize);

   if( iDstSize <= iNewSize)
      return FALSE;
   szDst[iNewSize] = '\0';

   szFound = strstr( szSrc, szTag);
   if( !szFound)
      return FALSE;

   iFoundOffset = (int)(szFound - szSrc);

   if( szSrc == szDst)
   {
      //   Copy-1:   Prefix + tag
      //      Already done in this case...
      //   Copy-2:   Postfix
      memmove( (szDst+iFoundOffset+iTagSize+iStrSize), szFound+iTagSize, strlen(szFound+iTagSize));
      //   Copy-3:   Insertion
      strncpy( szDst+iFoundOffset+iTagSize, szStr, iStrSize);
   }
   else
   {
      //   Copy-1:   Prefix + tag
      strncpy( szDst, szSrc, iFoundOffset+iTagSize);
      //   Copy-2:   Postfix
      strcpy( (szDst+iFoundOffset+iTagSize+iStrSize), szFound+iTagSize);
      //   Copy-3:   Insertion
      strncpy( szDst+iFoundOffset+iTagSize, szStr, iStrSize);
   }
   

   return TRUE;
}

BOOL  InsertString_BeforeTag( const char* szSrc,      // [in]  Source string
                              const char* szTag,      // [in]  Insert new string BEFORE this tag
                              const char* szStr,      // [in]  New string to insert
                              char*       szDst,      // [out] Put it here
                              int         iDstSize)   // [in]  Size of buffer
{
   const char* szFound = NULL;
   int         iFoundOffset;
   int         iSrcSize;
   int         iTagSize;
   int         iStrSize;
   int         iNewSize;

   if(STRING_IS_EMPTY(szSrc)   ||
      STRING_IS_EMPTY(szTag)   ||
      STRING_IS_EMPTY(szStr)   ||
      !szDst || !iDstSize)
      return FALSE;   //   Invalid argument

   iSrcSize = strlen(szSrc);
   iTagSize = strlen(szTag);
   iStrSize = strlen(szStr);
   iNewSize = (iSrcSize + iStrSize);

   if( iDstSize <= iNewSize)
      return FALSE;
   szDst[iNewSize] = '\0';

   szFound = strstr( szSrc, szTag);
   if( !szFound)
      return FALSE;

   iFoundOffset = (int)(szFound - szSrc);

   if( szSrc == szDst)
   {
      //   Copy-1:   Prefix + tag
      //      Already done in this case...
      //   Copy-2:   Postfix
      memmove( (szDst+iFoundOffset+iStrSize), szFound, strlen(szFound));
      //   Copy-3:   Insertion
      strncpy( szDst+iFoundOffset, szStr, iStrSize);
   }
   else
   {
      //   Copy-1:   Prefix + tag
      strncpy( szDst, szSrc, iFoundOffset);
      //   Copy-2:   Postfix
      strcpy( (szDst+iFoundOffset+iStrSize), szFound);
      //   Copy-3:   Insertion
      strncpy( szDst+iFoundOffset, szStr, iStrSize);
   }
   

   return TRUE;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
char  LowerChar( char ch)
{
   if( ('A'<=ch) && (ch<='Z'))
      return ch + ('a' - 'A');
   return ch;
}

void ToLower( char* szStr)
{
   assert(szStr);
   while( *szStr)
   {
      (*szStr) = LowerChar( *szStr);
      szStr++;
   }
}

void ToLowerN( char* szStr, size_t iCount)
{
   assert(szStr);

   if( iCount < strlen(szStr))
      while( iCount)
      {
         szStr[iCount-1] = LowerChar( szStr[iCount-1]);
         iCount--;
      }
   else
      ToLower( szStr);
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////

