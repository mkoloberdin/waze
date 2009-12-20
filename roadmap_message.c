/* roadmap_display.c - Manage screen signs.
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
 *   See roadmap_display.h.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "roadmap.h"
#include "roadmap_message.h"


static char *RoadMapMessageParameters[128] = {NULL};

static void roadmap_message_dummy (void) {}

static RoadMapCallback RoadMapMessageUpdate = roadmap_message_dummy;

RoadMapCallback roadmap_message_register (RoadMapCallback callback) {

   RoadMapCallback prev = RoadMapMessageUpdate;
   RoadMapMessageUpdate = callback;

   return prev;
}


void roadmap_message_update (void) {

   (*RoadMapMessageUpdate) ();
}


int roadmap_message_format (char *text, int length, const char *format) {

    char *f;
    char *p = text;
    char *end = text + length - 1;
    
    while (*format) {
        
        if (*format == '%') {
            
            format += 1;
            if (*format <= 0) {
                break;
            }
            
            f = RoadMapMessageParameters[(int)(*(format++))];
            if (f != NULL) {
                while (*f && p < end) {
                    *(p++) = *(f++);
                }
            } else {
                format = strchr (format, '|');
                
                if (format == NULL) {
                    return 0; /* Cannot build the string. */
                }
                format += 1;
                p = text; /* Restart. */
            }

        } else if (*format == '|') {
            
            break; /* We completed this alternative successfully. */
            
        } else {

            *(p++) = *(format++);
        }
        
        if (p >= end) {
            break;
        }
    }

    *p = 0;

    return p > text;
}


void roadmap_message_set (char parameter, const char *format, ...) {
    
    va_list ap;
    char    value[256];
    
    if (parameter <= 0) {
        roadmap_log( ROADMAP_ERROR, "invalid parameter code %d",  parameter);
        return;
    }
    /*
    *  Check this   AGA 
    */
    if (format == NULL) {
        roadmap_log( ROADMAP_ERROR, "format is NULL");
        return;
    }
    
    if (format == NULL) {
        roadmap_log( ROADMAP_ERROR, "format is NULL");
        return;
    }
    
    va_start(ap, format);
    vsnprintf(value, sizeof(value), format, ap);
    va_end(ap);
    
    if (RoadMapMessageParameters[(int)parameter] != NULL) {
        free (RoadMapMessageParameters[(int)parameter]);
    }
    if (value[0] == 0) {
        RoadMapMessageParameters[(int)parameter] = NULL;
    } else {
        RoadMapMessageParameters[(int)parameter] = strdup (value);
    }
}


void roadmap_message_unset (char parameter) {
    
    if (parameter <= 0) {
        roadmap_log (ROADMAP_ERROR,
                     "invalid parameter code %d",
                     parameter);
        return;
    }
    
    if (RoadMapMessageParameters[(int)parameter] != NULL) {
        free (RoadMapMessageParameters[(int)parameter]);
        RoadMapMessageParameters[(int)parameter] = NULL;
    }
}


int roadmap_message_is_set (char parameter) {
    
    if (parameter <= 0) {
        roadmap_log (ROADMAP_ERROR,
                     "invalid parameter code %d",
                     parameter);
        return 0;
    }
    
    return RoadMapMessageParameters[(int)parameter] != NULL;
}
