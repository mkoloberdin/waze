/* navigate_traffic.c - manage traffic statistics
 *
 * LICENSE:
 *
 *   Copyright 2006 Ehud Shabtai
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
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
 *   See navigate_traffic.h
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_line.h"
#include "roadmap_line_route.h"
#include "roadmap_line_speed.h"
#include "roadmap_lang.h"
#include "roadmap_layer.h"
#include "roadmap_locator.h"
#include "roadmap_config.h"
#include "roadmap_start.h"
#include "roadmap_skin.h"
#include "roadmap_main.h"

#include "navigate_traffic.h"

#define MAX_LAYERS (ROADMAP_ROAD_LAST + 1)
#define MAX_PEN_LAYERS 2
#define MAX_ROAD_COLORS 2

static RoadMapConfigDescriptor TrafficConfigJamColor =
                    ROADMAP_CONFIG_ITEM("Traffic", "JamRoadColor");

static RoadMapConfigDescriptor TrafficUseTrafficCfg =
                  ROADMAP_CONFIG_ITEM("Traffic", "Use for calculating routes");
static RoadMapConfigDescriptor TrafficDisplayEnabledCfg =
                  ROADMAP_CONFIG_ITEM("Traffic", "Display traffic information");

static int TrafficDisplayEnabled = 0;

static RoadMapPen TrafficPens[MAX_LAYERS][MAX_PEN_LAYERS];

#define STATE_UNKNOWN    0x00
#define STATE_NORMAL     0x01
#define STATE_CONGESTION 0x02

#define CACHE_ENTRY 2
typedef struct navigate_cache {
   int fips;
   int *bits;
   int size; /* amount of entries (2 bits per entry) */
} NavigateCache;

static NavigateCache Cache;
static int CacheMask;

#ifdef SSD

#include "ssd/ssd_dialog.h"
#include "ssd/ssd_container.h"
#include "ssd/ssd_text.h"
#include "ssd/ssd_choice.h"
#include "ssd/ssd_button.h"

static const char *yesno_label[2];
static const char *yesno[2];

static int button_callback (SsdWidget widget, const char *new_value) {

   if (!strcmp(widget->name, "OK")) {
      roadmap_config_set (&TrafficUseTrafficCfg,
                           (const char *)ssd_dialog_get_data ("navigate"));
      roadmap_config_set (&TrafficDisplayEnabledCfg,
                           (const char *)ssd_dialog_get_data ("display"));

      navigate_traffic_display
         (roadmap_config_match(&TrafficDisplayEnabledCfg, "yes"));

      ssd_dialog_hide_current (dec_ok);

      return 1;

   }

   ssd_dialog_hide_current (dec_close);
   return 1;
}

static void create_ssd_dialog (void) {

   SsdWidget box;
   SsdWidget dialog = ssd_dialog_new ("traffic_prefs",
                      roadmap_lang_get ("Traffic preferences"),
                      NULL,
                      SSD_CONTAINER_BORDER|SSD_CONTAINER_TITLE|
                      SSD_DIALOG_FLOAT|SSD_ALIGN_CENTER|SSD_ALIGN_VCENTER);

   if (!yesno[0]) {
      yesno_label[0] = roadmap_lang_get ("Yes");
      yesno_label[1] = roadmap_lang_get ("No");
      yesno[0] = "yes";
      yesno[1] = "no";
   }

   ssd_widget_set_color (dialog, "#000000", "#ffffffee");

   box = ssd_container_new ("navigate group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
                            SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_set_color (box, "#000000", NULL);

   ssd_widget_add (box,
      ssd_text_new ("navigate_label",
                     roadmap_lang_get ("Use for calculating routes"),
                    -1, SSD_TEXT_LABEL|SSD_END_ROW));

    ssd_widget_add (box,
         ssd_choice_new ("navigate",roadmap_lang_get ("Use for calculating routes"),2,
                         (const char **)yesno_label,
                         (const void **)yesno,
                         SSD_END_ROW, NULL));
        
   ssd_widget_add (dialog, box);

   box = ssd_container_new ("display group", NULL, SSD_MIN_SIZE, SSD_MIN_SIZE,
                            SSD_WIDGET_SPACE|SSD_END_ROW);
   ssd_widget_set_color (box, "#000000", NULL);

   ssd_widget_add (box,
      ssd_text_new ("display_label",
                    roadmap_lang_get ("Display traffic information"),
                    -1, SSD_TEXT_LABEL|SSD_END_ROW));

    ssd_widget_add (box,
         ssd_choice_new ("display", roadmap_lang_get ("Display traffic information"), 2,
                         (const char **)yesno_label,
                         (const void **)yesno,
                         SSD_END_ROW, NULL));
         
   ssd_widget_add (dialog, box);

   ssd_widget_add (dialog,
      ssd_button_label ("OK", roadmap_lang_get ("Ok"),
                        SSD_ALIGN_CENTER|SSD_START_NEW_ROW, button_callback));
}

void traffic_preferences (void) {

   const char *value;

   if (!ssd_dialog_activate ("traffic_prefs", NULL)) {
      create_ssd_dialog ();
      ssd_dialog_activate ("traffic_prefs", NULL);
   }

   if (roadmap_config_match(&TrafficUseTrafficCfg, "yes")) value = yesno[0];
   else value = yesno[1];

   ssd_dialog_set_data ("navigate", (void *) value);

   if (roadmap_config_match(&TrafficDisplayEnabledCfg, "yes")) value = yesno[0];
   else value=yesno[1];
   ssd_dialog_set_data ("display", (void *) value);
}

#else
   void traffic_preferences (void) {}
#endif

static void create_pens (void) {

   int i;
   int j;
   char name[80];

   /* FIXME should only create pens for road class */

   for (i=1; i<MAX_LAYERS; ++i) 
      for (j=0; j<MAX_PEN_LAYERS; j++) {

         RoadMapPen *pen = &TrafficPens[i][j];

         snprintf (name, sizeof(name), "TrafficPen%d", i*100+j*10);
         *pen = roadmap_canvas_create_pen (name);

         if (!j) {
            roadmap_canvas_set_foreground ("#000000");
         } else {
            roadmap_canvas_set_foreground
               (roadmap_config_get (&TrafficConfigJamColor));
         }
      }
}


static void verify_cache (int fips) {
   int count;

   if (Cache.fips == fips) return;

   if (Cache.fips != -1) navigate_traffic_refresh ();

   count = roadmap_line_count();

   if (Cache.size >= count) return;

   if (Cache.size) {
      free (Cache.bits);
   }

   Cache.fips = fips;
   Cache.size = count;
   Cache.bits = calloc ((Cache.size / (8 / CACHE_ENTRY * sizeof(int))) + 1,
                sizeof(int));

   roadmap_check_allocated(Cache.bits);
}


static int cache_get (int fips, int line) {
   int res;
   int shift;

   verify_cache (fips);

   res = Cache.bits[line / (8 / CACHE_ENTRY * sizeof(int))];
   shift = (line % (8 * sizeof(int) / CACHE_ENTRY)) * CACHE_ENTRY;

   res = res >> shift;

   return res & CacheMask;
}


static void cache_set (int fips, int line, int state) {
   int *res;
   int shift;

   res = &Cache.bits[line / (8 / CACHE_ENTRY * sizeof(int))];
   shift = (line % (8 * sizeof(int) / CACHE_ENTRY)) * CACHE_ENTRY;

   state = state << shift;
   *res = *res | state;
}


/* TODO: this is a bad callback which is called from roadmap_layer_adjust().
 * This should be changed. Currently when the editor is enabled, an explicit
 * call to roadmap_layer_adjust() is called. When this is fixed, that call
 * should be removed.
 */
void navigate_traffic_adjust_layer (int layer, int thickness, int pen_count) {
    
   int i;

   if (layer > ROADMAP_ROAD_LAST) return;

   if (thickness < 1) thickness = 1;
   if ((pen_count > 1) && (thickness < 3)) {
      pen_count = 1;
   }

   for (i=0; i<MAX_PEN_LAYERS; i++) {

      RoadMapPen *pen = &TrafficPens[layer][i];

      roadmap_canvas_select_pen (*pen);

      if (i == 1) {
         if (thickness < 3) thickness = 3;
         
         roadmap_canvas_set_thickness (thickness - 2);
      } else {
         roadmap_canvas_set_thickness (thickness);
      }

   }
}


void navigate_traffic_initialize (void) {

   int i;

   for (i=0; i<CACHE_ENTRY; i++) {
      CacheMask = (CacheMask << 1) | 1;
   }

   roadmap_config_declare
       ("schema", &TrafficConfigJamColor, "#8b0000", NULL);
   roadmap_config_declare_enumeration
       ("preferences", &TrafficDisplayEnabledCfg, NULL, "no", "yes", NULL);
   roadmap_config_declare_enumeration
       ("preferences", &TrafficUseTrafficCfg, NULL, "yes", "no", NULL);

   create_pens ();
   roadmap_skin_register (create_pens);

   navigate_traffic_display
      (roadmap_config_match(&TrafficDisplayEnabledCfg, "yes"));

   /*
   roadmap_start_add_action ("traffic", "Traffic data", NULL, NULL,
      "Chage Traffic data settings",
      traffic_preferences);
   */
}


void navigate_traffic_display (int status) {

   if (status && TrafficDisplayEnabled) {
      return;
   } else if (!status && !TrafficDisplayEnabled) {
      return;
   }

   TrafficDisplayEnabled = status;

   if (TrafficDisplayEnabled) {
      roadmap_layer_adjust ();
   }
}


int navigate_traffic_override_pen (int line,
                                   int cfcc,
                                   int fips,
                                   int pen_type,
                                   RoadMapPen *override_pen) {

   if (!TrafficDisplayEnabled) return 0;

   if (pen_type > 1) return 0;

   if (cfcc > ROADMAP_ROAD_MAIN) return 0;

   if (roadmap_locator_activate (fips) >= 0) {
      int avg1;
      int avg2;
      int speed1;
      int speed2;
      int cache_res;

      cache_res = cache_get (fips, line);

      if (cache_res == STATE_NORMAL) return 0;

      if (cache_res == STATE_CONGESTION) {
         *override_pen = TrafficPens[cfcc][pen_type];
         return 1;
      }

      speed1 = roadmap_line_speed_get_speed (line, 0);

      if (speed1) {
         avg1 = roadmap_line_speed_get_avg_speed (line, 0);
         if (avg1 < 15) {
            cache_set (fips, line, STATE_NORMAL);
            return 0;
         }

         if (speed1 < (avg1 * 2 / 3)) {
            *override_pen = TrafficPens[cfcc][pen_type];
            cache_set (fips, line, STATE_CONGESTION);
            return 1;
         }
      }

      speed2 = roadmap_line_speed_get_speed (line, 1);

      if (speed2) {
         avg2   = roadmap_line_speed_get_avg_speed (line, 1);
         if (avg2 < 15) {
            cache_set (fips, line, STATE_NORMAL);
            return 0;
         }

         if (speed2 < (avg2 * 2 / 3)) {
            *override_pen = TrafficPens[cfcc][pen_type];
            cache_set (fips, line, STATE_CONGESTION);
            return 1;
         }
      }
   }

   cache_set (fips, line, STATE_NORMAL);
   return 0;
}


void navigate_traffic_refresh (void) {
   Cache.fips = -1;
   if (Cache.size) {
      memset(Cache.bits, 0,
               Cache.size / (8/CACHE_ENTRY*sizeof(int)) * sizeof(int));
   }
}


