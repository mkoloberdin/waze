/* roadmap_dialog.c - manage the Widget used in roadmap dialogs.
 *
 * LICENSE:
 *
 *   Copyright 2007 Ehud Shabtai
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
#include <assert.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_lang.h"
#include "roadmap_start.h"

#include "roadmap_messagebox.h"
#define ROADMAP_DIALOG_NO_LANG
#include "roadmap_dialog.h"


#define ROADMAP_WIDGET_CONTAINER 0
#define ROADMAP_WIDGET_ENTRY     1
#define ROADMAP_WIDGET_CHOICE    2
#define ROADMAP_WIDGET_BUTTON    3
#define ROADMAP_WIDGET_LIST      4
#define ROADMAP_WIDGET_LABEL     5
#define ROADMAP_WIDGET_PASSWORD  6
#define ROADMAP_WIDGET_MUL_ENTRY 7
#define ROADMAP_WIDGET_PROGRESS  8

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
   struct roadmap_dialog_item *prev_dialog;

   char *name;

   void *context;  /* References a caller-specific context. */

   int widget_type;

#if 0   
   NOPH_Form_t form;
   NOPH_FormCommandMgr_t cmd_mgr;
   NOPH_Item_t item;
#else
   int form;
   int cmd_mgr;
   int item;   
#endif
   
   short rank;
   short count;
   RoadMapDialogItem children;

   RoadMapDialogCallback callback;

   char *value;
   RoadMapDialogSelection *choice;
   int num_choices;
   int data_is_string;
};

static RoadMapDialogItem RoadMapDialogWindows = NULL;
static RoadMapDialogItem RoadMapDialogCurrent = NULL;
//static NOPH_Display_t RoadMapDialogDisplay;
static void *RoadMapDialogEditFlowContext;


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

   child->widget_type   = ROADMAP_WIDGET_CONTAINER; /* Safe default. */
   child->form          = 0;
   child->item          = 0;
   child->count         = 0;
   child->name          = strdup(name);
   child->context       = NULL;
   child->parent        = parent;
   child->children      = NULL;
   child->callback      = NULL;
   child->value         = "";
   child->choice        = NULL;
   child->num_choices   = 0;
   child->data_is_string  = 0;

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
      child->prev_dialog = RoadMapDialogCurrent;
      RoadMapDialogCurrent = child;
      roadmap_log(ROADMAP_DEBUG, "New dialog as current:%s\n", name);
   }

   return child;
}


static void roadmap_dialog_hide_window (RoadMapDialogItem dialog) {

   if (RoadMapDialogCurrent != dialog) {
      RoadMapDialogItem itr;
      for (itr = RoadMapDialogCurrent; itr && (itr->prev_dialog != dialog);
      	   itr = itr->prev_dialog);

      if (!itr) return;
      
      assert (itr);
      itr->prev_dialog = itr->prev_dialog->prev_dialog;

      return;
   }

   RoadMapDialogCurrent = RoadMapDialogCurrent->prev_dialog;
   roadmap_log(ROADMAP_DEBUG, "Hide dialog :%s\n", dialog->name);

   if (!RoadMapDialogCurrent) {
      //NOPH_Display_setCurrent(RoadMapDialogDisplay, NOPH_GameCanvas_get());
   } else {
      //NOPH_Display_setCurrent(RoadMapDialogDisplay, RoadMapDialogCurrent->form);
   }

   //roadmap_dialog_destroy (dialog);
}


static RoadMapDialogItem roadmap_dialog_new_item (const char *frame,
                                                  const char *name,
                                                  int item) {

   RoadMapDialogItem parent;
   RoadMapDialogItem child;

   parent = roadmap_dialog_get (RoadMapDialogCurrent, frame);
   child  = roadmap_dialog_get (parent, name);

   //NOPH_Form_append (RoadMapDialogCurrent->form, item);
   child->item = item;

   return child;
}


static void roadmap_dialog_edit_flow (void) {
   RoadMapDialogItem item = (RoadMapDialogItem)RoadMapDialogEditFlowContext;
   RoadMapDialogCallback callback = item->callback;

   RoadMapDialogEditFlowContext = NULL;

   roadmap_main_remove_periodic (roadmap_dialog_edit_flow);

   roadmap_log(ROADMAP_DEBUG, "In roadmap_dialog_edit_flow: type:%s - %d\n", item->typeid, item->widget_type);

   if (callback != NULL) {

      while (item->parent != NULL) {
         item = item->parent;
      }
         
      if (RoadMapDialogCurrent == item) {
         (*callback) (item->name, item->context);
      }
   }
}


static void roadmap_dialog_chosen (char *name, void *context) {

   RoadMapDialogItem item = (RoadMapDialogItem)context;
   RoadMapDialogCallback callback = item->callback;

   roadmap_log(ROADMAP_DEBUG, "In roadmap_dialog_chosen: type:%s - %d\n", item->typeid, item->widget_type);
   switch (item->widget_type) {

   case ROADMAP_WIDGET_PASSWORD:
   case ROADMAP_WIDGET_ENTRY:
   {
     char value[255];
     //NOPH_TextField_getCString((NOPH_TextField_t)item->item, value, sizeof(value));
     if (strlen(item->value)) free(item->value);
     if (!strlen(value)) item->value = "";
     else item->value = strdup(value);

     roadmap_log(ROADMAP_DEBUG, "In roadmap_dialog_chosen(edit): str:%s, callback:0x%x\n", item->value, (int)callback);

     if (callback != NULL) {

         while (item->parent != NULL) {
            item = item->parent;
         }
         
         if (RoadMapDialogCurrent != item) {
            roadmap_log(ROADMAP_DEBUG, "****** callback(%s) not current (%s)! Ignoring!\n", item->name, RoadMapDialogCurrent->name);
            break;
            //item->prev_dialog = RoadMapDialogCurrent;
            //RoadMapDialogCurrent = item;
         }

         if (RoadMapDialogEditFlowContext) {
            roadmap_main_remove_periodic (roadmap_dialog_edit_flow);
         }

         RoadMapDialogEditFlowContext = context;
         roadmap_main_set_periodic (500, roadmap_dialog_edit_flow);

         //(*callback) (item->name, item->context);
      }
  }
  break;

   case ROADMAP_WIDGET_BUTTON:
     //printf("In roadmap_dialog_chosen(button): callback:0x%x\n", (int)callback);
     if (callback != NULL) {

         while (item->parent != NULL) {
            item = item->parent;
         }
         
         if (RoadMapDialogCurrent != item) {
            roadmap_log(ROADMAP_DEBUG, "****** callback(%s) not current (%s)! Ignoring!\n", item->name, RoadMapDialogCurrent->name);
            break;
            //item->prev_dialog = RoadMapDialogCurrent;
            //RoadMapDialogCurrent = item;
         }

         (*callback) (item->name, item->context);
      }
   break;

   case ROADMAP_WIDGET_LABEL:
   {
     char value[255];
     //NOPH_StringItem_getCString((NOPH_StringItem_t)item->item, value, sizeof(value));
     if (strlen(item->value)) free(item->value);
     if (!strlen(value)) item->value = "";
     else item->value = strdup(value);
   }
   break;

   case ROADMAP_WIDGET_MUL_ENTRY:
      break;

   case ROADMAP_WIDGET_CHOICE:
   case ROADMAP_WIDGET_LIST:

   {
      //int i = NOPH_ChoiceGroup_getSelectedIndex((NOPH_ChoiceGroup_t) item->item);
      int i = 0;
      char value[255];
      RoadMapDialogSelection *selection = item->choice + i;

      if (item->data_is_string) {
         //NOPH_ChoiceGroup_getCString((NOPH_ChoiceGroup_t)item->item, i, value, sizeof(value));
         if (strlen(item->value)) free(item->value);
         if (!strlen(value)) item->value = "";
         else item->value = strdup(value);

         roadmap_log(ROADMAP_DEBUG, "In roadmap_dialog_chosen(choice): i:%d str:%s, callback:0x%d\n",
            i, item->value, (int)selection->callback);
      } else {
         item->value = selection->value;
      }

      if (selection->callback != NULL) {

         while (item->parent != NULL) {
            item = item->parent;
         }

         if (RoadMapDialogCurrent != item) {
            RoadMapDialogItem current = RoadMapDialogCurrent;

            roadmap_log(ROADMAP_DEBUG, "****** callback(%s) not current (%s)!\n", item->name, RoadMapDialogCurrent->name);
            RoadMapDialogCurrent = item;
            (*selection->callback) (item->name, item->context);
            assert (item == RoadMapDialogCurrent);
            RoadMapDialogCurrent = current;
            //item->prev_dialog = RoadMapDialogCurrent;
            //RoadMapDialogCurrent = item;
         } else {
            (*selection->callback) (item->name, item->context);
         }

      }
   }

   break;

   }
}


int roadmap_dialog_activate (const char *name, void *context, int show) {

   RoadMapDialogItem dialog = roadmap_dialog_get (NULL, name);

   //if (!RoadMapDialogDisplay)
        //RoadMapDialogDisplay = NOPH_Display_getDisplay(NOPH_MIDlet_get());

   dialog->context = context;

   if (dialog->form != 0) {

      /* The dialog exists already: show it on top. */

      if (RoadMapDialogCurrent != dialog) {
         roadmap_log(ROADMAP_DEBUG, "Dialog (%s) exists but not current (%s)!\n", name, RoadMapDialogCurrent->name);
         dialog->prev_dialog = RoadMapDialogCurrent;
         RoadMapDialogCurrent = dialog;
      }
      //if (show) NOPH_Display_setCurrent(RoadMapDialogDisplay, dialog->form);

      return 0; /* Tell the caller the dialog already exists. */
   }

   /* Create the dialog's window. */

   //dialog->form = NOPH_Form_new (roadmap_start_get_title(name));
   //dialog->cmd_mgr = NOPH_FormCommandMgr_new (dialog->form);

   return 1; /* Tell the caller this is a new, undefined, dialog. */
}


void roadmap_dialog_hide (const char *name) {

   roadmap_dialog_hide_window (roadmap_dialog_get (NULL, name));
}


void roadmap_dialog_new_entry (const char *frame, const char *name,
                               RoadMapDialogCallback callback) {

#if 0                               
   NOPH_TextField_t item = NOPH_TextField_new (name, "", 100, NOPH_TextField_ANY);
   RoadMapDialogItem child = roadmap_dialog_new_item (frame, name, (NOPH_Item_t)item);
   child->widget_type = ROADMAP_WIDGET_ENTRY;
   child->value = "";
   child->callback = callback;

   NOPH_FormCommandMgr_addCallback(RoadMapDialogCurrent->cmd_mgr, (NOPH_Item_t)item, roadmap_dialog_chosen, "", child);
#endif   
}


void roadmap_dialog_new_mul_entry (const char *frame, const char *name,
                                   RoadMapDialogCallback callback) {

   //child->widget_type = ROADMAP_WIDGET_MUL_ENTRY;
}


void roadmap_dialog_new_progress (const char *frame, const char *name) {
#if 0
   NOPH_Gauge_t item;
   RoadMapDialogItem child;

   if (!strcmp(name, roadmap_lang_get("Please wait..."))) {
      /* This is a special progress bar which is used as a wait cursor */
      item = NOPH_Gauge_new
              (name, 0, NOPH_Gauge_INDEFINITE, NOPH_Gauge_CONTINUOUS_RUNNING);
   } else {
      item = NOPH_Gauge_new (name, 0, 100, 0);
   }

   child = roadmap_dialog_new_item (frame, name, (NOPH_Item_t)item);
   child->widget_type = ROADMAP_WIDGET_PROGRESS;
   child->value = "";
#endif   
}


void roadmap_dialog_new_image (const char *frame, const char *name) {}


void roadmap_dialog_new_password (const char *frame, const char *name) {
#if 0
   NOPH_TextField_t item = NOPH_TextField_new (name, "", 100, NOPH_TextField_PASSWORD);
   RoadMapDialogItem child = roadmap_dialog_new_item (frame, name, (NOPH_Item_t)item);
   child->widget_type = ROADMAP_WIDGET_PASSWORD;
   child->value = "";
   NOPH_FormCommandMgr_addCallback(RoadMapDialogCurrent->cmd_mgr, (NOPH_Item_t)item, roadmap_dialog_chosen, "", child);
#endif   
}


void roadmap_dialog_new_label (const char *frame, const char *_name) {
#if 0
   NOPH_StringItem_t item;
   RoadMapDialogItem child;

   if (*_name) {
     char *name = malloc(strlen(_name) + 3);
     strcpy(name, _name);
     strcat(name, ": ");
     item = NOPH_StringItem_new (name, "");
     free(name);
   } else {
     item = NOPH_StringItem_new (_name, "");
   }

   child = roadmap_dialog_new_item (frame, _name, (NOPH_Item_t)item);

   child->widget_type = ROADMAP_WIDGET_LABEL;
#endif
}


void roadmap_dialog_new_color (const char *frame, const char *name) {
   roadmap_dialog_new_entry (frame, name, NULL);
}


void roadmap_dialog_new_choice (const char *frame,
                                const char *name,
                                int count,
                                const char **labels,
                                void **values,
                                RoadMapDialogCallback callback) {
#if 0
   int i;
   NOPH_ChoiceGroup_t item = NOPH_ChoiceGroup_new (name, NOPH_Choice_POPUP);
   RoadMapDialogItem child = roadmap_dialog_new_item (frame, name, (NOPH_Item_t)item);
   RoadMapDialogSelection *choice;

   NOPH_Item_setLayout((NOPH_Item_t)item, NOPH_Item_LAYOUT_EXPAND);

   child->widget_type = ROADMAP_WIDGET_CHOICE;

   if ((void **)labels == values) {
      child->data_is_string = 1;
   }

   choice = (RoadMapDialogSelection *) calloc (count, sizeof(*choice));
   roadmap_check_allocated(choice);

   for (i = 0; i < count; ++i) {

      choice[i].typeid = "RoadMapDialogSelection";
      choice[i].item = child;
      choice[i].value = values[i];
      choice[i].callback = callback;

      NOPH_ChoiceGroup_append(item, labels[i], 0);
   }

   if (child->choice != NULL) {
      free(child->choice);
   }
   child->choice = choice;
   child->num_choices = count;
   child->value  = choice[0].value;
   if (child->data_is_string && strlen(child->value)) {
      child->value = strdup(child->value);
   }

   NOPH_FormCommandMgr_addCallback(RoadMapDialogCurrent->cmd_mgr, (NOPH_Item_t)item, roadmap_dialog_chosen, "", child);
#endif
}


void roadmap_dialog_new_list (const char  *frame, const char  *name) {
#if 0
   NOPH_ChoiceGroup_t item;
   RoadMapDialogItem child;

   const char *title = name;
   if (title[0] == '.') title += 1;

   item = NOPH_ChoiceGroup_new (title, NOPH_Choice_EXCLUSIVE);
   child = roadmap_dialog_new_item (frame, name, (NOPH_Item_t)item);

   child->item = item;
   child->widget_type = ROADMAP_WIDGET_LIST;
#endif   
}


void roadmap_dialog_show_list (const char  *frame,
                               const char  *name,
                               int    count,
                               char **labels,
                               void **values,
                               RoadMapDialogCallback callback) {
#if 0                               
   int i;
   RoadMapDialogItem parent;
   RoadMapDialogItem child;
   RoadMapDialogSelection *choice;
   char *empty_list[1] = {""};

   parent = roadmap_dialog_get (RoadMapDialogCurrent, frame);
#if 0   
   if (parent->item == 0) {
      roadmap_log (ROADMAP_ERROR,
                   "list %s in dialog %s filled before built", name, frame);
      return;
   }
#endif

   child = roadmap_dialog_get (parent, name);
   if (child->item == 0) {
      roadmap_log (ROADMAP_ERROR,
                   "list %s in dialog %s filled before finished", name, frame);
      return;
   }

   if (child->choice != NULL) {
      NOPH_ChoiceGroup_deleteAll((NOPH_ChoiceGroup_t)child->item);
      free (child->choice);
      child->choice = NULL;
   }

   if (!count) {
      count = 1;
      labels = empty_list;
      values = (void **)labels;
   }

   choice = (RoadMapDialogSelection *) calloc (count, sizeof(*choice));
   roadmap_check_allocated(choice);

   for (i = 0; i < count; ++i) {

      choice[i].typeid = "RoadMapDialogSelection";
      choice[i].item = child;
      choice[i].value = values[i];
      choice[i].callback = callback;
   }
   
   //qsort(choice, count, sizeof(*choice), listview_sort_cmp);

   for (i = 0; i < count; ++i) {

      NOPH_ChoiceGroup_append(child->item, choice[i].value, 0);
   }
   
   child->choice = choice;
   child->num_choices = count;
   child->value  = choice[0].value;

   NOPH_FormCommandMgr_addCallback(RoadMapDialogCurrent->cmd_mgr, (NOPH_Item_t)child->item, roadmap_dialog_chosen, "", child);
#endif   
}


void roadmap_dialog_add_button (const char *label,
                                RoadMapDialogCallback callback) {
#if 0                                
   RoadMapDialogItem dialog = RoadMapDialogCurrent;
   RoadMapDialogItem child;

   child = roadmap_dialog_get (dialog, label);

   child->callback = callback;
   child->widget_type = ROADMAP_WIDGET_BUTTON;

   NOPH_FormCommandMgr_addCommand (dialog->cmd_mgr, label, roadmap_dialog_chosen, "", child);
#endif   
}


void roadmap_dialog_complete (int use_keyboard) {

   RoadMapDialogItem dialog = RoadMapDialogCurrent;

   //NOPH_Display_setCurrent(RoadMapDialogDisplay, dialog->form);

#if 0

   count = 0;

   for (frame = dialog->children; frame != NULL; frame = frame->next) {
      if (frame->widget_type == ROADMAP_WIDGET_CONTAINER) {
         count += 1;
      }
   }

   if (count > 1) {

      /* There are several frames in that dialog: use a notebook widget
       * to let the user access all of them.
       */
      GtkWidget *notebook = gtk_notebook_new();

      gtk_notebook_set_scrollable (GTK_NOTEBOOK(notebook), TRUE);

      gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog->w)->vbox),
                          notebook, TRUE, TRUE, 0);

      for (frame = dialog->children; frame != NULL; frame = frame->next) {

         if (frame->widget_type == ROADMAP_WIDGET_CONTAINER) {

            GtkWidget *label = gtk_label_new (frame->name);

            gtk_notebook_append_page (GTK_NOTEBOOK(notebook), frame->w, label);
         }
      }

   } else if (count == 1) {

      /* There is only one frame in that dialog: show it straight. */

      for (frame = dialog->children; frame != NULL; frame = frame->next) {

         if (frame->widget_type == ROADMAP_WIDGET_CONTAINER) {

            gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog->w)->vbox),
                                frame->w, TRUE, TRUE, 0);
            break;
         }
      }

   } else {
      roadmap_log (ROADMAP_FATAL,
                   "no frame defined for dialog %s", dialog->name);
   }

   if (use_keyboard) {

      RoadMapDialogItem last_item = NULL;
      RoadMapKeyboard   keyboard  = roadmap_keyboard_new ();

      gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog->w)->vbox),
                          roadmap_keyboard_widget(keyboard),
                          TRUE, TRUE, 0);

      for (frame = dialog->children; frame != NULL; frame = frame->next) {

         if (frame->widget_type == ROADMAP_WIDGET_CONTAINER) {

            RoadMapDialogItem item;

            for (item = frame->children; item != NULL; item = item->next) {

                if (item->widget_type == ROADMAP_WIDGET_ENTRY) {

                   g_signal_connect (item->w, "button_press_event",
                                     (GCallback) roadmap_dialog_selected,
                                     keyboard);

                   last_item = item;
                }
            }
         }
      }
      if (last_item != NULL) {
         roadmap_keyboard_set_focus (keyboard, last_item->w);
      }
   }

   gtk_container_set_border_width
      (GTK_CONTAINER(GTK_BOX(GTK_DIALOG(dialog->w)->vbox)), 4);

   g_signal_connect (dialog->w, "destroy",
                     (GCallback) roadmap_dialog_destroyed,
                     dialog);

   gtk_grab_add (dialog->w);

   roadmap_main_set_window_size (dialog->w,
                                 roadmap_option_width(dialog->name),
                                 roadmap_option_height(dialog->name));

   gtk_widget_show_all (GTK_WIDGET(dialog->w));
#endif
}


void roadmap_dialog_select (const char *dialog) {

   RoadMapDialogItem item = roadmap_dialog_get (NULL, dialog);
   if (item != RoadMapDialogCurrent) {
      roadmap_log(ROADMAP_DEBUG, "roadmap_dialog_select(%s) not current (%s)!\n", item->name, RoadMapDialogCurrent->name);
      item->prev_dialog = RoadMapDialogCurrent;
      RoadMapDialogCurrent = item;
   }
}


void roadmap_dialog_set_focus (const char *frame, const char *name) {
   RoadMapDialogItem this_frame;
   RoadMapDialogItem this_item;

   this_frame  = roadmap_dialog_get (RoadMapDialogCurrent, frame);
   this_item   = roadmap_dialog_get (this_frame, name);

   //NOPH_Display_setCurrentItem(RoadMapDialogDisplay, (NOPH_Item_t)this_item->item);
}


void *roadmap_dialog_get_data (const char *frame, const char *name) {

   RoadMapDialogItem this_frame;
   RoadMapDialogItem this_item;


   this_frame  = roadmap_dialog_get (RoadMapDialogCurrent, frame);
   this_item   = roadmap_dialog_get (this_frame, name);

   return this_item->value;
}


void  roadmap_dialog_set_data (const char *frame, const char *name,
                               const void *data) {

   RoadMapDialogItem this_frame;
   RoadMapDialogItem this_item;
   int i;


   this_frame  = roadmap_dialog_get (RoadMapDialogCurrent, frame);
   this_item   = roadmap_dialog_get (this_frame, name);

   switch (this_item->widget_type) {

   case ROADMAP_WIDGET_PASSWORD:
   case ROADMAP_WIDGET_ENTRY:

      //NOPH_TextField_setString((NOPH_TextField_t)this_item->item, data);
      break;

   case ROADMAP_WIDGET_LABEL:
      //NOPH_StringItem_setText((NOPH_StringItem_t)this_item->item, data);
      break;

#if 0
   case ROADMAP_WIDGET_MUL_ENTRY:
      {
         GtkTextBuffer *buffer = gtk_text_buffer_new (NULL);
         gtk_text_buffer_set_text (buffer, (const char *)data, strlen(data));
         gtk_text_view_set_buffer (GTK_TEXT_VIEW(this_item->w), buffer);
         g_object_unref (buffer);
      }
      break;
#endif

   case ROADMAP_WIDGET_CHOICE:
   case ROADMAP_WIDGET_LIST:

      if (this_item->data_is_string) {
         char value[255];
         for (i=0; i < this_item->num_choices; i++) {
            //NOPH_ChoiceGroup_getCString((NOPH_ChoiceGroup_t)this_item->item, i, value, sizeof(value));
            if (!strcmp (value, data)) {
               //NOPH_ChoiceGroup_setSelectedIndex((NOPH_ChoiceGroup_t)this_item->item, i, 1);
               break;
            }
         }
      } else {
         for (i=0; i < this_item->num_choices; i++) {
            if (data == this_item->choice[i].value) {
               //NOPH_ChoiceGroup_setSelectedIndex((NOPH_ChoiceGroup_t)this_item->item, i, 1);
               break;
            }
         }
      }
      
      if ((i == this_item->num_choices) && this_item->data_is_string) {

         RoadMapDialogSelection *choice;
         int count = this_item->num_choices + 1;

         choice = (RoadMapDialogSelection *)
            realloc (this_item->choice, count * sizeof(*choice));
         roadmap_check_allocated(choice);

         this_item->choice = choice;
         //NOPH_ChoiceGroup_append(this_item->item, data, 0);

         this_item->choice[this_item->num_choices].value = (char *)data;
         this_item->choice[this_item->num_choices].callback = 0;
         this_item->choice[this_item->num_choices].typeid = "RoadMapDialogSelection";
         this_item->choice[this_item->num_choices].item = this_item;
         this_item->num_choices++;

         //NOPH_ChoiceGroup_setSelectedIndex((NOPH_ChoiceGroup_t)this_item->item, i, 1);
      }

      break;
   }
   this_item->value = (char *)data;
}

void  roadmap_dialog_set_progress (const char *frame, const char *name,
                                   int progress) {
   RoadMapDialogItem this_frame;
   RoadMapDialogItem this_item;

   this_frame  = roadmap_dialog_get (RoadMapDialogCurrent, frame);
   this_item   = roadmap_dialog_get (this_frame, name);

   if (this_item->widget_type != ROADMAP_WIDGET_PROGRESS) return;

   //NOPH_Gauge_setValue((NOPH_Gauge_t)this_item->item, progress);
}

static void messgaebox_dismiss (const char *name, void *context) {
   roadmap_dialog_hide ("Message Box");
}
