/* roadmap_option.c - Manage the RoadMap command line options.
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
 *   see roadmap.h.
 */

#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_config.h"

#ifdef _WIN32
   #ifdef   TOUCH_SCREEN
      #pragma message("    Target device type:    TOUCH-SCREEN")
   #else
      #pragma message("    Target device type:    MENU ONLY (NO TOUCH-SCREEN)")
   #endif   // TOUCH_SCREEN
#endif   // _WIN32



static RoadMapConfigDescriptor RoadMapConfigGeometryMain =
                        ROADMAP_CONFIG_ITEM("Geometry", "Main");

static RoadMapConfigDescriptor RoadMapConfigGeometryUnit =
                        ROADMAP_CONFIG_ITEM("Geometry", "Unit");

static RoadMapConfigDescriptor RoadMapConfigAddressPosition =
                        ROADMAP_CONFIG_ITEM("Address", "Position");

static RoadMapConfigDescriptor RoadMapConfigGeneralToolbar =
                        ROADMAP_CONFIG_ITEM("General", "Toolbar");

static RoadMapConfigDescriptor RoadMapConfigGeneralIcons =
                        ROADMAP_CONFIG_ITEM("General", "Icons");

static RoadMapConfigDescriptor RoadMapConfigMapCache =
                        ROADMAP_CONFIG_ITEM("Map", "Cache");

RoadMapConfigDescriptor RoadMapConfigGeneralLogLevel =
                        ROADMAP_CONFIG_ITEM("General", "Log level");

static int roadmap_option_verbose = DEFAULT_LOG_LEVEL;

static int roadmap_option_no_area = 0;
static int roadmap_option_square  = 0;
static int roadmap_option_cache_size = 0;
static int roadmap_option_synchronous = 0;

static char *roadmap_option_debug = "";
static char *roadmap_option_gps = NULL;

static float roadmap_option_fast_forward_factor = 1.0F;

static RoadMapUsage RoadMapOptionUsage = NULL;


static const char *roadmap_option_get_geometry (const char *name) {

    RoadMapConfigDescriptor descriptor;

    descriptor.category = "Geometry";
    descriptor.name = name;
    descriptor.reference = NULL;

    return roadmap_config_get (&descriptor);
}


int roadmap_is_visible (int category) {

   switch (category) {
      case ROADMAP_SHOW_AREA:
         return (! roadmap_option_no_area);
      case ROADMAP_SHOW_SQUARE:
         return roadmap_option_square;
   }

   return 1;
}


float roadmap_fast_forward_factor (void) {
	
	return roadmap_option_fast_forward_factor;
}


char *roadmap_gps_source (void) {

   return roadmap_option_gps;
}


int roadmap_verbosity (void) {

   return roadmap_option_verbose;
}


char *roadmap_debug (void) {

   return roadmap_option_debug;
}


int roadmap_option_cache (void) {

   if (roadmap_option_cache_size > 0) {
      return roadmap_option_cache_size;
   }
   return roadmap_config_get_integer (&RoadMapConfigMapCache);
}


int roadmap_option_width (const char *name) {

    const char *option = roadmap_option_get_geometry (name);

    if (option == NULL || option[0] == 0) {
        return 300;
    }
    return atoi (option);
}


int roadmap_option_height (const char *name) {

    const char *option = roadmap_option_get_geometry (name);
    char *separator;

    separator = strchr (option, 'x');
    if (separator == NULL) {
        return 200;
    }
    return atoi(separator+1);
}


int roadmap_option_is_synchronous (void) {

   return roadmap_option_synchronous;
}


static void roadmap_option_set_location (const char *value) {

    roadmap_config_set (&RoadMapConfigAddressPosition, value);
}


static void roadmap_option_set_metric (const char *value) {

    roadmap_config_set (&RoadMapConfigGeometryUnit, "metric");
}


static void roadmap_option_set_imperial (const char *value) {

    roadmap_config_set (&RoadMapConfigGeometryUnit, "imperial");
}


static void roadmap_option_set_no_area (const char *value) {

    roadmap_option_no_area = 1;
}


static void roadmap_option_set_geometry1 (const char *value) {

    roadmap_config_set (&RoadMapConfigGeometryMain, value);
}


static void roadmap_option_set_geometry2 (const char *value) {

    char *p;
    char *geometry;
    char buffer[256];
    RoadMapConfigDescriptor descriptor;

    strncpy_safe (buffer, value, sizeof(buffer));

    geometry = strchr (buffer, '=');
    if (geometry == NULL) {
        roadmap_log (ROADMAP_FATAL,
                     "%s: invalid geometry option syntax", value);
    }
    *(geometry++) = 0;

    for (p = strchr(buffer, '-'); p != NULL; p =strchr(p, '-')) {
        *p = ' ';
    }

    descriptor.category = "Geometry";
    descriptor.name = strdup(buffer);
    descriptor.reference = NULL;
    roadmap_config_declare ("preferences", &descriptor, "300x200", NULL);
    roadmap_config_set (&descriptor, geometry);
}


static void roadmap_option_set_no_toolbar (const char *value) {

    roadmap_config_set (&RoadMapConfigGeneralToolbar, "no");
}


static void roadmap_option_set_no_icon (const char *value) {

    roadmap_config_set (&RoadMapConfigGeneralIcons, "no");
}


static void roadmap_option_set_square (const char *value) {

    roadmap_option_square = 1;
}

static void roadmap_option_set_fastforward (const char *value) {
	
	roadmap_option_fast_forward_factor = atof(value);
	
}

static void roadmap_option_set_gps (const char *value) {

    if (roadmap_option_gps != NULL) {
        free (roadmap_option_gps);
    }
    roadmap_option_gps = strdup (value);
}


static void roadmap_option_set_cache (const char *value) {

    roadmap_option_cache_size = atoi(value);

    if (roadmap_option_cache_size <= 0) {
       roadmap_log (ROADMAP_FATAL, "invalid cache size %s", value);
    }
    roadmap_config_set (&RoadMapConfigMapCache, value);
}


static void roadmap_option_set_debug (const char *value) {

    if (roadmap_option_verbose > ROADMAP_MESSAGE_DEBUG) {
        roadmap_option_verbose = ROADMAP_MESSAGE_DEBUG;
    }
    if (value != NULL && value[0] != 0) {
       roadmap_option_debug = strdup (value);
    }
}


static void roadmap_option_set_verbose (const char *value) {

    if (roadmap_option_verbose > ROADMAP_MESSAGE_INFO) {
        roadmap_option_verbose = ROADMAP_MESSAGE_INFO;
    }
}

static void roadmap_option_set_synchronous (const char *value) {

    roadmap_option_synchronous = 1;
}

static void roadmap_option_usage (const char *value);


typedef void (*roadmap_option_handler) (const char *value);

struct roadmap_option_descriptor {

    const char *name;
    const char *format;

    roadmap_option_handler handler;

    const char *help;
};

static struct roadmap_option_descriptor RoadMapOptionMap[] = {

    {"--location=", "LONGITUDE,LATITUDE", roadmap_option_set_location,
        "Set the location point (see menu entry Screen/Show Location..)"},

    {"--metric", "", roadmap_option_set_metric,
        "Use the metric system for all units"},

    {"--imperial", "", roadmap_option_set_imperial,
        "Use the imperial system for all units"},

    {"--no-area", "", roadmap_option_set_no_area,
        "Do not show the polygons (parks, hospitals, airports, etc..)"},

    {"-geometry=", "WIDTHxHEIGHT", roadmap_option_set_geometry1,
        "Same as the --geometry option below"},

    {"--geometry=", "WIDTHxHEIGHT", roadmap_option_set_geometry1,
        "Set the geometry of the RoadMap main window"},

    {"--geometry:", "WINDOW=WIDTHxHEIGHT", roadmap_option_set_geometry2,
        "Set the geometry of a specific RoadMap window"},

    {"--no-toolbar", "", roadmap_option_set_no_toolbar,
        "Hide the RoadMap main window's toolbar"},

    {"--no-icon", "", roadmap_option_set_no_icon,
        "Do not show icons on the toolbar"},

    {"--square", "", roadmap_option_set_square,
        "Show the square boundaries as grey lines (for debug purpose)"},

    {"--fff=", "FLOAT", roadmap_option_set_fastforward,
        "Fast-forward factor of gps simulation (for debug purpose)"},

    {"--gps=", "URL", roadmap_option_set_gps,
        "Use a specific GPS source (mainly for replay of a GPS log)"},

    {"--gps-sync", "", roadmap_option_set_synchronous,
        "Update the map synchronously when receiving each GPS position"},

    {"--cache=", "INTEGER", roadmap_option_set_cache,
        "Set the number of entries in the RoadMap's map cache"},

    {"--debug", "", roadmap_option_set_debug,
        "Show all informational and debug traces"},

    {"--debug=", "SOURCE", roadmap_option_set_debug,
        "Show the informational and debug traces for a specific source."},

    {"--verbose", "", roadmap_option_set_verbose,
        "Show all informational traces"},

    {"--help=", "SECTION", roadmap_option_usage,
        "Show a section of the help message"},

    {"--help", "", roadmap_option_usage,
        "Show this help message"},

    {NULL, NULL, NULL, NULL}
};


static void roadmap_option_usage (const char *value) {

    struct roadmap_option_descriptor *option;

    if ((value == NULL) || (strcasecmp (value, "options") == 0)) {

       printf ("OPTIONS:\n");

       for (option = RoadMapOptionMap; option->name != NULL; ++option) {

          printf ("  %s%s\n", option->name, option->format);
          printf ("        %s.\n", option->help);
       }
       if (value != NULL) exit (0);
    }

    if (RoadMapOptionUsage != NULL) {
       RoadMapOptionUsage (value);
    }
    exit(0);
}


void roadmap_option (int argc, char **argv, RoadMapUsage usage) {

    int   i;
    int   length;
    int   compare;
    char *value;
    struct roadmap_option_descriptor *option;


    RoadMapOptionUsage = usage;

    for (i = 1; i < argc; i++) {

        compare = 1; /* Different. */

        for (option = RoadMapOptionMap; option->name != NULL; ++option) {

            if (option->format[0] == 0) {

                value = NULL;
                compare = strcmp (option->name, argv[i]);

            } else {
                length = strlen (option->name);
                value = argv[i] + length;
                compare = strncmp (option->name, argv[i], length);
            }

            if (compare == 0) {
                option->handler (value);
                break;
            }
        }

        if (compare != 0) {
            roadmap_log (ROADMAP_FATAL, "illegal option %s", argv[i]);
        }
    }

    RoadMapOptionUsage = NULL;
}


void roadmap_option_initialize (void) {

   roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigGeneralToolbar, NULL, "yes", "no", NULL);

   roadmap_config_declare_enumeration
      ("preferences", &RoadMapConfigGeneralIcons, NULL, "yes", "no", NULL);

   roadmap_config_declare ("preferences", &RoadMapConfigMapCache, "8", NULL );

   roadmap_config_declare( "preferences", &RoadMapConfigGeneralLogLevel, OBJ2STR( DEFAULT_LOG_LEVEL ), NULL );

   roadmap_option_set_verbosity( roadmap_config_get_integer( &RoadMapConfigGeneralLogLevel ) );
}

void roadmap_option_set_verbosity( int verbosity_level )
{
	roadmap_option_verbose = verbosity_level;
}
