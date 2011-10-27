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
#include "RealtimeExternalPoi.h"
#include "RealtimeTrafficDetection.h"
#include "../editor/editor_points.h"
#include "../editor/editor_cleanup.h"
#include "../roadmap_geo_location_info.h"
#include "../roadmap_tripserver.h"
#include "roadmap_login.h"
#include "../roadmap_ticker.h"
#include "../roadmap_social.h"
#include "../roadmap_foursquare.h"
#include "../roadmap_scoreboard.h"
#include "../roadmap_mood.h"
#include "../roadmap_groups.h"
#include "../roadmap_geo_config.h"
#ifdef IPHONE
#include "../iphone/roadmap_push_notifications.h"
#endif

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

         //case 2001: // - invalid session id
         case 2002: // - not connected
         //case 2003: // - facebook error
            //Facebook valid errors
            (*rc) = err_failed;
            return pNext + iBytesRead;

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
                             ",",              //   [in,opt]  Value termination
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
                              ",",              //   [in,opt]  Value termination
                              NULL,             //   [in,opt]  Allowed padding
                              &pCI->iPointsTimeStamp,  //   [out]     Put it here
                              1);  //   [in]      Remove additional termination chars
   if( !pNext)
   {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnLoginResponse() - Failed to read my points time stamp (%s)", pLast);
         (*rc) = err_parser_unexpected_data;
         return NULL;   //   Quit the 'receive' loop
   }

   //Exclusive Moods
   pNext = ReadIntFromString( pNext,            //   [in]      Source string
                             ",\r\n",              //   [in,opt]  Value termination
                             NULL,             //   [in,opt]  Allowed padding
                             &pCI->iExclusiveMoods,  //   [out]     Put it here
                             1);  //   [in]      Remove additional termination chars
   if( !pNext )
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnLoginResponse() - Failed to read iExclusiveMoods (%s)", pLast);
      (*rc) = err_parser_unexpected_data;
      return NULL;   //   Quit the 'receive' loop
   }

   roadmap_mood_set_exclusive_moods(pCI->iExclusiveMoods);

   pNext = ReadIntFromString( pNext,            //   [in]      Source string
                              ",",              //   [in,opt]  Value termination
                              NULL,             //   [in,opt]  Allowed padding
                              &pCI->iServerMaxProtocol,  //   [out]     Put it here
                              1);  //   [in]      Remove additional termination chars
   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnLoginResponse() - Failed to read ServerMaxProtocol", pLast);
      (*rc) = err_parser_unexpected_data;
      return NULL;   //   Quit the 'receive' loop
   }

   iBufferSize = MAX_SERVER_VERSION;
   pNext       = ExtractNetworkString(
               pNext,               // [in]     Source string
               pCI->serverVersion,   // [out,opt]Output buffer
               &iBufferSize,        // [in,out] Buffer size / Size of extracted string
               ",\r\n",              // [in]     Array of chars to terminate the copy operation
               TRIM_ALL_CHARS);     // [in]     Remove additional termination chars
   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnLoginResponse(case 2) - Did not find server-version");
      (*rc) = err_parser_unexpected_data;
      return NULL;   //   Quit the 'receive' loop
   }

   //   Done
   editor_points_set_old_points(pCI->iMyTotalPoints, pCI->iPointsTimeStamp);

   //   Update status variables:
   pCI->bLoggedIn = TRUE;

   roadmap_screen_redraw();
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
   char                 tempBuff[5];
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
                             1);  //   [in]      Remove additional termination CHARS

   if(!pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read points");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   //  14.   Join Date
   pNext = ReadIntFromString(
                             pNext,            //   [in]      Source string
                             ",",              //   [in,opt]   Value termination
                             NULL,             //   [in,opt]   Allowed padding
                             &UL.iJoinDate,     //   [out]      Put it here
                             1);  //   [in]      Remove additional termination CHARS

   if(!pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read join date");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }


   pNext       = ReadIntFromString(
                                  pNext,             // [in]     Source string
                                  ",",          //   [in]   Array of chars to terminate the copy operation
                                  NULL,             //   [in,opt]   Allowed padding
                                  &UL.iPingFlag,     //   [out]      Put it here
                                  1);   // [in]     Remove additional termination chars
   if( !pNext)
   {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read Can Ping Wazer flag");
         (*rc) = err_parser_unexpected_data;
         return NULL;
   }


   //   15.   Facebook name
   if( ',' == (*pNext))
   {
      roadmap_log( ROADMAP_DEBUG, "RTNet::OnGeneralResponse::AddUser() - FacebookUserName was not supplied.");
      pNext++;   //   Jump over the comma (,)
   }
   else
   {
      //   Read user name:
      iBufferSize = RT_USERFACEBOOK_MAXSIZE;
      pNext       = ExtractNetworkString(
                  pNext,           // [in]     Source string
                  UL.sFacebookName,        // [out,opt]Output buffer
                  &iBufferSize,    // [in,out] Buffer size / Size of extracted string
                  ",",             // [in]     Array of chars to terminate the copy operation
                  1); // [in]     Remove additional termination chars

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read facebook user name");
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
   }


   //   Show Facebook picture
   iBufferSize = sizeof(tempBuff);
   pNext       = ExtractNetworkString(
                  pNext,             // [in]     Source string
                  tempBuff,//   [out]   Output buffer
                  &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                  ",",          //   [in]   Array of chars to terminate the copy operation
                  1);   // [in]     Remove additional termination chars

    if( !pNext)
    {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read bShowFacebookPicture id=%d", UL.iID);
         (*rc) = err_parser_unexpected_data;
         return NULL;
    }


   if (tempBuff[0] == 'T')
      UL.bShowFacebookPicture = TRUE;
   else
      UL.bShowFacebookPicture = FALSE;

   //   Group
   if( ',' == (*pNext))
   {
      roadmap_log( ROADMAP_DEBUG, "RTNet::OnGeneralResponse::AddUser() - Group was not supplied.");
      pNext++;   //   Jump over the comma (,)
   }
   else
   {
      //   Read user name:
      iBufferSize = RT_USER_GROUP_MAXSIZE;
      pNext       = ExtractNetworkString(
                  pNext,           // [in]     Source string
                  UL.sGroup,        // [out,opt]Output buffer
                  &iBufferSize,    // [in,out] Buffer size / Size of extracted string
                  ",",             // [in]     Array of chars to terminate the copy operation
                  1); // [in]     Remove additional termination chars

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read group id=%d", UL.iID);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
   }

   //   is Facebook friend
   iBufferSize = sizeof(tempBuff);
   pNext       = ExtractNetworkString(
                  pNext,             // [in]     Source string
                  tempBuff,//   [out]   Output buffer
                  &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                  ",\r\n",          //   [in]   Array of chars to terminate the copy operation
                  1);   // [in]     Remove additional termination chars

    if( !pNext)
    {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read bFacebookFriend id=%d", UL.iID);
         (*rc) = err_parser_unexpected_data;
         return NULL;
    }


   if (tempBuff[0] == 'T')
      UL.bFacebookFriend = TRUE;
   else
      UL.bFacebookFriend = FALSE;

   //   Group Icon
   if( ',' == (*pNext))
   {
      pNext++;   //   Jump over the comma (,)
   }
   else
   {
      //   Read group icon:
      iBufferSize = RT_USER_GROUP_ICON_MAXSIZE;
      pNext       = ExtractNetworkString(
                  pNext,           // [in]     Source string
                  UL.sGroupIcon,        // [out,opt]Output buffer
                  &iBufferSize,    // [in,out] Buffer size / Size of extracted string
                  ",",             // [in]     Array of chars to terminate the copy operation
                  1); // [in]     Remove additional termination chars

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read group icon.");
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
   }

   //   Group Relevance
   pNext = ReadIntFromString(
            pNext,            //   [in]      Source string
            ",\r\n",              //   [in,opt]   Value termination
            NULL,             //   [in,opt]   Allowed padding
            &UL.iGroupRelevance,  //   [out]      Put it here
            1);               //   [in]      Remove additional termination CHARS


   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read group relevance.");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   
   
   //   VIP flags
   pNext = ReadIntFromString(
                             pNext,            //   [in]      Source string
                             ",\r\n",              //   [in,opt]   Value termination
                             NULL,             //   [in,opt]   Allowed padding
                             &UL.iVipFlags,  //   [out]      Put it here
                             1);               //   [in]      Remove additional termination CHARS
   
   
   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read VIP flags.");
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
   int                  iTemp;
   const char*          pLast;

   RTSystemMessage_Init( &Msg);

   //   1. ID:
   pNext = ReadIntFromString( pNext,            // [in]     Source string
                              ",",              // [in,opt] Value termination
                              NULL,             // [in,opt] Allowed padding
                              &Msg.iId,       // [out]    Put it here
                              1);  // [in]     Remove additional termination CHARS

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
                              1);  // [in]     Remove additional termination CHARS

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
                              1);  // [in]     Remove additional termination CHARS

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
               1);  // [in]     Remove additional termination chars

   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::SystemMessage() - Failed to read message title");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   if( iBufferSize)
   {
      iBufferSize++; // Additional space for the terminating null...
      Msg.title   = malloc(iBufferSize);
      pNext       = ExtractNetworkString(
                  pLast,         // [in]     Source string
                  Msg.title,     // [out,opt]Output buffer
                  &iBufferSize,  // [in,out] Buffer size / Size of extracted string
                  ",",           // [in]     Array of chars to terminate the copy operation
                  1);  // [in]     Remove additional termination chars

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
               1);  // [in]     Remove additional termination chars

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
   Msg.msg    = malloc(iBufferSize);
   pNext       = ExtractNetworkString(
               pLast,            // [in]     Source string
               Msg.msg,         // [out,opt]Output buffer
               &iBufferSize,     // [in,out] Buffer size / Size of extracted string
               ",\r\n",          // [in]     Array of chars to terminate the copy operation
               1);  // [in]     Remove additional termination chars


   //   5.  Icon:
   pLast       = pNext;
   iBufferSize = RTNET_SYSTEMMESSAGE_ICON_MAXSIZE;
   pNext       = ExtractNetworkString(
                pNext,            // [in]     Source string
                NULL,             // [out,opt]Output buffer
                &iBufferSize,     // [in,out] Buffer size / Size of extracted string
                ",",          // [in]     Array of chars to terminate the copy operation
                1);  // [in]     Remove additional termination chars

   if( !pNext)
   {
       RTSystemMessage_Free( &Msg);
       roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::SystemMessage() - Failed to read icon");
       (*rc) = err_parser_unexpected_data;
       return NULL;
    }

    if( iBufferSize)
    {
       iBufferSize++; // Additional space for the terminating null...
       Msg.icon    = malloc(iBufferSize);
       pNext       = ExtractNetworkString(
                      pLast,            // [in]     Source string
                      Msg.icon,         // [out,opt]Output buffer
                      &iBufferSize,     // [in,out] Buffer size / Size of extracted string
                      ",",          // [in]     Array of chars to terminate the copy operation
                      1);  // [in]     Remove additional termination chars

   }
   else{
      Msg.icon    = NULL;
   }

    //   6.  Title text color
   if( ',' != (*pNext)){
      iBufferSize = RTNET_SYSTEMMESSAGE_TEXT_COLOR_MAXSIZE; // Additional space for the terminating null...
      pNext       = ExtractNetworkString(
                                pLast,            // [in]     Source string
                                Msg.titleTextColor,         // [out,opt]Output buffer
                                &iBufferSize,     // [in,out] Buffer size / Size of extracted string
                                ",",          // [in]     Array of chars to terminate the copy operation
                                1);  // [in]     Remove additional termination chars
      if( !pNext)
      {
           roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::SystemMessage() - Failed to read title text color");
           (*rc) = err_parser_unexpected_data;
           return NULL;
      }
   }
   else{
      pNext++;
   }

   //   7.  Title text size
   pNext = ReadIntFromString( pNext,            // [in]     Source string
                                ",",              // [in,opt] Value termination
                                NULL,             // [in,opt] Allowed padding
                                &iTemp,       // [out]    Put it here
                                1);  // [in]     Remove additional termination CHARS

   if( !pNext)
   {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::SystemMessage() - Failed to read title text size");
        (*rc) = err_parser_unexpected_data;
        return NULL;
   }

   if (iTemp != 0)
      Msg.titleTextSize = iTemp;

   //   8.  msg text color
   if( ',' != (*pNext)){
      iBufferSize = RTNET_SYSTEMMESSAGE_TEXT_COLOR_MAXSIZE; // Additional space for the terminating null...
      pNext       = ExtractNetworkString(
                                pLast,            // [in]     Source string
                                Msg.msgTextColor,         // [out,opt]Output buffer
                                &iBufferSize,     // [in,out] Buffer size / Size of extracted string
                                ",",          // [in]     Array of chars to terminate the copy operation
                                1);  // [in]     Remove additional termination chars
      if( !pNext)
      {
           roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::SystemMessage() - Failed to read msg text color");
           (*rc) = err_parser_unexpected_data;
           return NULL;
      }
   }
   else{
      pNext++;
   }

   //   9.  msg text size
   pNext = ReadIntFromString( pNext,            // [in]     Source string
                                 ",\r\n",              // [in,opt] Value termination
                                 NULL,             // [in,opt] Allowed padding
                                 &iTemp,       // [out]    Put it here
                                 TRIM_ALL_CHARS);  // [in]     Remove additional termination CHARS

   if( !pNext)
   {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::SystemMessage() - Failed to read msg text size");
         (*rc) = err_parser_unexpected_data;
         return NULL;
   }

   if (iTemp != 0)
      Msg.msgTextSize = iTemp;

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
   char     tempBuff[5];

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
   iBufferSize = sizeof(tempBuff);
   pNext       = ExtractNetworkString(
                  pNext,             // [in]     Source string
                  tempBuff,//   [out]   Output buffer
                  &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                  ",",          //   [in]   Array of chars to terminate the copy operation
                  1);   // [in]     Remove additional termination chars

    if( !pNext)
    {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read user named id=%d", alert.iID);
         (*rc) = err_parser_unexpected_data;
         return NULL;
    }


   if (tempBuff[0] == 'T')
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
                  ",",               // [in]     Array of chars to terminate the copy operation
                  1);   //   [in]   Remove additional termination chars

   //   Read If the alert is a ping for me
   iBufferSize = sizeof(tempBuff);
   tempBuff[0] = 0;
   pNext       = ExtractNetworkString(
                   pNext,             // [in]     Source string
                   tempBuff,//   [out]   Output buffer
                   &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                   ",",          //   [in]   Array of chars to terminate the copy operation
                   1);   // [in]     Remove additional termination chars

   if( !pNext)
   {
       roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read PingWazer flag id=%d", alert.iID);
       (*rc) = err_parser_unexpected_data;
       return NULL;
   }

   if (tempBuff[0] == 'T')
      alert.bPingWazer = TRUE;
   else
      alert.bPingWazer = FALSE;

   //   Read If the alert is on my route
   iBufferSize = sizeof(tempBuff);
   tempBuff[0] = 0;
   pNext       = ExtractNetworkString(
                    pNext,             // [in]     Source string
                    tempBuff,//   [out]   Output buffer
                    &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                    ",",          //   [in]   Array of chars to terminate the copy operation
                    1);   // [in]     Remove additional termination chars

    if( !pNext)
    {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read AlertOnRoute flag id=%d", alert.iID);
        (*rc) = err_parser_unexpected_data;
        return NULL;
    }

    if (tempBuff[0] == 'T'){
       alert.bAlertIsOnRoute = TRUE;
    }else
       alert.bAlertIsOnRoute = FALSE;

   //   Alert Type - Optional
   if(',' == (*pNext))
   {
      pNext++;   //   Jump over the comma (,)
   }
   else
   {
      //   AlertType
      iBufferSize = RT_ALERTS_MAX_ALERT_TYPE;
      pNext       = ExtractNetworkString(
                  pNext,               // [in]     Source string
                  alert.sAlertType,  // [out,opt]Output buffer
                  &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                  ",",                 // [in]     Array of chars to terminate the copy operation
                  1);                  // [in]     Remove additional termination chars

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read sAlertType id=%d", alert.iID);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
   }

   //   addon Name
   if(',' == (*pNext))
   {
      pNext++;   //   Jump over the comma (,)
   }
   else
   {
      //   addon Name
      iBufferSize = RT_ALERTS_MAX_ADD_ON_NAME;
      pNext       = ExtractNetworkString(
                  pNext,               // [in]     Source string
                  alert.sAddOnName,  // [out,opt]Output buffer
                  &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                  ",",                 // [in]     Array of chars to terminate the copy operation
                  1);                  // [in]     Remove additional termination chars

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read AddonName id=%d", alert.iID);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
   }

   //   facebookName
    if(',' == (*pNext))
    {
       roadmap_log( ROADMAP_DEBUG, "RTNet::OnGeneralResponse::AddAlert() - facebookName by was not supplied. id=%d", alert.iID);
       pNext++;   //   Jump over the comma (,)
    }
    else
    {
       //   facebookName
       iBufferSize = RT_ALERT_FACEBOOK_USERNM_MAXSIZE;
       pNext       = ExtractNetworkString(
                   pNext,               // [in]     Source string
                   alert.sFacebookName,  // [out,opt]Output buffer
                   &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                   ",",                 // [in]     Array of chars to terminate the copy operation
                   1);                  // [in]     Remove additional termination chars

       if( !pNext || !(*pNext))
       {
          roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read facebookName id=%d", alert.iID);
          (*rc) = err_parser_unexpected_data;
          return NULL;
       }
   }

   //  Show Facebook Image
   iBufferSize = sizeof(tempBuff);
   tempBuff[0] = 0;
   pNext       = ExtractNetworkString(
                    pNext,             // [in]     Source string
                    tempBuff,//   [out]   Output buffer
                    &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                    ",",          //   [in]   Array of chars to terminate the copy operation
                    1);   // [in]     Remove additional termination chars

    if( !pNext)
    {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read bShowFacebookPicture flag id=%d", alert.iID);
        (*rc) = err_parser_unexpected_data;
        return NULL;
    }

    if (tempBuff[0] == 'T')
       alert.bShowFacebookPicture = TRUE;
    else
       alert.bShowFacebookPicture = FALSE;

    //   group
    iBufferSize = RT_ALERT_GROUP_MAXSIZE;
    pNext       = ExtractNetworkString(
                   pNext,               // [in]     Source string
                   alert.sGroup,  // [out,opt]Output buffer
                   &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                   ",\r\n",                 // [in]     Array of chars to terminate the copy operation
                   1);                  // [in]     Remove additional termination chars


    if( !pNext)
    {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read group id=%d", alert.iID);
        (*rc) = err_parser_unexpected_data;
        return NULL;
    }

    //   group icon
    iBufferSize = RT_ALERT_GROUP_ICON_MAXSIZE;
    pNext       = ExtractNetworkString(
                   pNext,               // [in]     Source string
                   alert.sGroupIcon,  // [out,opt]Output buffer
                   &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                   ",",                 // [in]     Array of chars to terminate the copy operation
                   1);                  // [in]     Remove additional termination chars
   if( !pNext)
   {
       roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to  group icon id=%d", alert.iID);
       (*rc) = err_parser_unexpected_data;
       return NULL;
   }

   //   Group Relevance
   pNext = ReadIntFromString(
            pNext,            //   [in]      Source string
            ",",              //   [in,opt]   Value termination
            NULL,             //   [in,opt]   Allowed padding
            &alert.iGroupRelevance,  //   [out]      Put it here
            1);               //   [in]      Remove additional termination CHARS


   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to  group relevance id=%d", alert.iID);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   When the alert was reported (x seconds ago)
    pNext = ReadIntFromString(
             pNext,            //   [in]      Source string
             ",",              //   [in,opt]   Value termination
             NULL,             //   [in,opt]   Allowed padding
             &alert.iReportedElapsedTime,  //   [out]      Put it here
             1);               //   [in]      Remove additional termination CHARS


    if( !pNext)
    {
       roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to  read ReportedElapsedTime id=%d", alert.iID);
       (*rc) = err_parser_unexpected_data;
       return NULL;
    }

    // Is this an archive alert

    iBufferSize = sizeof(tempBuff);
    tempBuff[0] = 0;
    pNext       = ExtractNetworkString(
                    pNext,             // [in]     Source string
                    tempBuff,//   [out]   Output buffer
                    &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                    ",",          //   [in]   Array of chars to terminate the copy operation
                    1);   // [in]     Remove additional termination chars

    if( !pNext)
    {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read bArchive flag id=%d", alert.iID);
        (*rc) = err_parser_unexpected_data;
        return NULL;
    }

    if (tempBuff[0] == 'T') {
       alert.bArchive = TRUE;
    } else {
       alert.bArchive = FALSE;
    }

    //   Subtype
     pNext = ReadIntFromString(
              pNext,            //   [in]      Source string
              ",",              //   [in,opt]   Value termination
              NULL,             //   [in,opt]   Allowed padding
              &alert.iSubType,  //   [out]      Put it here
              1);               //   [in]      Remove additional termination CHARS


    if( !pNext)
    {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to  subtype ");
        (*rc) = err_parser_unexpected_data;
        return NULL;
    }

    // Number of thums up
    pNext = ReadIntFromString( pNext,         // [in]     Source string
                               ",",       // [in,opt] Value termination
                               NULL,          // [in,opt] Allowed padding
                               &alert.iNumThumbsUp,          // [out]    Put it here
                               1);  // [in]     Remove additional termination CHARS

    if( !pNext)
    {
       roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read  Number of thumbs up");
       (*rc) = err_parser_unexpected_data;
       return NULL;
    }


    //  Thumbs Up by Me
    iBufferSize = sizeof(tempBuff);
    tempBuff[0] = 0;
    pNext       = ExtractNetworkString(
                     pNext,             // [in]     Source string
                     tempBuff,//   [out]   Output buffer
                     &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                     ",",          //   [in]   Array of chars to terminate the copy operation
                     1);   // [in]     Remove additional termination chars

     if( !pNext)
     {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read bThumbsUpByMe flag id=%d", alert.iID);
         (*rc) = err_parser_unexpected_data;
         return NULL;
     }

     if (tempBuff[0] == 'T')
        alert.bThumbsUpByMe = TRUE;
     else
        alert.bThumbsUpByMe = FALSE;

   //   Read voice ID:
   iBufferSize = RT_ALERT_VOICEID_MAXSIZE;
   pNext = ExtractNetworkString(
                                pNext,            //   [in]   Source string
                                alert.sVoiceIdStr, // [out,opt]Output buffer
                                &iBufferSize,   //   [in]   Output buffer size
                                ",\r\n",               // [in]     Array of chars to terminate the copy operation
                                TRIM_ALL_CHARS);   //   [in]   Remove additional termination chars
   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read Voice ID");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   	// num viewed
   	pNext = ReadIntFromString(pNext,          					//   [in]      Source string
                                ",\r\n",           					//   [in,opt]  Value termination
                                NULL,          					//   [in,opt]  Allowed padding
                                &alert.iNumViewed,	//   [out]     Output value
                                TRIM_ALL_CHARS);            					//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

   	if (!pNext) {
   		roadmap_log (ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read num viewed");
   		return NULL;
   	}

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

   if (!strcmp(serviceName,"SCOREBOARD")){
      return roadmap_scoreboard_response(status, rc, numParameters, pNext);
   }
   else if (!strcmp(serviceName,"TWITTER")){
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
#ifdef IPHONE
   else if (!strcmp(serviceName,"PUSHNOTIFICATIONS")){
      if (status == 200){
         roadmap_push_notifications_set_pending(FALSE);
      }
   }
#endif //IPHONE
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
   char                 Displayed[5];
   char                 tempBuff[5];

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
            ",",              //   [in,opt]   Value termination
            NULL,             //   [in,opt]   Allowed padding
            &comment.iRank,  //   [out]      Put it here
            1);               //   [in]      Remove additional termination CHARS

    if( !pNext)
    {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlertComment() - Failed to read rank");
      (*rc) = err_parser_unexpected_data;
      return NULL;
    }

    //   Read If the comment was displayed
    iBufferSize = sizeof(Displayed);
    pNext = ExtractNetworkString(
               pNext,            //   [in]   Source string
               Displayed,    //   [out]   Output buffer
               &iBufferSize,  //   [in]   Output buffer size
               ",",          //   [in]   Array of chars to terminate the copy operation
               1);   //   [in]   Remove additional termination chars

      if( !pNext)
     {
          roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlertComment() - Failed to read Displayed flag");
          (*rc) = err_parser_unexpected_data;
          return NULL;
      }

     if (Displayed[0] == 'T')
       comment.bDisplay= TRUE;
    else
       comment.bDisplay = FALSE;

     //FaceBook Name
     if(',' == (*pNext))
     {
        roadmap_log( ROADMAP_DEBUG, "RTNet::OnGeneralResponse::AddAlertComment() - facebookName by was not supplied.");
        pNext++;   //   Jump over the comma (,)
     }
     else
     {
        //   facebookName
        iBufferSize = RT_ALERT_FACEBOOK_USERNM_MAXSIZE;
        pNext       = ExtractNetworkString(
                    pNext,               // [in]     Source string
                    comment.sFacebookName,  // [out,opt]Output buffer
                    &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                    ",",                 // [in]     Array of chars to terminate the copy operation
                    1);                  // [in]     Remove additional termination chars

        if( !pNext || !(*pNext))
        {
           roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlertComment() - Failed to read facebookName");
           (*rc) = err_parser_unexpected_data;
           return NULL;
        }
   }

   //  Show Facebook Image
   iBufferSize = sizeof(tempBuff);
   tempBuff[0] = 0;
   pNext       = ExtractNetworkString(
                    pNext,             // [in]     Source string
                    tempBuff,//   [out]   Output buffer
                    &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                    ",\r\n",          //   [in]   Array of chars to terminate the copy operation
                    TRIM_ALL_CHARS);   // [in]     Remove additional termination chars

    if( !pNext)
    {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlertComment() - Failed to read bShowFacebookPicture flag id=%d", comment.iID);
        (*rc) = err_parser_unexpected_data;
        return NULL;
    }

    if (tempBuff[0] == 'T')
       comment.bShowFacebookPicture = TRUE;
    else
       comment.bShowFacebookPicture = FALSE;


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
   int				iSpeedTimes10;
   char           tempBuff[5];
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
   trafficInfo.fSpeedMpS = iSpeedTimes10 / 10.0;

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
   iBufferSize = RT_TRAFFIC_INFO_ADDRESS_MAXSIZE;
   pNext       = ExtractNetworkString(
               pNext,               // [in]     Source string
               trafficInfo.sEnd,  // [out,opt]Output buffer
               &iBufferSize,        // [in,out] Buffer size / Size of extracted string
               ",",                 // [in]     Array of chars to terminate the copy operation
               1);                  // [in]     Remove additional termination chars

   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read End");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }


   trafficInfo.iNumGeometryPoints = 0;

   //   Read If the alert is on my route
   iBufferSize = sizeof(tempBuff);
   tempBuff[0] = 0;
   pNext       = ExtractNetworkString(
                    pNext,             // [in]     Source string
                    tempBuff,//   [out]   Output buffer
                    &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                    ",",          //   [in]   Array of chars to terminate the copy operation
                    1);   // [in]     Remove additional termination chars

    if( !pNext)
    {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read isOnRoute flag id=%d", trafficInfo.iID);
        (*rc) = err_parser_unexpected_data;
        return NULL;
    }

    if (tempBuff[0] == 'T')
       trafficInfo.bIsOnRoute = TRUE;
    else
       trafficInfo.bIsOnRoute = FALSE;

    //   Read If the jam is alertable
    iBufferSize = sizeof(tempBuff);
    tempBuff[0] = 0;
    pNext       = ExtractNetworkString(
                     pNext,             // [in]     Source string
                     tempBuff,//   [out]   Output buffer
                     &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                     ",\r\n",          //   [in]   Array of chars to terminate the copy operation
                     TRIM_ALL_CHARS);   // [in]     Remove additional termination chars

     if( !pNext)
     {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddRoadInfo() - Failed to read bIsAlertable flag id=%d", trafficInfo.iID);
         (*rc) = err_parser_unexpected_data;
         return NULL;
     }

     if (tempBuff[0] == 'T')
        trafficInfo.bIsAlertable = TRUE;
     else
        trafficInfo.bIsAlertable = FALSE;

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
const char* RoadInfoGeom(/* IN  */   const char*       pNext,
                         /* IN  */   void*             pContext,
                         /* OUT */   BOOL*             more_data_needed,
                         /* OUT */   roadmap_result*   rc)
{
	int					iID;
	RTTrafficInfo		*pTrafficInfo;
	int					iNumCoords;
	RoadMapPosition	lastPosition;
	RoadMapPosition	diffPosition;
	int					iCoord;

   //   1.   Read RoadInfo ID:
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",",       		// [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &iID,          // [out]    Put it here
                              1);  // [in]     Remove additional termination CHARS

   if( !pNext || (-1 == iID))
   {
      roadmap_log( ROADMAP_ERROR, "RoadInfoGeom() - Failed to read  ID");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

	// Get RoadInfo record
	pTrafficInfo = RTTrafficInfo_RecordByID (iID);
	if (pTrafficInfo == NULL)
	{
      roadmap_log( ROADMAP_ERROR, "RoadInfoGeom() - ID not matching a road info");
      (*rc) = err_parser_failed;
      return NULL;
	}

   //   2.   Read Number of points:
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",",       		// [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &iNumCoords,          // [out]    Put it here
                              1);  // [in]     Remove additional termination CHARS

   if( !pNext )
   {
      roadmap_log( ROADMAP_ERROR, "RoadInfoGeom() - Failed to read  iNumCoords");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

	if (iNumCoords < 2 || iNumCoords % 2 != 0)
	{
      roadmap_log( ROADMAP_ERROR, "RoadInfoGeom() - Invalid value %d for  iNumCoords", iNumCoords);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   iNumCoords /= 2;
   lastPosition.latitude = 0;
   lastPosition.longitude = 0;

   if (iNumCoords > RT_TRAFFIC_INFO_MAX_GEOM)
   {
   	roadmap_log (ROADMAP_WARNING, "Too many coords (%d) for road info %d", iNumCoords, iID);
   }

   pTrafficInfo->iNumGeometryPoints = 0;
   for (iCoord = 0; iCoord < iNumCoords; iCoord++)
   {
	   pNext = ReadIntFromString( pNext,         // [in]     Source string
	                              ",",       		// [in,opt] Value termination
	                              NULL,          // [in,opt] Allowed padding
	                              &diffPosition.longitude,          // [out]    Put it here
	                              1);  // [in]     Remove additional termination CHARS

	   if( !pNext )
	   {
	      roadmap_log( ROADMAP_ERROR, "RoadInfoGeom() - Failed to read coordinate %d", iCoord);
	      (*rc) = err_parser_unexpected_data;
	      return NULL;
	   }

	   pNext = ReadIntFromString( pNext,         // [in]     Source string
	                              ",\r\n",       		// [in,opt] Value termination
	                              NULL,          // [in,opt] Allowed padding
	                              &diffPosition.latitude,          // [out]    Put it here
	                              iCoord < iNumCoords - 1 ? 1 : TRIM_ALL_CHARS);  // [in]     Remove additional termination CHARS

	   if( !pNext )
	   {
	      roadmap_log( ROADMAP_ERROR, "RoadInfoGeom() - Failed to read coordinate %d", iCoord);
	      (*rc) = err_parser_unexpected_data;
	      return NULL;
	   }

   	lastPosition.latitude += diffPosition.latitude;
   	lastPosition.longitude += diffPosition.longitude;
   	if (iCoord < RT_TRAFFIC_INFO_MAX_GEOM)
   	{
   		pTrafficInfo->geometry[iCoord] = lastPosition;
   		pTrafficInfo->iNumGeometryPoints++;
   	}
   }

   RTTrafficInfo_UpdateGeometry (pTrafficInfo);
   return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#define MAX_ROAD_INFO_SEGMENTS 200

const char* RoadInfoSegments(/* IN  */   const char*       pNext,
                         /* IN  */   void*             pContext,
                         /* OUT */   BOOL*             more_data_needed,
                         /* OUT */   roadmap_result*   rc)
{
	int					iID;
	int					nSegments;
	int					iSegment;
	int					tile;
	int					version;
	int					line_ids[MAX_ROAD_INFO_SEGMENTS];

   //   1.   Read RoadInfo ID:
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",",       		// [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &iID,          // [out]    Put it here
                              1);  // [in]     Remove additional termination CHARS

   if( !pNext || (-1 == iID))
   {
      roadmap_log( ROADMAP_ERROR, "RoadInfoGeom() - Failed to read  ID");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   2.   Read Tile ID:
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",",       		// [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &tile,          // [out]    Put it here
                              1);  // [in]     Remove additional termination CHARS

   if( !pNext )
   {
      roadmap_log( ROADMAP_ERROR, "RoadInfoGeom() - Failed to read tile ID");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   3.   Read Tile Version:
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",",       		// [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &version,          // [out]    Put it here
                              1);  // [in]     Remove additional termination CHARS

   if( !pNext )
   {
      roadmap_log( ROADMAP_ERROR, "RoadInfoGeom() - Failed to read tile version");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   4.   Read Number of segments:
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",",       		// [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &nSegments,          // [out]    Put it here
                              1);  // [in]     Remove additional termination CHARS

   if( !pNext )
   {
      roadmap_log( ROADMAP_ERROR, "RoadInfoGeom() - Failed to read no. of segments");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

	if (nSegments < 1)
	{
      roadmap_log( ROADMAP_ERROR, "RoadInfoGeom() - Invalid valued %d for no. of segments", nSegments);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   if (nSegments > MAX_ROAD_INFO_SEGMENTS)
   {
   	roadmap_log (ROADMAP_WARNING, "Too many segments (%d) for road info %d", nSegments, iID);
   }

   for (iSegment = 0; iSegment < nSegments; iSegment++)
   {
   	int line_id;

	   pNext = ReadIntFromString( pNext,         // [in]     Source string
	                              ",\r\n",       		// [in,opt] Value termination
	                              NULL,          // [in,opt] Allowed padding
	                              &line_id,          // [out]    Put it here
	                              iSegment < nSegments - 1 ? 1 : TRIM_ALL_CHARS);  // [in]     Remove additional termination CHARS

	   if( !pNext )
	   {
	      roadmap_log( ROADMAP_ERROR, "RoadInfoGeom() - Failed to read segment %d", iSegment);
	      (*rc) = err_parser_unexpected_data;
	      return NULL;
	   }

   	if (iSegment < MAX_ROAD_INFO_SEGMENTS)
   	{
   		line_ids[iSegment] = line_id;
   	}
   }

   RTTrafficInfo_AddSegments (iID, tile, version, nSegments, line_ids);
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
   editor_cleanup_test (iTimestamp - 24 * 60 * 60, FALSE);

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
   RTAlertRes AlertRes;
   int iBufferSize;

   //   1.   Read points:
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",",       // [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &AlertRes.iPoints,          // [out]    Put it here
                              1);  // [in]     Remove additional termination CHARS

   if( !pNext )
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::ReportAlertRes() - Failed to read  Points");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   iBufferSize = RT_ALERT_RES_TITLE_MAX_SIZE;
   AlertRes.title[0] = 0;
   pNext       = ExtractNetworkString(
               pNext,         // [in]     Source string
               &AlertRes.title[0],          // [out,opt]Output buffer
               &iBufferSize,  // [in,out] Buffer size / Size of extracted string
               ",",           // [in]     Array of chars to terminate the copy operation
               1);  // [in]     Remove additional termination chars

   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::ReportAlertRes() - Failed to read  tite");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   iBufferSize = RT_ALERT_RES_TEXT_MAX_SIZE;
   AlertRes.msg[0] = 0;
   pNext       = ExtractNetworkString(
                pNext,         // [in]     Source string
                &AlertRes.msg[0],          // [out,opt]Output buffer
                &iBufferSize,  // [in,out] Buffer size / Size of extracted string
                ",\r\n",           // [in]     Array of chars to terminate the copy operation
                TRIM_ALL_CHARS);  // [in]     Remove additional termination chars

   if( !pNext)
   {
       roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::ReportAlertRes() - Failed to read msg");
       (*rc) = err_parser_unexpected_data;
       return NULL;
   }

   if (AlertRes.msg[0] != 0){
      if (AlertRes.iPoints > 0)
         roadmap_messagebox_timeout(AlertRes.title, AlertRes.msg, 5);
      else
         roadmap_messagebox(AlertRes.title, AlertRes.msg);
   }

   if (AlertRes.iPoints > 0){
       editor_points_add_new_points(AlertRes.iPoints);
       editor_points_display_new_points_timed(AlertRes.iPoints, 5, report_event);
    }

   return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char* ReportTrafficRes( /* IN  */   const char*       pNext,
                            /* IN  */   void*             pContext,
                            /* OUT */   BOOL*             more_data_needed,
                            /* OUT */   roadmap_result*   rc){

   RealtimeTrafficDetection_Res TrafficDetecteionRes;
   int iBufferSize;

   //   1.   Read points:
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",",       // [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &TrafficDetecteionRes.iPoints,          // [out]    Put it here
                              1);  // [in]     Remove additional termination CHARS

   if( !pNext )
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::ReportTrafficRes() - Failed to read  Points");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   iBufferSize = RT_ALERT_RES_TITLE_MAX_SIZE;
   TrafficDetecteionRes.title[0] = 0;
   pNext       = ExtractNetworkString(
               pNext,         // [in]     Source string
               &TrafficDetecteionRes.title[0],          // [out,opt]Output buffer
               &iBufferSize,  // [in,out] Buffer size / Size of extracted string
               ",",           // [in]     Array of chars to terminate the copy operation
               1);  // [in]     Remove additional termination chars

   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::ReportTrafficRes() - Failed to read  tite");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   iBufferSize = RT_ALERT_RES_TEXT_MAX_SIZE;
   TrafficDetecteionRes.msg[0] = 0;
   pNext       = ExtractNetworkString(
                pNext,         // [in]     Source string
                &TrafficDetecteionRes.msg[0],          // [out,opt]Output buffer
                &iBufferSize,  // [in,out] Buffer size / Size of extracted string
                ",\r\n",           // [in]     Array of chars to terminate the copy operation
                TRIM_ALL_CHARS);  // [in]     Remove additional termination chars

   if( !pNext)
   {
       roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::ReportTrafficRes() - Failed to read msg");
       (*rc) = err_parser_unexpected_data;
       return NULL;
   }

   RealtimeTrafficDetection_OnRes(&TrafficDetecteionRes);

   return pNext;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
const char* PostAlertCommentRes( /* IN  */   const char*       pNext,
                            /* IN  */   void*             pContext,
                            /* OUT */   BOOL*             more_data_needed,
                            /* OUT */   roadmap_result*   rc){
   RTAlertRes AlertRes;
   int iBufferSize;

   //   1.   Read points:
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",",       // [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &AlertRes.iPoints,          // [out]    Put it here
                              1);  // [in]     Remove additional termination CHARS

   if( !pNext )
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::PostAlertCommentRes() - Failed to read  Points");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   iBufferSize = RT_ALERT_RES_TITLE_MAX_SIZE;
   AlertRes.title[0] = 0;
   pNext       = ExtractNetworkString(
               pNext,         // [in]     Source string
               &AlertRes.title[0],          // [out,opt]Output buffer
               &iBufferSize,  // [in,out] Buffer size / Size of extracted string
               ",",           // [in]     Array of chars to terminate the copy operation
               1);  // [in]     Remove additional termination chars

   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::PostAlertCommentRes() - Failed to read  tite");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   iBufferSize = RT_ALERT_RES_TEXT_MAX_SIZE;
   AlertRes.msg[0] = 0;
   pNext       = ExtractNetworkString(
                pNext,         // [in]     Source string
                &AlertRes.msg[0],          // [out,opt]Output buffer
                &iBufferSize,  // [in,out] Buffer size / Size of extracted string
                ",\r\n",           // [in]     Array of chars to terminate the copy operation
                TRIM_ALL_CHARS);  // [in]     Remove additional termination chars

   if( !pNext)
   {
       roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::PostAlertCommentRes() - Failed to read msg");
       (*rc) = err_parser_unexpected_data;
       return NULL;
   }

   if (AlertRes.msg[0] != 0){
      if (AlertRes.iPoints > 0)
         roadmap_messagebox_timeout(AlertRes.title, AlertRes.msg, 5);
      else
         roadmap_messagebox(AlertRes.title, AlertRes.msg);
   }

   if (AlertRes.iPoints > 0){
      editor_points_add_new_points(AlertRes.iPoints);
      editor_points_display_new_points_timed(AlertRes.iPoints, 5, comment_event);
   }
   return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char* ThumbsUpRes( /* IN  */   const char*       pNext,
                         /* IN  */   void*             pContext,
                         /* OUT */   BOOL*             more_data_needed,
                         /* OUT */   roadmap_result*   rc){
   RTAlertRes AlertRes;
   int iBufferSize;

   //   1.   Read points:
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",",       // [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &AlertRes.iPoints,          // [out]    Put it here
                              1);  // [in]     Remove additional termination CHARS

   if( !pNext )
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::ThumbsUpRes() - Failed to read  Points");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   iBufferSize = RT_ALERT_RES_TITLE_MAX_SIZE;
   AlertRes.title[0] = 0;
   pNext       = ExtractNetworkString(
               pNext,         // [in]     Source string
               &AlertRes.title[0],          // [out,opt]Output buffer
               &iBufferSize,  // [in,out] Buffer size / Size of extracted string
               ",",           // [in]     Array of chars to terminate the copy operation
               1);  // [in]     Remove additional termination chars

   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::ThumbsUpRes() - Failed to read  tite");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   iBufferSize = RT_ALERT_RES_TEXT_MAX_SIZE;
   AlertRes.msg[0] = 0;
   pNext       = ExtractNetworkString(
                pNext,         // [in]     Source string
                &AlertRes.msg[0],          // [out,opt]Output buffer
                &iBufferSize,  // [in,out] Buffer size / Size of extracted string
                ",\r\n",           // [in]     Array of chars to terminate the copy operation
                TRIM_ALL_CHARS);  // [in]     Remove additional termination chars

   if( !pNext)
   {
       roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::ThumbsUpRes() - Failed to read msg");
       (*rc) = err_parser_unexpected_data;
       return NULL;
   }

   if (AlertRes.iPoints > 0){
      editor_points_add_new_points(AlertRes.iPoints);
      editor_points_display_new_points_timed(AlertRes.iPoints, 5, thumbs_up_event);
   }

   return pNext;
}



//////////////////////////////////////////////////////////////////////////////////////////////////
const char* UpdateInboxCount(/* IN  */   const char*       pNext,
                             /* IN  */   void*             pContext,
                             /* OUT */   BOOL*             more_data_needed,
                             /* OUT */   roadmap_result*   rc)
{
   LPRTConnectionInfo   pCI = (LPRTConnectionInfo)pContext;

   //   Count
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",\r\n",       // [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &pCI->iInboxCount,          // [out]    Put it here
                              TRIM_ALL_CHARS);  // [in]     Remove additional termination CHARS

   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::UpdateInboxCount() - Failed to read  count");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   (*rc) = succeeded;

   return pNext;

}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char* UpdateAlert(/* IN  */   const char*       pNext,
                        /* IN  */   void*             pContext,
                        /* OUT */   BOOL*             more_data_needed,
                        /* OUT */   roadmap_result*   rc)
{
   int   iID;
   int   iNumThumbsUp;
   BOOL  bIsOnRoute;
   BOOL  bIsArchive;
   int   iBufferSize;
   char  tempBuff[5];
   int   iNumViewed;

   //   Alert ID
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",",       // [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &iID,          // [out]    Put it here
                              1);  // [in]     Remove additional termination CHARS

   if( !pNext || (-1 == iID))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::UpdateAlert() - Failed to read  ID");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   Number of Thumbs up
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",",       // [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &iNumThumbsUp,          // [out]    Put it here
                              1);  // [in]     Remove additional termination CHARS

   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::UpdateAlert() - Failed to read  Number of thumbs up");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //    Is On route
   iBufferSize = sizeof(tempBuff);
   tempBuff[0] = 0;
   pNext       = ExtractNetworkString(
                    pNext,             // [in]     Source string
                    tempBuff,//   [out]   Output buffer
                    &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                    ",",          //   [in]   Array of chars to terminate the copy operation
                    1);   // [in]     Remove additional termination chars

    if( !pNext)
    {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::UpdateAlert() - Failed to read isOnRoute flag id=%d", iID);
        (*rc) = err_parser_unexpected_data;
        return NULL;
    }

    if (tempBuff[0] == 'T')
       bIsOnRoute = TRUE;
    else
       bIsOnRoute = FALSE;

   //    Is archive
   iBufferSize = sizeof(tempBuff);
   tempBuff[0] = 0;
   pNext       = ExtractNetworkString(
                    pNext,             // [in]     Source string
                    tempBuff,//   [out]   Output buffer
                    &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                    ",\r\n",          //   [in]   Array of chars to terminate the copy operation
                    TRIM_ALL_CHARS);   // [in]     Remove additional termination chars

    if( !pNext)
    {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::UpdateAlert() - Failed to read isArchive flag id=%d", iID);
        (*rc) = err_parser_unexpected_data;
        return NULL;
    }

    if (tempBuff[0] == 'T')
       bIsArchive = TRUE;
    else
       bIsArchive = FALSE;

    	// num viewed
    	pNext = ReadIntFromString(pNext,          					//   [in]      Source string
                                 ",\r\n",           					//   [in,opt]  Value termination
                                 NULL,          					//   [in,opt]  Allowed padding
                                 &iNumViewed,	//   [out]     Output value
                                 TRIM_ALL_CHARS);            					//   [in]      TRIM_ALL_CHARS, DO_NOT_TRIM, or 'n'

    	if (!pNext) {
    		roadmap_log (ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddAlert() - Failed to read num viewed");
    		return NULL;
    	}

    //TEMP_AVI
   //   Update the alert
   RTAlerts_Update(iID, iNumThumbsUp, bIsOnRoute, bIsArchive, iNumViewed);

   return pNext;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
const char* ThumbsUpReceived(  /* IN  */   const char*       pNext,
                              /* IN  */   void*             pContext,
                              /* OUT */   BOOL*             more_data_needed,
                              /* OUT */   roadmap_result*   rc)
{
   int                  iBufferSize;
   char                 tempBuff[5];
   ThumbsUp             thumbsUp;

   RTAlerts_ThumbsUpRecordInit(&thumbsUp);
   //   ID
   pNext = ReadIntFromString(
                  pNext,         //   [in]      Source string
                  ",",           //   [in,opt]   Value termination
                  NULL,          //   [in,opt]   Allowed padding
                  &thumbsUp.iID,  //   [out]      Put it here
                  1);  //   [in]      Remove additional termination CHARS

   if( !pNext || (RT_INVALID_LOGINID_VALUE == thumbsUp.iID))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::ThumbsUpReceived() - Failed to read alert ID");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }


   //  User ID
   iBufferSize = RT_ALERT_USERNM_MAXSIZE;
   pNext = ExtractNetworkString(
                 pNext,               //   [in]   Source string
                 thumbsUp.sUserID,  //   [out]   Output buffer
                 &iBufferSize,
                                       //   [in]   Output buffer size
                 ",",                 //   [in]   Array of chars to terminate the copy operation
                 1);                  //   [in]   Remove additional termination chars

   if( !pNext)
   {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::ThumbsUpReceived() - Failed to read user ID");
        (*rc) = err_parser_unexpected_data;
        return NULL;
   }


   //  NickName
   iBufferSize = RT_ALERT_USERNM_MAXSIZE;
   pNext = ExtractNetworkString(
                  pNext,               //   [in]   Source string
                  thumbsUp.sNickName,  //   [out]   Output buffer
                  &iBufferSize,
                                        //   [in]   Output buffer size
                  ",",                 //   [in]   Array of chars to terminate the copy operation
                  1);                  //   [in]   Remove additional termination chars

   if( !pNext)
   {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::ThumbsUpReceived() - Failed to read user nick name");
        (*rc) = err_parser_unexpected_data;
        return NULL;
   }

   //  Facebook Name
   iBufferSize = RT_ALERT_FACEBOOK_USERNM_MAXSIZE;
   pNext = ExtractNetworkString(
                  pNext,               //   [in]   Source string
                  thumbsUp.sFacebookName,  //   [out]   Output buffer
                  &iBufferSize,
                                        //   [in]   Output buffer size
                  ",",                 //   [in]   Array of chars to terminate the copy operation
                  1);                  //   [in]   Remove additional termination chars

   if( !pNext)
   {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::ThumbsUpReceived() - Failed to read user facebook name");
        (*rc) = err_parser_unexpected_data;
        return NULL;
   }

   //  Show Facebook Image
   iBufferSize = sizeof(tempBuff);
   tempBuff[0] = 0;
   pNext       = ExtractNetworkString(
                    pNext,             // [in]     Source string
                    tempBuff,//   [out]   Output buffer
                    &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                    ",\r\n",          //   [in]   Array of chars to terminate the copy operation
                    TRIM_ALL_CHARS);   // [in]     Remove additional termination chars

    if( !pNext)
    {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::ThumbsUpReceived() - Failed to read bShowFacebookPicture flag id=%d", thumbsUp.iID);
        (*rc) = err_parser_unexpected_data;
        return NULL;
    }

    if (tempBuff[0] == 'T')
       thumbsUp.bShowFacebookPicture = TRUE;
    else
       thumbsUp.bShowFacebookPicture = FALSE;


    RTAlerts_ThumbsUpReceived(&thumbsUp);

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
   char     bonusTitle[256];
   char     bonusTitleColor[20];
   char     bonusTxt[256];
   char     bonusTxtColor[20];
   char     webExtraParams[256];
   char     tempBuff[10];
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

   if( !pNext)
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

    if( !pNext)
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
     if (str[0])
        bonus.pIconName = &str[0];

     //   8.   Type
     pNext = ReadIntFromString(
                          pNext,         //   [in]      Source string
                          ",",           //   [in,opt]   Value termination
                          NULL,          //   [in,opt]   Allowed padding
                          &bonus.iType,    //   [out]      Put it here
                          1);  //   [in]      Remove additional termination CHARS

     if( !pNext)
     {
              roadmap_log( ROADMAP_ERROR, "RTNet::AddBonus() - Failed to read type, using type bonus points");
     }

    bonusTitle[0] = 0;
    iBufferSize = sizeof(bonusTitle);
    pNext       = ExtractNetworkString(
                      pNext,               // [in]     Source string
                      &bonusTitle[0],  // [out,opt]Output buffer
                      &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                      ",",                 // [in]     Array of chars to terminate the copy operation
                      1);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
             roadmap_log( ROADMAP_ERROR, "RTNet::AddBonus - Failed to read bonus Title");
             (*rc) = err_parser_unexpected_data;
             return NULL;
    }
    if (bonusTitle[0])
       bonus.pBonusTitle = &bonusTitle[0];

    bonusTitleColor[0] = 0;
    iBufferSize = sizeof(bonusTitleColor);
    pNext       = ExtractNetworkString(
                      pNext,               // [in]     Source string
                      &bonusTitleColor[0],  // [out,opt]Output buffer
                      &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                      ",",                 // [in]     Array of chars to terminate the copy operation
                      1);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
             roadmap_log( ROADMAP_ERROR, "RTNet::AddBonus - Failed to read bonus Title color");
             (*rc) = err_parser_unexpected_data;
             return NULL;
    }
    if(bonusTitleColor[0])
       bonus.pBonusTitleColor = &bonusTitleColor[0];

    bonusTxt[0] = 0;
    iBufferSize = sizeof(bonusTxt);
    pNext       = ExtractNetworkString(
                      pNext,               // [in]     Source string
                      &bonusTxt[0],  // [out,opt]Output buffer
                      &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                      ",",                 // [in]     Array of chars to terminate the copy operation
                      1);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
             roadmap_log( ROADMAP_ERROR, "RTNet::AddBonus - Failed to read bonus Text");
             (*rc) = err_parser_unexpected_data;
             return NULL;
    }
    if (bonusTxt[0])
       bonus.pBonusText = &bonusTxt[0];

    bonusTxtColor[0] = 0;
    iBufferSize = sizeof(bonusTxtColor);
    pNext       = ExtractNetworkString(
                      pNext,               // [in]     Source string
                      &bonusTxtColor[0],  // [out,opt]Output buffer
                      &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                      ",",                 // [in]     Array of chars to terminate the copy operation
                      1);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
             roadmap_log( ROADMAP_ERROR, "RTNet::AddBonus - Failed to read bonus Text colro");
             (*rc) = err_parser_unexpected_data;
             return NULL;
    }
    if (bonusTxtColor[0])
       bonus.pBonusTextColor = &bonusTxtColor[0];

    tempBuff[0] = 0;
    iBufferSize = sizeof(tempBuff);
    pNext       = ExtractNetworkString(
                pNext,               // [in]     Source string
                &tempBuff[0],  // [out,opt]Output buffer
                &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                ",",                 // [in]     Array of chars to terminate the copy operation
                1);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
          roadmap_log( ROADMAP_ERROR, "RTNet::AddBonusTemplate - Failed to read HasWebDlg param");
          (*rc) = err_parser_unexpected_data;
          return NULL;
    }
    if (tempBuff[0] == 'T')
       bonus.bHasWebDlg = TRUE;
    else
       bonus.bHasWebDlg = FALSE;
    //Web Extra params
    webExtraParams[0] = 0;
    iBufferSize = sizeof(webExtraParams);
    pNext       = ExtractNetworkString(
                pNext,               // [in]     Source string
                &webExtraParams[0],  // [out,opt]Output buffer
                &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                ",",                 // [in]     Array of chars to terminate the copy operation
                1);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
          roadmap_log( ROADMAP_ERROR, "RTNet::AddBonusTemplate - Failed to read Web Extra params");
          (*rc) = err_parser_unexpected_data;
          return NULL;
    }
    if (webExtraParams[0])
       bonus.pWebDlgExtraParams = &webExtraParams[0];

    //Template ID
    pNext = ReadIntFromString(
                      pNext,         //   [in]      Source string
                      ",\r\n",           //   [in,opt]   Value termination
                      NULL,          //   [in,opt]   Allowed padding
                      &bonus.iTemplateID,    //   [out]      Put it here
                      TRIM_ALL_CHARS);  //   [in]      Remove additional termination CHARS

    if( !pNext)
    {
        roadmap_log( ROADMAP_ERROR, "RTNet::AddBonus() - Failed to read type, using TemplateID");
        (*rc) = err_parser_unexpected_data;
        return NULL;
    }

    bonus.bIsCustomeBonus = FALSE;

    RealtimeBonus_Add(&bonus);

    return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char *AddBonusTemplate (/* IN  */   const char*       pNext,
               /* IN  */   void*             pContext,
               /* OUT */   BOOL*             more_data_needed,
               /* OUT */   roadmap_result*   rc)
{
   RTBonusTemplate template;
   int      iBufferSize;
   char     iconName[256];
   char     bonusTitle[256];
   char     bonusTitleColor[20];
   char     bonusTxt[256];
   char     bonusTxtColor[20];
   char     webExtraParams[256];

   RealtimeBonus_BonusTemplate_Init(&template);


   // ID
   pNext = ReadIntFromString(
                    pNext,         //   [in]      Source string
                    ",",           //   [in,opt]   Value termination
                    NULL,          //   [in,opt]   Allowed padding
                    &template.iID,    //   [out]      Put it here
                    1);  //   [in]      Remove additional termination CHARS

    if( !pNext || (-1 == template.iID))
    {
        roadmap_log( ROADMAP_ERROR, "RTNet::AddBonusTemplate() - Failed to read  ID");
        (*rc) = err_parser_unexpected_data;
        return NULL;
    }

    // Radius
    pNext = ReadIntFromString(
                        pNext,         //   [in]      Source string
                        ",",           //   [in,opt]   Value termination
                        NULL,          //   [in,opt]   Allowed padding
                        &template.iRadius,    //   [out]      Put it here
                        1);  //   [in]      Remove additional termination CHARS

    if( !pNext)
    {
            roadmap_log( ROADMAP_ERROR, "RTNet::AddBonusTemplate() - Failed to read radius");
            (*rc) = err_parser_unexpected_data;
            return NULL;
    }

    //   5.   Read points:
    pNext = ReadIntFromString(
                         pNext,         //   [in]      Source string
                         ",",           //   [in,opt]   Value termination
                         NULL,          //   [in,opt]   Allowed padding
                         &template.iNumPoints,    //   [out]      Put it here
                         1);  //   [in]      Remove additional termination CHARS

    if( !pNext)
    {
             roadmap_log( ROADMAP_ERROR, "RTNet::AddBonusTemplate() - Failed to read number of points");
             (*rc) = err_parser_unexpected_data;
             return NULL;
    }

    // Icon
    iconName[0] = 0;
    iBufferSize = sizeof(iconName);
    pNext       = ExtractNetworkString(
                      pNext,               // [in]     Source string
                      &iconName[0],  // [out,opt]Output buffer
                      &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                      ",",                 // [in]     Array of chars to terminate the copy operation
                      1);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
             roadmap_log( ROADMAP_ERROR, "RTNet::AddBonusTemplate - Failed to read Icon Name");
             (*rc) = err_parser_unexpected_data;
             return NULL;
    }
    if (iconName[0])
       template.pIconName = &iconName[0];

    //Title
    bonusTitle[0] = 0;
    iBufferSize = sizeof(bonusTitle);
    pNext       = ExtractNetworkString(
                      pNext,               // [in]     Source string
                      &bonusTitle[0],  // [out,opt]Output buffer
                      &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                      ",",                 // [in]     Array of chars to terminate the copy operation
                      1);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
             roadmap_log( ROADMAP_ERROR, "RTNet::AddBonusTemplate - Failed to read title");
             (*rc) = err_parser_unexpected_data;
             return NULL;
    }
    if (bonusTitle[0])
       template.pTitle = &bonusTitle[0];

    //Title Color
    bonusTitleColor[0] = 0;
    iBufferSize = sizeof(bonusTitleColor);
    pNext       = ExtractNetworkString(
                      pNext,               // [in]     Source string
                      &bonusTitleColor[0],  // [out,opt]Output buffer
                      &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                      ",",                 // [in]     Array of chars to terminate the copy operation
                      1);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
             roadmap_log( ROADMAP_ERROR, "RTNet::AddBonusTemplate - Failed to read title color");
             (*rc) = err_parser_unexpected_data;
             return NULL;
    }
    if (bonusTitleColor[0])
       template.pTitleColor = &bonusTitleColor[0];

    //Text
    bonusTxt[0] = 0;
    iBufferSize = sizeof(bonusTxt);
    pNext       = ExtractNetworkString(
                      pNext,               // [in]     Source string
                      &bonusTxt[0],  // [out,opt]Output buffer
                      &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                      ",",                 // [in]     Array of chars to terminate the copy operation
                      1);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
             roadmap_log( ROADMAP_ERROR, "RTNet::AddBonusTemplate - Failed to read Txt");
             (*rc) = err_parser_unexpected_data;
             return NULL;
    }

    if(bonusTxt[0])
       template.pText = &bonusTxt[0];

    //Text Color
    bonusTxtColor[0] = 0;
    iBufferSize = sizeof(bonusTxtColor);
    pNext       = ExtractNetworkString(
                       pNext,               // [in]     Source string
                       &bonusTxtColor[0],  // [out,opt]Output buffer
                       &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                       ",",                 // [in]     Array of chars to terminate the copy operation
                       1);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
              roadmap_log( ROADMAP_ERROR, "RTNet::AddBonusTemplate - Failed to read Txt color");
              (*rc) = err_parser_unexpected_data;
              return NULL;
    }
    if (bonusTxtColor[0])
       template.pTextColor = &bonusTxtColor[0];


    //Web Extra params
    webExtraParams[0] = 0;
    iBufferSize = sizeof(webExtraParams);
    pNext       = ExtractNetworkString(
                      pNext,               // [in]     Source string
                      &webExtraParams[0],  // [out,opt]Output buffer
                      &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                      ",\r\n",                 // [in]     Array of chars to terminate the copy operation
                      TRIM_ALL_CHARS);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
             roadmap_log( ROADMAP_ERROR, "RTNet::AddBonusTemplate - Failed to read Web Extra params");
             (*rc) = err_parser_unexpected_data;
             return NULL;
    }
    if (webExtraParams[0])
       template.pWebDlgExtraParams = &webExtraParams[0];


    RealtimeBonus_BonusTemplate_Add(&template);
    return pNext;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
const char *AddCustomBonus (/* IN  */   const char*       pNext,
               /* IN  */   void*             pContext,
               /* OUT */   BOOL*             more_data_needed,
               /* OUT */   roadmap_result*   rc)
{
   RTBonus  bonus;
   char     bonusTxt[128];
   char     str[128];
   char     CollectText[256];
   char     CollectTitle[256];
   char     CollectIcon[256];
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
       roadmap_log( ROADMAP_ERROR, "RTNet::AddCustomBonus() - Failed to read  ID");
       (*rc) = err_parser_unexpected_data;
       return NULL;
   }


   //  2 BonusText
   bonusTxt[0] = 0;
   iBufferSize = sizeof(bonusTxt);
   pNext       = ExtractNetworkString(
                     pNext,               // [in]     Source string
                     &bonusTxt[0],  // [out,opt]Output buffer
                     &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                     ",",                 // [in]     Array of chars to terminate the copy operation
                     1);                  // [in]     Remove additional termination chars

   if( !pNext)
   {
            roadmap_log( ROADMAP_ERROR, "RTNet::AddCustomBonus - Failed to read bonus Text");
            (*rc) = err_parser_unexpected_data;
            return NULL;
   }
   bonus.pBonusText = &bonusTxt[0];

   //   3.   Read points:
   pNext = ReadIntFromString(
                       pNext,         //   [in]      Source string
                       ",",           //   [in,opt]   Value termination
                       NULL,          //   [in,opt]   Allowed padding
                       &bonus.iNumPoints,    //   [out]      Put it here
                       1);  //   [in]      Remove additional termination CHARS

   if( !pNext || (-1 == bonus.iNumPoints))
   {
           roadmap_log( ROADMAP_ERROR, "RTNet::AddCustomBonus() - Failed to read number of points");
           (*rc) = err_parser_unexpected_data;
           return NULL;
    }

    //  4. Icon name
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
              roadmap_log( ROADMAP_ERROR, "RTNet::AddCustomBonus - Failed to read Icon Name");
              (*rc) = err_parser_unexpected_data;
              return NULL;
    }
    bonus.pIconName = &str[0];

    //  The collect text
    CollectText[0] = 0;
    iBufferSize = sizeof(CollectText);
    pNext       = ExtractNetworkString(
                          pNext,               // [in]     Source string
                          &CollectText[0],  // [out,opt]Output buffer
                          &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                          ",",                 // [in]     Array of chars to terminate the copy operation
                          1);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
        roadmap_log( ROADMAP_ERROR, "RTNet::OpenMessageTicker - Failed to read Success Text");
        (*rc) = err_parser_unexpected_data;
        return NULL;
    }
    bonus.pCollectText = &CollectText[0];

    //  The collect title
    CollectTitle[0] = 0;
    iBufferSize = sizeof(CollectTitle);
    pNext       = ExtractNetworkString(
                          pNext,               // [in]     Source string
                          &CollectTitle[0],  // [out,opt]Output buffer
                          &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                          ",",                 // [in]     Array of chars to terminate the copy operation
                          1);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
        roadmap_log( ROADMAP_ERROR, "RTNet::OpenMessageTicker - Failed to read title");
        (*rc) = err_parser_unexpected_data;
        return NULL;
    }
    bonus.pCollectTitle = &CollectTitle[0];
    //  The collect icon
    CollectIcon[0] = 0;
    iBufferSize = sizeof(CollectIcon);
    pNext       = ExtractNetworkString(
                           pNext,               // [in]     Source string
                           &CollectIcon[0],  // [out,opt]Output buffer
                           &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                           ",\r\n",                 // [in]     Array of chars to terminate the copy operation
                           TRIM_ALL_CHARS);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
         roadmap_log( ROADMAP_ERROR, "RTNet::OpenMessageTicker - Failed to read icon");
         (*rc) = err_parser_unexpected_data;
         return NULL;
    }
    bonus.pCollectIcon = &CollectIcon[0];

    bonus.bIsCustomeBonus = TRUE;

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
   char  Title[256];
   char  Msg[256];
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

    if( !pNext)
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

    if( !pNext)
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
                         ",",                 // [in]     Array of chars to terminate the copy operation
                         1);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
       roadmap_log( ROADMAP_ERROR, "RTNet::AddBonus - Failed to read Gift");
       (*rc) = err_parser_unexpected_data;
       return NULL;
    }

    //  Title
    Title[0] = 0;
    iBufferSize = sizeof(Title);
    pNext       = ExtractNetworkString(
                          pNext,               // [in]     Source string
                          &Title[0],  // [out,opt]Output buffer
                          &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                          ",",                 // [in]     Array of chars to terminate the copy operation
                          1);                  // [in]     Remove additional termination chars

     if( !pNext)
     {
        roadmap_log( ROADMAP_ERROR, "RTNet::AddBonus - Failed to read Title");
        (*rc) = err_parser_unexpected_data;
        return NULL;
     }

     //  Msg
     Msg[0] = 0;
      iBufferSize = sizeof(Msg);
      pNext       = ExtractNetworkString(
                           pNext,               // [in]     Source string
                           &Msg[0],  // [out,opt]Output buffer
                           &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                           ",\r\n",                 // [in]     Array of chars to terminate the copy operation
                           TRIM_ALL_CHARS);                  // [in]     Remove additional termination chars

      if( !pNext)
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::AddBonus - Failed to read Msg");
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }


      RealtimeBonus_CollectedPointsConfirmed(iID, iType, iPoints, bHasGift, bIsBigGift, buff, Title, Msg);

    return pNext;
}

const char *UserGroups (/* IN  */   const char*       pNext,
                        /* IN  */   void*             pContext,
                        /* OUT */   BOOL*             more_data_needed,
                        /* OUT */   roadmap_result*   rc)
{
   int   iBufferSize;
   int   num_additional_followed_groups;
   int   i;
   int   total_groups = 0;
   char  name[GROUP_NAME_MAX_SIZE];
   char  icon[GROUP_ICON_MAX_SIZE];


   //   1.   Active group name
   name[0] = 0;
   if( ',' == (*pNext))
   {
      pNext++;   //   Jump over the comma (,)
   }
   else
   {
      //   Read Active group name
      iBufferSize = sizeof(name);
      pNext       = ExtractNetworkString(
                  pNext,           // [in]     Source string
                  name,        // [out,opt]Output buffer
                  &iBufferSize,    // [in,out] Buffer size / Size of extracted string
                  ",",             // [in]     Array of chars to terminate the copy operation
                  1); // [in]     Remove additional termination chars

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - active group name is empty");
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
   }

   if (name[0] != 0)
      total_groups = 1;
   roadmap_groups_set_active_group_name(name);

   icon[0] = 0;
   //   2.   Active group icon
   if( ',' == (*pNext))
   {
      roadmap_log( ROADMAP_DEBUG, "RTNet::OnGeneralResponse::UserGroups() - failed to read active group icon...");
      pNext++;   //   Jump over the comma (,)
   }
   else
   {
      //   Read the Active group icon
      iBufferSize = sizeof(icon);
      pNext       = ExtractNetworkString(
                  pNext,           // [in]     Source string
                  icon,        // [out,opt]Output buffer
                  &iBufferSize,    // [in,out] Buffer size / Size of extracted string
                  ",",             // [in]     Array of chars to terminate the copy operation
                  1); // [in]     Remove additional termination chars

      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - failed to read active group icon");
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
   }

   roadmap_groups_set_active_group_icon(icon);

   //   Number of additional followed groups
   pNext = ReadIntFromString( pNext,         // [in]     Source string
                              ",\r\n",       // [in,opt] Value termination
                              NULL,          // [in,opt] Allowed padding
                              &num_additional_followed_groups,          // [out]    Put it here
                              1);  // [in]     Remove additional termination CHARS

   if( !pNext)
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::CollectBonusRes - Failed to read Number of additional followed groups");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   total_groups += num_additional_followed_groups;

   for (i = 0; i < num_additional_followed_groups; i++){
      iBufferSize = sizeof(name);
      pNext       = ExtractNetworkString(
                      pNext,           // [in]     Source string
                      name,        // [out,opt]Output buffer
                      &iBufferSize,    // [in,out] Buffer size / Size of extracted string
                      ",",             // [in]     Array of chars to terminate the copy operation
                      1); // [in]     Remove additional termination chars
      if( !pNext || !(*pNext))
      {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read followed group name  (%d out of %d)", i, num_additional_followed_groups);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
      roadmap_groups_add_following_group_name(i, name);

      iBufferSize = sizeof(icon);
      pNext       = ExtractNetworkString(
                          pNext,           // [in]     Source string
                          icon,        // [out,opt]Output buffer
                          &iBufferSize,    // [in,out] Buffer size / Size of extracted string
                          ",\r\n",             // [in]     Array of chars to terminate the copy operation
                          1); // [in]     Remove additional termination chars

      if( !pNext)
      {
          roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddUser() - Failed to read followed group icon  (%d out of %d)", i, num_additional_followed_groups);
          (*rc) = err_parser_unexpected_data;
          return NULL;
      }
      roadmap_groups_add_following_group_icon(i, icon);
   }

   roadmap_groups_set_num_following(total_groups);
   return pNext;

}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char *OpenMoodSelection (/* IN  */   const char*       pNext,
               /* IN  */   void*             pContext,
               /* OUT */   BOOL*             more_data_needed,
               /* OUT */   roadmap_result*   rc){

      int dummy;
      pNext = ReadIntFromString( pNext,         // [in]     Source string
                                 ",\r\n",       // [in,opt] Value termination
                                 NULL,          // [in,opt] Allowed padding
                                 &dummy,          // [out]    Put it here
                                 TRIM_ALL_CHARS);  // [in]     Remove additional termination CHARS

      roadmap_log( ROADMAP_INFO, "OpenMoodSelection - Normal moods are now open");
      Realtime_SetIsNewbie(FALSE);
      return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char *OpenMessageTicker (/* IN  */   const char*       pNext,
               /* IN  */   void*             pContext,
               /* OUT */   BOOL*             more_data_needed,
               /* OUT */   roadmap_result*   rc)
{
   int   iPoints;
   char  text[256];
   char  title[256];
   char  icon[256];
   int   iBufferSize;

    //   Read points:
    pNext = ReadIntFromString( pNext,         // [in]     Source string
                               ",",       // [in,opt] Value termination
                               NULL,          // [in,opt] Allowed padding
                               &iPoints,          // [out]    Put it here
                               1);  // [in]     Remove additional termination CHARS

    if( !pNext)
    {
       roadmap_log( ROADMAP_ERROR, "RTNet::OpenMessageTicker - Failed to read points");
       (*rc) = err_parser_unexpected_data;
       return NULL;
    }

    //  The text
    text[0] = 0;
    iBufferSize = sizeof(text);
    pNext       = ExtractNetworkString(
                          pNext,               // [in]     Source string
                          &text[0],  // [out,opt]Output buffer
                          &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                          ",",                 // [in]     Array of chars to terminate the copy operation
                          1);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
        roadmap_log( ROADMAP_ERROR, "RTNet::OpenMessageTicker - Failed to read Success Text");
        (*rc) = err_parser_unexpected_data;
        return NULL;
    }

    //  The title
    title[0] = 0;
    iBufferSize = sizeof(title);
    pNext       = ExtractNetworkString(
                          pNext,               // [in]     Source string
                          &title[0],  // [out,opt]Output buffer
                          &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                          ",",                 // [in]     Array of chars to terminate the copy operation
                          1);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
        roadmap_log( ROADMAP_ERROR, "RTNet::OpenMessageTicker - Failed to read title");
        (*rc) = err_parser_unexpected_data;
        return NULL;
    }

    //  The icon
    icon[0] = 0;
    iBufferSize = sizeof(icon);
    pNext       = ExtractNetworkString(
                           pNext,               // [in]     Source string
                           &icon[0],  // [out,opt]Output buffer
                           &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                           ",\r\n",                 // [in]     Array of chars to terminate the copy operation
                           TRIM_ALL_CHARS);                  // [in]     Remove additional termination chars

    if( !pNext)
    {
         roadmap_log( ROADMAP_ERROR, "RTNet::OpenMessageTicker - Failed to read icon");
         (*rc) = err_parser_unexpected_data;
         return NULL;
    }
    RealtimeBonus_OpenMessageTicker(iPoints, text, title, icon);

    return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char *AddExternalPoiType (/* IN  */   const char*       pNext,
               /* IN  */   void*             pContext,
               /* OUT */   BOOL*             more_data_needed,
               /* OUT */   roadmap_result*   rc)
{
   RTExternalPoiType externalPoiType;
   int iBufferSize;

   RealtimeExternalPoi_ExternalPoiType_Init(&externalPoiType);

   //   Read Alert ID:
    pNext = ReadIntFromString(
                   pNext,         //   [in]      Source string
                   ",",           //   [in,opt]   Value termination
                   NULL,          //   [in,opt]   Allowed padding
                   &externalPoiType.iID,    //   [out]      Put it here
                   1);  //   [in]      Remove additional termination CHARS

    if( !pNext || !(*pNext))
    {
       roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoiType() - Failed to read  ID");
       (*rc) = err_parser_unexpected_data;
       return NULL;
    }

    //   Service ID
    pNext = ReadIntFromString(
                     pNext,         //   [in]      Source string
                     ",",           //   [in,opt]   Value termination
                     NULL,          //   [in,opt]   Allowed padding
                     &externalPoiType.iExternalPoiServiceID,    //   [out]      Put it here
                     1);  //   [in]      Remove additional termination CHARS

    if( !pNext || !(*pNext))
    {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoiType() - Failed to read  Service ID");
         (*rc) = err_parser_unexpected_data;
         return NULL;
    }

    //    Providor ID
    pNext = ReadIntFromString(
                     pNext,         //   [in]      Source string
                     ",",           //   [in,opt]   Value termination
                     NULL,          //   [in,opt]   Allowed padding
                     &externalPoiType.iExternalPoiProviderID,    //   [out]      Put it here
                     1);  //   [in]      Remove additional termination CHARS

    if( !pNext || !(*pNext))
    {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoiType() - Failed to read  Providor ID");
         (*rc) = err_parser_unexpected_data;
         return NULL;
    }

    //   Read small icon:
    iBufferSize = sizeof(externalPoiType.cSmallIcon);
    pNext       = ExtractNetworkString(
                pNext,               // [in]     Source string
                &externalPoiType.cSmallIcon[0],         // [out,opt]Output buffer
                &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                ",",                 // [in]     Array of chars to terminate the copy operation
                1);     // [in]     Remove additional termination chars

    if( !pNext || !(*pNext))
    {
       roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoiType() - Failed to read small icon.");
       (*rc) = err_parser_unexpected_data;
       return NULL;
    }

    //   Read big icon:
    iBufferSize = sizeof(externalPoiType.cBigIcon);
    pNext       = ExtractNetworkString(
                pNext,               // [in]     Source string
                &externalPoiType.cBigIcon[0],         // [out,opt]Output buffer
                &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                ",",                 // [in]     Array of chars to terminate the copy operation
                1);     // [in]     Remove additional termination chars

    if( !pNext || !(*pNext))
    {
       roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoiType() - Failed to read big icon.");
       (*rc) = err_parser_unexpected_data;
       return NULL;
    }

    //   Read cOnClickUrl :
    iBufferSize = sizeof(externalPoiType.cOnClickUrl);
    pNext       = ExtractNetworkString(
                pNext,               // [in]     Source string
                &externalPoiType.cOnClickUrl[0],         // [out,opt]Output buffer
                &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                ",",                 // [in]     Array of chars to terminate the copy operation
                1);     // [in]     Remove additional termination chars

    if( !pNext || !(*pNext))
    {
       roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoiType() - Failed to read OnClickUrl.");
       (*rc) = err_parser_unexpected_data;
       return NULL;
    }

    // Size
     pNext = ReadIntFromString(
                      pNext,         //   [in]      Source string
                      ",",           //   [in,opt]   Value termination
                      NULL,          //   [in,opt]   Allowed padding
                      &externalPoiType.iSize,    //   [out]      Put it here
                      1);  //   [in]      Remove additional termination CHARS

     if( !pNext || !(*pNext))
     {
          roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoiType() - Failed to read  Size");
          (*rc) = err_parser_unexpected_data;
          return NULL;
     }

     // Small icon zoom
     pNext = ReadIntFromString(
                       pNext,         //   [in]      Source string
                       ",",           //   [in,opt]   Value termination
                       NULL,          //   [in,opt]   Allowed padding
                       &externalPoiType.iMaxDisplayZoomSmallIcon,    //   [out]      Put it here
                       1);  //   [in]      Remove additional termination CHARS

     if( !pNext || !(*pNext))
     {
           roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoiType() - Failed to read Max Display Zoom small Icon");
           (*rc) = err_parser_unexpected_data;
           return NULL;
     }

     //Max Display Zoom - Big Icon
     pNext = ReadIntFromString(
                       pNext,         //   [in]      Source string
                       ",",           //   [in,opt]   Value termination
                       NULL,          //   [in,opt]   Allowed padding
                       &externalPoiType.iMaxDisplayZoomBigIcon,    //   [out]      Put it here
                       1);  //   [in]      Remove additional termination CHARS

     if( !pNext || !(*pNext))
     {
           roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoiType() - Failed to read Max Display Zoom big Icon");
           (*rc) = err_parser_unexpected_data;
           return NULL;
     }


     //  Promotion Type
     pNext = ReadIntFromString(
                        pNext,         //   [in]      Source string
                        ",",           //   [in,opt]   Value termination
                        NULL,          //   [in,opt]   Allowed padding
                        &externalPoiType.iPromotionType,    //   [out]      Put it here
                        1);  //   [in]      Remove additional termination CHARS

     if(!pNext)
     {
            roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoiType() - Failed to read  Promotion Type");
            (*rc) = err_parser_unexpected_data;
            return NULL;
     }

     //   Read small promotion icon:
     iBufferSize = sizeof(externalPoiType.cSmallPromotionIcon);
     pNext       = ExtractNetworkString(
                 pNext,               // [in]     Source string
                 &externalPoiType.cSmallPromotionIcon[0],         // [out,opt]Output buffer
                 &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                 ",",                 // [in]     Array of chars to terminate the copy operation
                 1);     // [in]     Remove additional termination chars

     if( !pNext || !(*pNext))
     {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoiType() - Failed to read promotion small icon.");
        (*rc) = err_parser_unexpected_data;
        return NULL;
     }

     //   Read big promotion icon:
     iBufferSize = sizeof(externalPoiType.cBigPromotionIcon);
     pNext       = ExtractNetworkString(
                 pNext,               // [in]     Source string
                 &externalPoiType.cBigPromotionIcon[0],         // [out,opt]Output buffer
                 &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                 ",",                 // [in]     Array of chars to terminate the copy operation
                 1);     // [in]     Remove additional termination chars

     if( !pNext || !(*pNext))
     {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoiType() - Failed to read big promotion icon.");
        (*rc) = err_parser_unexpected_data;
        return NULL;
     }

      //  Promotion ID
      pNext = ReadIntFromString(
                         pNext,         //   [in]      Source string
                         ",\r\n",           //   [in,opt]   Value termination
                         NULL,          //   [in,opt]   Allowed padding
                         &externalPoiType.iPromotionID,    //   [out]      Put it here
                         1);  //   [in]      Remove additional termination CHARS

      if(!pNext)
      {
             roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoiType() - Failed to read Promotion ID");
             (*rc) = err_parser_unexpected_data;
             return NULL;
      }

      //  Promotion radius
      pNext = ReadIntFromString(
                         pNext,         //   [in]      Source string
                         ",\r\n",           //   [in,opt]   Value termination
                         NULL,          //   [in,opt]   Allowed padding
                         &externalPoiType.iPromotionRadius,    //   [out]      Put it here
                         TRIM_ALL_CHARS);  //   [in]      Remove additional termination CHARS

      if(!pNext)
      {
             roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoiType() - Failed to read Promotion radius");
             (*rc) = err_parser_unexpected_data;
             return NULL;
      }

     externalPoiType.iIsNavigable = TRUE;
     RealtimeExternalPoi_ExternalPoiType_Add(&externalPoiType);
     return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char *AddExternalPoi (/* IN  */   const char*       pNext,
               /* IN  */   void*             pContext,
               /* OUT */   BOOL*             more_data_needed,
               /* OUT */   roadmap_result*   rc)
{

   int iBufferSize;
   RTExternalPoi externalPoi;
   RealtimeExternalPoi_ExternalPoi_Init(&externalPoi);

   //   ID
   pNext = ReadIntFromString(
               pNext,            //   [in]      Source string
               ",",              //   [in,opt]   Value termination
               NULL,             //   [in,opt]   Allowed padding
               &externalPoi.iID,   //   [out]      Put it here
               1);  //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoi() - Failed to read ID");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   Server ID
   pNext = ReadIntFromString(
               pNext,            //   [in]      Source string
               ",",              //   [in,opt]   Value termination
               NULL,             //   [in,opt]   Allowed padding
               &externalPoi.iServerID,   //   [out]      Put it here
               1);  //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoi() - Failed to read Server ID");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   Type
   pNext = ReadIntFromString(
                    pNext,         //   [in]      Source string
                    ",",           //   [in,opt]   Value termination
                    NULL,          //   [in,opt]   Allowed padding
                    &externalPoi.iTypeID,    //   [out]      Put it here
                    1);  //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoi() - Failed to read  Type");
        (*rc) = err_parser_unexpected_data;
        return NULL;
   }

   //    Service ID
   pNext = ReadIntFromString(
                    pNext,         //   [in]      Source string
                    ",",           //   [in,opt]   Value termination
                    NULL,          //   [in,opt]   Allowed padding
                    &externalPoi.iServiceID,    //   [out]      Put it here
                    1);  //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoi() - Failed to read  Service ID");
        (*rc) = err_parser_unexpected_data;
        return NULL;
   }

   //    Provider ID
   pNext = ReadIntFromString(
                    pNext,         //   [in]      Source string
                    ",",           //   [in,opt]   Value termination
                    NULL,          //   [in,opt]   Allowed padding
                    &externalPoi.iProviderID,    //   [out]      Put it here
                    1);  //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoi() - Failed to read  Provider ID");
        (*rc) = err_parser_unexpected_data;
        return NULL;
   }

   //   Longitude
   pNext = ReadIntFromString(
               pNext,            //   [in]      Source string
               ",",              //   [in,opt]   Value termination
               NULL,             //   [in,opt]   Allowed padding
               &externalPoi.iLongitude,   //   [out]      Put it here
               1);  //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoi() - Failed to read longitude");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   Latitude
   pNext = ReadIntFromString(
               pNext,            //   [in]      Source string
               ",",              //   [in,opt]   Value termination
               NULL,             //   [in,opt]   Allowed padding
               &externalPoi.iLatitude,    //   [out]      Put it here
               1);  //   [in]      Remove additional termination CHARS

   if( !pNext || !(*pNext))
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoi() - Failed to read altitude");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   //   Creation Time
   pNext = ReadIntFromString(
                pNext,            //   [in]      Source string
                ",",              //   [in,opt]   Value termination
                NULL,             //   [in,opt]   Allowed padding
                &externalPoi.iCreationTime,    //   [out]      Put it here
                1);  //   [in]      Remove additional termination CHARS

    if( !pNext || !(*pNext))
    {
       roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoi() - Failed to read Creation Time");
       (*rc) = err_parser_unexpected_data;
       return NULL;
    }

    //   Update time
    pNext = ReadIntFromString(
                 pNext,            //   [in]      Source string
                 ",",              //   [in,opt]   Value termination
                 NULL,             //   [in,opt]   Allowed padding
                 &externalPoi.iUpdateTime,    //   [out]      Put it here
                 1);  //   [in]      Remove additional termination CHARS

    if( !pNext || !(*pNext))
    {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoi() - Failed to read Update time");
        (*rc) = err_parser_unexpected_data;
        return NULL;
    }

    //   Promotion Type
    pNext = ReadIntFromString(
                  pNext,            //   [in]      Source string
                  ",",              //   [in,opt]   Value termination
                  NULL,             //   [in,opt]   Allowed padding
                  &externalPoi.iPromotionType,    //   [out]      Put it here
                  1);  //   [in]      Remove additional termination CHARS

    if( !pNext || !(*pNext))
    {
         roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoi() - Failed to read Promotion Type");
         (*rc) = err_parser_unexpected_data;
         return NULL;
    }

    //   is Promotioned
    pNext = ReadIntFromString(
                   pNext,            //   [in]      Source string
                   ",",              //   [in,opt]   Value termination
                   NULL,             //   [in,opt]   Allowed padding
                   &externalPoi.iIsPromotioned,    //   [out]      Put it here
                   1);  //   [in]      Remove additional termination CHARS

    if( !pNext || !(*pNext))
    {
          roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoi() - Failed to read isPromotioned");
          (*rc) = err_parser_unexpected_data;
          return NULL;
    }

    iBufferSize = sizeof(externalPoi.cResourceUrlParams);
     pNext       = ExtractNetworkString(
                 pNext,               // [in]     Source string
                 &externalPoi.cResourceUrlParams[0],         // [out,opt]Output buffer
                 &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                 ",",                 // [in]     Array of chars to terminate the copy operation
                 1);     // [in]     Remove additional termination chars

     if( !pNext)
     {
        roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoi() - Failed to read ResourceUrlParams.");
        (*rc) = err_parser_unexpected_data;
        return NULL;
     }

     //  Promotion ID
     pNext = ReadIntFromString(
                         pNext,         //   [in]      Source string
                         ",",           //   [in,opt]   Value termination
                         NULL,          //   [in,opt]   Allowed padding
                         &externalPoi.iPromotionID,    //   [out]      Put it here
                         1);  //   [in]      Remove additional termination CHARS

     if(!pNext)
     {
             roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoi() - Failed to read Promotion ID");
             (*rc) = err_parser_unexpected_data;
             return NULL;
     }

     //  Promotion radius
     pNext = ReadIntFromString(
                          pNext,         //   [in]      Source string
                          ",",           //   [in,opt]   Value termination
                          NULL,          //   [in,opt]   Allowed padding
                          &externalPoi.iPromotionRadius,    //   [out]      Put it here
                          1);  //   [in]      Remove additional termination CHARS

     if(!pNext)
     {
            roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoi() - Failed to read Promotion radius");
            (*rc) = err_parser_unexpected_data;
            return NULL;
     }


     //  ShowNearBy
     pNext = ReadIntFromString(
                          pNext,         //   [in]      Source string
                          ",\r\n",           //   [in,opt]   Value termination
                          NULL,          //   [in,opt]   Allowed padding
                          &externalPoi.iShowNearByFlags,    //   [out]      Put it here
                          TRIM_ALL_CHARS);  //   [in]      Remove additional termination CHARS

     if(!pNext)
     {
            roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::AddExternalPoi() - Failed to show Nearby");
            (*rc) = err_parser_unexpected_data;
            return NULL;
     }

     if ((externalPoi.iShowNearByFlags & SHOW_NEAR_BY_AS_AD) && (externalPoi.iPromotionID == -1))
        externalPoi.iShowNearByFlags = 0;

     if (externalPoi.iShowNearByFlags & SHOW_NEAR_BY_AS_AD)
        externalPoi.iPromotionID = -1;

     RealtimeExternalPoi_ExternalPoi_Add(&externalPoi);
     return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char *RmExternalPoi (/* IN  */   const char*       pNext,
               /* IN  */   void*             pContext,
               /* OUT */   BOOL*             more_data_needed,
               /* OUT */   roadmap_result*   rc)
{
   int iID;

   pNext = ReadIntFromString(
         pNext, //   [in]      Source string
         ",\r\n", //   [in,opt]   Value termination
         NULL, //   [in,opt]   Allowed padding
         &iID, //   [out]      Put it here
         TRIM_ALL_CHARS); //   [in]      Remove additional termination CHARS

   if( !pNext )
   {
      roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::RmExternalPoi() - Failed to read  ID");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }

   RealtimeExternalPoi_ExternalPoi_Remove(iID);
   return pNext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char *SetExternalPoiDrawOrder (/* IN  */   const char*       pNext,
               /* IN  */   void*             pContext,
               /* OUT */   BOOL*             more_data_needed,
               /* OUT */   roadmap_result*   rc)
{

   int entitiesNum;
   int i;
   int last;
   int entityId;

    pNext = ReadIntFromString(
          pNext, //   [in]      Source string
          ",\r\n", //   [in,opt]   Value termination
          NULL, //   [in,opt]   Allowed padding
          &entitiesNum, //   [out]      Put it here
          1); //   [in]      Remove additional termination CHARS

    if( !pNext)
    {
       roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::SetExternalPoiDrawOrder() - Failed to read entities number ");
       (*rc) = err_parser_unexpected_data;
    }


    RealtimeExternalPoi_DisplayList_clear();

    last = 0;
    for (i = 0; i < entitiesNum; i++) {
       if (i == entitiesNum - 1) {
          last = 1;
       }

       pNext = ReadIntFromString(
             pNext, //   [in]      Source string
             (last ? ",\r\n" : ","), // [in]     Array of chars to terminate the copy operation
             NULL, //   [in,opt]   Allowed padding
             &entityId, //   [out]      Put it here
             1); //   [in]      Remove additional termination CHARS

       if( !pNext || (!last && !(*pNext)))
       {
          roadmap_log( ROADMAP_ERROR, "RTNet::OnGeneralResponse::SetExternalPoiDrawOrder() - Failed to read entity order field %i ", i);
          (*rc) = err_parser_unexpected_data;
          return NULL;
       }
       RealtimeExternalPoi_DisplayList_add_ID(entityId);
    }

    RealtimeExternalPoi_DisplayList();
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
