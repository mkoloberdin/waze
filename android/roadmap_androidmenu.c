/* roadmap_androidmenu.c - Android Menu functionality
 *
 * LICENSE:
 *
  *   Copyright 2009 Alex Agranovich (AGA)
 *
 *   This file is part of RoadMap.
 *
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
 */

#include <string.h>
#include <stdlib.h>
#include "roadmap_androidmenu.h"
#include "ssd/ssd_contextmenu_defs.h"
#include "roadmap.h"
#include "roadmap_file.h"
#include "roadmap_start.h"
#include "roadmap_lang.h"
#include "roadmap_screen.h"
#include "JNI/FreeMapJNI.h"

extern void roadmap_search_menu( void );
extern void start_alerts_menu( void );
extern void show_me_on_map( void );
extern void RealtimeAlertsList( void );
extern void start_settings_quick_menu( void );


#define		ANDROID_MENU_MAX_LBL_SIZE			64
#define		ANDROID_MENU_MAX_ICON_NAME_SIZE		64

typedef struct
{
	int item_id;												// Id of the item in the menu
	char label[ANDROID_MENU_MAX_LBL_SIZE];				// label of item in the menu
	char icon_name[ANDROID_MENU_MAX_ICON_NAME_SIZE];		// Name of the icon file
	// 1 - icon is native(accessed through API), 0 - icon is of the application (file system)
	int is_icon_native;
	int portrait_order;									// The order of the item in the portrait mode
	int landscape_order;								// The order of the item in the landscape mode
	// 0 - regular, 1 - more button
	int item_type;
	const RoadMapAction* action;

} android_menu_item;


static android_menu_item sgMenuItems[ANDROID_MENU_MAX_ITEMS_NUM];
static int sgMenuItemsCount = 0;

#define GET_MENU_PARAM( dst, src, size ) \
{ \
	strncpy( dst, src, size );	\
    dst[size] = 0;	\
}

static void roadmap_androidmenu_load( const char* data, int size );
static void roadmap_androidmenu_update();
/*************************************************************************************************
 * roadmap_main_menu_handler()
 * Menu buttons press handler
 *
 */
void roadmap_androidmenu_initialize()
{
	const char* cursor;
	RoadMapFileContext file;

    // Loading the file
	cursor = roadmap_file_map( "skin", ANDROID_MENU_FILE_NAME, NULL, "r", &file );
	if (cursor == NULL)
	{
		roadmap_log ( ROADMAP_ERROR, "Menu file %s is missing", ANDROID_MENU_FILE_NAME );
		return;
	}

	// Parsing the file
	roadmap_androidmenu_load( roadmap_file_base( file ), roadmap_file_size( file ) );

	// Update the java layer
	roadmap_androidmenu_update();
}

static void roadmap_androidmenu_load( const char* data, int size )
{
	   const char *p;
	   const char *end;
	   int argc;
	   int argl[256];
	   const char *argv[256];
	   char param[64];

	   android_menu_item* item = NULL;
	   end  = data + size;

	   while (data < end) {

	      while (data[0] == '#' || data[0] < ' ') {

	         if (*(data++) == '#') {
	            while ((data < end) && (data[0] >= ' ')) data += 1;
	         }
	         while (data < end && data[0] == '\n' && data[0] != '\r') data += 1;
	      }

	      argc = 1;
	      argv[0] = data;

	      for (p = data; p < end; ++p) {

	         if (*p == ' ') {
	            argl[argc-1] = p - argv[argc-1];
	            argv[argc]   = p+1;
	            argc += 1;
	            if (argc >= 255) break;
	         }

	         if ( p >= end || *p < ' ' ) break;
	      }
	      argl[argc-1] = p - argv[argc-1];

	      while (p < end && *p < ' ' && *p > 0) ++p;
	      data = p;


	      if ( item == NULL && argv[0][0] != 'N' )
	      {
	    	  roadmap_log( ROADMAP_ERROR, "The menu item is missing" );
	    	  break;
	      }


	      switch (argv[0][0])
	      {
			  case 'N':
			  {
				  item = &sgMenuItems[sgMenuItemsCount++];
				  break;
			  }
			  case '!':
			  {
				  GET_MENU_PARAM( param, argv[1], argl[1] );
				  item->item_id = atoi( param );
				  break;
			  }
           case 'T':
           {
              GET_MENU_PARAM( param, argv[1], argl[1] );
              item->item_type = atoi( param );
              break;
           }
           case 'I':
			  {
				  GET_MENU_PARAM( item->icon_name, argv[1], argl[1] );
				  GET_MENU_PARAM( param, argv[2], argl[2] );
				  item->is_icon_native = atoi( param );
				  break;
			  }
			  case 'O':		// Order
			  {
				  GET_MENU_PARAM( param, argv[1], argl[1] );
				  item->portrait_order = atoi( param );
				  GET_MENU_PARAM( param, argv[2], argl[2] );
				  item->landscape_order = atoi( param );
				  break;
			  }
			  case 'L':
			  {
				  GET_MENU_PARAM( item->label, argv[1], argl[1] );
				  break;
			  }
			  case 'A':
			  {
				  GET_MENU_PARAM( param, argv[1], argl[1] );
				  item->action = roadmap_start_find_action( param );
				  break;
			  }

			  default:
			  {
		    	  roadmap_log( ROADMAP_ERROR, "Unknown attribute: %c", argv[0][0] );
		    	  break;
			  }
	      }
	      while (p < end && *p < ' ') p++;
	      data = p;
	   }
}

/*************************************************************************************************
 * Updates the Java layer with the menu
 *
 */
static void roadmap_androidmenu_update()
{
	int i;
	android_menu_item* item;

	for ( i=0; i < sgMenuItemsCount; ++i )
	{
		item = &sgMenuItems[i];
		WazeMenuManager_AddOptionsMenuItem( item->item_id, roadmap_lang_get( item->label ),
											item->icon_name, item->is_icon_native, item->portrait_order, item->landscape_order, item->item_type  );
	}
}


/*************************************************************************************************
 * roadmap_main_menu_handler()
 * Menu buttons press handler
 *
 */
void roadmap_androidmenu_handler( int item_id )
{
	const RoadMapAction* action;

	if ( item_id >= 0 && item_id < sgMenuItemsCount )
	{
		action = sgMenuItems[item_id].action;
		action->callback();
		roadmap_screen_redraw();
	}
	else
	{
		roadmap_log ( ROADMAP_ERROR, "Cannot handle the menu action. Illegal item id: %d", item_id );
	}

}
