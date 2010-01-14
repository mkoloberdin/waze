/* roadmap_dialog.c - manage the Widget used in roadmap dialogs.
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
 *
 *   Based on an implementation by Pascal F. Martin.
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

#include <windows.h>
#include "resource.h"
#include <commctrl.h>
#ifdef UNDER_CE
#include <aygshell.h>
#endif

#include "../roadmap.h"
#include "../roadmap_types.h"
#include "../roadmap_start.h"
#include "../roadmap_lang.h"
#include "../roadmap_canvas.h"

#define ROADMAP_DIALOG_NO_LANG
#include "../roadmap_dialog.h"

extern HWND RoadMapMainWindow;
extern HINSTANCE g_hInst;

#define ROADMAP_WIDGET_CONTAINER 0
#define ROADMAP_WIDGET_ENTRY     1
#define ROADMAP_WIDGET_CHOICE    2
#define ROADMAP_WIDGET_BUTTON    3
#define ROADMAP_WIDGET_LIST      4
#define ROADMAP_WIDGET_LABEL     5
#define ROADMAP_WIDGET_PASSWORD  6
#define ROADMAP_WIDGET_PROGRESS  7
#define ROADMAP_WIDGET_IMAGE     8
#define ROADMAP_WIDGET_MUL_ENTRY 9

const unsigned int MAX_ROW_HEIGHT = 20;
const unsigned int MAX_ROW_SPACE = 5;
const unsigned int MAX_LIST_HEIGHT = 80;
const unsigned int BUTTON_WIDTH = 50;

static INT_PTR CALLBACK DialogFunc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK TabDialogFunc(HWND hDlg, UINT message, WPARAM wParam,
				LPARAM lParam);

/* We maintain a three-level tree of lists:
* level 1: list of dialogs.
* level 2: for each dialog, list of frames.
* level 3: for each frame, list of "input" items.
* In addition, "choice" items have a list of values.
*/
typedef struct {

	char *type_id;
	struct roadmap_dialog_item *item;
	RoadMapDialogCallback callback;
	char *value;
	const char *label;

} RoadMapDialogSelection;


struct roadmap_dialog_item;
typedef struct roadmap_dialog_item *RoadMapDialogItem;

struct roadmap_dialog_item {

	char *type_id;

	struct roadmap_dialog_item *next;
	struct roadmap_dialog_item *parent;

	const char *name;
	const char *title;

	void *context;  /* References a caller-specific context. */

	int widget_type;
	HWND w;
	HWND label;

	short rank;
	short count;
	RoadMapDialogItem children;

	RoadMapDialogCallback callback;

	char *value;
	char *ansi_value;
	RoadMapDialogSelection *choice;
   int num_choices;
   unsigned char data_is_string;
   unsigned char use_keyboard;
};

static RoadMapDialogItem RoadMapDialogWindows = NULL;
static RoadMapDialogItem RoadMapDialogCurrent = NULL;


static RoadMapDialogItem roadmap_dialog_get (RoadMapDialogItem parent,
											 const char *name)
{
	RoadMapDialogItem child;
   RoadMapDialogItem last_child = NULL;

	if (parent == NULL) {
		child = RoadMapDialogWindows;
	} else {
		child = parent->children;
	}

	while (child != NULL) {
		if (strcmp (child->name, name) == 0) {
			return child;
		}
      last_child = child;
		child = child->next;
	}

	/* We did not find this child: create a new one. */

	child = (RoadMapDialogItem) malloc (sizeof (*child));

	roadmap_check_allocated(child);

	child->type_id = "RoadMapDialogItem";

	child->widget_type = ROADMAP_WIDGET_CONTAINER; /* Safe default. */
	child->w        = NULL;
	child->label    = NULL;
	child->count    = 0;
	child->name     = strdup(name);
	child->title    = "";
	child->context  = NULL;
	child->parent   = parent;
	child->children = NULL;
	child->callback = NULL;
	child->value    = "";
	child->ansi_value = NULL;
	child->choice   = NULL;
   child->num_choices = 0;
   child->data_is_string = 0;
   child->use_keyboard = 0;

	if (parent != NULL) {

		child->rank = parent->count;
      child->next = NULL;

      if (last_child == NULL) {
         parent->children = child;
      } else {
         last_child->next = child;
      }

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


static void roadmap_dialog_hide_window (RoadMapDialogItem dialog)
{
	if (dialog->w != NULL) {

		ShowWindow(dialog->w, SW_HIDE);

#ifdef UNDER_CE
      if (dialog->use_keyboard) {
         SHSipPreference(dialog->w, SIP_DOWN);
      }
#endif
	}
}


static int roadmap_dialog_action (RoadMapDialogItem item)
{
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


static int roadmap_dialog_destroyed (HWND w, RoadMapDialogItem item)
{
	RoadMapDialogItem child;

	for (child = item->children; child != NULL; child = child->next) {
		roadmap_dialog_destroyed (w, child);
	}
	item->w = NULL;

	return TRUE;
}


static int roadmap_dialog_chosen (RoadMapDialogSelection *selection)
{
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


static RoadMapDialogItem roadmap_dialog_new_item (const char *frame,
												  const char *name)
{
	RoadMapDialogItem parent;
	RoadMapDialogItem child;

	parent = roadmap_dialog_get (RoadMapDialogCurrent, frame);
	child  = roadmap_dialog_get (parent, name);

	return child;
}


static void show_sip_button (HWND parent) {

   HWND hWndSipButton;

   hWndSipButton = FindWindow(TEXT("MS_SIPBUTTON"), NULL);

   if (hWndSipButton) {
      SetWindowPos(hWndSipButton, parent, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
   }
}

int roadmap_dialog_activate (const char *name, void *context, int show)
{
	RoadMapDialogItem dialog = roadmap_dialog_get (NULL, name);
	
	dialog->context = context;
	
	if (dialog->w != NULL) {
		
		/* The dialog exists already: show it on top. */
		
		RoadMapDialogCurrent = dialog;

		ShowWindow(dialog->w, SW_SHOW);

      if (dialog->use_keyboard) {
         show_sip_button (dialog->w);
      }

#ifdef UNDER_CE		
      if (dialog->use_keyboard) {
         SHSipPreference(dialog->w, SIP_UP);
      }
#endif
		
		return 0; /* Tell the caller the dialog already exists. */
	}
	
	dialog->title = roadmap_start_get_title(name);
	return 1; /* Tell the caller this is a new, undefined, dialog. */
}


void roadmap_dialog_hide (const char *name)
{
	roadmap_dialog_hide_window (roadmap_dialog_get (NULL, name));
   //roadmap_main_toggle_full_screen ();
}


void roadmap_dialog_new_entry (const char *frame, const char *name,
                               RoadMapDialogCallback callback)
{
	RoadMapDialogItem child = roadmap_dialog_new_item (frame, name);
	
   child->callback = callback;
   child->widget_type = ROADMAP_WIDGET_ENTRY;
}


void roadmap_dialog_new_mul_entry (const char *frame, const char *name,
                                   RoadMapDialogCallback callback)
{
	RoadMapDialogItem child = roadmap_dialog_new_item (frame, name);
	
   child->callback = callback;
   child->widget_type = ROADMAP_WIDGET_MUL_ENTRY;
}


void roadmap_dialog_new_password (const char *frame, const char *name)
{
	RoadMapDialogItem child = roadmap_dialog_new_item (frame, name);
	
   child->widget_type = ROADMAP_WIDGET_PASSWORD;
}


void roadmap_dialog_new_progress (const char *frame, const char *name)
{
	RoadMapDialogItem child = roadmap_dialog_new_item (frame, name);
	
   child->widget_type = ROADMAP_WIDGET_PROGRESS;
}


void roadmap_dialog_new_label (const char *frame, const char *name)
{
	RoadMapDialogItem child = roadmap_dialog_new_item (frame, name);
	
	child->widget_type = ROADMAP_WIDGET_LABEL;
}


void roadmap_dialog_new_image (const char *frame, const char *name)
{
	RoadMapDialogItem child = roadmap_dialog_new_item (frame, name);
	
	child->widget_type = ROADMAP_WIDGET_IMAGE;
}


void roadmap_dialog_new_color (const char *frame, const char *name)
{
	roadmap_dialog_new_entry (frame, name, NULL);
}


void roadmap_dialog_new_choice (const char *frame,
								const char *name,
								int count,
								const char **labels,
								void **values,
								RoadMapDialogCallback callback)
{
	int i;
	RoadMapDialogItem child = roadmap_dialog_new_item (frame, name);
	RoadMapDialogSelection *choice;
	
	child->widget_type = ROADMAP_WIDGET_CHOICE;
	
	choice = (RoadMapDialogSelection *) calloc (count+1, sizeof(*choice));
	roadmap_check_allocated(choice);
	
	for (i = 0; i < count; ++i) {
		choice[i].type_id = "RoadMapDialogSelection";
		choice[i].item = child;
		choice[i].value = (char*)values[i];
		choice[i].label = labels[i];
		choice[i].callback = NULL;
	}
	
	if (child->choice != NULL) {
		free(child->choice);
	}
	child->choice = choice;
	child->callback = callback;
	child->value  = choice[0].value;
   child->num_choices = count;

   if ((void **)labels == values) {
      child->data_is_string = 1;
   }
}


void roadmap_dialog_new_list (const char  *frame, const char  *name)
{
	RoadMapDialogItem child =
		roadmap_dialog_new_item (frame, name);
	
	child->widget_type = ROADMAP_WIDGET_LIST;
}


static int listview_sort_cmp (const void *a, const void *b) {

   RoadMapDialogSelection *c1 = (RoadMapDialogSelection *)a;
   RoadMapDialogSelection *c2 = (RoadMapDialogSelection *)b;

   return strcmp (c1->label, c2->label);
}


void roadmap_dialog_show_list (const char  *frame,
							   const char  *name,
							   int    count,
							   char **labels,
							   void **values,
							   RoadMapDialogCallback callback)
{
	int i;
	RoadMapDialogItem parent;
	RoadMapDialogItem child;
	RoadMapDialogSelection *choice;
	LPWSTR str;
	
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
	
	if (child->choice != NULL) {
		SendMessage(child->w, (UINT) LB_RESETCONTENT, 0, 0);
		free (child->choice);
		child->choice = NULL;
	}
	
	choice = (RoadMapDialogSelection *)calloc(count + 1, sizeof(*choice));
	roadmap_check_allocated(choice);
	
	for (i = 0; i < count; ++i) {
		
		choice[i].type_id = "RoadMapDialogSelection";
		choice[i].item = child;
		choice[i].value = (char*)values[i];
		choice[i].label = labels[i];
		choice[i].callback = callback;		
	}

   qsort(choice, count, sizeof(*choice), listview_sort_cmp);

	for (i = 0; i < count; ++i) {
				
		str = ConvertToWideChar(choice[i].label, CP_UTF8);
		SendMessage(child->w, (UINT) LB_ADDSTRING, 0, (LPARAM)str);
		free(str);
	}

	child->choice = choice;
	child->value  = choice[0].value;
   child->num_choices = count;
	
	/* SendMessage(child->w, (UINT) LB_SETCURSEL, 0, 0); */
}


void roadmap_dialog_add_button (const char *label,
								RoadMapDialogCallback callback)
{
	RoadMapDialogItem dialog = RoadMapDialogCurrent;
	RoadMapDialogItem child;
	
	child = roadmap_dialog_get (dialog, label);
	
	child->callback = callback;
	child->widget_type = ROADMAP_WIDGET_BUTTON;
}


/* Property Sheet callback */
static int CALLBACK DoPropSheetProc(HWND hWndDlg, UINT uMsg, LPARAM lParam)
{
	if (hWndDlg == NULL) return 0;
	if (uMsg == PSCB_INITIALIZED) {

		SetWindowLong (hWndDlg, GWL_STYLE, WS_CHILD | WS_VISIBLE);
		SetWindowLong (hWndDlg, GWL_EXSTYLE,
			0x00010000/*WS_EX_CONTROLPARENT*/);
		SetParent(hWndDlg, RoadMapDialogCurrent->w);
	}
	return 0;
} 


static char *roadmap_dialog_selection_get(RoadMapDialogItem item, int i)
{
   static char *text = NULL;
   TCHAR data[255];
   int len;

   if (text) {
      free(text);
      text = NULL;
   }

   len = SendMessage(item->w, CB_GETLBTEXTLEN, (WPARAM)i, 0);
   if (len >= 255) return "";

   if (SendMessage(item->w, CB_GETLBTEXT, (WPARAM)i, (LPARAM)data) ==
      CB_ERR) {

      return "";
   }

   text = ConvertToMultiByte(data, CP_UTF8);

   if (!text) return "";

   return text;
}


void roadmap_dialog_complete (int use_keyboard)
{
	int count, i;
	RoadMapDialogItem dialog = RoadMapDialogCurrent;
	RoadMapDialogItem frame;
	HWND sheet;
	RECT client;
	PROPSHEETHEADER psh;
	PROPSHEETPAGE *psp;
	LPWSTR str;
	
	
	count = 0;
	
	for (frame = dialog->children; frame != NULL; frame = frame->next) {
		if (frame->widget_type == ROADMAP_WIDGET_CONTAINER) {
			count += 1;
		}
	}
	
	if (count == 0) count = 1;

	psp = (PROPSHEETPAGE*) calloc(count, sizeof(*psp));
	
   dialog->use_keyboard = use_keyboard;

	dialog->w = CreateDialogParam(g_hInst, (LPCWSTR)IDD_GENERIC, GetActiveWindow(), /*RoadMapMainWindow, FIXME*/
		DialogFunc, (LPARAM)dialog);
	
	ShowWindow(dialog->w, SW_HIDE);
   if (dialog->title[0] == '.') {
      str = ConvertToWideChar(dialog->title+1, CP_UTF8);
   } else {
   	str = ConvertToWideChar(dialog->title, CP_UTF8);
   }
	SetWindowText(dialog->w, str);
	free(str);

   if (use_keyboard) {
      show_sip_button (dialog->w);
   }

   if (count == 1) {
#ifdef UNDER_CE
      if (use_keyboard) {
         SHSipPreference(dialog->w, SIP_UP);
         //roadmap_main_toggle_full_screen ();
      }
#endif
      ShowWindow(dialog->w, SW_SHOW);
      return;
   }
   
	i = 1;
	for (frame = dialog->children; frame != NULL; frame = frame->next) {
		if (frame->widget_type != ROADMAP_WIDGET_CONTAINER) continue;
		psp[count-i].dwSize = sizeof(PROPSHEETPAGE);
		psp[count-i].dwFlags = PSP_PREMATURE|PSP_USETITLE;
		psp[count-i].hInstance = g_hInst;
		psp[count-i].pszTemplate = MAKEINTRESOURCE(IDD_GENERIC);

      if (frame->name[0] == '.') {
         psp[count-i].pszTitle = ConvertToWideChar(frame->name+1, CP_UTF8);
      } else {
   	   psp[count-i].pszTitle = ConvertToWideChar(frame->name, CP_UTF8);
      }

		psp[count-i].pfnDlgProc = TabDialogFunc;
		psp[count-i].lParam = (LPARAM) frame;
		i++;
	}
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags =
		PSH_PROPSHEETPAGE|PSH_USECALLBACK|PSH_MODELESS;
#ifdef UNDER_CE
	psh.dwFlags |= PSH_MAXIMIZE;
#else
	psh.dwFlags |= PSH_NOAPPLYNOW;
#endif
	psh.hwndParent = dialog->w;
	psh.hInstance = g_hInst;
	psh.pszCaption = ConvertToWideChar(dialog->name, CP_UTF8);
	psh.nPages = count;
	psh.nStartPage = 0;
	psh.ppsp = psp;
	psh.pfnCallback = DoPropSheetProc;
	sheet = (HWND)PropertySheet(&psh);
	
	ShowWindow(GetDlgItem(sheet, IDOK), SW_HIDE);
	ShowWindow(GetDlgItem(sheet, IDCANCEL), SW_HIDE);

	GetClientRect(dialog->w, &client);
   SetWindowPos(sheet, NULL,
      0, 0,
      client.right - client.left,
		client.bottom - client.top - MAX_ROW_HEIGHT,
      SWP_NOZORDER);
      
/*	MoveWindow(sheet, client.top, client.left,
		client.bottom - client.top - MAX_ROW_HEIGHT,
		client.right - client.left, TRUE);*/

#ifdef UNDER_CE
   if (use_keyboard) {
      SHSipPreference(dialog->w, SIP_UP);
      //roadmap_main_toggle_full_screen ();
   }
#endif

	ShowWindow(dialog->w, SW_SHOW);
   
	free((void*)psh.pszCaption);
	for (i=0; i< count; i++) {
		free((void*)psp[i].pszTitle);
	}

}


void roadmap_dialog_select (const char *dialog)
{
	RoadMapDialogCurrent = roadmap_dialog_get (NULL, dialog);
}


void roadmap_dialog_set_focus (const char *frame, const char *name)
{
 	RoadMapDialogItem this_frame;
	RoadMapDialogItem this_item;
	
	this_frame  = roadmap_dialog_get (RoadMapDialogCurrent, frame);
	this_item   = roadmap_dialog_get (this_frame, name);

   if (this_item->w) {
      SetFocus (this_item->w);
	}
}


void *roadmap_dialog_get_data (const char *frame, const char *name)
{
	RoadMapDialogItem this_frame;
	RoadMapDialogItem this_item;
	TCHAR str[200];
	
	this_frame  = roadmap_dialog_get (RoadMapDialogCurrent, frame);
	this_item   = roadmap_dialog_get (this_frame, name);
	
	switch (this_item->widget_type) {

   case ROADMAP_WIDGET_PASSWORD:
	case ROADMAP_WIDGET_ENTRY:
   case ROADMAP_WIDGET_LABEL:
   case ROADMAP_WIDGET_MUL_ENTRY:
		GetWindowText(this_item->w, str, sizeof(str)/sizeof(str[0]));
		if (this_item->ansi_value != NULL) {
			free(this_item->ansi_value);
		}
		this_item->ansi_value = ConvertToMultiByte(str, CP_UTF8);
		return this_item->ansi_value;

	case ROADMAP_WIDGET_CHOICE:
      if (this_item->data_is_string) {
		   if ((this_item->ansi_value != NULL) &&
            this_item->ansi_value[0]) {

			   free(this_item->ansi_value);
		   }
      
		   this_item->ansi_value =
            strdup(roadmap_dialog_selection_get(this_item,
                     SendMessage(this_item->w, CB_GETCURSEL, 0, 0)));

		   return this_item->ansi_value;
      }
      break;
	}
	
	return this_item->value;
}


void  roadmap_dialog_set_progress (const char *frame, const char *name,
                                   int progress) {

	RoadMapDialogItem this_frame;
	RoadMapDialogItem this_item;
	
	
	this_frame  = roadmap_dialog_get (RoadMapDialogCurrent, frame);
	this_item   = roadmap_dialog_get (this_frame, name);

   if (this_item->widget_type != ROADMAP_WIDGET_PROGRESS) return;

   SendMessage(this_item->w, PBM_SETPOS, (WPARAM)progress, 0);
}

void  roadmap_dialog_set_data (const char *frame, const char *name,
							   const void *data)
{
	RoadMapDialogItem this_frame;
	RoadMapDialogItem this_item;
	
	
	this_frame  = roadmap_dialog_get (RoadMapDialogCurrent, frame);
	this_item   = roadmap_dialog_get (this_frame, name);
	
	switch (this_item->widget_type) {

   case ROADMAP_WIDGET_PASSWORD:
	case ROADMAP_WIDGET_ENTRY:
   case ROADMAP_WIDGET_MUL_ENTRY:
	case ROADMAP_WIDGET_LABEL:
		{
			LPWSTR str = ConvertToWideChar((char*)data, CP_UTF8);
			SetWindowText(this_item->w, str);
			free(str);
		}
		break;
	case ROADMAP_WIDGET_CHOICE:
		{
         int i;
         for (i=0; i<this_item->num_choices; i++) {
            if ((!this_item->data_is_string &&
                  (data == this_item->choice[i].value)) ||
                (this_item->data_is_string &&
                 !strcmp (roadmap_dialog_selection_get(this_item, i), data))) {

               SendMessage
                  (this_item->w, (UINT) CB_SETCURSEL, (WPARAM) i, (LPARAM) 0);
               break;
            }
         }
         
         if ((i == this_item->num_choices) && this_item->data_is_string) {
            
            RoadMapDialogSelection *choice;
            LPWSTR label;
            int count = this_item->num_choices + 1;
            
            choice = (RoadMapDialogSelection *)
               realloc (this_item->choice, count * sizeof(*choice));
            roadmap_check_allocated(choice);
            
            this_item->choice = choice;
            
            memcpy (this_item->choice + this_item->num_choices,
               this_item->choice + this_item->num_choices - 1,
               sizeof(RoadMapDialogSelection));
            
            this_item->choice[this_item->num_choices].value = (char *)data;
            this_item->num_choices++;
            
            label = ConvertToWideChar((const char *)data, CP_UTF8);
            SendMessage(this_item->w, (UINT) CB_ADDSTRING, (WPARAM) 0,
               (LPARAM) label);
            SendMessage(this_item->w, (UINT) CB_SETCURSEL, (WPARAM) i, (LPARAM) 0);
            free(label);
         }
         
		}
		break;

	}
	this_item->value = (char *)data;
}


static HWND create_item(RoadMapDialogItem item, HWND parent)
{
	LPWSTR name_unicode = NULL;
	DWORD dwStyle = WS_VISIBLE | WS_CHILD;
	int length;
	char *title;

	const char *name = item->name;

	length = strlen(name);
	title = (char*)malloc (length + 6);

	if (title != NULL) {

		title[0] = ' ';

      if (name[0] == '.') {
         length--;
         name++;
		   strcpy (title+1, name);
      } else {
   		strcpy (title+1, name);
	   	if (name[length-1] != ':') {
		   	title[++length] = ':';
   		}
      }

		title[++length] = ' ';
		title[++length] = 0;
	}

	switch (item->widget_type) {

   case ROADMAP_WIDGET_PASSWORD:
	case ROADMAP_WIDGET_ENTRY:
   case ROADMAP_WIDGET_MUL_ENTRY:
		name_unicode = ConvertToWideChar(title, CP_UTF8);
		item->label = CreateWindowEx (
			0,
			L"STATIC",            // Class name
			name_unicode,  		 // Window name
			dwStyle,              // Window style
			0,                    // x-coordinate of the upper-left corner
			0,                    // y-coordinate of the upper-left corner
			CW_USEDEFAULT,        // The width of the tree-view control window
			CW_USEDEFAULT,        // The height of the tree-view control window
			parent,               // Window handle to the parent window
			(HMENU) NULL,         // The tree-view control identifier
			g_hInst,              // The instance handle
			NULL);                // Specify NULL for this parameter when you 

		dwStyle |= WS_BORDER|ES_AUTOHSCROLL;
	   
      if (item->widget_type == ROADMAP_WIDGET_PASSWORD) {
         dwStyle |= ES_PASSWORD;
      } else if (item->widget_type == ROADMAP_WIDGET_MUL_ENTRY) {
         dwStyle |= WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL;

         if (roadmap_lang_rtl ()) dwStyle |= ES_RIGHT;
      }

		item->w = CreateWindowEx (
			0,
			L"EDIT",              // Class name
			NULL,		             // Window name
			dwStyle,              // Window style
			0,                    // x-coordinate of the upper-left corner
			0,                    // y-coordinate of the upper-left corner
			CW_USEDEFAULT,        // The width of the tree-view control window
			CW_USEDEFAULT,        // The height of the tree-view control window
			parent,               // Window handle to the parent window
			(HMENU) NULL,         // The tree-view control identifier
			g_hInst,              // The instance handle
			NULL);                // Specify NULL for this parameter when you 

		SetWindowLong(item->w, GWL_USERDATA, (LONG)item);
		break;

	case ROADMAP_WIDGET_PROGRESS:

		//dwStyle |= WS_BORDER|ES_AUTOHSCROLL;
	   
		item->w = CreateWindowEx (
			0,
			PROGRESS_CLASS,       // Class name
			NULL,		             // Window name
			dwStyle,              // Window style
			0,                    // x-coordinate of the upper-left corner
			0,                    // y-coordinate of the upper-left corner
			CW_USEDEFAULT,        // The width of the tree-view control window
			CW_USEDEFAULT,        // The height of the tree-view control window
			parent,               // Window handle to the parent window
			(HMENU) NULL,         // The tree-view control identifier
			g_hInst,              // The instance handle
			NULL);                // Specify NULL for this parameter when you 

		SetWindowLong(item->w, GWL_USERDATA, (LONG)item);
      SendMessage(item->w, PBM_SETRANGE, 0, MAKELPARAM(0,100));       
		break;

	case ROADMAP_WIDGET_CHOICE: 
		name_unicode = ConvertToWideChar(title, CP_UTF8);
		item->label = CreateWindowEx (
			0,
			L"STATIC",            // Class name
			name_unicode,		  // Window name
			dwStyle,              // Window style
			0,                    // x-coordinate of the upper-left corner
			0,                    // y-coordinate of the upper-left corner
			CW_USEDEFAULT,        // The width of the tree-view control window
			CW_USEDEFAULT,        // The height of the tree-view control window
			parent,               // Window handle to the parent window
			(HMENU) NULL,		  // The tree-view control identifier
			g_hInst,              // The instance handle
			NULL);                // Specify NULL for this parameter when you 

		dwStyle |= WS_BORDER|CBS_DROPDOWNLIST|WS_VSCROLL;
		item->w = CreateWindowEx (
			0,
			L"COMBOBOX",          // Class name
			NULL,		          // Window name
			dwStyle,              // Window style
			0,                    // x-coordinate of the upper-left corner
			0,                    // y-coordinate of the upper-left corner
			CW_USEDEFAULT,        // The width of the tree-view control window
			CW_USEDEFAULT,        // The height of the tree-view control window
			parent,               // Window handle to the parent window
			(HMENU) NULL,         // The tree-view control identifier
			g_hInst,              // The instance handle
			NULL);                // Specify NULL for this parameter when you 

		SetWindowLong(item->w, GWL_USERDATA, (LONG)item);

		if (item->choice != NULL) {
			int i =0;
			while (item->choice[i].item != NULL) {
				LPWSTR label = ConvertToWideChar(item->choice[i].label, CP_UTF8);
				SendMessage(item->w, (UINT) CB_ADDSTRING, (WPARAM) 0,
							   (LPARAM) label);
				free(label);
				i++;
			}
			SendMessage(item->w, (UINT) CB_SETCURSEL, (WPARAM) 0, (LPARAM) 0);
		}
		break;

	case ROADMAP_WIDGET_LABEL:
		name_unicode = ConvertToWideChar(title, CP_UTF8);
		item->label = CreateWindowEx (
			0,
			L"STATIC",            // Class name
			name_unicode,		    // Window name
			dwStyle,              // Window style
			0,                    // x-coordinate of the upper-left corner
			0,                    // y-coordinate of the upper-left corner
			CW_USEDEFAULT,        // The width of the tree-view control window
			CW_USEDEFAULT,        // The height of the tree-view control window
			parent,               // Window handle to the parent window
			(HMENU) NULL,         // The tree-view control identifier
			g_hInst,              // The instance handle
			NULL);                // Specify NULL for this parameter when you 

		item->w = CreateWindowEx (
			0,
			L"STATIC",            // Class name
			NULL,				       // Window name
			dwStyle,              // Window style
			0,                    // x-coordinate of the upper-left corner
			0,                    // y-coordinate of the upper-left corner
			CW_USEDEFAULT,        // The width of the tree-view control window
			CW_USEDEFAULT,        // The height of the tree-view control window
			parent,               // Window handle to the parent window
			(HMENU) NULL,         // The tree-view control identifier
			g_hInst,              // The instance handle
			NULL);                // Specify NULL for this parameter when you 
		break;

	case ROADMAP_WIDGET_IMAGE:
      name_unicode = ConvertToWideChar(name, CP_UTF8);
		item->w = CreateWindowEx (
			0,
			L"STATIC",            // Class name
			NULL,				       // Window name
			dwStyle | SS_BITMAP,  // Window style
			0,                    // x-coordinate of the upper-left corner
			0,                    // y-coordinate of the upper-left corner
			CW_USEDEFAULT,        // The width of the tree-view control window
			CW_USEDEFAULT,        // The height of the tree-view control window
			parent,               // Window handle to the parent window
			(HMENU) NULL,         // The tree-view control identifier
			g_hInst,              // The instance handle
			NULL);                // Specify NULL for this parameter when you 
  
#ifdef UNDER_CE
      SendMessage(item->w, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP,
         (LPARAM)SHLoadDIBitmap(name_unicode));
#else
      SendMessage(item->w, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP,
         (LPARAM)LoadImage(NULL, name_unicode, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE));
#endif
		break;


	case ROADMAP_WIDGET_BUTTON:
		name_unicode = ConvertToWideChar(name, CP_UTF8);

		item->w = CreateWindowEx (
			0,
			L"BUTTON",            // Class name
			name_unicode,         // Window name
			dwStyle,              // Window style
			0,                    // x-coordinate of the upper-left corner
			0,                    // y-coordinate of the upper-left corner
			CW_USEDEFAULT,        // The width of the tree-view control window
			CW_USEDEFAULT,        // The height of the tree-view control window
			parent,               // Window handle to the parent window
			(HMENU) NULL,         // The tree-view control identifier
			g_hInst,              // The instance handle
			NULL);                // Specify NULL for this parameter when you 
		SetWindowLong(item->w, GWL_USERDATA, (LONG)item);
		break;

	case ROADMAP_WIDGET_LIST:
		name_unicode = ConvertToWideChar(title, CP_UTF8);
		item->label = CreateWindowEx (
			0,
			L"STATIC",            // Class name
			name_unicode,         // Window name
			dwStyle,              // Window style
			0,                    // x-coordinate of the upper-left corner
			0,                    // y-coordinate of the upper-left corner
			CW_USEDEFAULT,        // The width of the tree-view control window
			CW_USEDEFAULT,        // The height of the tree-view control window
			parent,               // Window handle to the parent window
			(HMENU) NULL,         // The tree-view control identifier
			g_hInst,              // The instance handle
			NULL);                // Specify NULL for this parameter when you 

		dwStyle |= WS_BORDER|LBS_NOTIFY|WS_VSCROLL;
		item->w = CreateWindowEx (
			0,
			L"LISTBOX",           // Class name
			NULL,                 // Window name
			dwStyle,              // Window style
			0,                    // x-coordinate of the upper-left corner
			0,                    // y-coordinate of the upper-left corner
			CW_USEDEFAULT,        // The width of the tree-view control window
			CW_USEDEFAULT,        // The height of the tree-view control window
			parent,               // Window handle to the parent window
			(HMENU) NULL,         // The tree-view control identifier
			g_hInst,              // The instance handle
			NULL);                // Specify NULL for this parameter when you 

		SetWindowLong(item->w, GWL_USERDATA, (LONG)item);
		break;
	}

	if (name_unicode != NULL) free(name_unicode);
	free(title);
	return item->w;
}

static void CreateControllers(HWND hDlg, RoadMapDialogItem frame)
{
	RoadMapDialogItem child = frame->children;

	while (child != NULL) {
		create_item(child, hDlg);
		child = child->next;
	}

#ifdef UNDER_CE
   CreateWindow (WC_SIPPREF, TEXT(""), WS_CHILD, -10, -10, 5, 5,
      hDlg, (HMENU) NULL, g_hInst, NULL);
#endif
}


int calc_frame_height(RoadMapDialogItem frame)
{
   RoadMapDialogItem item;
   int num_entries = 0;

   for (item = frame->children; item != NULL; item = item->next) {

      if (item->widget_type == ROADMAP_WIDGET_CONTAINER) continue;
      if (item->widget_type == ROADMAP_WIDGET_IMAGE) continue;
      
      if (item->widget_type == ROADMAP_WIDGET_LIST) {
         num_entries += 5;
      } else if (item->widget_type == ROADMAP_WIDGET_MUL_ENTRY) {
         num_entries += 3;
      } else {
         num_entries++;
      }      
   }
   
   num_entries++;

   return num_entries * (MAX_ROW_HEIGHT + MAX_ROW_SPACE);
}


static void MoveControlls (HWND hDlg, RoadMapDialogItem frame, int width, int height)
{
	HDC dc;
   RoadMapDialogItem item;
   int max_name_len = 0;
   int max_name_height = 0;
   unsigned int num_entries = 0;
   unsigned int row_height;
   unsigned int row_space;
   unsigned int first_column_width;
   unsigned int column_edge_width;
   unsigned int column_separator;
   unsigned int second_column_width;
   int curr_y;
   int label_y_add;
   RECT rc;
   
   if (frame == NULL) return;
   
   GetClientRect(frame->w, &rc);
   dc = GetDC(frame->w);
   for (item = frame->children; item != NULL; item = item->next) {
      LPWSTR name;
      SIZE text_size;
      if (item->widget_type == ROADMAP_WIDGET_CONTAINER) continue;
      if (item->widget_type == ROADMAP_WIDGET_IMAGE) continue;
      
      if (item->widget_type == ROADMAP_WIDGET_LIST) {
         num_entries += 4;
      } else if (item->widget_type == ROADMAP_WIDGET_MUL_ENTRY) {
         num_entries += 3;
      } else {
         num_entries++;
      }
      
      name = ConvertToWideChar(item->name, CP_UTF8);
      GetTextExtentPoint(dc, name, wcslen(name), &text_size);
      if ((text_size.cx < width/2) && text_size.cx > max_name_len) max_name_len = text_size.cx;
      if (text_size.cy > max_name_height) max_name_height =
         text_size.cy;
      free(name);
   }
   
   row_height = (int)(height / (num_entries + 1));
   row_space = (int) (row_height / num_entries) - 1;
   if (row_space < 0) row_space = 1;
   
   if (row_height > MAX_ROW_HEIGHT) {
      row_height = MAX_ROW_HEIGHT;
      row_space = MAX_ROW_SPACE;
   }
   
   
   first_column_width = max_name_len + 10;
   column_edge_width = 3;
   column_separator = 4;
   second_column_width = width - column_edge_width*2 -
      column_separator;
   
   curr_y = 3;
   label_y_add = (row_height - max_name_height) / 2;
   
   if (label_y_add < 0) label_y_add = 0;
   
   for (item = frame->children; item != NULL; item = item->next) {
      HWND widget = item->w;
      HWND label = item->label;
      if (item->widget_type == ROADMAP_WIDGET_CONTAINER) {
         continue;
      }
      
      if (item->widget_type == ROADMAP_WIDGET_IMAGE) {
         RECT rc;
         GetClientRect(item->w, &rc);
         SetWindowPos(item->w, NULL, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
         curr_y += rc.bottom;
         continue;
      }
      
      if (item->widget_type  == ROADMAP_WIDGET_PROGRESS) {
         MoveWindow(widget,
            column_separator, curr_y,
            width - column_separator*2,
            row_height, TRUE);
         curr_y += row_height;
         
      } else if ((item->widget_type  == ROADMAP_WIDGET_LIST) ||
         (item->widget_type  == ROADMAP_WIDGET_MUL_ENTRY)) {
         
         int rows_mul = 3;
         
         if (item->widget_type  == ROADMAP_WIDGET_LIST) rows_mul = 5;
         
         if (!roadmap_lang_rtl ()) {
            MoveWindow(label,
               column_edge_width, curr_y + label_y_add,
               first_column_width, row_height - label_y_add,
               TRUE);
         } else {
            MoveWindow(label,
               width - column_edge_width - first_column_width,
               curr_y + label_y_add,
               first_column_width, row_height - label_y_add,
               TRUE);
         }
         curr_y += row_height;
         MoveWindow(widget,
            column_edge_width, curr_y,
            width - column_edge_width*2,
            row_height*rows_mul, TRUE);
         curr_y += row_height*rows_mul + row_space;
      } else {
         if (label != NULL) {
            unsigned int widget_height = row_height;
            
            if (item->widget_type == ROADMAP_WIDGET_CHOICE) {

               widget_height = roadmap_canvas_height () - rc.top - curr_y - 2;

               if (widget_height > MAX_LIST_HEIGHT*2) {
                  widget_height = MAX_LIST_HEIGHT*2;
               }
            }
            
            if (!roadmap_lang_rtl ()) {
               MoveWindow(label,
                  column_edge_width, curr_y + label_y_add,
                  first_column_width, row_height - label_y_add,
                  TRUE);
               
               MoveWindow(widget,
                  column_edge_width + first_column_width +
                  column_separator,
                  curr_y,
                  width - (column_edge_width*2 +
                  first_column_width + column_separator),
                  widget_height, TRUE);
            } else {
               SIZE text_size;
               LPWSTR name;
               
               name = ConvertToWideChar(item->name, CP_UTF8);
               GetTextExtentPoint(dc, name, wcslen(name), &text_size);
               free(name);
               
               MoveWindow(label,
                  width - column_edge_width - text_size.cx - 10,
                  curr_y + label_y_add,
                  text_size.cx + 10, row_height - label_y_add,
                  TRUE);
               
               if (item->widget_type == ROADMAP_WIDGET_LABEL) {
                  
                  MoveWindow(widget,
                     column_edge_width * 2,
                     curr_y + label_y_add,
                     width - (column_edge_width*2 +
                     first_column_width + column_separator),
                     row_height - label_y_add, TRUE);
               } else {
                  MoveWindow(widget,
                     column_edge_width,
                     curr_y,
                     width - (column_edge_width*2 +
                     first_column_width + column_separator),
                     widget_height, TRUE);
               }
            }
         } else {
            MoveWindow(widget,
               column_edge_width, curr_y, first_column_width,
               row_height, TRUE);
         }
         curr_y += row_height + row_space;
      }
   }
   
   ReleaseDC(hDlg, dc);
}

      
INT_PTR CALLBACK DialogFunc(HWND hDlg, UINT message, WPARAM wParam,
				LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			RoadMapDialogItem dialog = (RoadMapDialogItem)lParam;
			RoadMapDialogItem frame;
         RoadMapDialogItem child_frame = NULL;
			int num_buttons;
         int num_containers = 0;
         int min_height = 0;
         int has_cancel_button = 0;
#ifdef UNDER_CE
         SHINITDLGINFO shidi;
#else
			SetWindowLong(hDlg, GWL_STYLE,
			GetWindowLong(hDlg, GWL_STYLE) | WS_OVERLAPPEDWINDOW);

#endif
			SetWindowLong(hDlg, GWL_USERDATA, (LONG)dialog);

			/* create buttons */
			num_buttons = 0;
			for (frame = dialog->children; frame != NULL; frame = frame->next) {
				if (frame->widget_type != ROADMAP_WIDGET_CONTAINER) {
					create_item(frame, hDlg);

               if (!strcmp(frame->name, roadmap_lang_get ("Cancel"))) {
                  has_cancel_button = 1;
               }

					num_buttons++;
            } else {
               int height = calc_frame_height (frame);
               if (height > min_height) min_height = height;
               num_containers++;
               child_frame = frame;

            }
			}

#ifdef UNDER_CE
         if (has_cancel_button) {
			   SetWindowLong(hDlg, GWL_STYLE,
   		      GetWindowLong(hDlg, GWL_STYLE) | WS_NONAVDONEBUTTON);

            SHDoneButton(hDlg, SHDB_HIDE);
         }
#endif

         if (num_containers == 1) {
			   CreateControllers(hDlg, child_frame);
            child_frame->w = hDlg;
         }

         if (min_height < 120) min_height = 120;

#ifdef UNDER_CE

			shidi.dwMask = SHIDIM_FLAGS;
         if (num_containers > 3) {
			   shidi.dwFlags = SHIDIF_FULLSCREENNOMENUBAR;
         } else {
            shidi.dwFlags = 0;
            SetWindowPos(hDlg, HWND_TOP, 10, 30, 220, min_height+30, SWP_DRAWFRAME);
         }

			shidi.hDlg = hDlg;
			SHInitDialog(&shidi);

	      return (INT_PTR)TRUE;
#else
		 if (num_containers < 3) {
			SetWindowPos(hDlg, HWND_TOP, 10, 30, 300, min_height+100, SWP_NOMOVE|SWP_DRAWFRAME);
		 }
#endif

		}
		/* Fall through to resize the dialog */

		{
				RECT rc;

				GetClientRect(hDlg, &rc);
		    	lParam = MAKELPARAM(rc.right-rc.left, rc.bottom-rc.top);
		}
	

	case WM_SIZE:
		{
			RoadMapDialogItem dialog = (RoadMapDialogItem)GetWindowLong(hDlg,
				GWL_USERDATA);
			RoadMapDialogItem frame;
         RoadMapDialogItem child_frame;
         int num_containers = 0;
			int width = LOWORD(lParam);
			int height = HIWORD(lParam);

   		if (dialog == NULL) return (INT_PTR)FALSE;
	   	else {
				//buttons
				int curr_y = height - MAX_ROW_HEIGHT;
				int row_height = MAX_ROW_HEIGHT - 1;
				int column_separator = 5;
				int curr_x;
				int num_buttons = 0;
				unsigned int cur_button_width = BUTTON_WIDTH;

				for (frame = dialog->children; frame != NULL;
								frame = frame->next) {
					if (frame->widget_type == ROADMAP_WIDGET_BUTTON) {
							num_buttons++;
					}
				}

            if (num_buttons <= 2) cur_button_width = (int)(BUTTON_WIDTH * 1.5);
				if (((cur_button_width + column_separator) * num_buttons) >=
								(unsigned)width) {
					curr_x = width - cur_button_width - 1;
				} else {
					curr_x = width - cur_button_width -
						(width - (cur_button_width + column_separator) *
						 num_buttons) / 2;
				}

				for (frame = dialog->children; frame != NULL;
								frame = frame->next) {
					if (frame->widget_type != ROADMAP_WIDGET_CONTAINER) {
						MoveWindow(frame->w,
							curr_x, curr_y, cur_button_width, row_height, TRUE);
						curr_x -= cur_button_width + column_separator;
               } else {
                  num_containers++;
                  child_frame = frame;
               }
				}
			}

         if (num_containers == 1) {
            MoveControlls (hDlg, child_frame, width, height-MAX_ROW_HEIGHT);
         }

			if (dialog == NULL) break;
/*
			for (frame = dialog->children; frame != NULL; frame = frame->next) {
				if (frame->widget_type == ROADMAP_WIDGET_CONTAINER) {
					HWND sheet = GetParent(frame->w);
					if (sheet != NULL) {
						MoveWindow(sheet, 0, 0,
							width,
							height - MAX_ROW_HEIGHT,
							TRUE);
						return TRUE;
					}
				}
			}
*/
		}
		break;

	case WM_COMMAND:

      if (((HWND)lParam == hDlg) && (LOWORD(wParam) == IDOK)) {

        roadmap_dialog_hide_window
            ((RoadMapDialogItem)GetWindowLong((HWND)lParam, GWL_USERDATA));
        return TRUE;
      }

		if ((HIWORD(wParam) == BN_CLICKED) ||
			(HIWORD(wParam) == LBN_SELCHANGE) ||
         (HIWORD(wParam) == EN_CHANGE) ||
			(HIWORD(wParam) == CBN_SELCHANGE)) {
				int i = -1;
				RoadMapDialogItem item =
					(RoadMapDialogItem)GetWindowLong((HWND)lParam,
													 GWL_USERDATA);
				if (item == NULL) return FALSE;


				if (item->widget_type == ROADMAP_WIDGET_CHOICE) {
					i = SendMessage(item->w, (UINT) CB_GETCURSEL, (WPARAM) 0,
						(LPARAM) 0);
				} else if (item->widget_type == ROADMAP_WIDGET_LIST) {
					i = SendMessage(item->w, (UINT) LB_GETCURSEL, (WPARAM) 0,
						(LPARAM) 0);
				}

				if (i > -1) {
					roadmap_dialog_chosen(&item->choice[i]);
				}

				roadmap_dialog_action(item);

				return TRUE;
		}
      
      return FALSE;

	case WM_CLOSE:
		EndDialog(hDlg, message);
		return TRUE;

/*
   case WM_ERASEBKGND:
      {
		   HDC hdc = (HDC)wParam;
		   HBRUSH brush = CreateSolidBrush (RGB(255,255,224));
         RECT rc;

		   GetClientRect(hDlg, &rc);
		   FillRect(hdc, &rc, brush);

		   DeleteObject(brush);
		   return TRUE;
      }
*/
	}

	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK TabDialogFunc(HWND hDlg, UINT message, WPARAM wParam,
							   LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			PROPSHEETPAGE *psp;
			RoadMapDialogItem frame;
         RECT rc;
#ifdef UNDER_CE
			SHINITDLGINFO sid;

			sid.dwMask = SHIDIM_FLAGS;  /* This is the only allowed value. */
			sid.dwFlags = 0;//SHIDIF_SIZEDLGFULLSCREEN;  /* Make DB full screen. */
			sid.hDlg = hDlg; 

			//SHInitDialog(&sid);
#endif
			psp = (PROPSHEETPAGE*) lParam;
			frame = (RoadMapDialogItem)psp->lParam;
			frame->w = hDlg;
			SetWindowLong(hDlg, GWL_USERDATA, (LONG)frame);
			CreateControllers(hDlg, frame);

			GetWindowRect(GetParent(GetParent(hDlg)), &rc);

/*         SetWindowPos(hDlg, NULL,
            0, 0,
            rc.right - rc.left,
		      rc.bottom - rc.top,
		      SWP_NOZORDER);*/

/*			MoveWindow(hDlg, 0, 0,
				rc.right-rc.left+1,
            rc.bottom-rc.top+1,
				TRUE);*/
		}
		return (INT_PTR)TRUE;

#ifdef UNDER_CE
	case WM_SETTINGCHANGE:
		{
			SIPINFO si;
			RECT tab;
			HWND hmbar;
			RoadMapDialogItem frame;
			int w, h;
			GetWindowRect(hDlg,&tab);

			SHSipInfo(SPI_GETSIPINFO,lParam,&si,0);
			w = tab.right - tab.left + 1;
			h = si.rcVisibleDesktop.bottom-tab.top + 1;
			frame = (RoadMapDialogItem)GetWindowLong(hDlg, GWL_USERDATA);
			if (frame == NULL) break;
			hmbar = SHFindMenuBar(RoadMapMainWindow);
			if (hmbar != NULL) {
				RECT rc;
				int mh;

				GetWindowRect(hmbar,&rc);
				mh=rc.bottom-rc.top;
				if ((si.fdwFlags&SIPF_ON)) h+=1; else h-=mh-1;
			}
			lParam = MAKELPARAM(w, h);
			wParam = 0;
		}
#endif
		/* fall through to resize */
	case WM_SIZE:
		{
         RECT tab;
			RoadMapDialogItem frame = (RoadMapDialogItem)GetWindowLong(hDlg, GWL_USERDATA);
         int width;
         int height;

			if (frame == NULL) break;

         GetClientRect(GetParent(GetParent(hDlg)), &tab);
         width = tab.right - tab.left;
         height = tab.bottom - tab.top - MAX_ROW_HEIGHT;
#ifndef UNDER_CE
         width -= 10;
         height -= 20;
#endif

         MoveControlls (hDlg, frame, width, height-MAX_ROW_HEIGHT);
		}
		return TRUE;

	case WM_COMMAND:
		if ((HIWORD(wParam) == BN_CLICKED) ||
			(HIWORD(wParam) == LBN_SELCHANGE) ||
         (HIWORD(wParam) == EN_CHANGE) ||
			(HIWORD(wParam) == CBN_SELCHANGE)) {
				int i = -1;
				RoadMapDialogItem item =
					(RoadMapDialogItem)GetWindowLong((HWND)lParam,
													 GWL_USERDATA);
				if (item == NULL) return FALSE;


				if (item->widget_type == ROADMAP_WIDGET_CHOICE) {
					i = SendMessage(item->w, (UINT) CB_GETCURSEL, (WPARAM) 0,
						(LPARAM) 0);
				} else if (item->widget_type == ROADMAP_WIDGET_LIST) {
					i = SendMessage(item->w, (UINT) LB_GETCURSEL, (WPARAM) 0,
						(LPARAM) 0);
				}

				if (i > -1) {
					roadmap_dialog_chosen(&item->choice[i]);
				}

				roadmap_dialog_action(item);

				return TRUE;
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg, message);
		return TRUE;
	}
	return (INT_PTR)FALSE;
}

