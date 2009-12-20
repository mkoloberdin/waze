/* roadmap_help.c - Manage access to some help.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 *   See roadmap_help.h.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_config.h"
#include "roadmap_internet.h"
#include "ssd/ssd_contextmenu.h"
#include "roadmap_factory.h"
#include "roadmap_path.h"
#include "roadmap_file.h"
#include "roadmap_spawn.h"

#include "roadmap_help.h"

#ifdef IPHONE
#include "roadmap_list_menu.h"
#include "roadmap_introduction.h"
#include "roadmap_image_viewer.h"
#endif //IPHONE

#define RDM_URLHEAD "file://"
#define RDM_MANUAL "manual.html"

#ifndef ROADMAP_BROWSER
#define ROADMAP_BROWSER "dillo"
#endif

#define CFG_HELP_GUIDED_TOUR_URL_DEFAULT "http://d2bhe1se45kh4t.cloudfront.net/guided_tour_en.mp4"
#define CFG_HELP_WHAT_TO_EXPECT_URL_DEFAULT "http://d2bhe1se45kh4t.cloudfront.net/guided_tour_en.mp4"

extern ssd_contextmenu_ptr    s_main_menu;
extern const char*            grid_menu_labels[];
extern RoadMapAction          RoadMapStartActions[];
extern BOOL get_menu_item_names(  const char*          menu_name,
                                ssd_contextmenu_ptr  parent,
                                const char*          items[],
                                int*                 count);

static RoadMapConfigDescriptor RoadMapConfigBrowser =
                        ROADMAP_CONFIG_ITEM("Help", "Browser");

static RoadMapConfigDescriptor RoadMapConfigBrowserOptions =
                        ROADMAP_CONFIG_ITEM("Help", "Arguments");

static RoadMapConfigDescriptor RoadMapConfigHelpGuidedTour =
                        ROADMAP_CONFIG_ITEM("Help", "Guided tour url");

static RoadMapConfigDescriptor RoadMapConfigHelpWhatToExpectUrl =
                        ROADMAP_CONFIG_ITEM("Help", "What to expect url");

static RoadMapConfigDescriptor RoadMapConfigHelpShowWhatToExpect =
                        ROADMAP_CONFIG_ITEM("Help", "Show what to expect");

static RoadMapConfigDescriptor RoadMapConfigHelpRollerNutshell =
                        ROADMAP_CONFIG_ITEM("Help", "Roller nutshell");

static char *RoadMapHelpManual = NULL;


/* -- The help display functions. -------------------------------------- */

static void roadmap_help_make_url (const char *path) {

   int size;

   const char *options = roadmap_config_get(&RoadMapConfigBrowserOptions);
   char *url;

   size = strlen(options)
             + strlen(RDM_URLHEAD)
             + strlen(path)
             + strlen(RDM_MANUAL)
             + 8;

   url = malloc (size);

   strcpy(url, RDM_URLHEAD);
   strcat(url, path);
   strcat(url, "/" RDM_MANUAL "#%s");

   if (options[0] != 0) {
      RoadMapHelpManual = malloc(size);
      sprintf (RoadMapHelpManual, options, url);
      free (url);
   } else {
      RoadMapHelpManual = url;
   }
}

static int roadmap_help_prepare (void) {

   const char *path;


   /* First look for the user directory. */
   path = roadmap_path_user();
   if (roadmap_file_exists(path, RDM_MANUAL)) {
      roadmap_help_make_url (path);
      return 1;
   }


   /* Then look throughout the system path list. */

   for (path = roadmap_path_first("config");
         path != NULL;
         path = roadmap_path_next("config", path))
   {
      if (roadmap_file_exists(path, RDM_MANUAL)) {

         roadmap_help_make_url (path);
         return 1;
      }
   }

   roadmap_log(ROADMAP_ERROR, "manual not found");
   return 0;
}

static void roadmap_help_show (const char *index) {

    char *arguments;

    if (RoadMapHelpManual == NULL) {
       if (! roadmap_help_prepare()) {
          return;
       }
    }
    if (index == NULL || index[0] == 0) index = "#id.toc";

    roadmap_log(ROADMAP_DEBUG, "activating help %s", index);

    arguments = malloc (strlen(RoadMapHelpManual) + strlen(index) + 1);
    sprintf (arguments, RoadMapHelpManual, index);

    roadmap_spawn(roadmap_config_get(&RoadMapConfigBrowser), arguments);
    free(arguments);
}

static void roadmap_help_install (void) {roadmap_help_show ("directories");}
static void roadmap_help_options (void) {roadmap_help_show ("options");}
static void roadmap_help_voice   (void) {roadmap_help_show ("voice");}
static void roadmap_help_key     (void) {roadmap_help_show ("key");}
static void roadmap_help_street  (void) {roadmap_help_show ("street");}
static void roadmap_help_trips   (void) {roadmap_help_show ("trips");}


/* -- The help display dictionnary. ------------------------------------ */

typedef struct {
   const char *label;
   RoadMapCallback callback;
} RoadMapHelpList;

static RoadMapHelpList RoadMapHelpTopics[] = {
   {"Installation directories",  roadmap_help_install},
   {"Command line options",      roadmap_help_options},
   {"Voice messages",            roadmap_help_voice},
   {"Keyboard bindings",         roadmap_help_key},
   {"Entering street names",     roadmap_help_street},
   {"Managing trips",            roadmap_help_trips},
   {NULL, NULL}
};
static RoadMapHelpList *RoadMapHelpTopicsCursor = NULL;


/* -- The help initialization functions. ------------------------------- */

static int roadmap_help_get_topic (const char **label,
                                   RoadMapCallback *callback) {

   if (RoadMapHelpTopicsCursor->label == NULL) {
      RoadMapHelpTopicsCursor = NULL;
      return 0;
   }

   *label = RoadMapHelpTopicsCursor->label;
   *callback = RoadMapHelpTopicsCursor->callback;

   return 1;
}

int roadmap_help_first_topic (const char **label,
                              RoadMapCallback *callback) {

   RoadMapHelpTopicsCursor = RoadMapHelpTopics;

   return roadmap_help_get_topic(label, callback);
}

int roadmap_help_next_topic (const char **label,
                             RoadMapCallback *callback) {

   if (RoadMapHelpTopicsCursor == NULL) {
      roadmap_log(ROADMAP_ERROR, "next called before first");
      return 0;
   }
   if (RoadMapHelpTopicsCursor->label == NULL) {
      RoadMapHelpTopicsCursor = NULL;
      return 0;
   }
   
   RoadMapHelpTopicsCursor += 1;
   return roadmap_help_get_topic(label, callback);
}

void roadmap_help_menu(void){
#ifdef IPHONE
   int                  count = 0;
   const char           *help_menu[10];
   
   if (roadmap_introduction_is_available()){
      help_menu[count++] = "nutshell";
      help_menu[count++] = "guided_tour";
      if (!strcmp(roadmap_config_get(&RoadMapConfigHelpShowWhatToExpect), "yes")) {
         help_menu[count++] = "what_to_expect";
      }
      help_menu[count++] = "geoinfo";
   }
   
   help_menu[count++] = "submit_debug_logs";
   help_menu[count++] = "about";
   help_menu[count++] = NULL;
   
	roadmap_list_menu_simple ("Help menu", 
                             NULL, 
                             help_menu,
                             NULL,
                             NULL,
                             RoadMapStartActions,
                             0);
#endif //IPHONE
}

#ifdef IPHONE
void roadmap_help_guided_tour () {
   roadmap_main_play_movie (roadmap_config_get(&RoadMapConfigHelpGuidedTour));
}

void roadmap_help_what_to_expect () {
   roadmap_main_play_movie (roadmap_config_get(&RoadMapConfigHelpWhatToExpectUrl));
}

static const char *nutshell_images[] = {

   "nutshell_01",
   "nutshell_02",
   "nutshell_03",
   "nutshell_04",
   "nutshell_05",
   "nutshell_06",
   "nutshell_07",
   NULL
};

static const char *nutshell_int_images[] = {

   "nutshell_int_01",
   "nutshell_int_02",
   "nutshell_int_03",
   "nutshell_int_04",
   "nutshell_int_05",
   "nutshell_int_06",
   "nutshell_int_07",
   "nutshell_int_08",
   NULL
};

void roadmap_help_nutshell () {
   int count;
   
   if (strcmp(roadmap_config_get(&RoadMapConfigHelpRollerNutshell), "yes")) {
       for (count = 0; nutshell_images[count] != NULL; ++count) {}
       roadmap_image_viewer_show(nutshell_images, count);
   } else {
      for (count = 0; nutshell_int_images[count] != NULL; ++count) {}
      roadmap_image_viewer_show(nutshell_int_images, count);
   }
       
   
}
#endif //IPHONE

void roadmap_open_help(void){
#if defined (_WIN32) || defined (__SYMBIAN32__)
	roadmap_internet_open_browser(roadmap_config_get(&RoadMapConfigBrowserOptions));
#endif
}

void roadmap_help_initialize (void) {

   roadmap_config_declare
      ("preferences", &RoadMapConfigBrowserOptions, "%s", NULL);

   roadmap_config_declare
      ("preferences", &RoadMapConfigBrowser, ROADMAP_BROWSER, NULL);
   
   roadmap_config_declare
      ("preferences", &RoadMapConfigHelpGuidedTour, CFG_HELP_GUIDED_TOUR_URL_DEFAULT, NULL);

   roadmap_config_declare
      ("preferences", &RoadMapConfigHelpWhatToExpectUrl, CFG_HELP_WHAT_TO_EXPECT_URL_DEFAULT, NULL);

   roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigHelpShowWhatToExpect, NULL, "no", "yes", NULL);

   
   roadmap_config_declare_enumeration 
      ("preferences", &RoadMapConfigHelpRollerNutshell, NULL, "no", "yes", NULL);
}

