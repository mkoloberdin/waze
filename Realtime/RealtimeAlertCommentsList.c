/* RealtimeAlertsCommentsList.c - manage the Real Time Alerts Commemnts list display
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
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
 *   See RealtimeAlertsCommentsList.h
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_history.h"
#include "roadmap_locator.h"
#include "roadmap_street.h"
#include "roadmap_lang.h"
#include "roadmap_messagebox.h"
#include "roadmap_geocode.h"
#include "roadmap_config.h"
#include "roadmap_trip.h"
#include "roadmap_county.h"
#include "roadmap_display.h"
#include "roadmap_math.h"
#include "roadmap_navigate.h"
#include "roadmap_mood.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_generic_list_dialog.h"
#include "ssd/ssd_contextmenu.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_container.h"

#include "RealtimeAlerts.h"
#include "Realtime.h"
#include "RealtimeAlertsList.h"
#include "RealtimeAlertCommentsList.h"

#define MAX_COMMENTS_ENTRIES 100


#ifndef TOUCH_SCREEN

static void on_option_add_comment();

// Context menu:
static ssd_cm_item      context_menu_items[] =
{
   SSD_CM_INIT_ITEM  ( "Show on map",  rtcl_cm_show),
   SSD_CM_INIT_ITEM  ( "Add comment", rtcl_cm_add_comments),
   SSD_CM_INIT_ITEM  ( "Exit_key",     rtcl_cm_exit)
};

static ssd_contextmenu  context_menu = SSD_CM_INIT_MENU( context_menu_items);
static   BOOL                    g_context_menu_is_active= FALSE;
#endif

typedef struct
{

    const char *title;
    int alert_id;

} RoadMapRealTimeAlertListCommentsDialog;

typedef struct
{

    char *value;
    char *label;
} RoadMapRealTimeAlertCommentsList;

static int g_Alert_Id;
#ifndef TOUCH_SCREEN 
static void on_option_add_comment();
///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_show(){
	 ssd_generic_list_dialog_hide_all();
    RTAlerts_Popup_By_Id(g_Alert_Id);
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_selected(  BOOL              made_selection,
                                 ssd_cm_item_ptr   item,
                                 void*             context)
{
   real_time_list_context_menu_items   selection;
   int                                 exit_code = dec_ok;

   g_context_menu_is_active = FALSE;

   if( !made_selection)
      return;

   selection = (real_time_list_context_menu_items)item->id;

   switch( selection)
   {
      case rtcl_cm_show:
      	on_option_show();
         break;

      case rtcl_cm_add_comments:
         on_option_add_comment();
         break;


      case rtcl_cm_exit:
         exit_code = dec_cancel;
      	ssd_dialog_hide_all( exit_code);
      	roadmap_screen_refresh ();
         break;

      default:
         break;
   }
}

///////////////////////////////////////////////////////////////////////////////////////////
static int on_options(SsdWidget widget, const char *new_value, void *context)
{
	int   menu_x;

   if(g_context_menu_is_active)
   {
      ssd_dialog_hide_current(dec_cancel);
      g_context_menu_is_active = FALSE;
   }

   if  (ssd_widget_rtl (NULL))
	   menu_x = SSD_X_SCREEN_RIGHT;
	else
		menu_x = SSD_X_SCREEN_LEFT;

   ssd_context_menu_show(  menu_x,   // X
                           SSD_Y_SCREEN_BOTTOM, // Y
                           &context_menu,
                           on_option_selected,
                           NULL,
                           dir_default,
                           0);

   g_context_menu_is_active = TRUE;

   return 0;
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_add_comment(){
    real_time_post_alert_comment_by_id(g_Alert_Id);
}


///////////////////////////////////////////////////////////////////////////////////////////
static int AlertSelected( SsdWidget this, const char *new_value)
{


    ssd_dialog_hide_all(dec_close);
    RTAlerts_Popup_By_Id(g_Alert_Id);

    return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////
static int PostCommentSelected( SsdWidget this, const char *new_value)
{
    on_option_add_comment();
    return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////
int RealtimeAlertCommentsList(int iAlertId)
{
   int SavedZoom = -1;
   RoadMapPosition SavedPosition;
   SsdWidget dialog;
   int timeDiff;
   time_t now;
   SsdWidget group, container, image_con, text_con;
   SsdWidget bitmap;
   SsdWidget text;
   int iNumberOfComments = -1;
   int iCount = 0;
   RTAlert *alert;
   char AlertStr[300];
   char CommentStr[300];
   char ReportedByStr[100];
   char DescriptionStr[300];
   char str[100];
   int distance;
   RoadMapGpsPosition CurrentPosition;
   RoadMapPosition position, current_pos;
   const RoadMapPosition *gps_pos;
   PluginLine line;
   int Direction;
   int distance_far;
   char dist_str[100];
   char unit_str[20];
   char DistanceStr[250];
   const char *myUserName;

   static RoadMapRealTimeAlertListCommentsDialog context =
   { "Real Time Alert Comments", -1 };

   RTAlertCommentsEntry *CommentEntry;

   dialog = ssd_dialog_new ("alert_comments",
                             roadmap_lang_get("Real Time Alerts Comments"),
                             NULL,
                             SSD_CONTAINER_TITLE);
#ifndef TOUCH_SCREEN
   ssd_widget_set_left_softkey_text(dialog, roadmap_lang_get("Options"));
   ssd_widget_set_left_softkey_callback(dialog, on_options);
#endif
   group = ssd_container_new ("alert_comments.container", NULL,
                               SSD_MAX_SIZE,90,SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_WIDGET_SPACE|SSD_CONTAINER_BORDER|SSD_WS_TABSTOP);
    


   AlertStr[0] = 0;
   DistanceStr[0] = 0;
   str[0] = 0;
   ReportedByStr[0] = 0;


   iNumberOfComments = RTAlerts_Get_Number_of_Comments(iAlertId);
   if (iNumberOfComments == 0)
   {
       return -1;
   }


   alert = RTAlerts_Get_By_ID(iAlertId);
   position.longitude = alert->iLongitude;
   position.latitude = alert->iLatitude;
   context.alert_id = iAlertId;
   g_Alert_Id = iAlertId;

   roadmap_math_get_context(&SavedPosition, &SavedZoom);

   roadmap_math_set_context((RoadMapPosition *)&position, 20);

   if (roadmap_navigate_get_current(&CurrentPosition, &line, &Direction) == -1)
   {
       // check the distance to the alert
       gps_pos = roadmap_trip_get_position("GPS");
       if (gps_pos != NULL)
       {
           current_pos.latitude = gps_pos->latitude;
           current_pos.longitude = gps_pos->longitude;
       }
       else
       {
           current_pos.latitude = -1;
           current_pos.longitude = -1;
       }
   }
   else
   {
       current_pos.latitude = CurrentPosition.latitude;
       current_pos.longitude = CurrentPosition.longitude;
   }

   if ((current_pos.latitude != -1) && (current_pos.longitude != -1))
   {
       distance = roadmap_math_distance(&current_pos, &position);
       distance_far = roadmap_math_to_trip_distance(distance);

       if (distance_far > 0)
       {
           int tenths = roadmap_math_to_trip_distance_tenths(distance);
           snprintf(dist_str, sizeof(str), "%d.%d", distance_far, tenths % 10);
           snprintf(unit_str, sizeof(unit_str), "%s",
                   roadmap_lang_get(roadmap_math_trip_unit()));
       }
       else
       {
           snprintf(dist_str, sizeof(str), "%d", roadmap_math_distance_to_current(distance));
           snprintf(unit_str, sizeof(unit_str), "%s",
                   roadmap_lang_get(roadmap_math_distance_unit()));
       }
   }

   roadmap_math_set_context(&SavedPosition, SavedZoom);
   snprintf(DistanceStr + strlen(DistanceStr), sizeof(DistanceStr) - strlen(DistanceStr),
           "%s", alert->sLocationStr);

   now = time(NULL);
   timeDiff = (int)difftime(now, (time_t)alert->iReportTime);
   if (timeDiff <0)
      timeDiff = 0;

   if (alert->iType == RT_ALERT_TYPE_TRAFFIC_INFO)
     snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
               - strlen(AlertStr),"%s%s",
               NEW_LINE,roadmap_lang_get("Updated "));
   else
     snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr),"%s%s",
                NEW_LINE, roadmap_lang_get("Reported "));


   if (timeDiff < 60)
      snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
               - strlen(AlertStr),
                   roadmap_lang_get("%d seconds ago"), timeDiff);
   else if ((timeDiff > 60) && (timeDiff < 3600))
      snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
               - strlen(AlertStr),
               roadmap_lang_get("%d minutes ago"), timeDiff/60);
   else
       snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr),
                roadmap_lang_get("%2.1f hours ago"), (float)timeDiff
                /3600);

   // Display who reported the alert
   if (alert->sReportedBy[0] != 0){
     snprintf(ReportedByStr, sizeof(ReportedByStr),
                 " %s %s  ", roadmap_lang_get("by"), alert->sReportedBy);
   }
  
   DescriptionStr[0] = 0;
   if (alert->sDescription[0] != 0){
      //Display the alert description
      snprintf(DescriptionStr, sizeof(DescriptionStr),
                "%s", alert->sDescription);

   }

   image_con = ssd_container_new ("icon_container", NULL, 50,
          SSD_MIN_SIZE,  SSD_ALIGN_VCENTER);
   ssd_widget_set_color(image_con, NULL, NULL);
   bitmap = ssd_bitmap_new("AlertIcon", RTAlerts_Get_Icon(alert->iID), SSD_ALIGN_CENTER);
   ssd_widget_add(image_con, bitmap);
   text = ssd_text_new ("Alert Disatnce", dist_str, 16,SSD_ALIGN_CENTER|SSD_END_ROW);
   ssd_widget_add(image_con, text);

   text = ssd_text_new ("Alert Disatnce", unit_str, 14,SSD_ALIGN_CENTER);
   ssd_widget_set_color(text, "#383838", NULL);
   ssd_widget_add(image_con, text);
   ssd_widget_add(group, image_con);

   text_con = ssd_container_new ("icon_container", NULL, SSD_MIN_SIZE,
          SSD_MIN_SIZE, 0);
   ssd_widget_set_color(text_con, NULL, NULL);

   text = ssd_text_new("Distance_str", DistanceStr, 16,SSD_END_ROW);
   ssd_widget_set_color(text,"#9d1508", NULL);
   ssd_widget_add(text_con, text);
   snprintf(str, sizeof(str), "%d", alert->iID);
   text = ssd_text_new("AlertTXt",AlertStr,-1,SSD_START_NEW_ROW|SSD_END_ROW);
   ssd_widget_add(text_con, text);
   text = ssd_text_new("ReportedByTxt",ReportedByStr,-1,0);
   ssd_widget_add(text_con, text);
   RTAlerts_update_stars(text_con, alert);
   ssd_widget_add (text_con,
      ssd_container_new ("space_before_descrip", NULL, 0, 10, SSD_END_ROW));
   RTAlerts_show_space_before_desc(text_con,alert);
   text = ssd_text_new("ReportedByTxt",DescriptionStr,-1,0);	
   ssd_widget_add(text_con, text);
   ssd_widget_add(group, text_con);
   if (alert->iMood != 0){
    SsdWidget mood_con, mood;

    mood_con = ssd_container_new("mood_Cont", NULL, 40,40,SSD_ALIGN_RIGHT|SSD_ALIGN_BOTTOM);
    ssd_widget_set_color(mood_con, NULL, NULL);
    mood = ssd_bitmap_new("mood",(void *)roadmap_mood_to_string(alert->iMood),SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER);
    ssd_widget_add(mood_con, mood);
    ssd_widget_add(group, mood_con);
   }
   ssd_widget_add(dialog, group);
   ssd_widget_set_pointer_force_click( group );
   
   group->callback = AlertSelected;

   CommentEntry = alert->Comment;

   iCount++;

   myUserName = RealTime_GetUserName();

   while (iCount < MAX_COMMENTS_ENTRIES && (CommentEntry != NULL))
   {
       time_t now;
       int timeDiff;
       SsdWidget bitmap;
       SsdWidget text_container;
       SsdWidget title_container;

       CommentStr[0] = 0;

       if (CommentEntry->comment.bCommentByMe)
          bitmap = ssd_bitmap_new("CommentSent", "response_bubble2",SSD_END_ROW);
       else
          bitmap = ssd_bitmap_new("CommentSent", "response_bubble1",SSD_ALIGN_RIGHT);

       if (CommentEntry->comment.bCommentByMe)
          text_container = ssd_container_new("text_con", NULL, 188, 68, 0 );
       else
          text_container = ssd_container_new("text_con", NULL, 212, 55, 0 );

       ssd_widget_set_color(text_container, NULL, NULL);
       if (ssd_widget_rtl(NULL)){
          if (CommentEntry->comment.bCommentByMe)
             ssd_widget_set_offset(text_container, 25, 2);
          else
             ssd_widget_set_offset(text_container, 10, 2);
       }
       else{
          if (CommentEntry->comment.bCommentByMe)
             ssd_widget_set_offset(text_container, 10, 2);
          else
             ssd_widget_set_offset(text_container, 25, 2);
       }
       title_container = ssd_container_new("text_con", NULL, SSD_MIN_SIZE, 25, SSD_END_ROW);
       ssd_widget_set_color(title_container, NULL, NULL);

       snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
               - strlen(CommentStr), " %s %s  ", roadmap_lang_get("by"),
               CommentEntry->comment.sPostedBy);
       text = ssd_text_new("CommnetTxt", CommentStr, 16,SSD_ALIGN_VCENTER);
       ssd_widget_set_color(text, "#206892",NULL);
       ssd_widget_add(title_container, text);     
       RTAlerts_add_comment_stars(title_container,  &(CommentEntry->comment));
       CommentStr[0] = 0;

       now = time(NULL);
       timeDiff = (int)difftime(now, (time_t)CommentEntry->comment.i64ReportTime);
       if (timeDiff <0)   timeDiff = 0;

       if (timeDiff < 60)
           snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                   - strlen(CommentStr),
                   roadmap_lang_get("%d seconds ago"), timeDiff);
       else if ((timeDiff > 60) && (timeDiff < 3600))
           snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                   - strlen(CommentStr),
                   roadmap_lang_get("%d minutes ago"), timeDiff/60);
       else
           snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
                   - strlen(CommentStr),
                   roadmap_lang_get("%2.1f hours ago"),
                   (float)timeDiff/3600);


       text = ssd_text_new("CommnetTxt", CommentStr, 10,SSD_END_ROW);
       ssd_widget_set_color(text,"#585858", NULL);
       ssd_widget_add(text_container, title_container);

       ssd_widget_add(text_container, text);
       
       
       CommentStr[0] = 0;
       container = ssd_container_new("AddCommentCont", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_END_ROW);
       ssd_widget_set_color(container, NULL, NULL);
       snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
               - strlen(CommentStr), "  %s",
               CommentEntry->comment.sDescription);
       text = ssd_text_new("CommnetTxt", CommentStr, -1,0);
       ssd_widget_add(container, text);
       ssd_widget_add(text_container, container);
       ssd_widget_add(bitmap, text_container);

       if (CommentEntry->comment.iMood != 0){
         SsdWidget mood_con, mood;

        mood_con = ssd_container_new("mood_Cont", NULL, 32,32,SSD_ALIGN_RIGHT|SSD_ALIGN_BOTTOM);
        ssd_widget_set_color(mood_con, NULL, NULL);
        mood = ssd_bitmap_new("mood",(void *)roadmap_mood_to_string(CommentEntry->comment.iMood),SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER);
        ssd_widget_add(mood_con, mood);
        ssd_widget_add(text_container, mood_con);
       }

       if (CommentEntry->comment.bCommentByMe)
          if (ssd_widget_rtl(NULL))
             container = ssd_container_new("AddCommentCont", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WS_TABSTOP|SSD_END_ROW);
          else
             container = ssd_container_new("AddCommentCont", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WS_TABSTOP|SSD_ALIGN_RIGHT);
       else
          if (ssd_widget_rtl(NULL))
             container = ssd_container_new("AddCommentCont", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WS_TABSTOP|SSD_ALIGN_RIGHT);
          else
             container = ssd_container_new("AddCommentCont", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WS_TABSTOP|SSD_END_ROW);
       ssd_widget_set_color(container, NULL, NULL);
       ssd_widget_add(container, bitmap);
       ssd_widget_add(dialog, container);
       iCount++;
       CommentEntry = CommentEntry->next;
   }
#ifdef TOUCH_SCREEN
   if (ssd_widget_rtl(NULL))
      container = ssd_container_new("AddCommentCont", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WS_TABSTOP|SSD_END_ROW);
   else
      container = ssd_container_new("AddCommentCont", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WS_TABSTOP|SSD_ALIGN_RIGHT);
   ssd_widget_set_color(container, NULL, NULL);
   bitmap = ssd_bitmap_new("CommentSent", "response_bubble2",0);
   text = ssd_text_new("Commnet txt",roadmap_lang_get("Add a comment"),16,SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER);
   ssd_widget_set_color(text, "#206892",NULL);
   ssd_widget_add(bitmap, text);
   container->callback = PostCommentSelected;
   ssd_widget_add(container, bitmap);

   ssd_widget_add(dialog, container);
   ssd_widget_set_pointer_force_click( container );
#endif
    ssd_dialog_activate("alert_comments", NULL);

    return 0;
}

