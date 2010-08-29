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
#include "roadmap_social_image.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_generic_list_dialog.h"
#include "ssd/ssd_contextmenu.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_bitmap.h"
#include "roadmap_res.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_confirm_dialog.h"
#include "ssd/ssd_button.h"

#include "RealtimeAlerts.h"
#include "Realtime.h"
#include "RealtimeAlertsList.h"
#include "RealtimeAlertCommentsList.h"

#define MAX_COMMENTS_ENTRIES 100


static void on_option_add_comment();
static void on_options_report_abuse();

// Context menu:
static ssd_cm_item      context_menu_items[] =
{
   SSD_CM_INIT_ITEM  ( "Show on map",  rtcl_cm_show),
   SSD_CM_INIT_ITEM  ( "Add comment",  rtcl_cm_add_comments),
   SSD_CM_INIT_ITEM  ( "Report abuse", rtcl_cm_report_abuse),
   SSD_CM_INIT_ITEM  ( "Cancel",       rtl_cm_cancel)
};

static ssd_contextmenu  context_menu = SSD_CM_INIT_MENU( context_menu_items);
static   BOOL                    g_context_menu_is_active= FALSE;

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
static int g_Comment_id;


static void on_option_add_comment();
///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_show(){
	 ssd_generic_list_dialog_hide_all();
    RTAlerts_Popup_By_Id(g_Alert_Id,FALSE);
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

      case rtcl_cm_report_abuse:
         on_options_report_abuse();
         break;

      case rtcl_cm_cancel:
         break;

      default:
         break;
   }
}

///////////////////////////////////////////////////////////////////////////////////////////
static int on_options_comments(SsdWidget widget, const char *new_value, void *context)
{
	int   menu_x;
	SsdWidget focus;
	BOOL is_comment = FALSE;

	focus = ssd_dialog_get_focus();
	if (focus && !strcmp(focus->name, "CommentsCont")){
	   RTAlertComment *comment = (RTAlertComment *)focus->context;

	   if (!comment->bCommentByMe){
	      is_comment = TRUE;
	      g_Comment_id = comment->iID;
	   }
	   else{
	      g_Comment_id = -1;
	   }
	}
	else{
	   g_Comment_id = -1;
	}

   if(g_context_menu_is_active)
   {
      ssd_dialog_hide_current(dec_cancel);
      g_context_menu_is_active = FALSE;
   }

   if  (ssd_widget_rtl (NULL))
	   menu_x = SSD_X_SCREEN_RIGHT;
	else
		menu_x = SSD_X_SCREEN_LEFT;

   ssd_contextmenu_show_item( &context_menu,
                              rtcl_cm_report_abuse,
                              is_comment,
                              FALSE);

   ssd_context_menu_show(  menu_x,   // X
                           SSD_Y_SCREEN_BOTTOM, // Y
                           &context_menu,
                           on_option_selected,
                           NULL,
                           dir_default,
                           0,
                           TRUE);

   g_context_menu_is_active = TRUE;

   return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_add_comment(){
    real_time_post_alert_comment_by_id(g_Alert_Id);
}


///////////////////////////////////////////////////////////////////////////////////////////
static void report_abuse_confirm_dlg_callback(int exit_code, void *context){
   if (exit_code == dec_yes){
      Realtime_ReportAbuse(g_Alert_Id, g_Comment_id);
   }
}

///////////////////////////////////////////////////////////////////////////////////////////
static void report_abuse(void){
   ssd_confirm_dialog_custom (roadmap_lang_get("Report abuse"), roadmap_lang_get("Reports by several users will disable this user from reporting"), FALSE,report_abuse_confirm_dlg_callback, NULL, roadmap_lang_get("Report"), roadmap_lang_get("Cancel")) ;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void on_options_report_abuse(){
   report_abuse();
}



///////////////////////////////////////////////////////////////////////////////////////////
static int AlertSelected( SsdWidget this, const char *new_value)
{


    ssd_dialog_hide_all(dec_close);
    RTAlerts_Popup_By_Id(g_Alert_Id,FALSE);

    return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////
static int PostCommentSelected( SsdWidget this, const char *new_value)
{
    on_option_add_comment();
    return 1;
}


///////////////////////////////////////////////////////////////////////////////////////////
static int on_report_abuse (SsdWidget widget, const char *new_value){
   RTAlertComment *comment = (RTAlertComment *)widget->context;
   g_Comment_id = comment->iID;

   report_abuse();
   return 0;
}

static void draw_comment_image (SsdWidget widget, RoadMapGuiRect *rect, int flags){
   RoadMapImage top = NULL;
   RoadMapImage mid = NULL;
   RoadMapImage bottom = NULL;
   RoadMapGuiPoint point;
   int top_height = -1;
   int mid_height = -1;
   int bottom_height = -1;
   int num_items;

   if (!top)
      top = roadmap_res_get ( RES_BITMAP, RES_SKIN, "response_bubble1_top" );

   if (!mid)
      mid = roadmap_res_get ( RES_BITMAP, RES_SKIN, "response_bubble1_mid" );

   if (!bottom)
      bottom = roadmap_res_get ( RES_BITMAP, RES_SKIN, "response_bubble1_bottom" );

   if (!top || !mid || !bottom)
      return;

   if (top_height == -1)
      top_height = roadmap_canvas_image_height(top);

   if (mid_height == -1)
      mid_height = roadmap_canvas_image_height(mid);

   if (bottom_height == -1)
      bottom_height = roadmap_canvas_image_height(bottom);

   if ((flags & SSD_GET_SIZE))
   {
//      if ((rect->maxy - rect->miny) < (top_height+bottom_height))
         //rect->maxy = rect->miny + top_height + bottom_height;
      return;
   }

   point.x = rect->minx;
   point.y = rect->miny;
   roadmap_canvas_draw_image(top, &point, 0, IMAGE_NORMAL);

   point.y = rect->miny+top_height;

   num_items = (rect->maxy - rect->miny - top_height - bottom_height)/mid_height + 5;
   if (num_items > 0){
      int i;
      for (i = 0; i < num_items; i++){
         roadmap_canvas_draw_image(mid, &point, 0, IMAGE_NORMAL);
         point.y += mid_height;
      }
   }
   else if (num_items < -10){
      point.y -= 10;
   }

   roadmap_canvas_draw_image(bottom, &point, 0, IMAGE_NORMAL);
}

static int get_width(){
   RoadMapImage top = NULL;
   if (!top)
      top = roadmap_res_get ( RES_BITMAP, RES_SKIN, "response_bubble1_top" );

   if (top)
      return roadmap_canvas_image_width(top);

   return 0;
}


static int comment_callback (SsdWidget widget, const char *new_value){
      int   menu_x;

      RTAlertComment *comment = (RTAlertComment *)widget->context;
      if (!comment)
         return 0;

      g_Comment_id = comment->iID;

      ssd_dialog_set_focus(widget->parent);

      if(g_context_menu_is_active)
      {
         ssd_dialog_hide_current(dec_cancel);
         g_context_menu_is_active = FALSE;
      }

      if  (ssd_widget_rtl (NULL))
         menu_x = SSD_X_SCREEN_RIGHT;
      else
         menu_x = SSD_X_SCREEN_LEFT;

      ssd_contextmenu_show_item( &context_menu,
                                 rtcl_cm_add_comments,
                                 FALSE,
                                 FALSE);

      ssd_contextmenu_show_item( &context_menu,
                                 rtcl_cm_show,
                                 FALSE,
                                 FALSE);

      ssd_context_menu_show(  menu_x,   // X
                              SSD_Y_SCREEN_BOTTOM, // Y
                              &context_menu,
                              on_option_selected,
                              NULL,
                              dir_default,
                              0,
                              TRUE);

      g_context_menu_is_active = TRUE;

   return 1;
}
///////////////////////////////////////////////////////////////////////////////////////////
int RealtimeAlertCommentsList(int iAlertId)
{
   int SavedZoom = -1;
   RoadMapPosition SavedPosition;
   SsdWidget dialog;
   SsdWidget button;
   int timeDiff;
   time_t now;
   SsdWidget group, container, image_con, text_con;
   SsdWidget bitmap;
   SsdWidget facebook_image;
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
   const char *moodStr;
   char *icon[3];
   int i, num_addOns;
   int height = 90;
   int width  = 52;
   int mood_size = 40;
   int title_height = 25;
   int container_width = roadmap_canvas_width();
   SsdWidget mood_con, mood;

   static RoadMapRealTimeAlertListCommentsDialog context =
   { "Real Time Alert Comments", -1 };

   RTAlertCommentsEntry *CommentEntry;

   dialog = ssd_dialog_new ("alert_comments",
                             roadmap_lang_get("Real Time Alerts Comments"),
                             NULL,
                             SSD_CONTAINER_TITLE);
#ifndef TOUCH_SCREEN
   ssd_widget_set_left_softkey_text(dialog, roadmap_lang_get("Options"));
   ssd_widget_set_left_softkey_callback(dialog, on_options_comments);
#endif

   if ( roadmap_screen_is_hd_screen() ){
	   height = 150;
	   width = 78;
	   mood_size = 60;
	   title_height = 40;
   }
   container_width -= (width+10);

   group = ssd_container_new ("alert_comments.container", NULL,
                               SSD_MAX_SIZE,SSD_MIN_SIZE,SSD_ROUNDED_CORNERS|SSD_ROUNDED_WHITE|SSD_WIDGET_SPACE|SSD_CONTAINER_BORDER|SSD_WS_TABSTOP);



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
               "\n",roadmap_lang_get("Updated "));
   else
     snprintf(AlertStr + strlen(AlertStr), sizeof(AlertStr)
                - strlen(AlertStr),"%s%s",
                "\n", roadmap_lang_get("Reported "));


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


   image_con = ssd_container_new ("icon_container", NULL, width,
          SSD_MIN_SIZE,  SSD_ALIGN_RIGHT);
   ssd_widget_set_color(image_con, NULL, NULL);
   bitmap = ssd_bitmap_new("AlertIcon", RTAlerts_Get_Icon(alert->iID), SSD_ALIGN_CENTER);
   num_addOns = RTAlerts_Get_Number_Of_AddOns(alert->iID);
   for (i = 0 ; i < num_addOns; i++){
          const char *AddOn = RTAlerts_Get_AddOn(alert->iID, i);
          if (AddOn)
             ssd_bitmap_add_overlay(bitmap,AddOn);
   }

   ssd_widget_add(image_con, bitmap);
   text = ssd_text_new ("Alert Disatnce", dist_str, 16,SSD_ALIGN_CENTER|SSD_END_ROW);
   ssd_widget_add(image_con, text);

   text = ssd_text_new ("Alert Disatnce", unit_str, 14,SSD_ALIGN_CENTER);
   ssd_widget_set_color(text, "#383838", NULL);
   ssd_widget_add(image_con, text);

   if (alert->iMood != 0){
     SsdWidget mood_con, mood;

     mood_con = ssd_container_new("mood_Cont", NULL, mood_size,mood_size,SSD_ALIGN_RIGHT);
     ssd_widget_set_color(mood_con, NULL, NULL);
     moodStr = roadmap_mood_to_string(alert->iMood);
     mood = ssd_bitmap_new("mood",(void *)moodStr,SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER);
     free((void *)moodStr);
     ssd_widget_add(mood_con, mood);
     ssd_widget_add(image_con, mood_con);
    }
   ssd_widget_add(group, image_con);

   text_con = ssd_container_new ("icon_container", NULL, container_width,
          SSD_MIN_SIZE, 0);
   ssd_widget_set_color(text_con, NULL, NULL);

   text = ssd_text_new("Distance_str", DistanceStr, 16,SSD_END_ROW);
   ssd_widget_set_color(text,"#9d1508", NULL);
   ssd_widget_add(text_con, text);
   snprintf(str, sizeof(str), "%d", alert->iID);
   text = ssd_text_new("AlertTXt",AlertStr,-1,SSD_START_NEW_ROW|SSD_END_ROW|SSD_TEXT_NORMAL_FONT);
   ssd_widget_add(text_con, text);
   text = ssd_text_new("ReportedByTxt",ReportedByStr,-1,SSD_TEXT_NORMAL_FONT);
   ssd_widget_add(text_con, text);
   RTAlerts_update_stars(text_con, alert);
   ssd_widget_add (text_con,
      ssd_container_new ("space_before_descrip", NULL, 0, 10, SSD_END_ROW));
   RTAlerts_show_space_before_desc(text_con,alert);
   text = ssd_text_new("ReportedByTxt",DescriptionStr,-1,0);
   ssd_widget_add(text_con, text);
   ssd_widget_add(group, text_con);

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
       int title_height = 25;
       SsdSize size, max;

       if ( roadmap_screen_is_hd_screen() )
          title_height = 40;

       CommentStr[0] = 0;

      if (CommentEntry->comment.bCommentByMe)
          bitmap = ssd_bitmap_new("CommentSent", "response_bubble2",SSD_END_ROW);
      else{
          bitmap = ssd_container_new("CommentRec",NULL, get_width(), SSD_MIN_SIZE, 0);
          ssd_widget_set_color(bitmap, NULL, NULL);
          ssd_widget_set_context(bitmap, (void *)&CommentEntry->comment);
          bitmap->draw = draw_comment_image;
          bitmap->callback = comment_callback;
      }

	   if (CommentEntry->comment.bCommentByMe){
	      if ( roadmap_screen_is_hd_screen() )
	         text_container = ssd_container_new("text_con", NULL, 280, 100, 0 );
	      else
	         text_container = ssd_container_new("text_con", NULL, 200, 80, 0 );

	   }else{
	      if ( roadmap_screen_is_hd_screen() )
	         text_container = ssd_container_new("text_con", NULL, 320, SSD_MIN_SIZE, 0 );
	      else
	         text_container = ssd_container_new("text_con", NULL, 205, SSD_MIN_SIZE, 0 );
	   }
       ssd_widget_set_color(text_container, NULL, NULL);
       if (ssd_widget_rtl(NULL)){
          if (CommentEntry->comment.bCommentByMe)
          {
            if ( roadmap_screen_is_hd_screen() )
               ssd_widget_set_offset(text_container, 35, 5);
            else
             ssd_widget_set_offset(text_container, 25, 2);
          }
          else
             ssd_widget_set_offset(text_container, 10, 2);
       }
       else{

          if (CommentEntry->comment.bCommentByMe)
             if ( roadmap_screen_is_hd_screen() )
                ssd_widget_set_offset(text_container, 10, 5 );
             else
                ssd_widget_set_offset(text_container, 10, 2);
          else
          {
            bitmap->flags |= SSD_ALIGN_RIGHT;
            if ( roadmap_screen_is_hd_screen() )
               ssd_widget_set_offset(text_container, 35, 2);
            else
               ssd_widget_set_offset(text_container, 25, 2);
           }
       }
       title_container = ssd_container_new("text_con", NULL, SSD_MIN_SIZE, title_height, SSD_END_ROW );
       ssd_widget_set_color(title_container, NULL, NULL);
       snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
               - strlen(CommentStr), " %s %s  ", roadmap_lang_get("by"),
               CommentEntry->comment.sPostedBy);
       text = ssd_text_new("CommnetTxt", CommentStr, -1,SSD_ALIGN_VCENTER );
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


       text = ssd_text_new("CommnetTxt", CommentStr, 10, SSD_END_ROW|SSD_TEXT_NORMAL_FONT );
       ssd_widget_set_color(text,"#585858", NULL);
       ssd_widget_add(text_container, title_container);

       ssd_widget_add(text_container, text);
       CommentStr[0] = 0;
       if ( roadmap_screen_is_hd_screen() )
         container = ssd_container_new("AddCommentCont", NULL, SSD_MAX_SIZE, 60, 0 );
      else
         container = ssd_container_new("AddCommentCont", NULL, SSD_MAX_SIZE, SSD_MIN_SIZE, 0 );

       ssd_widget_set_color(container, NULL, NULL);
       snprintf(CommentStr + strlen(CommentStr), sizeof(CommentStr)
               - strlen(CommentStr), "  %s",
               CommentEntry->comment.sDescription);
       text = ssd_text_new("CommnetTxt", CommentStr, -1,SSD_TEXT_NORMAL_FONT|SSD_ALIGN_VCENTER);
       ssd_widget_add(container, text);
       ssd_widget_add(text_container, container);


       ssd_widget_add(bitmap, text_container);

#ifdef TOUCH_SCREEN
//       if (!CommentEntry->comment.bCommentByMe){
//          icon[0] = "button_report_abuse";
//          icon[1] = "button_report_abuse_down";
//          icon[2] = NULL;
//          button = ssd_button_new( "Reprt_Abuse_Button", "ReportAbuse", (const char**) &icon[0], 2, SSD_ALIGN_CENTER, on_report_abuse );
//          ssd_widget_set_context(button, (void *)&CommentEntry->comment);
//          ssd_widget_add(bitmap, button);
//       }
#endif

       if (CommentEntry->comment.bCommentByMe){
          if (ssd_widget_rtl(NULL))
             container = ssd_container_new("CommentsCont", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WS_TABSTOP|SSD_END_ROW);
          else
             container = ssd_container_new("CommentsCont", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WS_TABSTOP|SSD_ALIGN_RIGHT);
       }else{
          int f_width = 52;
          int f_height = 52;
          SsdWidget fb_image_cont;

          if (ssd_widget_rtl(NULL))
             container = ssd_container_new("CommentsCont", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WS_TABSTOP|SSD_ALIGN_RIGHT);
          else
             container = ssd_container_new("CommentsCont", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE, SSD_WS_TABSTOP|SSD_END_ROW);


          if (roadmap_screen_is_hd_screen()){
              f_height = 77;
              f_width = 77;
          }

#ifndef TOUCH_SCREEN
          if (alert->iMood != 0){
               SsdWidget mood_con, mood;

               mood_con = ssd_container_new("mood_Cont", NULL, mood_size,mood_size,SSD_ALIGN_RIGHT|SSD_ALIGN_BOTTOM);
               ssd_widget_set_color(mood_con, NULL, NULL);
               moodStr = roadmap_mood_to_string(CommentEntry->comment.iMood);
               mood = ssd_bitmap_new("mood",(void *)moodStr,SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER);
               free((void *)moodStr);
               ssd_widget_add(mood_con, mood);
               if (ssd_widget_rtl(NULL))
                  ssd_widget_set_offset(mood_con, 10, 0);
               else
                  ssd_widget_set_offset(mood_con, -10, 0);
               ssd_widget_add(bitmap, mood_con);
          }
#else
          fb_image_cont = ssd_container_new ("FB_IMAGE_container", "", f_width, f_height, SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT);

          if (CommentEntry->comment.bShowFacebookPicture){
             ssd_widget_set_color(fb_image_cont, "#ffffff", "#ffffff");
             facebook_image = ssd_bitmap_new("facebook_image", "facebook_default_image", SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER);
             ssd_widget_add(fb_image_cont, facebook_image);
             if (CommentEntry->comment.iMood != 0){
                moodStr = roadmap_mood_to_string(CommentEntry->comment.iMood);
                mood = ssd_bitmap_new("mood",(void *)moodStr,SSD_ALIGN_BOTTOM|SSD_ALIGN_RIGHT);
                if (ssd_widget_rtl(NULL))
                   ssd_widget_set_offset(mood, 24, 16);
                else
                   ssd_widget_set_offset(mood, 12, 16);
                free((void *)moodStr);
                ssd_widget_add(fb_image_cont, mood);
             }
             roadmap_social_image_download_update_bitmap( SOCIAL_IMAGE_SOURCE_FACEBOOK, SOCIAL_IMAGE_ID_TYPE_ALERT, alert->iID, CommentEntry->comment.iID,  SOCIAL_IMAGE_SIZE_SQUARE, facebook_image );

          }
          else{
             ssd_widget_set_color(fb_image_cont, NULL, NULL);
             if (CommentEntry->comment.iMood != 0){
                moodStr = roadmap_mood_to_string(CommentEntry->comment.iMood);
                mood = ssd_bitmap_new("mood",(void *)moodStr,SSD_ALIGN_CENTER|SSD_ALIGN_BOTTOM);
                free((void *)moodStr);
                ssd_widget_add(fb_image_cont, mood);
             }
          }
          ssd_widget_add(container, fb_image_cont);
#endif
       }

       ssd_widget_set_context(container, (void *)&CommentEntry->comment);
       ssd_widget_set_color(container, NULL, NULL);
       ssd_widget_add(container, bitmap);
       ssd_widget_add(dialog, container);
       ssd_dialog_add_vspace(dialog, 15, 0);
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

