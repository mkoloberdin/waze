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
#include "RealtimeExternalPoiNotifier.h"

extern int main_navigator( const RoadMapPosition *point,
                           address_info_ptr       ai);

#define EXTERNAL_POI_DLG_NAME "RealtimeExternalPoiDlg"

static  RoadMapGuiRect old_rect = {-1, -1, -1, -1};

/////////////////////////////////////////////////////////////////////////////////////////////////
static int on_button_close (SsdWidget widget, const char *new_value){
   ssd_dialog_hide(EXTERNAL_POI_DLG_NAME, dec_close);
   return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static int on_button_navigate (SsdWidget widget, const char *new_value){
   RoadMapPosition pos;
   address_info ai;
   RTExternalPoi *pEntity = (RTExternalPoi *)widget->context;

   if (!pEntity)
      return 0;

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

/////////////////////////////////////////////////////////////////////////////////////////////////
static void on_dialog_close  (int exit_code, void* context){
   old_rect.minx = old_rect.maxx = old_rect.miny = old_rect.maxy = -1;
   roadmap_browser_hide();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static int get_browser_width(RTExternalPoi *externalPoi){
  return SSD_MAX_SIZE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static int get_browser_height(RTExternalPoi *externalPoi){
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
static int get_screen_type(){
#ifndef TOUCH_SCREEN
      if (!is_screen_wide())
         return 1;
#endif
      return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
static char *build_url(RTExternalPoi *externalPoi, int iHeight, int iWidth){
   static char url[1024];

   snprintf(url, sizeof(url),"%s?poi_id=%d&sessionid=%d&cookie=%s&deviceid=%d&width=%d&height=%d&client_version=%s&lang=%s&t=%d%s",
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
            externalPoi->cResourceUrlParams);
   return &url[0];
}

#ifndef TOUCH_SCREEN
/////////////////////////////////////////////////////////////////////////////////////////////////
static int Drive_sk_cb(SsdWidget widget, const char *new_value, void *context){
   RoadMapPosition pos;
   address_info ai;
   RTExternalPoi *pEntity = (RTExternalPoi *)context;
   if (!pEntity)
       return 0;

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

/////////////////////////////////////////////////////////////////////////////////////////////////
static void set_softkeys(SsdWidget dlg, RTExternalPoi *externalPoi){
   dlg->context = externalPoi;
   if (externalPoi->ExternalPoiType->iIsNavigable){
      ssd_widget_set_left_softkey_callback(dlg, Drive_sk_cb);
      ssd_widget_set_left_softkey_text(dlg, roadmap_lang_get("Navigate"));
   }
   else{
      ssd_widget_set_left_softkey_callback(dlg, NULL);
      ssd_widget_set_left_softkey_text(dlg, "");
   }

}
#endif //TOUCH_SCREEN


void draw_browser_rect(SsdWidget widget, RoadMapGuiRect *rect, int flags){
   RTExternalPoi *externalPoi = (RTExternalPoi *)widget->context;
   RMBrowserContext context;
   int width, height;
   if ((flags & SSD_GET_SIZE)){
      return;
   }

   if (externalPoi == NULL)
      return;

   if ( (old_rect.minx != rect->minx) || (old_rect.maxx != rect->maxx) || (old_rect.miny != rect->miny) || (old_rect.maxy != rect->maxy)){
      if ( (old_rect.minx != -1) && (old_rect.maxx != -1) && (old_rect.miny != -1) && (old_rect.maxy != -1))
         roadmap_browser_hide();
      old_rect = *rect;
      context.flags = BROWSER_FLAG_WINDOW_TYPE_TRANSPARENT|BROWSER_FLAG_WINDOW_TYPE_NO_SCROLL;
      context.rect = *rect;
      height = rect->maxy - rect->miny +1;
      width = rect->maxx - rect->minx +1;
      strncpy_safe( context.url, build_url( externalPoi, height, width ), WEB_VIEW_URL_MAXSIZE );
      context.attrs.on_close_cb = NULL;
      context.attrs.title_attrs.title = NULL;
      roadmap_browser_show_embeded(&context);
   }

}
/////////////////////////////////////////////////////////////////////////////////////////////////
void RealtimeExternalPoiDlg(RTExternalPoi *externalPoi){
   static SsdWidget dialog;
   SsdWidget button;
   SsdWidget browserCont;
   SsdSize dlg_size, cnt_size;

   int browser_cont_flags = 0;
   roadmap_log(ROADMAP_DEBUG, "RealtimeExternalPoiDlg - external POI id =%d",externalPoi->iID);

   RealtimeExternalPoiNotifier_NotifyOnPopUp(externalPoi->iServerID);

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
   ssd_dialog_add_vspace(dialog, 2 ,0);
#else
   ssd_dialog_add_vspace(dialog, 5 ,0);
#endif

   browserCont = ssd_container_new("RealtimeExternalPoiDlg.BrowserContainer","", get_browser_width(externalPoi), get_browser_height(externalPoi) , SSD_ALIGN_CENTER|browser_cont_flags);
   browserCont->context = (void *)externalPoi;
   browserCont->draw = draw_browser_rect;
   ssd_widget_set_color(browserCont, NULL, NULL);
   ssd_widget_add(dialog, browserCont);

#ifdef TOUCH_SCREEN
    ssd_dialog_add_vspace(dialog, 5 ,0);

    button = ssd_button_label("Close_button", roadmap_lang_get("Close"), SSD_ALIGN_CENTER, on_button_close);
    button->context = externalPoi;
    ssd_widget_add(dialog, button);

    if (externalPoi->ExternalPoiType->iIsNavigable){
       button = ssd_button_label("Navigate_button", roadmap_lang_get("Navigate"), SSD_ALIGN_CENTER, on_button_navigate);
       button->context = externalPoi;
       ssd_widget_add(dialog, button);
    }
#else
    set_softkeys(dialog, externalPoi);
#endif
   ssd_dialog_activate (EXTERNAL_POI_DLG_NAME, NULL);

   ssd_dialog_recalculate( EXTERNAL_POI_DLG_NAME );
   ssd_widget_get_size( dialog, &dlg_size, NULL );
   ssd_widget_get_size( browserCont, &cnt_size, NULL );


}
