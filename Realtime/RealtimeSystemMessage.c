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
#include "../roadmap_config.h"
#include "../roadmap_main.h"
#include "../roadmap_lang.h"
#include "../roadmap_res.h"
#include "../roadmap_res_download.h"
#include "../roadmap_screen.h"
#include "roadmap_messagebox.h"
#include "Realtime.h"
#include "RealtimeDefs.h"
#include "ssd/ssd_widget.h"
#include "ssd/ssd_dialog.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_bitmap.h"
#include "../roadmap_browser.h"

static RoadMapConfigDescriptor RoadMapConfigLastIdDisplayed =
      ROADMAP_CONFIG_ITEM("System Messages", "Last message ID displayed");

char DEFAULT_TITLE_TEXT_COLOR[] = "#f6a201";
char DEFAULT_MSG_TEXT_COLOR[] = "#ffffff";
#define DEFAULT_TITLE_TEXT_SIZE 20
#define DEFAULT_MSG_TEXT_SIZE   16

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void RTSystemMessage_Init( LPRTSystemMessage this)
{
   memset( this, 0, sizeof(RTSystemMessage));
   strncpy(this->titleTextColor, DEFAULT_TITLE_TEXT_COLOR, 16);
   strncpy(this->msgTextColor, DEFAULT_MSG_TEXT_COLOR, 16);
   this->titleTextSize = DEFAULT_TITLE_TEXT_SIZE;
   this->msgTextSize = DEFAULT_MSG_TEXT_SIZE;
}

void RTSystemMessage_Free( LPRTSystemMessage this)
{
   FREE_MEM(this->title)
   FREE_MEM(this->msg)
   FREE_MEM(this->icon)
   RTSystemMessage_Init( this);
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
static   RTSystemMessage   RTSystemMessageQueue[RTSYSTEMMESSAGE_QUEUE_MAXSIZE];
static   int               FirstItem = -1;
static   int               ItemsCount=  0;

static int IncrementIndex( int i)
{
   i++;

   if( i < RTSYSTEMMESSAGE_QUEUE_MAXSIZE)
      return i;

   return 0;
}

static int AllocCell()
{
   int iNextFreeCell;

   if( RTSYSTEMMESSAGE_QUEUE_MAXSIZE == ItemsCount)
      return -1;

   if( -1 == FirstItem)
   {
      FirstItem = 0;
      ItemsCount= 1;
      return 0;
   }

   iNextFreeCell = (FirstItem + ItemsCount);
   ItemsCount++;

   if( iNextFreeCell < RTSYSTEMMESSAGE_QUEUE_MAXSIZE)
      return iNextFreeCell;

   return (iNextFreeCell - RTSYSTEMMESSAGE_QUEUE_MAXSIZE);
}

void RTSystemMessagesDisplay_CB( int exit_code ){
   RTSystemMessagesDisplay();
}

void RTSystemMessagesDisplay_Timer( void ){
   RTSystemMessagesDisplay();
}

static LPRTSystemMessage AllocSystemMessage()
{
   LPRTSystemMessage pCell = NULL;
   int               iCell = AllocCell();
   if( -1 == iCell)
      return NULL;

   pCell = RTSystemMessageQueue + iCell;
   RTSystemMessage_Init( pCell);

   return pCell;
}

static BOOL PopOldest( LPRTSystemMessage pSM)
{
   LPRTSystemMessage pCell;

   if( !ItemsCount || (-1 == FirstItem))
   {
      if( pSM)
         RTSystemMessage_Init( pSM);
      return FALSE;
   }

   pCell = RTSystemMessageQueue + FirstItem;

   if( pSM)
   {
      // Copy item data:
      (*pSM) = (*pCell);
      // Detach item from queue:
      RTSystemMessage_Init( pCell);
   }
   else
      // Item is NOT being copied; Release item resources:
      RTSystemMessage_Free( pCell);

   if( 1 == ItemsCount)
   {
      ItemsCount =  0;
      FirstItem  = -1;
   }
   else
   {
      ItemsCount--;
      FirstItem = IncrementIndex( FirstItem);
   }

   return TRUE;
}
//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
void  RTSystemMessageQueue_Push( LPRTSystemMessage this)
{
   LPRTSystemMessage p;

   if( RTSystemMessageQueue_IsFull())
      PopOldest( NULL);

   if (!RTSystemMessageQueue_Size())
      roadmap_main_set_periodic( 60000, RTSystemMessagesDisplay_Timer );

   p = AllocSystemMessage();

   if (this->icon != NULL){
      if (roadmap_res_get(RES_BITMAP,RES_SKIN, this->icon) == NULL){
            roadmap_res_download(RES_DOWNLOAD_IMAGE, this->icon,NULL, "",FALSE,0, NULL, NULL );
      }
   }

   (*p) = (*this);

}

BOOL  RTSystemMessageQueue_Pop ( LPRTSystemMessage this)
{
   BOOL success = PopOldest( this);
   if (!RTSystemMessageQueue_Size())
      roadmap_main_remove_periodic(RTSystemMessagesDisplay_Timer );
   return success;
}

int   RTSystemMessageQueue_Size()
{ return ItemsCount;}

BOOL  RTSystemMessageQueue_IsEmpty()
{ return 0 == ItemsCount;}

BOOL  RTSystemMessageQueue_IsFull()
{ return RTSYSTEMMESSAGE_QUEUE_MAXSIZE == ItemsCount;}

void  RTSystemMessageQueue_Empty()
{ while( PopOldest( NULL));}

int RTSystemMessagesGetLastMessageDisplayed(void){
   return roadmap_config_get_integer(&RoadMapConfigLastIdDisplayed);
}

void RTSystemMessagesSetLastMessageDisplayed(int iLastMessageDisplayed){
   roadmap_config_set_integer(&RoadMapConfigLastIdDisplayed, iLastMessageDisplayed);
   roadmap_config_save(TRUE);
}

static int button_callback ( SsdWidget widget, const char *new_value ){
   ssd_dialog_hide_current(dec_close);
   RTSystemMessagesDisplay();
   return 1;
}

static SsdWidget create_dialog(void){
   SsdWidget title, dialog, text, bitmap, banner;
   int banner_height = 35;
   int title_bar_height = SSD_MIN_SIZE;

   dialog = ssd_dialog_new ( "SystemMessage", "", NULL,
           SSD_CONTAINER_BORDER|SSD_DIALOG_FLOAT|
           SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_BLACK);


   if (roadmap_screen_is_hd_screen()){
      banner_height = 60;
   }
   ssd_dialog_add_vspace(dialog, 5, SSD_END_ROW);
   banner = ssd_container_new ("banner", NULL, SSD_MAX_SIZE, banner_height, SSD_END_ROW);
   ssd_widget_set_color(banner, NULL, NULL);
   bitmap = ssd_bitmap_new("SystemMessageIcon", "empty_image",SSD_ALIGN_CENTER|SSD_END_ROW|SSD_ALIGN_VCENTER);
   ssd_widget_add(banner, bitmap);
   ssd_widget_add(dialog, banner);

   ssd_dialog_add_vspace(dialog, 5, SSD_END_ROW);
   title = ssd_container_new ("title_bar", NULL, SSD_MAX_SIZE, title_bar_height, SSD_END_ROW);
   ssd_widget_set_color(title, NULL, NULL);
   text = ssd_text_new ("title_text", "" , DEFAULT_TITLE_TEXT_SIZE, SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER);
   ssd_text_set_color(text, DEFAULT_TITLE_TEXT_COLOR);
   ssd_widget_add(title, text);
   ssd_widget_add(dialog, title);


   ssd_dialog_add_vspace(dialog, 5, SSD_END_ROW);
   text =  ssd_text_new ("text", "", DEFAULT_MSG_TEXT_SIZE, SSD_ALIGN_CENTER|SSD_TEXT_NORMAL_FONT);
   ssd_text_set_color(text, DEFAULT_MSG_TEXT_COLOR);
   ssd_widget_add (dialog,text);

   ssd_dialog_add_vspace(dialog, 14, SSD_END_ROW);

   ssd_widget_add (dialog,
      ssd_button_label ("confirm", roadmap_lang_get("Ok"),
                        SSD_ALIGN_CENTER|SSD_START_NEW_ROW|SSD_WS_DEFWIDGET|
                        SSD_WS_TABSTOP|SSD_END_ROW,
                        button_callback));

   ssd_dialog_add_vspace(dialog, 10, SSD_END_ROW);
   return dialog;
}

static void RTSystemMessagesDisplay_Dlg(const char *title, const char *titleTextColor, int titleTextSize, const char *text, const char *msgTextColor, int msgTextSize, const char *icon ){
   static SsdWidget dialog = NULL;
   if (!dialog){
      dialog = create_dialog();
   }

   if (!title){
      ssd_widget_hide(ssd_widget_get(dialog, "title_bar"));
   }
   else{
      ssd_widget_show(ssd_widget_get(dialog, "title_bar"));
   }

   if (!icon){
      ssd_widget_hide(ssd_widget_get(dialog, "banner"));
   }
   else{
      if (roadmap_res_get(RES_BITMAP,RES_SKIN, icon) != NULL){
         ssd_bitmap_update(ssd_widget_get(dialog, "SystemMessageIcon"), icon);
         ssd_widget_reset_position(ssd_widget_get(dialog, "SystemMessageIcon"));
         ssd_widget_reset_cache(ssd_widget_get(dialog, "SystemMessageIcon"));
         ssd_widget_show(ssd_widget_get(dialog, "banner"));
      }
   }
   ssd_text_set_text( ssd_widget_get(dialog, "title_text"), title );
   ssd_text_set_color(ssd_widget_get(dialog, "title_text"), strdup(titleTextColor));
   ssd_text_set_font_size(ssd_widget_get(dialog, "title_text"), titleTextSize);

   ssd_widget_reset_position(ssd_widget_get(dialog, "title_bar"));
   ssd_widget_reset_cache(ssd_widget_get(dialog, "title_bar"));
   ssd_widget_reset_position(ssd_widget_get(dialog, "title_text"));
   ssd_widget_reset_cache(ssd_widget_get(dialog, "title_text"));

   ssd_text_set_text( ssd_widget_get(dialog, "text"), text );
   ssd_text_set_color(ssd_widget_get(dialog, "text"), strdup(msgTextColor));
   ssd_text_set_font_size(ssd_widget_get(dialog, "text"), msgTextSize);
   ssd_dialog_activate("SystemMessage", NULL);
   roadmap_screen_redraw();

}

static  RoadMapGuiRect old_rect = {-1, -1, -1, -1};

/////////////////////////////////////////////////////////////////////////////////////////////////
static void draw_browser(RTSystemMessage *systemMessage, RoadMapGuiRect *rect, int flags, BOOL force){
   int width, height;
   RMBrowserContext context;
   if (force || (old_rect.minx != rect->minx) || (old_rect.maxx != rect->maxx) || (old_rect.miny != rect->miny) || (old_rect.maxy != rect->maxy)){
      if ( (old_rect.minx != -1) && (old_rect.maxx != -1) && (old_rect.miny != -1) && (old_rect.maxy != -1))
         roadmap_browser_hide();
      old_rect = *rect;
      context.flags = BROWSER_FLAG_WINDOW_TYPE_TRANSPARENT|BROWSER_FLAG_WINDOW_TYPE_NO_SCROLL;
      context.rect = *rect;
      height = rect->maxy - rect->miny +1;
      width = rect->maxx - rect->minx +1;
      strncpy_safe( context.url, systemMessage->msg, WEB_VIEW_URL_MAXSIZE );
      context.attrs.on_close_cb = NULL;
      context.attrs.on_load_cb = NULL;
      context.attrs.data = NULL;
      context.attrs.title_attrs.title = NULL;
      roadmap_browser_show_embeded(&context);
   }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static void draw_browser_rect(SsdWidget widget, RoadMapGuiRect *rect, int flags){
   RTSystemMessage *systemMessage = (RTSystemMessage *)widget->context;

   if ((flags & SSD_GET_SIZE)){
      return;
   }

   if (systemMessage == NULL)
      return;

   draw_browser(systemMessage, rect, flags, FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static void on_dialog_close  (int exit_code, void* context){
   old_rect.minx = old_rect.maxx = old_rect.miny = old_rect.maxy = -1;
   roadmap_browser_hide();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static int on_button_close (SsdWidget widget, const char *new_value){
   ssd_dialog_hide("SystemMessageWebBased", dec_close);
   return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static int get_browser_width(){
  return SSD_MAX_SIZE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
static int get_browser_height(){
#ifndef TOUCH_SCREEN
      if (!is_screen_wide())
         return 240;
      return 155;
#else
      return roadmap_canvas_height()/2;
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void RTSystemMessageShowWebMessageDlg(RTSystemMessage *systemMessage){
   static SsdWidget dialog;
   SsdWidget button;
   SsdWidget browserCont;
   SsdSize dlg_size, cnt_size;

   int browser_cont_flags = 0;

   if ( dialog != NULL )
   {
      if (ssd_dialog_currently_active_name() &&
          !strcmp(ssd_dialog_currently_active_name(), "SystemMessageWebBased"))
         ssd_dialog_hide_current(dec_close);

      ssd_dialog_free( "SystemMessageWebBased", FALSE );
      dialog = NULL;
   }


   dialog = ssd_dialog_new ( "SystemMessageWebBased", "", on_dialog_close,
         SSD_CONTAINER_BORDER|SSD_DIALOG_FLOAT|SSD_DIALOG_MODAL|
         SSD_ALIGN_CENTER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_BLACK);


#ifndef TOUCH_SCREEN
   browser_cont_flags = SSD_ALIGN_VCENTER;
   ssd_dialog_add_vspace(dialog, 2 ,0);
#else
   ssd_dialog_add_vspace(dialog, 5 ,0);
#endif

   browserCont = ssd_container_new("RealtimeExternalPoiDlg.BrowserContainer","", get_browser_width(), get_browser_height() , SSD_ALIGN_CENTER|browser_cont_flags);
   browserCont->context = (void *)systemMessage;
   browserCont->draw = draw_browser_rect;
   ssd_widget_set_color(browserCont, NULL, NULL);
   ssd_widget_add(dialog, browserCont);

#ifdef TOUCH_SCREEN
    ssd_dialog_add_vspace(dialog, 5 ,0);

    button = ssd_button_label("Close_button", roadmap_lang_get("Close"), SSD_ALIGN_CENTER, on_button_close);
    button->context = systemMessage;
    ssd_widget_add(dialog, button);


#else
    ssd_widget_set_left_softkey_callback(dialog, NULL);
      ssd_widget_set_left_softkey_text(dialog, "");
#endif

   ssd_dialog_activate ("SystemMessageWebBased", NULL);
   if (!roadmap_screen_refresh())
      roadmap_screen_redraw();

   ssd_dialog_recalculate( "SystemMessageWebBased" );
   ssd_widget_get_size( dialog, &dlg_size, NULL );
   ssd_widget_get_size( browserCont, &cnt_size, NULL );

}

static void RTSystemMessageShowWebMessage(RTSystemMessage *systemMessage){
   if (systemMessage->iType == URL_SYSTEM_MESSAGE_POPUP)
       RTSystemMessageShowWebMessageDlg(systemMessage);
   else
      roadmap_browser_show(systemMessage->title, systemMessage->msg, RTSystemMessagesDisplay, NULL, NULL, BROWSER_BAR_NORMAL );
}

void RTSystemMessagesDisplay(void){
   if( RTSystemMessageQueue_Size()){
      RTSystemMessage systemMessage;
      RTSystemMessage_Init(&systemMessage);
      RTSystemMessageQueue_Pop(&systemMessage);
      RTSystemMessagesSetLastMessageDisplayed(systemMessage.iId);

      if ((systemMessage.iType == URL_SYSTEM_MESSAGE_POPUP) || (systemMessage.iType == URL_SYSTEM_MESSAGE_FULL_SCREEN)){
            RTSystemMessageShowWebMessage(&systemMessage);
      }
      else{
         RTSystemMessagesDisplay_Dlg(systemMessage.title, systemMessage.titleTextColor, systemMessage.titleTextSize, systemMessage.msg, systemMessage.msgTextColor,systemMessage.msgTextSize, systemMessage.icon);
      }

   }
}

void RTSystemMessagesInit(void){


   roadmap_config_declare ("user",
                           &RoadMapConfigLastIdDisplayed,
                           "0", NULL);

}

//////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////
