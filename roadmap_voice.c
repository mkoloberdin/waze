/* roadmap_voice.c - Manage voice announcements.
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
 *   See roadmap_voice.h.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"
#include "roadmap_math.h"
#include "roadmap_config.h"
#include "roadmap_message.h"
#include "roadmap_spawn.h"

#include "roadmap_voice.h"


struct roadmap_voice_config {

    RoadMapConfigDescriptor config;
    const char *default_text;
};

static RoadMapConfigDescriptor RoadMapVoiceMute =
                        ROADMAP_CONFIG_ITEM("Voice", "Mute");

static struct roadmap_voice_config RoadMapVoiceText[] = {
    {ROADMAP_CONFIG_ITEM("Voice", "Approach"), "flite -t 'Approaching %N'"},
    {ROADMAP_CONFIG_ITEM("Voice", "Current Street"), "flite -t 'On %N'"},
    {ROADMAP_CONFIG_ITEM("Voice", "Next Intersection"), "flite -t 'Next intersection: %N'"},
    {ROADMAP_CONFIG_ITEM("Voice", "Selected Street"), "flite -t 'Selected %N'"},
    {ROADMAP_CONFIG_ITEM("Voice", "Driving Instruction"), "flite -t 'In %t, %I'|flite -t '%I to %T'|flite -t '%I'"},
    {ROADMAP_CONFIG_ITEM_EMPTY, NULL}
};


/* This will be overwritten with the setting from the session context. */
static int   RoadMapVoiceMuted = 1;

static int   RoadMapVoiceInUse = 0;
static char *RoadMapVoiceCurrentCommand = NULL;
static char *RoadMapVoiceCurrentArguments = NULL;
static char *RoadMapVoiceNextCommand = NULL;
static char *RoadMapVoiceNextArguments = NULL;

static RoadMapFeedback RoadMapVoiceActive;


struct voice_translation {
    char *from;
    char *to;
};


/* The translations below must be sorted by decreasing size,
 * so that the longest match is always selected first.
 */
static struct voice_translation RoadMapVoiceTranslation[] = {
    {"Blvd", "boulevard"},
    {"Hwy",  "Highway"},
    {"Cir",  "circle"},
    {"St",   "street"},
    {"Rd",   "road"},
    {"Pl",   "place"},
    {"Ct",   "court"},
    {"Dr",   "drive"},
    {"Ln",   "lane"},
    {"N",    "north"},
    {"W",    "west"},
    {"S",    "south"},
    {"E",    "east"},
    {"right",    "ra-ight"},
    {"m",    "meters"},
    {"Km",    "kilo-meters"},
    {NULL, NULL}
};


static void roadmap_voice_launch (const char *name, const char *arguments) {

    if (RoadMapVoiceMuted) {
       if (strcasecmp
             (roadmap_config_get (&RoadMapVoiceMute), "no") == 0) {
          RoadMapVoiceMuted = 0;
       } else {
          return;
       }
    }

    if ((RoadMapVoiceCurrentCommand != NULL) &&
        (RoadMapVoiceCurrentArguments != NULL) &&
        (strcmp (name, RoadMapVoiceCurrentCommand) == 0) &&
        (strcmp (arguments, RoadMapVoiceCurrentArguments) == 0)) {

        /* Do not repeat the same message again. */

        RoadMapVoiceInUse = 0;
        roadmap_log(ROADMAP_DEBUG, "voice now idle");
        return;
    }

    roadmap_log(ROADMAP_DEBUG, "activating message %s", arguments);

    RoadMapVoiceInUse = 1;

    if (RoadMapVoiceCurrentCommand != NULL) {
        free (RoadMapVoiceCurrentCommand);
    }
    if (RoadMapVoiceCurrentArguments != NULL) {
        free (RoadMapVoiceCurrentArguments);
    }
    RoadMapVoiceCurrentCommand = strdup (name);
    RoadMapVoiceCurrentArguments = strdup (arguments);

    roadmap_spawn_with_feedback (name, arguments, &RoadMapVoiceActive);
}


static void roadmap_voice_queue (const char *name, const char *arguments) {

    if (RoadMapVoiceInUse) {

        roadmap_log(ROADMAP_DEBUG, "queuing message: %s", arguments);

        /* Replace the previously queued message (too old now). */

        if (RoadMapVoiceNextCommand != NULL) {
            free(RoadMapVoiceNextCommand);
        }
        RoadMapVoiceNextCommand = strdup (name);

        if (RoadMapVoiceNextArguments != NULL) {
            free(RoadMapVoiceNextArguments);
        }
        RoadMapVoiceNextArguments = strdup (arguments);

        roadmap_spawn_check ();

    } else {

        roadmap_voice_launch (name, arguments);
    }
}


static void roadmap_voice_complete (void *data) {

    if (RoadMapVoiceNextCommand != NULL) {

        /* Play the queued message now. */

        roadmap_voice_launch
            (RoadMapVoiceNextCommand, RoadMapVoiceNextArguments);

        free (RoadMapVoiceNextCommand);
        RoadMapVoiceNextCommand = NULL;

        if (RoadMapVoiceNextArguments != NULL) {
            free (RoadMapVoiceNextArguments);
            RoadMapVoiceNextArguments = NULL;
        }

    } else {

        /* The sound device is now available (as far as we know). */

        roadmap_log(ROADMAP_DEBUG, "voice now idle");

        RoadMapVoiceInUse = 0;
    }
}


static int roadmap_voice_expand (const char *input, char *output, int size) {

    int length;
    int acronym_length;
    const char *acronym;
    struct voice_translation *cursor;
    const char *acronym_found;
    struct voice_translation *cursor_found;

    if (size <= 0) {
        return 0;
    }

    acronym = input;
    acronym_length = 0;
    acronym_found = input + strlen(input);
    cursor_found  = NULL;

    for (cursor = RoadMapVoiceTranslation; cursor->from != NULL; ++cursor) {

        acronym = strstr (input, cursor->from);
        if (acronym != NULL) {
            if (acronym < acronym_found) {
                acronym_found = acronym;
                cursor_found  = cursor;
            }
        }
    }

    if (cursor_found == NULL) {
        strncpy (output, input, size);
        return 1;
    }

    acronym = acronym_found;
    cursor  = cursor_found;
    acronym_length = strlen(cursor->from);

    length = acronym - input;

    if (length > size) return 0;

    /* Copy the unexpanded part, up to the acronym that was found. */
    strncpy (output, input, length);
    output += length;
    size -= length;

    if (size <= 0) return 0;

    if ((acronym_length != 0) &&
        (acronym[acronym_length] == 0 ||
         (! isalnum(acronym[acronym_length])))) {

        /* This is a valid acronym: translate it. */
        length = strlen(cursor->to);
        strncpy (output, cursor->to, size);
        output += length;
        size   -= length;

        if (size <= 0) return 0;

    } else {
        /* This is not a valid acronym: leave it unchanged. */
        strncpy (output, acronym, acronym_length);
        output += acronym_length;
        size   -= acronym_length;
    }


    return roadmap_voice_expand (acronym + acronym_length, output, size);
}


void roadmap_voice_announce (const char *title) {

    int   i;
    char  text[1024];
    char  expanded[1024];
    char *final;
    char *arguments;


    if (RoadMapVoiceMuted) {
       if (strcasecmp
             (roadmap_config_get (&RoadMapVoiceMute), "no") == 0) {
          RoadMapVoiceMuted = 0;
       } else {
          return;
       }
    }

    RoadMapVoiceActive.handler = roadmap_voice_complete;


    for (i = 0; RoadMapVoiceText[i].default_text != NULL; ++i) {

        if (strcmp (title, RoadMapVoiceText[i].config.name) == 0) {
            break;
        }
    }

    if (RoadMapVoiceText[i].default_text == NULL) {
        roadmap_log (ROADMAP_ERROR, "invalid voice %s", title);
        return;
    }

    if (!roadmap_message_format
             (text, sizeof(text),
              roadmap_config_get (&RoadMapVoiceText[i].config)) ||

         (text[0] == 0)) {

       /* No message. */
       return;
    }

    if (roadmap_voice_expand (text, expanded, sizeof(expanded))) {
        final = expanded;
    } else {
        final = text;
    }

    arguments = strchr (final, ' ');

    if (arguments == NULL) {

        roadmap_voice_queue (final, "");

    } else {

        *arguments = 0;

        while (isspace(*(++arguments))) ;

        roadmap_voice_queue (final, arguments);
    }
}


void roadmap_voice_mute (void) {

    RoadMapVoiceMuted = 1;
    roadmap_config_set (&RoadMapVoiceMute, "yes");

    roadmap_spawn_check ();
}

void roadmap_voice_enable (void) {

    RoadMapVoiceMuted = 0;
    roadmap_config_set (&RoadMapVoiceMute, "no");

    roadmap_spawn_check ();
}


void roadmap_voice_initialize (void) {

    int i;

    roadmap_config_declare_enumeration
               ("session", &RoadMapVoiceMute, NULL, "yes", "no", NULL);

    for (i = 0; RoadMapVoiceText[i].default_text != NULL; ++i) {
        roadmap_config_declare
            ("preferences",
             &RoadMapVoiceText[i].config, RoadMapVoiceText[i].default_text,
             NULL);
    }
}
