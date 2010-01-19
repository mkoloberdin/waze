/* roadmap_spawn.cpp - Process control interface for the RoadMap application.
 *
 * LICENSE:
 *
 *   Copyright 2008 Giant Steps Ltd.
 *   Base off code Copyright 2005 Ehud Shabtai
 *
 *   Based on an implementation by Pascal F. Martin.
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

extern "C" {
#include "roadmap.h"
#include "roadmap_list.h"
#include "roadmap_spawn.h"
}

static char *RoadMapSpawnPath = NULL;

static RoadMapList RoadMapSpawnActive;


static int roadmap_spawn_child (const char *name,
                                const char *command_line,
                                RoadMapPipe pipes[2],
                                int feedback)
{
  //TODO un-stub (using WIN32 code)
  return 0;
}

void roadmap_spawn_initialize (const char *argv0)
{
  //TODO un-stub (using WIN32 code)
}

int roadmap_spawn ( const char *name,
                    const char *command_line)
{
  //return roadmap_spawn_child (name, command_line, NULL, 0);
  return 0;
}

int  roadmap_spawn_with_feedback (const char *name,
                                  const char *command_line,
                                  RoadMapFeedback *feedback)
{
  //roadmap_list_append (&RoadMapSpawnActive, &feedback->link);
  //feedback->child = roadmap_spawn_child (name, command_line, NULL, 1);

  //return feedback->child;
  return 0;
}

int  roadmap_spawn_with_pipe (const char *name,
                              const char *command_line,
                              RoadMapPipe pipes[2],
                              RoadMapFeedback *feedback)
{
  //roadmap_list_append (&RoadMapSpawnActive, &feedback->link);
  //feedback->child = roadmap_spawn_child (name, command_line, pipes, 0);

  //return feedback->child;
  return 0;
}


void roadmap_spawn_check (void)
{
  //TODO un-stub (using WIN32 code)
}


void roadmap_spawn_command (const char *command)
{
   //roadmap_spawn(command, NULL);
}


int roadmap_spawn_write_pipe (RoadMapPipe pipe, const void *data, int length)
{
   return -1;
}


int roadmap_spawn_read_pipe (RoadMapPipe pipe, void *data, int size)
{
   return -1;
}


void roadmap_spawn_close_pipe (RoadMapPipe pipe) {} 
