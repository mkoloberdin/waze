/* roadmap_recorder_dlg.c
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

#include "roadmap.h"
#include "roadmap_main.h"
#include "roadmap_sound.h"
#include "roadmap_lang.h"
#include "roadmap_gui.h"
#include "roadmap_time.h"
#include "roadmap_recorder.h"
#include "roadmap_screen.h"
#include "roadmap_config.h"
#include "roadmap_object.h"
#include "roadmap_types.h"
#include "roadmap_time.h"
#include "roadmap_messagebox.h"
#include "roadmap_res.h"
#include "roadmap_res_download.h"
#include "roadmap_recorder.h"
#include "roadmap_object.h"
#include "roadmap_math.h"

#include "ssd/ssd_dialog.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_button.h"
#include "ssd/ssd_bitmap.h"
#include "ssd/ssd_popup.h"
#include "ssd/ssd_progress.h"

enum states {
   recorder_init,
   recorder_autostart,
   recorder_recording,
   recorder_stopped
};


#define MAX_RECORDER_TIME              (60 * 1000)
#define RECORDER_COUNTDOWN             3 //seconds
#define RECORD_DLG_NAME                "contextmenu_dialog"

static recorder_closed_cb gRecorderCb = NULL;
static int gState = recorder_init;
static SsdWidget  gDialog;

static int gCountdownCounter;
static RoadMapCallback gCountdownTimer;
static RoadMapCallback gRrefreshTimer;
static int   gStartTime;
static int   gTimeout;
static char  *gFileName;
static void  *gContext;

static void setCountdownTimer(void);
static void removeCountdownTimer(void);
static void setNotificationLabel(char *value);
static void onRecord(void);
static void setRefreshTimer(void);
static void removeRefreshTimer(void);
static void setDoneButtonText(const char *value);
static void onDone(void);
static void updateProgress(int percentage);
static void refreshTimerProgress(void);

static void draw_bg (SsdWidget widget, RoadMapGuiRect *rect, int flags){
   static RoadMapPen pen = NULL;
   RoadMapGuiPoint fill_points[4];
   int count;
   if (!pen)
      pen = roadmap_canvas_create_pen ("fill_pop_up_pen");

   roadmap_canvas_set_foreground("#000000");
   roadmap_canvas_set_opacity(170);

   fill_points[0].x =0 ;
   fill_points[0].y =0;
   fill_points[1].x =roadmap_canvas_width();
   fill_points[1].y = 0;
   fill_points[2].x = roadmap_canvas_width();
   fill_points[2].y = roadmap_canvas_height();
   fill_points[3].x = 0;
   fill_points[3].y = roadmap_canvas_height();
   count = 4;

   roadmap_canvas_draw_multiple_polygons (1, &count, fill_points, 1,0 );

}
/////////////////////////////////////////////////////////////////////
static int on_button_record( SsdWidget this, const char *new_value){
   onRecord();
   return 1;
}

/////////////////////////////////////////////////////////////////////
static int on_button_cancel( SsdWidget this, const char *new_value){
   removeRefreshTimer();
   removeCountdownTimer();
   if (gState == recorder_stopped) {
       roadmap_file_remove(NULL, gFileName);
   }

   ssd_dialog_hide(RECORD_DLG_NAME, dec_no);
   return 1;
}

/////////////////////////////////////////////////////////////////////
static int on_done_botton( SsdWidget this, const char *new_value){
   onDone();
   return 1;
}


/////////////////////////////////////////////////////////////////////
static void recorder_closed (int exit_code) {
   gDialog = NULL;

   if (gRecorderCb) {
      gRecorderCb (exit_code, gContext);
      gRecorderCb = NULL;
   }
}


/////////////////////////////////////////////////////////////////////
void recorder_dlg(const char *textMsg){
   SsdWidget progress;
   SsdWidget text;
   SsdWidget bitmap;
   SsdWidget button;
   SsdWidget bg_container;
   SsdWidget button_bitmap;
   const char *small_button_icon[]   = {"button_small_up", "button_small_down", "button_small_up"};
   const char *button_icon[]   = {"button_up", "button_down", "button_disabled"};


   if (gDialog) {
      return;
   }

   // Create the dialog
   gDialog = ssd_dialog_new (RECORD_DLG_NAME, "", recorder_closed,
         SSD_PERSISTENT|SSD_CONTAINER_BORDER|SSD_DIALOG_FLOAT|
         SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER|SSD_ROUNDED_CORNERS|SSD_ROUNDED_BLACK|SSD_POINTER_NONE|SSD_SHADOW_BG);


   text = ssd_text_new("notificationLabel", "", 16, SSD_ALIGN_CENTER);
   ssd_text_set_color(text, "#ffffff");
   ssd_widget_add(gDialog, text);
   ssd_dialog_add_vspace(gDialog, 2, 0);

   if ( (textMsg) && (strlen(textMsg) > 0) ){
      text = ssd_text_new("recordingText", textMsg, 13, SSD_ALIGN_CENTER|SSD_TEXT_NORMAL_FONT);
      ssd_text_set_color(text, "#ffffff");
      ssd_widget_add(gDialog, text);
      ssd_dialog_add_vspace(gDialog, 5, 0);
   }
   // Progess image
   bitmap = ssd_bitmap_new("Record_Dlg.Timer.Bg", "recorder_timer_bg", SSD_ALIGN_CENTER);
   ssd_widget_add(gDialog, bitmap);
   ssd_dialog_add_vspace(bitmap, 5, 0);
   text = ssd_text_new("Record_Dlg.Timer.Txt", "00:00", 20, SSD_ALIGN_CENTER|SSD_END_ROW);
   ssd_widget_add(bitmap, text);
   ssd_dialog_add_vspace(bitmap, 5, 0);
   progress = ssd_progress_new("Record_Dlg.Timer.Progress",1, FALSE, SSD_ALIGN_CENTER);
   ssd_widget_add(bitmap, progress);

   //Buttons
   ssd_dialog_add_vspace(gDialog, 5, 0);

   //Record button
   button = ssd_button_new ("Record_button", "", button_icon, 3, 0, on_button_record);
   button_bitmap = ssd_bitmap_new("Record_button.bitmap", "recorder_record", SSD_ALIGN_CENTER);
   ssd_widget_add(button, button_bitmap);

    #if defined (_WIN32) && !defined (OPENGL)
       text = ssd_text_new ("Record_button.label", roadmap_lang_get("Record"), 9, SSD_ALIGN_VCENTER| SSD_ALIGN_CENTER) ;
    #else
       text = ssd_text_new ("Record_button.label", roadmap_lang_get("Record"), 11, SSD_ALIGN_VCENTER|SSD_ALIGN_CENTER) ;
    #endif

    ssd_widget_set_color(text, "#ffffff", "#ffffff");
    ssd_widget_add (button,text);
    ssd_widget_hide(button);
    ssd_widget_add(gDialog, button);


   //Cancel button
   button = ssd_button_label_custom("Cancel_button", roadmap_lang_get("Cancel"), SSD_ALIGN_CENTER, on_button_cancel, small_button_icon, 3,"#ffffff", "#ffffff",11);
   ssd_widget_add(gDialog, button);


   //Done button
   button = ssd_button_label_custom("Done_button", roadmap_lang_get("Use"), SSD_ALIGN_CENTER, on_done_botton, small_button_icon, 3,"#ffffff", "#ffffff",11);
   ssd_button_disable(button);
   ssd_widget_add(gDialog, button);
   ssd_widget_hide(button);

   // Activate the dialog
   ssd_dialog_activate(RECORD_DLG_NAME, 0);
}

/////////////////////////////////////////////////////////////////////
static void setRecordingButtonImages(char *name){
   SsdWidget bmp;

    if (!gDialog)
       return;

    bmp = ssd_widget_get(gDialog, "Record_button.bitmap");
    if (!bmp)
       return;

    ssd_bitmap_update(bmp, name);

}

/////////////////////////////////////////////////////////////////////
static void setRecordingButtonText(const char *value){
   SsdWidget txt;

    if (!gDialog)
       return;

    txt = ssd_widget_get(gDialog, "Record_button.label");
    if (!txt)
       return;

    ssd_text_set_text(txt, value);

}

/////////////////////////////////////////////////////////////////////
static void enableDoneButton(void){
   SsdWidget button;

   if (!gDialog)
     return;

   button = ssd_widget_get(gDialog, "Done_button");
   if (!button)
      return;

   ssd_button_enable(button);
}

/////////////////////////////////////////////////////////////////////
static void onRecord(void){
   RoadMapSoundList     list  = NULL;
   char text[128];

   list = roadmap_sound_list_create (SOUND_LIST_NO_FREE);

   enableDoneButton();

   if (gState == recorder_recording) {
     roadmap_sound_list_add (list, "rec_end");
     roadmap_sound_play_list (list);
     gState = recorder_stopped;
     setCountdownTimer();
     refreshTimerProgress();
     removeRefreshTimer();
     setRecordingButtonImages("recorder_record");
     setRecordingButtonText(roadmap_lang_get("Record"));
     roadmap_sound_stop_recording();
     updateProgress(100);
   }
   else {
    removeCountdownTimer();
    roadmap_sound_list_add (list, "rec_start");
    roadmap_sound_play_list (list);
    updateProgress(0);
    if (gState == recorder_stopped) {
         roadmap_file_remove(NULL, gFileName);
    }

    setRecordingButtonImages("recorder_stop");
    setRecordingButtonText(roadmap_lang_get("Stop"));
    setDoneButtonText(roadmap_lang_get("Use"));
    roadmap_sound_record(gFileName, gTimeout/1000);
    gState = recorder_recording;
    gStartTime = roadmap_time_get_millis();
    setRefreshTimer();

  }

  gStartTime = roadmap_time_get_millis();

   switch (gState) {
        case recorder_init:
           //snprintf(text, sizeof(text), "%s",roadmap_lang_get("Tap \"Record\" when ready"));
           snprintf(text, sizeof(text), "%s","");
           break;
        case recorder_recording:
           //snprintf(text, sizeof(text), "%s",roadmap_lang_get("Tap \"Stop\" when finished"));
           snprintf(text, sizeof(text), "%s",roadmap_lang_get("Recording..."));
           break;
        case recorder_stopped:
           snprintf(text, sizeof(text), "%s",roadmap_lang_get("Recording complete"));
           break;
        default:
           break;
   }

   setNotificationLabel(text);
}

/////////////////////////////////////////////////////////////////////
static void onDone(void){
   if (gState == recorder_recording)
      onRecord();

   //kill timer if exists
   removeRefreshTimer();
   removeCountdownTimer();
   ssd_dialog_hide(RECORD_DLG_NAME, dec_yes);

}

/////////////////////////////////////////////////////////////////////
static void setNotificationLabel(char *value){
   SsdWidget text;
   if (!gDialog)
      return;

   text = ssd_widget_get(gDialog, "notificationLabel");
   if (!text)
      return;

   ssd_text_set_text(text, value);
}

/////////////////////////////////////////////////////////////////////
static void setProgressLabel(char *value){
   SsdWidget text;
   if (!gDialog)
      return;

   text = ssd_widget_get(gDialog, "Record_Dlg.Timer.Txt");
   if (!text)
      return;

   ssd_text_set_text(text, value);
}

/////////////////////////////////////////////////////////////////////
static void setDoneButtonText(const char *value){
   SsdWidget button;
   if (!gDialog)
      return;

   button = ssd_widget_get(gDialog, "Done_button");
   if (!button)
      return;

   ssd_button_change_text(button, (const char*)value);
}

static void updateProgress(int percentage){
   SsdWidget progerss;
   if (!gDialog)
      return;

   progerss = ssd_widget_get(gDialog, "Record_Dlg.Timer.Progress");
   if (!progerss)
      return;

   ssd_progress_set_value(progerss, percentage);
}

/////////////////////////////////////////////////////////////////////
static void refreshTimerProgress(void){
   int elapsed_time, minutes, seconds;
   char text[128];

   if (gState == recorder_stopped) {
      snprintf(text, sizeof(text), "%s (%d)",
               roadmap_lang_get("Use"),
               gCountdownCounter);
      setDoneButtonText(text);
      return;
   }

   if (gState == recorder_autostart) {
      snprintf(text, sizeof(text), "%s %d %s...",
               roadmap_lang_get("Recording in"),
               gCountdownCounter,
               roadmap_lang_get("seconds"));
      setNotificationLabel(text);
   }

   if (gState == recorder_init ||
       gState == recorder_autostart) {
      elapsed_time = minutes = seconds = 0;
   } else {
      elapsed_time = roadmap_time_get_millis() - gStartTime;
      minutes = elapsed_time / (60 * 1000);
      seconds = elapsed_time / 1000 - minutes * 60;
   }

   snprintf(text, sizeof(text), "%02d:%02d",minutes, seconds);
   updateProgress(elapsed_time*100/gTimeout);
   setProgressLabel(text);
   roadmap_screen_refresh();
}
/////////////////////////////////////////////////////////////////////
static void onCountdownTimeout(void){

   if (--gCountdownCounter  ==  0) {
      removeCountdownTimer();
      if (gState == recorder_autostart) {
         onRecord();
      } else if (gState == recorder_stopped) {
         onDone();
      }
   } else {
      refreshTimerProgress();
   }

}


/////////////////////////////////////////////////////////////////////
static void setCountdownTimer(void){
   if (gCountdownTimer) return;

   if (gState == recorder_autostart)
      gCountdownCounter = RECORDER_COUNTDOWN;
   else
      gCountdownCounter = 1;

   gCountdownTimer = onCountdownTimeout;

   roadmap_main_set_periodic(1000, gCountdownTimer);
}

/////////////////////////////////////////////////////////////////////
static void removeCountdownTimer(void){

   if (gCountdownTimer) {
      roadmap_main_remove_periodic(gCountdownTimer);
      gCountdownTimer = NULL;
   }
}

/////////////////////////////////////////////////////////////////////
static void onTimeout(void){
   if (gState == recorder_recording)
      onRecord();
   updateProgress(100);
}


/////////////////////////////////////////////////////////////////////
static void onRefreshTimeout(void){

   if (roadmap_time_get_millis() - gStartTime > gTimeout)
      onTimeout();
   else {
      refreshTimerProgress();
   }

}

/////////////////////////////////////////////////////////////////////
static void setRefreshTimer(void){

   if (gRrefreshTimer) return;

   gRrefreshTimer = onRefreshTimeout;

   roadmap_main_set_periodic(1000, gRrefreshTimer);

}

/////////////////////////////////////////////////////////////////////
static void removeRefreshTimer(void){

   if (gRrefreshTimer) {
      roadmap_main_remove_periodic(gRrefreshTimer);
      gRrefreshTimer = NULL;
   }
}

/////////////////////////////////////////////////////////////////////
void roadmap_recorder (const char *text, int seconds, recorder_closed_cb on_recorder_closed, int auto_start, const char* path, const char* file_name, void *context) {
   if (gDialog) {
      return;
   }

   gRecorderCb = on_recorder_closed;
   gTimeout = seconds*1000;
   gFileName = roadmap_path_join(path, file_name);

   gStartTime = 0;
   gState = recorder_init;
   gRrefreshTimer = NULL;
   gContext = context;

   if (gTimeout <= 0 || gTimeout > MAX_RECORDER_TIME)
      gTimeout = MAX_RECORDER_TIME;
   refreshTimerProgress();

   recorder_dlg(roadmap_lang_get(text));

   if (auto_start) {
      gState = recorder_autostart;
      setCountdownTimer();
      refreshTimerProgress();
   }

}
