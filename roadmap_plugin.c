/* roadmap_plugin.c - plugin layer
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
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
 *   See roadmap_plugin.h.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "roadmap.h"
#include "roadmap_line.h"
#include "roadmap_line_route.h"
#include "roadmap_locator.h"
#include "roadmap_street.h"
#include "roadmap_screen.h"
#include "roadmap_math.h"
#include "roadmap_shape.h"
#include "roadmap_square.h"
#include "roadmap_street.h"

#include "roadmap_plugin.h"

#define MAX_PLUGINS 10

static RoadMapPluginHooks *hooks[MAX_PLUGINS] = {0};
static int PluginCount = 0;

static RoadMapPluginHooks *get_hooks (int id) {

   assert (id < MAX_PLUGINS);

   return hooks[id];
}


int roadmap_plugin_register (RoadMapPluginHooks *hook) {

   int i;
   for (i=1; i<MAX_PLUGINS; i++) {
      
      if (hooks[i] == NULL) {
         hooks[i] = hook;
         PluginCount++;
         return i;
      }
   }

   return -1;
}


void roadmap_plugin_unregister (int plugin_id) {

int i;

   hooks[plugin_id] = NULL;
   for (i=plugin_id; i<MAX_PLUGINS-1; i++) {
   		hooks[i] = hooks[i+1];
   }   
  
  PluginCount--;
}


int roadmap_plugin_same_line (const PluginLine *line1,
                              const PluginLine *line2) {

   if ((line1 == NULL) || (line2 == NULL)) return 0;

   return ( (line1->plugin_id == line2->plugin_id) &&
            ((line1->plugin_id != ROADMAP_PLUGIN_ID) || 
             (line1->square == line2->square)) &&
            (line1->line_id == line2->line_id) &&
            (line1->fips == line2->fips) );
}


int roadmap_plugin_same_street (const PluginStreet *street1,
                                const PluginStreet *street2) {

   return ( (street1->plugin_id == street2->plugin_id) &&
            ((street1->plugin_id != ROADMAP_PLUGIN_ID) ||
             (street1->square == street2->square)) &&
            (street1->street_id == street2->street_id) );
}


int roadmap_plugin_get_id (const PluginLine *line) {
   return line->plugin_id;
}


int roadmap_plugin_get_square (const PluginLine *line) {
   return line->square;
}


int roadmap_plugin_get_fips (const PluginLine *line) {
   return line->fips;
}


int roadmap_plugin_get_line_id (const PluginLine *line) {

   if (line->plugin_id == ROADMAP_PLUGIN_ID) {
   	roadmap_square_set_current(line->square);
   }
   return line->line_id;
}


int roadmap_plugin_get_line_cfcc (const PluginLine *line) {
   return line->cfcc;
}


int roadmap_plugin_get_street_id (const PluginStreet *street) {

   return street->street_id;
}


void roadmap_plugin_set_line (PluginLine *line,
                              int plugin_id,
                              int line_id,
                              int cfcc,
                              int square,
                              int fips) {

   line->plugin_id = plugin_id;
   line->line_id = line_id;
   line->cfcc = cfcc;
   line->square = square;
   line->fips = fips;
}


void roadmap_plugin_set_street (PluginStreet *street,
                                int plugin_id,
                                int square,
                                int street_id) {

   street->plugin_id = plugin_id;
   street->square = square;
   street->street_id = street_id;
}


int roadmap_plugin_activate_db (const PluginLine *line) {

   if (line->plugin_id == ROADMAP_PLUGIN_ID) {

      if (roadmap_locator_activate (line->fips) != ROADMAP_US_OK) {
         return -1;
      }

      return 0;

   } else {
      RoadMapPluginHooks *hooks = get_hooks (line->plugin_id);

      if (hooks == NULL) {
         roadmap_log (ROADMAP_ERROR, "plugin id:%d is missing.",
               line->plugin_id);

         return -1;
      }

      if (hooks->activate_db != NULL) {
         return (*hooks->activate_db) (line);
      }

      return 0;
   }
}


int roadmap_plugin_get_distance
            (const RoadMapPosition *point,
             const PluginLine *line,
             RoadMapNeighbour *result) {

   if (line->plugin_id == ROADMAP_PLUGIN_ID) {

		roadmap_square_set_current (line->square);
      return roadmap_street_get_distance
                     (point,
                      line->line_id,
                      line->cfcc,
                      result);
   } else {

      RoadMapPluginHooks *hooks = get_hooks (line->plugin_id);

      if (hooks == NULL) {
         roadmap_log (ROADMAP_ERROR, "plugin id:%d is missing.",
               line->plugin_id);

         return 0;
      }

      if (hooks->activate_db != NULL) {
         return (*hooks->get_distance) (point, line, result);
      }

      return 0;
   }
}
      

void roadmap_plugin_get_line_points (const PluginLine *line,
                                     RoadMapPosition  *from_pos,
                                     RoadMapPosition  *to_pos,
                                     int              *first_shape,
                                     int              *last_shape,
                                     RoadMapShapeItr  *shape_itr) {

   roadmap_plugin_line_from (line, from_pos);
   roadmap_plugin_line_to (line, to_pos);

   if (line->plugin_id == ROADMAP_PLUGIN_ID) {

      roadmap_line_shapes (line->line_id, first_shape, last_shape);
      *shape_itr = NULL;
   } else {
      RoadMapPluginHooks *hooks = get_hooks (line->plugin_id);
      
      if (hooks == NULL) {
         roadmap_log (ROADMAP_ERROR, "plugin id:%d is missing.",
               line->plugin_id);

         *first_shape = *last_shape = -1;
         *shape_itr   = NULL;
         return;
      }

      //FIXME implement for plugins
#if 0
      if (hooks->line_shapes != NULL) {
         (*hooks->line_shapes) (line, first_shape, last_shape, shape_itr);

      } else {
#else
         {
#endif   
         *first_shape = *last_shape = -1;
         *shape_itr   = NULL;
      }

      return;
   }
}


void roadmap_plugin_line_from (const PluginLine *line, RoadMapPosition *pos) {

   if (line->plugin_id == ROADMAP_PLUGIN_ID) {

	   roadmap_square_set_current (line->square);
      roadmap_line_from (line->line_id, pos);
   } else {
      RoadMapPluginHooks *hooks = get_hooks (line->plugin_id);
      
      if (hooks == NULL) {
         roadmap_log (ROADMAP_ERROR, "plugin id:%d is missing.",
               line->plugin_id);

         pos->longitude = pos->latitude = 0;
         return;
      }

      if (hooks->line_from != NULL) {
         (*hooks->line_from) (line, pos);

      } else {
         pos->longitude = pos->latitude = 0;
      }

      return;
   }
}


void roadmap_plugin_line_to (const PluginLine *line, RoadMapPosition *pos) {

   if (line->plugin_id == ROADMAP_PLUGIN_ID) {

   	roadmap_square_set_current (line->square);
      roadmap_line_to (line->line_id, pos);
   } else {
      RoadMapPluginHooks *hooks = get_hooks (line->plugin_id);
      
      if (hooks == NULL) {
         roadmap_log (ROADMAP_ERROR, "plugin id:%d is missing.",
               line->plugin_id);

         pos->longitude = pos->latitude = 0;
         return;
      }

      if (hooks->line_to != NULL) {
         (*hooks->line_to) (line, pos);

      } else {
         pos->longitude = pos->latitude = 0;
      }

      return;
   }
}


int roadmap_plugin_override_line (int line, int cfcc, int fips) {

	int i;

#ifdef __SYMBIAN32__
  return 0;
#elif defined (J2ME)
   return 0;
#else
   //return 0;
//   return hooks[1]->override_line (line, cfcc, fips);
#endif   
//#if 0
      for (i=1; i<=PluginCount; i++) {

      RoadMapPluginHooks *hooks = get_hooks (i);
      if (hooks == NULL) continue;

      if (hooks->override_line != NULL) {

         int res = hooks->override_line (line, cfcc, fips);

         if (res) return res;
      }
   }

   return 0;
//#endif   
}


int roadmap_plugin_override_pen (int line,
                                 int cfcc,
                                 int fips,
                                 int pen_type,
                                 RoadMapPen *override_pen) {
   return 0;

   #if 0
   int res = hooks[1]->override_pen
                     (line, cfcc, fips, pen_type, override_pen);
   return res;

   int i;

   for (i=1; i<=PluginCount; i++) {

      RoadMapPluginHooks *hooks = get_hooks (i);
      if (hooks == NULL) continue;

      if (hooks->override_pen != NULL) {

         int res = hooks->override_pen
                     (line, cfcc, fips, pen_type, override_pen);

         if (res) return res;
      }
   }

   return 0;
#endif
}


void roadmap_plugin_screen_repaint (int max_pen) {

   int i;

   for (i=1; i<=PluginCount; i++) {

      RoadMapPluginHooks *hooks = get_hooks (i);
      if ((hooks == NULL) || (hooks->screen_repaint == NULL)) continue;

      hooks->screen_repaint (max_pen);
   }
}


void roadmap_plugin_get_street (const PluginLine *line, PluginStreet *street) {

   if (line->plugin_id == ROADMAP_PLUGIN_ID) {

      RoadMapStreetProperties properties;

		roadmap_square_set_current (line->square);
      roadmap_street_get_properties (line->line_id, &properties);
      street->plugin_id = ROADMAP_PLUGIN_ID;
      street->street_id = properties.street;
      street->square = line->square;

   } else {
      RoadMapPluginHooks *hooks = get_hooks (line->plugin_id);
      street->plugin_id = line->plugin_id;
      street->square = line->square;

      if (hooks == NULL) {
         roadmap_log (ROADMAP_ERROR, "plugin id:%d is missing.",
               line->plugin_id);

         street->street_id = -1;
         return;
      }

      if (hooks->get_street != NULL) {
         (*hooks->get_street) (line, street);

      } else {
         street->street_id = -1;
      }

      return;
   }
}


const char *roadmap_plugin_street_full_name (const PluginLine *line) {

   if (line->plugin_id == ROADMAP_PLUGIN_ID) {

      RoadMapStreetProperties properties;
      roadmap_square_set_current (line->square);
      roadmap_street_get_properties (line->line_id, &properties);

      return roadmap_street_get_full_name (&properties);

   } else {
      RoadMapPluginHooks *hooks = get_hooks (line->plugin_id);

      if (hooks == NULL) {
         roadmap_log (ROADMAP_ERROR, "plugin id:%d is missing.",
               line->plugin_id);

         return "";
      }

      if (hooks->get_street_full_name != NULL) {
         return (*hooks->get_street_full_name) (line);
      }

      return "";
   }
}


void roadmap_plugin_get_street_properties (const PluginLine *line,
                                           PluginStreetProperties *props,
                                           int type) {
   
   if (line->plugin_id == ROADMAP_PLUGIN_ID) {

      RoadMapStreetProperties rm_properties;

	   roadmap_square_set_current (line->square);

      if (type == PLUGIN_STREET_ONLY) {
         roadmap_street_get_street (line->line_id, &rm_properties);
         props->street = roadmap_street_get_street_name (&rm_properties);
         props->plugin_street.plugin_id = ROADMAP_PLUGIN_ID;
         props->plugin_street.street_id = rm_properties.street;
         return;
      }

      roadmap_street_get_properties (line->line_id, &rm_properties);

      props->address = roadmap_street_get_street_address (&rm_properties);
      props->street = roadmap_street_get_street_name (&rm_properties);
      props->street_t2s = roadmap_street_get_street_t2s (&rm_properties);
      props->city = roadmap_street_get_city_name (&rm_properties);
      props->plugin_street.plugin_id = ROADMAP_PLUGIN_ID;
      props->plugin_street.street_id = rm_properties.street;
      return;

   } else {
      RoadMapPluginHooks *hooks = get_hooks (line->plugin_id);

      props->address = "";
      props->street = "";
      props->street_t2s = "";
      props->city = "";

      if (hooks == NULL) {
         roadmap_log (ROADMAP_ERROR, "plugin id:%d is missing.",
               line->plugin_id);

         return;
      }

      if (hooks->get_street_properties != NULL) {
         (*hooks->get_street_properties) (line, props, type);
      }

      return;
   }
}


int roadmap_plugin_find_connected_lines (RoadMapPosition *crossing,
                                         PluginLine *plugin_lines,
                                         int max) {

   int i;
   int count = 0;

   for (i=1; i<=PluginCount; i++) {

      RoadMapPluginHooks *hooks = get_hooks (i);
      if ((hooks == NULL) || (hooks->find_connected_lines == NULL)) continue;

      count +=
         hooks->find_connected_lines
            (crossing, plugin_lines + count, max - count);
   }

   return count;
}

void roadmap_plugin_adjust_layer (int layer,
                                  int thickness,
                                  int pen_count) {

   int i;
   for (i=1; i<=PluginCount; i++) {

      RoadMapPluginHooks *hooks = get_hooks (i);
      if ((hooks == NULL) || (hooks->adjust_layer == NULL)) continue;

      hooks->adjust_layer (layer, thickness, pen_count);
   }
}


int roadmap_plugin_get_closest
       (const RoadMapPosition *position,
        int *categories, int categories_count, int max_shapes,
        RoadMapNeighbour *neighbours, int count,
        int max) {

   int i;

   for (i=1; i<=PluginCount; i++) {

      RoadMapPluginHooks *hooks = get_hooks (i);
      if (hooks == NULL) continue;

      if (hooks->get_closest != NULL) {

         count = hooks->get_closest
                     (position, categories, categories_count, max_shapes,
                      neighbours, count, max);
      }
   }

   return count;
}


int roadmap_plugin_get_direction (PluginLine *line, int who) {
   
   if (line->plugin_id == ROADMAP_PLUGIN_ID) {

		roadmap_square_set_current (line->square);
      return roadmap_line_route_get_direction (line->line_id, who);

   } else {
      RoadMapPluginHooks *hooks = get_hooks (line->plugin_id);

      if (hooks == NULL) {
         roadmap_log (ROADMAP_ERROR, "plugin id:%d is missing.",
               line->plugin_id);

         return 0;
      }

      if (hooks->route_direction != NULL) {
         return (*hooks->route_direction) (line, who);
      }

      return 0;
   }

}


int roadmap_plugin_calc_length (const RoadMapPosition *position,
                                const PluginLine *line,
                                int *total_length) {

   RoadMapPosition line_from_pos;
   RoadMapPosition line_to_pos;
   int first_shape;
   int last_shape;
   RoadMapShapeItr shape_itr;

   roadmap_plugin_get_line_points (line, &line_from_pos, &line_to_pos,
                                  &first_shape, &last_shape, &shape_itr);
                                    
   return roadmap_math_calc_line_length (position, &line_from_pos, &line_to_pos,
                                         first_shape, last_shape, shape_itr,
                                         total_length);
}


void roadmap_plugin_shutdown (void) {

   int i;

   for (i=1; i<=PluginCount; i++) {

      RoadMapPluginHooks *hooks = get_hooks (i);
      if (hooks == NULL) continue;

      if (hooks->shutdown != NULL) {

         hooks->shutdown ();
      }
   }
}


static int check_same_line (const PluginLine *line, void *context, int extend_flags) {

	PluginLine *comp_line = (PluginLine *)context;
		
	if (line->square == comp_line->square &&
		 line->line_id == comp_line->line_id) {
		 	
		 comp_line->plugin_id = 1;
	}
	
	return comp_line->plugin_id;
}


int roadmap_plugin_same_db_line (const PluginLine *line1,
                              	const PluginLine *line2) {
	
	PluginLine temp_line;
	
	if (roadmap_plugin_same_line (line1, line2)) return 1;
	
	if (line1->plugin_id != ROADMAP_PLUGIN_ID ||
		 line1->plugin_id != line2->plugin_id) {
		return 0;
	} 
                              		
	temp_line = *line2;
	temp_line.plugin_id = 0; // using this for count
	
	roadmap_street_extend_line_ends (line1, NULL, NULL, FLAG_EXTEND_BOTH, check_same_line, &temp_line);
	return temp_line.plugin_id;
}
