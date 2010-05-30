/* roadmap_fileselection.c - manage the Widget used in roadmap dialogs.
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
 *   See roadmap_fileselection.h
 */

#include <string.h>
#include <stdlib.h>

#include <unistd.h>

#include "../roadmap_fileselection.h"

#include "../roadmap.h"
#include "../roadmap_types.h"
#include "../roadmap_file.h"


/* Some cross compiler environment don't define this ? */
#ifndef PATH_MAX
#define PATH_MAX 512
#endif


/* We maintain a list of dialogs that have been created. */

struct roadmap_fileselection_item;
typedef struct roadmap_fileselection_item RoadMapFileSelection;
    
/*
struct roadmap_fileselection_item {

    RoadMapFileSelection *next;
    
    const char *title;
    const char *mode;

    GtkWidget *dialog;
    RoadMapFileCallback callback;

};


static RoadMapFileSelection *RoadMapFileWindows = NULL;


static RoadMapFileSelection *roadmap_fileselection_search (const char *title) {
    
    RoadMapFileSelection *item;
    
    for (item = RoadMapFileWindows; item != NULL; item = item->next) {
        if (strcmp (title, item->title) == 0) {
            break;
        }
    }
    
    return item;
}


static void roadmap_fileselection_ok (GtkFileSelection *selector, gpointer user_data) {
    
    RoadMapFileSelection *item = (RoadMapFileSelection *)user_data;
    
    if (item->callback != NULL) {
        item->callback (gtk_file_selection_get_filename (GTK_FILE_SELECTION(item->dialog)),
                        item->mode);
    }
}

*/
void roadmap_fileselection_new (const char *title,
                                const char *filter,
                                const char *path,
                                const char *mode,
                                RoadMapFileCallback callback) {
                                /*
    
    RoadMapFileSelection *item = roadmap_fileselection_search (title);
 
    if (item == NULL) {
        
        char current_directory[PATH_MAX];
        
        if (path == NULL) {
            
            current_directory[0] = 0;
            
        } else if (getcwd (current_directory, sizeof(current_directory)) == NULL) {
            
            roadmap_log (ROADMAP_ERROR, "getcwd failed");
            current_directory[0] = 0;
            
        } else {
            
            chdir (path);
        }
        
        item = (RoadMapFileSelection *) malloc (sizeof (*item));
        roadmap_check_allocated(item);

        item->title  = strdup(title);
        item->dialog = gtk_file_selection_new (item->title);
        
        g_signal_connect (GTK_FILE_SELECTION(item->dialog)->ok_button,
                          "clicked",
                          (GCallback) roadmap_fileselection_ok,
                          item);
   		*/	   
        /* Ensure that the dialog box is hidden when the user clicks a button. */
  /* 
        g_signal_connect_swapped (GTK_FILE_SELECTION(item->dialog)->ok_button,
                                  "clicked",
                                  (GCallback) gtk_widget_hide_all,
                                  (gpointer) item->dialog);

        g_signal_connect_swapped (GTK_FILE_SELECTION(item->dialog)->cancel_button,
                                  "clicked",
                                  (GCallback) gtk_widget_hide_all,
                                  (gpointer) item->dialog);
        
        */
        /* Restore the original directory path.. */
        /*
        if (current_directory[0] != 0) {
            chdir (current_directory);
        }
    }
    
    item->mode = mode;
    item->callback = callback;
    
    if (filter != NULL) {
        gtk_file_selection_complete (GTK_FILE_SELECTION(item->dialog), filter);
    }
    
    if (item->dialog->window != NULL) {
        gdk_window_show (item->dialog->window);
        gdk_window_raise (item->dialog->window);
    }
    gtk_widget_show_all (GTK_WIDGET(item->dialog));
*/
}