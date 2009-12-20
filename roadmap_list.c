/* roadmap_list.c - Manage a list.
 *
 * LICENSE:
 *
 *
 *   This file is part of RoadMap.
 *
 *   Current linked list implementation borrowed from gpsbabel's
 *   queue.c, which is:
 *       Copyright (C) 2002 Robert Lipe, robertlipe@usa.net
 *
 *   Previous implementation, and API spec, 
 *       Copyright 2002 Pascal F. Martin
 *
 *   Merge done by Paul Fox, 2006
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
 *   See roadmap_list.h.
 */

#include "roadmap_list.h"

/* places "new" after "existing" */
void roadmap_list_enqueue(RoadMapListItem *new, RoadMapListItem *existing)
{
        new->next = existing->next;
        new->prev = (RoadMapListItem *)existing;
        existing->next->prev = new;
        existing->next = new;
}

RoadMapListItem * roadmap_list_remove(RoadMapListItem *element)
{
        queue *prev = element->prev;
        queue *next = element->next;

        next->prev = prev;
        prev->next = next;

        element->prev = element->next = element;

        return element;
}

int roadmap_list_count(const RoadMapList *qh)
{
        RoadMapListItem *e;
        int i = 0;

        for (e = QUEUE_FIRST(qh); e != (queue *)qh; e = QUEUE_NEXT(e))
                i++;

        return i;
}
