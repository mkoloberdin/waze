/* navigate_plugin.c - implement plugin interfaces
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
 *   See navigate_plugin.h
 */

#include <stdlib.h>

#include "roadmap_plugin.h"

#include "navigate_main.h"
#include "navigate_traffic.h"
#include "navigate_plugin.h"


static RoadMapPluginHooks navigate_plugin_hooks = {

      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      &navigate_traffic_override_pen,
      &navigate_main_screen_repaint,
      NULL,
      NULL,
      NULL,
      NULL,
      &navigate_traffic_adjust_layer,
      NULL,
      NULL,
      NULL
};


int navigate_plugin_register (void) {

   return roadmap_plugin_register (&navigate_plugin_hooks);
}


void navigate_plugin_unregister (int plugin_id) {

   roadmap_plugin_unregister (plugin_id);
}

