/* roadmap_display.c - Manage screen signs.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 *   See roadmap_display.h.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "roadmap.h"
#include "roadmap_lang.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"
#include "roadmap_math.h"
#include "roadmap_line.h"
#include "roadmap_street.h"
#include "roadmap_config.h"
#include "roadmap_canvas.h"
#include "roadmap_message.h"
#include "roadmap_sprite.h"
#include "roadmap_voice.h"
#include "roadmap_skin.h"
#include "roadmap_plugin.h"
#include "roadmap_square.h"
#include "roadmap_math.h"
#include "roadmap_res.h"
#include "roadmap_bar.h"
#include "roadmap_border.h"
#include "roadmap_ticker.h"

#include "navigate/navigate_bar.h"
#include "ssd/ssd_widget.h"

#include "roadmap_display.h"
#include "roadmap_device.h"


static char *RoadMapDisplayPage = NULL;

static RoadMapConfigDescriptor RoadMapConfigDisplayDuration =
                        ROADMAP_CONFIG_ITEM("Display", "Duration");

static RoadMapConfigDescriptor RoadMapConfigDisplayBottomRight =
                        ROADMAP_CONFIG_ITEM("Display", "Bottom Right");

static RoadMapConfigDescriptor RoadMapConfigDisplayBottomLeft =
                        ROADMAP_CONFIG_ITEM("Display", "Bottom Left");

static RoadMapConfigDescriptor RoadMapConfigDisplayTopRight =
                        ROADMAP_CONFIG_ITEM("Display", "Top Right");

static RoadMapConfigDescriptor RoadMapConfigConsoleBackground =
                        ROADMAP_CONFIG_ITEM("Console", "Background");

static RoadMapConfigDescriptor RoadMapConfigConsoleForeground =
                        ROADMAP_CONFIG_ITEM("Console", "Foreground");

static RoadMapConfigDescriptor RoadMapConfigActivityBackground =
                        ROADMAP_CONFIG_ITEM("Activity", "Background");

static RoadMapConfigDescriptor RoadMapConfigActivityForeground =
                        ROADMAP_CONFIG_ITEM("Activity", "Foreground");

static RoadMapConfigDescriptor RoadMapConfigWarningBackground =
                        ROADMAP_CONFIG_ITEM("Warning", "Background");

static RoadMapConfigDescriptor RoadMapConfigWarningForeground =
                        ROADMAP_CONFIG_ITEM("Warning", "Foreground");


static RoadMapPen RoadMapMessageContour;
static RoadMapPen RoadMapConsoleBackground;
static RoadMapPen RoadMapConsoleForeground;
static RoadMapPen RoadMapActivityBackground;
static RoadMapPen RoadMapActivityForeground;
static RoadMapPen RoadMapWarningBackground;
static RoadMapPen RoadMapWarningForeground;


#define SIGN_BOTTOM   0
#define SIGN_TOP      1
#define SIGN_CENTER   2

#define SIGN_TEXT	 0
#define SIGN_POP_UP  1
#define SIGN_IMAGE	 2

#define ROADMAP_CONSOLE_REGULAR  0
#define ROADMAP_CONSOLE_ACTIVITY 1
#define ROADMAP_CONSOLE_WARNING 2

typedef struct {

    const char *page;
    const char *title;
	int type;

    char *content;
    char *id;

    int    on_current_page;
    int    has_position;
    int    was_visible;
    time_t deadline;
    RoadMapPosition position;
    RoadMapPosition endpoint[2];

    RoadMapPen background;
    RoadMapPen foreground;

    PluginLine line;

    RoadMapConfigDescriptor format_descriptor;
    RoadMapConfigDescriptor background_descriptor;
    RoadMapConfigDescriptor foreground_descriptor;

    int         where;
    const char *default_format;
    const char *default_background;
    const char *default_foreground;
    char *image;
    PluginStreet street;
    int style;
    int pointer_type;
    int header_type;
} RoadMapSign;


#define ROADMAP_SIGN(p,n,z, w,t,b,f,a,c, y) \
    {p, n, z, NULL, NULL, 0, 0, 0, 0, {0, 0},{{0,0}, {0,0}}, NULL, NULL, \
   PLUGIN_LINE_NULL, \
        {n, "Text", 0, NULL}, \
        {n, "Background", 0, NULL}, \
        {n, "Foreground", 0, NULL}, \
     w, t, b, f, NULL, PLUGIN_STREET_NULL,a,c,y}


RoadMapSign RoadMapStreetSign[] = {

    ROADMAP_SIGN("NO SCREEN", "Current Street", SIGN_TEXT, SIGN_BOTTOM, "%N, %C|%N", "#698b69", "#ffffff",STYLE_NORMAL,POINTER_NONE, HEADER_NONE),
    ROADMAP_SIGN("GPS", "Approach", SIGN_TEXT, SIGN_TOP, "Approaching %N, %C|Approaching %N", "#bdd1e7", "#000000",STYLE_NORMAL,POINTER_NONE, HEADER_NONE),
    ROADMAP_SIGN(NULL, "Selected Street", SIGN_TEXT,SIGN_BOTTOM, "%F", "#e4f1f9", "#000000",STYLE_NORMAL,POINTER_NONE, HEADER_NONE),
    ROADMAP_SIGN(NULL, "Info",  SIGN_TEXT,SIGN_CENTER, NULL, "#ffff00", "#000000",STYLE_NORMAL,POINTER_NONE, HEADER_NONE),
    ROADMAP_SIGN(NULL, "Error", SIGN_TEXT,SIGN_CENTER, NULL, "#ff0000", "#ffffff",STYLE_NORMAL,POINTER_NONE, HEADER_NONE),
    ROADMAP_SIGN("GPS", "Driving Instruction", SIGN_TEXT,SIGN_TOP, "In %D %I", "#ff0000", "#ffffff",STYLE_NORMAL,POINTER_NONE, HEADER_NONE),
    ROADMAP_SIGN(NULL, "Approach Alert", SIGN_POP_UP,SIGN_TOP, "", "#e4f1f9", "#000000",STYLE_NORMAL,POINTER_NONE, HEADER_NONE),
	ROADMAP_SIGN(NULL, "Distance To Alert",  SIGN_TEXT, SIGN_TOP, NULL, "#FF0000", "#FFFFFF",STYLE_NORMAL,POINTER_NONE, HEADER_NONE),
	ROADMAP_SIGN(NULL, "Shortcuts",  SIGN_IMAGE, SIGN_TOP, NULL, "", "",STYLE_NORMAL,POINTER_NONE, HEADER_NONE),
	ROADMAP_SIGN(NULL, "Navigation list Pop Up", SIGN_POP_UP, SIGN_TOP, "", "#e4f1f9", "#FFFFFF",STYLE_NORMAL,POINTER_POSITION, HEADER_NONE),
    ROADMAP_SIGN(NULL, NULL,0, 0, NULL, NULL, NULL,STYLE_NORMAL,POINTER_NONE, HEADER_NONE)
};


static RoadMapPen roadmap_display_new_pen
                        (RoadMapConfigDescriptor * descriptor) {

    const char *color = roadmap_config_get (descriptor);

    if (strcasecmp (color, "#000000") != 0) {

        RoadMapPen pen;
        char pen_name[256];

        if (sizeof(pen_name) <
              strlen(descriptor->category) + strlen(descriptor->name) + 2) {
           roadmap_log(ROADMAP_FATAL,
                       "not enough space for pen name %s.%s\n",
                       descriptor->category,
                       descriptor->name);
        }
        strcpy (pen_name, descriptor->category);
        strcat (pen_name, ".");
        strcat (pen_name, descriptor->name);

        pen = roadmap_canvas_create_pen (pen_name);
        roadmap_canvas_set_foreground (color);

        return pen;
    }

    return RoadMapMessageContour;
}


static void roadmap_display_create_pens (void) {

    static int RoadMapDisplayPensCreated = 0;

    RoadMapSign *sign;


    if (RoadMapDisplayPensCreated) return;

    RoadMapDisplayPensCreated = 1;


    RoadMapMessageContour = roadmap_canvas_create_pen ("message.contour");
    roadmap_canvas_set_foreground ("#000000");

    RoadMapConsoleBackground =
        roadmap_display_new_pen (&RoadMapConfigConsoleBackground);

    RoadMapConsoleForeground =
        roadmap_display_new_pen (&RoadMapConfigConsoleForeground);

    RoadMapActivityBackground =
        roadmap_display_new_pen (&RoadMapConfigActivityBackground);

    RoadMapActivityForeground =
        roadmap_display_new_pen (&RoadMapConfigActivityForeground);

    RoadMapWarningBackground =
        roadmap_display_new_pen (&RoadMapConfigWarningBackground);

    RoadMapWarningForeground =
        roadmap_display_new_pen (&RoadMapConfigWarningForeground);


    for (sign = RoadMapStreetSign; sign->title != NULL; ++sign) {

        sign->background =
            roadmap_display_new_pen (&sign->background_descriptor);

        roadmap_canvas_set_opacity (230);

        sign->foreground =
            roadmap_display_new_pen (&sign->foreground_descriptor);
    }
}


static void roadmap_display_string
                (char *text, int lines, int height, RoadMapGuiPoint *position, int corner) {

    char *text_line = &text[0];
    char display_line[256];
    int width, ascent, descent;
    unsigned int pos;
    int text_size = -1;
    int offset = 0;

    //
    if  (strchr(text, '\n') != 0){

    	while (strchr(text_line, '\n')){
    		pos = strchr(text_line,'\n')-text_line;
    		if (pos >= sizeof(display_line))
    			pos = sizeof(display_line) - 1;
    		strncpy(display_line, text_line, pos);
    		display_line[pos]=0;
    		text_line = strchr(text_line, '\n') + 1;
	    	if (!strncmp("<h1>", display_line,strlen("<h1>"))){
	    		text_size = 18;
	    		offset = strlen("<h1>");
	    		roadmap_canvas_set_foreground("#ffffff");
	    	}
	    	else if (!strncmp("<h2>", display_line,strlen("<h2>"))){
	    		text_size = 16;
	    		offset = strlen("<h2>");
#ifdef TOUCH_SCREEN
				roadmap_canvas_set_foreground("#ffffff");
#else
	    		roadmap_canvas_set_foreground("#000000");
#endif

	    	}
	    	else if (!strncmp("<r1>", display_line,strlen("<b1>"))){
	    		text_size = 18;
	    		offset = strlen("<r1>");
	    		roadmap_canvas_set_foreground("#FF0000");
	    	}
	    	else if (!strncmp("<b1>", display_line,strlen("<b1>"))){
	    		text_size = 18;
	    		offset = strlen("<b1>");
				roadmap_canvas_set_foreground("#ffffff");
	    	}
	    	else if (!strncmp("<h3>", display_line,strlen("<h3>"))){
	    		text_size = -1;
	    		offset = strlen("<h3>");
	    		roadmap_canvas_set_foreground("#000000");
	    	}
	    	else{
	    		offset = 0;
	    		text_size = -1;
	    		roadmap_canvas_set_foreground("#000000");
	    	}

	    	roadmap_canvas_draw_string_size
        				(position,corner, text_size,display_line+offset);
        	roadmap_canvas_get_text_extents (display_line, text_size, &width, &ascent, &descent, NULL);

       		height = ascent + descent + 4;


        	position->y += height;
    	}

    	if (!strncmp("<h1>", text_line,strlen("<h1>"))){
	    		text_size = 18;
	    		offset = strlen("<h1>");
	    }
	    else if (!strncmp("<h3>", text_line,strlen("<h3>"))){
	    		text_size = 13;
	    		offset = strlen("<h3>");
	    }
	    else{
	    	offset = 0;
	    	text_size = -1;
	    }

	    roadmap_canvas_draw_string_size
        				(position,corner, text_size,text_line+offset);

    	return;
    }

    if (lines > 1) {

        /* There is more than one line of text to display:
         * find where to cut the text. We choose to cut at
         * a space, either before of after the string midpoint,
         * whichever end with the shortest chunks.
         */

        char *text_end = text_line + strlen(text_line);
        char *p1 = text_line + (strlen(text_line) / 2);
        char *p2 = p1;

        while (p1 > text_line) {
            if (*p1 == ' ') {
                break;
            }
            p1 -= 1;
        }
        while (p2 < text_end) {
            if (*p2 == ' ') {
                break;
            }
            p2 += 1;
        }
        if (text_end - p1 > p2 - text_line) {
            p1 = p2;
        }
        if (p1 > text_line) {

            char saved = *p1;
            *p1 = 0;

            roadmap_canvas_draw_string
                (position, corner, text_line);

            *p1 = saved;
            text_line = p1 + 1;
            position->y += height;
        }

    }

    roadmap_canvas_draw_string
        (position, corner, text_line);
}


static RoadMapSign *roadmap_display_search_sign (const char *title) {

    RoadMapSign *sign;

    for (sign = RoadMapStreetSign; sign->title != NULL; ++sign) {
        if (strcmp (sign->title, title) == 0) {
            return sign;
        }
    }
    roadmap_log (ROADMAP_ERROR, "unknown display sign '%s'", title);
    return NULL;
}


int roadmap_display_is_sign_active(const char *title) {
	time_t now = time(NULL);
	RoadMapSign *sign = roadmap_display_search_sign(title);
	if (sign && ((sign->deadline == -1) || (sign->deadline > now)))
		return TRUE;
	else
		return FALSE;
}

static void roadmap_display_highlight (const RoadMapPosition *position) {

    RoadMapGuiPoint point;

    roadmap_math_coordinate (position, &point);
    roadmap_math_rotate_coordinates (1, &point);
    roadmap_sprite_draw ("Highlight", &point, 0);
}


static void roadmap_display_sign (RoadMapSign *sign) {
    static const char *shadow_bg = "#606060";
    RoadMapPen shadow_pen;
    RoadMapGuiPoint points[7];
    RoadMapGuiPoint shadow_points[7];
    RoadMapGuiPoint text_position;
    int width, height, ascent, descent;
    int screen_width;
    int sign_width, sign_height, text_height;
    int count;
    int i;
    int lines;
    RoadMapGuiPoint icon_screen_point;
	RoadMapImage image;

    roadmap_log_push ("roadmap_display_sign");

    roadmap_canvas_get_text_extents
        (sign->content, -1, &width, &ascent, &descent, NULL);


    width += 8; /* Keep some room around the text. */

    height = roadmap_canvas_height();

    screen_width = roadmap_canvas_width();

    /* Check if the text fits into one line, or if we need to use
     * more than one.
     */
    if (width + 10 < screen_width) {
        sign_width = width;
        lines = 1;
    } else {
        sign_width = screen_width - 10;
        lines = 1 + ((width + 10) / screen_width);
    }

	for (i = 0; sign->content[i] != '\0'; i++)
		if (sign->content[i]=='\n')
			lines++;

    text_height = ascent + descent + 5;
    sign_height = lines * text_height + 5;

   // screen_width = roadmap_canvas_width();

    if (sign->has_position) {

        int visible = roadmap_math_point_is_visible (&sign->position);

        if (sign->was_visible && (! visible)) {
            sign->deadline = 0;
            roadmap_log_pop ();
            return;
        }
        sign->was_visible = visible;

        roadmap_math_coordinate (&sign->position, points);
        roadmap_math_rotate_coordinates (1, points);

        if (sign->where == SIGN_TOP) {

            points[1].x = 5 + (screen_width - sign_width) / 2;
            points[2].x = points[1].x - 5;
            points[4].x = (screen_width + sign_width) / 2;
            points[6].x = points[1].x + 10;

            text_position.x = points[2].x + 4;

        } else if (points[0].x < screen_width / 2) {

            points[1].x = 10;
            points[2].x = 5;
            points[4].x = sign_width + 5;
            points[6].x = 20;

            text_position.x = 9;

        } else {

            points[1].x = screen_width - 10;
            points[2].x = screen_width - 5;
            points[4].x = screen_width - sign_width - 5;
            points[6].x = screen_width - 20;

            text_position.x = points[4].x + 4;
        }
        points[3].x = points[2].x;
        points[5].x = points[4].x;


       if (sign->where == SIGN_TOP || (points[0].y > height / 2)) {
            points[1].y = sign_height + roadmap_bar_top_height() +roadmap_ticker_height()+5;
            if (points[0].y < points[1].y){
				points[0].x = points[1].x;
				points[0].y = points[1].y;
            }
            points[3].y = roadmap_bar_top_height() +roadmap_ticker_height()+5;

            text_position.y = roadmap_bar_top_height() + roadmap_ticker_height()+8;
        }
        else {

            points[1].y = height - sign_height - roadmap_bar_bottom_height() - 5;
            points[3].y = height - roadmap_bar_bottom_height() - 5;

            text_position.y = points[1].y + 3;
        }
        points[2].y = points[1].y;
        points[4].y = points[3].y;
        points[5].y = points[1].y;
        points[6].y = points[1].y;

        count = 7;

        roadmap_display_highlight (&sign->endpoint[0]);
        roadmap_display_highlight (&sign->endpoint[1]);

    } else {

        points[0].x = (screen_width - sign_width) / 2;
        points[1].x = (screen_width + sign_width) / 2;
        points[2].x = points[1].x;
        points[3].x = points[0].x;

        switch (sign->where)
        {
           case SIGN_BOTTOM:

              points[0].y = height - sign_height - roadmap_bar_bottom_height();
              break;

           case SIGN_TOP:
              points[0].y = roadmap_bar_top_height() +roadmap_ticker_height()+3;
              break;

           case SIGN_CENTER:

              points[0].y = (height - sign_height) / 2;
              break;
        }
        points[1].y = points[0].y;
        points[2].y = points[0].y + sign_height;
        points[3].y = points[2].y;

        text_position.x = points[0].x + 4;
        text_position.y = points[0].y + 3;

        count = 4;
    }

	for (i=0; i<count;i++){
		shadow_points[i].x = points[i].x + 3;
		shadow_points[i].y = points[i].y -  3;
	}

	shadow_pen = roadmap_canvas_create_pen ("pop_up_shadow");

   	roadmap_canvas_set_foreground(shadow_bg);
	roadmap_canvas_set_opacity(115);
    roadmap_canvas_draw_multiple_polygons (1, &count, shadow_points, 1, 0);

    roadmap_canvas_select_pen (sign->background);
    roadmap_canvas_set_opacity(255);
    roadmap_canvas_draw_multiple_polygons (1, &count, points, 1, 0);


    roadmap_canvas_select_pen (RoadMapMessageContour);
    roadmap_canvas_draw_multiple_polygons (1, &count, points, 0, 0);

    if (sign->image != NULL){
    		image =  (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, sign->image);
    		icon_screen_point.x =  points[3].x + 2;
            icon_screen_point.y = 	 points[3].y+sign_height/2 - roadmap_canvas_image_width(image)/2;
            roadmap_canvas_draw_image (image, &icon_screen_point,
            				                                  0, IMAGE_NORMAL);
    }

    roadmap_canvas_select_pen (sign->foreground);
    roadmap_display_string
        (sign->content, lines, text_height, &text_position, ROADMAP_CANVAS_TOPLEFT);

    roadmap_log_pop ();
}



void roadmap_display_image_sign(RoadMapSign *sign) {
	int screen_width;
	int screen_height;
	RoadMapGuiPoint position;
	RoadMapImage image;

	screen_width = roadmap_canvas_width();
	screen_height = roadmap_canvas_height();

	if (sign->where == SIGN_TOP){
		position.x = 3;
		position.y = roadmap_bar_top_height() + 3;
	}

	if (sign->image != NULL){
    		image =  (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, sign->image);
            roadmap_canvas_draw_image (image, &position, 0, IMAGE_NORMAL);
    }

}

void roadmap_display_sign_pop_up(RoadMapSign *sign) {

	RoadMapGuiPoint text_position;
	RoadMapGuiPoint top, bottom;
	int text_height = 0;
	RoadMapGuiPoint icon_screen_point;
    int width, ascent, descent;
	int lines = 1;
	int screen_width;
	int sign_width;
	int i;
	int allign = ROADMAP_CANVAS_TOPLEFT;

	if (ssd_widget_rtl (NULL))
		allign = ROADMAP_CANVAS_TOPRIGHT;


	roadmap_canvas_get_text_extents
        (sign->content, -1, &width, &ascent, &descent, NULL);

    text_height = ascent + descent + 5;

	screen_width = roadmap_canvas_width();


	for (i = 0; sign->content[i] != '\0'; i++)
		if (sign->content[i]=='\n')
			lines++;

	if (lines < 2)
		lines = 2;

	sign->deadline = -1;

	top.x = 1;
	top.y = roadmap_bar_top_height()+1;

	bottom.x = screen_width;


#ifdef TOUCH_SCREEN
	bottom.y = (lines) * 21 + top.y + 8;
	sign_width = roadmap_display_border(sign->style, sign->header_type, sign->pointer_type, &bottom, &top, "#d2dfef", &sign->position);
#else
	bottom.y = (lines) * 21 + top.y;
	sign_width = roadmap_display_border(sign->style, sign->header_type, sign->pointer_type, &bottom, &top, "#e4f1f9", &sign->position);
#endif
    if (sign->image != NULL){
    		RoadMapImage close;
    		RoadMapImage image =  (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, sign->image);
    		if (image){

    			if (ssd_widget_rtl (NULL))
    				icon_screen_point.x =  top.x + 12;
    			else
    				icon_screen_point.x =  top.x + sign_width - 12 - roadmap_canvas_image_width(image);
            	icon_screen_point.y = 	 bottom.y - roadmap_canvas_image_height(image) -12;
            	roadmap_canvas_draw_image (image, &icon_screen_point,
            					                                  0, IMAGE_NORMAL);
    		}

    		close =  (RoadMapImage) roadmap_res_get(RES_BITMAP, RES_SKIN, "rm_quit");
    		if (image){
    			icon_screen_point.y = top.y + 4;
    			if (ssd_widget_rtl (NULL))
    				icon_screen_point.x =  top.x + 2;
    			else
    				icon_screen_point.x =  top.x + sign_width - 12 - roadmap_canvas_image_width(close);
            	roadmap_canvas_draw_image (close, &icon_screen_point,
            					                                  0, IMAGE_NORMAL);
    		}

    }

	roadmap_canvas_select_pen (sign->foreground);
	if (ssd_widget_rtl (NULL))
		text_position.x = sign_width  - 10 ;
	else
		text_position.x = 10;

#ifdef TOUCH_SCREEN
	text_position.y =  roadmap_bar_top_height() + 5 ;
#else
	text_position.y =  roadmap_bar_top_height() + 9 ;
#endif

    roadmap_display_string
       (sign->content, lines, text_height, &text_position, allign);

}


void roadmap_display_page (const char *name) {

   RoadMapSign *sign;

   if (RoadMapDisplayPage != NULL) {
      free (RoadMapDisplayPage);
   }

   if (name == NULL) {

      RoadMapDisplayPage = NULL;

      for (sign = RoadMapStreetSign; sign->title != NULL; ++sign) {
         sign->on_current_page = 0;
      }

   } else {

      RoadMapDisplayPage = strdup(name);

      for (sign = RoadMapStreetSign; sign->title != NULL; ++sign) {

         if ((sign->page == NULL) ||
             (! strcmp (sign->page, RoadMapDisplayPage))) {
            sign->on_current_page = 1;
         } else {
            sign->on_current_page = 0;
         }
      }
   }
}


int roadmap_display_activate
        (const char *title,
         const PluginLine *line,
         const RoadMapPosition *position,
         PluginStreet *street) {

    int   street_has_changed;
    int   message_has_changed;
    const char *format;
    char  text[256];
    RoadMapSign *sign;
    PluginStreetProperties properties;

    roadmap_log_push ("roadmap_display_activate");

    sign = roadmap_display_search_sign (title);
    if (sign == NULL) {
        roadmap_log_pop ();
        return -1;
    }

    if (sign->format_descriptor.category == NULL) {
       return -1; /* This is not a sign: this is a text. */
    }
    format = roadmap_config_get (&sign->format_descriptor);

    street_has_changed = 0;

    if (!roadmap_plugin_same_line (&sign->line, line)) {

       roadmap_plugin_get_street (line, street);

        if (sign->id != NULL) {
            free (sign->id);
        }

        sign->id =
            strdup (roadmap_plugin_street_full_name (line));

        sign->line = *line;
        sign->was_visible = 0;

        if (!roadmap_plugin_same_street (street, &sign->street)) {
           sign->street = *street;
           street_has_changed = 1;
        }
    }


    roadmap_message_set ('F', sign->id);

    roadmap_plugin_get_street_properties (line, &properties, 0);

    roadmap_message_set ('#', properties.address);
    roadmap_message_set ('N', properties.street);
    //roadmap_message_set ('T', properties.street_t2s);
    roadmap_message_set ('C', properties.city);

    if (!strcmp(title, "Current Street")){
    	roadmap_message_set ('Y', properties.street);
    	roadmap_message_set ('Z', properties.city);
    }

    if (! roadmap_message_format (text, sizeof(text), roadmap_lang_get(format))) {
        roadmap_log_pop ();
        *street = sign->street;
        return 0;
    }
    message_has_changed =
        (sign->content == NULL || strcmp (sign->content, text) != 0);

    if (roadmap_config_get_integer (&RoadMapConfigDisplayDuration) == -1) {
       sign->deadline = -1;

    } else {

       sign->deadline =
           time(NULL)
               + roadmap_config_get_integer (&RoadMapConfigDisplayDuration);
    }


    if (street_has_changed) {
        roadmap_voice_announce (sign->title);
    }

    if (message_has_changed) {

        if (sign->content != NULL) {
            free (sign->content);
        }
        if (text[0] == 0) {
            sign->content = strdup("(this street has no name)");
        } else {
            sign->content = strdup (text);
        }
    }

	roadmap_plugin_line_from (line, &sign->endpoint[0]);
    roadmap_plugin_line_to (line, &sign->endpoint[1]);

    if (position == NULL) {
        sign->has_position = 0;
    } else {
        sign->has_position = 1;
        sign->position = *position;
    }

    roadmap_log_pop ();
    *street = sign->street;
    return 0;
}


void roadmap_display_update_points
        (const char *title,
         RoadMapPosition *from,
         RoadMapPosition *to) {


    RoadMapSign *sign = roadmap_display_search_sign (title);

    if (sign == NULL) {
        roadmap_log_pop ();
        return;
    }

    if (sign->format_descriptor.category == NULL) {
       return; /* This is not a sign: this is a text. */
    }

	 sign->endpoint[0] = *from;
    sign->endpoint[1] = *to;
}


int roadmap_display_pop_up
        (const char *title,
         const char *image,
         const RoadMapPosition *position,
         const char *format, ...) {

    char text[1024];
    va_list parameters;
    RoadMapSign *sign;

    roadmap_log_push ("roadmap_display_pop_up");

    sign = roadmap_display_search_sign (title);
    if (sign == NULL) {
        roadmap_log_pop ();
        return -1;
    }

    if (sign->format_descriptor.category == NULL) {
       roadmap_log_pop ();
       return -1; /* This is not a sign: this is a text. */
    }

   va_start(parameters, format);
   vsnprintf (text, sizeof(text), format, parameters);
   va_end(parameters);

   if (sign->content != NULL) {
      free (sign->content);
   }

   if (sign->image != NULL) {
      free (sign->image);
   }

   sign->content = strdup(text);
   sign->has_position = 1;
   if (position != NULL)
   		sign->position = *position;
   else{
   		sign->position.latitude = -1;
   		sign->position.longitude = -1;
   }

   sign->was_visible=0;
   if (roadmap_config_get_integer (&RoadMapConfigDisplayDuration) == -1) {
      sign->deadline = -1;

   } else {

      sign->deadline =
          time(NULL)
              + roadmap_config_get_integer (&RoadMapConfigDisplayDuration);
   }
   if (image != NULL)
	   sign->image = strdup(image);
   else
   	   sign->image = NULL;

    roadmap_log_pop ();
    return 0;
}

int roadmap_activate_image_sign(const char *title,
         				   		const char *image){
   RoadMapSign *sign;

   sign = roadmap_display_search_sign (title);
   if (sign == NULL) {
        return -1;
   }

   if (sign->type != SIGN_IMAGE){
      return -1;
   }

   if (roadmap_config_get_integer (&RoadMapConfigDisplayDuration) == -1) {
      sign->deadline = -1;

   } else {

      sign->deadline =
          time(NULL)
              + roadmap_config_get_integer (&RoadMapConfigDisplayDuration);
   }

   sign->image = strdup(image);

   sign->content = sign->image;
   return 0;
}


void roadmap_display_hide (const char *title) {

    RoadMapSign *sign;

    sign = roadmap_display_search_sign (title);
    if (sign != NULL) {
        sign->deadline = 0;
    }
}


void roadmap_display_show (const char *title) {

    RoadMapSign *sign;

    sign = roadmap_display_search_sign (title);
    if (sign != NULL) {
        sign->deadline = -1;
    }
}


static void roadmap_display_console_box
                (int type, int corner, const char *format) {

    char text[256];
    int count;
    int width, ascent, descent;
    int i;
    static const char *shadow_bg = "#606060";
    int warning_font_size = 13;
    int offset = 62;

#ifdef ANDROID
    warning_font_size = 15;
#endif


    RoadMapGuiPoint frame[4];
    RoadMapGuiPoint shadow_points[4];
    RoadMapPen shadow_pen;

#ifdef _WIN32
   offset = 33;
#endif

    count = 4;
#ifdef _WIN32
	offset = 33;
#endif

    if (! roadmap_message_format (text, sizeof(text), format)) {
        return;
    }
    if ( type == ROADMAP_CONSOLE_WARNING ){
        	roadmap_canvas_get_text_extents (text, warning_font_size, &width, &ascent, &descent, NULL);
        	while (width > (roadmap_canvas_width()-10)){
        	   warning_font_size--;
        	   roadmap_canvas_get_text_extents (text, warning_font_size, &width, &ascent, &descent, NULL);
        	}
    }
    else if (type == ROADMAP_CONSOLE_ACTIVITY ){
    	roadmap_canvas_get_text_extents (text, 12, &width, &ascent, &descent, NULL);
    }
    else
    {
    	roadmap_canvas_get_text_extents (text, 16, &width, &ascent, &descent, NULL);
    }

    if (type == ROADMAP_CONSOLE_ACTIVITY) {
#ifdef TOUCH_SCREEN
       if (roadmap_horizontal_screen_orientation())
   		frame[2].x = roadmap_canvas_width() - offset;
       else
#endif
          frame[2].x = roadmap_canvas_width() ;
   		frame[0].x = frame[2].x - width - 4;
    } else if (corner & ROADMAP_CANVAS_RIGHT) {
#ifdef TOUCH_SCREEN
       if (roadmap_horizontal_screen_orientation())
        frame[2].x = roadmap_canvas_width() - 5-offset;
       else
#endif
        frame[2].x = roadmap_canvas_width() - 5 ;
        frame[0].x = frame[2].x - width - 6;
    } else {
        frame[0].x = 5;
        frame[2].x = frame[0].x + width + 6;
    }
    frame[1].x = frame[0].x;
    frame[3].x = frame[2].x;

    if (corner & ROADMAP_CANVAS_BOTTOM) {
        frame[0].y = roadmap_canvas_height () - ascent - descent - 11 - 22;
        frame[1].y = roadmap_canvas_height () - 6 - 22;
    } else {
       if ( type == ROADMAP_CONSOLE_ACTIVITY || type == ROADMAP_CONSOLE_WARNING )
       {
    	   frame[0].y = roadmap_bar_top_height() + 3;
       }
       else
       {
          frame[0].y = 40;
       }

       frame[1].y = ascent + descent + frame[0].y + 5;
    }
    frame[2].y = frame[1].y;
    frame[3].y = frame[0].y;

    for (i=0; i<count;i++){
    	shadow_points[i].x = frame[i].x + 3;
    	shadow_points[i].y = frame[i].y -  3;
    }

    shadow_pen = roadmap_canvas_create_pen ("pop_up_shadow");
	if (type != ROADMAP_CONSOLE_ACTIVITY){
    roadmap_canvas_set_foreground(shadow_bg);
	roadmap_canvas_set_opacity(115);
    roadmap_canvas_draw_multiple_polygons (1, &count, shadow_points, 1, 0);
	}

    count = 4;

    if (type == ROADMAP_CONSOLE_WARNING ) {
    	roadmap_canvas_select_pen (RoadMapWarningBackground);
    }
    else if (type == ROADMAP_CONSOLE_ACTIVITY) {
       roadmap_canvas_select_pen (RoadMapActivityBackground);
    } else {
       roadmap_canvas_select_pen (RoadMapConsoleBackground);
    }

   roadmap_canvas_draw_multiple_polygons (1, &count, frame, 1, 0);
  	if (type == ROADMAP_CONSOLE_WARNING) {
  		roadmap_canvas_select_pen (RoadMapWarningForeground);
  	}
  	else if (type == ROADMAP_CONSOLE_ACTIVITY) {
      roadmap_canvas_select_pen (RoadMapActivityForeground);
    } else {
       roadmap_canvas_select_pen (RoadMapConsoleForeground);
    }

   if (type != ROADMAP_CONSOLE_WARNING )
    roadmap_canvas_draw_multiple_polygons (1, &count, frame, 0, 0);

    frame[0].x = frame[3].x - 3;
    frame[0].y = frame[3].y + 3;
    if (type == ROADMAP_CONSOLE_WARNING )
    {
    	roadmap_canvas_draw_string_size (frame,
    	        	    	                    ROADMAP_CANVAS_RIGHT|ROADMAP_CANVAS_TOP,
    	        	    	                warning_font_size,text);
    }
    else if (type == ROADMAP_CONSOLE_ACTIVITY)
    	roadmap_canvas_draw_string_size (frame,
        	    	                    ROADMAP_CANVAS_RIGHT|ROADMAP_CANVAS_TOP,
            	    	                12,text);
	else
	    roadmap_canvas_draw_string (frame,
    	                            ROADMAP_CANVAS_RIGHT|ROADMAP_CANVAS_TOP,
        	                        text);
}


void roadmap_display_text (const char *title, const char *format, ...) {

   RoadMapSign *sign = roadmap_display_search_sign (title);

   char text[1024];
   va_list parameters;

   va_start(parameters, format);
   vsnprintf (text, sizeof(text), format, parameters);
   va_end(parameters);

   if (sign->content != NULL) {
      free (sign->content);
   }
   sign->content = strdup(text);

   if (roadmap_config_get_integer (&RoadMapConfigDisplayDuration) == -1) {
      sign->deadline = -1;

   } else {
      sign->deadline =
         time(NULL) +
         roadmap_config_get_integer (&RoadMapConfigDisplayDuration);
   }
}


void roadmap_display_signs (void) {

    time_t now = time(NULL);
    RoadMapSign *sign;


    roadmap_display_create_pens ();

    if ( roadmap_message_is_set( 'w' ) )
    {
		/* Run over the console if warning exists *** AGA *** */
		roadmap_display_console_box( ROADMAP_CONSOLE_WARNING,
			 ROADMAP_CANVAS_RIGHT, "%w");
    }
    else
    {
    	roadmap_display_console_box
    	        (ROADMAP_CONSOLE_ACTIVITY,
    	         ROADMAP_CANVAS_RIGHT, "%!");
    }

    for (sign = RoadMapStreetSign; sign->title != NULL; ++sign) {

        if ((sign->page == NULL) ||
            (RoadMapDisplayPage == NULL) ||
            (! strcmp (sign->page, RoadMapDisplayPage))) {

           if ( ((sign->deadline == -1) || (sign->deadline > now))
                     && sign->content != NULL) {

				if (sign->type == SIGN_POP_UP)
					roadmap_display_sign_pop_up (sign);
				else if (sign->type == SIGN_IMAGE)
					roadmap_display_image_sign(sign);
                else
               		roadmap_display_sign (sign);

           }
        }
    }
}


const char *roadmap_display_get_id (const char *title) {

    RoadMapSign *sign = roadmap_display_search_sign (title);

    if (sign == NULL || (! sign->has_position)) return NULL;

    return sign->id;
}


void roadmap_display_initialize (void) {

    RoadMapSign *sign;

    roadmap_config_declare
        ("preferences", &RoadMapConfigDisplayDuration, "5", NULL);
    roadmap_config_declare
        ("preferences", &RoadMapConfigDisplayBottomRight, "%D (%W)|%D", NULL);
    roadmap_config_declare
        ("preferences", &RoadMapConfigDisplayBottomLeft, "%S", NULL);
    roadmap_config_declare
        ("preferences", &RoadMapConfigDisplayTopRight, "ETA: %A|%T", NULL);

    roadmap_config_declare
        ("schema", &RoadMapConfigConsoleBackground, "#ffff00", NULL);
    roadmap_config_declare
        ("schema", &RoadMapConfigConsoleForeground, "#000000", NULL);

    roadmap_config_declare
        ("schema", &RoadMapConfigWarningBackground, "#d7d7d7", NULL);
    roadmap_config_declare
        ("schema", &RoadMapConfigWarningForeground, "#026d97", NULL);

    roadmap_config_declare
        ("schema", &RoadMapConfigActivityBackground, "#535252", NULL);
    roadmap_config_declare
        ("schema", &RoadMapConfigActivityForeground, "#FFFFFF", NULL);

    for (sign = RoadMapStreetSign; sign->title != NULL; ++sign) {

        if (sign->default_format != NULL) {
           roadmap_config_declare
              ("preferences",
               &sign->format_descriptor, sign->default_format, NULL);
        }

        roadmap_config_declare
            ("preferences",
             &sign->background_descriptor, sign->default_background, NULL);

        roadmap_config_declare
            ("preferences",
             &sign->foreground_descriptor, sign->default_foreground, NULL);
    }

    roadmap_skin_register (roadmap_display_create_pens);
}
