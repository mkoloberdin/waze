/* roadmap_scoreboard.m
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi R.
 *   Copyright 2009, Waze Ltd
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


#include <assert.h>
#include <string.h>
#include "roadmap.h"
#include "roadmap_lang.h"
#include "roadmap_device.h"
#include "roadmap_device_events.h"
#include "roadmap_login.h"
#include "Realtime.h"
#include "roadmap_messagebox.h"

#include "roadmap_main.h"
#include "roadmap_iphonemain.h"
#include "widgets/iphoneCell.h"
#include "roadmap_iphoneimage.h"
#include "roadmap_scoreboard.h"
#include "roadmap_iphonescoreboard.h"



static RoadMapConfigDescriptor RoadMapConfigFeatureEnabled =
   ROADMAP_CONFIG_ITEM("Scoreboard", "Feature enabled");



#define WEB_SCOREBOARD  1

#if (WEB_SCOREBOARD)
#include "roadmap_browser.h"
#endif

#define MAX_SCORE_ITEMS_COUNT             100
#define SCOREBOARD_FONT_SIZE              14

#define SCOREBOARD_ATTRIBUTE_MAX_SIZE     100
#define SCOREBOARD_PERIOD_MAX_SIZE        20
#define SCOREBOARD_GEOGRAPHY_MAX_SIZE     20
#define SCOREBOARD_CUSTOM_TEXT_MAX_SIZE   100

#define WIDTH_POINTS_LBL      62
#define WIDTH_DIFF_LBL        37
#define WIDTH_ARROW_LBL       25

enum entry_status {
   INVALID_STATUS = 0,
   REQUESTED_STATUS,
   VALID_STATUS
};

enum periods {
   SCOREBOARD_PERIOD_WEEKLY = 0,
   SCOREBOARD_PERIOD_ALL
};

enum geographies {
   SCOREBOARD_GEOGRAPHY_COUNTRY = 0,
   SCOREBOARD_GEOGRAPHY_GLOBAL
};

#define DEFAULT_PERIOD     SCOREBOARD_PERIOD_WEEKLY
#define DEFAULT_GEOGRAPHY  SCOREBOARD_GEOGRAPHY_COUNTRY


typedef struct score_entry_t {
   char           userName[RM_LOGIN_MAX_USERNAME_LENGTH];
   char           attribute[SCOREBOARD_ATTRIBUTE_MAX_SIZE];
   int            score;
   int            rank;
   int            rankDiff;
   int            status;
} score_entry;

typedef struct scores_table_t {
   score_entry    scoreItems[MAX_SCORE_ITEMS_COUNT];
   BOOL           isEmpty;
   int            offset;
   int            lastRequested;
} score_table;

score_table    gs_scoreTable;


typedef struct my_score_t {
   int            score;
   int            rank;
   int            rankDiff;
   int            lastRank;
   BOOL           isValid;
   BOOL           isShowingMe;
   char           period[SCOREBOARD_PERIOD_MAX_SIZE];
   char           geography[SCOREBOARD_GEOGRAPHY_MAX_SIZE];
   char           leftText[SCOREBOARD_CUSTOM_TEXT_MAX_SIZE];
   char           rightText[SCOREBOARD_CUSTOM_TEXT_MAX_SIZE];
} my_score;

static my_score         gs_myScore;

static ScoreboardDialog *gs_dialog;
static RoadMapCallback  gs_requestMyCallback = NULL;
static RoadMapCallback  gs_requestRowsCallback = NULL;

static UIImage     *upImage = NULL;
static UIImage     *downImage = NULL;


///////////////////////////////////////////////////////////////
static void score_table_reset (void) {
   memset(&gs_scoreTable.scoreItems[0], 0, sizeof(gs_scoreTable.scoreItems[0]) *(MAX_SCORE_ITEMS_COUNT));
   gs_scoreTable.isEmpty = TRUE;   
}

///////////////////////////////////////////////////////////////
const char *roadmap_scoreboard_geography_name (int geography) {
   switch (geography) {
      case SCOREBOARD_GEOGRAPHY_COUNTRY:
         return "country";
         break;
      case SCOREBOARD_GEOGRAPHY_GLOBAL:
         return "global";
      default:
         break;
   }
   
   return "";
}

///////////////////////////////////////////////////////////////
const char *roadmap_scoreboard_period_name (int period) {
   switch (period) {
      case SCOREBOARD_PERIOD_WEEKLY:
         return "weekly";
         break;
      case SCOREBOARD_PERIOD_ALL:
         return "all";
      default:
         break;
   }
   
   return "";
}

///////////////////////////////////////////////////////////////
void roadmap_scoreboard_set_period (int period) {
   
   snprintf(gs_myScore.period, sizeof(gs_myScore.period), "%s", roadmap_scoreboard_period_name(period));
   score_table_reset();
   gs_scoreTable.offset = 1;
   gs_myScore.isValid = FALSE;
   gs_myScore.lastRank = 0;
   
   [gs_dialog refreshMy];
   gs_myScore.isShowingMe = FALSE;
   [gs_dialog scrollToRank:1];
   [gs_dialog refreshScores];
}

///////////////////////////////////////////////////////////////
void roadmap_scoreboard_set_geography (int geography) {
   
   snprintf(gs_myScore.geography, sizeof(gs_myScore.geography), "%s", roadmap_scoreboard_geography_name(geography));
   score_table_reset();
   gs_scoreTable.offset = 1;
   gs_myScore.isValid = FALSE;
   gs_myScore.lastRank = 0;
   
   [gs_dialog refreshMy];
   gs_myScore.isShowingMe = FALSE;
   [gs_dialog scrollToRank:1];
   [gs_dialog refreshScores];
}


///////////////////////////////////////////////////////////////
static void score_table_center (int center) {
   int current_center = MAX_SCORE_ITEMS_COUNT/2 + gs_scoreTable.offset;
   int shift;
   int shiftCount;
   int absShift;
   
   shift = center - current_center;
   absShift = abs(shift);
   //roadmap_log (ROADMAP_DEBUG, "score_table_center() - center: %d, current_center: %d, shift: %d, offset before: %d",
     //           center, current_center, shift, gs_scoreTable.offset);
   
   if (absShift < MAX_SCORE_ITEMS_COUNT/4)
      return; //Allow threashold of half table
   
   if (gs_scoreTable.offset + shift < 1) {
      shift = 1 - gs_scoreTable.offset;
      absShift = abs(shift);
   }
   
   if (shift == 0)
      return;
   
   //shift rows if possible
   if (absShift < MAX_SCORE_ITEMS_COUNT) {
      shiftCount = MAX_SCORE_ITEMS_COUNT - abs(shift);
      if (shift < 0) {
         //roadmap_log (ROADMAP_DEBUG, "score_table_center() - shifting from %d to %d, move: %d, set: %d",
           //           absShift, 0, shiftCount, MAX_SCORE_ITEMS_COUNT - shiftCount);
         memmove(&gs_scoreTable.scoreItems[absShift], &gs_scoreTable.scoreItems[0], 
                 sizeof(gs_scoreTable.scoreItems[0]) *shiftCount);
         memset(&gs_scoreTable.scoreItems[0], 0, sizeof(gs_scoreTable.scoreItems[0]) *(MAX_SCORE_ITEMS_COUNT - shiftCount));
      } else {
         //roadmap_log (ROADMAP_DEBUG, "score_table_center() - shifting from %d to %d, move: %d, set: %d",
           //           0, absShift, shiftCount, MAX_SCORE_ITEMS_COUNT - shiftCount);
         memmove(&gs_scoreTable.scoreItems[0], &gs_scoreTable.scoreItems[absShift], 
                 sizeof(gs_scoreTable.scoreItems[0]) *shiftCount);
         memset(&gs_scoreTable.scoreItems[shiftCount], 0, sizeof(gs_scoreTable.scoreItems[0]) *(MAX_SCORE_ITEMS_COUNT - shiftCount));
      }
      
      gs_scoreTable.offset += shift;
   } else {
      score_table_reset();
      gs_scoreTable.offset = center - MAX_SCORE_ITEMS_COUNT /2;
      if (gs_scoreTable.offset < 1)
         gs_scoreTable.offset = 1;
      
   }
}

///////////////////////////////////////////////////////////////
static BOOL read_dummy_values (const char** in_pData, int* in_count, int* in_row_count) {
   const char* pData = *in_pData;
   int count = *in_count;
   int row_count = *in_row_count;
   char dummyValue[256];
   int iBufferSize;
   
   while (row_count) {
      //    skip value
      iBufferSize = sizeof(dummyValue);
      pData       = ExtractNetworkString(
                                         pData,             // [in]     Source string
                                         dummyValue,        // [out,opt]Output buffer
                                         &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                                         ",\r\n",           // [in]     Array of chars to terminate the copy operation
                                         1);                // [in]     Remove additional termination chars
      
      if( !pData || !(*pData))
      {
         roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_search_results(): Failed to read unknown value=%s", dummyValue);
         return FALSE;
      }
      count--;
      row_count--;
   }
   
   return TRUE;
}
///////////////////////////////////////////////////////////////
static const char* parse_getpoints_results(roadmap_result* rc, int NumParams, const char*  pData) {
   //Expected data:
   // pointsReport,<numParams>,<period>,<geography>,<from_rank>,<count>,<numParams>,
   //    <my points>,<my_rank>,<my_rank_diff>,<numParams>,<geo-text>,<period-text>,
   //    <numParams>,<user>, <attribute>, <points>, <rank>, <rank_diff>[,<numParams>,<user>, ...]
   
   char CommandName[128];
   int iBufferSize;
   int count;
   int row_count;
   int from_rank;
   int rank_count;
   int min_row_count;
   BOOL discard_all = FALSE;
   score_entry score_row;
   RoadMapCallback callback;
   
   iBufferSize =  128;
   
   if (NumParams == 0)
      return pData;
   
   pData       = ExtractNetworkString(
                                      pData,             // [in]     Source string
                                      CommandName,//   [out]   Output buffer
                                      &iBufferSize,      // [in,out] Buffer size / Size of extracted string
                                      ",\r\n",          //   [in]   Array of chars to terminate the copy operation
                                      1);   // [in]     Remove additional termination chars
   
   if (strcmp(CommandName, "pointsReport") != 0) {
      roadmap_log(ROADMAP_ERROR, "Scoreboard - parse_getpoints_results(): could not find command: pointsReport (received: '%s')", CommandName);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   
   count = (NumParams - 1);
   
   if (!(*pData) || count == 0) {
      roadmap_log(ROADMAP_DEBUG, "Scoreboard - received empty scoreboard list");
      return pData;
   }
   
   
   // first row
   //    numParams
   pData       = ReadIntFromString(
                                   pData,         //   [in]      Source string
                                   ",",           //   [in,opt]   Value termination
                                   NULL,          //   [in,opt]   Allowed padding
                                   &row_count,    //   [out]      Put it here
                                   1);            //   [in]      Remove additional termination CHARS
   
   if( !pData || !(*pData))
   {
      roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_getpoints_results(): Failed to read my info row_count=%d", row_count);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   count--;
   
   min_row_count = 4; //We currently expect at least params: <period>,<geography>,<from_rank>,<count>
   if (row_count < min_row_count) {
      roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_getpoints_results(): first row has less than %d params", min_row_count);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   
   //   period type
   iBufferSize = SCOREBOARD_PERIOD_MAX_SIZE;
   char period[SCOREBOARD_PERIOD_MAX_SIZE];
   pData       = ExtractNetworkString(
                                      pData,               // [in]     Source string
                                      period,   // [out,opt]Output buffer
                                      &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                                      ",",                 // [in]     Array of chars to terminate the copy operation
                                      1);                  // [in]     Remove additional termination chars
   
   if( !pData || !(*pData))
   {
      roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_search_results(): Failed to read period type=%s", period);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   count--;
   row_count--;
   
   if (strcmp(gs_myScore.period, period)) {
      roadmap_log( ROADMAP_DEBUG, "Scoreboard - parse_search_results(): Wrong period type=%s expected=$s", period, gs_myScore.period);
      discard_all = TRUE;
   } else {
      strncpy_safe (gs_myScore.period, period, sizeof(gs_myScore.period));
   }

   
   //   geography type
   iBufferSize = SCOREBOARD_GEOGRAPHY_MAX_SIZE;
   char geography[SCOREBOARD_GEOGRAPHY_MAX_SIZE];
   pData       = ExtractNetworkString(
                                      pData,               // [in]     Source string
                                      geography,// [out,opt]Output buffer
                                      &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                                      ",",                 // [in]     Array of chars to terminate the copy operation
                                      1);                  // [in]     Remove additional termination chars
   
   if( !pData || !(*pData))
   {
      roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_getpoints_results(): Failed to read geography type=%s", geography);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   count--;
   row_count--;
   
   if (strcmp(gs_myScore.geography, geography)) {
      roadmap_log( ROADMAP_DEBUG, "Scoreboard - parse_search_results(): Wrong geography type=%s expected=$s", geography, gs_myScore.geography);
      discard_all = TRUE;
   } else {
      strncpy_safe (gs_myScore.geography, geography, sizeof(gs_myScore.geography));
   }
   
   //   from_rank
   pData       = ReadIntFromString(
                                   pData,         //   [in]      Source string
                                   ",",           //   [in,opt]   Value termination
                                   NULL,          //   [in,opt]   Allowed padding
                                   &from_rank,    //   [out]      Put it here
                                   1);            //   [in]      Remove additional termination CHARS
   
   if( !pData || !(*pData))
   {
      roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_getpoints_results(): Failed to read from_rank=%d", from_rank);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   count--;
   row_count--;
   
   //   count
   pData       = ReadIntFromString(
                                   pData,         //   [in]      Source string
                                   ",\r\n",       //   [in,opt]   Value termination
                                   NULL,          //   [in,opt]   Allowed padding
                                   &rank_count,   //   [out]      Put it here
                                   1);            //   [in]      Remove additional termination CHARS
   
   if( !pData || !(*pData))
   {
      roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_getpoints_results(): Failed to read rank_count=%d", rank_count);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   count--;
   row_count--;
   
   if (!read_dummy_values(&pData, &count, &row_count)) {
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   
   
   //My info row
   
   //    numParams
   pData       = ReadIntFromString(
                                   pData,         //   [in]      Source string
                                   ",",           //   [in,opt]   Value termination
                                   NULL,          //   [in,opt]   Allowed padding
                                   &row_count,    //   [out]      Put it here
                                   1);            //   [in]      Remove additional termination CHARS
   
   if( !pData || !(*pData))
   {
      roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_getpoints_results(): Failed to read my info row_count=%d", row_count);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   count--;
   
   min_row_count = 3; //We currently expect at least params: <my points>,<my_rank>,<my_rank_diff>
   if (row_count < min_row_count) {
      roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_getpoints_results(): my info row has less than %d params", min_row_count);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   
   //    my_points
   pData       = ReadIntFromString(
                                   pData,                //   [in]      Source string
                                   ",",                  //   [in,opt]   Value termination
                                   NULL,                 //   [in,opt]   Allowed padding
                                   &gs_myScore.score,    //   [out]      Put it here
                                   1);                   //   [in]      Remove additional termination CHARS
   
   if( !pData || !(*pData))
   {
      roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_getpoints_results(): Failed to read my_points=%d", gs_myScore.score);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   count--;
   row_count--;
   
   //    my_rank
   pData       = ReadIntFromString(
                                   pData,                //   [in]      Source string
                                   ",",                  //   [in,opt]   Value termination
                                   NULL,                 //   [in,opt]   Allowed padding
                                   &gs_myScore.rank,     //   [out]      Put it here
                                   1);                   //   [in]      Remove additional termination CHARS
   
   if( !pData || !(*pData))
   {
      roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_getpoints_results(): Failed to read my_rank=%d", gs_myScore.rank);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   count--;
   row_count--;
   
   //    my_rank_diff
   pData       = ReadIntFromString(
                                   pData,                //   [in]      Source string
                                   ",\r\n",                  //   [in,opt]   Value termination
                                   NULL,                 //   [in,opt]   Allowed padding
                                   &gs_myScore.rankDiff, //   [out]      Put it here
                                   1);                   //   [in]      Remove additional termination CHARS
   
   if( !pData || (!(*pData) && count > 1))
   {
      roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_getpoints_results(): Failed to read my_rank_diff=%d", gs_myScore.rankDiff);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   count--;
   row_count--;
   
   if (!read_dummy_values(&pData, &count, &row_count)) {
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   
   // Additional info row
   
   //    numParams
   pData       = ReadIntFromString(
                                   pData,         //   [in]      Source string
                                   ",",           //   [in,opt]   Value termination
                                   NULL,          //   [in,opt]   Allowed padding
                                   &row_count,    //   [out]      Put it here
                                   1);            //   [in]      Remove additional termination CHARS
   
   if( !pData || !(*pData))
   {
      roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_getpoints_results(): Failed to read additional info row_count=%d", row_count);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   count--;
   
   min_row_count = 2; //We currently expect at least params: <geo-text>,<period-text>
   if (row_count < min_row_count) {
      roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_getpoints_results(): my info row has less than %d params", min_row_count);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   
   //    geo-text
   iBufferSize = SCOREBOARD_CUSTOM_TEXT_MAX_SIZE;
   pData       = ExtractNetworkString(
                                      pData,               // [in]     Source string
                                      gs_myScore.leftText, // [out,opt]Output buffer
                                      &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                                      ",",                 // [in]     Array of chars to terminate the copy operation
                                      1);                  // [in]     Remove additional termination chars
   
   if( !pData || !(*pData))
   {
      roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_getpoints_results(): Failed to read left text=%s", gs_myScore.leftText);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   count--;
   row_count--;
   
   //    period-text
   iBufferSize = SCOREBOARD_CUSTOM_TEXT_MAX_SIZE;
   pData       = ExtractNetworkString(
                                      pData,               // [in]     Source string
                                      gs_myScore.rightText,// [out,opt]Output buffer
                                      &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                                      ",\r\n",                 // [in]     Array of chars to terminate the copy operation
                                      1);                  // [in]     Remove additional termination chars
   
   if( !pData || (!(*pData) && count > 1))
   {
      roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_getpoints_results(): Failed to read right text=%s", gs_myScore.rightText);
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   count--;
   row_count--;
   
   if (!read_dummy_values(&pData, &count, &row_count)) {
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   
   // read user rank rows
   while (rank_count) {
      //    numParams
      pData       = ReadIntFromString(
                                      pData,         //   [in]      Source string
                                      ",",           //   [in,opt]   Value termination
                                      NULL,          //   [in,opt]   Allowed padding
                                      &row_count,    //   [out]      Put it here
                                      1);            //   [in]      Remove additional termination CHARS
      
      if( !pData || !(*pData))
      {
         roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_getpoints_results(): Failed to read user rank row_count=%d", row_count);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
      count--;
      
      min_row_count = 5; //We currently expect at least params: <user>, <attribute>, <points>, <rank>, <rank_diff>
      if (row_count < min_row_count) {
         roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_getpoints_results(): user rank row has less than %d params", min_row_count);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
      
      //   user
      iBufferSize = RM_LOGIN_MAX_USERNAME_LENGTH;
      pData       = ExtractNetworkString(
                                         pData,               // [in]     Source string
                                         score_row.userName,  // [out,opt]Output buffer
                                         &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                                         ",",                 // [in]     Array of chars to terminate the copy operation
                                         1);                  // [in]     Remove additional termination chars
      
      if( !pData || !(*pData))
      {
         roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_search_results(): Failed to read user rank name=%s", score_row.userName);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
      count--;
      row_count--;
      
      //   attribute
      iBufferSize = SCOREBOARD_ATTRIBUTE_MAX_SIZE;
      pData       = ExtractNetworkString(
                                         pData,               // [in]     Source string
                                         score_row.attribute, // [out,opt]Output buffer
                                         &iBufferSize,        // [in,out] Buffer size / Size of extracted string
                                         ",",                 // [in]     Array of chars to terminate the copy operation
                                         1);                  // [in]     Remove additional termination chars
      
      if( !pData || !(*pData))
      {
         roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_search_results(): Failed to read user attribute=%s", score_row.attribute);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
      count--;
      row_count--;
      
      //   points
      pData       = ReadIntFromString(
                                      pData,             //   [in]      Source string
                                      ",",               //   [in,opt]   Value termination
                                      NULL,              //   [in,opt]   Allowed padding
                                      &score_row.score,  //   [out]      Put it here
                                      1);                //   [in]      Remove additional termination CHARS
      
      if( !pData || !(*pData))
      {
         roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_getpoints_results(): Failed to read user score=%d", score_row.score);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
      count--;
      row_count--;

      //   rank
      pData       = ReadIntFromString(
                                      pData,             //   [in]      Source string
                                      ",",               //   [in,opt]   Value termination
                                      NULL,              //   [in,opt]   Allowed padding
                                      &score_row.rank,   //   [out]      Put it here
                                      1);                //   [in]      Remove additional termination CHARS
      
      if( !pData || !(*pData))
      {
         roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_getpoints_results(): Failed to read user score=%d", score_row.rank);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
      count--;
      row_count--;
      
      //    user rank_diff
      pData       = ReadIntFromString(
                                      pData,                //   [in]      Source string
                                      ",\r\n",                  //   [in,opt]   Value termination
                                      NULL,                 //   [in,opt]   Allowed padding
                                      &score_row.rankDiff, //   [out]      Put it here
                                      1);                   //   [in]      Remove additional termination CHARS
      
      if( !pData || (!(*pData) && count > 1))
      {
         roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_getpoints_results(): Failed to read user rank_diff=%d", score_row.rankDiff);
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
      count--;
      row_count--;
      
      if (!read_dummy_values(&pData, &count, &row_count)) {
         (*rc) = err_parser_unexpected_data;
         return NULL;
      }
      
      if (score_row.rank >= gs_scoreTable.offset &&
          score_row.rank <= gs_scoreTable.offset + MAX_SCORE_ITEMS_COUNT - 1 &&
          !discard_all) {
         score_row.status = VALID_STATUS;
         gs_scoreTable.isEmpty = FALSE;
         gs_scoreTable.scoreItems[score_row.rank - gs_scoreTable.offset] = score_row;
      } else {
         roadmap_log (ROADMAP_DEBUG, "Scoreboard - parse_getpoints_results(): no room for rank: %d in table with offset: %d and item count: %d",
                      score_row.rank, gs_scoreTable.offset, MAX_SCORE_ITEMS_COUNT);
      }

      
      rank_count--;
   }
   
   if (count > 0) {
      roadmap_log( ROADMAP_ERROR, "Scoreboard - parse_getpoints_results(): More data not expected");
      (*rc) = err_parser_unexpected_data;
      return NULL;
   }
   
   if (!gs_myScore.isValid &&
       !discard_all) {
      gs_myScore.isValid = TRUE;
      gs_myScore.lastRank = gs_myScore.rank + MAX_SCORE_ITEMS_COUNT/2;
      if (gs_requestMyCallback) {
         callback = gs_requestMyCallback;
         //gs_requestMyCallback = NULL;
         callback();
      }
         
   }
   
   if (!gs_scoreTable.isEmpty && gs_requestRowsCallback && !discard_all) {
      callback = gs_requestRowsCallback;
      //gs_requestRowsCallback = NULL;
      callback(); //TODO: call this only if we have valid changes
   }
      
   
   return pData;
}

///////////////////////////////////////////////////////////////
const char* roadmap_scoreboard_response(int status, roadmap_result* rc, int NumParams, const char*  pData){
   if (status != 200){
      roadmap_log( ROADMAP_ERROR, "roadmap_scoreboard_response () - 'getPoints' failed (status= %d)", status );
      return pData;
   }
   
   roadmap_log( ROADMAP_DEBUG, "roadmap_scoreboard_response () - received 'getPoints' response");
   return parse_getpoints_results(rc, NumParams, pData);
}

///////////////////////////////////////////////////////////////
void roadmap_scoreboard_request_rows (int rank, RoadMapCallback callback) {
   int from, to;
   BOOL  cont;
   int near;
   
   //score_table_center(rank);
   
   near = rank - gs_scoreTable.offset;
   if (near < 0) return;
   
   from = near;
   to = near;
   
   if (gs_scoreTable.scoreItems[near].status != INVALID_STATUS)
      return;
   
   gs_scoreTable.scoreItems[near].status = REQUESTED_STATUS;
   
   // Calculate the range to request, based on invalid entries near the requested row
   cont = TRUE;
   while (cont && (from >= 1) && (near - from < MAX_SCORE_ITEMS_COUNT / 2)){
      if (gs_scoreTable.scoreItems[from - 1].status == INVALID_STATUS) {
         gs_scoreTable.scoreItems[from - 1].status = REQUESTED_STATUS;
         from--;
      } else
         cont = FALSE;
   }
   
   cont = TRUE;
   while (cont && (to - from + 1) < 100 && (to - 1 < MAX_SCORE_ITEMS_COUNT)) {
      if (gs_scoreTable.scoreItems[to + 1].status  == INVALID_STATUS) {
         gs_scoreTable.scoreItems[to + 1].status  = REQUESTED_STATUS;
         to++;
      } else
         cont = FALSE;
   }
   
   
   {
      gs_requestRowsCallback = callback;
      roadmap_log(ROADMAP_DEBUG, "Requesting rows: %d count: %d", from + gs_scoreTable.offset, to - from +1);
      Realtime_Scoreboard_getPoints(gs_myScore.period, gs_myScore.geography, from + gs_scoreTable.offset, to - from +1);
   }
}

///////////////////////////////////////////////////////////////
void roadmap_scoreboard_request_my_score (RoadMapCallback callback) {
   gs_requestMyCallback = callback;
   Realtime_Scoreboard_getPoints(gs_myScore.period, gs_myScore.geography, 0, 0);
}

///////////////////////////////////////////////////////////////
static void request_rows_cb (void) {
   [gs_dialog refreshScores];
}

///////////////////////////////////////////////////////////////
static void request_my_score_cb (void) {
   [gs_dialog refreshMy];
}

///////////////////////////////////////////////////////////////
void roadmap_scoreboard_request_failed (roadmap_result rc) {
   //roadmap_main_show_root(YES);
   roadmap_log (ROADMAP_WARNING, "roadmap_scoreboard_request_failed(): Could not download scoreboard");
   roadmap_messagebox_timeout("Error", "Could not download scoreboard", 5);
}

///////////////////////////////////////////////////////////////
score_table *roadmap_scoreboard_score_table (void) {
   return &gs_scoreTable;
}

///////////////////////////////////////////////////////////////
my_score *roadmap_scoreboard_my_score (void) {
   return &gs_myScore;
}

///////////////////////////////////////////////////////////////
BOOL roadmap_scoreboard_feature_enabled (void) {
   //return FALSE; //TODO: remove this
   if (0 == strcmp (roadmap_config_get (&RoadMapConfigFeatureEnabled), "yes")){
      return TRUE;
   }
   
   return FALSE;
}

///////////////////////////////////////////////////////////////
void roadmap_scoreboard_show_me(void) {
   if (gs_myScore.rank > MAX_SCORE_ITEMS_COUNT &&
       gs_myScore.isValid &&
       !gs_myScore.isShowingMe) {
      
      score_table_center(gs_myScore.rank);
      
      gs_myScore.isShowingMe = TRUE;
      [gs_dialog scrollToRank:gs_myScore.rank];
      [gs_dialog refreshScores];
   } else if (gs_myScore.rank > 0 &&
              gs_myScore.isValid) {
      [gs_dialog scrollToRank:gs_myScore.rank];
      [gs_dialog refreshScores];
   }

}

///////////////////////////////////////////////////////////////
void roadmap_scoreboard_show_up(void) {
   if (gs_myScore.isShowingMe) {
      
      score_table_center(MAX_SCORE_ITEMS_COUNT /2);
      
      gs_myScore.isShowingMe = FALSE;
   }
   
   [gs_dialog scrollToRank:1];
   [gs_dialog refreshScores];
}

///////////////////////////////////////////////////////////////
static void roadmap_scoreboard_init(void) {
   gs_scoreTable.offset = 1;
   
   score_table_reset();
   snprintf(gs_myScore.period, sizeof(gs_myScore.period), "%s", roadmap_scoreboard_period_name(DEFAULT_PERIOD));
   snprintf(gs_myScore.geography, sizeof(gs_myScore.geography), "%s", roadmap_scoreboard_geography_name(DEFAULT_GEOGRAPHY));
   
   gs_myScore.isValid = FALSE;
   gs_myScore.isShowingMe = FALSE;
   
   roadmap_config_declare_enumeration ("preferences", &RoadMapConfigFeatureEnabled, NULL, "no",
                                       "yes", NULL);



   upImage = roadmap_iphoneimage_load("scoreboard_up");
   downImage = roadmap_iphoneimage_load("scoreboard_down");
}

///////////////////////////////////////////////////////////////
void roadmap_scoreboard(void) {
#if (WEB_SCOREBOARD)
   roadmap_browser_show("Scoreboard", "http://www.waze.com", FALSE);
#else
   static BOOL initialized = FALSE;
   
   if (!initialized) {
      roadmap_scoreboard_init();
      initialized = TRUE;
   }
   
   if (!roadmap_scoreboard_feature_enabled()) {
      roadmap_messagebox_timeout("Info", "Scoreboard is currently not available in your area", 5);
      return;
   }
   
	gs_dialog = [[ScoreboardDialog alloc] initWithStyle:UITableViewStyleGrouped];
	[gs_dialog show];
   
   roadmap_scoreboard_set_period(DEFAULT_PERIOD);
   roadmap_scoreboard_set_geography(DEFAULT_GEOGRAPHY);
#endif //WEB_SCOREBOARD
}






///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
@implementation ScoreboardMyView
- (void) onShowMe
{
   roadmap_scoreboard_show_me();
}

- (void) resize
{
   CGRect frame = self.frame;
   CGRect rect;
   
   [leftLabel sizeToFit];
   rect = leftLabel.bounds;
   rect.origin.x = 10;
   rect.origin.y = 10;
   rect.size.width = self.bounds.size.width /2 -10;
   leftLabel.frame = rect;
   
   [rightLabel sizeToFit];
   rect = rightLabel.bounds;
   rect.origin.x = self.bounds.size.width /2;
   rect.origin.y = 10;
   rect.size.width = self.bounds.size.width /2 -10;
   rightLabel.frame = rect;
   
   rect = CGRectMake(2, frame.size.height - 43, frame.size.width - 4, 30);
   showMeButton.frame = rect;
   
   
   [specialImageV sizeToFit];
   rect = specialImageV.bounds;
   rect.origin.x = 5;
   rect.origin.y = frame.size.height - rect.size.height - 20;
   specialImageV.frame = rect;
   
   [youLabel sizeToFit];
   rect = youLabel.bounds;
   rect.origin.x = 30;
   rect.origin.y = frame.size.height - rect.size.height - 20;
   rect.size.width = frame.size.width - 5 - WIDTH_DIFF_LBL - WIDTH_ARROW_LBL - WIDTH_POINTS_LBL - rect.origin.x;
   youLabel.frame = rect;
   
   [pointsLabel sizeToFit];
   rect = pointsLabel.bounds;
   rect.origin.x = frame.size.width - 5 - WIDTH_DIFF_LBL - WIDTH_ARROW_LBL - WIDTH_POINTS_LBL;
   rect.origin.y = frame.size.height - rect.size.height - 20;
   rect.size.width = WIDTH_POINTS_LBL;
   pointsLabel.frame = rect;
   
   [trendImageV sizeToFit];
   rect = trendImageV.bounds;
   rect.origin.x = frame.size.width - 5 - WIDTH_DIFF_LBL - WIDTH_ARROW_LBL;
   rect.origin.y = frame.size.height - rect.size.height - 20;
   trendImageV.frame = rect;
   
   [activityIndicator sizeToFit];
   rect = activityIndicator.bounds;
   rect.origin.x = frame.size.width - 5 - WIDTH_DIFF_LBL - WIDTH_ARROW_LBL;
   rect.origin.y = frame.size.height - rect.size.height - 20;
   activityIndicator.frame = rect;
   
   [deltaLabel sizeToFit];
   rect = deltaLabel.bounds;
   rect.origin.x = frame.size.width - 5 - WIDTH_DIFF_LBL;
   rect.origin.y = frame.size.height - rect.size.height - 20;
   rect.size.width = WIDTH_DIFF_LBL;
   deltaLabel.frame = rect;
}

- (id)initWithFrame:(CGRect)aRect
{
   self = [super initWithFrame:aRect];
   
   if (self) {
      showMeButton = [UIButton buttonWithType:UIButtonTypeRoundedRect];
      [showMeButton addTarget:self action:@selector(onShowMe) forControlEvents:UIControlEventTouchUpInside];
      //showMeButton.backgroundColor = [UIColor colorWithRed:0.2f green:0.46f blue:0.55f alpha:1.0f];
      [self addSubview:showMeButton];
      specialImageV = [[UIImageView alloc] initWithFrame:aRect];
      [self addSubview:specialImageV];
      
      [specialImageV release];
      leftLabel = [[UILabel alloc] initWithFrame:aRect];
      leftLabel.minimumFontSize = 12;
      leftLabel.adjustsFontSizeToFitWidth = YES;
      leftLabel.font = [UIFont systemFontOfSize:16];
      leftLabel.backgroundColor = [UIColor clearColor];
      [self addSubview:leftLabel];
      [leftLabel release];
      
      rightLabel = [[UILabel alloc] initWithFrame:aRect];
      rightLabel.minimumFontSize = 12;
      rightLabel.adjustsFontSizeToFitWidth = YES;
      rightLabel.textAlignment = UITextAlignmentRight;
      rightLabel.font = [UIFont systemFontOfSize:16];
      rightLabel.backgroundColor = [UIColor clearColor];
      [self addSubview:rightLabel];
      [rightLabel release];
      
      youLabel = [[UILabel alloc] initWithFrame:aRect];
      youLabel.font = [UIFont boldSystemFontOfSize:SCOREBOARD_FONT_SIZE];
      youLabel.textColor = [UIColor colorWithRed:0.2f green:0.46f blue:0.55f alpha:1.0f];
      youLabel.backgroundColor = [UIColor clearColor];
      [self addSubview:youLabel];
      [youLabel release];
      
      pointsLabel = [[UILabel alloc] initWithFrame:aRect];
      pointsLabel.font = [UIFont boldSystemFontOfSize:SCOREBOARD_FONT_SIZE];
      pointsLabel.minimumFontSize = SCOREBOARD_FONT_SIZE - 2;
      pointsLabel.adjustsFontSizeToFitWidth = YES;
      pointsLabel.backgroundColor = [UIColor clearColor];
      [self addSubview:pointsLabel];
      [pointsLabel release];
      trendImageV = [[UIImageView alloc] initWithFrame:aRect];
      [self addSubview:trendImageV];
      [trendImageV release];
      
      activityIndicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
      [activityIndicator setHidesWhenStopped:YES];
      [self addSubview:activityIndicator];
      [activityIndicator release];
      
      deltaLabel = [[UILabel alloc] initWithFrame:aRect];
      deltaLabel.font = [UIFont boldSystemFontOfSize:SCOREBOARD_FONT_SIZE];
      deltaLabel.backgroundColor = [UIColor clearColor];
      [self addSubview:deltaLabel];
      [deltaLabel release];
      
      [self clearContent];
   }
   
   return self;
}

- (void) setYou: (NSString*) you andPoints: (NSString *)points andDelta: (int)delta
    andLeftText: (NSString *)left andRightText: (NSString *)right andSpecialIcon: (const char*)icon
{
   UIImage *image = NULL;
   
   youLabel.text = you;
   pointsLabel.text = points;
   if (delta < 0)
      image = downImage;
   else if (delta > 0)
      image = upImage;
   else
      image = NULL;
   
   trendImageV.image = image;

   
   if (delta > 1000)
      deltaLabel.text = [NSString stringWithFormat:@"+%dK", delta/1000];
   else if (delta < -1000)
      deltaLabel.text = [NSString stringWithFormat:@"%dK", delta/1000];
   else if (delta > 0)
      deltaLabel.text = [NSString stringWithFormat:@"+%d", delta];
   else if (delta < 0)
      deltaLabel.text = [NSString stringWithFormat:@"%d", delta];
   else
      deltaLabel.text = @"";
   
   leftLabel.text = left;
   rightLabel.text = right;
   
   image = roadmap_iphoneimage_load(icon);
   specialImageV.image = image;
   if (image)
      [image release];
   
   [activityIndicator stopAnimating];
   
   [self resize];
}

- (void) clearContent
{
   youLabel.text = [NSString stringWithUTF8String:roadmap_lang_get("(loading)")];
   pointsLabel.text = @"";
   trendImageV.image = NULL;
   [activityIndicator startAnimating];
   deltaLabel.text = @"";
   
   [self resize];
}

- (void)layoutSubviews
{
   [self resize];
}

- (void)dealloc
{	
   [super dealloc];
}
@end

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
@implementation ScoreboardRowView
- (void) resize
{
   CGRect frame = self.frame;
   CGRect rect;
   int maxWidth;
   
   [specialImageV sizeToFit];
   rect = specialImageV.bounds;
   rect.origin.x = 5;
   rect.origin.y = frame.size.height - rect.size.height - 3;
   specialImageV.frame = rect;
   
   [attribLabel sizeToFit];
   
   [nameLabel sizeToFit];
   rect = nameLabel.bounds;
   rect.origin.x = 30;
   rect.origin.y = frame.size.height - rect.size.height - 3;
   maxWidth = frame.size.width - 5 - WIDTH_DIFF_LBL - WIDTH_ARROW_LBL - WIDTH_POINTS_LBL - rect.origin.x - attribLabel.frame.size.width - 5;
   if (rect.size.width > maxWidth)
      rect.size.width = maxWidth;
   nameLabel.frame = rect;
   
   rect = attribLabel.bounds;
   rect.origin.x = nameLabel.frame.origin.x + nameLabel.frame.size.width + 3;
   rect.origin.y = frame.size.height - rect.size.height - 3;
   attribLabel.frame = rect;
   
   [pointsLabel sizeToFit];
   rect = pointsLabel.bounds;
   rect.origin.x = frame.size.width - 5 - WIDTH_DIFF_LBL - WIDTH_ARROW_LBL - WIDTH_POINTS_LBL;
   rect.origin.y = frame.size.height - rect.size.height - 3;
   rect.size.width = WIDTH_POINTS_LBL;
   pointsLabel.frame = rect;
   
   [trendImageV sizeToFit];
   rect = trendImageV.bounds;
   rect.origin.x = frame.size.width - 5 - WIDTH_DIFF_LBL - WIDTH_ARROW_LBL;
   rect.origin.y = frame.size.height - rect.size.height - 3;
   trendImageV.frame = rect;
   
   /*
   [activityIndicator sizeToFit];
   rect = activityIndicator.bounds;
   rect.origin.x = frame.size.width - 5 - WIDTH_DIFF_LBL - WIDTH_ARROW_LBL;
   rect.origin.y = frame.size.height - rect.size.height - 3;
   activityIndicator.frame = rect;
   */
   
   [deltaLabel sizeToFit];
   rect = deltaLabel.bounds;
   rect.origin.x = frame.size.width - 5 - WIDTH_DIFF_LBL;
   rect.origin.y = frame.size.height - rect.size.height - 3;
   rect.size.width = WIDTH_DIFF_LBL;
   deltaLabel.frame = rect;
}

- (id)initWithFrame:(CGRect)aRect
{
   self = [super initWithFrame:aRect];
   
   isHighlighted = FALSE;
   
   if (self) {
      specialImageV = [[UIImageView alloc] initWithFrame:aRect];
      [specialImageV setHidden:YES]; //disabled for now
      [self addSubview:specialImageV];
      [specialImageV release];
      nameLabel = [[UILabel alloc] initWithFrame:aRect];
      nameLabel.font = [UIFont systemFontOfSize:SCOREBOARD_FONT_SIZE];
      nameLabel.textColor = [UIColor colorWithRed:0.2f green:0.46f blue:0.55f alpha:1.0f];
      [self addSubview:nameLabel];
      [nameLabel release];
      attribLabel = [[UILabel alloc] initWithFrame:aRect];
      attribLabel.font = [UIFont systemFontOfSize:SCOREBOARD_FONT_SIZE - 2];
      attribLabel.textColor = [UIColor darkGrayColor];
      [self addSubview:attribLabel];
      [attribLabel release];
      pointsLabel = [[UILabel alloc] initWithFrame:aRect];
      pointsLabel.font = [UIFont systemFontOfSize:SCOREBOARD_FONT_SIZE];
      pointsLabel.minimumFontSize = SCOREBOARD_FONT_SIZE - 2;
      pointsLabel.adjustsFontSizeToFitWidth = YES;
      [self addSubview:pointsLabel];
      [pointsLabel release];
      trendImageV = [[UIImageView alloc] initWithFrame:aRect];
      [self addSubview:trendImageV];
      [trendImageV release];
      /*
      activityIndicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleGray];
      [activityIndicator setHidesWhenStopped:YES];
      [self addSubview:activityIndicator];
      [activityIndicator release];
       */
      deltaLabel = [[UILabel alloc] initWithFrame:aRect];
      deltaLabel.font = [UIFont systemFontOfSize:SCOREBOARD_FONT_SIZE];
      [self addSubview:deltaLabel];
      [deltaLabel release];
      
      [self clearContent];
   }
   
   return self;
}

- (void) setHighlight: (BOOL)highlight
{
   UIColor *color;
   UIFont *font;
   
   if (isHighlighted == highlight)
      return;
   
   if (!highlight) {
      color = [UIColor whiteColor];      
      font = [UIFont systemFontOfSize:SCOREBOARD_FONT_SIZE];
   } else {
      color = [UIColor colorWithRed:0.85 green:0.91 blue:0.93 alpha:1.0];
      font = [UIFont boldSystemFontOfSize:SCOREBOARD_FONT_SIZE];
   }
   
   nameLabel.backgroundColor = color;
   pointsLabel.backgroundColor = color;
   deltaLabel.backgroundColor = color;
   attribLabel.backgroundColor = color;
   //self.superview.superview.backgroundColor = color;
   nameLabel.font = font;
   pointsLabel.font = font;
   deltaLabel.font = font;
   
   isHighlighted = highlight;
}

- (void) setName: (NSString*)name andAttrib: (NSString *)attribute andPoints: (NSString *)points 
        andDelta: (int)delta andHighlight: (BOOL)highlight andSpecialIcon: (const char*)icon
{
   UIImage *image = NULL;
   
   nameLabel.text = name;
   attribLabel.text = attribute;
   pointsLabel.text = points;
   if (delta < 0)
      image = downImage;
   else if (delta > 0)
      image = upImage;
   else
      image = NULL;
   trendImageV.image = image;

   if (delta > 1000)
      deltaLabel.text = [NSString stringWithFormat:@"+%dK", delta/1000];
   else if (delta < -1000)
      deltaLabel.text = [NSString stringWithFormat:@"%dK", delta/1000];
   else if (delta > 0)
      deltaLabel.text = [NSString stringWithFormat:@"+%d", delta];
   else if (delta < 0)
      deltaLabel.text = [NSString stringWithFormat:@"%d", delta];
   else
      deltaLabel.text = @"";
   
   //[activityIndicator stopAnimating];
   
   image = roadmap_iphoneimage_load(icon);
   specialImageV.image = image;
   if (image)
      [image release];
   
   [self setHighlight: highlight];
   
   [self resize];
}


- (void) clearContent
{
   nameLabel.text = @"";
   pointsLabel.text = @"";
   trendImageV.image = NULL;
   //[activityIndicator startAnimating];
   deltaLabel.text = @"";
   specialImageV.image = NULL;
   
   [self setHighlight:FALSE];
   
   [self resize];
}

- (void)layoutSubviews
{
   [self resize];
}

- (void)dealloc
{
   
   [super dealloc];
}
@end


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
@implementation ScoreboardDialog

- (void) onClose
{
   [self.navigationController setToolbarHidden:YES];
   roadmap_main_show_root(NO);
}

- (void) onScrollUp
{
   //[self.tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0] atScrollPosition:0 animated:NO];
   
   roadmap_scoreboard_show_up();
}

- (id)initWithStyle:(UITableViewStyle)style
{
	static int initialized = 0;
	
	self = [super initWithStyle:style];
	
	if (!initialized) {

		initialized = 1;
	}
	
	return self;
}

- (void) viewDidLoad
{
   
	UITableView *tableView = [self tableView];
	
	//[tableView setBackgroundColor:[UIColor clearColor]];
   [tableView setBackgroundColor:roadmap_main_table_color()];
   tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
   tableView.separatorColor = [UIColor whiteColor];
   
   roadmap_main_reload_bg_image();
}

- (void)onPeriodSelector: (id)selector
{
   UISegmentedControl *seg = (UISegmentedControl*) selector;
   if (seg.selectedSegmentIndex == 0) {
      roadmap_scoreboard_set_period(SCOREBOARD_PERIOD_WEEKLY);
   } else {
      roadmap_scoreboard_set_period(SCOREBOARD_PERIOD_ALL);
   }

}

- (void)onGeographySelector: (id)selector
{
   UISegmentedControl *seg = (UISegmentedControl*) selector;
   if (seg.selectedSegmentIndex == 0) {
      roadmap_scoreboard_set_geography(SCOREBOARD_GEOGRAPHY_COUNTRY);
   } else {
      roadmap_scoreboard_set_geography(SCOREBOARD_GEOGRAPHY_GLOBAL);
   }
   
}

- (void)viewWillAppear:(BOOL)animated
{
   [super viewWillAppear:animated];
   /*static*/ BOOL first_time = TRUE;
   UITableView *tableView = [self tableView];
   
   if (first_time) {
      first_time = FALSE;
      [tableView reloadData];
   }
   
   NSMutableArray *barArray =[NSMutableArray arrayWithCapacity:1];
   NSMutableArray *array = [NSMutableArray arrayWithCapacity:1];
   UISegmentedControl *seg;
   UIBarButtonItem *barItem;
   UIBarButtonItem *flexSpacer = [[UIBarButtonItem alloc] initWithBarButtonSystemItem: UIBarButtonSystemItemFlexibleSpace
                                                                               target:nil action:nil]; 
   
   [array addObject:[NSString stringWithUTF8String:roadmap_lang_get("State")]];
   [array addObject:[NSString stringWithUTF8String:roadmap_lang_get("All")]];
   seg = [[UISegmentedControl alloc] initWithItems:array];
   seg.selectedSegmentIndex = DEFAULT_GEOGRAPHY;
   seg.segmentedControlStyle = UISegmentedControlStyleBar;
   [seg addTarget:self action:@selector(onGeographySelector:) forControlEvents:UIControlEventValueChanged];
   [array removeAllObjects];
   barItem = [[UIBarButtonItem alloc] initWithCustomView:seg];
   //barItem.width = 240;
   //[barArray addObject:flexSpacer];
   [barArray addObject:barItem];
   [barArray addObject:flexSpacer];
   
   [array removeAllObjects];
   [array addObject:[NSString stringWithUTF8String:roadmap_lang_get("Weekly")]];
   [array addObject:[NSString stringWithUTF8String:roadmap_lang_get("All times")]];
   seg = [[UISegmentedControl alloc] initWithItems:array];
   seg.selectedSegmentIndex = DEFAULT_PERIOD;
   seg.segmentedControlStyle = UISegmentedControlStyleBar;
   [seg addTarget:self action:@selector(onPeriodSelector:) forControlEvents:UIControlEventValueChanged];
   barItem = [[UIBarButtonItem alloc] initWithCustomView:seg];
   
   [barArray addObject:barItem];
   self.toolbarItems = barArray;
   
   [self.navigationController setToolbarHidden:NO];
   
   /*
   UISegmentedControl *seg;
   UIBarButtonItem *barItem;   
   NSMutableArray *array = [NSMutableArray arrayWithCapacity:1];
   [array addObject:[NSString stringWithUTF8String:roadmap_lang_get("State/Country")]];
   [array addObject:[NSString stringWithUTF8String:roadmap_lang_get("World")]];
   seg = [[UISegmentedControl alloc] initWithItems:array];
   seg.selectedSegmentIndex = DEFAULT_GEOGRAPHY;
   seg.segmentedControlStyle = UISegmentedControlStyleBar;
   [seg addTarget:self action:@selector(onGeographySelector:) forControlEvents:UIControlEventValueChanged];
   [array removeAllObjects];
   barItem = [[UIBarButtonItem alloc] initWithCustomView:seg];
   UINavigationItem *navItem = [self navigationItem];
   [navItem setRightBarButtonItem:barButton];
    */
}

- (void)viewWillDisappear:(BOOL)animated
{
   [self.navigationController setToolbarHidden:YES];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
   return roadmap_main_should_rotate (interfaceOrientation);
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
   roadmap_device_event_notification( device_event_window_orientation_changed);
}

- (void) scrollToRank: (int)rank
{
   NSIndexPath *iPath;
   UITableView *tableView = [self tableView];
   int row_index;
   score_table *scoreItemsTable = roadmap_scoreboard_score_table();
   
   row_index = rank - scoreItemsTable->offset;
   
   iPath = [NSIndexPath indexPathForRow:row_index inSection:0];
   [tableView scrollToRowAtIndexPath:iPath atScrollPosition:UITableViewScrollPositionMiddle animated:NO];
}


- (void) refreshMy
{
   UITableView *tableView = [self tableView];
   [tableView reloadData];
}

- (void) refreshScores
{
   UITableView *tableView = [self tableView];
   ScoreboardRowView *cellView;
   NSArray *visibleRows = [tableView indexPathsForVisibleRows];
   //NSMutableArray *array = [NSMutableArray arrayWithCapacity:0];
   score_table *scoreItemsTable = roadmap_scoreboard_score_table();
   my_score *myItem = roadmap_scoreboard_my_score();
   char str[100];
   const char *specialIcon;
   BOOL highlight = FALSE;
   int rank;
   int table_index;
   UIColor *color;

   
   int i;
   for (i = 0; i < [visibleRows count]; i++) {
      NSIndexPath *path = [visibleRows objectAtIndex:i];
      if (path.row == 0)
         continue;
      
      UITableViewCell *cell = [tableView cellForRowAtIndexPath:path];
      if ([[cell reuseIdentifier] isEqualToString:@"scoreRowWait"]) {
         //[array addObject:path];
         rank = path.row + scoreItemsTable->offset - 1;
         table_index = path.row - 1;
         
         if (scoreItemsTable->scoreItems[table_index].status != VALID_STATUS) {
            roadmap_scoreboard_request_rows(rank, request_rows_cb);
            lastRank = -1;
            continue;
         }
         
         switch (scoreItemsTable->scoreItems[table_index].rank) {
            case 1:
               specialIcon = "scoreboard_gold";
               break;
            case 2:
               specialIcon = "scoreboard_silver";
               break;
            case 3:
               specialIcon = "scoreboard_bronze";
               break;
            default:
               specialIcon = "";
               break;
         }
         
         snprintf(str, sizeof(str), "%d %s", scoreItemsTable->scoreItems[table_index].score, roadmap_lang_get("pt."));
         
         highlight =  (rank == myItem->rank);
         if (!highlight) {
            color = [UIColor whiteColor];
         } else {
            color = [UIColor colorWithRed:0.85 green:0.91 blue:0.93 alpha:1.0];
         }
         cell.backgroundColor = color;
         
         cellView = [cell.contentView.subviews objectAtIndex:0];
         if (cellView) {
            [cellView setName:[NSString stringWithFormat:@"%d. %s", scoreItemsTable->scoreItems[table_index].rank/*rank*/, scoreItemsTable->scoreItems[table_index].userName]
                                         andAttrib:[NSString stringWithUTF8String:scoreItemsTable->scoreItems[table_index].attribute]
                                         andPoints:[NSString stringWithUTF8String:str]
                                          andDelta:scoreItemsTable->scoreItems[table_index].rankDiff
                                      andHighlight:highlight
                                    andSpecialIcon:specialIcon];
            //cell.reuseIdentifier = @"scoreRow";
         }
      }
         
   }
   /*
   if ([array count] > 0)
      [tableView reloadRowsAtIndexPaths:array withRowAnimation:NO];
    */
}

- (void) show
{

	[self setTitle:[NSString stringWithUTF8String:roadmap_lang_get("Scoreboard")]];
   
   //set right button
	UINavigationItem *navItem = [self navigationItem];
   UIBarButtonItem *barButton = [[UIBarButtonItem alloc] initWithTitle:[NSString stringWithUTF8String:roadmap_lang_get("Up")]
                                                                 style:UIBarButtonItemStylePlain target:self action:@selector(onScrollUp)];
   
   [navItem setRightBarButtonItem:barButton];
   
	
	roadmap_main_push_view (self);
   
}


- (void)dealloc
{	
   gs_requestMyCallback = NULL;
   gs_requestRowsCallback = NULL;

	
	[super dealloc];
}



//////////////////////////////////////////////////////////
//Table view delegate
- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
   /*
   if (indexPath.section == 0)
      return 100;
   else
      return 25;
    */
   
   if (indexPath.row == 0)
      return 90;
   else
      return 25;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
   /*
   my_score *myItem = roadmap_scoreboard_my_score();
   
   if (!myItem->isValid)
      return 1;
   
   return 2;
    */
   return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
   /*
   if (section == 0)
      return 1;
   */
   my_score *myItem = roadmap_scoreboard_my_score();
   if (myItem->isValid)
      return MAX_SCORE_ITEMS_COUNT + 1;
   else
      return 1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
   my_score *myItem = roadmap_scoreboard_my_score();
   score_table *scoreItemsTable= roadmap_scoreboard_score_table();
   NSString *cellId = NULL;
   UITableViewCell *cell;
   UIView *cellView;
   UIColor *color;
   int cellType;
   char str[100];
   char name[RM_LOGIN_MAX_USERNAME_LENGTH + 10];
   const char *specialIcon;
   BOOL highlight = FALSE;
   int table_index;
   int rank;
   
   enum cell_types {
      myScoreWaitType,
      myScoreType,
      scoreRowWaitType,
      scoreRowType
   };
   
   rank = indexPath.row + scoreItemsTable->offset - 1;
   table_index = indexPath.row - 1;
   
   if (!myItem->isValid) {
      cellId = @"myScoreWait";
      cellType = myScoreWaitType;
   } else if (indexPath.row == 0) {
      cellId = @"myScore";
      cellType = myScoreType;
   } else if (scoreItemsTable->isEmpty || table_index < 0 || table_index > MAX_SCORE_ITEMS_COUNT -1) {
      cellId = @"scoreRowWait";
      cellType = scoreRowWaitType;
   } else if (scoreItemsTable->scoreItems[table_index].status != VALID_STATUS) {
      cellId = @"scoreRowWait";
      cellType = scoreRowWaitType;
   } else {
      cellId = @"scoreRow";
      cellType = scoreRowType;
   }
   
	cell = (UITableViewCell *)[tableView dequeueReusableCellWithIdentifier:cellId];
   
   if (cell == nil) {
      cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:cellId] autorelease];
      
      if (indexPath.row == 0) {
         cellView = [[ScoreboardMyView alloc] initWithFrame:cell.frame];
      } else {
         cellView = [[ScoreboardRowView alloc] initWithFrame:cell.frame];
      }
      cell.selectionStyle = UITableViewCellSelectionStyleNone;
      
      [cell.contentView addSubview:cellView];
      
      cell.autoresizesSubviews = YES;
      cell.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
      
      
      cellView.autoresizesSubviews = YES;
      cellView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
   } else {
      cellView = [cell.contentView.subviews objectAtIndex:0];
   }
   
   
   
   switch (cellType) {
      case myScoreType:
         switch (myItem->rank) {
            case 1:
               specialIcon = "scoreboard_gold";
               break;
            case 2:
               specialIcon = "scoreboard_silver";
               break;
            case 3:
               specialIcon = "scoreboard_bronze";
               break;
            default:
               specialIcon = "";
               break;
         }
         snprintf(str, sizeof(str), "%d %s", myItem->score, roadmap_lang_get("pt."));
         if (myItem->rank > 0)
            snprintf(name, sizeof(name), "%d. %s", myItem->rank, RealTime_GetUserName());
         else
            snprintf(name, sizeof(name), "%s", RealTime_GetUserName());
         [(ScoreboardMyView *)cellView setYou:[NSString stringWithUTF8String:name]
                                    andPoints:[NSString stringWithUTF8String:str]
                                     andDelta:myItem->rankDiff
                                  andLeftText:[NSString stringWithUTF8String:roadmap_lang_get(myItem->leftText)]
                                 andRightText:[NSString stringWithUTF8String:roadmap_lang_get(myItem->rightText)]
                               andSpecialIcon:specialIcon];
         break;
      case myScoreWaitType:
         [(ScoreboardMyView *)cellView clearContent];
         break;
      case scoreRowType:
         snprintf(str, sizeof(str), "%d %s", scoreItemsTable->scoreItems[table_index].score, roadmap_lang_get("pt."));
         highlight =  (rank == myItem->rank);
         if (!highlight) {
            color = [UIColor whiteColor];
         } else {
            color = [UIColor colorWithRed:0.85 green:0.91 blue:0.93 alpha:1.0];
         }
         cell.backgroundColor = color;
         
         switch (scoreItemsTable->scoreItems[table_index].rank) {
            case 1:
               specialIcon = "scoreboard_gold";
               break;
            case 2:
               specialIcon = "scoreboard_silver";
               break;
            case 3:
               specialIcon = "scoreboard_bronze";
               break;
            default:
               specialIcon = "";
               break;
         }
         
         if (scoreItemsTable->scoreItems[table_index].rank > 0)
            snprintf(name, sizeof(name), "%d. %s", scoreItemsTable->scoreItems[table_index].rank/*rank*/, scoreItemsTable->scoreItems[table_index].userName);
         else
            snprintf(name, sizeof(name), "%s", scoreItemsTable->scoreItems[table_index].userName);
         
         [(ScoreboardRowView *)cellView setName:[NSString stringWithUTF8String:name]
                                      andAttrib:[NSString stringWithUTF8String:roadmap_lang_get(scoreItemsTable->scoreItems[table_index].attribute)]
                                      andPoints:[NSString stringWithUTF8String:str]
                                       andDelta:scoreItemsTable->scoreItems[table_index].rankDiff
                                   andHighlight:highlight
                                 andSpecialIcon:specialIcon];
         break;
      case scoreRowWaitType:
         //[(ScoreboardRowView *)cellView clearContent];
         snprintf(name, sizeof(name), "%d. %s", rank, roadmap_lang_get("(loading)"));
         [(ScoreboardRowView *)cellView setName:[NSString stringWithUTF8String:name]
                                      andAttrib:@""
                                      andPoints:@""
                                       andDelta:0
                                   andHighlight:FALSE
                                 andSpecialIcon:""];
         cell.backgroundColor = [UIColor whiteColor];
         break;
      default:
         break;
   }
   
   
   
   if (cellType == myScoreWaitType)
      roadmap_scoreboard_request_my_score(request_my_score_cb);
   else if (cellType == scoreRowWaitType) {
      if (tableView.decelerating) {
         lastRank = rank;
      } else {
         roadmap_scoreboard_request_rows(rank, request_rows_cb);
         lastRank = -1;
      }

          
   }
      
   
	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
   return;
   
   
   NSIndexPath *iPath;
   my_score *myItem = roadmap_scoreboard_my_score();
   
   [tableView deselectRowAtIndexPath:[tableView indexPathForSelectedRow] animated:NO];
   
   if (!myItem->isValid || myItem->rank < 1)
      return;

   
   if (indexPath.row == 0) {
      iPath = [NSIndexPath indexPathForRow:myItem->rank inSection:1];
      [tableView scrollToRowAtIndexPath:iPath atScrollPosition:UITableViewScrollPositionMiddle animated:NO];
   }
}

- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath {
   /*
   if (indexPath.section != 0)
      return NULL;
   else
      return indexPath;
    */
   
   return NULL;
}


//////////////////////////////
// UIScrollViewDelegate
- (void)scrollViewDidEndDecelerating:(UIScrollView *)scrollView {
   //[super scrollViewDidEndDecelerating:scrollView];
   
   //roadmap_log (ROADMAP_DEBUG, "Did end decelerating");
   if (lastRank != -1) {
      roadmap_scoreboard_request_rows(lastRank, request_rows_cb);
      lastRank = -1;
   }
}


@end

