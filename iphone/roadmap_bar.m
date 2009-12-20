/* roadmap|_bar.m - Handle main bar
 *
 * LICENSE:
 *
 *   Copyright 2008 Avi B.S
 *   Copyright 2008 Ehud Shabtai
 *   Copyright 2009 Avi R
 *   Copyright 2009, Waze Ltd
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
 */
 
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_canvas.h"
#include "roadmap_screen_obj.h"
#include "roadmap_screen.h"
#include "roadmap_file.h"
#include "roadmap_res.h"
#include "roadmap_math.h"
#include "roadmap_time.h"
#include "roadmap_state.h"
#include "roadmap_message.h"
#include "roadmap_bar.h"
#include "roadmap_factory.h" 
#include "roadmap_start.h"
#include "roadmap_pointer.h"
#include "roadmap_softkeys.h"
#include "roadmap_skin.h"
#include "roadmap_main.h"
#include "roadmap_path.h"
#include "roadmap_lang.h"
#include "roadmap_config.h"
#include "roadmap_map_settings.h"
#include "roadmap_iphoneimage.h"
#include "Realtime/RealtimeAlerts.h"

#include "roadmap_iphonebar.h"

#define MAX_STATES 30
#define TOP_BAR_IMAGE 		"top_bar_background" 
#define BOTTOM_BAR_IMAGE 	"bottom_bar_background"
#define BOTTOM_BUTTON		"toolbar_button_up"
#define BOTTOM_BUTTON_SEL	"toolbar_button_down"

#define Y_POS 5.0f
#define X_POS_START 65.0f
 

static RoadMapTopBarView *topBarView;
static RoadMapBottomBarView *bottomBarView;

void roadmap_bar_switch_skins(void);

static BOOL bar_initialized = FALSE;
static BOOL block_buttons = FALSE;
static BOOL gHideTopBar = -1;
static char gLastLang[20];

#define TOPBAR_FLAG 	0x10000
#define BOTTOM_BAR_FLAG 0x20000

#define IMAGE_STATE_NORMAL   0
#define IMAGE_STATE_SELECTED 1


const char * get_time_str(void);
const char * get_current_street(void);
const char * get_time_to_destination(void);
const char * get_dist_to_destination(void);
const char * get_info_with_num_alerts(void);

#define MAX_OBJECTS 40


typedef struct {

    const char *name;

    RoadMapPen foreground;
    
    const char *default_foreground;
    
    RoadMapBarTextFn bat_text_fn;

} BarText; 

#define ROADMAP_TEXT(p,w,t) \
    {p, NULL, w, t}
     
BarText RoadMapTexts[] = {

    ROADMAP_TEXT("clock", "#303030", get_time_str),
    ROADMAP_TEXT("num_alerts", "#303030",RTAlerts_Count_Str),
    ROADMAP_TEXT("left_softkey", "#303030",roadmap_softkeys_get_left_soft_key_text),
    ROADMAP_TEXT("right_softkey", "#303030",roadmap_softkeys_get_right_soft_key_text),
    ROADMAP_TEXT("right_softkey", "#303030",get_current_street),
    ROADMAP_TEXT("time_to_dest", "#303030",get_time_to_destination),
    ROADMAP_TEXT("dist_to_dest", "#303030",get_dist_to_destination),
    ROADMAP_TEXT("info_with_num_alerts", "#303030",get_info_with_num_alerts),
    ROADMAP_TEXT(NULL, NULL, NULL)
};

typedef struct {
	
    const char *name;
	
	const char *title;
	
} BarTitle; 

BarTitle RoadMapTitles[] = {
	{"search", "Drive to"},
	{"live_info", "Events"},
	{"report", "Report"},
	{NULL, NULL}
};

typedef struct {
   char           *name;
   const char     *image_file[MAX_STATES];
   int 			  pos_x;
   int 			  pos_y;	
	UIImage			*images[MAX_STATES];
	UIImage			*image_selected[MAX_STATES];
   UIButton       *button;
	UILabel			*label;
   const char     *state;
   int            states_count;		
   RoadMapStateFn state_fn;
   BarText		  *bar_text;
   int			  font_size;
   int 			  text_alling;
   char 		  *text_color;
   RoadMapGuiRect bbox;
   const RoadMapAction *action;
   RoadMapStateFn  condition_fn;
   int			   condition_value;
   int 			   image_state;
} BarObject;

typedef struct {
	BarObject *object[MAX_OBJECTS];
	int		   draw_bg;
	int		   count;
} BarObjectTable_s;


static BarObjectTable_s TopBarObjectTable;
static BarObjectTable_s BottomBarObjectTable;


static BarText * roadmap_bar_text_find(const char *name){
   BarText *text;

    for (text = RoadMapTexts; text->name != NULL; ++text) {
        if (strcmp (text->name, name) == 0) {
            return text;
        }
    }
    roadmap_log (ROADMAP_ERROR, "unknown bar text '%s'", name);
    return NULL;
	
}

static void roadmap_bar_pos (BarObject *object,
                             RoadMapGuiPoint *pos) {

   pos->x = object->pos_x;
   pos->y = object->pos_y;

   if (pos->x < 0) 
      pos->x += topBarView.bounds.size.width;

   if (pos->y < 0) 
      pos->y += topBarView.bounds.size.height;
}

static char *roadmap_bar_object_string (const char *data, int length) {

    char *p = malloc (length+1);

    roadmap_check_allocated(p);

    strncpy (p, data, length);
    p[length] = 0;

    return p;
}

static void roadmap_bar_decode_arg (char *output, int o_size,
                                           const char *input, int i_size) {

   int size;

   o_size -= 1;
      
   size = i_size < o_size ? i_size : o_size;

   strncpy (output, input, size);
   output[size] = '\0';
}


static void roadmap_bar_decode_integer (int *value, int argc,
                                               const char **argv, int *argl) {

   char arg[255];

   argc -= 1;

   if (argc != 1) {
      roadmap_log (ROADMAP_ERROR, "roadmap bar: illegal integer value - %s",
                  argv[0]);
      return;
   }

   roadmap_bar_decode_arg (arg, sizeof(arg), argv[1], argl[1]);
   *value = atoi(arg);
}  
      
static void roadmap_bar_decode_position
                        (BarObject *object,
                         int argc, const char **argv, int *argl) {

   char arg[255];
   int pos;

   argc -= 1;
   if (argc != 2) {
      roadmap_log (ROADMAP_ERROR, "roadmap bar:'%s' illegal position.",
                  object->name);
      return;
   }

   roadmap_bar_decode_arg (arg, sizeof(arg), argv[1], argl[1]);
   pos = atoi(arg);
   object->pos_x = pos;

   roadmap_bar_decode_arg (arg, sizeof(arg), argv[2], argl[2]);
   pos = atoi(arg);
   object->pos_y = pos;
}

static void roadmap_bar_decode_icon
                        (BarObject *object,
                         int argc, const char **argv, int *argl) {

   int i;

   argc -= 1;

   if (object->states_count > MAX_STATES) {
      roadmap_log (ROADMAP_ERROR, "roadmap bar:'%s' has too many states.",
                  object->name);
      return;
   }

   for (i = 1; i <= argc; ++i) {
      char arg[256];
      
      roadmap_bar_decode_arg (arg, sizeof(arg), argv[i], argl[i]);

	   UIImage *image = roadmap_iphoneimage_load(arg);
	  
      if (image == NULL) {
            roadmap_log (ROADMAP_ERROR,
                  "roadmap bar:'%s' can't load image:%s.",
                  object->name,
                  arg);
      }
      else
      	if (i == 1)
       		 object->images[object->states_count] = image;
        else
        	object->image_selected[object->states_count] = image;
   }

   object->image_state = IMAGE_STATE_NORMAL;
   object->states_count++;
}


static void roadmap_bar_decode_state
                        (BarObject *object,
                         int argc, const char **argv, int *argl) {

   char arg[255];

   argc -= 1;
   if (argc != 1) {
      roadmap_log (ROADMAP_ERROR, "roadmap bar:'%s' illegal state indicator.",
                  object->name);
      return;
   }

   roadmap_bar_decode_arg (arg, sizeof(arg), argv[1], argl[1]);

   object->state_fn = roadmap_state_find (arg);

   if (!object->state_fn) {
      roadmap_log (ROADMAP_ERROR,
                  "roadmap bar:'%s' can't find state indicator.",
                  object->name);
   }
}


static void roadmap_bar_decode_text_color
                        (BarObject *object,
                         int argc, const char **argv, int *argl) {

   char arg[255];

   argc -= 1;
   if (argc != 1) {
      roadmap_log (ROADMAP_ERROR, "roadmap bar:'%s' illegal text color.",
                  object->name);
      return;
   }

   roadmap_bar_decode_arg (arg, sizeof(arg), argv[1], argl[1]);

   object->text_color = roadmap_bar_object_string (argv[1], argl[1]);
}
static void roadmap_bar_decode_condition
                        (BarObject *object,
                         int argc, const char **argv, int *argl) {

   char arg[255];

   argc -= 1;
   if (argc != 2) {
      roadmap_log (ROADMAP_ERROR, "roadmap bar:'%s' illegal condition indicator.",
                  object->name);
      return;
   }

   roadmap_bar_decode_arg (arg, sizeof(arg), argv[1], argl[1]);

   object->condition_fn = roadmap_state_find (arg);

   if (!object->condition_fn) {
      roadmap_log (ROADMAP_ERROR,
                  "roadmap bar:'%s' can't find condition indicator.",
                  object->name);
      return;
   }
   
   roadmap_bar_decode_arg (arg, sizeof(arg), argv[2], argl[2]);
   object->condition_value = atoi(arg);
}

static void roadmap_bar_decode_text
                        (BarObject *object,
                         int argc, const char **argv, int *argl) {

   char arg[255];

   argc -= 1;
   if (argc != 1) {
      roadmap_log (ROADMAP_ERROR, "roadmap bar:'%s' illegal condition indicator.",
                  object->name);
      return;
   }

   roadmap_bar_decode_arg (arg, sizeof(arg), argv[1], argl[1]);

   object->bar_text = roadmap_bar_text_find (arg);

}

static BarObject *roadmap_bar_new
          (int argc, const char **argv, int *argl) {
             
             BarObject *object = calloc(sizeof(*object), 1);
             
             roadmap_check_allocated(object);
             
             object->name = roadmap_bar_object_string (argv[1], argl[1]);
             object->text_alling = -1;
             object->text_color = NULL;
             object->pos_x	= -100;
             object->pos_y	= -100;
             
             
             
   return object;
}

static void roadmap_bar_decode_bbox
                        (BarObject *object,
                         int argc, const char **argv, int *argl) {

   char arg[255];

   argc -= 1;
   if (argc != 4) {
      roadmap_log (ROADMAP_ERROR, "roadmap bar:'%s' illegal bbox.",
                  object->name);
      return;
   }

   roadmap_bar_decode_arg (arg, sizeof(arg), argv[1], argl[1]);
   object->bbox.minx = atoi(arg);
   roadmap_bar_decode_arg (arg, sizeof(arg), argv[2], argl[2]);
   object->bbox.miny = atoi(arg);
   roadmap_bar_decode_arg (arg, sizeof(arg), argv[3], argl[3]);
   object->bbox.maxx = atoi(arg);
   roadmap_bar_decode_arg (arg, sizeof(arg), argv[4], argl[4]);
   object->bbox.maxy = atoi(arg);
}

static void roadmap_bar_decode_action
                        (BarObject *object,
                         int argc, const char **argv, int *argl) {

   char arg[255];

   argc -= 1;
   if (argc < 1) {
      roadmap_log (ROADMAP_ERROR, "roadmap bar:'%s' illegal action.",
                  object->name);
      return;
   }

   roadmap_bar_decode_arg (arg, sizeof(arg), argv[1], argl[1]);

   object->action = roadmap_start_find_action (arg);

   if (!object->action) {
      roadmap_log (ROADMAP_ERROR, "roadmap bar:'%s' can't find action.",
                  object->name);
   }
}


static void roadmap_bar_load (const char *data, int size, BarObjectTable_s *BarObjectTable) {

   int argc;
   int argl[256];
   const char *argv[256];

   const char *p;
   const char *end;

   BarObject *object = NULL;


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

         if (p >= end || *p < ' ') break;
      }
      argl[argc-1] = p - argv[argc-1];

      while (p < end && *p < ' ' && *p > 0) ++p;
      data = p;

      if (object == NULL) {

         if (argv[0][0] != 'N') {
            roadmap_log (ROADMAP_ERROR, "roadmap bar name is missing");
            break;
         }

      }

      switch (argv[0][0]) {

      case 'P':

         roadmap_bar_decode_position (object, argc, argv, argl);
         break;

      case 'I':

         roadmap_bar_decode_icon (object, argc, argv, argl);
         break;

     case 'A':

         roadmap_bar_decode_action (object, argc, argv, argl);
         break;

      case 'B':

         roadmap_bar_decode_bbox (object, argc, argv, argl);
         break;

      case 'S':

         roadmap_bar_decode_state (object, argc, argv, argl);
         break;

      case 'N':
         object = roadmap_bar_new (argc, argv, argl);
         
         BarObjectTable->object[BarObjectTable->count] = object;
         BarObjectTable->count++;
         break;
      case 'C':
      	 roadmap_bar_decode_condition (object, argc, argv, argl);
         break;
           
      case 'T':
      	roadmap_bar_decode_text (object, argc, argv, argl);
      	break;
      	
      case 'F':
      	roadmap_bar_decode_integer (&object->font_size, argc, argv,
                                             argl);
        break;
        
     case 'L':
     	roadmap_bar_decode_integer(&object->text_alling, argc, argv,
                                             argl);   
        break;
     case 'D':
      	roadmap_bar_decode_text_color (object, argc, argv, argl);
        break;     
        
	  case 'Y':
      	roadmap_bar_decode_integer (&BarObjectTable->draw_bg, argc, argv,
                                             argl);
        break;	
      }

	  
      while (p < end && *p < ' ') p++;
      data = p;
   }
}


static void roadmap_top_bar_set_ui (BarObjectTable_s *BarObjectTable) {
	int i;
	int font_size;
	RoadMapGuiPoint TextLocation;
   UIButton *button;
   
	for (i=0; i < BarObjectTable->count; i++) {
    	RoadMapGuiPoint ObjectLocation;
		UILabel *label = NULL;
		BarObject *object = BarObjectTable->object[i];
		
    	if (object == NULL)
    		continue;
		
		if (object->images[0] != NULL) {
         button = [UIButton buttonWithType:UIButtonTypeCustom];
         [button setTag:i];
         [button setImage: object->images[0] forState: UIControlStateNormal];
         if (object->image_selected[0] != NULL) {
            [button setImage: object->image_selected[0] forState: UIControlStateHighlighted];
         } else if (object->action) {
            [button setShowsTouchWhenHighlighted: YES];
         }
         
         roadmap_bar_pos(object, &ObjectLocation);
         CGRect rect = [button frame];
         rect.origin.x = ObjectLocation.x;
         rect.origin.y = ObjectLocation.y;
         rect.size = [object->images[0] size];
         
         [button setFrame: rect];
         
         if (object->action) {
				[button addTarget:topBarView action:@selector(buttonEvent:) forControlEvents: UIControlEventTouchUpInside];
         }
         
         object->button = button;
         [topBarView addSubview:object->button];
		} else if (object->image_selected[0] != NULL) {
         button = [UIButton buttonWithType:UIButtonTypeCustom];
         [button setTag:i];
         [button setImage: object->image_selected[0] forState: UIControlStateNormal];
         object->button = button;
         [topBarView addSubview:object->button];
      }
		
		roadmap_bar_pos(object, &ObjectLocation);
		CGRect rect = [object->button frame];
		rect.origin.x = ObjectLocation.x;
		rect.origin.y = ObjectLocation.y;
		[object->button setFrame:rect];
		
		if (object->bar_text) {
			
			if (object->font_size != 0) 
				font_size = object->font_size;
			else
				font_size = 10;

			
			roadmap_bar_pos(object, &TextLocation);
			
			rect.origin.x = TextLocation.x;
			rect.origin.y = TextLocation.y;
			rect.size.width = 50.0f;
			rect.size.height = 20.0f;
			label = [[UILabel alloc] initWithFrame:rect];
			[label setFont: [UIFont systemFontOfSize:font_size]];
			[label setTextColor: [UIColor whiteColor]]; //TODO: allow variable color
			[label setBackgroundColor: [UIColor clearColor]];
			
			object->label = label;
			[topBarView addSubview: object->label];
		}           		    	                
	}
}


static void roadmap_bottom_bar_set_ui  (BarObjectTable_s *BarObjectTable) {
	int i;
	int font_size;
	NSMutableArray *buttonArray = [NSMutableArray arrayWithCapacity:0];
	UIBarButtonItem *flexSpacer = [[UIBarButtonItem alloc] initWithBarButtonSystemItem: UIBarButtonSystemItemFlexibleSpace
																				target:nil action:nil]; 

	UIImage *img = roadmap_iphoneimage_load(BOTTOM_BUTTON);
	UIImage *buttonImage;
	if (img) {
		buttonImage = [img stretchableImageWithLeftCapWidth:5 topCapHeight:0];
		[img release];
	}

	img = roadmap_iphoneimage_load(BOTTOM_BUTTON_SEL);
	UIImage *buttonImageSel = [img stretchableImageWithLeftCapWidth:5 topCapHeight:0];
	[img release];
   
	
	for (i=0; i < BarObjectTable->count; i++) {

		BarObject *object = BarObjectTable->object[i];
		
    	if (object == NULL)
    		continue;
		
		if (object->images[0] != NULL) {
			if (object->action) {
				UIButton *button = [UIButton buttonWithType:UIButtonTypeCustom];
				[button setTag:i];
				[button setImage: object->images[0] forState: UIControlStateNormal];
				if (object->image_selected[0] != NULL) {
					[button setImage: object->image_selected[0] forState: UIControlStateHighlighted];
				} else {
					[button setShowsTouchWhenHighlighted: YES];
				}
				
				[button sizeToFit];
            
				[button addTarget:bottomBarView action:@selector(buttonEvent:) forControlEvents: UIControlEventTouchUpInside];
				UIBarButtonItem *barButton = [[UIBarButtonItem alloc] initWithCustomView:button];
            
            object->button = button;
				
				[buttonArray addObject:barButton];
				[buttonArray addObject:flexSpacer];
			}
		} else {
         NSString *title = NULL;
         
			if (object->font_size != 0) 
				font_size = object->font_size;
			else
				font_size = 10;
			
         
         if (object->bar_text) {
            title = [NSString stringWithUTF8String: object->bar_text->bat_text_fn()];
         } else {
            int index = 0;
            
            while (RoadMapTitles[index].name != NULL) {
               if (strcmp (RoadMapTitles[index].name, object->name) == 0) {
                  title = [NSString stringWithUTF8String: roadmap_lang_get(RoadMapTitles[index].title)];
                  break;
               }
               index++;
            }
            
            if (!title)
               title = [NSString stringWithUTF8String: roadmap_lang_get(object->name)];
         }
			
			UIButton *button = [UIButton buttonWithType:UIButtonTypeCustom];
			
			[button setTag:i];
			[button setBackgroundImage: buttonImage forState: UIControlStateNormal];
			if (buttonImageSel != NULL) {
				[button setBackgroundImage: buttonImageSel forState: UIControlStateHighlighted];
			} else {
				[button setShowsTouchWhenHighlighted: YES];
			}
			
			[button setTitle:title forState:UIControlStateNormal];
         button.titleLabel.font = [UIFont systemFontOfSize:14.0f];
			[button sizeToFit];
         
			[button addTarget:bottomBarView action:@selector(buttonEvent:) forControlEvents: UIControlEventTouchUpInside];
			UIBarButtonItem *barButton = [[UIBarButtonItem alloc] initWithCustomView:button];
         
         object->button = button;
			
			[buttonArray addObject:barButton];
			[buttonArray addObject:flexSpacer];
		}           		    	                
	}
	
	[buttonArray removeLastObject];
	[bottomBarView setItems: buttonArray];
   
   strncpy_safe (gLastLang, roadmap_lang_get_system_lang(), sizeof(gLastLang));
}


	
const char * get_time_str(void){
	time_t now;
	now = time(NULL);
	return roadmap_time_get_hours_minutes(now);
}

const char * get_current_street(void){
	static char text[256];
	roadmap_message_format (text, sizeof(text), "%Y");
	return &text[0];
	
}


const char *get_time_to_destination(void){
  static char text[256];

  	if (roadmap_message_format (text, sizeof(text), "%A|%T"))
  		return &text[0];
  	return "";
}
  	
const char *get_dist_to_destination(void){
  static char text[256];

  	if (roadmap_message_format (text, sizeof(text), "%D (%W)|%D"))
  		return &text[0];
  	return "";
}

const char *get_info_with_num_alerts(void){
   static char text[256];
   
   snprintf(text, sizeof(text), "  %s (%s)  ",roadmap_lang_get("Live info"), RTAlerts_Count_Str());
   
   return &text[0];
}
  	
void draw_top_bar_objects(BarObjectTable_s *table){
   int i;
   int state, condition;
	int xPos = X_POS_START;
   RoadMapGuiPoint TextLocation;
	CGRect rect;
	static int first_time = 1;
   
   if (!topBarView)
      return;
   
	if (!first_time) {
		[UIView beginAnimations:NULL context:NULL];
		[UIView setAnimationDuration:0.3f];
		[UIView setAnimationCurve:UIViewAnimationCurveEaseOut];
	}
	
	for (i=0; i < table->count; i++) {
    	RoadMapGuiPoint ObjectLocation; 
    	if (table->object[i] == NULL)
    		continue;
      
		if (table->object[i]->button) {
			if (table->object[i]->condition_fn){
				condition = (*table->object[i]->condition_fn) ();
				if (condition !=  table->object[i]->condition_value)
					[table->object[i]->button setHidden:YES];
				else
					[table->object[i]->button setHidden:NO];
			}
			
			roadmap_bar_pos(table->object[i], &ObjectLocation);
			rect = [table->object[i]->button frame];
			if ((ObjectLocation.x >= 0) && (ObjectLocation.y >= 0)) {
				rect.origin.x = ObjectLocation.x;
				rect.origin.y = ObjectLocation.y;
			} else { //Auto position
				rect.origin.x = xPos;
				xPos += rect.size.width + 5;
				rect.origin.y = [topBarView bounds].size.height - Y_POS - rect.size.height;
			}
			[table->object[i]->button setFrame:rect];
			if (table->object[i]->state_fn) {
				[table->object[i]->button setHidden:NO];
				state = (*table->object[i]->state_fn) ();
				
				if ((state < 0) || (state >= MAX_STATES)){
				}
				else{
					//if (table->object[i]->action)
					//	continue;
					if (table->object[i]->images[state] != NULL){
						if ((table->object[i]->image_state == IMAGE_STATE_SELECTED) && (table->object[i]->image_selected[state] != NULL))
							[table->object[i]->button setImage:table->object[i]->image_selected[state] forState: UIControlStateNormal];
						else 
							[table->object[i]->button setImage:table->object[i]->images[state] forState: UIControlStateNormal];
					} else {
						[table->object[i]->button setHidden:YES];
					}
				}
			}
			else{
				if ((table->object[i]->image_state == IMAGE_STATE_SELECTED) && (table->object[i]->image_selected[0] != NULL))
					[table->object[i]->button setImage:table->object[i]->image_selected[0] forState: UIControlStateNormal];
				else if (table->object[i]->images[0])
					[table->object[i]->button setImage:table->object[i]->images[0] forState: UIControlStateNormal];
				else
					[table->object[i]->button setHidden:YES];
			}
         
         if ([table->object[i]->button isHidden]) {
            rect = [table->object[i]->button frame];
            xPos -= rect.size.width + 5;
         }
		}
		
		
    	if (table->object[i]->bar_text){
			
			[table->object[i]->label setText: [NSString stringWithUTF8String: (*table->object[i]->bar_text->bat_text_fn)()]];
			[table->object[i]->label sizeToFit];
			
			xPos -= 5; //TODO: fix this - distance from object before should be set in top_bar file
			roadmap_bar_pos(table->object[i], &TextLocation);
			CGRect rect = [table->object[i]->label frame];
			if ((TextLocation.x > 0) && (TextLocation.y > 0)) {
				rect.origin.x = TextLocation.x;
				rect.origin.y = TextLocation.y;
			} else { //Auto position
				rect.origin.x = xPos;
				xPos += rect.size.width + 5;
				rect.origin.y = [topBarView bounds].size.height - Y_POS - rect.size.height;
			}
			[table->object[i]->label setFrame:rect];

    	}           		    	                
    }
	
	if (first_time) {
		//first_time = 0;
	} else {
		[UIView commitAnimations];
	}
}

void roadmap_bar_draw_top_bar (BOOL draw_bg) {
		
	if (!bar_initialized)
		return;

   if ( gHideTopBar == 1)
      return;
   
	draw_top_bar_objects(&TopBarObjectTable);    
}

void draw_bottom_bar_objects(BarObjectTable_s *table){
   int i;
   NSString *title;
	
	for (i=0; i < table->count; i++) {
    	if (table->object[i] == NULL)
    		continue;
      
      title = NULL;
      
		if (table->object[i]->button &&
         table->object[i]->bar_text) {
         
         title = [NSString stringWithUTF8String: table->object[i]->bar_text->bat_text_fn()];
         [table->object[i]->button setTitle:title forState:UIControlStateNormal];
         [table->object[i]->button sizeToFit];
      } else if (strcmp(gLastLang, roadmap_lang_get_system_lang()) && (table->object[i]->images[0] == NULL)) {
         int index = 0;
         
         while (RoadMapTitles[index].name != NULL) {
            if (strcmp (RoadMapTitles[index].name, table->object[i]->name) == 0) {
               title = [NSString stringWithUTF8String: roadmap_lang_get(RoadMapTitles[index].title)];
               break;
            }
            index++;
         }
         
         if (!title)
            title = [NSString stringWithUTF8String: roadmap_lang_get(table->object[i]->name)];
         
         [table->object[i]->button setTitle:title forState:UIControlStateNormal];
         [table->object[i]->button sizeToFit];
      }
   }

   strncpy_safe (gLastLang, roadmap_lang_get_system_lang(), sizeof(gLastLang));
}

void roadmap_bar_draw_bottom_bar (BOOL draw_bg) {
   
	if (!bar_initialized)
		return;
   
	draw_bottom_bar_objects(&BottomBarObjectTable);    
}

BOOL roadmap_top_bar_shown(){
	return (gHideTopBar == 0);
}

void roadmap_top_bar_hide(){
   if (gHideTopBar == 1)
      return;
   
   gHideTopBar = 1;
   
   [topBarView hideView];
   
   //roadmap_main_layout();
}

void roadmap_top_bar_show(){
   if (gHideTopBar == 0)
      return;
   
   gHideTopBar = 0;
   
   [topBarView showView];
   
   //roadmap_main_layout();
}

void roadmap_bar_draw(void){
	if (!bar_initialized)
		return;
	
   if (roadmap_screen_show_top_bar()) 
      roadmap_top_bar_show();
   else
      roadmap_top_bar_hide();
   
   
	roadmap_bar_draw_top_bar(FALSE);
   roadmap_bar_draw_bottom_bar(FALSE);
}

static void roadmap_bar_after_refresh (void) {

    roadmap_bar_draw ();
    
}

void roadmap_bar_initialize(void){
	int width;
	int i;
	const char *cursor;
   RoadMapFileContext file;
   
   gLastLang[0] = 0;
   
	TopBarObjectTable.count = 0;
	
	for (i=0; i< MAX_OBJECTS;i++){
		TopBarObjectTable.object[i] = NULL;
		BottomBarObjectTable.object[i] = NULL;
	}
	TopBarObjectTable.draw_bg = TRUE;
	BottomBarObjectTable.draw_bg = TRUE;
	
	width = roadmap_canvas_width ();
   
	// Load top bar
	cursor = roadmap_file_map ("skin", "top_bar", NULL, "r", &file);
	if (cursor == NULL){
		roadmap_log (ROADMAP_ERROR, "roadmap bar top_bar file is missing");
		return;
	}
   
   roadmap_bar_load(roadmap_file_base(file), roadmap_file_size(file), &TopBarObjectTable);
	roadmap_top_bar_set_ui (&TopBarObjectTable);
   
   roadmap_file_unmap (&file);
   
   // Load bottom bar
   cursor = roadmap_file_map ("skin", "bottom_bar", NULL, "r", &file);
	if (cursor == NULL){
		roadmap_log (ROADMAP_ERROR, "roadmap bar bottom_bar file is missing");
		return;
	}
   
   roadmap_bar_load(roadmap_file_base(file), roadmap_file_size(file), &BottomBarObjectTable);
	roadmap_bottom_bar_set_ui (&BottomBarObjectTable);
   
   roadmap_file_unmap (&file);
   
	
	//prev_xxx = roadmap_screen_subscribe_after_refresh (roadmap_bar_after_refresh); //TODO: this should be changed - 
   // we don't need refresh each time the map is refreshed
   
	bar_initialized = TRUE;
    roadmap_main_layout();
}

int roadmap_bar_top_height(){
   int topBarHeight = 0;
   
   if (roadmap_map_settings_isShowTopBarOnTap() && roadmap_top_bar_shown())
      topBarHeight = topBarView.bounds.size.height;
   
   return topBarHeight;
}

int roadmap_bar_bottom_height(){
	return 0; //iPhone  - bar not part of canvas
}

void roadmap_bar_switch_skins(void){
}
				


//////////////////////////////////////////////////

UIView *roadmap_bar_create_top_bar () {
	struct CGRect rect;
	
   if (topBarView)
      return topBarView;
   
	rect = [[UIScreen mainScreen] bounds];
	topBarView = [[RoadMapTopBarView alloc] initWithFrame:rect];
   [topBarView setAutoresizesSubviews: YES];
   [topBarView setAutoresizingMask: UIViewAutoresizingFlexibleWidth];

	UIImage *topBarImage = roadmap_iphoneimage_load(TOP_BAR_IMAGE);
	
	if (topBarImage) {
      rect = topBarView.frame;
      rect.size.height = topBarImage.size.height;
      topBarView.frame = rect;
		UIImageView *topBarImageV = [[UIImageView alloc] initWithImage:
                                   [topBarImage stretchableImageWithLeftCapWidth:0 topCapHeight:0]];
      [topBarImage release];
		//position of bar bg
		rect = topBarView.bounds;
		[topBarImageV setFrame: rect];
      [topBarImageV setTag:1000];
		[topBarView addSubview:topBarImageV];
      [topBarImageV release];
	}
	return topBarView;
}


@implementation RoadMapTopBarView

- (void)layoutSubviews
{
   CGRect rect;
   UIImageView *topBarImageV = (UIImageView *)[self viewWithTag:1000];
   if (topBarImageV) {
      rect = topBarImageV.frame;
      rect.size.width = self.bounds.size.width;
      topBarImageV.frame = rect;
   }
}

- (void) buttonEvent: (id) sender {
   if (block_buttons)
      return;
   
	int index = [(UIButton*) sender tag];
	BarObject *object = TopBarObjectTable.object[index];
	
	(*(object->action->callback)) ();
}

- (void) showView
{
   CGRect rect = self.frame;
   
   //Animate top bar into screen
	[UIView beginAnimations:NULL context:NULL];
	rect.origin.y = 0;
	[UIView setAnimationDuration:0.3f];
	[UIView setAnimationCurve:UIViewAnimationCurveEaseInOut];
	[self setFrame:rect];
	[UIView commitAnimations];
}

- (void) hideView
{
   CGRect rect = self.frame;
   
   //Animate top bar out of screen
	[UIView beginAnimations:NULL context:NULL];
	rect.origin.y -= self.bounds.size.height;
	[UIView setAnimationDuration:0.3f];
	[UIView setAnimationCurve:UIViewAnimationCurveEaseInOut];
	[self setFrame:rect];
	[UIView commitAnimations];
}

@end


UIToolbar *roadmap_bar_create_bottom_bar (int height, int pos_y) {
	struct CGRect rect;
   
   if (bottomBarView)
      return bottomBarView;
		
	rect=[UIScreen mainScreen].applicationFrame;
	rect.origin.y = pos_y;
	rect.size.height = height;
	
	bottomBarView = [[RoadMapBottomBarView alloc] initWithFrame:rect];
   [bottomBarView setAutoresizesSubviews: YES];
   [bottomBarView setAutoresizingMask: UIViewAutoresizingFlexibleWidth];
	
	return bottomBarView;
}


@implementation RoadMapBottomBarView

- (void) buttonEvent: (id) sender {
   if (block_buttons)
      return;
   
	int index = [(UIBarButtonItem*) sender tag];
	BarObject *object = BottomBarObjectTable.object[index];
	
	(*(object->action->callback)) ();
}

@end