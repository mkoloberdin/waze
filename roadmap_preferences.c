/* roadmap_preferences.c - handle the roadmap dialogs managing user preferences.
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
 *   void roadmap_preferences_edit (void);
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"
#include "roadmap_config.h"
#include "roadmap_dialog.h"
#include "roadmap_preferences.h"


typedef struct EditableItemRecord EditableItem;

struct EditableItemRecord {

   EditableItem *next;
   RoadMapConfigDescriptor config;
};


typedef struct category_list CategoryList;

struct category_list {

   CategoryList *next;
   EditableItem *children;

   const char *name;
};


typedef struct configuration_context ConfigurationContext;

struct configuration_context {

   ConfigurationContext *next;
   CategoryList         *children;

   const char *name;
};

static ConfigurationContext *RoadMapConfigurationDialogs = NULL;


static void roadmap_preferences_cancel (const char *name, void *data) {

   roadmap_dialog_hide (name);
}

static void roadmap_preferences_ok (const char *name, void *data) {

   ConfigurationContext *context = (ConfigurationContext *) data;
   CategoryList *list;
   EditableItem *item;

   if (context == NULL) return;

   for (list = context->children; list != NULL; list = list->next) {

      for (item = list->children; item != NULL; item = item->next) {

         roadmap_config_set
              (&item->config,
               (char *) roadmap_dialog_get_data
                              (item->config.category, item->config.name));
      }
   }

   roadmap_preferences_cancel (name, data);
}

static void roadmap_preferences_force (const char *name, void *data) {

   roadmap_preferences_ok (name, data);
   roadmap_config_save (1);
}


static void roadmap_preferences_new_item
                        (ConfigurationContext *context,
                         RoadMapConfigDescriptor *cursor) {

   CategoryList *list;
   EditableItem *item;

   for (list = context->children; list != NULL; list = list->next) {

      if (strcmp (cursor->category, list->name) == 0) {

         for (item = list->children; item != NULL; item = item->next) {

            if (strcmp (cursor->name, item->config.name) == 0) {
               return;
            }
         }
      }
   }

   if (list == NULL) {

      list = malloc (sizeof(CategoryList));
      roadmap_check_allocated(list);

      list->name     = cursor->category;
      list->children = NULL;

      list->next = context->children;
      context->children = list;
   }

   item = malloc (sizeof(EditableItem));
   roadmap_check_allocated(item);

   item->config = *cursor;

   item->next = list->children;
   list->children = item;
}


static void roadmap_preferences_new_dialog
               (ConfigurationContext *context,
                RoadMapConfigDescriptor *cursor) {

   void *enumeration;

   const char *value;

   int   count;
   const char *values[256];


   while (cursor->reference != NULL) {

      value = roadmap_config_get (cursor);
      roadmap_preferences_new_item (context, cursor);

      switch (roadmap_config_get_type (cursor)) {

      case ROADMAP_CONFIG_ENUM:

         count = 0;
         values[0] = (char *)value; /* Always make the original value appear first. */

         for (enumeration = roadmap_config_get_enumeration (cursor);
              enumeration != NULL;
              enumeration = roadmap_config_get_enumeration_next (enumeration)) {

            values[count] = roadmap_config_get_enumeration_value (enumeration);

            //if (strcmp (values[count], value) != 0) {

               if (count >= 256) {
                  roadmap_log (ROADMAP_FATAL,
                               "too many values for item %s.%s",
                               cursor->category, cursor->name);
               }
               count += 1;
            //}
         }
         roadmap_dialog_new_choice
            (cursor->category, cursor->name, count, values, (void **)values, NULL);
         break;

      case ROADMAP_CONFIG_COLOR:
      case ROADMAP_CONFIG_STRING:

         roadmap_dialog_new_entry (cursor->category, cursor->name, NULL);
         break;

      case ROADMAP_CONFIG_PASSWORD:

         roadmap_dialog_new_password (cursor->category, cursor->name);
         break;

      default:
         roadmap_log (ROADMAP_FATAL,
                      "invalid preference item type %d",
                      roadmap_config_get_type (cursor));
      }

      roadmap_config_next (cursor);
   }

   roadmap_dialog_add_button ("Ok", roadmap_preferences_ok);
   roadmap_dialog_add_button ("Force", roadmap_preferences_force);
   roadmap_dialog_add_button ("Cancel", roadmap_preferences_cancel);


   roadmap_dialog_complete (roadmap_preferences_use_keyboard ());
}


static void roadmap_preferences_reset_dialog
                (ConfigurationContext *context) {

   CategoryList *list;
   EditableItem *item;

   if (context == NULL) return;

   for (list = context->children; list != NULL; list = list->next) {

      for (item = list->children; item != NULL; item = item->next) {

         roadmap_dialog_set_data (item->config.category,
                                  item->config.name,
                                  (char *)roadmap_config_get (&item->config));
      }
   }
}


static void roadmap_preferences_show (const char *file, const char *title) {

    ConfigurationContext *context;

    RoadMapConfigDescriptor cursor;


    if (! roadmap_config_first (file, &cursor)) {
        roadmap_log (ROADMAP_ERROR, "no item found for %s", title);
        return;
    }

    for (context = RoadMapConfigurationDialogs;
         context != NULL;
         context = context->next) {

        if (strcmp (context->name, title) == 0) break;
    }

    if (context == NULL) {

        context = (ConfigurationContext *)
                        malloc (sizeof(ConfigurationContext));
        roadmap_check_allocated(context);

        context->name = strdup(title);
        context->children = NULL;

        context->next = RoadMapConfigurationDialogs;
        RoadMapConfigurationDialogs = context;
    }

    if (roadmap_dialog_activate (title, context, 1)) {

        roadmap_preferences_new_dialog (context, &cursor);
    }
    roadmap_preferences_reset_dialog (context);
}


int roadmap_preferences_use_keyboard (void) {

    static RoadMapConfigDescriptor RoadMapConfigGeneralKeyboard =
                            ROADMAP_CONFIG_ITEM("General", "Keyboard");

    return roadmap_config_match (&RoadMapConfigGeneralKeyboard, "yes");
}


void roadmap_preferences_edit (void) {
    
    roadmap_preferences_show ("preferences", "Preferences");
}

