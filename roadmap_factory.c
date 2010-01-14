/* roadmap_factory.c - The menu/toolbar/shortcut factory for RoadMap.
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
 *   See roadmap_factory.h
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_config.h"
#include "roadmap_main.h"
#include "roadmap_preferences.h"
#include "roadmap_help.h"
#include "roadmap_path.h"
#include "roadmap_lang.h"

#include "roadmap_factory.h"


static RoadMapConfigDescriptor RoadMapConfigGeneralToolbar =
                        ROADMAP_CONFIG_ITEM("General", "Toolbar");

static RoadMapConfigDescriptor RoadMapConfigGeneralIcons =
                        ROADMAP_CONFIG_ITEM("General", "Icons");


const char RoadMapFactorySeparator[] = "--separator--";
const char RoadMapFactoryHelpTopics[] = "--help-topics--";


static struct RoadMapFactoryKeyMap *RoadMapFactoryBindings = NULL;

static int RoadMapFactoryKeyLength = 0;


static void roadmap_factory_keyboard (char *key) {

   const struct RoadMapFactoryKeyMap *binding;

   if (RoadMapFactoryBindings == NULL) return;

   for (binding = RoadMapFactoryBindings; binding->key != NULL; ++binding) {

      if (strcasecmp (binding->key, key) == 0) {
         if (binding->action != NULL) {
            RoadMapCallback callback = binding->action->callback;
            if (callback != NULL) {
               (*callback) ();
               break;
            }
         }
      }
   }
}

static void roadmap_factory_add_help (RoadMapMenu menu) {

   int ok;
   const char *label;
   RoadMapCallback callback;

   for (ok = roadmap_help_first_topic(&label, &callback);
        ok;
        ok = roadmap_help_next_topic(&label, &callback)) {

      roadmap_main_add_menu_item (menu,
                                  roadmap_lang_get (label),
                                  roadmap_lang_get (label),
                                  callback);
   }
}


static const RoadMapAction *roadmap_factory_find_action
                              (const RoadMapAction *actions, const char *item) {

   while (actions->name != NULL) {
      if (strcmp (actions->name, item) == 0) return actions;
      ++actions;
   }

   return NULL;
}


static const char *roadmap_factory_terse (const RoadMapAction *action) {

   if (action->label_terse != NULL) {
      return action->label_terse;
   }
   if (action->label_short != NULL) {
      return action->label_short;
   }
   return action->label_long;
}


static const char **roadmap_factory_load_config (const char *file_name,
                                                 const RoadMapAction *actions,
                                                 const char *path) {

   static const char *loaded[256];

   int   count = 0;

   char *p;
   char buffer[256];
   FILE *file = roadmap_file_fopen (path, file_name, "sr");

   if (file == NULL) return NULL;

   while (! feof(file)) {

      fgets (buffer, sizeof(buffer), file);

      if (feof(file) || ferror(file)) break;

      buffer[sizeof(buffer)-1] = 0;

      /* remove the end-of-line character. */
      p = strchr (buffer, '\n');
      if (p != NULL) *p = 0;

      /* Remove any leading space. */
      for (p = buffer; isspace(*p); ++p) ;

      if ((*p == 0) || (*p == '#')) continue; /* Empty line. */

      if (*p == '|' || *p == '-') {
         loaded[count] = RoadMapFactorySeparator;
         count++;
      } else {
         const RoadMapAction *this_action;

         this_action = roadmap_factory_find_action (actions, p);
         if (this_action == NULL) {
            roadmap_log (ROADMAP_ERROR, "invalid action name '%s'", p);
         } else {
            loaded[count++] = this_action->name;
         }
      }
   }
   fclose(file);

   if (count <= 0) return NULL;

   loaded[count] = NULL;
   return loaded;
}

static BOOL roadmap_factory_load_menu_item(
                                 ssd_contextmenu_ptr  menu,
                                 const char*          item_name,
                                 const RoadMapAction* actions)
                                 
{
   const RoadMapAction* action;
   ssd_cm_item_ptr      new_item = menu->item + menu->item_count;
   const char*          p;
   int                  i;
   
   memset( new_item, 0, sizeof(ssd_cm_item));
   
   p = strchr( item_name, '.');
   if( p)
   {
      ssd_contextmenu_ptr  popup_menu = NULL;
      char                 popup_name[CONTEXT_MENU_LABEL_MAX_SIZE+1];
      int                  size = (p-item_name);
      
      if( CONTEXT_MENU_LABEL_MAX_SIZE < size)
      {
         assert(0);
         return FALSE;
      }
      
      strncpy( popup_name, item_name, size);
      popup_name[size] = '\0';
      
      action = roadmap_factory_find_action( actions, popup_name);
      if( !action)
      {
         roadmap_log (ROADMAP_ERROR, "roadmap_factory_load_menu_item() - invalid POPUP name '%s'", popup_name);
         return FALSE;
      }
      
      new_item->flags = CONTEXT_MENU_FLAG_POPUP;
      new_item->label = action->label_long;
      new_item->action= (void*)action;
      new_item->icon = strdup(popup_name);
      
      // Try to find existing popup menu:
      for( i=0; i<menu->item_count; i++)
      {
         ssd_cm_item_ptr existing_item = menu->item + i;
         
         if((CONTEXT_MENU_FLAG_POPUP & existing_item->flags) &&  
            !strcmp( existing_item->label, new_item->label))
         {
            popup_menu = existing_item->popup;
            break;
         }
      }
      
      if( !popup_menu)
      {
         if( CONTEXT_MENU_MAX_ITEMS_COUNT == menu->item_count)
         {
            assert(0);
            return FALSE;
         }
         
         menu->item_count++;
         new_item->popup = malloc(sizeof(ssd_contextmenu));
         memset( new_item->popup, 0, sizeof(ssd_contextmenu));
         
         new_item->popup->item = calloc( sizeof(ssd_cm_item), CONTEXT_MENU_MAX_ITEMS_COUNT);
         
         popup_menu = new_item->popup;
      }

      return roadmap_factory_load_menu_item( popup_menu, p+1, actions);
   }
   
   if( CONTEXT_MENU_MAX_ITEMS_COUNT == menu->item_count)
   {
      assert(0);
      return FALSE;
   }
   
   if( ((*item_name) == '|') || ((*item_name) == '-'))
   {
      new_item->flags = CONTEXT_MENU_FLAG_SEPERATOR;
      
      menu->item_count++;
   }
   else
   {
      action = roadmap_factory_find_action (actions, item_name);
      if( !action)
      {
         roadmap_log (ROADMAP_ERROR, "invalid action name '%s'", item_name);
         return FALSE;
      }
      
      new_item->icon = strdup(item_name) ;
      new_item->flags = CONTEXT_MENU_FLAG_NORMAL;
      new_item->label = action->label_long;
      new_item->action= (void*)action;
      
      menu->item_count++;
   }

   return TRUE;
}

static ssd_contextmenu_ptr roadmap_factory_load_menu_file(
                                 const char*          file_name,
                                 const RoadMapAction* actions,
                                 const char*          path)
{
   char*                p;
   char                 buffer[256];
   ssd_contextmenu_ptr  menu;
   FILE*                file = roadmap_file_fopen (path, file_name, "sr");

   if( !file)
   {
      assert(0);
      return NULL;
   }
   
   menu = malloc( sizeof(ssd_contextmenu));
   memset( menu, 0, sizeof(ssd_contextmenu));
   menu->item = calloc( sizeof(ssd_cm_item), CONTEXT_MENU_MAX_ITEMS_COUNT);

   buffer[sizeof(buffer)-1] = 0;

   while (! feof(file)) {

      fgets( buffer, sizeof(buffer)-1, file);

      if (feof(file) || ferror(file)) break;

      // remove the end-of-line character. 
      p = strchr (buffer, '\r');
      if (p != NULL) *p = 0;
      p = strchr (buffer, '\n');
      if (p != NULL) *p = 0;

      // Remove any leading space. 
      for (p = buffer; isspace(*p); ++p) ;

      if ((*p == 0) || (*p == '#')) continue; // Empty line. 
      
      if( !roadmap_factory_load_menu_item( menu, p, actions))
      {
         assert(0);
         ssd_contextmenu_delete( menu, FALSE);
         menu = NULL;
         break;
      }
   }
   fclose(file);

   return menu;
}

ssd_contextmenu_ptr roadmap_factory_load_menu(
                        const char*          filename,
                        const RoadMapAction* actions)
{
   const char*          path = roadmap_path_first("config");
   ssd_contextmenu_ptr  menu = roadmap_factory_load_menu_file( 
                                          filename, 
                                          actions, 
                                          roadmap_path_user());

   if( menu)
      return menu;
   
   path = roadmap_path_first("config");
   while( !menu && path)
   {
      menu = roadmap_factory_load_menu_file( filename, actions, path);
      path = roadmap_path_next("config", path);
   }
   
   return menu;
}

const char **roadmap_factory_user_config (const char *name,
                                          const char *category,
                                          const RoadMapAction *actions) {

   const char **loaded;

   char file_name[256];

	if (name == NULL)
		return NULL;
		
   snprintf (file_name, sizeof(file_name), "%s.%s", name, category);

   loaded = roadmap_factory_load_config
               (file_name, actions, roadmap_path_user());

   if (loaded == NULL) {

      const char *path;

      for (path = roadmap_path_first("config");
           path != NULL;
           path = roadmap_path_next("config", path)) {

         loaded = roadmap_factory_load_config (file_name, actions, path);
         if (loaded != NULL) break;
      }
   }
   return loaded;
}

void roadmap_factory (const char           *name,
                      const RoadMapAction  *actions,
                      const char           *menu[],
                      const char           *toolbar[]) {

   int i;
   int prefix = strlen(ROADMAP_MENU);

   int use_toolbar =
            roadmap_config_match (&RoadMapConfigGeneralToolbar, "yes");

   int use_icons =
            roadmap_config_match (&RoadMapConfigGeneralIcons, "yes");

   RoadMapMenu gui_menu = NULL;

   for (i = 0; menu[i] != NULL; ++i) {

      const char *item = menu[i];

      if (item == RoadMapFactorySeparator) {

         roadmap_main_add_separator (gui_menu);

      } else if (item == RoadMapFactoryHelpTopics) {

         roadmap_factory_add_help (gui_menu);

      } else if (strncmp (item, ROADMAP_MENU, prefix) == 0) {

         gui_menu = roadmap_main_new_menu ();
         roadmap_main_add_menu (gui_menu, roadmap_lang_get (item + prefix));

      } else {
         const RoadMapAction *this_action;

         this_action = roadmap_factory_find_action (actions, item);
         if (this_action != NULL) {
            roadmap_main_add_menu_item
                              (gui_menu,
                               roadmap_lang_get (this_action->label_long),
                               roadmap_lang_get (this_action->tip),
                               this_action->callback);
         }
      }
   }

   if (use_toolbar) {

      const char **usertoolbar =
         roadmap_factory_user_config (name, "toolbar", actions);

      if (usertoolbar == NULL) usertoolbar = toolbar;

      for (i = 0; usertoolbar[i] != NULL; ++i) {

         const char *item = usertoolbar[i];

         if (item == RoadMapFactorySeparator) {

            roadmap_main_add_tool_space ();

         } else {

            const RoadMapAction *this_action;

            this_action = roadmap_factory_find_action (actions, item);

            if (this_action != NULL) {
               roadmap_main_add_tool (roadmap_factory_terse(this_action),
                                      (use_icons) ? this_action->name : NULL,
                                      this_action->tip,
                                      this_action->callback);
            }
         }
      }
   }

   /*
   if (RoadMapFactoryBindings != NULL) {
      roadmap_main_set_keyboard
         (RoadMapFactoryBindings, roadmap_factory_keyboard);
   }
   */
}


void roadmap_factory_keymap (const RoadMapAction  *actions,
                             const char           *shortcuts[]) {

   int i;

   if (RoadMapFactoryBindings != NULL) {
      roadmap_log (ROADMAP_FATAL, "RoadMap factory was called twice");
   }

   /* Count how many shortcuts we have to process. */
   for (i = 0; shortcuts[i] != NULL; ++i) ;

   /* Create the keyboard mapping table. */

   if (i > 0) {

      int j = 0;

      RoadMapFactoryBindings = 
         (struct RoadMapFactoryKeyMap *)
             calloc (i+1, sizeof(struct RoadMapFactoryKeyMap));
      roadmap_check_allocated(RoadMapFactoryBindings);

      for (i = 0; shortcuts[i] != NULL; ++i) {

         char *text;
         char *separator;
         const RoadMapAction *this_action;

         text = strdup (shortcuts[i]);
         roadmap_check_allocated(text);

         separator = strstr (text, ROADMAP_MAPPED_TO);
         if (separator != NULL) {

            char *p;

            /* Separate the name of the key from the name of the action. */

            for (p = separator; *p && (*p <= ' '); --p) *p = 0;

            p = separator + strlen(ROADMAP_MAPPED_TO);
            while (*p && (*p <= ' ')) ++p;
            this_action = roadmap_factory_find_action (actions, p);

            if (this_action != NULL) {

               int length = strlen(text);

               if (length > RoadMapFactoryKeyLength) {
                  RoadMapFactoryKeyLength = length;
               }
               RoadMapFactoryBindings[j].key = text;
               RoadMapFactoryBindings[j].action = this_action;
               ++j;
            } else {
               free (text);
            }
         }
      }
      RoadMapFactoryBindings[j].key = NULL;

      roadmap_main_set_keyboard
         (RoadMapFactoryBindings, roadmap_factory_keyboard);
   }
}


static void roadmap_factory_show_keymap (void) {

   const struct RoadMapFactoryKeyMap *binding;

   printf ("\nKEYMAP:\n");

   for (binding = RoadMapFactoryBindings; binding->key != NULL; ++binding) {

      const RoadMapAction *action = binding->action;

      if (action != NULL) {
         printf ("  %-*.*s  %s.\n",
                 RoadMapFactoryKeyLength,
                 RoadMapFactoryKeyLength,
                 binding->key,
                 action->tip);
      }
   }
}

void roadmap_factory_usage (const char *section, const RoadMapAction *action) {

   if ((section == NULL) || (strcasecmp(section, "KEYMAP") == 0)) {
      roadmap_factory_show_keymap();
   }

   if ((section == NULL) || (strcasecmp(section, "ACTIONS") == 0)) {

       printf ("\nACTIONS:\n");

       while (action->name != NULL) {

          printf ("%-20.20s %s\n", action->name, action->tip);
          action += 1;
       }
   }
}

RoadMapMenu roadmap_factory_menu (const char           *name,
                                  const char           *items[],
                                  const RoadMapAction  *actions) {

   int i;
   RoadMapMenu menu = NULL;
   const char **menu_items =
      roadmap_factory_user_config (name, "menu", actions);

   menu = roadmap_main_new_menu ();
   if (menu_items == NULL) menu_items = items;

   for (i = 0; menu_items[i] != NULL; ++i) {

      const char *item = menu_items[i];

      if (item == RoadMapFactorySeparator) {

         roadmap_main_add_separator (menu);

      } else {

         const RoadMapAction *this_action;

         this_action = roadmap_factory_find_action (actions, item);

         if (this_action != NULL) {
            roadmap_main_add_menu_item
                              (menu,
                               roadmap_lang_get (this_action->label_long),
                               roadmap_lang_get (this_action->tip),
                               this_action->callback);
         }
      }
   }

   return menu;
}


