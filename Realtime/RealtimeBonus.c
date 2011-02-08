/* RealtimeBonus.c
 *
 * LICENSE:
 *
 *   Copyright 2009 Avi Ben-Shoshan
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
 *
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../roadmap.h"
#include "../roadmap_main.h"
#include "../roadmap_string.h"
#include "../roadmap_object.h"
#include "../roadmap_lang.h"
#include "../roadmap_sound.h"
#include "../roadmap_start.h"
#include "../roadmap_screen.h"
#include "../roadmap_line.h"
#include "../roadmap_line_route.h"
#include "../roadmap_alerter.h"
#include "../roadmap_ticker.h"
#include "../roadmap_res.h"
#include "../roadmap_navigate.h"
#include "../roadmap_messagebox.h"
#include "../roadmap_social.h"
#include "../roadmap_browser.h"
#include "../editor/editor_points.h"
#include "../editor/editor_screen.h"
#include "../roadmap_res_download.h"
#include "../roadmap_map_settings.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_popup.h"
#include "ssd/ssd_button.h"
#include "Realtime.h"
#include "RealtimeBonus.h"
#include "../roadmap_math.h"
#include "../roadmap_message_ticker.h"
#include "../roadmap_layer.h"

static RTBonusTable gBonusTable;
static RTBonusTemplatesTable gBonusTemplateTable;

RTBonus *g_pbonus;

static void AddBonusToMap(RTBonus *pApdon, BOOL isCustom);
static  RoadMapGuiRect old_rect = {-1, -1, -1, -1};

#define ANIMATION_SOUND "bonus"
#define MINIMUM_DISTANCE_TO_CHECK 5
roadmap_alert_provider RoadmapRealTimeMapbonusnsProvider = { "RealTimeBonus", RealtimeBonus_Count,
      RealtimeBonus_Get_Id, RealtimeBonus_Get_Position, RealtimeBonus_Get_Speed, NULL, NULL, NULL,
      RealtimeBonus_Get_Distance, NULL, RealtimeBonus_Is_Alertable, NULL, NULL, NULL,
      RealtimeBonus_Check_Same_Street, RealtimeBonus_HandleEvent, RealtimeBonus_is_square_dependent,
      RealtimeBonus_get_location_info, RealtimeBonus_distance_check,RealtimeBonus_get_priority, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};


static RoadMapConfigDescriptor RoadMapConfigCustomBonusRadius =
      ROADMAP_CONFIG_ITEM("Custom bonus", "Radius");

static RoadMapConfigDescriptor RoadMapConfigCustomBonusLastID =
      ROADMAP_CONFIG_ITEM("Custom bonus", "Last Bonus ID");

static RoadMapConfigDescriptor RoadMapConfigCustomBonusFeatureEnabled =
      ROADMAP_CONFIG_ITEM("Custom bonus", "Feature enabled");

static RoadMapConfigDescriptor RoadMapConfigBonusURL =
      ROADMAP_CONFIG_ITEM("Bonus", "URL");

static int g_CustomIndex = -1;


//////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL custom_bonus_feature_enabled (void) {
   if (0 == strcmp (roadmap_config_get (&RoadMapConfigCustomBonusFeatureEnabled), "yes")){
      return TRUE;
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int get_custom_bonus_radius(void){
   return roadmap_config_get_integer(&RoadMapConfigCustomBonusRadius);
}

static int get_custom_bonus_last_shown_id(void){
   return roadmap_config_get_integer(&RoadMapConfigCustomBonusLastID);
}

static void set_custom_bonus_last_shown_id(int id){
   roadmap_config_set_integer(&RoadMapConfigCustomBonusLastID, id);
   roadmap_config_save(FALSE);
}

void RealTimeBonus_SegmentChangedCB (PluginLine *current, int direction){

   int line_length;
   RoadMapPosition      pos;

   line_length  = roadmap_line_length(current->line_id);
   if (line_length < 150)
      return;

   roadmap_square_set_current(current->square);

   if (direction == ROUTE_DIRECTION_WITH_LINE){
      roadmap_street_extend_line_ends(current, NULL, &pos, FLAG_EXTEND_TO, NULL, NULL);
   }
   else{
      roadmap_street_extend_line_ends(current, &pos, NULL, FLAG_EXTEND_FROM, NULL, NULL);
   }

   if (g_CustomIndex == -1)
      return;

   if (gBonusTable.bonus[g_CustomIndex] == NULL)
      return;

   gBonusTable.bonus[g_CustomIndex]->position.latitude = pos.latitude;
   gBonusTable.bonus[g_CustomIndex]->position.longitude = pos.longitude;
   //Adding the custom bonus
   if (roadmap_map_settings_road_goodies()){
      static RoadMapSoundList list;
      if (!list) {
         list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
         roadmap_sound_list_add (list, "ping2");
         roadmap_res_get (RES_SOUND, 0, "ping2");
      }
      roadmap_sound_play_list (list);

      AddBonusToMap(gBonusTable.bonus[g_CustomIndex], TRUE);
   }

   g_CustomIndex = -1;
   roadmap_navigate_unregister_segment_changed(RealTimeBonus_SegmentChangedCB);

}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void Play_Animation_Sound (void) {
   static RoadMapSoundList list;
   if (!list) {
      list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);
      roadmap_sound_list_add (list, ANIMATION_SOUND);
      roadmap_res_get (RES_SOUND, 0, ANIMATION_SOUND);
   }
   roadmap_sound_play_list (list);
}


//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeBonus_Animate_Pacman (void) {
   static char pacman_name[50];
   static int index = 1;
   static int repeat = 0;

#ifndef J2ME
   if ( (index == 5) && (repeat == 8)) {
      roadmap_main_remove_periodic (RealtimeBonus_Animate_Pacman);
      editor_screen_set_override_car (NULL);
      index = 1;
      repeat = 0;
   }
   else if (index == 5) {
      index = 2;
      repeat++;
      sprintf (pacman_name, "pacman%d", index);
      editor_screen_set_override_car (pacman_name);
   }
   else {
      index++;
      sprintf (pacman_name, "pacman%d", index);
      editor_screen_set_override_car (pacman_name);
   }

   if (index == 2)
      roadmap_main_set_periodic (300, RealtimeBonus_Animate_Pacman);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeBonus_Record_Init (RTBonus *pbonus) {
   pbonus->iID = -1;
   pbonus->iType = 0;
   pbonus->iToken = -1;
   pbonus->position.latitude = -1;
   pbonus->position.longitude = -1;
   pbonus->iNumPoints = 0;
   pbonus->iRadius = 0;
   pbonus->pIconName = NULL;
   pbonus->sGUIID[0] = 0;
   pbonus->collected = FALSE;
   pbonus->displayed = FALSE;
   pbonus->location_info.line_id = -1;
   pbonus->location_info.square_id = ALERTER_SQUARE_ID_UNINITIALIZED;
   pbonus->location_info.time_stamp = -1;
   pbonus->pBonusText = NULL;
   pbonus->pBonusTextColor = NULL;
   pbonus->pBonusTitle = NULL;
   pbonus->pBonusTitleColor = NULL;
   pbonus->pCollectText = NULL;
   pbonus->pCollectTitle = NULL;
   pbonus->pCollectIcon = NULL;
   pbonus->bHasWebDlg = FALSE;
   pbonus->bIsCustomeBonus = FALSE;
   pbonus->pWebDlgExtraParams = NULL;
   pbonus->iTemplateID = -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeBonus_Count (void) {
   return gBonusTable.iCount;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RealtimeBonus_IsEmpty (void) {

   return (0 == gBonusTable.iCount);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
RTBonus *RealtimeBonus_Get_Record (int index) {
   if (index >= MAX_ADD_ONS)
      return NULL;

   return gBonusTable.bonus[index];

}

//////////////////////////////////////////////////////////////////////////////////////////////////
RTBonus *RealtimeBonus_Get (int iID) {
   int i;
   for (i = 0; i < MAX_ADD_ONS; i++) {
      if (gBonusTable.bonus[i] && gBonusTable.bonus[i]->iID == iID)
         return RealtimeBonus_Get_Record (i);
   }

   return NULL;

}

//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeBonus_RemoveFromTable (int iID) {
   int i;
   for (i = 0; i < MAX_ADD_ONS; i++) {
      if (gBonusTable.bonus[i] && gBonusTable.bonus[i]->iID == iID) {
         if (gBonusTable.bonus[i]->bIsCustomeBonus && !gBonusTable.bonus[i]->displayed){
            roadmap_navigate_unregister_segment_changed(RealTimeBonus_SegmentChangedCB);
            g_CustomIndex = -1;
         }
         free (gBonusTable.bonus[i]);
         gBonusTable.bonus[i] = NULL;
         gBonusTable.iCount--;
         return;
      }
   }
   roadmap_log( ROADMAP_DEBUG, "RealtimeBonus_RemoveFromTable - Id not found (id =%d)", iID);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RealtimeBonus_Exists (int iID) {
   if (NULL == RealtimeBonus_Get (iID))
      return FALSE;

   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
const char *GetBaseUrl(void){
   return roadmap_config_get(&RoadMapConfigBonusURL);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static char *build_url(RTBonus *pbonus, int iHeight, int iWidth){
   static char url[1024];

   snprintf(url, sizeof(url),"%s?bonus_id=%d&sessionid=%d&cookie=%s&deviceid=%d&width=%d&height=%d&client_version=%s&lang=%s&template=%d&%s",
            GetBaseUrl(),
            pbonus->iID,
            Realtime_GetServerId(),
            Realtime_GetServerCookie(),
            RT_DEVICE_ID,
            iWidth,
            iHeight,
            roadmap_start_version(),
            roadmap_lang_get_system_lang(),
            pbonus->iTemplateID,
            (pbonus->pWebDlgExtraParams == NULL) ? "" :  pbonus->pWebDlgExtraParams);
   return &url[0];
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static int get_browser_height(){
   if (roadmap_screen_is_hd_screen())
      return 235;
   else{
#ifndef TOUCH_SCREEN
      if (!is_screen_wide())
         return 240;
#endif
      return 155;
   }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static void on_dialog_close  (int exit_code, void* context){
   old_rect.minx = old_rect.maxx = old_rect.miny = old_rect.maxy = -1;
   roadmap_browser_hide();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static int on_button_close (SsdWidget widget, const char *new_value){
   ssd_dialog_hide("BonusBrowserDlg", dec_close);
   return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static void draw_browser_rect(SsdWidget widget, RoadMapGuiRect *rect, int flags){
   RTBonus *pbonus = (RTBonus *)widget->context;
   RMBrowserContext context;
   int width, height;
   if ((flags & SSD_GET_SIZE)){
      return;
   }

   if (pbonus == NULL)
      return;

   if ( (old_rect.minx != rect->minx) || (old_rect.maxx != rect->maxx) || (old_rect.miny != rect->miny) || (old_rect.maxy != rect->maxy)){
      if ( (old_rect.minx != -1) && (old_rect.maxx != -1) && (old_rect.miny != -1) && (old_rect.maxy != -1))
         roadmap_browser_hide();
      old_rect = *rect;
      context.flags = BROWSER_FLAG_WINDOW_TYPE_TRANSPARENT|BROWSER_FLAG_WINDOW_TYPE_NO_SCROLL;
      context.rect = *rect;
      height = rect->maxy - rect->miny +1;
      width = rect->maxx - rect->minx +1;
      strncpy_safe( context.url, build_url( pbonus, height, width ), WEB_VIEW_URL_MAXSIZE );
      context.attrs.on_close_cb = NULL;
      context.attrs.title_attrs.title = NULL;
      roadmap_browser_show_embeded(&context);
   }

}
//////////////////////////////////////////////////////////////////////////////////////////////////
static int on_next (SsdWidget widget, const char *new_value) {
    static SsdWidget dialog;
    SsdWidget button;
    SsdWidget browserCont;
    SsdSize dlg_size, cnt_size;
    int browser_cont_flags = 0;

	ssd_dialog_hide_current(dec_close);

    if ( dialog != NULL )
    {
      if (ssd_dialog_currently_active_name() && !strcmp(ssd_dialog_currently_active_name(), "BonusBrowserDlg"))
            ssd_dialog_hide_current(dec_close);

         ssd_dialog_free( "BonusBrowserDlg", FALSE );
         dialog = NULL;
    }


    dialog = ssd_dialog_new ( "BonusBrowserDlg", "", on_dialog_close,
            SSD_CONTAINER_BORDER|SSD_DIALOG_FLOAT|SSD_DIALOG_MODAL|
            SSD_ALIGN_CENTER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_BLACK);

#ifndef TOUCH_SCREEN
    browser_cont_flags = SSD_ALIGN_VCENTER;
    ssd_dialog_add_vspace(dialog, 2 ,0);
#else
    ssd_dialog_add_vspace(dialog, 5 ,0);
#endif

    browserCont = ssd_container_new("BonusBrowserDlg.BrowserContainer","", SSD_MAX_SIZE, get_browser_height() , SSD_ALIGN_CENTER|browser_cont_flags);
    browserCont->context = (void *)widget->context;
    browserCont->draw = draw_browser_rect;
    ssd_widget_set_color(browserCont, NULL, NULL);
    ssd_widget_add(dialog, browserCont);

#ifdef TOUCH_SCREEN
    ssd_dialog_add_vspace(dialog, 5 ,0);

    button = ssd_button_label("Close_button", roadmap_lang_get("Close"), SSD_ALIGN_CENTER, on_button_close);
    ssd_widget_add(dialog, button);
#else
    ssd_widget_set_left_softkey_callback(dialog, NULL);
    ssd_widget_set_left_softkey_text(dialog, "");
#endif
    ssd_dialog_activate ("BonusBrowserDlg", NULL);

    ssd_dialog_recalculate( "BonusBrowserDlg" );
    ssd_widget_get_size( dialog, &dlg_size, NULL );
    ssd_widget_get_size( browserCont, &cnt_size, NULL );

   return 1;
}

//////////////////////////////////////////////////////////////////
static int extract_id (const char *id) {
   char * TempStr = strdup (id);
   char * tempC;
   tempC = strtok ((char *) TempStr, "_");
   if (tempC != NULL)
      return (atoi (tempC));
   else
      return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeBonus_FloatingPopUp (RTBonus *pbonus, SsdCallback on_next_cb) {
   char msg[512];
   const char *title;

   if (!pbonus)
      return;

   if (pbonus->pBonusTitle){
      title = roadmap_lang_get (pbonus->pBonusTitle);
   }
   else{
      if (pbonus->iType == BONUS_TYPE_POINTS)
         title = roadmap_lang_get ("Bonus points");
      else if (pbonus->iType == BONUS_TYPE_TREASURE)
         title = roadmap_lang_get ("Treasure chest");
   }

   if (pbonus->iType == BONUS_TYPE_POINTS){
      if (pbonus->pBonusText){
         snprintf (msg, sizeof(msg), "%s", roadmap_lang_get(pbonus->pBonusText));
      } else if (pbonus->iType == BONUS_TYPE_POINTS){
         snprintf (msg, sizeof(msg), "%d %s",pbonus->iNumPoints, roadmap_lang_get ("points!"));
      } else{
         msg[0] = 0;
      }
   }
   else{
      if (pbonus->pBonusText){
         snprintf (msg, sizeof(msg), "%s", roadmap_lang_get(pbonus->pBonusText));
      }else if (pbonus->iType == BONUS_TYPE_TREASURE){
         if (!Realtime_is_random_user())
            snprintf (msg, sizeof(msg), "%s", roadmap_lang_get ("There may be presents in this treasure chest! You'll know when you drive over it."));
         else
            snprintf (msg, sizeof(msg), "%s", roadmap_lang_get ("There may be presents in this treasure chest! You'll know when you drive over it. Note: You need to be a registered user in order to receive the gift inside. Register in 'Settings > Profile'"));
      }
   }
   ssd_popup_show_float("BonusPopUPDlg",
                             title,
                             pbonus->pBonusTitleColor,
                             msg,
                             pbonus->pBonusTextColor,
                             pbonus->pIconName,
                             &pbonus->position,
                             ADJ_SCALE(-20),
                             NULL,
                             on_next_cb,
                             "More",
                             (void *)pbonus);

}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int on_close (SsdWidget widget, const char *new_value) {
   if (ssd_dialog_is_currently_active () && (!strcmp (ssd_dialog_currently_active_name (),
                  "BonusPopUPDlg")))
      ssd_dialog_hide_current (dec_close);

   return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeBonus_PopUpNext (SsdWidget widget, const char *new_value) {
   SsdWidget popup, spacer, container, button, text, icon;
   char msg[512];
   RTBonus *pbonus = g_pbonus;
   const char *title;
   char *text_color = "#ffffff";
   int width;

   if (!pbonus)
      return 0;

   if (ssd_dialog_is_currently_active () && (!strcmp (ssd_dialog_currently_active_name (),
                  "BonusPopUPDlg")))
      ssd_dialog_hide_current (dec_cancel);

   if (pbonus->pBonusTitle){
      title = roadmap_lang_get (pbonus->pBonusTitle);
   }
   else{
      if (pbonus->iType == BONUS_TYPE_TREASURE)
         title = roadmap_lang_get ("Treasure chest");
   }

   popup = ssd_popup_new ("BonusPopUPDlg", title, NULL,
                  SSD_MAX_SIZE, SSD_MIN_SIZE, &pbonus->position, SSD_POINTER_LOCATION
                                 | SSD_ROUNDED_BLACK,DIALOG_ANIMATION_FROM_TOP);

   spacer = ssd_container_new ("space", "", SSD_MIN_SIZE, 5, SSD_END_ROW);
   ssd_widget_set_color (spacer, NULL, NULL);
   ssd_widget_add (popup, spacer);

   container = ssd_container_new ("IconCon", "", 50, SSD_MIN_SIZE, SSD_ALIGN_RIGHT);
   ssd_widget_set_color (container, NULL, NULL);
   icon = ssd_bitmap_new ("bonusIcon", pbonus->pIconName, SSD_ALIGN_VCENTER | SSD_ALIGN_CENTER);
   ssd_widget_add (container, icon);
   ssd_widget_add (popup, container);

   if (pbonus->pBonusText){
      snprintf (msg, sizeof(msg), "%s", pbonus->pBonusText);
   }else if (pbonus->iType == BONUS_TYPE_TREASURE){
      if (!Realtime_is_random_user())
         snprintf (msg, sizeof(msg), "%s", roadmap_lang_get ("There may be presents in this treasure chest! You'll know when you drive over it."));
      else
         snprintf (msg, sizeof(msg), "%s", roadmap_lang_get ("There may be presents in this treasure chest! You'll know when you drive over it. Note: You need to be a registered user in order to receive the gift inside. Register in 'Settings > Profile'"));
   }

   if (roadmap_canvas_width() > roadmap_canvas_height())
      width = roadmap_canvas_height() - 90;
   else
      width = roadmap_canvas_width() - 90;
   container = ssd_container_new ("IconCon", "", width, SSD_MIN_SIZE, 0);
   ssd_widget_set_color (container, NULL, NULL);
   text = ssd_text_new ("PointsTxt", msg, 16, SSD_END_ROW);
   if (pbonus->pBonusTextColor)
      text_color = pbonus->pBonusTextColor;

   ssd_text_set_color (text,text_color);
   ssd_widget_add (container, text);
   ssd_widget_add (popup, container);

#ifdef TOUCH_SCREEN
   spacer = ssd_container_new ("space", "", SSD_MIN_SIZE, 5, SSD_END_ROW);
   ssd_widget_set_color (spacer, NULL, NULL);
   ssd_widget_add (popup, spacer);

   button = ssd_button_label ("Close_button", roadmap_lang_get ("Close"), SSD_ALIGN_CENTER,
                  on_close);
   ssd_widget_add (popup, button);
#endif //TOUCH_SCREEN
   ssd_dialog_activate ("BonusPopUPDlg", NULL);

   if (!roadmap_screen_refresh ()) {
      roadmap_screen_redraw ();
   }
   return 1;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeBonus_PopUp (int iID) {
   RTBonus *pbonus = RealtimeBonus_Get (iID);


   if (!pbonus)
      return;

   if (pbonus->iType == BONUS_TYPE_POINTS){
      RealtimeBonus_FloatingPopUp(pbonus, pbonus->bHasWebDlg ? on_next : NULL);
      return;
   }
   else if (pbonus->iType == BONUS_TYPE_TREASURE){
      g_pbonus = pbonus;
      RealtimeBonus_FloatingPopUp(pbonus, pbonus->bHasWebDlg ? on_next : NULL);
      //RealtimeBonus_FloatingPopUp(pbonus, RealtimeBonus_PopUpNext);
      return;
   }

}
//////////////////////////////////////////////////////////////////////////////////////////////////
static void OnbonusShortClick (const char *name,
               const char *sprite,
               const char *image,
               const RoadMapGpsPosition *gps_position,
               const RoadMapGuiPoint    *offset,
               BOOL is_visible,
               int scale,
               int opacity,
               int scale_y,
               const char *id,
               const char *text) {
   RealtimeBonus_PopUp (extract_id (id));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void AddBonusToMap(RTBonus *pApdon, BOOL isCustom){
   RoadMapGpsPosition Pos;

   RoadMapDynamicString Group = roadmap_string_new ("BonusPoints");
   RoadMapDynamicString GUI_ID = roadmap_string_new (pApdon->sGUIID);
   RoadMapDynamicString Name = roadmap_string_new (pApdon->sGUIID);
   RoadMapDynamicString Sprite = roadmap_string_new ("Bonus");
   RoadMapDynamicString Image = roadmap_string_new (pApdon->pIconName);
   RoadMapDynamicString Text = NULL;

   if (isCustom){
      char temp[10];
      if (pApdon->iNumPoints != 0){
         sprintf(temp, "%d", pApdon->iNumPoints);
         Text = roadmap_string_new (temp);
      }
   }

   Pos.longitude = pApdon->position.longitude;
   Pos.latitude = pApdon->position.latitude;
   Pos.altitude = 0;
   Pos.speed = 0;
   Pos.steering = 0;
   roadmap_object_add (Group, GUI_ID, Name, Sprite, Image, &Pos, NULL, OBJECT_ANIMATION_POP_IN | OBJECT_ANIMATION_WHEN_VISIBLE, Text);
   roadmap_object_set_zoom (GUI_ID, -1, roadmap_layer_get_declutter(ROADMAP_ROAD_MAIN));

   pApdon->displayed = TRUE;
   roadmap_object_set_action (GUI_ID, OnbonusShortClick);
   roadmap_string_release (Group);
   roadmap_string_release (GUI_ID);
   roadmap_string_release (Name);
   roadmap_string_release (Sprite);
   roadmap_string_release (Image);
}

static void ticker_closed_cb(void){
   set_custom_bonus_last_shown_id(gBonusTable.bonus[g_CustomIndex]->iID);
   roadmap_navigate_register_segment_changed(RealTimeBonus_SegmentChangedCB);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void display_ticker(void){
   if (g_CustomIndex  == -1)
      return;
   if (get_custom_bonus_last_shown_id() != gBonusTable.bonus[g_CustomIndex]->iID){
      roadmap_message_ticker_show_cb(gBonusTable.bonus[g_CustomIndex]->pCollectTitle, gBonusTable.bonus[g_CustomIndex]->pCollectText, gBonusTable.bonus[g_CustomIndex]->pCollectIcon, -1, ticker_closed_cb);
   }
   else{
      ticker_closed_cb();
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void SpeedCheck_Timer(void){
   RoadMapGpsPosition pos;
   BOOL gps_active;


   gps_active = roadmap_gps_have_reception();
   if (!gps_active)
         return;

   roadmap_navigate_get_current(&pos, NULL, NULL);
   if (pos.speed < 2){
      roadmap_main_remove_periodic(SpeedCheck_Timer);
      display_ticker();
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int onBonusAdd (RTBonus *pApdon) {

   if (pApdon->bIsCustomeBonus){
      roadmap_main_set_periodic(1000, SpeedCheck_Timer);
   }
   else{
      if (roadmap_map_settings_road_goodies())
         AddBonusToMap(pApdon, FALSE);
   }
   return TRUE;
}

static void on_resource_downloaded (const char* res_name, int status, void *context, char *last_modified){
   int i;
   for (i = 0; i < MAX_ADD_ONS; i++) {
       if (gBonusTable.bonus[i] && !strcmp(res_name, gBonusTable.bonus[i]->pIconName))
          onBonusAdd(gBonusTable.bonus[i]);
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////
static int onBonusDelete (RTBonus *pApdon) {
   RoadMapDynamicString GUI_ID = roadmap_string_new (pApdon->sGUIID);
   if ((!pApdon->bIsCustomeBonus || pApdon->displayed) && roadmap_map_settings_road_goodies())
   roadmap_object_remove (GUI_ID);
   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeBonus_CreateGUIID (RTBonus *pbonus) {
   snprintf (pbonus->sGUIID, MAX_GUID_ID, "%d_BonusPoints", pbonus->iID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RealtimeBonus_Add (RTBonus *pbonus) {
   int index;
   int i;


   if (!roadmap_map_settings_road_goodies() && !pbonus->bIsCustomeBonus)
      return TRUE;

   // Full?
   if (MAX_ADD_ONS == gBonusTable.iCount){
      roadmap_log( ROADMAP_ERROR, "RealtimeBonus_Add() - Failed (Table is full)");
      return FALSE;
   }

   if ((pbonus->bIsCustomeBonus) && (g_CustomIndex != -1))
      return TRUE;


   if ((pbonus->bIsCustomeBonus) && !custom_bonus_feature_enabled()){
      Realtime_CollectCustomBonus(pbonus->iID,
            FALSE,
            FALSE);
      editor_points_add_new_points (pbonus->iNumPoints);
      return TRUE;
   }

   if (RealtimeBonus_Exists (pbonus->iID))
      RealtimeBonus_Delete(pbonus->iID);


   for (i = 0; i < MAX_ADD_ONS; i++) {
      if (gBonusTable.bonus[i] == NULL) {
         index = i;
         break;
      }
   }

   gBonusTable.bonus[index] = calloc (1, sizeof(RTBonus));
   RealtimeBonus_Record_Init (gBonusTable.bonus[index]);
   gBonusTable.bonus[index]->iID = pbonus->iID;
   gBonusTable.bonus[index]->iTemplateID = pbonus->iTemplateID;

   // Get Template params
   if (pbonus->iTemplateID != -1){
         RTBonusTemplate *template = RealtimeBonus_BonusTemplate_Get(pbonus->iTemplateID);
         if (template){

            if ((template->iRadius != -1) && (pbonus->iRadius == -1))
               pbonus->iRadius = template->iRadius;

            if ((template->iRadius != -1) && (pbonus->iRadius == -1))
               pbonus->iNumPoints = template->iNumPoints;

            if ( (template->pIconName) && (pbonus->pIconName == NULL) )
                  pbonus->pIconName = template->pIconName;

            if ( (template->pText) && (pbonus->pBonusText == NULL) )
                  pbonus->pBonusText = template->pText;

            if ( (template->pTextColor) && (pbonus->pBonusTextColor == NULL) )
                  pbonus->pBonusTextColor = template->pTextColor;

            if ( (template->pTitle) && (pbonus->pBonusTitle == NULL) )
                  pbonus->pBonusTitle = template->pTitle;

            if ( (template->pTitleColor) && (pbonus->pBonusTitleColor == NULL) )
                  pbonus->pBonusTitleColor = template->pTitleColor;

            if ( (template->pWebDlgExtraParams) && (pbonus->pWebDlgExtraParams == NULL) )
               pbonus->pWebDlgExtraParams = template->pWebDlgExtraParams;
         }
         else{
            roadmap_log( ROADMAP_ERROR, "RealtimeBonus_Add() - Template record (%d) not found for bonus ID=%d",pbonus->iTemplateID, pbonus->iID);
         }
   }

   if (pbonus->iType == -1)
      pbonus->iType = BONUS_TYPE_POINTS;

   if ((pbonus->iRadius == -1) || (pbonus->iNumPoints == -1)){
      roadmap_log( ROADMAP_ERROR, "RealtimeBonus_Add() - radius or num points were not given (bonusId=%d, points=%d, radius=%d)",pbonus->iID, pbonus->iNumPoints, pbonus->iRadius);
      return TRUE;
   }

   gBonusTable.bonus[index]->iType = pbonus->iType;
   gBonusTable.bonus[index]->iNumPoints = pbonus->iNumPoints;
   gBonusTable.bonus[index]->iToken = pbonus->iToken;
   gBonusTable.bonus[index]->iRadius = pbonus->iRadius;
   gBonusTable.bonus[index]->position.latitude = pbonus->position.latitude;
   gBonusTable.bonus[index]->position.longitude = pbonus->position.longitude;
   gBonusTable.bonus[index]->pIconName = strdup (pbonus->pIconName);
   if (pbonus->pBonusText)
      gBonusTable.bonus[index]->pBonusText = strdup (pbonus->pBonusText);

   if (pbonus->pBonusTextColor)
      gBonusTable.bonus[index]->pBonusTextColor = strdup (pbonus->pBonusTextColor);

   if (pbonus->pBonusTitle)
      gBonusTable.bonus[index]->pBonusTitle = strdup (pbonus->pBonusTitle);

   if (pbonus->pBonusTitleColor)
      gBonusTable.bonus[index]->pBonusTitleColor = strdup (pbonus->pBonusTitleColor);

   if (pbonus->pCollectText)
      gBonusTable.bonus[index]->pCollectText = strdup (pbonus->pCollectText);

   if (pbonus->pCollectTitle)
      gBonusTable.bonus[index]->pCollectTitle = strdup (pbonus->pCollectTitle);

   if (pbonus->pCollectIcon)
      gBonusTable.bonus[index]->pCollectIcon = strdup (pbonus->pCollectIcon);

   if (pbonus->pWebDlgExtraParams)
      gBonusTable.bonus[index]->pWebDlgExtraParams = strdup (pbonus->pWebDlgExtraParams);

   gBonusTable.bonus[index]->bIsCustomeBonus = pbonus->bIsCustomeBonus;
   gBonusTable.bonus[index]->bHasWebDlg = pbonus->bHasWebDlg;

   gBonusTable.bonus[index]->collected = FALSE;
   RealtimeBonus_CreateGUIID (gBonusTable.bonus[index]);
   gBonusTable.iCount++;

   if (gBonusTable.bonus[index]->bIsCustomeBonus){
      if (gBonusTable.bonus[index]->iNumPoints != 0)
         gBonusTable.bonus[index]->iType = BONUS_TYPE_POINTS;
      gBonusTable.bonus[index]->iRadius = get_custom_bonus_radius();
      g_CustomIndex = index;
   }

   if (gBonusTable.bonus[index]->pCollectIcon && roadmap_res_get(RES_BITMAP,RES_SKIN, gBonusTable.bonus[index]->pCollectIcon) == NULL){
      roadmap_res_download(RES_DOWNLOAD_IMAGE, gBonusTable.bonus[index]->pCollectIcon,NULL, "",FALSE,0, NULL, NULL );
   }

   if (gBonusTable.bonus[index]->pIconName && roadmap_res_get(RES_BITMAP,RES_SKIN, gBonusTable.bonus[index]->pIconName) == NULL){
      roadmap_res_download(RES_DOWNLOAD_IMAGE, gBonusTable.bonus[index]->pIconName,NULL, "",FALSE,0, on_resource_downloaded, NULL );
   }
   else
      onBonusAdd (gBonusTable.bonus[index]);
   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RealtimeBonus_Delete (int iID) {
   RTBonus *pbonus = RealtimeBonus_Get (iID);
   if (!pbonus)
      return FALSE;

   if (pbonus->pIconName)
      free (pbonus->pIconName);

   if (pbonus->pBonusText)
      free (pbonus->pBonusText);

   if (pbonus->pBonusTextColor)
      free (pbonus->pBonusTextColor);

   if (pbonus->pBonusTitle)
      free (pbonus->pBonusTitle);

   if (pbonus->pBonusTitleColor)
      free (pbonus->pBonusTitleColor);

   if (pbonus->pCollectText)
      free (pbonus->pCollectText);

   if (pbonus->pCollectTitle)
      free (pbonus->pCollectTitle);

   if (pbonus->pCollectIcon)
      free (pbonus->pCollectIcon);

   if (pbonus->pWebDlgExtraParams)
      free(pbonus->pWebDlgExtraParams);

   onBonusDelete (pbonus);
   RealtimeBonus_RemoveFromTable (iID);
   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeBonus_Check_Same_Street (int record) {
   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeBonus_HandleEvent (int iID) {

   RTBonus *pbonus = RealtimeBonus_Get (iID);
   if (!pbonus)
      return TRUE;

   if (pbonus->collected)
      return TRUE;

   pbonus->collected = TRUE;

   if (roadmap_map_settings_road_goodies()){
   if ((pbonus->iType == BONUS_TYPE_TREASURE) && Realtime_is_random_user()){
      roadmap_messagebox_timeout("", roadmap_lang_get ("There may be presents in this treasure chest but you need to be a registered user in order to get them. Register in 'Settings > Profile'"), 10);
      return TRUE;
   }

   roadmap_log( ROADMAP_DEBUG, "Yeahhh Collecting a gift id=%d)", iID);
   Play_Animation_Sound ();
   RealtimeBonus_Animate_Pacman ();
   }

   if (pbonus->bIsCustomeBonus){
      Realtime_CollectCustomBonus(iID,
            roadmap_twitter_is_munching_enabled() && roadmap_twitter_logged_in(),
            roadmap_facebook_is_munching_enabled() && roadmap_facebook_logged_in());
      editor_points_add_new_points (pbonus->iNumPoints);

   }
   else{
   Realtime_CollectBonus(iID, pbonus->iToken,
         roadmap_twitter_is_munching_enabled() && roadmap_twitter_logged_in(),
         roadmap_facebook_is_munching_enabled() && roadmap_facebook_logged_in());
   }
   onBonusDelete(pbonus);
   return TRUE;
}

BOOL RealtimeBonus_is_square_dependent(void){
	return FALSE;
}

roadmap_alerter_location_info *  RealtimeBonus_get_location_info(int record){
   RTBonus *pbonus = RealtimeBonus_Get_Record (record);
   if (!pbonus)
      return NULL;
   return &(pbonus->location_info);
}

int RealtimeBonus_get_priority(void){
	return ALERTER_PRIORITY_HIGH;
}

BOOL RealtimeBonus_distance_check(RoadMapPosition gps_pos){
	static RoadMapPosition last_checked_position = {0,0};
	int distance;
	if(!last_checked_position.latitude){ // first time
		last_checked_position = gps_pos;
		return TRUE;
	}

	distance = roadmap_math_distance(&gps_pos, &last_checked_position);
	if (distance < MINIMUM_DISTANCE_TO_CHECK)
		return FALSE;
	else{
		last_checked_position = gps_pos;
		return TRUE;
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeBonus_CollectedPointsConfirmed(int iID, int iType, int iPoints, BOOL bHasGift, BOOL bIsBigPrize, const char *gift, const char *title, const char *msg){

   if (iType == BONUS_TYPE_POINTS){
      if (iPoints != 0){
         roadmap_log( ROADMAP_DEBUG, "RealtimeBonus_CollectedPointsRes() - (id =%d, points = %d)", iID, iPoints);
         editor_points_add_new_points (iPoints);
         editor_points_display_new_points_timed (iPoints, 6, bonus_points);
      }
      else{
         roadmap_log( ROADMAP_ERROR, "RealtimeBonus_CollectedPointsRes() - Failed (id =%d, points = %d)", iID, iPoints);
      }
   }
   else if (iType == BONUS_TYPE_TREASURE){
         if (msg){
            if (title)
            roadmap_messagebox_timeout(roadmap_lang_get(title), roadmap_lang_get(msg), 10);
         }
         else if (!bHasGift){ // The chest box was empty
            roadmap_messagebox_timeout("", roadmap_lang_get ("Bummer, this treasure chest was emptied. Keep searching for other chests, some have valuable prizes in them."), 10);
         }
         else{ // We got a gift!
            char msg[250];
            if (iPoints != 0){
               snprintf(msg, sizeof(msg),roadmap_lang_get ("Huray! You just won %s and an extra %d points. Check your email for info."), roadmap_lang_get(gift), iPoints);
               roadmap_messagebox("",msg);
               editor_points_add_new_points (iPoints);
               editor_points_display_new_points_timed (iPoints, 6, bonus_points);
            }
            else{
               if (bIsBigPrize){
                  snprintf(msg, sizeof(msg),roadmap_lang_get ("Huray!!! You won the big prize:  %s . Check your email for info."), roadmap_lang_get(gift));
                  roadmap_messagebox("",msg);
               }
               else{
                  snprintf(msg, sizeof(msg),roadmap_lang_get ("Huray! You just won %s. Check your email for info."), roadmap_lang_get(gift));
                  roadmap_messagebox("",msg);
               }
            }
         }
   }
   else{
      roadmap_log( ROADMAP_ERROR, "RealtimeBonus_CollectedPointsRes() - Unknown type = %d (ID=%d)", iType,iID);
   }
   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeBonus_OpenMessageTicker(int iPoints,  const char *text, const char *title, const char *icon){

   roadmap_log( ROADMAP_DEBUG, "RealtimeBonus_OpenMessageTicker() - (points = %d, title=%s, text=%s, icon=%s)", iPoints,title, text, icon);

   if (iPoints > 0){
         editor_points_add_new_points (iPoints);
   }

   roadmap_message_ticker_show(title, text, icon, -1);

   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
unsigned int RealtimeBonus_Get_Speed (int index) {
   return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeBonus_Get_Id (int index) {
   RTBonus *pbonus = RealtimeBonus_Get_Record (index);

   if (pbonus != NULL)
      return pbonus->iID;
   else
      return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeBonus_Get_Position (int index, RoadMapPosition *position, int *steering) {
   RTBonus *pbonus = RealtimeBonus_Get_Record (index);
   if (!pbonus)
      return;

   if (!position)
      return;

   position->longitude = pbonus->position.longitude;
   position->latitude = pbonus->position.latitude;

   if (!steering)
      return;

   *steering = 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeBonus_Get_Type (int iID) {
   RTBonus *pbonus = RealtimeBonus_Get (iID);
   if (!pbonus)
      return -1;
   return pbonus->iType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const char *RealtimeBonus_Get_Icon_Name (int iID) {
   RTBonus *pbonus = RealtimeBonus_Get (iID);
   if (!pbonus)
      return NULL;
   return pbonus->pIconName;

}

//////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeBonus_Get_Token (int iID) {
   RTBonus *pbonus = RealtimeBonus_Get (iID);
   if (!pbonus)
      return -1;

   return pbonus->iToken;

}

//////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeBonus_Get_Distance (int index) {
   RTBonus *pbonus = RealtimeBonus_Get_Record (index);

   if (pbonus != NULL)
      return pbonus->iRadius;
   else
      return -1;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeBonus_Get_NumberOfPoints (int iID) {
   RTBonus *pbonus = RealtimeBonus_Get (iID);
   if (!pbonus)
      return -1;

   return pbonus->iNumPoints;

}

//////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeBonus_Is_Alertable (int index) {
   RTBonus *pbonus = RealtimeBonus_Get_Record (index);
   if (pbonus == NULL)
      return FALSE;

   if (!pbonus->bIsCustomeBonus || pbonus->displayed)
   return TRUE;

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeBonus_Init (void) {
   int i;
   static BOOL registered_provider = FALSE;
   if(!registered_provider){
   		registered_provider = TRUE;
   		roadmap_alerter_register (&RoadmapRealTimeMapbonusnsProvider);
   }

   gBonusTable.iCount = 0;
   for (i = 0; i < MAX_ADD_ONS; i++) {
      gBonusTable.bonus[i] = NULL;
   }

   gBonusTemplateTable.iCount = 0;
   for (i = 0; i < MAX_BONUS_TEMPLATES; i++) {
      gBonusTemplateTable.template[i] = NULL;
   }


   roadmap_config_declare ("preferences", &RoadMapConfigCustomBonusRadius, "30", NULL);

   roadmap_config_declare ("user", &RoadMapConfigCustomBonusLastID, "0", NULL);

   roadmap_config_declare_enumeration ("preferences", &RoadMapConfigCustomBonusFeatureEnabled, NULL, "yes",
                  "no", NULL);

   roadmap_config_declare ("preferences", &RoadMapConfigBonusURL, "", NULL);

}


//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeBonus_BonusTemplate_Init (RTBonusTemplate *pbonusTemplate) {
   pbonusTemplate->iID = -1;
   pbonusTemplate->iNumPoints = 0;
   pbonusTemplate->iRadius = 0;
   pbonusTemplate->pIconName = NULL;
   pbonusTemplate->pText = NULL;
   pbonusTemplate->pTextColor = NULL;
   pbonusTemplate->pTitle = NULL;
   pbonusTemplate->pTitleColor = NULL;
   pbonusTemplate->pWebDlgExtraParams = NULL;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeBonus_BonusTemplate_Count (void) {
   return gBonusTemplateTable.iCount;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RealtimeBonus_BonusTemplate_IsEmpty (void) {

   return (0 == gBonusTemplateTable.iCount);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
RTBonusTemplate *RealtimeBonus_BonusTemplate_Get_Record (int index) {
   if (index >= MAX_BONUS_TEMPLATES)
      return NULL;

   return gBonusTemplateTable.template[index];
}

//////////////////////////////////////////////////////////////////////////////////////////////////
RTBonusTemplate *RealtimeBonus_BonusTemplate_Get (int iID) {
   int i;
   for (i = 0; i < MAX_BONUS_TEMPLATES; i++) {
      if (gBonusTemplateTable.template[i] && gBonusTemplateTable.template[i]->iID == iID)
         return RealtimeBonus_BonusTemplate_Get_Record (i);
   }

   return NULL;

}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RealtimeBonus_BonusTemplate_Exists (int iID) {
   if (NULL == RealtimeBonus_BonusTemplate_Get (iID))
      return FALSE;

   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeBonus_BonusTemplate_RemoveFromTable (int iID) {
   int i;
   for (i = 0; i < MAX_BONUS_TEMPLATES; i++) {
      if (gBonusTemplateTable.template[i] && gBonusTemplateTable.template[i]->iID == iID) {
         free (gBonusTemplateTable.template[i]);
         gBonusTemplateTable.template[i] = NULL;
         gBonusTemplateTable.iCount--;
         return;
      }
   }
   roadmap_log( ROADMAP_DEBUG, "RealtimeBonus_BonusTemplate_RemoveFromTable - Id not found (id =%d)", iID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RealtimeBonus_BonusTemplate_Delete (int iID) {
   RTBonusTemplate *template = RealtimeBonus_BonusTemplate_Get (iID);

   if (template->pIconName)
      free (template->pIconName);

   if (template->pText)
      free (template->pText);

   if (template->pTextColor)
      free (template->pTextColor);

   if (template->pTitle)
      free (template->pTitle);

   if (template->pTitleColor)
      free (template->pTitleColor);

   if (template->pWebDlgExtraParams)
      free (template->pWebDlgExtraParams);


   RealtimeBonus_BonusTemplate_RemoveFromTable (iID);
   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RealtimeBonus_BonusTemplate_Add (RTBonusTemplate *pTemplate) {
   int index;
   int i;

   // Full?
   if (MAX_BONUS_TEMPLATES == RealtimeBonus_BonusTemplate_Count()){
      roadmap_log( ROADMAP_ERROR, "RealtimeBonus_BonusTemplate_Add() - Failed (Table is full)");
      return FALSE;
   }


   if (RealtimeBonus_BonusTemplate_Exists (pTemplate->iID))
      RealtimeBonus_BonusTemplate_Delete(pTemplate->iID);


   for (i = 0; i < MAX_BONUS_TEMPLATES; i++) {
      if (gBonusTemplateTable.template[i] == NULL) {
         index = i;
         break;
      }
   }

   gBonusTemplateTable.template[index] = calloc (1, sizeof(RTBonusTemplate));
   RealtimeBonus_BonusTemplate_Init (gBonusTemplateTable.template[index]);

   gBonusTemplateTable.template[index]->iID = pTemplate->iID;
   gBonusTemplateTable.template[index]->iRadius = pTemplate->iRadius;
   gBonusTemplateTable.template[index]->iNumPoints = pTemplate->iNumPoints;

   if (pTemplate->pIconName)
      gBonusTemplateTable.template[index]->pIconName = strdup (pTemplate->pIconName);

   if (pTemplate->pTitle)
      gBonusTemplateTable.template[index]->pTitle = strdup (pTemplate->pTitle);

   if (pTemplate->pTitleColor)
      gBonusTemplateTable.template[index]->pTitleColor = strdup (pTemplate->pTitleColor);

   if (pTemplate->pText)
      gBonusTemplateTable.template[index]->pText = strdup (pTemplate->pText);

   if (pTemplate->pTextColor)
      gBonusTemplateTable.template[index]->pTextColor = strdup (pTemplate->pTextColor);

   if (pTemplate->pWebDlgExtraParams)
      gBonusTemplateTable.template[index]->pWebDlgExtraParams = strdup (pTemplate->pWebDlgExtraParams);

   if (gBonusTemplateTable.template[index]->pIconName && roadmap_res_get(RES_BITMAP,RES_SKIN, gBonusTemplateTable.template[index]->pIconName) == NULL){
      roadmap_res_download(RES_DOWNLOAD_IMAGE, gBonusTemplateTable.template[index]->pIconName,NULL, "",FALSE,0, NULL, NULL );
   }


   gBonusTemplateTable.iCount++;

   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeBonus_Term (void) {
   int i;
   RTBonus * pbonus;
   RTBonusTemplate *template;

   // Delete all bonuses
   for (i = 0; i < MAX_ADD_ONS; i++) {
      pbonus = RealtimeBonus_Get_Record (i);

      if (pbonus){
         onBonusDelete(pbonus);
         free (pbonus);
      }

      gBonusTable.bonus[i] = NULL;
   }

   gBonusTable.iCount = 0;

   // Delete all templates
   for (i = 0; i < MAX_BONUS_TEMPLATES; i++) {
      template = RealtimeBonus_BonusTemplate_Get_Record (i);

      if (template){
         free (template);
      }

      gBonusTemplateTable.template[i] = NULL;
   }

   gBonusTemplateTable.iCount = 0;
}
