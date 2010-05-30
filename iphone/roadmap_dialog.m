/* roadmap_dialog.c - manage the Widget used in roadmap dialogs.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright 2008 Morten Bek Ditlevsen
 *   Copyright 2008 Ehud Shabtai
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
 *   See roadmap_dialog.h
 */

#include <string.h>
#include <stdlib.h>

#include "../roadmap.h"
#include "../roadmap_types.h"
#include "../roadmap_start.h"
#include "roadmap_iphonemain.h"

#define ROADMAP_DIALOG_NO_LANG
#include "../roadmap_dialog.h"


#define ROADMAP_WIDGET_CONTAINER 0
#define ROADMAP_WIDGET_ENTRY     1
#define ROADMAP_WIDGET_CHOICE    2
#define ROADMAP_WIDGET_BUTTON    3
#define ROADMAP_WIDGET_LIST      4
#define ROADMAP_WIDGET_LABEL     5
#define ROADMAP_WIDGET_HIDDEN    6

enum {
    RM_LIST_WAYPOINT_NAME,
    RM_LIST_WAYPOINT_COLUMNS
};

/* We maintain a three-level tree of lists:
 * level 1: list of dialogs.
 * level 2: for each dialog, list of frames.
 * level 3: for each frame, list of "input" items.
 * In addition, "choice" items have a list of values.
 */
typedef struct {

   char *typeid;

   struct roadmap_dialog_item *item;
   RoadMapDialogCallback callback;
   char *value;

} RoadMapDialogSelection;


struct roadmap_dialog_item;
typedef struct roadmap_dialog_item *RoadMapDialogItem;

struct roadmap_dialog_item {

   char *typeid;

   struct roadmap_dialog_item *next;
   struct roadmap_dialog_item *parent;

   char *name;

   void *context;  /* References a caller-specific context. */

   int widget_type;
   UIView *w;

   short rank;
   short count;
   RoadMapDialogItem children;

   RoadMapDialogCallback callback;

   char *value;
   RoadMapDialogSelection *choice;
};

static RoadMapDialogItem RoadMapDialogWindows = NULL;
static RoadMapDialogItem RoadMapDialogCurrent = NULL;


static RoadMapDialogItem roadmap_dialog_get (RoadMapDialogItem parent,
                                             const char *name) {

   RoadMapDialogItem child;

   if (parent == NULL) {
      child = RoadMapDialogWindows;
   } else {
      child = parent->children;
   }

   while (child != NULL) {
      if (strcmp (child->name, name) == 0) {
         return child;
      }
      child = child->next;
   }

   /* We did not find this child: create a new one. */

   child = (RoadMapDialogItem) malloc (sizeof (*child));

   roadmap_check_allocated(child);

   child->typeid = "RoadMapDialogItem";

   child->widget_type = ROADMAP_WIDGET_CONTAINER; /* Safe default. */
   child->w        = NULL;
   child->count    = 0;
   child->name     = strdup(name);
   child->context  = NULL;
   child->parent   = parent;
   child->children = NULL;
   child->callback = NULL;
   child->value    = "";
   child->choice   = NULL;

   if (parent != NULL) {

      child->rank = parent->count;
      child->next = parent->children;
      parent->children = child;
      parent->count += 1;

   } else {

      /* This is a top-level list element (dialog window). */
      if (RoadMapDialogWindows == NULL) {
         child->rank = 0;
      } else {
         child->rank = RoadMapDialogWindows->rank + 1;
      }
      child->next = RoadMapDialogWindows;
      RoadMapDialogWindows = child;
      RoadMapDialogCurrent = child;
   }

   return child;
}
static void roadmap_dialog_hide_window (RoadMapDialogItem dialog) {

   if (dialog->w != NULL) {
		//AVI: bad[dialog->w _setHidden: YES];
    //  gtk_widget_hide (dialog->w);
   }
}

/*
static gint roadmap_dialog_action (GtkWidget *w, gpointer data) {

   RoadMapDialogItem item = (RoadMapDialogItem)data;
   RoadMapDialogCallback callback = item->callback;

   if (callback != NULL) {

      while (item->parent != NULL) {
         item = item->parent;
      }
      RoadMapDialogCurrent = item;

      (*callback) (item->name, item->context);
   }

   return FALSE;
}


static gint roadmap_dialog_destroyed (GtkWidget *w, gpointer data) {

   RoadMapDialogItem item = (RoadMapDialogItem)data;
   RoadMapDialogItem child;
*/
   /* Forget about the whole Gtk dialog: it is being destroyed. */
   /*

   for (child = item->children; child != NULL; child = child->next) {
      roadmap_dialog_destroyed (w, child);
   }
   item->w = NULL;

   return TRUE;
}


static gint roadmap_dialog_selected
               (GtkWidget *w, GdkEventButton *event, gpointer data) {

   roadmap_keyboard_set_focus ((RoadMapKeyboard) data, w);

   return FALSE;
}


static gint roadmap_dialog_chosen (gpointer data, GtkMenuItem *w) {

   RoadMapDialogSelection *selection = (RoadMapDialogSelection *)data;

   if (selection != NULL) {

      selection->item->value = selection->value;

      if (selection->callback != NULL) {

         RoadMapDialogItem item = selection->item;

         while (item->parent != NULL) {
            item = item->parent;
         }
         RoadMapDialogCurrent = item;

         (*selection->callback) (item->name, item->context);
      }
   }

   return FALSE;
}


static gboolean roadmap_dialog_list_selected
                    (GtkTreeSelection *selection,
                     GtkTreeModel *model,
                     GtkTreePath *path,
                     gboolean path_currently_selected,
                     gpointer data) {

   RoadMapDialogSelection *choice = (RoadMapDialogSelection *) data;

   if (! path_currently_selected) {

      int i = *gtk_tree_path_get_indices(path);

      if (choice != NULL && i >= 0) {

         roadmap_dialog_chosen (choice + i, NULL);
      }
   }

   return TRUE;
}
*/

static RoadMapDialogItem roadmap_dialog_new_item (const char *frame,
                                                  const char *name,
                                                  UIView *w,
                                                  int expand) {

   RoadMapDialogItem parent;
   RoadMapDialogItem child;

   parent = roadmap_dialog_get (RoadMapDialogCurrent, frame);
   child  = roadmap_dialog_get (parent, name);

   if (w == NULL) {
      child->w = w;
      return child;
   }

   if (parent->w == NULL) {

      /* This is a new frame: create the Gtk table for it. */
/*      parent->w = gtk_table_new (1, 2, FALSE);

      gtk_table_set_row_spacings (GTK_TABLE(parent->w), 2);
*/
   } else {

      /* This is an existing frame: increase the size of the frame.*/
 //     gtk_table_resize (GTK_TABLE(parent->w), parent->count, 2);
   }

   if (expand) {
  //    expand = GTK_EXPAND+GTK_FILL+GTK_SHRINK;
   }

   if (name[0] != '.') {

   //   GtkWidget *label;
      int length = strlen(name);
      char *title = malloc (length + 6);

      if (title != NULL) {

         title[0] = ' ';
         strcpy (title+1, name);
         if (name[length-1] != ':') {
            title[++length] = ':';
         }
         title[++length] = ' ';
         title[++length] = 0;

    //     label = gtk_label_new (title);
         free(title);

      } else {

     //    label = gtk_label_new (name);
      }
/*
      gtk_misc_set_alignment (GTK_MISC(label), 1.0, 0.5);

      gtk_table_attach (GTK_TABLE(parent->w),
                        label, 0, 1, child->rank, child->rank+1,
                        GTK_FILL, GTK_FILL, 2, 2);

      gtk_table_attach (GTK_TABLE(parent->w),
                        w, 1, 2, child->rank, child->rank+1,
                        GTK_EXPAND+GTK_FILL+GTK_SHRINK,
                        expand, 2, 2);
                        */
   } else {
/*
      gtk_table_attach (GTK_TABLE(parent->w),
                        w, 0, 2, child->rank, child->rank+1,
                        GTK_EXPAND+GTK_FILL+GTK_SHRINK,
                        expand, 2, 2);
                        */
   }

   child->w = w;

   return child;
}


int roadmap_dialog_activate (const char *name, void *context, int show)
{

   RoadMapDialogItem dialog = roadmap_dialog_get (NULL, name);

   dialog->context = context;

   if (dialog->w != NULL) {

      /* The dialog exists already: show it on top. */

      RoadMapDialogCurrent = dialog;
/*
      gdk_window_show (dialog->w->window);
      gdk_window_raise (dialog->w->window);
      gtk_widget_show_all (GTK_WIDGET(dialog->w));
      */

      return 0; /* Tell the caller the dialog already exists. */
   }

   /* Create the dialog's window. */

//   dialog->w = gtk_dialog_new();
//   gtk_window_set_title (GTK_WINDOW(dialog->w), roadmap_start_get_title(name));

/*   gtk_button_box_set_layout
      (GTK_BUTTON_BOX(GTK_DIALOG(dialog->w)->action_area),
       GTK_BUTTONBOX_SPREAD);
       */

   return 1; /* Tell the caller this is a new, undefined, dialog. */
}


void roadmap_dialog_hide (const char *name) {

   roadmap_dialog_hide_window (roadmap_dialog_get (NULL, name));
}

void roadmap_dialog_new_entry (const char *frame, const char *name,
                               RoadMapDialogCallback callback) {

//   GtkWidget *w = gtk_entry_new ();
   RoadMapDialogItem child = roadmap_dialog_new_item (frame, name, NULL /*w*/, 0);

   child->widget_type = ROADMAP_WIDGET_ENTRY;
}

void roadmap_dialog_new_label (const char *frame, const char *name) {

//   GtkWidget *w = gtk_label_new (name);
   RoadMapDialogItem child = roadmap_dialog_new_item (frame, name, /*w*/NULL, 0);

   child->widget_type = ROADMAP_WIDGET_LABEL;
}
/*

void roadmap_dialog_new_color (const char *frame, const char *name) {

   roadmap_dialog_new_entry (frame, name);
}
*/
void roadmap_dialog_new_hidden (const char *frame, const char *name) {
   RoadMapDialogItem child = roadmap_dialog_new_item (frame, name, 0, 0);

   child->widget_type = ROADMAP_WIDGET_HIDDEN;
}

void roadmap_dialog_new_choice (const char *frame,
                                const char *name,
                                int count,
                                const char **labels,
                                void **values,
                                RoadMapDialogCallback callback) {

   int i;
   char **vals = (char **)values;

   //GtkWidget *w = gtk_option_menu_new ();
   RoadMapDialogItem child = roadmap_dialog_new_item (frame, name, NULL /*w*/, 0);
/*   GtkWidget *menu;
   GtkWidget *menu_item;
   */
   RoadMapDialogSelection *choice;

   child->widget_type = ROADMAP_WIDGET_CHOICE;

//   menu = gtk_menu_new ();

   choice = (RoadMapDialogSelection *) calloc (count, sizeof(*choice));
   roadmap_check_allocated(choice);

   for (i = 0; i < count; ++i) {

      choice[i].typeid = "RoadMapDialogSelection";
      choice[i].item = child;
      choice[i].value = vals[i];
      choice[i].callback = NULL;

 /*     menu_item = gtk_menu_item_new_with_label (labels[i]);
      gtk_menu_shell_append (GTK_MENU_SHELL(menu), menu_item);

      g_signal_connect_swapped
                  (menu_item,
                   "activate",
                   (GCallback) roadmap_dialog_chosen,
                   (gpointer) (choice+i));

      gtk_widget_show (menu_item);
      */
   }
   /*
   gtk_option_menu_set_menu (GTK_OPTION_MENU(w), menu);
   gtk_option_menu_set_history (GTK_OPTION_MENU(w), current);
   */

   if (child->choice != NULL) {
      free(child->choice);
   }
   child->choice = choice;
   child->value  = choice[0].value;
}

void roadmap_dialog_new_list (const char  *frame, const char  *name) {
/*
   GtkWidget         *listbox;
   GtkListStore      *store;
   GtkTreeViewColumn *column;
   */


   //GtkWidget *scrollbox = gtk_scrolled_window_new (NULL, NULL);

   RoadMapDialogItem child =
      roadmap_dialog_new_item (frame, name, NULL /*scrollbox*/, 1);

/*
   store = gtk_list_store_new (RM_LIST_WAYPOINT_COLUMNS, G_TYPE_STRING);
   listbox = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
   g_object_unref (G_OBJECT (store));
   */


/*
   gtk_scrolled_window_add_with_viewport
         (GTK_SCROLLED_WINDOW(scrollbox), listbox);
         */

   child->w = NULL; //listbox;
   child->widget_type = ROADMAP_WIDGET_LIST;

   if (name[0] == '.') name += 1;

/*
   column = gtk_tree_view_column_new_with_attributes
                (name, gtk_cell_renderer_text_new (),
                 "text", RM_LIST_WAYPOINT_NAME,
                 NULL);

   gtk_tree_view_append_column (GTK_TREE_VIEW (listbox), column);

   gtk_tree_selection_set_mode
       (gtk_tree_view_get_selection (GTK_TREE_VIEW (listbox)),
        GTK_SELECTION_SINGLE);
        */
}


void roadmap_dialog_show_list (const char  *frame,
                               const char  *name,
                               int    count,
                               char **labels,
                               void **values,
                               RoadMapDialogCallback callback) {

   int i;
   RoadMapDialogItem parent;
   RoadMapDialogItem child;
   RoadMapDialogSelection *choice;

   /*GtkTreeModel *model;
   GtkTreeIter   iterator;

*/
   parent = roadmap_dialog_get (RoadMapDialogCurrent, frame);
   if (parent->w == NULL) {
      roadmap_log (ROADMAP_ERROR,
                   "list %s in dialog %s filled before built", name, frame);
      return;
   }

   child  = roadmap_dialog_get (parent, name);
   if (child->w == NULL) {
      roadmap_log (ROADMAP_ERROR,
                   "list %s in dialog %s filled before finished", name, frame);
      return;
   }
 //  model = gtk_tree_view_get_model (GTK_TREE_VIEW(child->w));

   if (child->choice != NULL) {
  //    gtk_list_store_clear (GTK_LIST_STORE(model));
      free (child->choice);
      child->choice = NULL;
   }

   choice = (RoadMapDialogSelection *) calloc (count, sizeof(*choice));
   roadmap_check_allocated(choice);

/*
   gtk_tree_selection_set_select_function
       (gtk_tree_view_get_selection (GTK_TREE_VIEW (child->w)),
        roadmap_dialog_list_selected,
        (gpointer)choice,
        NULL);
        */

   for (i = 0; i < count; ++i) {

      choice[i].typeid = "RoadMapDialogSelection";
      choice[i].item = child;
      choice[i].value = values[i];
      choice[i].callback = callback;

/*
      gtk_list_store_append (GTK_LIST_STORE(model), &iterator);
      gtk_list_store_set (GTK_LIST_STORE(model), &iterator,
                          RM_LIST_WAYPOINT_NAME, labels[i],
                          -1);
   */
   }
   child->choice = choice;
   child->value  = choice[0].value;

//   gtk_widget_show (parent->w);
}

void roadmap_dialog_add_button (const char *label, RoadMapDialogCallback callback){

   RoadMapDialogItem dialog = RoadMapDialogCurrent;
   RoadMapDialogItem child;

//   GtkWidget *button = gtk_button_new_with_label (label);

   child = roadmap_dialog_get (dialog, label);

   child->w = NULL; //button;
   child->callback = callback;
   child->widget_type = ROADMAP_WIDGET_BUTTON;
/*
   g_signal_connect (button, "clicked",
                     (GCallback) roadmap_dialog_action, child);

   GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

   gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog->w)->action_area),
                       button, TRUE, FALSE, 0);

   gtk_widget_grab_default (button);
   */
}


void roadmap_dialog_complete (int use_keyboard) {

   int count;
   RoadMapDialogItem dialog = RoadMapDialogCurrent;
   RoadMapDialogItem frame;


   count = 0;

   for (frame = dialog->children; frame != NULL; frame = frame->next) {
      if (frame->widget_type == ROADMAP_WIDGET_CONTAINER) {
         count += 1;
      }
   }

   if (count > 1) {

      /* There are several frames in that dialog: use a notebook widget
       * to let the user access all of them.*/
       
       /*
      GtkWidget *notebook = gtk_notebook_new();

      gtk_notebook_set_scrollable (GTK_NOTEBOOK(notebook), TRUE);

      gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog->w)->vbox),
                          notebook, TRUE, TRUE, 0);
                          */

      for (frame = dialog->children; frame != NULL; frame = frame->next) {

         if (frame->widget_type == ROADMAP_WIDGET_CONTAINER) {

/*
            GtkWidget *label = gtk_label_new (frame->name);

            gtk_notebook_append_page (GTK_NOTEBOOK(notebook), frame->w, label);
            */
         }
      }

   } else if (count == 1) {

      /* There is only one frame in that dialog: show it straight. */

      for (frame = dialog->children; frame != NULL; frame = frame->next) {

         if (frame->widget_type == ROADMAP_WIDGET_CONTAINER) {

/*            gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog->w)->vbox),
                                frame->w, TRUE, TRUE, 0);
                                */
            break;
         }
      }

   } else {
      roadmap_log (ROADMAP_FATAL,
                   "no frame defined for dialog %s", dialog->name);
   }

   if (use_keyboard) {

      RoadMapDialogItem last_item = NULL;

      for (frame = dialog->children; frame != NULL; frame = frame->next) {

         if (frame->widget_type == ROADMAP_WIDGET_CONTAINER) {

            RoadMapDialogItem item;

            for (item = frame->children; item != NULL; item = item->next) {

                if (item->widget_type == ROADMAP_WIDGET_ENTRY) {

/*
                   g_signal_connect (item->w, "button_press_event",
                                     (GCallback) roadmap_dialog_selected,
                                     keyboard);
*/
                   last_item = item;
                }
            }
         }
      }
      if (last_item != NULL) {
      }
   }

/*
   gtk_container_set_border_width
      (GTK_CONTAINER(GTK_BOX(GTK_DIALOG(dialog->w)->vbox)), 4);

   g_signal_connect (dialog->w, "destroy",
                     (GCallback) roadmap_dialog_destroyed,
                     dialog);

   roadmap_main_set_window_size (dialog->w,
                                 roadmap_option_width(dialog->name),
                                 roadmap_option_height(dialog->name));

   gtk_widget_show_all (GTK_WIDGET(dialog->w));
   */
}


void roadmap_dialog_select (const char *dialog) {

   RoadMapDialogCurrent = roadmap_dialog_get (NULL, dialog);
}


void *roadmap_dialog_get_data (const char *frame, const char *name) {

   RoadMapDialogItem this_frame;
   RoadMapDialogItem this_item;


   this_frame  = roadmap_dialog_get (RoadMapDialogCurrent, frame);
   this_item   = roadmap_dialog_get (this_frame, name);

   switch (this_item->widget_type) {

       case ROADMAP_WIDGET_ENTRY:
           break;

           //    return (void *)gtk_entry_get_text (GTK_ENTRY(this_item->w));
   }

   return this_item->value;
}


void  roadmap_dialog_set_data (const char *frame, const char *name,
                               const void *data) {

   RoadMapDialogItem this_frame;
   RoadMapDialogItem this_item;


   this_frame  = roadmap_dialog_get (RoadMapDialogCurrent, frame);
   this_item   = roadmap_dialog_get (this_frame, name);

   switch (this_item->widget_type) {

   case ROADMAP_WIDGET_ENTRY:

 //     gtk_entry_set_text (GTK_ENTRY(this_item->w), (const char *)data);
      break;

   case ROADMAP_WIDGET_LABEL:

//      gtk_label_set_text (GTK_LABEL(this_item->w), (const char *)data);
      break;
   }
   this_item->value = (char *)data;
}



void roadmap_dialog_new_mul_entry (const char *frame, const char *name,
                                   RoadMapDialogCallback callback)
{
}

void roadmap_dialog_new_password (const char *frame, const char *name)
{
}

void roadmap_dialog_new_image (const char *frame, const char *name)
{
}

void roadmap_dialog_new_progress (const char *frame, const char *name)
{
}

void  roadmap_dialog_set_progress (const char *frame, const char *name,
                                   int progress)
{
}

void  roadmap_dialog_set_focus (const char *frame, const char *name)
{
}