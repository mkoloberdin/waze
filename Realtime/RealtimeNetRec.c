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


#include <string.h>
#include <stdlib.h>
#include "../roadmap_messagebox.h"
#include "RealtimeNet.h" 
#include "RealtimeAlerts.h"
#include "RealtimeBonus.h"
#include "RealtimeTrafficInfo.h"
#include "RealtimeOffline.h"
#include "../editor/editor_points.h"
#include "../editor/editor_cleanup.h"
#include "../roadmap_geo_location_info.h"
#include "../roadmap_tripserver.h"
#include "roadmap_login.h"
#include "../roadmap_ticker.h"
#include "../roadmap_twitter.h"
#include "../roadmap_foursquare.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

static void ParseUpdateAccountErrors( int status, roadmap_result*  rc );
static void ParseCreateAccountErrors( int status, roadmap_result*  rc );



//////////////////////////////////////////////////////////////////////////////////////////////////
const char* VerifyStatus(  /* IN  */   const char*       pNext,
                           /* IN  */   void*             pContext,
                           /* OUT */   BOOL*             more_data_needed,
                           /* OUT */   roadmap_result*   rc)
{
   http_response_status Status;
   transaction_result   Res;
   int                  iBytesRead = 0;

   //   Verify we have any data:
   if( strlen(pNext) < 1)
   {

      (*rc) = err_parser_unexpected_data;
      roadmap_log( ROADMAP_ERROR, "VerifyStatus() - Invalid custom data size (%d)", strlen(pNext));
      return NULL;
   }

   //   Search for response status:
   Res = http_response_status_load( &Status, pNext, FALSE, &iBytesRead);

   switch( Res)
   {
      case trans_in_progress:
         (*more_data_needed) = TRUE;
         return pNext;

      case trans_failed:
         if( succeeded == (*rc))
            (*rc) = err_parser_unexpected_data;
         roadmap_log( ROADMAP_ERROR, "VerifyStatus() - Failed to read server response (%s)", pNext);
         return NULL;

      case trans_succeeded:
         break;   // Continue with flow...
   }

   //   Verify response is ok:
   if( 200 != Status.rc)
   {
      switch( Status.rc)
      {
         case 501:
            (*rc) = err_rt_unknown_login_id;
            break;

         case 600:
            (*rc) = err_as_could_not_find_matches; // Address Search: Did not find matches for string
            break;

         default:
            (*rc) = err_failed;
      }

      roadmap_log( ROADMAP_ERROR, "VerifyStatus() - Server response is %d: '%s'", Status.rc, Status.text);
      return NULL;   //   Quit the 'receive' loop
   }

  return pNext + iBytesRead;
}


// The next method verifies two parts of the response:
// 1. [MANDATORY] Standard response-status in the format of
//                "RC,200,<optional status string>\n"
// 2. [OPTIONAL]  The expected data-tag 'szExpectedTag', which
//                can be NULL.
const char* VerifyStatusAndTag(  /* IN  */   const char*       pNext,
                                 /* IN  */   void*             pContext,
                                 /* IN  */   const char*       szExpectedTag,
                                 /* OUT */   BOOL*             more_data_needed,
                                 /* OUT */   roadmap_result*   rc)
{
   char                 Tag[WST_RESPONSE_TAG_MAXSIZE+1];
   http_response_status Status;
   transaction_result   Res;
   int                  iBytesRead     = 0;
   BOOL                 bTryingToLogin = FALSE;
   int                  iBufferSize;

   (*rc) = succeeded;

   if( szExpectedTag && (*szExpectedTag) && !strcmp( szExpectedTag, "LoginSuccessful"))
      bTryingToLogin = TRUE;

   //   Verify we have any data:
   if( strlen(pNext) < 1)
   {
      (*rc) = err_parser_unexpected_data;
      roadmap_log( ROADMAP_ERROR, "RTNet::VerifyStatusAndTag() - Invalid custom data size (%d)", strlen(pNext));
      return NULL;   //   Quit the 'receive' loop
   }

   //   Search for response status:
   Res = http_response_status_load( &Status, pNext, TRUE, &iBytesRead);
   switch( Res)
   {
      case trans_in_progress:
         (*more_data_needed) = TRUE;
         return pNext;

      case trans_failed:
         if( succeeded == (*rc))
            (*rc) = err_parser_unexpected_data;
         roadmap_log( ROADMAP_ERROR, "RTNet::VerifyStatusAndTag() - Failed to read server response (%s)", pNext);
         return NULL;

      case trans_succeeded:
         break;   // Continue with flow...
   }

   //   Verify response is ok:
   if( 200 != Status.rc)
   {
      switch( Status.rc)
      {
         case 501:
            (*rc) = err_rt_unknown_login_id;
            break;

         default:
         {
            if( bTryingToLogin)
               (*rc) = err_rt_login_failed;
            else
               (*rc) = err_failed;

            break;
         }
      }

      roadmap_log( ROADMAP_ERROR, "RTNet::VerifyStatusAndTag() - Server response is %d: '%s'", Status.rc, Status.text);
      return NULL;   //   Quit the 'receive' loop
   }

   ///////////////////////////////////
   //   Response status is Cosher   //
   ///////////////////////////////////

   pNext += iBytesRead;

   //   Where we asked to read tag?
   if( !szExpectedTag)
      return pNext;

   //   Before parsing - see if we have the whole line:
   if( NULL == strchr( pNext, '\n'))
   {
      (*more_data_needed) = TRUE;
      return pNext;   //   Continue reading...
   }

   iBufferSize = WST_RESPONSE_TAG_MAXSIZE;
   pNext = ExtractNetworkString(
                  pNext,      // [in]     Source string
                  Tag,              // [out,opt]Output buffer
                  &iBufferSize,     // [in,out] Buffer size / Size of extracted string
                  ",\r\n",          // [in]     Array of chars to terminate the copy operation
                  TRIM_ALL_CHARS);  // [in]     Remove additional termination chars
   if( !pNext)
   {
      (*rc) = err_parser_unexpected_data;
      roadmap_log( ROADMAP_ERROR, "RTNet::VerifyStatusAndTag() - Failed to read server-response tag");
      return NULL;   //   Quit the 'receive' loop
   }

   //   Found expected tag:
   if( strcmp( szExpectedTag, Tag))
   {
      (*rc) = err_parser_unexpected_data;

      // Special case:  login error
      if( bTryingToLogin && !strcmp( Tag, "LoginError"))
      {
         (*rc) = err_rt_login_failed;

         if( pNext && (*pNext))
         {
            int iErrorCode = atoi(pNext);
            if( 1 == iErrorCode)
               (*rc) = err_rt_wrong_name_or_password;
         }
      }

      roadmap_log( ROADMAP_ERROR, "RTNet::VerifyStatusAndTag() - Tag found (%s) is not the expected tag (%s)",
                  Tag, szExpectedTag);
      return NULL;   //   Quit the 'receive' loop
   }

   return pNext;
}

const char* OnRegisterResponse(  /* IN  */   const char*       pNext,
                                 /* IN  */   void*             pContext,
                                 /* OUT */   BOOL*             more_data_needed,
                                 /* OUT */   roadmap_result*   rc)
{
   int iBufferSize;

   RTNET_ONRESPONSE_BEGIN( "OnRegisterResponse", "RegisterSuccessful")

   // 1. Auto generated name
   iBufferSize = RT_USERNM_MAXSIZE;
   pNext       = ExtractNetworkString(
               pNext,               // [in]     Source string
               pCI->UserNm,         // [out,opt]Output buffer
               &iBufferSize,        // [in,out] Buffer size / Size of extracted string
               ",",                 // [in]     Array of chars to terminate the copy operation
               TRIM_ALL_CHARS);     // [in]     Remove additional termination chars
   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnRegisterResponse() - Did not find (auto generated) user-name in the response");
      (*rc) = err_parser_unexpected_data;
      return NULL;   //   Quit the 'receive' loop
   }

   // 2. Auto generated pw
   iBufferSize = RT_USERPW_MAXSIZE;
   pNext       = ExtractNetworkString(
               pNext,               // [in]     Source string
               pCI->UserPW,         // [out,opt]Output buffer
               &iBufferSize,        // [in,out] Buffer size / Size of extracted string
               ",\r\n",             // [in]     Array of chars to terminate the copy operation
               TRIM_ALL_CHARS);     // [in]     Remove additional termination chars
   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnRegisterResponse() - Did not find (auto generated) user-pw in the response");
      (*rc) = err_parser_unexpected_data;
      return NULL;   //   Quit the 'receive' loop
   }

   // Reset nick-name (not relevant here)
   memset( pCI->UserNk, 0, RT_USERNK_MAXSIZE);

   //   Done
   return pNext;
}

const char* OnLoginResponse(  /* IN  */   const char*       pNext,
                              /* IN  */   void*             pContext,
                              /* OUT */   BOOL*             more_data_needed,
                              /* OUT */   roadmap_result*   rc)
{
   const char* pLast = NULL;   //   For logging
   int         iBufferSize;

   RTNET_ONRESPONSE_BEGIN( "OnLoginResponse", "LoginSuccessful")

   //   1.   Read login-id:
   pLast = pNext;
   pNext = ReadIntFromString( pNext,            //   [in]      Source string
                              ",",              //   [in,opt]  Value termination
                              NULL,             //   [in,opt]  Allowed padding
                              &pCI->iServerID,  //   [out]     Put it here
                              TRIM_ALL_CHARS);  //   [in]      Remove additional termination chars
   if( !pNext || (RT_INVALID_LOGINID_VALUE == pCI->iServerID))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnLoginResponse() - Failed to read Login-ID from response (%s)", pLast);
      (*rc) = err_parser_unexpected_data;
      return NULL;   //   Quit the 'receive' loop
   }

   //   2.   Next value is the server cookie:
   if( !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnLoginResponse() - Did not find server-cookie (secretKet) in the response");
      (*rc) = err_parser_unexpected_data;
      return NULL;   //   Quit the 'receive' loop
   }

   iBufferSize = RTNET_SERVERCOOKIE_MAXSIZE;
   pNext       = ExtractNetworkString(
               pNext,               // [in]     Source string
               pCI->ServerCookie,   // [out,opt]Output buffer
               &iBufferSize,        // [in,out] Buffer size / Size of extracted string
               ",",              // [in]     Array of chars to terminate the copy operation
               1);     // [in]     Remove additional termination chars
   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnLoginResponse(case 2) - Did not find server-cookie (secretKet) in the response");
      (*rc) = err_parser_unexpected_data;
      return NULL;   //   Quit the 'receive' loop
   }

   pNext = ReadIntFromString( pNext,            //   [in]      Source string
                              ",",              //   [in,opt]  Value termination
                              NULL,             //   [in,opt]  Allowed padding
                              &pCI->iMyRanking,  //   [out]     Put it here
                              1);  //   [in]      Remove additional termination chars
   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnLoginResponse() - Failed to read my ranking (%s)", pLast);
      (*rc) = err_parser_unexpected_data;
      return NULL;   //   Quit the 'receive' loop
   }

   pNext = ReadIntFromString( pNext,            //   [in]      Source string
                              ",",              //   [in,opt]  Value termination
                              NULL,             //   [in,opt]  Allowed padding
                              &pCI->iMyTotalPoints,  //   [out]     Put it here
                              1);  //   [in]      Remove additional termination chars
   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnLoginResponse() - Failed to read my total points (%s)", pLast);
      (*rc) = err_parser_unexpected_data;
      return NULL;   //   Quit the 'receive' loop
   }

   

   pNext = ReadIntFromString( pNext,            //   [in]      Source string
                              ",",              //   [in,opt]  Value termination
                              NULL,             //   [in,opt]  Allowed padding
                              &pCI->iMyRating,  //   [out]     Put it here
                              1);  //   [in]      Remove additional termination chars
   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnLoginResponse() - Failed to read my rating (%s)", pLast);
      (*rc) = err_parser_unexpected_data;
      return NULL;   //   Quit the 'receive' loop
   }

   pNext = ReadIntFromString( pNext,            //   [in]      Source string
                              ",",              //   [in,opt]  Value termination
                              NULL,             //   [in,opt]  Allowed padding
                              &pCI->iMyPreviousRanking,  //   [out]     Put it here
                              TRIM_ALL_CHARS);  //   [in]      Remove additional termination chars
   if( !pNext )
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnLoginResponse() - Failed to read my previous ranking (%s)", pLast);
      (*rc) = err_parser_unexpected_data;
      return NULL;   //   Quit the 'receive' loop
   }
   
   //Addon
   pNext = ReadIntFromString( pNext,            //   [in]      Source string
                             ",\r\n",              //   [in,opt]  Value termination
                             NULL,             //   [in,opt]  Allowed padding
                             &pCI->iMyAddon,  //   [out]     Put it here
                             TRIM_ALL_CHARS);  //   [in]      Remove additional termination chars
   if( !pNext )
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnLoginResponse() - Failed to read my addon (%s)", pLast);
      (*rc) = err_parser_unexpected_data;
      return NULL;   //   Quit the 'receive' loop
   }

   pNext = ReadIntFromString( pNext,            //   [in]      Source string
                              ",\r\n",              //   [in,opt]  Value termination
                              NULL,             //   [in,opt]  Allowed padding
                              &pCI->iPointsTimeStamp,  //   [out]     Put it here
                              TRIM_ALL_CHARS);  //   [in]      Remove additional termination chars
   if( !pNext)
   {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnLoginResponse() - Failed to read my points time stamp (%s)", pLast);
         (*rc) = err_parser_unexpected_data;
         return NULL;   //   Quit the 'receive' loop
   }

   //   Done
   editor_points_set_old_points(pCI->iMyTotalPoints, pCI->iPointsTimeStamp); 
   
   //   Update status variables:
   pCI->bLoggedIn = TRUE;

   return pNext;
}

const char* OnLogOutResponse( /* IN  */   const char*       pNext,
                              /* IN  */   void*             pContext,
                              /* OUT */   BOOL*             more_data_needed,
                              /* OUT */   roadmap_result*   rc)
{
   RTNET_ONRESPONSE_BEGIN( "OnLogOutResponse", "LogoutSuccessful")

   //   Update status variables:
   pCI->bLoggedIn    = FALSE;
   pCI->iServerID    = RT_INVALID_LOGINID_VALUE;
   memset( pCI->ServerCookie, 0, sizeof( pCI->ServerCookie));

   return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
//   <UID:int>,[USER-NAME:string],<longitude:double>,<latitude:double>,<azimuth:int(0..359)>,<speed:double>,<last access time:long>,
//             <mood:int>,<title:string>,<addon:int>,<stars:int>,<rank:int>,<points:int>,<joinDate:int(time)>
const char* AddUser( /* IN  */   const char*       pNext,
                     /* IN  */   void*             pContext,
                     /* OUT */   BOOL*             more_data_needed,
                     /* OUT */   roadmap_result*   rc)
{
   LPRTConnectionInfo   pCI = (LPRTConnectionInfo)pContext;
   int                  iBufferSize;
   RTUserLocation       UL;
   int						iSpeedTimes10;

   //   Initialize structure:
   RTUserLocation_Init( &UL);

   //   1.   Read user ID:
   pNext = ReadIntFromString( pNext,         //   [in]      Source string
                              ",",           //   [in,opt]   Value termination
                              NULL,          //   [in,opt]   Allowed padding
                              &UL.iID,       //   [out]      Put it here
                              DO_NOT_TRIM);  //   [in]      Remove additional termination CHARS

   if( !pNext || (',' != (*pNext)) || (RT_INVALID_LOGINID_VALUE == UL.iID))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read user ID");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   Jump over the first ',' that 'ReadIntFromString()' (above) did encounter:
   pNext++;

   //   2.   User Name - is optional:
   if( ',' == (*pNext))
   {
      roadmap_log( ROADMAP_DEBUG, "RTNet::OnGeneralResponse::AddUser() - User name was not supplied. Probably a guest-login...");
      pNext++;   //   Jump over the comma (,)
   }
   else
   {
      //   Read user name:
      iBufferSize = RT_USERNM_MAXSIZE;
      pNext       = ExtractNetworkString(
                  pNext,           // [in]     Source string
                  UL.sName,        // [out,opt]Output buffer
                  &iBufferSize,    // [in,out] Buffer size / Size of extracted string
                  ",",             // [in]     Array of chars to terminate the copy operation
                  TRIM_ALL_CHARS); // [in]     Remove additional termination chars

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read user name");
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
   }

   //   3.   Longitude
   pNext = ReadIntFromString(
               pNext,            //   [in]      Source string
               ",",              //   [in,opt]   Value termination
               NULL,             //   [in,opt]   Allowed padding
               &UL.position.longitude,   //   [out]      Put it here
               TRIM_ALL_CHARS);  //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read longitude");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   4.   Latitude
   pNext = ReadIntFromString(
               pNext,            //   [in]      Source string
               ",",              //   [in,opt]   Value termination
               NULL,             //   [in,opt]   Allowed padding
               &UL.position.latitude,    //   [out]      Put it here
               TRIM_ALL_CHARS);  //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read altitude");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   5.   Azimuth
   pNext = ReadIntFromString(
               pNext,            //   [in]      Source string
               ",",              //   [in,opt]   Value termination
               NULL,             //   [in,opt]   Allowed padding
               &UL.iAzimuth,     //   [out]      Put it here
               TRIM_ALL_CHARS);  //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read azimuth");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }


   //   6.   Speed
   pNext = ReadIntFromString(
                        pNext,            //   [in]      Source string
                        ",",              //   [in,opt]   Value termination
                        NULL,             //   [in,opt]   Allowed padding
                        &iSpeedTimes10,       //   [out]      Put it here
                        TRIM_ALL_CHARS);  //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read speed");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   UL.fSpeed = iSpeedTimes10 / 10.0;

   //   7.   Last access
   pNext = ReadIntFromString(
                  pNext,            //   [in]      Source string
                  ",",          //   [in,opt]   Value termination
                  NULL,             //   [in,opt]   Allowed padding
                  &UL.iLastAccessTime,
                                    //   [out]      Put it here
                  1);  //   [in]      Remove additional termination CHARS

   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read last access");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   //   8.   Mood
   pNext = ReadIntFromString(
               pNext,            //   [in]      Source string
               ",",              //   [in,opt]   Value termination
               NULL,             //   [in,opt]   Allowed padding
               &UL.iMood,     //   [out]      Put it here
               DO_NOT_TRIM);  //   [in]      Remove additional termination CHARS

   if(!pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read mood");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   
   //   Jump over the first ',' that 'ReadIntFromString()' (above) did encounter:
   pNext++;
   
   //   9.   Title
   if( ',' == (*pNext))
   {
      roadmap_log( ROADMAP_DEBUG, "RTNet::OnGeneralResponse::AddUser() - Title was not supplied.");
      pNext++;   //   Jump over the comma (,)
   }
   else
   {
      iBufferSize = RT_USERTTL_MAXSIZE;
      pNext       = ExtractNetworkString(
                                         pNext,           // [in]     Source string
                                         UL.sTitle,        // [out,opt]Output buffer
                                         &iBufferSize,    // [in,out] Buffer size / Size of extracted string
                                         ",",             // [in]     Array of chars to terminate the copy operation
                                         TRIM_ALL_CHARS); // [in]     Remove additional termination chars
      
      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read title");
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
   }
   //  10.   Addon
   pNext = ReadIntFromString(
                             pNext,            //   [in]      Source string
                             ",",              //   [in,opt]   Value termination
                             NULL,             //   [in,opt]   Allowed padding
                             &UL.iAddon,     //   [out]      Put it here
                             TRIM_ALL_CHARS);  //   [in]      Remove additional termination CHARS
   
   if(!pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read addon");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   //  11.   Stars
   pNext = ReadIntFromString(
                             pNext,            //   [in]      Source string
                             ",",              //   [in,opt]   Value termination
                             NULL,             //   [in,opt]   Allowed padding
                             &UL.iStars,     //   [out]      Put it here
                             TRIM_ALL_CHARS);  //   [in]      Remove additional termination CHARS
   
   if(!pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read stars");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   //  12.   Rank
   pNext = ReadIntFromString(
                             pNext,            //   [in]      Source string
                             ",",              //   [in,opt]   Value termination
                             NULL,             //   [in,opt]   Allowed padding
                             &UL.iRank,     //   [out]      Put it here
                             TRIM_ALL_CHARS);  //   [in]      Remove additional termination CHARS
   
   if(!pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read rank");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   //  13.   Points
   pNext = ReadIntFromString(
                             pNext,            //   [in]      Source string
                             ",",              //   [in,opt]   Value termination
                             NULL,             //   [in,opt]   Allowed padding
                             &UL.iPoints,     //   [out]      Put it here
                             TRIM_ALL_CHARS);  //   [in]      Remove additional termination CHARS
   
   if(!pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read points");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   //  14.   Join Date
   pNext = ReadIntFromString(
                             pNext,            //   [in]      Source string
                             ",\r\n",              //   [in,opt]   Value termination
                             NULL,             //   [in,opt]   Allowed padding
                             &UL.iJoinDate,     //   [out]      Put it here
                             TRIM_ALL_CHARS);  //   [in]      Remove additional termination CHARS
   
   if(!pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read join date");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   Create unique ID for the GUI object-system:
   RTUserLocation_CreateGUIID( &UL);

   //   If users exists: Update, Otherwize: Add:
   if( !RTUsers_UpdateOrAdd( &(pCI->Users), &UL))
   {
      roadmap_log(   ROADMAP_ERROR,
                  "RTNet::OnGeneralResponse::AddUser() - Failed to 'UpdateOrAdd' user (ID: %d); Maybe users-list is full? (Size: %d)",
                  UL.iID, RTUsers_Count( &(pCI->Users)));

      (*rc) = err_internal_error;
      return NULL;
   }

   return pNext;
}

//   <ID:int><MsgType:int>,<MsgShowType:int>,[<MsgTitle:String[64]>],<MsgString:String[512]>
const char* SystemMessage( /* IN  */   const char*       pNext,
                           /* IN  */   void*             pContext,
                           /* OUT */   BOOL*             more_data_needed,
                           /* OUT */   roadmap_result*   rc)
{
   RTSystemMessage      Msg;
   int                  iBufferSize;
   const char*          pLast;

   RTSystemMessage_Init( &Msg);

   //   1. ID:
   pNext = ReadIntFromString( pNext,            // [in]     Source string
                              ",",              // [in,opt] Value termination
                              NULL,             // [in,opt] Allowed padding
                              &Msg.iId,       // [out]    Put it here
                              TRIM_ALL_CHARS);  // [in]     Remove additional termination CHARS

   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::SystemMessage() - Failed to read message ID");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   
   //   2.  Message type:
   pNext = ReadIntFromString( pNext,            // [in]     Source string
                              ",",              // [in,opt] Value termination
                              NULL,             // [in,opt] Allowed padding
                              &Msg.iType,       // [out]    Put it here
                              TRIM_ALL_CHARS);  // [in]     Remove additional termination CHARS

   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::SystemMessage() - Failed to read message type");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   3.  Show type:
   pNext = ReadIntFromString( pNext,            // [in]     Source string
                              ",",              // [in,opt] Value termination
                              NULL,             // [in,opt] Allowed padding
                              &Msg.iShow,       // [out]    Put it here
                              TRIM_ALL_CHARS);  // [in]     Remove additional termination CHARS

   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::SystemMessage() - Failed to read message SHOW type");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   4.  Title:
   iBufferSize = RTNET_SYSTEMMESSAGE_TITLE_MAXSIZE;
   pLast       = pNext;
   pNext       = ExtractNetworkString(
               pNext,         // [in]     Source string
               NULL,          // [out,opt]Output buffer
               &iBufferSize,  // [in,out] Buffer size / Size of extracted string
               ",",           // [in]     Array of chars to terminate the copy operation
               DO_NOT_TRIM);  // [in]     Remove additional termination chars

   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::SystemMessage() - Failed to read message title");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   if( iBufferSize)
   {
      iBufferSize++; // Additional space for the terminating null...
      Msg.Title   = malloc(iBufferSize);
      pNext       = ExtractNetworkString(
                  pLast,         // [in]     Source string
                  Msg.Title,     // [out,opt]Output buffer
                  &iBufferSize,  // [in,out] Buffer size / Size of extracted string
                  ",",           // [in]     Array of chars to terminate the copy operation
                  DO_NOT_TRIM);  // [in]     Remove additional termination chars

      if( !pNext)
      {
         assert(0);
         RTSystemMessage_Free( &Msg);
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::SystemMessage() - Failed to read message title (case-II)");
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
   }

   if( ',' == (*pNext))
      pNext++;

   //   5.  Text:
   pLast       = pNext;
   iBufferSize = RTNET_SYSTEMMESSAGE_TEXT_MAXSIZE;
   pNext       = ExtractNetworkString(
               pNext,            // [in]     Source string
               NULL,             // [out,opt]Output buffer
               &iBufferSize,     // [in,out] Buffer size / Size of extracted string
               ",\r\n",          // [in]     Array of chars to terminate the copy operation
               TRIM_ALL_CHARS);  // [in]     Remove additional termination chars

   if( !pNext)
   {
      RTSystemMessage_Free( &Msg);
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::SystemMessage() - Failed to read message title");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   if( !iBufferSize)
   {
      RTSystemMessage_Free( &Msg);
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::SystemMessage() - Failed to read message title - Messsage-text was not supplied");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   iBufferSize++; // Additional space for the terminating null...
   Msg.Text    = malloc(iBufferSize);
   pNext       = ExtractNetworkString(
               pLast,            // [in]     Source string
               Msg.Text,         // [out,opt]Output buffer
               &iBufferSize,     // [in,out] Buffer size / Size of extracted string
               ",\r\n",          // [in]     Array of chars to terminate the copy operation
               TRIM_ALL_CHARS);  // [in]     Remove additional termination chars

   // Push item to queue
   RTSystemMessageQueue_Push( &Msg);
   // Detach from object:
   RTSystemMessage_Init( &Msg);

   return pNext;
}

//   <Upgrade-Type:int>,<Latest-Version:String[32]>,<URL:String[256]>
const char* VersionUpgrade(/* IN  */   const char*       pNext,
                           /* IN  */   void*             pContext,
                           /* OUT */   BOOL*             more_data_needed,
                           /* OUT */   roadmap_result*   rc)
{
   extern
   VersionUpgradeInfo   gs_VU;
   int                  iBufferSize;
   int                  i;

   gs_VU.eSeverity = VUS_NA;

   // 1. Severity:
   pNext = ReadIntFromString( pNext,            // [in]     Source string
                              ",",              // [in,opt] Value termination
                              NULL,             // [in,opt] Allowed padding
                              &i,               // [out]    Put it here
                              TRIM_ALL_CHARS);  // [in]     Remove additional termination CHARS
   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::VersionUpgrade() - Failed to read severity");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   if( (i<1) || (3<i))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::VersionUpgrade() - Invalid value for VU severity (%d)", i);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   gs_VU.eSeverity = (EVersionUpgradeSeverity)i;

   // 2. New version value:
   iBufferSize = VERSION_STRING_MAXSIZE;
   pNext       = ExtractNetworkString(
               pNext,           // [in]     Source string
               gs_VU.NewVersion,// [out,opt]Output buffer
               &iBufferSize,    // [in,out] Buffer size / Size of extracted string
               ",",             // [in]     Array of chars to terminate the copy operation
               TRIM_ALL_CHARS); // [in]     Remove additional termination chars

   if( !pNext || !(*pNext))
   {
      VersionUpgradeInfo_Init( &gs_VU);
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::VersionUpgrade() - Failed to read version value");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   // 3. URL:
   iBufferSize = GENERAL_URL_MAXSIZE;
   pNext       = ExtractNetworkString(
               pNext,            // [in]     Source string
               gs_VU.URL,        // [out,opt]Output buffer
               &iBufferSize,     // [in,out] Buffer size / Size of extracted string
               ",\r\n",          // [in]     Array of chars to terminate the copy operation
               TRIM_ALL_CHARS);  // [in]     Remove additional termination chars

   if( !pNext)
   {
      VersionUpgradeInfo_Init( &gs_VU);
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::VersionUpgrade() - Failed to read version URL");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   roadmap_log( ROADMAP_INFO, "RTNet::OnGeneralResponse::VersionUpgrade() - !!! HAVE A NEW VERSION !!! (Severity: %d)", gs_VU.eSeverity);

   return pNext;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//   <AID:int>,<latitude:double>,<longitude:double>,<node1:???><node2:???><direction:int(0..2)><azymuth:int(0..359)><reportTime:long><reportBy:string><speed:int><type:int><description:string>
const char* AddAlert(   /* IN  */   const char*       pNext,
                        /* IN  */   void*             pContext,
                        /* OUT */   BOOL*             more_data_needed,
                        /* OUT */   roadmap_result*   rc)
{
   RTAlert  alert;
   int      iBufferSize;
   char     reportedBy[5];

   //   Initialize structure:
   RTAlerts_Alert_Init(&alert);

   //   1.   Read Alert ID:
   pNext = ReadIntFromString(
                  pNext,         //   [in]      Source string
                  ",",           //   [in,opt]   Value termination
                  NULL,          //   [in,opt]   Allowed padding
                  &alert.iID,    //   [out]      Put it here
                  DO_NOT_TRIM);  //   [in]      Remove additional termination CHARS

   if( !pNext || (',' != (*pNext)) || (RT_INVALID_LOGINID_VALUE == alert.iID))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read  ID");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   Jump over the first ',' that 'ReadIntFromString()' (above) did encounter:
   pNext++;

   //   2.   Type
   pNext = ReadIntFromString(
                  pNext,            //   [in]      Source string
                  ",",              //   [in,opt]   Value termination
                  NULL,             //   [in,opt]   Allowed padding
                  &alert.iType,     //   [out]      Put it here
                  1);               //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read type id=%d", alert.iID);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   3.   Longitude
   pNext = ReadIntFromString(
                  pNext,            //   [in]      Source string
                  ",",              //   [in,opt]   Value termination
                  NULL,             //   [in,opt]   Allowed padding
                  &alert.iLongitude,      //   [out]      Put it here
                  1);               //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read longitude id=%d", alert.iID);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   4.   Latitude
   pNext = ReadIntFromString(
               pNext,            //   [in]      Source string
               ",",              //   [in,opt]   Value termination
               NULL,             //   [in,opt]   Allowed padding
               &alert.iLatitude,        //   [out]      Put it here
               1);               //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read altitude id=%d", alert.iID);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   5.   Node1
   pNext = ReadIntFromString(
               pNext,            //   [in]      Source string
               ",",              //   [in,opt]   Value termination
               "-",              //   [in,opt]   Allowed padding
               &alert.iNode1,    //   [out]      Put it here
               1);               //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read node1 id=%d", alert.iID);
   }

   //   6.   Node2
   pNext = ReadIntFromString(
                  pNext,            //   [in]      Source string
                  ",",              //   [in,opt]   Value termination
                  "-",              //   [in,opt]   Allowed padding
                  &alert.iNode2,    //   [out]      Put it here
                  1);               //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
      // roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read node2");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   7.   Direction
   pNext = ReadIntFromString(
            pNext,            //   [in]      Source string
            ",",              //   [in,opt]   Value termination
            NULL,             //   [in,opt]   Allowed padding
            &alert.iDirection,//   [out]      Put it here
            1);               //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read direction id=%d", alert.iID);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   8.   Azymuth
   pNext = ReadIntFromString(
            pNext,            //   [in]      Source string
            ",",              //   [in,opt]   Value termination
            NULL,             //   [in,opt]   Allowed padding
            &alert.iAzymuth,  //   [out]      Put it here
            1);               //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read azymuth id=%d", alert.iID);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }


   //   9.   Description  - is optional:
   if(',' == (*pNext))
   {
      roadmap_log( ROADMAP_DEBUG, "RTNet::OnGeneralResponse::AddAlert() - Description by was not supplied. id=%d", alert.iID);
      pNext++;   //   Jump over the comma (,)
   }
   else
   {
      //   Description
      iBufferSize = RT_ALERT_DESCRIPTION_MAXSIZE;
      pNext       = ExtractNetworkString(
                  pNext,               // [in]     Source string
                  alert.sDescription,  // [out,opt]Output buffer
                  &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                  ",",                 // [in]     Array of chars to terminate the copy operation
                  1);                  // [in]     Remove additional termination chars

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read description id=%d", alert.iID);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
   }

   //   10.   Report Time
   pNext = ReadIntFromString(
               pNext,               //   [in]      Source string
               ",",                 //   [in,opt]  Value termination
               NULL,                //   [in,opt]  Allowed padding
               &alert.iReportTime,//   [out]     Put it here
               1);                  //   [in]      Remove additional termination CHARS

   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read report time id=%d", alert.iID);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   Read user name:
   iBufferSize = RT_ALERT_USERNM_MAXSIZE;
   pNext = ExtractNetworkString(
                  pNext,            //   [in]   Source string
                  alert.sReportedBy, // [out,opt]Output buffer
                  &iBufferSize,   //   [in]   Output buffer size
                  ",",               // [in]     Array of chars to terminate the copy operation
                  1);   //   [in]   Remove additional termination chars

    if( !pNext)
    {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read PostedByMe flag id=%d", alert.iID);
         alert.bAlertByMe = FALSE;
    }

   //   Read If the alert was reported by me
   iBufferSize = sizeof(reportedBy);
   pNext       = ExtractNetworkString(
                  pNext,             // [in]     Source string
                  reportedBy,//   [out]   Output buffer
                  &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                  ",",          //   [in]   Array of chars to terminate the copy operation
                  1);   // [in]     Remove additional termination chars

    if( !pNext)
    {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read user named id=%d", alert.iID);
         (*rc) = err_parser_unexpected_data;
         return NULL;
    }


   if (reportedBy[0] == 'T')
     alert.bAlertByMe = TRUE;
   else
     alert.bAlertByMe = FALSE;

  	//   Read near Str
   iBufferSize = RT_ALERT_LOCATION_MAX_SIZE;
   pNext = ExtractNetworkString(
                  pNext,            //   [in]   Source string
                  alert.sNearStr, // [out,opt]Output buffer
                  &iBufferSize,   //   [in]   Output buffer size
                  ",",               // [in]     Array of chars to terminate the copy operation
                  1);   //   [in]   Remove additional termination chars

   alert.iSpeed = 0 ;
  	//   Read Mood
   pNext = ReadIntFromString(
            pNext,            //   [in]      Source string
            ",",              //   [in,opt]   Value termination
            NULL,             //   [in,opt]   Allowed padding
            &alert.iMood,  //   [out]      Put it here
            1);               //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read moodd id=%d", alert.iID);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

	//   Read Rank
   pNext = ReadIntFromString(
            pNext,            //   [in]      Source string
            ",",              //   [in,opt]   Value termination
            NULL,             //   [in,opt]   Allowed padding
            &alert.iRank,  //   [out]      Put it here
            1);               //   [in]      Remove additional termination CHARS

   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read rank id=%d", alert.iID);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   Read image ID:
   iBufferSize = RT_ALERT_IMAGEID_MAXSIZE;
   pNext = ExtractNetworkString(
                  pNext,            //   [in]   Source string
                  alert.sImageIdStr, // [out,opt]Output buffer
                  &iBufferSize,   //   [in]   Output buffer size
                  ",",               // [in]     Array of chars to terminate the copy operation
                  1);   //   [in]   Remove additional termination chars

   //   Read street:
   iBufferSize = RT_ALERT_LOCATION_MAX_SIZE;
   pNext = ExtractNetworkString(
                  pNext,            //   [in]   Source string
                  alert.sStreetStr, // [out,opt]Output buffer
                  &iBufferSize,   //   [in]   Output buffer size
                  ",",               // [in]     Array of chars to terminate the copy operation
                  1);   //   [in]   Remove additional termination chars

   //   Read city:
   iBufferSize = RT_ALERT_LOCATION_MAX_SIZE;
   pNext = ExtractNetworkString(
                  pNext,            //   [in]   Source string
                  alert.sCityStr, // [out,opt]Output buffer
                  &iBufferSize,   //   [in]   Output buffer size
                  ",\r\n",               // [in]     Array of chars to terminate the copy operation
                  TRIM_ALL_CHARS);   //   [in]   Remove additional termination chars

   //   Add the Alert
   if( !RTAlerts_Add(&alert))
   {
      roadmap_log(ROADMAP_ERROR,
                  "RTNet::OnGeneralResponse::AddAlert() - Failed to 'Add' alert (ID: %d);  (Alert List Size: %d)",
                  alert.iID, RTAlerts_Count());
      (*rc) = err_internal_error;
      return NULL;
   }

   return pNext;
}


const char* BridgeToRes( /* IN  */   const char*       pNext,
                        	/* IN  */   void*             pContext,
                        	/* OUT */   BOOL*             more_data_needed,
                        	/* OUT */   roadmap_result*   rc){

   int  status;
   char serviceName[100];
   int iBufferSize;
   int numParameters;

   iBufferSize = sizeof(serviceName);
   pNext       = ExtractNetworkString(
                   pNext,             // [in]     Source string
                   serviceName,//   [out]   Output buffer
                   &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                   ",",          //   [in]   Array of chars to terminate the copy operation
                   1);   // [in]     Remove additional termination chars

   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::BridgeToRes() - Failed to read  Service Name");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   1.   BridgeToStatus
   pNext = ReadIntFromString(
                  pNext,         //   [in]      Source string
                  ",\r\n",           //   [in,opt]   Value termination
                  NULL,          //   [in,opt]   Allowed padding
                  &status,    //   [out]      Put it here
                  TRIM_ALL_CHARS);  //   [in]      Remove additional termination CHARS

   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::BridgeToRes() - Failed to read  status");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   1.   Number of parameters
   pNext = ReadIntFromString(
                  pNext,         //   [in]      Source string
                  ",\r\n",           //   [in,opt]   Value termination
                  NULL,          //   [in,opt]   Allowed padding
                  &numParameters,//   [out]      Put it here
                  1);            //   [in]      Remove additional termination CHARS
   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::BridgeToRes() - Failed to read  number of parameters");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   if (!strcmp(serviceName,"TWITTER")){
      if (status != 200){
         roadmap_twitter_login_failed(status);
      }
   }
   else if (!strcmp(serviceName,"FOURSQUARE")){
      return roadmap_foursquare_response(status, rc, numParameters, pNext);
   }
   else if (!strcmp(serviceName,"CREATEACCOUNT"))
   {
	  ParseCreateAccountErrors( status, rc );
   }
   else if (!strcmp(serviceName,"UPDATEPROFILE")){
	   ParseUpdateAccountErrors( status, rc );
   }

   else if (!strcmp(serviceName,"UPDATEMAP")){
      if (status != 200){
       	 roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::BridgeToRes() - UPDATEMAP - got error from server about UPDATEMAP request");
      }
      else{
         roadmap_log( ROADMAP_DEBUG, "RTNet::OnGeneralResponse::BridgeToRes() - UPDATEMAP - got good response from server regarding UPDATEMAP request");
      }
   }
   else if (!strcmp(serviceName,"TRIPSERVER")){

      return roadmap_tripserver_response(status, numParameters, pNext);

   }
   return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//   <AID:int>
const char* AddAlertComment(  /* IN  */   const char*       pNext,
                              /* IN  */   void*             pContext,
                              /* OUT */   BOOL*             more_data_needed,
                              /* OUT */   roadmap_result*   rc)
{
   RTAlertComment       comment;
   int                  iBufferSize;
   char                 reportedBy[5];

   //   Initialize structure:
   RTAlerts_Comment_Init(&comment);

   //   1.   Read Alert ID:
   pNext = ReadIntFromString(
                  pNext,         //   [in]      Source string
                  ",",           //   [in,opt]   Value termination
                  NULL,          //   [in,opt]   Allowed padding
                  &comment.iAlertId,  //   [out]      Put it here
                  DO_NOT_TRIM);  //   [in]      Remove additional termination CHARS

   if( !pNext || (',' != (*pNext)) || (RT_INVALID_LOGINID_VALUE == comment.iAlertId))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlertComment() - Failed to read alert ID");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   Jump over the first ',' that 'ReadIntFromString()' (above) did encounter:
   pNext++;

   //   2.   Comment Id
   pNext = ReadIntFromString(
                  pNext,            //   [in]      Source string
                  ",",              //   [in,opt]   Value termination
                  NULL,             //   [in,opt]   Allowed padding
                  &comment.iID,     //   [out]      Put it here
                  1);               //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlertComment() - Failed to read Comment Id");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //  3. Description
   iBufferSize = RT_ALERT_DESCRIPTION_MAXSIZE;
   pNext = ExtractNetworkString(
                  pNext,            //   [in]   Source string
                  comment.sDescription,//   [out]   Output buffer
                  &iBufferSize,
                                    //   [in]   Output buffer size
                  ",",          //   [in]   Array of chars to terminate the copy operation
                  TRIM_ALL_CHARS);   //   [in]   Remove additional termination chars
   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlertComment() - Failed to read description");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   4.   Post Time
   pNext = ReadInt64FromString(
                pNext,               //   [in]      Source string
                ",",                 //   [in,opt]   Value termination
                NULL,                //   [in,opt]   Allowed padding
                &comment.i64ReportTime,//   [out]      Put it here
                1);                  //   [in]      Remove additional termination CHARS

   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlertComment() - Failed to read report time");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //  5. Read who posted the comment
   iBufferSize = RT_ALERT_USERNM_MAXSIZE;
   pNext = ExtractNetworkString(
                 pNext,               //   [in]   Source string
                  comment.sPostedBy,  //   [out]   Output buffer
                  &iBufferSize,
                                       //   [in]   Output buffer size
                  ",",                 //   [in]   Array of chars to terminate the copy operation
                  1);                  //   [in]   Remove additional termination chars

   if( !pNext)
   {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlertComment() - Failed to read user name");
        (*rc) = err_parser_unexpected_data;
        return NULL;
   }

    //   Read If the comment was posted by me
    iBufferSize = sizeof(reportedBy);
    pNext = ExtractNetworkString(
               pNext,            //   [in]   Source string
               reportedBy,    //   [out]   Output buffer
               &iBufferSize,  //   [in]   Output buffer size
               ",",          //   [in]   Array of chars to terminate the copy operation
               1);   //   [in]   Remove additional termination chars

      if( !pNext)
     {
          roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlertComment() - Failed to read PostedByMe flag");
          comment.bCommentByMe = FALSE;
      }

     if (reportedBy[0] == 'T')
       comment.bCommentByMe= TRUE;
    else
       comment.bCommentByMe = FALSE;
    //   Read Mood
    pNext = ReadIntFromString(
            pNext,            //   [in]      Source string
            ",",              //   [in,opt]   Value termination
            NULL,             //   [in,opt]   Allowed padding
            &comment.iMood,  //   [out]      Put it here
            1);               //   [in]      Remove additional termination CHARS

    if( !pNext || !(*pNext))
    {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlertComment() - Failed to read mood");
      (*rc) = err_parser_unexpected_data;
      return NULL;
    }

	//   Read Rank
    pNext = ReadIntFromString(
            pNext,            //   [in]      Source string
            ",\r\n",              //   [in,opt]   Value termination
            NULL,             //   [in,opt]   Allowed padding
            &comment.iRank,  //   [out]      Put it here
            TRIM_ALL_CHARS);               //   [in]      Remove additional termination CHARS

    if( !pNext)
    {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlertComment() - Failed to read rank");
      (*rc) = err_parser_unexpected_data;
      return NULL;
    }

   //   Add the Comment
   if( !RTAlerts_Comment_Add(&comment))
   {
      roadmap_log(ROADMAP_ERROR,
               "RTNet::OnGeneralResponse::AddAlertComment() - Failed to add comment (ID: %d)",
               comment.iID);
      (*rc) = err_internal_error;
      return NULL;
   }

   return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//   <AID:int>
const char* RemoveAlert(/* IN  */   const char*       pNext,
                        /* IN  */   void*             pContext,
                        /* OUT */   BOOL*             more_data_needed,
                        /* OUT */   roadmap_result*   rc)
{
   int   iID;

   //   1.   Read Alert ID:
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",\r\n",       // [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &iID,          // [out]    Put it here
                              DO_NOT_TRIM);  // [in]     Remove additional termination CHARS

   if( !pNext || (-1 == iID))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::RemoveAlert() - Failed to read  ID");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   Remove the alert
   if( !RTAlerts_Remove(iID))
   {
      roadmap_log(ROADMAP_DEBUG,
                  "RTNet::OnGeneralResponse::AddAlert() - Failed to 'Remove' alert (ID: %d);",iID);
   }

   return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//   <ID><Speed><type><street><city><start><end><num_segs> <nodeid><lat><long>
const char* AddRoadInfo(/* IN  */   const char*       pNext,
                        /* IN  */   void*             pContext,
                        /* OUT */   BOOL*             more_data_needed,
                        /* OUT */   roadmap_result*   rc)
{
   RTTrafficInfo  trafficInfo;
   int            iBufferSize;
   int            i;
   int				iNodeId;
   int				iLongitude;
   int				iLatitude;
   int            iNumElements;
   int				iSpeedTimes10;

   //   Initialize structure:
   RTTrafficInfo_InitRecord(&trafficInfo);

   //   1.   RoadInfo ID:
   pNext = ReadIntFromString(
                  pNext,         //   [in]      Source string
                  ",",           //   [in,opt]   Value termination
                  NULL,          //   [in,opt]   Allowed padding
                  &trafficInfo.iID,    //   [out]      Put it here
                  1);  //   [in]      Remove additional termination CHARS

   if( !pNext || ! (*pNext) || (-1 == trafficInfo.iID))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read  ID");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   2.   Speed

   pNext = ReadIntFromString(
                  pNext,            //   [in]      Source string
                  ",",              //   [in,opt]   Value termination
                  NULL,             //   [in,opt]   Allowed padding
                  &iSpeedTimes10,     //   [out]      Put it here
                  1);               //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read speed");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   trafficInfo.fSpeed = iSpeedTimes10 / 10.0;

   //   3.   Type
   pNext = ReadIntFromString(
                  pNext,            //   [in]      Source string
                  ",",              //   [in,opt]   Value termination
                  NULL,             //   [in,opt]   Allowed padding
                  &trafficInfo.iType,     //   [out]      Put it here
                  1);               //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read type");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   4.   User contribution
   pNext = ReadIntFromString(
                  pNext,            //   [in]      Source string
                  ",",              //   [in,opt]   Value termination
                  NULL,             //   [in,opt]   Allowed padding
                  &trafficInfo.iUserContribution,     //   [out]      Put it here
                  1);               //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read user contribution");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
	//roadmap_log (ROADMAP_ERROR, "Jam id %d user contrib %d", trafficInfo.iID, trafficInfo.iUserContribution);

   //   5.   Street  - is optional
   if(',' == (*pNext))
   {
      roadmap_log( ROADMAP_DEBUG, "RTNet::OnGeneralResponse::AddRoadInfo() - Street is empty. ");
      pNext++;   //   Jump over the comma (,)
   }
   else
   {
      iBufferSize = RT_TRAFFIC_INFO_ADDRESS_MAXSIZE;
      pNext       = ExtractNetworkString(
                  pNext,               // [in]     Source string
                  trafficInfo.sStreet,  // [out,opt]Output buffer
                  &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                  ",",                 // [in]     Array of chars to terminate the copy operation
                  1);                  // [in]     Remove additional termination chars

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read Street");
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
   }

   //   6.   City  - is optional
   if(',' == (*pNext))
   {
      roadmap_log( ROADMAP_DEBUG, "RTNet::OnGeneralResponse::AddRoadInfo() - City is empty. ");
      pNext++;   //   Jump over the comma (,)
   }
   else
   {
      iBufferSize = RT_TRAFFIC_INFO_ADDRESS_MAXSIZE;
      pNext       = ExtractNetworkString(
                  pNext,               // [in]     Source string
                  trafficInfo.sCity,  // [out,opt]Output buffer
                  &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                  ",",                 // [in]     Array of chars to terminate the copy operation
                  1);                  // [in]     Remove additional termination chars

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read City");
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
   }

   //   7.   Start  - is optional
   if(',' == (*pNext))
   {
      roadmap_log( ROADMAP_DEBUG, "RTNet::OnGeneralResponse::AddRoadInfo() - Start is emprty. ");
      pNext++;   //   Jump over the comma (,)
   }
   else
   {
      iBufferSize = RT_TRAFFIC_INFO_ADDRESS_MAXSIZE;
      pNext       = ExtractNetworkString(
                  pNext,               // [in]     Source string
                  trafficInfo.sStart,  // [out,opt]Output buffer
                  &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                  ",",                 // [in]     Array of chars to terminate the copy operation
                  1);                  // [in]     Remove additional termination chars

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read Start");
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
   }

   //   8.   End  - is optional
   if(',' == (*pNext))
   {
      roadmap_log( ROADMAP_DEBUG, "RTNet::OnGeneralResponse::AddRoadInfo() - End is empty. ");
      pNext++;   //   Jump over the comma (,)
   }
   else
   {
      iBufferSize = RT_TRAFFIC_INFO_ADDRESS_MAXSIZE;
      pNext       = ExtractNetworkString(
                  pNext,               // [in]     Source string
                  trafficInfo.sEnd,  // [out,opt]Output buffer
                  &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                  ",",                 // [in]     Array of chars to terminate the copy operation
                  1);                  // [in]     Remove additional termination chars

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read End");
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
   }


   //   9.   NumSegments
   pNext = ReadIntFromString(
                  pNext,            //   [in]      Source string
                  ",",              //   [in,opt]   Value termination
                  NULL,             //   [in,opt]   Allowed padding
                  &iNumElements,     //   [out]      Put it here
                  1);               //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read number of segments");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   trafficInfo.iNumNodes = 0;

   for (i=0; i<iNumElements/3;i++){

      //   NodeId
      pNext = ReadIntFromString(
         pNext,            //   [in]      Source string
         ",",              //   [in,opt]   Value termination
         NULL,             //   [in,opt]   Allowed padding
         &iNodeId,        //   [out]      Put it here
         1);               //   [in]      Remove additional termination CHARS

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read NodeId for segment %d, ID=%d", i,trafficInfo.iID );
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }

      //   Longitude of the segment
      pNext = ReadIntFromString(
         pNext,            //   [in]      Source string
         ",",              //   [in,opt]   Value termination
         NULL,             //   [in,opt]   Allowed padding
         &iLongitude,        //   [out]      Put it here
         1);               //   [in]      Remove additional termination CHARS

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read longitude for segment %d, ID=%d", i,trafficInfo.iID );
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }

      //   Latitue of the segment
      pNext = ReadIntFromString(
         pNext,            //   [in]      Source string
         ",\r\n",              //   [in,opt]   Value termination
         NULL,             //   [in,opt]   Allowed padding
         &iLatitude,        //   [out]      Put it here
         TRIM_ALL_CHARS);               //   [in]      Remove additional termination CHARS

      if( !pNext)
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read latitude for segment %d, ID=%d", i,trafficInfo.iID );
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }

		if (i < RT_TRAFFIC_INFO_MAX_NODES)
		{
			trafficInfo.sNodes[i].iNodeId					= iNodeId;
      	trafficInfo.sNodes[i].Position.latitude 	= iLatitude;
      	trafficInfo.sNodes[i].Position.longitude 	= iLongitude;
      	trafficInfo.iNumNodes++;
		}

   }

   //   Add the RoadInfo

   if( !RTTrafficInfo_Add(&trafficInfo))
   {
      roadmap_log(ROADMAP_ERROR,
                  "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to 'Add' road_info (ID: %d);  (List Size: %d)",
                  trafficInfo.iID, RTTrafficInfo_Count());

      (*rc) = err_internal_error;
      return NULL;
   }


   return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//   <AID:int>
const char* RmRoadInfo( /* IN  */   const char*       pNext,
                        /* IN  */   void*             pContext,
                        /* OUT */   BOOL*             more_data_needed,
                        /* OUT */   roadmap_result*   rc)
{
   int   iID;

   //   1.   Read RoadInfo ID:
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",\r\n",       // [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &iID,          // [out]    Put it here
                              DO_NOT_TRIM);  // [in]     Remove additional termination CHARS

   if( !pNext || (-1 == iID))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::RemoveRoadInfo() - Failed to read  ID");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }


   //   Remove the RoadInfo
   if( !RTTrafficInfo_Remove(iID) )
   {
      roadmap_log(ROADMAP_DEBUG,
                  "RTNet::OnGeneralResponse::RemoveRoadInfo() - Failed to 'Remove' RoadInfo (ID: %d);",iID);
   }

   return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//   <timestamp:int>
const char* MapUpdateTime( /* IN  */   const char*       pNext,
                        	/* IN  */   void*             pContext,
                        	/* OUT */   BOOL*             more_data_needed,
                        	/* OUT */   roadmap_result*   rc)
{
   int   iTimestamp;

   //   1.   Read timestamp:
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",\r\n",       // [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &iTimestamp,   // [out]    Put it here
                              DO_NOT_TRIM);  // [in]     Remove additional termination CHARS

   if( !pNext )
   {
      roadmap_log( ROADMAP_ERROR, "MapUpdateTime() - Failed to read  ID");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }


   //   Try to clean up the editor
   editor_cleanup_test (iTimestamp - 24 * 60 * 60);

   return pNext;
}

const char* GeoLocation( /* IN  */   const char*       pNext,
                           /* IN  */   void*             pContext,
                           /* OUT */   BOOL*             more_data_needed,
                           /* OUT */   roadmap_result*   rc){
   int   iScore;
   int   iBufferSize ;
   char str[128];

   //   Metro name
   iBufferSize = 128;
   pNext       = ExtractNetworkString(
                pNext,               // [in]     Source string
                &str[0],  // [out,opt]Output buffer
                &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                ",",                 // [in]     Array of chars to terminate the copy operation
                1);                  // [in]     Remove additional termination chars

    if( !pNext || !(*pNext))
    {
       roadmap_log( ROADMAP_ERROR, "RTNet::GeoLocation - Failed to read Metro name");
       (*rc) = err_parser_unexpected_data;
       return NULL;
    }
    roadmap_geo_location_set_metropolitan(str);

    //   State
    iBufferSize = 128;
    pNext       = ExtractNetworkString(
                 pNext,               // [in]     Source string
                 &str[0],  // [out,opt]Output buffer
                 &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                 ",",                 // [in]     Array of chars to terminate the copy operation
                 1);                  // [in]     Remove additional termination chars

     if( !pNext || !(*pNext))
     {
        roadmap_log( ROADMAP_ERROR, "RTNet::GeoLocation - Failed to read state");
        (*rc) = err_parser_unexpected_data;
        return NULL;
     }
     roadmap_geo_location_set_state(str);

     //   Map score
     pNext = ReadIntFromString(
         pNext,            //   [in]      Source string
         ",",              //   [in,opt]   Value termination
         NULL,             //   [in,opt]   Allowed padding
         &iScore,        //   [out]      Put it here
         1);               //   [in]      Remove additional termination CHARS
     if( !pNext || !(*pNext))
     {
        roadmap_log( ROADMAP_ERROR, "RTNet::GeoLocation - Failed to map score");
        (*rc) = err_parser_unexpected_data;
        return NULL;
     }
     roadmap_geo_location_set_map_score(iScore);

     //   Traffic score
     pNext = ReadIntFromString(
         pNext,            //   [in]      Source string
         ",",              //   [in,opt]   Value termination
         NULL,             //   [in,opt]   Allowed padding
         &iScore,        //   [out]      Put it here
         1);               //   [in]      Remove additional termination CHARS
     if( !pNext || !(*pNext))
     {
        roadmap_log( ROADMAP_ERROR, "RTNet::GeoLocation - Failed to traffic score");
        (*rc) = err_parser_unexpected_data;
        return NULL;
     }
     roadmap_geo_location_set_traffic_score(iScore);

     //   Usage score
     str[0] = 0;
     pNext = ReadIntFromString(
         pNext,            //   [in]      Source string
         ",",              //   [in,opt]   Value termination
         NULL,             //   [in,opt]   Allowed padding
         &iScore,        //   [out]      Put it here
         1);               //   [in]      Remove additional termination CHARS
     if( !pNext || !(*pNext))
     {
        roadmap_log( ROADMAP_ERROR, "RTNet::GeoLocation - Failed to usage score");
        (*rc) = err_parser_unexpected_data;
        return NULL;
     }
     roadmap_geo_location_set_usage_score(iScore);

     //   Overall score
    pNext = ReadIntFromString(
        pNext,            //   [in]      Source string
        ",",              //   [in,opt]   Value termination
        NULL,             //   [in,opt]   Allowed padding
        &iScore,        //   [out]      Put it here
        1);               //   [in]      Remove additional termination CHARS
    if( !pNext)
    {
       roadmap_log( ROADMAP_ERROR, "RTNet::GeoLocation - Failed to overall score");
       (*rc) = err_parser_unexpected_data;
       return NULL;
    }
    roadmap_geo_location_set_overall_score(iScore);

    //   Map score Str
     str[0] = 0;
     iBufferSize = 128;
     pNext       = ExtractNetworkString(
                  pNext,               // [in]     Source string
                  &str[0],  // [out,opt]Output buffer
                  &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                  ",",                 // [in]     Array of chars to terminate the copy operation
                  1);                  // [in]     Remove additional termination chars

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::GeoLocation - Failed to read Map score Str");
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
      roadmap_geo_location_set_map_score_str(&str[0]);

      //   Traffic score Str
      str[0] = 0;
      iBufferSize = 128;
      pNext       = ExtractNetworkString(
                   pNext,               // [in]     Source string
                   &str[0],  // [out,opt]Output buffer
                   &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                   ",",                 // [in]     Array of chars to terminate the copy operation
                   1);                  // [in]     Remove additional termination chars

       if( !pNext || !(*pNext))
       {
          roadmap_log( ROADMAP_ERROR, "RTNet::GeoLocation - Failed to read Traffic score Str");
          (*rc) = err_parser_unexpected_data;
          return NULL;
       }
      roadmap_geo_location_set_traffic_score_str(&str[0]);

      //   Usage score Str
      str[0] = 0;
      iBufferSize = 128;
      pNext       = ExtractNetworkString(
                   pNext,               // [in]     Source string
                   &str[0],  // [out,opt]Output buffer
                   &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                   ",\r\n",                 // [in]     Array of chars to terminate the copy operation
                   TRIM_ALL_CHARS);                  // [in]     Remove additional termination chars

      if( !pNext)
       {
          roadmap_log( ROADMAP_ERROR, "RTNet::GeoLocation - Failed to read Usage score Str");
          (*rc) = err_parser_unexpected_data;
          return NULL;
       }
      roadmap_geo_location_set_usage_score_str(&str[0]);


     roadmap_geo_location_info();

    return pNext;

}

//////////////////////////////////////////////////////////////////////////////////////////////////
//   <delta:int>
const char* UpdateUserPoints( /* IN  */   const char*       pNext,
                              /* IN  */   void*             pContext,
                              /* OUT */   BOOL*             more_data_needed,
                              /* OUT */   roadmap_result*   rc)
{
   int   iUserPointsDelta;

   //   1.   Read user points:
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",\r\n",       // [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &iUserPointsDelta,  // [out]    Put it here
                              DO_NOT_TRIM);  // [in]     Remove additional termination CHARS

   if( !pNext )
   {
      roadmap_log( ROADMAP_ERROR, "UpdateUserPoints() - Failed to read points");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }


   //   Update user points
   roadmap_log( ROADMAP_WARNING, "UpdateUserPoints() - updating user points delta: %d", iUserPointsDelta);
   editor_points_add_new_points(iUserPointsDelta);

   return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char* ReportAlertRes( /* IN  */   const char*       pNext,
                            /* IN  */   void*             pContext,
                            /* OUT */   BOOL*             more_data_needed,
                            /* OUT */   roadmap_result*   rc){
   int   iPoints;

   //   1.   Read points:
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",\r\n",       // [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &iPoints,          // [out]    Put it here
                              DO_NOT_TRIM);  // [in]     Remove additional termination CHARS

   if( !pNext || (-1 == iPoints))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::ReportAlertRes() - Failed to read  Points");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
	
   editor_points_add_new_points(iPoints);
   editor_points_display_new_points_timed(iPoints, 5,report_event);
   return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char* PostAlertCommentRes( /* IN  */   const char*       pNext,
                            /* IN  */   void*             pContext,
                            /* OUT */   BOOL*             more_data_needed,
                            /* OUT */   roadmap_result*   rc){
   int   iPoints;

   //   1.   Read Alert ID:
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",\r\n",       // [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &iPoints,          // [out]    Put it here
                              DO_NOT_TRIM);  // [in]     Remove additional termination CHARS

   if( !pNext || (-1 == iPoints))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::PostAlertCommentRes() - Failed to read  Points");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   editor_points_add_new_points(iPoints);
   editor_points_display_new_points_timed(iPoints, 5, comment_event);
   return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char* onGeoConfigResponse(  /* IN  */   const char*       pNext,
                              /* IN  */   void*             pContext,
                              /* OUT */   BOOL*             more_data_needed,
                              /* OUT */   roadmap_result*   rc)
{
   return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char *AddBonus (/* IN  */   const char*       pNext,
               /* IN  */   void*             pContext,
               /* OUT */   BOOL*             more_data_needed,
               /* OUT */   roadmap_result*   rc)
{
   RTBonus  bonus;
   char     str[128];
   int      iBufferSize;
   
   RealtimeBonus_Record_Init(&bonus);
   //   1.   Read ID:
   pNext = ReadIntFromString(
                   pNext,         //   [in]      Source string
                   ",",           //   [in,opt]   Value termination
                   NULL,          //   [in,opt]   Allowed padding
                   &bonus.iID,    //   [out]      Put it here
                   1);  //   [in]      Remove additional termination CHARS

   if( !pNext || (-1 == bonus.iID))
   {
       roadmap_log( ROADMAP_ERROR, "RTNet::AddBonus() - Failed to read  ID");
       (*rc) = err_parser_unexpected_data;
       return NULL;
   }
   
   //   2.   Read Token:
   pNext = ReadIntFromString(
                    pNext,         //   [in]      Source string
                    ",",           //   [in,opt]   Value termination
                    NULL,          //   [in,opt]   Allowed padding
                    &bonus.iToken,    //   [out]      Put it here
                    1);  //   [in]      Remove additional termination CHARS

   if( !pNext  || (-1 == bonus.iToken))
   {
        roadmap_log( ROADMAP_ERROR, "RTNet::AddBonus() - Failed to read  Token");
        (*rc) = err_parser_unexpected_data;
        return NULL;
   }

   //   3.   Read Lon:
   pNext = ReadIntFromString(
                     pNext,         //   [in]      Source string
                     ",",           //   [in,opt]   Value termination
                     NULL,          //   [in,opt]   Allowed padding
                     &bonus.position.longitude,    //   [out]      Put it here
                     1);  //   [in]      Remove additional termination CHARS

   if( !pNext ||  (-1 == bonus.position.longitude))
   {
         roadmap_log( ROADMAP_ERROR, "RTNet::AddBonus() - Failed to read  longitude");
         (*rc) = err_parser_unexpected_data;
         return NULL;
   }

   //   4.   Read Lat:
   pNext = ReadIntFromString(
                      pNext,         //   [in]      Source string
                      ",",           //   [in,opt]   Value termination
                      NULL,          //   [in,opt]   Allowed padding
                      &bonus.position.latitude,    //   [out]      Put it here
                      1);  //   [in]      Remove additional termination CHARS

   if( !pNext  || (-1 == bonus.position.latitude))
   {
          roadmap_log( ROADMAP_ERROR, "RTNet::AddBonus() - Failed to read  latitude");
          (*rc) = err_parser_unexpected_data;
          return NULL;
   }

   //   5.   Read points:
   pNext = ReadIntFromString(
                       pNext,         //   [in]      Source string
                       ",",           //   [in,opt]   Value termination
                       NULL,          //   [in,opt]   Allowed padding
                       &bonus.iNumPoints,    //   [out]      Put it here
                       1);  //   [in]      Remove additional termination CHARS

   if( !pNext || (-1 == bonus.iNumPoints))
   {
           roadmap_log( ROADMAP_ERROR, "RTNet::AddBonus() - Failed to read number of points");
           (*rc) = err_parser_unexpected_data;
           return NULL;
    }

    //   6.   Read radius:
    pNext = ReadIntFromString(
                        pNext,         //   [in]      Source string
                        ",",           //   [in,opt]   Value termination
                        NULL,          //   [in,opt]   Allowed padding
                        &bonus.iRadius,    //   [out]      Put it here
                        1);  //   [in]      Remove additional termination CHARS

    if( !pNext || (-1 == bonus.iRadius))
    {
            roadmap_log( ROADMAP_ERROR, "RTNet::AddBonus() - Failed to read radius");
            (*rc) = err_parser_unexpected_data;
            return NULL;
     }

     //  7. Icon name
     str[0] = 0;
     iBufferSize = 128;
     pNext       = ExtractNetworkString(
                       pNext,               // [in]     Source string
                       &str[0],  // [out,opt]Output buffer
                       &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                       ",",                 // [in]     Array of chars to terminate the copy operation
                       1);                  // [in]     Remove additional termination chars

     if( !pNext)
     {
              roadmap_log( ROADMAP_ERROR, "RTNet::AddBonus - Failed to read Icon Name");
              (*rc) = err_parser_unexpected_data;
              return NULL;
     }
     bonus.pIconName = &str[0];
     
     //   8.   Type
     pNext = ReadIntFromString(
                          pNext,         //   [in]      Source string
                          ",\r\n",           //   [in,opt]   Value termination
                          NULL,          //   [in,opt]   Allowed padding
                          &bonus.iType,    //   [out]      Put it here
                          TRIM_ALL_CHARS);  //   [in]      Remove additional termination CHARS

     if( !pNext || (-1 == bonus.iType))
     {
              roadmap_log( ROADMAP_ERROR, "RTNet::AddBonus() - Failed to read type, using type bonus points");
              bonus.iType = BONUS_TYPE_POINTS;
     }
     
  
     RealtimeBonus_Add(&bonus);
     
     return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char *RmBonus (/* IN  */   const char*       pNext,
               /* IN  */   void*             pContext,
               /* OUT */   BOOL*             more_data_needed,
               /* OUT */   roadmap_result*   rc)
{
   int   iId;

   //   Read ID:
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",\r\n",       // [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &iId,          // [out]    Put it here
                              TRIM_ALL_CHARS);  // [in]     Remove additional termination CHARS

   if( !pNext || (-1 == iId))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::RmBonus - Failed to read ID");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   
   RealtimeBonus_Delete(iId);
   return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char *CollectBonusRes (/* IN  */   const char*       pNext,
               /* IN  */   void*             pContext,
               /* OUT */   BOOL*             more_data_needed,
               /* OUT */   roadmap_result*   rc)
{
   int   iID, iPoints, iType;
   char  buff[256];
   int   iBufferSize;
   BOOL  bHasGift, bIsBigGift;
   //   1.   Read ID:
    pNext = ReadIntFromString(
                    pNext,         //   [in]      Source string
                    ",",           //   [in,opt]   Value termination
                    NULL,          //   [in,opt]   Allowed padding
                    &iID,    //   [out]      Put it here
                    1);  //   [in]      Remove additional termination CHARS

    if( !pNext || (-1 == iID))
    {
        roadmap_log( ROADMAP_ERROR, "RTNet::CollectBonusRes() - Failed to read  ID");
        (*rc) = err_parser_unexpected_data;
        return NULL;
    }
    
    //   Read points:
    pNext = ReadIntFromString( pNext,         // [in]     Source string
                               ",",       // [in,opt] Value termination
                               NULL,          // [in,opt] Allowed padding
                               &iPoints,          // [out]    Put it here
                               1);  // [in]     Remove additional termination CHARS

    if( !pNext || (-1 == iPoints))
    {
       roadmap_log( ROADMAP_ERROR, "RTNet::CollectBonusRes - Failed to read points");
       (*rc) = err_parser_unexpected_data;
       return NULL;
    }
    
    //   Read type
    pNext = ReadIntFromString( pNext,         // [in]     Source string
                               ",",       // [in,opt] Value termination
                               NULL,          // [in,opt] Allowed padding
                               &iType,          // [out]    Put it here
                               1);  // [in]     Remove additional termination CHARS

    if( !pNext || (-1 == iType))
    {
       roadmap_log( ROADMAP_ERROR, "RTNet::CollectBonusRes - Failed to read type");
       (*rc) = err_parser_unexpected_data;
       return NULL;
    }

    //  Has Gift
    buff[0] = 0;
    iBufferSize = sizeof(buff);
    pNext       = ExtractNetworkString(
                         pNext,               // [in]     Source string
                         &buff[0],  // [out,opt]Output buffer
                         &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                         ",",                 // [in]     Array of chars to terminate the copy operation
                         1);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
       roadmap_log( ROADMAP_ERROR, "RTNet::AddBonus - Failed to read Has Gift flag");
       (*rc) = err_parser_unexpected_data;
       return NULL;
    }
       if (buff[0] == 'T')
         bHasGift = TRUE;
       else
         bHasGift = FALSE;
       

    //  Big gift
    buff[0] = 0;
    iBufferSize = sizeof(buff);
    pNext       = ExtractNetworkString(
                         pNext,               // [in]     Source string
                         &buff[0],  // [out,opt]Output buffer
                         &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                         ",",                 // [in]     Array of chars to terminate the copy operation
                         1);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
       roadmap_log( ROADMAP_ERROR, "RTNet::AddBonus - Failed to read Big Gift flag");
       (*rc) = err_parser_unexpected_data;
       return NULL;
    }
 
    if (buff[0] == 'T')
       bIsBigGift = TRUE;
    else
       bIsBigGift = FALSE;

    //  The gift
    buff[0] = 0;
    iBufferSize = sizeof(buff);
    pNext       = ExtractNetworkString(
                         pNext,               // [in]     Source string
                         &buff[0],  // [out,opt]Output buffer
                         &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                         ",\r\n",                 // [in]     Array of chars to terminate the copy operation
                         TRIM_ALL_CHARS);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
       roadmap_log( ROADMAP_ERROR, "RTNet::AddBonus - Failed to read Gift");
       (*rc) = err_parser_unexpected_data;
       return NULL;
    }
    RealtimeBonus_CollectedPointsConfirmed(iID, iType, iPoints, bHasGift, bIsBigGift, buff);
    
    return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void ParseCreateAccountErrors( int status, roadmap_result*  rc )
{
   if ( status != 200 )
   {
	   roadmap_log( ROADMAP_ERROR, "Create account transaction has failed with status code: %d", status );
   }

   switch (status)
   {
	  case 200: //success
	  {
		 *rc = succeeded;
		 break;
	  }
	  case 901: //invalid user name
	  {
		 *rc = err_upd_account_invalid_user_name;
		 break;
	  }
	  case 902://user already exists
	  {
		 *rc = err_upd_account_name_already_exists;
		 break;
	  }
	  case 903://invalid password
	  {
		 *rc = err_upd_account_invalid_password;
		 break;
	  }
	  case 904:// invalid email
	  {
		 *rc = err_upd_account_invalid_email;
		 break;
	  }
	  case 905://Email address already exist
	  {
		 *rc = err_upd_account_email_exists;
		 break;
	  }
	  case 906://internal server error cannot complete request
	  {
		 *rc = err_upd_account_cannot_complete_request;
		 break;
	  }
	  default:
	  {
		 *rc = err_parser_unexpected_data;
		 roadmap_log (ROADMAP_ERROR,"roadmap_login_details_on_server_response - invalid status code (%d)" , status);
	  }
  }
}

void ParseUpdateAccountErrors( int status, roadmap_result*  rc )
{
   if ( status != 200 )
   {
	   roadmap_log( ROADMAP_ERROR, "Update account transaction has failed with status code: %d", status );
   }

   switch (status)
   {
	  case 200: //success
	  {
		 *rc = succeeded;
		 break;
	  }
	  case 911: //invalid user name
	  {
		 *rc = err_upd_account_invalid_user_name;
		 break;
	  }
	  case 912://user already exists
	  {
		 *rc = err_upd_account_name_already_exists;
		 break;
	  }
	  case 913://invalid password
	  {
		 *rc = err_upd_account_invalid_password;
		 break;
	  }
	  case 914:// invalid email
	  {
		 *rc = err_upd_account_invalid_email;
		 break;
	  }
	  case 915://Email address already exist
	  {
		 *rc = err_upd_account_email_exists;
		 break;
	  }
	  case 916://internal server error cannot complete request
	  {
		 *rc = err_upd_account_cannot_complete_request;
		 break;
	  }
	  default:
	  {
		 *rc = err_parser_unexpected_data;
		 roadmap_log (ROADMAP_ERROR,"roadmap_login_details_on_server_response - invalid status code (%d)" , status);
	  }
   }
}
