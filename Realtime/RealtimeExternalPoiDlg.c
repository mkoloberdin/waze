/* RealtimeExternalPoiDlg.c - Manage External POIs Dialog
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
#include "../roadmap_config.h"
#include "../roadmap_object.h"
#include "../roadmap_start.h"
#include "../roadmap_lang.h"
#include "../roadmap_types.h"
#include "../roadmap_time.h"
#include "../roadmap_messagebox.h"
#include "../roadmap_lang.h"
#include "../roadmap_res.h"
#include "../roadmap_res_download.h"
#include "../roadmap_object.h"
#include "../roadmap_browser.h"
#include "../navigate/navigate_main.h"
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
#include "RealtimePopUp.h"

extern int main_navigator( const RoadMapPosition *point,
                           address_info_ptr       ai);


static void ExternalPoiDlgTimer(void);
static void draw_browser(RTExternalPoi *externalPoi, RoadMapGuiRect *rect, int flags, BOOL force);

static  RoadMapGuiRect old_rect = {-1, -1, -1, -1};

static int g_dlg_seconds;
static BOOL g_timer_active = FALSE;

#ifndef TOUCH_SCREEN
// Context menu:
typedef enum external_poi_context_menu_items
{
   ep_cm_navigate,
   ep_cm_info,
   ep_cm_promo,
   ep_cm_cancel,
   ep_cm__count,
   ep_cm__invalid
}  external_poi_context_menu_items;

// Context menu:
static ssd_cm_item      context_menu_items[] =
{
   SSD_CM_INIT_ITEM  ( "Navigate",     ep_cm_navigate),
   SSD_CM_INIT_ITEM  ( "Info",         ep_cm_info),
   SSD_CM_INIT_ITEM  ( "Promotion",    ep_cm_promo),
};

static   BOOL g_context_menu_is_active= FALSE;

static ssd_contextmenu  context_menu = SSD_CM_INIT_MENU( context_menu_items);


#else
/////////////////////////////////////////////////////////////////////////////////////////////////
static int on_button_close (SsdWidget widget, const char *new_value){
   ssd_dialog_hide(EXTERNAL_POI_DLG_NAME, dec_close);
   RealtimePopUp_set_Stoped_Popups();
   return 1;
}

static int on_promo_button (SsdWidget widget, const char *new_value){
   char *icon[3];
   RTExternalPoi *pEntity = (RTExternalPoi *)widget->context;

   if (!strcmp(ssd_button_get_name(widget), "button_promo")){
      icon[0] = "button_info";
      icon[1] = "button_promo";
      pEntity->bShowPromo = TRUE;
      draw_browser(pEntity, &old_rect, 0, TRUE);
   }
   else{
      icon[0] = "button_promo";
      icon[1] = "button_info";
      pEntity->bShowPromo = FALSE;
      draw_browser(pEntity, &old_rect, 0,  TRUE);
      RealtimeExternalPoiNotifier_NotifyOnInfoPressed(pEntity->iServerID, pEntity->iPromotionID);
   }
   icon[2] = NULL;

   ssd_button_change_icon(widget, (const char **)icon, 2);
   return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static int on_button_navigate (SsdWidget widget, const char *new_value){
   RoadMapPosition pos;
   address_info ai;
   RTExternalPoi *pEntity = (RTExternalPoi *)widget->context;

   if (!pEntity)
      return 0;

   ai.name = "";
   ai.city = "";
   ai.country = "";
   ai.house = "";
   ai.state = "";
   ai.street = "";

   ssd_dialog_hide(EXTERNAL_POI_DLG_NAME, dec_close);

   pos.latitude = pEntity->iLatitude;
   pos.longitude = pEntity->iLongitude;
   RealtimeExternalPoiNotifier_NotifyOnNavigate(pEntity->iServerID);

   main_navigator(&pos, &ai);
   save_destination_to_history();
   return 1;

}

#endif

/////////////////////////////////////////////////////////////////////////////////////////////////
static void on_dialog_close  (int exit_code, void* context){
   old_rect.minx = old_rect.maxx = old_rect.miny = old_rect.maxy = -1;
   if (g_timer_active)
      roadmap_main_remove_periodic(ExternalPoiDlgTimer);
   RealtimeExternalPoi_SetScrolling(FALSE);
   roadmap_browser_hide();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static int get_browser_width(){
  return SSD_MAX_SIZE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static int get_browser_height(){
   /*if (roadmap_screen_is_hd_screen())
      return 235;
   else*/{
#ifndef TOUCH_SCREEN
      if (!is_screen_wide())
         return ADJ_SCALE(240);
#endif
      return ADJ_SCALE(155);
   }
}


/////////////////////////////////////////////////////////////////////////////////////////////////
static int get_screen_type(){
#ifndef TOUCH_SCREEN
      if (!is_screen_wide())
         return 1;
#endif
      return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
static char *build_url(RTExternalPoi *externalPoi, int iHeight, int iWidth, BOOL isPromotion){
   static char url[1024];
   int promotion_id = -1;

   if  (isPromotion)
      promotion_id = externalPoi->iPromotionID;

   snprintf(url, sizeof(url),"%s?poi_id=%d&sessionid=%d&cookie=%s&deviceid=%d&width=%d&height=%d&client_version=%s&lang=%s&t=%d%s&providerid=%d&serviceid=%d&promotionid=%d",
                RealtimeExternalPoi_GetUrl(),
                externalPoi->iServerID,
                Realtime_GetServerId(),
                Realtime_GetServerCookie(),
                RT_DEVICE_ID,
                iWidth,
                iHeight,
                roadmap_start_version(),
                roadmap_lang_get_system_lang(),
                get_screen_type(),
                externalPoi->cResourceUrlParams,
                externalPoi->iProviderID,
                externalPoi->iServiceID,
                promotion_id) ;

   return &url[0];
}

/////////////////////////////////////////////////////////////////////////////////////////////////
char *RealtimeExternalPoiDlg_GetPromotionUrl(RTExternalPoi *externalPoi){
   int width = roadmap_canvas_width();
   if (roadmap_canvas_height() < width)
      width = roadmap_canvas_height();

   width -= 40;

   return build_url(externalPoi,get_browser_height(),width, TRUE);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static void draw_browser(RTExternalPoi *externalPoi, RoadMapGuiRect *rect, int flags, BOOL force){
   int width, height;
   RMBrowserContext context;
   BOOL is_promotion;
   if (force || (old_rect.minx != rect->minx) || (old_rect.maxx != rect->maxx) || (old_rect.miny != rect->miny) || (old_rect.maxy != rect->maxy)){
      if ( (old_rect.minx != -1) && (old_rect.maxx != -1) && (old_rect.miny != -1) && (old_rect.maxy != -1))
         roadmap_browser_hide();
      old_rect = *rect;
      context.flags = BROWSER_FLAG_WINDOW_TYPE_TRANSPARENT|BROWSER_FLAG_WINDOW_TYPE_NO_SCROLL;
      context.rect = *rect;
      height = rect->maxy - rect->miny +1;
      width = rect->maxx - rect->minx +1;
      is_promotion = (RealtimeExternalPoi_Is_Promotion(externalPoi) &&  externalPoi->bShowPromo);
      strncpy_safe( context.url, build_url( externalPoi, height, width , is_promotion), WEB_VIEW_URL_MAXSIZE );
      context.attrs.on_close_cb = NULL;
      context.attrs.on_load_cb = NULL;
      context.attrs.data = NULL;
      context.attrs.title_attrs.title = NULL;
      roadmap_browser_show_embeded(&context);
   }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static void draw_browser_rect(SsdWidget widget, RoadMapGuiRect *rect, int flags){
   RTExternalPoi *externalPoi = (RTExternalPoi *)widget->context;

   if ((flags & SSD_GET_SIZE)){
      return;
   }

   if (externalPoi == NULL)
      return;

   draw_browser(externalPoi, rect, flags, FALSE);

}

#ifndef TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////////////////////////////////
static int Drive_sk_cb(SsdWidget widget, const char *new_value, void *context){
   RoadMapPosition pos;
   address_info ai;
   RTExternalPoi *pEntity = (RTExternalPoi *)context;
   if (!pEntity)
       return 0;

    ai.name = "";
    ai.city = "";
    ai.country = "";
    ai.house = "";
    ai.state = "";
    ai.street = "";

    ssd_dialog_hide(EXTERNAL_POI_DLG_NAME, dec_close);

    pos.latitude = pEntity->iLatitude;
    pos.longitude = pEntity->iLongitude;
    RealtimeExternalPoiNotifier_NotifyOnNavigate(pEntity->iServerID);

    main_navigator(&pos, &ai);

    return 1;
}


///////////////////////////////////////////////////////////////////////////////////////////
static void on_option_selected(  BOOL              made_selection,
                                 ssd_cm_item_ptr   item,
                                 void*             context)
{
   external_poi_context_menu_items   selection;
   RTExternalPoi *pEntity = (RTExternalPoi *)context;
   SsdWidget widget;

   g_context_menu_is_active = FALSE;

   if( !made_selection)
      return;

   selection = (external_poi_context_menu_items)item->id;

   switch( selection)
   {
      case ep_cm_navigate:
         Drive_sk_cb(NULL, NULL, context);
         break;

      case ep_cm_info:
         pEntity->bShowPromo = FALSE;
         roadmap_screen_refresh ();
         RealtimeExternalPoiNotifier_NotifyOnInfoPressed(pEntity->iServerID, pEntity->iPromotionID);
         break;

      case ep_cm_promo:
         pEntity->bShowPromo = TRUE;
         roadmap_screen_refresh ();
         break;

     case ep_cm_cancel:
         g_context_menu_is_active = FALSE;
         roadmap_screen_refresh ();
         break;

      default:
         break;
   }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static int on_options(SsdWidget widget, const char *new_value, void *context)
{
   int menu_x;
   RTExternalPoi *pEntity = (RTExternalPoi *)context;

   if  (ssd_widget_rtl (NULL))
      menu_x = SSD_X_SCREEN_RIGHT;
   else
      menu_x = SSD_X_SCREEN_LEFT;

   ssd_contextmenu_show_item( &context_menu,
                              ep_cm_info,
                              pEntity->bShowPromo,
                              FALSE);

   ssd_contextmenu_show_item( &context_menu,
                              ep_cm_promo,
                              !pEntity->bShowPromo,
                              FALSE);

   ssd_context_menu_show(  menu_x,              // X
                        SSD_Y_SCREEN_BOTTOM, // Y
                        &context_menu,
                        on_option_selected,
                        context,
                        dir_default,
                        0,
                        TRUE);

   g_context_menu_is_active = TRUE;
   return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static void set_softkeys(SsdWidget dlg, RTExternalPoi *externalPoi){
   dlg->context = externalPoi;

   if (RealtimeExternalPoi_Is_Promotion(externalPoi)){
      externalPoi->bShowPromo = TRUE;
      ssd_widget_set_left_softkey_callback(dlg, on_options);
      ssd_widget_set_left_softkey_text(dlg, roadmap_lang_get("Options"));
   }
   else{
      if (externalPoi->ExternalPoiType->iIsNavigable){
         ssd_widget_set_left_softkey_callback(dlg, Drive_sk_cb);
         ssd_widget_set_left_softkey_text(dlg, roadmap_lang_get("Navigate"));
      }
      else{
         ssd_widget_set_left_softkey_callback(dlg, NULL);
         ssd_widget_set_left_softkey_text(dlg, "");
      }
   }
}
#endif //TOUCH_SCREEN


/////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeExternalPoiDlg(RTExternalPoi *externalPoi){
   static SsdWidget dialog;
   SsdWidget button;
   SsdWidget browserCont;
   SsdSize dlg_size, cnt_size;
   char *icon[3];


   int browser_cont_flags = 0;
   roadmap_log(ROADMAP_DEBUG, "RealtimeExternalPoiDlg - external POI id =%d",externalPoi->iID);

   if ( dialog != NULL )
   {
      if (ssd_dialog_currently_active_name() &&
          !strcmp(ssd_dialog_currently_active_name(), EXTERNAL_POI_DLG_NAME))
         ssd_dialog_hide_current(dec_close);

      ssd_dialog_free( EXTERNAL_POI_DLG_NAME, FALSE );
      dialog = NULL;
   }


   dialog = ssd_dialog_new ( EXTERNAL_POI_DLG_NAME, "", on_dialog_close,
         SSD_CONTAINER_BORDER|SSD_DIALOG_FLOAT|SSD_DIALOG_MODAL|
         SSD_ALIGN_CENTER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_BLACK);


#ifndef TOUCH_SCREEN
   browser_cont_flags = SSD_ALIGN_VCENTER;
   ssd_dialog_add_vspace(dialog, ADJ_SCALE(2) ,0);
#else
   ssd_dialog_add_vspace(dialog, ADJ_SCALE(5) ,0);
#endif

   browserCont = ssd_container_new("RealtimeExternalPoiDlg.BrowserContainer","", get_browser_width(), get_browser_height() , SSD_ALIGN_CENTER|browser_cont_flags);
   browserCont->context = (void *)externalPoi;
   browserCont->draw = draw_browser_rect;
   ssd_widget_set_color(browserCont, NULL, NULL);
   ssd_widget_add(dialog, browserCont);

#ifdef TOUCH_SCREEN
    ssd_dialog_add_vspace(dialog, ADJ_SCALE(5) ,0);

    button = ssd_button_label("Close_button", roadmap_lang_get("Close"), SSD_ALIGN_CENTER, on_button_close);
    button->context = externalPoi;
    ssd_widget_add(dialog, button);


    if (externalPoi->ExternalPoiType->iIsNavigable){
       button = ssd_button_label("Navigate_button", roadmap_lang_get("Navigate"), SSD_ALIGN_CENTER, on_button_navigate);
       button->context = externalPoi;
       ssd_widget_add(dialog, button);
    }

    if (RealtimeExternalPoi_Is_Promotion(externalPoi)){
       icon[0] = "button_info";
       icon[1] = "button_promo";
       icon[2] = NULL;
       button = ssd_button_new( "Promo_button", "Promo", (const char**) &icon[0], 2, 0, on_promo_button );
       externalPoi->bShowPromo = TRUE;
       button->context = externalPoi;
       ssd_widget_add(dialog, button);
    }
    else{
       externalPoi->bShowPromo = FALSE;
    }
#else
    set_softkeys(dialog, externalPoi);
#endif
   ssd_dialog_activate (EXTERNAL_POI_DLG_NAME, NULL);
   if (!roadmap_screen_refresh())
      roadmap_screen_redraw();

   ssd_dialog_recalculate( EXTERNAL_POI_DLG_NAME );
   ssd_widget_get_size( dialog, &dlg_size, NULL );
   ssd_widget_get_size( browserCont, &cnt_size, NULL );


}

static void ExternalPoiDlgTimer(void){
   g_dlg_seconds --;
   if (g_dlg_seconds > 0){
      if (ssd_dialog_is_currently_active() && (!strcmp(ssd_dialog_currently_active_name(), EXTERNAL_POI_DLG_NAME))){
         char button_txt[20];
         SsdWidget dialog = ssd_dialog_get_currently_active();
         SsdWidget button = ssd_widget_get(dialog, "Close_button" );
         if (g_dlg_seconds != 0)
            sprintf(button_txt, "%s (%d)", roadmap_lang_get ("Close"), g_dlg_seconds);
         else
            sprintf(button_txt, "%s", roadmap_lang_get ("Close"));
         if (button)
            ssd_button_change_text(button,button_txt );
      }
      if (!roadmap_screen_refresh())
         roadmap_screen_redraw();
      return;
   }

   ssd_dialog_hide(EXTERNAL_POI_DLG_NAME, dec_close);
   if (RealtimeExternalPoi_IsPromotionsScrolling())
      ShowPromotions(FALSE);


   if (!roadmap_screen_refresh())
       roadmap_screen_redraw();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeExternalPoiDlg_Timed(RTExternalPoi *externalPoi, int seconds){

   RealtimeExternalPoiDlg(externalPoi);
   g_dlg_seconds = seconds;
   g_timer_active = TRUE;
   roadmap_main_set_periodic (1000, ExternalPoiDlgTimer);

}
