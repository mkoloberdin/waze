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
#include <stdio.h>   //   _snprintf

#include "RealtimeDefs.h"

#include "ssd/ssd_dialog.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_popup.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_keyboard_dialog.h"
#include "ssd/ssd_progress_msg_dialog.h"
#include "roadmap_lang.h"
#include "roadmap_screen.h"
#include "Realtime/Realtime.h"
#include "Realtime/RealtimeAlerts.h"
#include "roadmap_navigate.h"
#include "roadmap_gps.h"
#include "roadmap_trip.h"
#include "roadmap_math.h"
#include "roadmap_mood.h"
#include "roadmap_layer.h"
#include "roadmap_messagebox.h"

#ifdef IPHONE
#include "iphone/roadmap_editbox.h"
#endif //IPHONE

#include "roadmap_analytics.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

static RoadMapConfigDescriptor RoadMapConfigDisclaimerShown =
      ROADMAP_CONFIG_ITEM("Ping a wazer", "Disclaimer Shown");


//////////////////////////////////////////////////////////////////////////////////////////////////
static PFN_ONUSER gs_pfnOnAddUser   = NULL;
static PFN_ONUSER gs_pfnOnMoveUser  = NULL;
static PFN_ONUSER gs_pfnOnRemoveUser= NULL;

static LPRTUserLocation g_user;
//////////////////////////////////////////////////////////////////////////////////////////////////
static int ValueRanges [] = {0,10,50,100,200,300,400,500,1000,5000,10000,50000,100000,1000000,10000000,100000000};
static  void prepareValueString ( int iRank, char * resultString,const char * nickName);
#define MAX_SIZE_RANGE_STR  30

static const char* ANALYTICS_EVENT_PING_NAME = "PING_A_WAZER";

//////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL DisclaimerShown(void){
   if (0 == strcmp (roadmap_config_get (&RoadMapConfigDisclaimerShown), "yes"))
      return TRUE;
   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void DisclaimerDisplayed(void){
   roadmap_config_set (&RoadMapConfigDisclaimerShown, "yes");
   roadmap_config_save(FALSE);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void RTUserLocation_Init( LPRTUserLocation this)
{
   this->iID              	= RT_INVALID_LOGINID_VALUE;
   this->position.longitude= 0;
   this->position.latitude = 0;
   this->iAzimuth         	= 0;
   this->fSpeed           	= 0.F;
   this->iLastAccessTime	= 0;
   this->bWasUpdated      	= FALSE;
   this->iMood            	= -1;
   this->iAddon           	= -1;
   this->iStars           	= -1;
   this->iRank            	= -1;
   this->iPoints          	= -1;
   this->iJoinDate      	= 0;
   this->iPingFlag         = 0;
   memset( this->sName, 0, sizeof(this->sName));
   memset( this->sGUIID,0, sizeof(this->sGUIID));
   memset( this->sTitle, 0, sizeof(this->sTitle));
}

void RTUserLocation_CreateGUIID( LPRTUserLocation this)
{ snprintf( this->sGUIID, RT_USERID_MAXSIZE, "Friend_%d", this->iID);}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void RTUsers_Init(LPRTUsers   this,
                  PFN_ONUSER  pfnOnAddUser,
                  PFN_ONUSER  pfnOnMoveUser,
                  PFN_ONUSER  pfnOnRemoveUser)
{
   int i;
   roadmap_config_declare_enumeration ("user", &RoadMapConfigDisclaimerShown, NULL, "no",
                  "yes", NULL);

   for( i=0; i<RL_MAXIMUM_USERS_COUNT; i++)
      RTUserLocation_Init( &(this->Users[i]));

   this->iCount      = 0;
   gs_pfnOnAddUser   = pfnOnAddUser;
   gs_pfnOnMoveUser  = pfnOnMoveUser;
   gs_pfnOnRemoveUser= pfnOnRemoveUser;
}

void RTUsers_Reset(LPRTUsers   this)
{
   int i;

   for( i=0; i<RL_MAXIMUM_USERS_COUNT; i++)
      RTUserLocation_Init( &(this->Users[i]));

   this->iCount = 0;
}

void RTUsers_Term( LPRTUsers this)
{
   RTUsers_ClearAll( this);
   gs_pfnOnAddUser     = NULL;
   gs_pfnOnMoveUser    = NULL;
   gs_pfnOnRemoveUser  = NULL;
}

int RTUsers_Count( LPRTUsers this)
{ return this->iCount;}

BOOL RTUsers_IsEmpty( LPRTUsers this)
{ return (0 == this->iCount);}

BOOL RTUsers_Add( LPRTUsers this, LPRTUserLocation pUser)
{
   assert(gs_pfnOnAddUser);

   //   Full?
   if( RL_MAXIMUM_USERS_COUNT == this->iCount)
      return FALSE;

   //   Already exists?
   if( RTUsers_Exists( this, pUser->iID))
      return FALSE;

   this->Users[this->iCount]   = (*pUser);
   this->Users[this->iCount].bWasUpdated= TRUE;

   this->iCount++;
   gs_pfnOnAddUser( pUser);

   return TRUE;
}

BOOL RTUsers_Update( LPRTUsers this, LPRTUserLocation pUser)
{
   LPRTUserLocation pUI = RTUsers_UserByID( this, pUser->iID);

   assert(gs_pfnOnMoveUser);

   if( !pUI)
      return FALSE;

   (*pUI) = (*pUser);
   gs_pfnOnMoveUser( pUser);
   pUI->bWasUpdated = TRUE;
   return TRUE;
}

BOOL RTUsers_UpdateOrAdd( LPRTUsers this, LPRTUserLocation pUser)
{
   if( !RTUsers_Update( this, pUser) && !RTUsers_Add( this, pUser))
      return FALSE;

   pUser->bWasUpdated = TRUE;
   return TRUE;
}

BOOL  RTUsers_RemoveByIndex( LPRTUsers this, int iIndex)
{
   int i;

   assert(gs_pfnOnRemoveUser);

   //   Are we empty?
   if( (iIndex < 0) || (this->iCount <= iIndex))
      return FALSE;

   gs_pfnOnRemoveUser( &(this->Users[iIndex]));

   for( i=iIndex; i<(this->iCount-1); i++)
      this->Users[i] = this->Users[i+1];

   this->iCount--;
   RTUserLocation_Init( &(this->Users[this->iCount]));

   return TRUE;
}

BOOL RTUsers_RemoveByID( LPRTUsers this, int iUserID)
{
   int   i;

   //   Are we empty?
   if( 0 == this->iCount)
      return FALSE;

   for( i=0; i<this->iCount; i++)
      if( this->Users[i].iID == iUserID)
      {
         RTUsers_RemoveByIndex( this, i);
         return TRUE;
      }

   return FALSE;
}

BOOL RTUsers_Exists( LPRTUsers this, int iUserID)
{
   if( NULL == RTUsers_UserByID( this, iUserID))
      return FALSE;

   return TRUE;
}

void RTUsers_ClearAll( LPRTUsers this)
{
   int i;

   assert(gs_pfnOnRemoveUser);

   //   Find user:
   for( i=0; i<this->iCount; i++)
   {
      LPRTUserLocation pUI = &(this->Users[i]);

      gs_pfnOnRemoveUser( pUI);
      RTUserLocation_Init( pUI);
   }

   this->iCount = 0;

}

void RTUsers_ResetUpdateFlag( LPRTUsers this)
{
   int i;

   //   Find user:
   for( i=0; i<this->iCount; i++)
      this->Users[i].bWasUpdated = FALSE;
}

void RTUsers_RedoUpdateFlag( LPRTUsers this)
{
   int i;

   //   Find user:
   for( i=0; i<this->iCount; i++)
      this->Users[i].bWasUpdated = TRUE;
}

void RTUsers_RemoveUnupdatedUsers( LPRTUsers this, int* pUpdatedCount, int* pRemovedCount)
{
   int i;

   (*pUpdatedCount) = 0;
   (*pRemovedCount) = 0;

   for( i=0; i<this->iCount; i++)
      if( this->Users[i].bWasUpdated)
         (*pUpdatedCount)++;
      else
      {
         RTUsers_RemoveByIndex( this, i);
         (*pRemovedCount)++;
         i--;
      }
}

LPRTUserLocation RTUsers_User( LPRTUsers this, int iIndex)
{
   if( (0 <= iIndex) && (iIndex < this->iCount))
      return &(this->Users[iIndex]);
   //   Else
   return NULL;
}

LPRTUserLocation RTUsers_UserByID( LPRTUsers this, int iUserID)
{
   int i;

   //   Find user:
   for( i=0; i<this->iCount; i++)
      if( this->Users[i].iID == iUserID)
         return &(this->Users[i]);

   return NULL;
}


static LPRTUserLocation RTUsers_UserByGUIID( LPRTUsers this, const char *id)
{
   int i;

   //   Find user:
   for( i=0; i<this->iCount; i++)
      if(strcmp(this->Users[i].sGUIID, id) == 0)
         return &(this->Users[i]);

   return NULL;
}
static void RTUsers_get_speed_str( LPRTUsers this, const char *id, char* buf, int buf_len)
{
   LPRTUserLocation user;
   int speed;

   char str[100];
   buf[0] = 0;
   user = RTUsers_UserByGUIID(this, id);
    if (!user) {
       return;
    }
    speed = roadmap_math_to_speed_unit(user->fSpeed);

    if (speed < 10)
       snprintf (str, sizeof(str), "%s", roadmap_lang_get("less than 10"));
    else if (speed < 40)
       snprintf (str, sizeof(str), "%s", roadmap_lang_get("around 25"));
    else
       snprintf (str, sizeof(str), "%s",roadmap_lang_get("over 40"));

    snprintf( buf, buf_len, "%s: %s %s", roadmap_lang_get("Speed"), str, roadmap_lang_get(roadmap_math_speed_unit()));
}

static void RTUsers_get_distance_str( LPRTUsers this, const char *id, char* buf, int buf_len)
{
   RoadMapPosition position, current_pos;
   const RoadMapPosition *GpsPosition;
   int distance;
   RoadMapGpsPosition CurrentPosition;
   PluginLine line;
   int Direction;
   int distance_far;
   char str[100];
   char unit_str[20];
   int tenths;
   LPRTUserLocation user;
   buf[0] = 0;

   user = RTUsers_UserByGUIID(this, id);
   if (!user) {
      return;
   }

   position = user->position;

   // check the distance to the user
   if ( roadmap_navigate_get_current( &CurrentPosition, &line, &Direction ) != -1 )
   {

      current_pos.latitude = CurrentPosition.latitude;
      current_pos.longitude = CurrentPosition.longitude;
      distance = roadmap_math_distance( &current_pos, &position );
      distance_far = roadmap_math_to_trip_distance( distance );
   }
   else
   {
      GpsPosition = roadmap_trip_get_position("Location");
      if (GpsPosition != NULL)
      {
         current_pos.latitude = GpsPosition->latitude;
         current_pos.longitude = GpsPosition->longitude;
         distance = roadmap_math_distance( &current_pos, &position );
         distance_far = roadmap_math_to_trip_distance( distance );
      }
      else
        	return; // Can not compute distance return
   }

   tenths = roadmap_math_to_trip_distance_tenths( distance );
   snprintf( str, sizeof(str), "%d.%d", distance_far, tenths % 10 );
   snprintf( unit_str, sizeof( unit_str ), "%s", roadmap_math_trip_unit());

   snprintf( buf, buf_len, roadmap_lang_get("%s %s Away"), str, roadmap_lang_get(unit_str));
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Zoomin map to the alert.
 * @param UserPosition - The position of the alert
 * @param iCenterAround - Whether to center around the alert or the GPS position
 * @return None
 */
static void RTUsers_zoom(RoadMapPosition UserPosition, int iCenterAround)
{
   int distance;
   RoadMapGpsPosition CurrentPosition;
   RoadMapPosition pos;
   PluginLine line;
   int Direction;
   int scale;

   if (iCenterAround == RT_USERS_CENTER_ON_ALERT)
   {
      roadmap_trip_set_point("Hold", &UserPosition);
      roadmap_screen_update_center(&UserPosition);
      scale = 1000;
      //gCentered = TRUE;

   } else {

      //gCentered = FALSE;
      if (roadmap_navigate_get_current(&CurrentPosition, &line, &Direction) != -1)
      {

         pos.latitude = CurrentPosition.latitude;
         pos.longitude = CurrentPosition.longitude;
         distance = roadmap_math_distance(&pos, &UserPosition);

         if (distance < 1000)
            scale = 1000;
         else if (distance < 2000)
            scale = 1500;
         else if (distance < 5000)
            scale = 2500;
         else
            scale = -1;

         if (scale == -1) {
            scale = 5000;
         }
         roadmap_screen_update_center(&pos);
      } else {
         scale = 5000;
         roadmap_screen_update_center(&UserPosition);
      }
   }

   roadmap_math_set_scale(scale, roadmap_screen_height() / 3);
   roadmap_layer_adjust();
   roadmap_screen_set_orientation_fixed();
}

static int on_close (SsdWidget widget, const char *new_value){
   if (ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), "UsersPopUPDlg")))
      ssd_dialog_hide_current(dec_close);

   return 0;
}

static BOOL post_comment_keyboard_callback(int         exit_code,
                                          const char* value,
                                          void*       context)
{
    BOOL success;
    LPRTUserLocation user = (LPRTUserLocation)context;
    RoadMapGpsPosition position;
    
#ifdef IPHONE_NATIVE
	roadmap_main_show_root(0);
#endif //IPHONE_NATIVE
   
    if( dec_ok != exit_code)
        return TRUE;

   if (value[0] == 0)
      return FALSE;

   ssd_progress_msg_dialog_show( roadmap_lang_get( "Sending Ping . . ." ) );
   position.latitude = user->position.latitude;
   position.longitude = user->position.longitude;
   position.steering = user->iAzimuth;

   success = Realtime_PinqWazer(&position, -1, -1, user->iID, RT_ALERT_TYPE_CHIT_CHAT, value, NULL, FALSE );
   if (success){
       ssd_dialog_hide_all(dec_ok);
   }   
   
   roadmap_analytics_log_event(ANALYTICS_EVENT_PING_NAME, NULL, NULL);
   
   return TRUE;
}
static LPRTUserLocation g_user;

static void disclaimer_cb( int exit_code ){
#if (defined(__SYMBIAN32__) && !defined(TOUCH_SCREEN))
    ShowEditbox(roadmap_lang_get("Chit chat"), "", post_comment_keyboard_callback,
          g_user, EEditBoxEmptyForbidden | EEditBoxAlphaNumeric );
#else
   ssd_show_keyboard_dialog(  roadmap_lang_get("Chit chat"),
                              NULL,
                              post_comment_keyboard_callback,
                              g_user);
#endif
}

static int ping (LPRTUserLocation user){

   if (Realtime_is_random_user()){
      roadmap_messagebox_timeout("Error","You need to be a registered user in order to send Pings. Register in 'Settings > Profile'", 8);
      return 0;
   }

   if (!DisclaimerShown()){
      g_user = user;
      roadmap_messagebox_cb("","Be good! Your message will pop on this user's screen but is also seen publicly in chit chat.",disclaimer_cb);
      DisclaimerDisplayed();
      return 0;
   }
   
#if (defined(__SYMBIAN32__) && !defined(TOUCH_SCREEN) || defined(IPHONE_NATIVE))
    ShowEditbox(roadmap_lang_get("Chit chat"), "", post_comment_keyboard_callback,
            user, EEditBoxEmptyForbidden | EEditBoxAlphaNumeric );
#else
   ssd_show_keyboard_dialog(  roadmap_lang_get("Chit chat"),
                              NULL,
                              post_comment_keyboard_callback,
                              user);
#endif

   return 1;
}

static int on_ping (SsdWidget widget, const char *new_value){
   LPRTUserLocation user;
   if (!widget)
      return 0;

   user = (LPRTUserLocation)widget->context;
   ping(user);

   return 1;
}

#ifndef TOUCH_SCREEN
int  on_sk_ping(SsdWidget widget, const char *new_value, void *context){
   if (!g_user)
      return;
   ping(g_user);
   return 1;
}
#endif

void RTUsers_Popup (LPRTUsers this, const char *id, int iCenterAround)
{
   SsdWidget text, position_con, image_con;
   RoadMapPosition position;
   static SsdWidget popup;
   SsdWidget stars_icon;
   SsdWidget mood_icon;
   SsdSize size;
   SsdWidget spacer;
#ifdef TOUCH_SCREEN
   SsdWidget button;
#endif

   char name[100];
   char title[100];
   char rates[200];
   char joined[200];
   char distance[100];
   char ping[200];
   time_t now;
   int timeDiff;
   struct tm  ts_now;
   struct tm  ts_joined;
   int delta;
   const char *mood_str;
   LPRTUserLocation user;
   int width = roadmap_canvas_width();
   char RankRangeStr[MAX_SIZE_RANGE_STR];
   char PointRangeStr[MAX_SIZE_RANGE_STR];
   int  image_cont_width = 70;

#ifndef TOUCH_SCREEN
   g_user = NULL;
#endif
   if (width > roadmap_canvas_height())
#ifdef IPHONE
      width = 320;
#else
   width = roadmap_canvas_height();
#endif // IPHONE
   
   user = RTUsers_UserByGUIID(this, id);
   if (!user) {
      return;
   }

   if (roadmap_screen_is_hd_screen ()){
      image_cont_width = 100;
   }
   if (ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), "UsersPopUPDlg")))
      ssd_dialog_hide_current(dec_cancel);

   position = user->position;

   if (iCenterAround != RT_USERS_CENTER_NONE)
      RTUsers_zoom(position, iCenterAround);

   mood_str = roadmap_mood_to_string(user->iMood);
   mood_icon = ssd_bitmap_new("mood_icon", mood_str, SSD_ALIGN_CENTER);
   free((void *)mood_str);

   ssd_widget_get_size(mood_icon, &size, NULL);

   position_con = ssd_container_new ("position_container", "", width-size.width-image_cont_width-10, SSD_MIN_SIZE,0);
   ssd_widget_set_color(position_con, NULL, NULL);


   //name
   if (strcmp(user->sName, "") == 0) {
      strncpy_safe(name, roadmap_lang_get("anonymous"), sizeof(name));
   } else {
      strncpy_safe(name, user->sName, sizeof(name));
   }
   text = ssd_text_new("user_name", name, 18, 0);
	ssd_widget_set_color(text,"#f6a201", NULL);
	ssd_widget_add(position_con, text);

	ssd_dialog_add_hspace(position_con, 6, 0);
	//stars
   stars_icon = ssd_bitmap_new("stars_icon",RTAlerts_Get_Stars_Icon(user->iStars), SSD_END_ROW);
   ssd_widget_add(position_con, stars_icon);

   //title
   if (strcmp(user->sTitle, "") != 0) {
      strncpy_safe(title, user->sTitle, sizeof(title));
      text = ssd_text_new("title", title, 14, SSD_END_ROW);
      ssd_widget_set_color(text,"#ffffff", NULL);
      ssd_widget_add(position_con, text);
   }


   //rates
   if (user->iRank > 0) {
   	  prepareValueString(user->iRank,RankRangeStr,name);
   	  prepareValueString(user->iPoints,PointRangeStr,name);
   	  if (user->iRank<1000){
   	  	 snprintf(rates, sizeof(rates),
               "%s %s, %s: %s", PointRangeStr, roadmap_lang_get("pt."), roadmap_lang_get("Rank"), RankRangeStr);
   	  }else{
      snprintf(rates, sizeof(rates),
               "%s %s, %s: %s", PointRangeStr, roadmap_lang_get("pt."), roadmap_lang_get("Rank"), RankRangeStr);
   	  }

      text = ssd_text_new("rates", rates, 14, SSD_END_ROW|SSD_TEXT_NORMAL_FONT);
      ssd_widget_set_color(text,"#ffffff", NULL);
      ssd_widget_add(position_con, text);
   }

   //joined
   now = time(NULL);
   timeDiff = (int) difftime( now, (time_t) user->iJoinDate );
   ts_now = *localtime(&now);
   ts_joined = *localtime((time_t*) &user->iJoinDate);


   snprintf( joined, sizeof(joined),
            "%s ", roadmap_lang_get("Joined"));
   if (ts_now.tm_year - ts_joined.tm_year == 2)
      snprintf( joined + strlen(joined), sizeof(joined) - strlen(joined), "%s",
               roadmap_lang_get("one year ago"));
   else if (ts_now.tm_year - ts_joined.tm_year > 2)
      snprintf( joined + strlen(joined), sizeof(joined) - strlen(joined),
               roadmap_lang_get("%d years ago"), ts_now.tm_year - ts_joined.tm_year -1);
   else if (ts_now.tm_mon != ts_joined.tm_mon){
      if (ts_now.tm_mon > ts_joined.tm_mon)
         delta = ts_now.tm_mon - ts_joined.tm_mon;
      else
         delta = ts_now.tm_mon + 12 - ts_joined.tm_mon;

      if (delta == 1)
         snprintf( joined + strlen(joined), sizeof(joined) - strlen(joined), "%s",
                  roadmap_lang_get("one month ago"));
      else
         snprintf( joined + strlen(joined), sizeof(joined) - strlen(joined),
                  roadmap_lang_get("%d months ago"), delta );
   } else
      snprintf( joined + strlen(joined), sizeof(joined) - strlen(joined),
               roadmap_lang_get("%d days ago"), timeDiff / (60*60*24));

   text = ssd_text_new("joined", joined, 14, SSD_END_ROW|SSD_TEXT_NORMAL_FONT);
   ssd_widget_set_color(text,"#ffffff", NULL);
   ssd_widget_add(position_con, text);

   //Speed
   distance[0] = 0;
   RTUsers_get_speed_str(this, id, distance, sizeof(distance));
   text = ssd_text_new("speed", distance, 14, SSD_END_ROW|SSD_TEXT_NORMAL_FONT);
   ssd_widget_set_color(text,"#ffffff", NULL);
   ssd_widget_add(position_con, text);

   if (user->iPingFlag == RT_USERS_PING_FLAG_ALLOW) {
      strncpy_safe(ping, roadmap_lang_get("This wazer agrees to be pinged"), sizeof(ping));
   } else if (user->iPingFlag == RT_USERS_PING_FLAG_OLD_VER) {
      strncpy_safe(ping, roadmap_lang_get("This wazer's version doesn't support ping"), sizeof(ping));
   } else {
      strncpy_safe(ping, roadmap_lang_get("This wazer's ping feature is turned off"), sizeof(ping));
   }
   ssd_dialog_add_hspace(position_con, 1, SSD_END_ROW);

   text = ssd_text_new("Ping Text", ping, 12, SSD_END_ROW);
   ssd_widget_set_color(text,"#f6a201", NULL);
   ssd_widget_add(position_con, text);

   popup = ssd_popup_new("UsersPopUPDlg", "", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE, &position, SSD_POINTER_LOCATION|SSD_ROUNDED_BLACK);

   ssd_dialog_add_vspace(popup, 2,0);
   image_con =
     ssd_container_new ("IMAGE_container", "", image_cont_width, SSD_MIN_SIZE, SSD_ALIGN_RIGHT);
   ssd_widget_set_color(image_con, NULL, NULL);

   //mood
   ssd_widget_add(image_con, mood_icon);


   //distance
   RTUsers_get_distance_str(this, id, distance, sizeof(distance));
   text = ssd_text_new("distance", distance, 14, SSD_ALIGN_CENTER);
   ssd_widget_set_color(text,"#ffffff", NULL);
   ssd_widget_add(image_con, text);

   ssd_widget_add(popup, image_con);

   ssd_widget_add(popup, position_con);


#ifdef TOUCH_SCREEN
   spacer = ssd_container_new( "space", "", SSD_MIN_SIZE, 5, SSD_END_ROW );
   ssd_widget_set_color( spacer, NULL, NULL );
   ssd_widget_add( popup, spacer );

   button = ssd_button_label("Close_button", roadmap_lang_get("Close"), SSD_ALIGN_CENTER|SSD_WS_DEFWIDGET|SSD_WS_TABSTOP, on_close);
   ssd_widget_add(popup, button);

   button = ssd_button_label("Ping_button", roadmap_lang_get("Ping"), SSD_ALIGN_CENTER|SSD_WS_TABSTOP, on_ping);
   if (user->iPingFlag == RT_USERS_PING_FLAG_ALLOW)
      ssd_button_enable(button);
   else
      ssd_button_disable(button);
   ssd_widget_set_context(button, (void *)user);
   ssd_widget_add(popup, button);
#else
   if (user->iPingFlag == RT_USERS_PING_FLAG_ALLOW){
      g_user = user;
      ssd_widget_set_left_softkey_callback(popup->parent, on_sk_ping);
      ssd_widget_set_left_softkey_text(popup->parent, roadmap_lang_get("Ping"));
   }
#endif //TOUCH_SCREEN

   ssd_dialog_activate("UsersPopUPDlg", NULL);

   if (!roadmap_screen_refresh()){
      roadmap_screen_redraw();
   }

   Realtime_SendCurrentViewDimentions();

}

/*
 * In - value : the given value that needs to be converted into range form
 * In - resultString : a preallocated string, to hold the correct range form.
 * example - Value = 9444, resultString will get 5k-10k.
 * The conversion is done according to the input ranges, declated in the static ValueRanges array,
 */

static void prepareValueString ( int value, char * resultString, const char * nickName){
	unsigned int i;
	int lowVal;
	int highVal;
	assert(value>=0);
	if (strcmp(nickName,roadmap_lang_get("anonymous")))
	{
		snprintf(resultString, MAX_SIZE_RANGE_STR, "%d",value);
		return;
	}
	for(i = 0; i<(sizeof(ValueRanges)/sizeof(int));i++){ // find the right ValueRanges
		if (value < ValueRanges[i])
			break;
	}
	lowVal = ValueRanges[i-1];
	highVal = ValueRanges[i];
	if ( lowVal > 1000000)
		snprintf(resultString, MAX_SIZE_RANGE_STR, "%dm-%dm",lowVal/1000000,highVal/1000000);
	else if (lowVal > 1000)
		snprintf(resultString, MAX_SIZE_RANGE_STR, "%dk-%dk",lowVal/1000,highVal/1000);
	else
		snprintf(resultString, MAX_SIZE_RANGE_STR, "%d-%d",lowVal,highVal);
}
