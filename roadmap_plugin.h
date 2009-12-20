/* roadmap_plugin.h - plugin interfaces
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
 */

#ifndef INCLUDE__ROADMAP_PLUGIN__H
#define INCLUDE__ROADMAP_PLUGIN__H

#include "roadmap_canvas.h"
#include "roadmap_screen.h"

#define INVALID_PLUGIN_ID -1
#define ROADMAP_PLUGIN_ID 0

#define PLUGIN_MAKE_LINE(plugin_id, line_id, cfcc, square, fips) \
   {plugin_id, line_id, cfcc, square, fips}

#define PLUGIN_VALID(plugin) (plugin.plugin_id >= 0)
#define INVALIDATE_PLUGIN(plugin) (plugin.plugin_id = -1)

#define PLUGIN_STREET_ONLY 0x1

/* plugin types */

typedef struct {
   int plugin_id;
   int line_id;
   int cfcc;
   int square;
   int fips;
} PluginLine;

typedef struct {
   int plugin_id;
   int square;
   int street_id;
} PluginStreet;

typedef struct {
   const char *address;
   const char *street;
   const char *street_t2s;
   const char *city;
   PluginStreet plugin_street;
} PluginStreetProperties;

#define PLUGIN_LINE_NULL {-1, -1, -1, -1, -1}
#define PLUGIN_STREET_NULL {-1, -1, -1}

struct RoadMapNeighbour_t;

int roadmap_plugin_same_line (const PluginLine *line1, const PluginLine *line2);

int roadmap_plugin_same_db_line (const PluginLine *line1,
                              	const PluginLine *line2);
                              	
int roadmap_plugin_same_street (const PluginStreet *street1,
                                const PluginStreet *street2);

void roadmap_plugin_get_street (const PluginLine *line, PluginStreet *street);

void roadmap_plugin_line_from (const PluginLine *line, RoadMapPosition *pos);

void roadmap_plugin_line_to (const PluginLine *line, RoadMapPosition *pos);

void roadmap_plugin_get_line_points (const PluginLine *line,
                                     RoadMapPosition  *from_pos,
                                     RoadMapPosition  *to_pos,
                                     int              *first_shape,
                                     int              *last_shape,
                                     RoadMapShapeItr  *shape_itr);

int roadmap_plugin_get_id (const PluginLine *line);

int roadmap_plugin_get_square (const PluginLine *line);

int roadmap_plugin_get_fips (const PluginLine *line);

int roadmap_plugin_get_line_id (const PluginLine *line);

int roadmap_plugin_get_line_cfcc (const PluginLine *line);

int roadmap_plugin_get_street_id (const PluginStreet *street);

void roadmap_plugin_set_line (PluginLine *line,
                              int plugin_id,
                              int line_id,
                              int cfcc,
                              int square,
                              int fips);

void roadmap_plugin_set_street (PluginStreet *street,
                                int plugin_id,
                                int square,
                                int street_id);

int roadmap_plugin_activate_db (const PluginLine *line);

int roadmap_plugin_get_distance
            (const RoadMapPosition *point,
             const PluginLine *line,
             struct RoadMapNeighbour_t *result);

/* hook functions definition */

typedef int (*plugin_override_line_hook) (int line, int cfcc, int fips);

typedef int (*plugin_override_pen_hook) (int line,
                                         int cfcc,
                                         int fips,
                                         int pen_type,
                                         RoadMapPen *override_pen);

typedef void (*plugin_screen_repaint_hook) (int max_pen);
typedef int  (*plugin_activate_db_func) (const PluginLine *line);
typedef void (*plugin_line_pos_func)
   (const PluginLine *line, RoadMapPosition *pos);

typedef int (*plugin_get_distance_func)
               (const RoadMapPosition *point,
                const PluginLine *line,
                struct RoadMapNeighbour_t *result);

typedef void (*plugin_get_street_func)
               (const PluginLine *line, PluginStreet *street);

typedef const char *(*plugin_street_full_name_func)
                       (const PluginLine *line);

typedef void (*plugin_street_properties_func)
              (const PluginLine *line, PluginStreetProperties *props,
               int type);

typedef int (*plugin_find_connected_lines_func)
                  (const RoadMapPosition *crossing,
                   PluginLine *plugin_lines,
                   int max);

typedef void (*plugin_adjust_layer_hook)
               (int layer, int thickness, int pen_count);

typedef int (*plugin_get_closest_func)
       (const RoadMapPosition *position,
        int *categories, int categories_count, int max_shapes,
        struct RoadMapNeighbour_t *neighbours, int count,
        int max);

typedef int (*plugin_line_route_direction) (PluginLine *line, int who);

typedef void (*plugin_shutdown) (void);

typedef struct {
   plugin_line_pos_func             line_from;
   plugin_line_pos_func             line_to;
   plugin_activate_db_func          activate_db;
   plugin_get_distance_func         get_distance;
   plugin_override_line_hook        override_line;
   plugin_override_pen_hook         override_pen;
   plugin_screen_repaint_hook       screen_repaint;
   plugin_get_street_func           get_street;
   plugin_street_full_name_func     get_street_full_name;
   plugin_street_properties_func    get_street_properties;
   plugin_find_connected_lines_func find_connected_lines;
   plugin_adjust_layer_hook         adjust_layer;
   plugin_get_closest_func          get_closest;
   plugin_line_route_direction      route_direction;
   plugin_shutdown                  shutdown;

} RoadMapPluginHooks;


/* Interface functions */

int roadmap_plugin_register (RoadMapPluginHooks *hooks);
void roadmap_plugin_unregister (int plugin_id);

int roadmap_plugin_override_line (int line, int cfcc, int fips);

int roadmap_plugin_override_pen (int line,
                                 int cfcc,
                                 int fips,
                                 int pen_type,
                                 RoadMapPen *override_pen);

void roadmap_plugin_screen_repaint (int max_pen);

const char *roadmap_plugin_street_full_name (const PluginLine *line);

void roadmap_plugin_get_street_properties (const PluginLine *line,
                                           PluginStreetProperties *props,
                                           int type);

int roadmap_plugin_find_connected_lines (RoadMapPosition *crossing,
                                         PluginLine *plugin_lines,
                                         int max);

void roadmap_plugin_adjust_layer (int layer,
                                  int thickness,
                                  int pen_count);

int roadmap_plugin_get_closest
       (const RoadMapPosition *position,
        int *categories, int categories_count, int max_shapes,
        struct RoadMapNeighbour_t *neighbours, int count,
        int max);

int roadmap_plugin_get_direction (PluginLine *line, int who);

int roadmap_plugin_calc_length (const RoadMapPosition *position,
                                const PluginLine *line,
                                int *total_length);

void roadmap_plugin_shutdown (void);

#endif /* INCLUDE__ROADMAP_PLUGIN__H */

