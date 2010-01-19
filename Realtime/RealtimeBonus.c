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
#include "../roadmap_screen.h"
#include "../roadmap_alerter.h"
#include "../roadmap_ticker.h"
#include "../roadmap_res.h"
#include "../roadmap_messagebox.h"
#include "../roadmap_twitter.h"
#include "../editor/editor_points.h"
#include "../editor/editor_screen.h"
#include "../roadmap_res_download.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_popup.h"
#include "ssd/ssd_button.h" 

#include "Realtime.h"
#include "RealtimeBonus.h"

static RTBonusTable gBonusTable;

#define ANIMATION_SOUND "bonus"

roadmap_alert_providor RoadmapRealTimeMapbonusnsProvidor = { "RealTimeAlerts", RealtimeBonus_Count,
      RealtimeBonus_Get_Id, RealtimeBonus_Get_Position, RealtimeBonus_Get_Speed, NULL, NULL, NULL,
      RealtimeBonus_Get_Distance, NULL, RealtimeBonus_Is_Alertable, NULL, NULL, NULL,
      RealtimeBonus_Check_Same_Street, RealtimeBonus_HandleEvent };

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
   
   if ( (index == 5) && (repeat == 2)) {
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
   if (index >= RealtimeBonus_Count ())
      return NULL;
   
   return gBonusTable.bonus[index];
   
}

//////////////////////////////////////////////////////////////////////////////////////////////////
RTBonus *RealtimeBonus_Get (int iID) {
   int i;
   for (i = 0; i < RealtimeBonus_Count (); i++) {
      if (gBonusTable.bonus[i] && gBonusTable.bonus[i]->iID == iID)
         return RealtimeBonus_Get_Record (i);
   }
   
   return NULL;
   
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeBonus_RemoveFromTable (int iID) {
   int i;
   for (i = 0; i < RealtimeBonus_Count (); i++) {
      if (gBonusTable.bonus[i] && gBonusTable.bonus[i]->iID == iID) {
         free (gBonusTable.bonus[i]);
         gBonusTable.bonus[i] = NULL;
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

//////////////////////////////////////////////////////////////////////////////////////////////////
static int on_close (SsdWidget widget, const char *new_value) {
   if (ssd_dialog_is_currently_active () && (!strcmp (ssd_dialog_currently_active_name (),
                  "BonusPopUPDlg")))
      ssd_dialog_hide_current (dec_close);
   
   return 0;
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
void RealtimeBonus_PopUp (int iID) {
   SsdWidget popup, spacer, container, button, text, icon;
   char msg[512];
   RTBonus *pbonus = RealtimeBonus_Get (iID);
   const char *title;
   int width;
   
   if (!pbonus)
      return;

   if (ssd_dialog_is_currently_active () && (!strcmp (ssd_dialog_currently_active_name (),
                  "BonusPopUPDlg")))
      ssd_dialog_hide_current (dec_cancel);
   
   if (pbonus->iType == BONUS_TYPE_POINTS){
      title = roadmap_lang_get ("Bonus points");
   }
   else if (pbonus->iType == BONUS_TYPE_TREASURE){
      title = roadmap_lang_get ("Treasure chest");
   }
   
   popup = ssd_popup_new ("BonusPopUPDlg", title, NULL,
                  SSD_MAX_SIZE, SSD_MIN_SIZE, &pbonus->position, SSD_POINTER_LOCATION
                                 | SSD_ROUNDED_BLACK | SSD_HEADER_BLACK);
   
   spacer = ssd_container_new ("space", "", SSD_MIN_SIZE, 5, SSD_END_ROW);
   ssd_widget_set_color (spacer, NULL, NULL);
   ssd_widget_add (popup, spacer);
   
   container = ssd_container_new ("IconCon", "", 50, SSD_MIN_SIZE, SSD_ALIGN_RIGHT);
   ssd_widget_set_color (container, NULL, NULL);
   icon = ssd_bitmap_new ("bonusIcon", pbonus->pIconName, SSD_ALIGN_VCENTER | SSD_ALIGN_CENTER);
   ssd_widget_add (container, icon);
   ssd_widget_add (popup, container);
   
   if (pbonus->iType == BONUS_TYPE_POINTS){
      snprintf (msg, sizeof(msg), "%d %s", pbonus->iNumPoints, roadmap_lang_get ("points!"));
   }
   else if (pbonus->iType == BONUS_TYPE_TREASURE){
      if (!Realtime_is_random_user())
         snprintf (msg, sizeof(msg), "%s", roadmap_lang_get ("There may be presents in this treasure chest! You'll know when you drive over it."));
      else
         snprintf (msg, sizeof(msg), "%s", roadmap_lang_get ("There may be presents in this treasure chest! You'll know when you drive over it. Note: You need to be a registered user in order to receive the gift inside. Register in 'Settings > Profile'"));
   }
   
   if (roadmap_canvas_width() > roadmap_canvas_height())
      width = roadmap_canvas_height() - 70;
   else
      width = roadmap_canvas_width() - 70;      
   container = ssd_container_new ("IconCon", "", width, SSD_MIN_SIZE, 0);
   ssd_widget_set_color (container, NULL, NULL);
   text = ssd_text_new ("PointsTxt", msg, 16, SSD_END_ROW);
   ssd_text_set_color (text, "#ffffff");
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
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static void OnbonusShortClick (const char *name,
               const char *sprite,
               const char *image,
               const RoadMapGpsPosition *gps_position,
               const char *id) {
   RealtimeBonus_PopUp (extract_id (id));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int onBonusAdd (RTBonus *pApdon) {
   RoadMapGpsPosition Pos;
   
   RoadMapDynamicString Group = roadmap_string_new ("BonusPoints");
   RoadMapDynamicString GUI_ID = roadmap_string_new (pApdon->sGUIID);
   RoadMapDynamicString Name = roadmap_string_new (pApdon->sGUIID);
   RoadMapDynamicString Sprite = roadmap_string_new ("Bonus");
   RoadMapDynamicString Image = roadmap_string_new (pApdon->pIconName);
   roadmap_object_add (Group, GUI_ID, Name, Sprite, Image);
   
   Pos.longitude = pApdon->position.longitude;
   Pos.latitude = pApdon->position.latitude;
   Pos.altitude = 0;
   Pos.speed = 0;
   Pos.steering = 0;
   
   roadmap_object_move (GUI_ID, &Pos);
   roadmap_object_set_action (GUI_ID, OnbonusShortClick);
   roadmap_string_release (Group);
   roadmap_string_release (GUI_ID);
   roadmap_string_release (Name);
   roadmap_string_release (Sprite);
   roadmap_string_release (Image);
   return TRUE;
}

static void on_resource_downloaded (const char* res_name, int status, void *context, char *last_modified){
   int i;
   for (i = 0; i < RealtimeBonus_Count (); i++) {
       if (gBonusTable.bonus[i] && !strcmp(res_name, gBonusTable.bonus[i]->pIconName))
          onBonusAdd(gBonusTable.bonus[i]);
    }
}
//////////////////////////////////////////////////////////////////////////////////////////////////
static int onBonusDelete (RTBonus *pApdon) {
   RoadMapDynamicString GUI_ID = roadmap_string_new (pApdon->sGUIID);
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
   
   // Full?
   if (MAX_ADD_ONS == gBonusTable.iCount)
      return FALSE;

   if (RealtimeBonus_Exists (pbonus->iID))
      RealtimeBonus_Delete(pbonus->iID);

   if (RealtimeBonus_IsEmpty ())
      roadmap_alerter_register (&RoadmapRealTimeMapbonusnsProvidor);
   
   index = gBonusTable.iCount;
   
   gBonusTable.bonus[index] = calloc (1, sizeof(RTBonus));
   RealtimeBonus_Record_Init (gBonusTable.bonus[index]);
   gBonusTable.bonus[index]->iID = pbonus->iID;
   gBonusTable.bonus[index]->iType = pbonus->iType;
   gBonusTable.bonus[index]->iNumPoints = pbonus->iNumPoints;
   gBonusTable.bonus[index]->iToken = pbonus->iToken;
   gBonusTable.bonus[index]->iRadius = pbonus->iRadius;
   gBonusTable.bonus[index]->position.latitude = pbonus->position.latitude;
   gBonusTable.bonus[index]->position.longitude = pbonus->position.longitude;
   gBonusTable.bonus[index]->pIconName = strdup (pbonus->pIconName);
   gBonusTable.bonus[index]->collected = FALSE;
   RealtimeBonus_CreateGUIID (gBonusTable.bonus[index]);
   gBonusTable.iCount++;
   
   if (roadmap_res_get(RES_BITMAP,RES_SKIN, gBonusTable.bonus[index]->pIconName) == NULL){
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
   
   if ((pbonus->iType == BONUS_TYPE_TREASURE) && Realtime_is_random_user()){
      roadmap_messagebox_timeout("", roadmap_lang_get ("There may be presents in this treasure chest but you need to be a registered user in order to get them. Register in 'Settings > Profile'"), 10);
      return TRUE;
   }

   roadmap_log( ROADMAP_DEBUG, "Yeahhh Collecting a gift id=%d)", iID);
   Play_Animation_Sound ();
   RealtimeBonus_Animate_Pacman ();
   Realtime_CollectBonus(iID, pbonus->iToken, roadmap_twitter_is_munching_enabled());
   onBonusDelete(pbonus);
   return TRUE; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeBonus_CollectedPointsConfirmed(int iID, int iType, int iPoints, BOOL bHasGift, BOOL bIsBigPrize, const char *gift){
   
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
         if (!bHasGift){ // The chest box was empty
            roadmap_messagebox_timeout("", roadmap_lang_get ("Bummer, this treasure chest was emptied. Keep searching for other chests, some have valuable prizes in them."), 10);
         }
         else{ // We got a gift!
            char msg[250];
            if (iPoints != 0){
               snprintf(msg, sizeof(msg),roadmap_lang_get ("Huray! You just won %s and an extra %d points. Check your email for info."), gift, iPoints);
               roadmap_messagebox("",msg);
               editor_points_add_new_points (iPoints);
               editor_points_display_new_points_timed (iPoints, 6, bonus_points);
            }
            else{
               if (bIsBigPrize){
                  snprintf(msg, sizeof(msg),roadmap_lang_get ("Huray!!! You won the big prize:  %s . Check your email for info."), gift);
                  roadmap_messagebox("",msg);
               }
               else{
                  snprintf(msg, sizeof(msg),roadmap_lang_get ("Huray! You just won %s. Check your email for info."), gift);
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
   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeBonus_Init (void) {
   int i;
   gBonusTable.iCount = 0;
   for (i = 0; i < MAX_ADD_ONS; i++) {
      gBonusTable.bonus[i] = NULL;
   }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeBonus_Term (void) {
   int i;
   RTBonus * pbonus;
   
   for (i = 0; i < RealtimeBonus_Count (); i++) {
      pbonus = RealtimeBonus_Get_Record (i);
      if (pbonus)
         free (pbonus);
      gBonusTable.bonus[i] = NULL;
   }
   
   gBonusTable.iCount = 0;
   
}
