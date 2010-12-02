/* RealtimeExternalPoi.c - Manage External POIs
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi B.S
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
#include "../roadmap_gui.h"
#include "../roadmap_screen.h"
#include "../roadmap_config.h"
#include "../roadmap_object.h"
#include "../roadmap_types.h"
#include "../roadmap_time.h"
#include "../roadmap_messagebox.h"
#include "../roadmap_lang.h"
#include "../roadmap_res.h"
#include "../roadmap_res_download.h"
#include "../roadmap_object.h"
#include "../roadmap_math.h"

#include "../ssd/ssd_dialog.h"
#include "../ssd/ssd_container.h"
#include "../ssd/ssd_button.h"
#include "../ssd/ssd_text.h"
#include "../ssd/ssd_button.h"
#include "../ssd/ssd_bitmap.h"
#include "../ssd/ssd_popup.h"


#include "Realtime.h"
#include "RealtimeExternalPoi.h"
#include "RealtimeExternalPoiDlg.h"
#include "RealtimeExternalPoiNotifier.h"

static RoadMapConfigDescriptor RoadMapConfigMaxExternalPoisDisplay =
      ROADMAP_CONFIG_ITEM("ExternalPOI", "Max POIs Display");

static RoadMapConfigDescriptor RoadMapConfigExternalPoisFeatureEnabled =
      ROADMAP_CONFIG_ITEM("ExternalPOI", "Feature Enabled");

static RoadMapConfigDescriptor RoadMapConfigMaxExternalPoisDisplaySmallScreen =
      ROADMAP_CONFIG_ITEM("ExternalPOI", "Max POIs Display Small Screen");

static RoadMapConfigDescriptor RoadMapConfigExternalPoisURL =
      ROADMAP_CONFIG_ITEM("ExternalPOI", "URL");

static void RealtimeExternalPoi_AfterRefresh(void);
static void RemovePoiObject(RTExternalPoi *pEntity);
static BOOL is_visible(RTExternalPoi *poi);

static BOOL UpdatingDisplayList = FALSE;

typedef struct {
   RTExternalPoiType *externalPoiType[RT_MAXIMUM_EXTERNAL_POI_TYPES];
   int iCount;
} RTExternalPoiTypesList;

typedef struct {
   RTExternalPoi *externalPoiData[RT_MAXIMUM_EXTERNAL_POI_MAP_DISPLAY_COUNT];
   int iCount;
} RTExternalPoiList;

typedef struct {
   int entitiesID[RT_MAXIMUM_EXTERNAL_POI_MAP_DISPLAY_COUNT];
   int iCount;
} RTExternalPoiDisplayList;

RTExternalPoiTypesList     gExternalPoiTypesTable;
RTExternalPoiList          gExternalPoisTable;
RTExternalPoiDisplayList   gExternalPoisDisplayList;

static RoadMapScreenSubscriber prev_after_refresh = NULL;

//////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RealtimeExternalPoi_FeatureEnabled (void) {
   if (0 == strcmp (roadmap_config_get (&RoadMapConfigExternalPoisFeatureEnabled), "yes")){
      return TRUE;
   }

   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static void init_ExternalPoiTypesTable(void){
   int i;
   gExternalPoiTypesTable.iCount = 0;
   for (i = 0; i < RT_MAXIMUM_EXTERNAL_POI_TYPES; i++){
      if (gExternalPoiTypesTable.externalPoiType[i] != NULL)
         free(gExternalPoiTypesTable.externalPoiType[i]);

      gExternalPoiTypesTable.externalPoiType[i] = NULL;
   }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static void init_ExternalPoiTable(void){
   int i;
   gExternalPoisTable.iCount = 0;
   for (i = 0; i < RT_MAXIMUM_EXTERNAL_POI_MAP_DISPLAY_COUNT; i++){
      if (gExternalPoisTable.externalPoiData[i] != NULL)
         free(gExternalPoisTable.externalPoiData[i]);

      gExternalPoisTable.externalPoiData[i] = NULL;
   }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static void init_ExternalPoisDisplayTable(void){
   memset(&gExternalPoisDisplayList, 0, sizeof(gExternalPoisDisplayList));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
static int get_max_pois_display(void){
   if ((roadmap_canvas_width() < 300) || ((roadmap_canvas_height() < 300) && is_screen_wide()))
      return roadmap_config_get_integer(&RoadMapConfigMaxExternalPoisDisplaySmallScreen);
   else
      return roadmap_config_get_integer(&RoadMapConfigMaxExternalPoisDisplay);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
const char *RealtimeExternalPoi_GetUrl(void){
   return roadmap_config_get(&RoadMapConfigExternalPoisURL);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeExternalPoi_Init(void){

#ifdef _WIN32
   roadmap_config_declare_enumeration ("preferences", &RoadMapConfigExternalPoisFeatureEnabled, NULL, "no",
                  "yes", NULL);
#else
   roadmap_config_declare_enumeration ("preferences", &RoadMapConfigExternalPoisFeatureEnabled, NULL, "yes",
                  "no", NULL);
#endif
   if (!RealtimeExternalPoi_FeatureEnabled())
      return;

   // Initialize tables
   init_ExternalPoiTypesTable();
   init_ExternalPoiTable();
   init_ExternalPoisDisplayTable();
   RealtimeExternalPoiNotifier_DisplayedList_Init();

   // Initialize Config
   roadmap_config_declare ("preferences", &RoadMapConfigMaxExternalPoisDisplay, "4", NULL);

   roadmap_config_declare ("preferences", &RoadMapConfigMaxExternalPoisDisplaySmallScreen, "3", NULL);

   roadmap_config_declare( "preferences", &RoadMapConfigExternalPoisURL, "", NULL);



   prev_after_refresh = roadmap_screen_subscribe_after_refresh(RealtimeExternalPoi_AfterRefresh);


}

/////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeExternalPoi_Reset(void){
   RealtimeExternalPoi_Term();
   init_ExternalPoiTypesTable();
   init_ExternalPoiTable();
   init_ExternalPoisDisplayTable();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeExternalPoi_Term(void){
   if (!RealtimeExternalPoi_FeatureEnabled())
      return;

   init_ExternalPoiTypesTable();
   init_ExternalPoiTable();
   init_ExternalPoisDisplayTable();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeExternalPoi_ExternalPoiType_Init(RTExternalPoiType *pEntity) {
   pEntity->iID = -1;
   pEntity->iExternalPoiServiceID = -1;
   pEntity->iExternalPoiProviderID = -1;
   memset( pEntity->cBigIcon, 0, sizeof(pEntity->cBigIcon));
   memset( pEntity->cSmallIcon,0, sizeof(pEntity->cSmallIcon));
   memset( pEntity->cOnClickUrl, 0, sizeof(pEntity->cOnClickUrl));
   pEntity->iSize = -1;
   pEntity->iMaxDisplayZoomBigIcon= -1;
   pEntity->iMaxDisplayZoomSmallIcon = -1;
   pEntity->iPromotionType = -1;
   pEntity->iIsNavigable = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static void On_ExternalPoiType_Add(RTExternalPoiType *pEntity){

   //check if icons need to be downloaded
   if (pEntity->cBigIcon[0] != 0){
      if (roadmap_res_get(RES_BITMAP,RES_SKIN, pEntity->cBigIcon) == NULL){
         roadmap_res_download(RES_DOWNLOAD_IMAGE, pEntity->cBigIcon, NULL, "",FALSE, 0, NULL, NULL );
      }
   }

   if (pEntity->cSmallIcon[0] != 0){
      if (roadmap_res_get(RES_BITMAP,RES_SKIN, pEntity->cSmallIcon) == NULL){
         roadmap_res_download(RES_DOWNLOAD_IMAGE, pEntity->cSmallIcon, NULL, "",FALSE, 0, NULL, NULL );
      }
   }

}

/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RealtimeExternalPoi_ExternalPoiType_Add(RTExternalPoiType *pEntity) {

   RTExternalPoiType *externalPoiType;

   if (!RealtimeExternalPoi_FeatureEnabled())
      return TRUE;


   if (pEntity == NULL){
      roadmap_log(ROADMAP_ERROR, "RealtimeExternalPoi_ExternalPoiType_Add - Failed to add entry (entry is NULL)");
      return FALSE;
   }

   roadmap_log(ROADMAP_DEBUG, "RealtimeExternalPoi_ExternalPoiType_Add - id=%d,snall_icon=%s,large_icon=%s,size=%d, zoom_big=%d, zoom_sml=%d",pEntity->iID, pEntity->cSmallIcon, pEntity->cBigIcon, pEntity->iSize, pEntity->iMaxDisplayZoomBigIcon, pEntity->iMaxDisplayZoomSmallIcon);

   if( RealtimeExternalPoi_ExternalPoiType_Count() == RT_MAXIMUM_EXTERNAL_POI_TYPES){
      roadmap_log(ROADMAP_ERROR, "RealtimeExternalPoi_ExternalPoiType_Add - Failed to add entry (Table is full with %d entries)", RealtimeExternalPoi_ExternalPoiType_Count());
      return FALSE;
   }
   externalPoiType = calloc (1, sizeof(RTExternalPoiType));
   RealtimeExternalPoi_ExternalPoiType_Init(externalPoiType);

   (*externalPoiType)   = (*pEntity);
   gExternalPoiTypesTable.externalPoiType[gExternalPoiTypesTable.iCount] = externalPoiType;
   On_ExternalPoiType_Add(externalPoiType);
   gExternalPoiTypesTable.iCount++;

   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeExternalPoi_ExternalPoiType_Count(void)
{
   return gExternalPoiTypesTable.iCount;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RealtimeExternalPoi_ExternalPoiType_IsEmpty(void)
{
   return (gExternalPoiTypesTable.iCount == 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
RTExternalPoiType * RealtimeExternalPoi_ExternalPoiType_GetById(int iID)
{
   int i;
    for (i = 0; i < RT_MAXIMUM_EXTERNAL_POI_TYPES; i++){
       if (gExternalPoiTypesTable.externalPoiType[i] != NULL){
          if (gExternalPoiTypesTable.externalPoiType[i]->iID == iID)
             return gExternalPoiTypesTable.externalPoiType[i];
       }
    }

    return NULL;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeExternalPoi_ExternalPoi_Init(RTExternalPoi *pEntity) {
   pEntity->iID = -1;
   pEntity->isDiplayed = FALSE;
   pEntity->iTypeID = -1;
   pEntity->iServerID  = -1;
   pEntity->iProviderID  = -1;
   pEntity->iServiceID  = -1;
   pEntity->ExternalPoiType = NULL;
   pEntity->iLongitude = -1;
   pEntity->iLatitude = -1;
   pEntity->iCreationTime = -1;
   pEntity->iUpdateTime = -1;
   pEntity->iPromotionType = -1;
   pEntity->iIsPromotioned = -1;
   memset( pEntity->cResourceUrlParams,0, sizeof(pEntity->cResourceUrlParams));
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static void On_ExternalPoi_Add(RTExternalPoi *pEntity) {
}

/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RealtimeExternalPoi_ExternalPoi_Add(RTExternalPoi *pEntity) {
   RTExternalPoi *externalPoi;
   RTExternalPoiType *externalPoiType;
   int i;

   if (!RealtimeExternalPoi_FeatureEnabled())
      return TRUE;

   if (pEntity == NULL){
      roadmap_log(ROADMAP_ERROR, "RealtimeExternalPoi_ExternalPoi_Add - Failed to add entry (entry is NULL)");
      return FALSE;
   }
   roadmap_log(ROADMAP_DEBUG, "RealtimeExternalPoi_ExternalPoi_Add - id=%d,lat=%d,lon=%d,type=%d",pEntity->iID, pEntity->iLatitude, pEntity->iLongitude, pEntity->iTypeID);

   if( RealtimeExternalPoi_ExternalPoi_Count() == RT_MAXIMUM_EXTERNAL_POI_MAP_DISPLAY_COUNT){
      roadmap_log(ROADMAP_ERROR, "RealtimeExternalPoi_ExternalPoi_Add - Failed to add entry (Table is full with %d entries)", RealtimeExternalPoi_ExternalPoi_Count());
      return FALSE;
   }

   externalPoiType = RealtimeExternalPoi_ExternalPoiType_GetById(pEntity->iTypeID);
   if (externalPoiType == NULL){
      roadmap_log(ROADMAP_ERROR, "RealtimeExternalPoi_ExternalPoi_Add - Failed to add entry (Type %d is not defined)", pEntity->iTypeID);
      return FALSE;
   }

   externalPoi = calloc (1, sizeof(RTExternalPoi));
   RealtimeExternalPoi_ExternalPoi_Init(externalPoi);

   pEntity->ExternalPoiType = externalPoiType;
   (*externalPoi)   = (*pEntity);


   for (i = 0; i < RT_MAXIMUM_EXTERNAL_POI_MAP_DISPLAY_COUNT; i++){
         if (gExternalPoisTable.externalPoiData[i] == NULL){
            gExternalPoisTable.externalPoiData[i] = externalPoi;
            On_ExternalPoi_Add(externalPoi);
            gExternalPoisTable.iCount++;
            return TRUE;
         }
   }
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RealtimeExternalPoi_ExternalPoi_Remove(int iID) {

   RTExternalPoi *pEntity;
   int index;

   if (!RealtimeExternalPoi_FeatureEnabled())
      return TRUE;

   roadmap_log(ROADMAP_DEBUG, "RealtimeExternalPoi_ExternalPoi_Remove - id =%d",iID);

   pEntity = RealtimeExternalPoi_ExternalPoi_GetById(iID);
   if (pEntity == NULL){
      roadmap_log(ROADMAP_ERROR, "RealtimeExternalPoi_ExternalPoi_Remove - Failed to find external POI for id =%d",iID);
      return TRUE;
   }

   index = RealtimeExternalPoi_ExternalPoi_GetIndexById(iID);
   if (index == -1){
      roadmap_log(ROADMAP_ERROR, "RealtimeExternalPoi_ExternalPoi_Remove - (2) Failed to find external POI for id =%d",iID);
      return TRUE;
   }

   if (gExternalPoisTable.externalPoiData[index]->isDiplayed){
      RemovePoiObject(gExternalPoisTable.externalPoiData[index]);
   }

   free(gExternalPoisTable.externalPoiData[index]);
   gExternalPoisTable.externalPoiData[index] = NULL;
   gExternalPoisTable.iCount--;

   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeExternalPoi_ExternalPoi_Count(void)
{
   return gExternalPoisTable.iCount;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RealtimeExternalPoi_ExternalPoi_IsEmpty(void)
{
   return (gExternalPoisTable.iCount == 0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
RTExternalPoi * RealtimeExternalPoi_ExternalPoi_GetById(int iID)
{
   int i;
    for (i = 0; i < RT_MAXIMUM_EXTERNAL_POI_MAP_DISPLAY_COUNT; i++){
       if (gExternalPoisTable.externalPoiData[i] != NULL){
          if (gExternalPoisTable.externalPoiData[i]->iID == iID)
             return gExternalPoisTable.externalPoiData[i];
       }
    }

    return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeExternalPoi_ExternalPoi_GetIndexById(int iID)
{
   int i;
    for (i = 0; i < RT_MAXIMUM_EXTERNAL_POI_MAP_DISPLAY_COUNT; i++){
       if (gExternalPoisTable.externalPoiData[i] != NULL){
          if (gExternalPoisTable.externalPoiData[i]->iID == iID)
             return i;
       }
    }

    return -1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static void OnPoiShortClick (const char *name,
                              const char *sprite,
                              const char *image,
                              const RoadMapGpsPosition *gps_position,
                              const RoadMapGuiPoint    *offset,
                              BOOL is_visible,
                              int scale,
                              int opacity,
                              const char *id,
                              const char *text) {

   RTExternalPoi * externalPoi;
   int PoiID = atoi(name);
   externalPoi = RealtimeExternalPoi_ExternalPoi_GetById(PoiID);
   if (externalPoi != NULL){
      RealtimeExternalPoiDlg(externalPoi);
   }
   else{
      roadmap_log(ROADMAP_ERROR, "OnPoiShortClick - Failed to find external POI for id =%s",name);
   }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static void CreatePoiObject(RTExternalPoi *pEntity)
{
   RoadMapDynamicString Image;
   RoadMapDynamicString GUI_ID;
   RoadMapDynamicString Group = roadmap_string_new( "RealtimeExternalPoi");
   RoadMapDynamicString Name;
   RoadMapDynamicString Sprite= roadmap_string_new( "Waypoint");
   char                 text[128];
   RoadMapGpsPosition   Pos;
   RoadMapGuiPoint      Offset;
   RoadMapImage         image;

   if (pEntity == NULL){
      roadmap_log(ROADMAP_ERROR, "CreatePoiObject -(entry is NULL)");
      return;
   }

   roadmap_log(ROADMAP_DEBUG, "RealtimeExternalPoi_CreatePoiObject - id =%d",pEntity->iID);

   if (pEntity->isDiplayed){
      return;
   }

   snprintf(text, sizeof(text), "%d", pEntity->iID);
   Name  = roadmap_string_new( text);

   Pos.longitude = pEntity->iLongitude;
   Pos.latitude = pEntity->iLatitude;
   Offset.x = 4;
   //main object
   image = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, pEntity->ExternalPoiType->cBigIcon);

   if (image)
      Offset.y = Offset.y = -roadmap_canvas_image_height(image) /2 +4;
   else
      Offset.y = 0;

   Image = roadmap_string_new(pEntity->ExternalPoiType->cBigIcon);
   snprintf(text, sizeof(text), "RealtimeExternalPoi_%d_Big", pEntity->iID);
   GUI_ID = roadmap_string_new(text);
   roadmap_object_add_with_priority( Group, GUI_ID, Name, Sprite, Image, &Pos, &Offset,
                     OBJECT_ANIMATION_FADE_IN | OBJECT_ANIMATION_FADE_OUT | OBJECT_ANIMATION_WHEN_VISIBLE, NULL, OBJECT_PRIORITY_HIGHEST+pEntity->iPromotionType);
   roadmap_object_set_action(GUI_ID, OnPoiShortClick);
   roadmap_object_set_zoom (GUI_ID, -1, pEntity->ExternalPoiType->iMaxDisplayZoomBigIcon);

   roadmap_string_release(Image);
   roadmap_string_release(GUI_ID);

   if (pEntity->ExternalPoiType->cSmallIcon[0] != 0){
      image = (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, pEntity->ExternalPoiType->cSmallIcon);
      if (image)
          Offset.y = -roadmap_canvas_image_height(image) /2 +4;
      Image = roadmap_string_new(pEntity->ExternalPoiType->cSmallIcon);
      snprintf(text, sizeof(text), "RealtimeExternalPoi_%d_Small", pEntity->iID);
      GUI_ID = roadmap_string_new(text);
      roadmap_object_add( Group, GUI_ID, Name, Sprite, Image, &Pos, &Offset,
                        OBJECT_ANIMATION_FADE_IN | OBJECT_ANIMATION_FADE_OUT | OBJECT_ANIMATION_WHEN_VISIBLE, NULL);
      roadmap_object_set_action(GUI_ID, OnPoiShortClick);
      roadmap_object_set_zoom (GUI_ID, pEntity->ExternalPoiType->iMaxDisplayZoomBigIcon+1, pEntity->ExternalPoiType->iMaxDisplayZoomSmallIcon);

      roadmap_string_release(Image);
      roadmap_string_release(GUI_ID);

   }

   //Mark the POI as displayed
   pEntity->isDiplayed = TRUE;

   // Add ID to list of POIs displayed
   RealtimeExternalPoiNotifier_DisplayedList_add_ID(pEntity->iServerID);


}

/////////////////////////////////////////////////////////////////////////////////////////////////
static void RemovePoiObject(RTExternalPoi *pEntity)
{
   RoadMapDynamicString GUI_ID;
   char                 text[128];
   BOOL                 refresh = FALSE;

   if (pEntity == NULL){
      roadmap_log(ROADMAP_ERROR, "RemovePoiObject -(entry is NULL)");
      return;
   }

   roadmap_log(ROADMAP_DEBUG, "RealtimeExternalPoi_RemovePoiObject - id =%d",pEntity->iID);

   if (is_visible(pEntity))
      refresh = TRUE;

   snprintf(text, sizeof(text), "RealtimeExternalPoi_%d_Big", pEntity->iID);
   GUI_ID = roadmap_string_new(text);
   roadmap_object_remove(GUI_ID);
   roadmap_string_release(GUI_ID);

   if (pEntity->ExternalPoiType->cSmallIcon[0] != 0){
      snprintf(text, sizeof(text), "RealtimeExternalPoi_%d_Small", pEntity->iID);
      GUI_ID = roadmap_string_new(text);
      roadmap_object_remove(GUI_ID);
      roadmap_string_release(GUI_ID);
   }

   pEntity->isDiplayed = FALSE;
   if (refresh)
      roadmap_screen_refresh();
}
/////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeExternalPoi_DisplayList_Count() {
   return gExternalPoisDisplayList.iCount;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RealtimeExternalPoi_DisplayList_IsEmpty() {

   return (gExternalPoisDisplayList.iCount == 0);
}


/////////////////////////////////////////////////////////////////////////////////////////////////
static void remove_all_objects(void){
   int i;
   for (i = 0; i < RealtimeExternalPoi_ExternalPoi_Count(); i++){
       if ((gExternalPoisTable.externalPoiData[i] != NULL) && (gExternalPoisTable.externalPoiData[i]->isDiplayed)){
          RemovePoiObject(gExternalPoisTable.externalPoiData[i]);
       }
    }



}

/////////////////////////////////////////////////////////////////////////////////////////////////
static int RealtimeExternalPoi_DisplayList_Compare (const void *r1, const void *r2) {
   int id1 = *((int *)r1);
   int id2 = *((int *)r2);

   RTExternalPoi *record1 = RealtimeExternalPoi_ExternalPoi_GetById(id1);
   RTExternalPoi *record2 = RealtimeExternalPoi_ExternalPoi_GetById(id2);;

   if ((record1 == NULL) || (record2 == NULL))
      return 0;

   return record2->iPromotionType - record1->iPromotionType;

}

/////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL is_visible(RTExternalPoi *poi) {
   RoadMapPosition position;
   BOOL visible;

   if (poi == NULL){
      roadmap_log(ROADMAP_ERROR, "RealtimeExternalPoi.is_visible -(poi is NULL)");
      return FALSE;
   }

   position.latitude = poi->iLatitude;
   position.longitude = poi->iLongitude;

   visible = roadmap_math_point_is_visible(&position);

#ifdef VIEW_MODE_3D_OGL
   if (roadmap_screen_get_view_mode() == VIEW_MODE_3D &&
       roadmap_canvas3_get_angle() > 0.8) {
      RoadMapGuiPoint object_coord;
      roadmap_math_coordinate(&position, &object_coord);
      roadmap_math_rotate_project_coordinate (&object_coord);

      if (1.0 * object_coord.y / roadmap_canvas_height() < 0.1) {
         visible = FALSE;
      }
   }
#endif

   return visible;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeExternalPoi_UpdateDisplayList(void) {
   int i;
   int count = 0;
   int max = get_max_pois_display();


   if (UpdatingDisplayList)
      return;

   // check if we need to remove something
   for (i = 0; i < RealtimeExternalPoi_DisplayList_Count(); i++){
      RTExternalPoi * externalPoi = RealtimeExternalPoi_ExternalPoi_GetById(gExternalPoisDisplayList.entitiesID[i]);
      if (externalPoi != NULL){
         if  (externalPoi->isDiplayed){
            if (is_visible(externalPoi) && (count < max) ){
               count++;
            }
            else{
               RemovePoiObject(externalPoi);
            }
         }
      }
   }


   for (i = 0; i < RealtimeExternalPoi_DisplayList_Count(); i++){
      RTExternalPoi * externalPoi = RealtimeExternalPoi_ExternalPoi_GetById(gExternalPoisDisplayList.entitiesID[i]);
      if (externalPoi != NULL){
         if ((count < max) && (is_visible(externalPoi)) && !externalPoi->isDiplayed){
            CreatePoiObject(externalPoi);
            count++;
         }
      }
   }

}

/////////////////////////////////////////////////////////////////////////////////////////////////
static void RealtimeExternalPoi_AfterRefresh(void)
{
   static RoadMapArea          LastMapPos;
   RoadMapArea MapPosition;

   if (RealtimeExternalPoi_DisplayList_IsEmpty()){
      if (prev_after_refresh) {
         (*prev_after_refresh) ();
      }
      return;
   }

   roadmap_math_displayed_screen_edges( &MapPosition);
   if((LastMapPos.west  == MapPosition.west ) &&
      (LastMapPos.south == MapPosition.south) &&
      (LastMapPos.east  == MapPosition.east ) &&
      (LastMapPos.north == MapPosition.north))
   {
      if (prev_after_refresh) {
         (*prev_after_refresh) ();
      }
      return ;
   }

   LastMapPos = MapPosition;
   RealtimeExternalPoi_UpdateDisplayList();
   if (prev_after_refresh) {
      (*prev_after_refresh) ();
   }
}


/////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeExternalPoi_DisplayList(void) {
   int i;
   int count = 0;
   int max = get_max_pois_display();

   if (!RealtimeExternalPoi_FeatureEnabled())
      return;

   if (RealtimeExternalPoi_DisplayList_IsEmpty())
      return;

   // Sort the display list according to priority
   qsort (&gExternalPoisDisplayList.entitiesID[0], RealtimeExternalPoi_DisplayList_Count(), sizeof(int), RealtimeExternalPoi_DisplayList_Compare);

   for (i = 0; i < RealtimeExternalPoi_DisplayList_Count(); i++){
      RTExternalPoi * externalPoi = RealtimeExternalPoi_ExternalPoi_GetById(gExternalPoisDisplayList.entitiesID[i]);
      if (externalPoi != NULL){
         if ((count < max) && (is_visible(externalPoi))){
            CreatePoiObject(externalPoi);
            count++;
         }
      }
   }
   UpdatingDisplayList = FALSE;
   roadmap_screen_refresh();

}

/////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeExternalPoi_DisplayList_clear() {
   remove_all_objects();
   memset(&gExternalPoisDisplayList, 0, sizeof(gExternalPoisDisplayList));
   UpdatingDisplayList = TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeExternalPoi_DisplayList_add_ID(int ID) {
   if (!RealtimeExternalPoi_FeatureEnabled())
      return;

   gExternalPoisDisplayList.entitiesID[gExternalPoisDisplayList.iCount++] = ID;

}

/////////////////////////////////////////////////////////////////////////////////////////////////
int RealtimeExternalPoi_DisplayList_get_ID(int index) {
   return gExternalPoisDisplayList.entitiesID[index];
}

