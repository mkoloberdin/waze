/* roadmap_spawn.h - Process control interface for the RoadMap application.
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

#ifndef INCLUDE__ROADMAP_SPAWN__H
#define INCLUDE__ROADMAP_SPAWN__H

#include "roadmap_list.h"


typedef int RoadMapPipe; /* UNIX style. */
#define ROADMAP_SPAWN_INVALID_PIPE   -1
#define ROADMAP_SPAWN_IS_VALID_PIPE(x) (x != ROADMAP_SPAWN_INVALID_PIPE)


typedef void (*RoadMapFeedbackHandler) (void *data);

struct roadmap_spawn_feedback {
    
    RoadMapListItem link;
    int child;

    RoadMapFeedbackHandler handler;
    void *data;
};

typedef struct roadmap_spawn_feedback RoadMapFeedback;
    

void roadmap_spawn_initialize (const char *argv0);

int  roadmap_spawn (const char *name, const char *command_line);

int  roadmap_spawn_with_feedback
         (const char *name,
          const char *command_line,
          RoadMapFeedback *feedback);

int  roadmap_spawn_with_pipe
         (const char *name,
          const char *command_line,
          RoadMapPipe pipes[2],      /* pipes[0] -> read, pipes[1] -> write */
          RoadMapFeedback *feedback);

int  roadmap_spawn_write_pipe (RoadMapPipe pipe, const void *data, int length);
int  roadmap_spawn_read_pipe  (RoadMapPipe pipe, void *data, int size);
void roadmap_spawn_close_pipe (RoadMapPipe pipe);

void roadmap_spawn_check (void);

void roadmap_spawn_command (const char *command);

#endif // INCLUDE__ROADMAP_SPAWN__H

