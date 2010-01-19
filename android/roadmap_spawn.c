/* roadmap_spawn.c - Stub for the android. Dummy funcions
 *
 * LICENSE:
 *
 *   Copyright 2008
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
 *   See roadmap_spawn.h
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

#include "roadmap.h"
#include "roadmap_list.h"
#include "roadmap_spawn.h"


static void roadmap_spawn_set_arguments
               (int argc, char *argv[], const char *command_line) {

}


static void roadmap_spawn_cleanup (void)
{
}

static int roadmap_spawn_child (const char *name,
                                const char *command_line,
                                RoadMapPipe pipes[2])
{
   return 0;
}

void roadmap_spawn_initialize (const char *argv0)
{
}


int roadmap_spawn (const char *name,
                   const char *command_line)
{

   return 0;
}


int  roadmap_spawn_with_feedback
         (const char *name,
          const char *command_line,
          RoadMapFeedback *feedback) {
    return 0;
}


int  roadmap_spawn_with_pipe
         (const char *name,
          const char *command_line,
          int pipes[2],
          RoadMapFeedback *feedback)
{

    return 0;
}


void roadmap_spawn_check (void)
{
}

void roadmap_spawn_command (const char *command)
{
}


int roadmap_spawn_write_pipe (RoadMapPipe pipe, const void *data, int length)
{
   return 0;
}


int roadmap_spawn_read_pipe (RoadMapPipe pipe, void *data, int size)
{
   return 0;
}


void roadmap_spawn_close_pipe (RoadMapPipe pipe)
{
}



