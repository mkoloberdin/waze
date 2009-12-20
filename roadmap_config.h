/* roadmap_config.h - A module to handle all RoadMap configuration issues.
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
 */

#ifndef _ROADMAP_CONFIG__H_
#define _ROADMAP_CONFIG__H_

#include <stdio.h>

#include "roadmap.h"
#include "roadmap_types.h"


#define ROADMAP_CONFIG_STRING   0
#define ROADMAP_CONFIG_ENUM     1
#define ROADMAP_CONFIG_COLOR    2
#define ROADMAP_CONFIG_PASSWORD 3


struct RoadMapConfigItemRecord;
typedef struct RoadMapConfigItemRecord RoadMapConfigItem;

typedef struct {
    
    const char *category;
    const char *name;
    int         age;
    
    RoadMapConfigItem *reference;
    
} RoadMapConfigDescriptor;


#define ROADMAP_CONFIG_ITEM_EMPTY  {NULL, NULL, 0, NULL}
#define ROADMAP_CONFIG_ITEM(c,n)   {c, n, 0, NULL}
#define ROADMAP_CONFIG_LOCATION(n) {"Locations", n, 0, NULL}


void roadmap_config_declare
        (const char *file,
         RoadMapConfigDescriptor *descriptor, const char *default_value,
         int *is_new);

void roadmap_config_declare_password
        (const char *file,
         RoadMapConfigDescriptor *descriptor, const char *default_value);

RoadMapConfigItem *roadmap_config_declare_enumeration
        (const char *file,
         RoadMapConfigDescriptor *descriptor,
         RoadMapCallback callback,
         const char *enumeration_value, ...);

void roadmap_config_add_enumeration_value (RoadMapConfigItem *item,
                                           const char *value);

void roadmap_config_declare_color
        (const char *file,
         RoadMapConfigDescriptor *descriptor, const char *default_value);

 
char *roadmap_config_skip_until (char *p, char c);

char *roadmap_config_skip_spaces (char *p);

char *roadmap_config_extract_data (char *line, int size);


int roadmap_config_first (const char *config,
                          RoadMapConfigDescriptor *descriptor);
 
int roadmap_config_next (RoadMapConfigDescriptor *descriptor);


void *roadmap_config_get_enumeration (RoadMapConfigDescriptor *descriptor);
char *roadmap_config_get_enumeration_value (void *enumeration);
void *roadmap_config_get_enumeration_next (void *enumeration);


void  roadmap_config_initialize (void);
void  roadmap_config_save       (int force);


int   roadmap_config_get_type (RoadMapConfigDescriptor *descriptor);

const char *roadmap_config_get (RoadMapConfigDescriptor *descriptor);
void        roadmap_config_set
                (RoadMapConfigDescriptor *descriptor, const char *value);

int   roadmap_config_get_integer (RoadMapConfigDescriptor *descriptor);
void  roadmap_config_set_integer (RoadMapConfigDescriptor *descriptor, int x);

int   roadmap_config_match
        (RoadMapConfigDescriptor *descriptor, const char *text);
void  roadmap_config_get_position
        (RoadMapConfigDescriptor *descriptor, RoadMapPosition *position);
void  roadmap_config_set_position
        (RoadMapConfigDescriptor *descriptor, const RoadMapPosition *position);

int  roadmap_config_reload (const char *name);

#endif // _ROADMAP_CONFIG__H_
