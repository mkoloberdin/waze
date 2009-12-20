/* roadmap_list.h - Manage a list.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *
 *   Merged with queue.h, from the gpsbabel program, which is
 *     Copyright (C) 2002-2005 Robert Lipe, robertlipe@usa.net
 *   by Paul Fox, 2006.
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

#ifndef INCLUDE__ROADMAP_LIST__H
#define INCLUDE__ROADMAP_LIST__H

/* Linked lists are fully circular -- no pointer is ever NULL. 
 * When empty, a list head's "list_first" and "list_last"
 * pointers point at the list head itself.
 * 
 * The head of a list is identical to any element in the list. 
 * To aid in debugging, the head is given a different type --
 * this helps reduce confusion when calling the insert/remove
 * routines, by letting the compiler enforce type safety.
 *
 * However, the gcc optimizer complains about some of the casts
 * ("dereferencing type-punned pointer will break strict-aliasing
 * rules") when the list head and the items have separate
 * typedefs.  So, when optimizing, we simply declare them both as
 * the same thing.
 *
 * [ Neither Pascal's nor Robert Lipe's original code had this
 * problem.  The former was always typesafe, and the latter
 * always unsafe.  So the current situation is my fault.  -pgf ]
 */

#include "roadmap.h"

struct roadmap_list_link {
    struct roadmap_list_link *next;
    struct roadmap_list_link *prev;
};

typedef struct roadmap_list_link RoadMapListItem;

#if ROADMAP_LISTS_TYPESAFE

/* Separate list-head structure. */
struct roadmap_list_head {
    struct roadmap_list_link *list_first;
    struct roadmap_list_link *list_last;
};

typedef struct roadmap_list_head RoadMapList;

#else

/* Use the same struct for both the list head and the items. */
#define list_first next
#define list_last  prev
typedef struct roadmap_list_link RoadMapList;

#endif

/* List heads must be initialized before first use.  This
 * initialization could be done with a declaration like:
 *  RoadMapList MyListHead = 
 *      { (RoadMapListItem *)&MyListHead, (RoadMapListItem *)&MyListHead };
 * but the runtime macro seems cleaner:
 */
#define ROADMAP_LIST_INIT(head) (head)->list_first = \
                ((head)->list_last = (queue *)head)

/* The enqueue routine works on both list heads and list items.  The
 * assumption is that the two are isomorphic.
 */
void roadmap_list_enqueue(RoadMapListItem *before, RoadMapListItem *after);

/* Removes an item from whatever list it's part of */
RoadMapListItem *roadmap_list_remove(RoadMapListItem *item);

/* Returns the number of elements in a list */
int roadmap_list_count(const RoadMapList *list);

/* Inline functions (not macros) to help enforce type safety. */
static inline void roadmap_list_append(RoadMapList *lh, RoadMapListItem *e) {
        roadmap_list_enqueue(e, (RoadMapListItem *)lh->list_last);
}
static inline void roadmap_list_insert(RoadMapList *lh, RoadMapListItem *e) {
        roadmap_list_enqueue(e, (RoadMapListItem *)lh);
}
static inline void roadmap_list_put_before
                (RoadMapListItem *olde, RoadMapListItem *newe) {
        roadmap_list_enqueue(newe, olde->prev);
}
static inline void roadmap_list_put_after
                (RoadMapListItem *olde, RoadMapListItem *newe) {
        roadmap_list_enqueue(newe, olde);
}

#define ROADMAP_LIST_FIRST(head)   ((head)->list_first)
#define ROADMAP_LIST_LAST(head)    ((head)->list_last)
#define ROADMAP_LIST_NEXT(element) ((element)->next)
#define ROADMAP_LIST_PREV(element) ((element)->prev)
#define ROADMAP_LIST_EMPTY(head)   ((head)->list_first == (RoadMapListItem *)(head))

/* Anything attached to oldhead will be re-anchord at newhead.
 * oldhead will be cleared, and anything previously at newhead
 * will be lost.
 */
#define ROADMAP_LIST_MOVE(newhead,oldhead) \
        if ( ROADMAP_LIST_EMPTY(oldhead) ) {\
                ROADMAP_LIST_INIT(newhead); \
        } \
        else { \
                (newhead)->list_first = (oldhead)->list_first; \
                (newhead)->list_last = (oldhead)->list_last; \
                (newhead)->list_first->prev = (RoadMapListItem *)(newhead); \
                (newhead)->list_last->next = (RoadMapListItem *)(newhead); \
        } \
        ROADMAP_LIST_INIT(oldhead)

/* Anything attached to fromhead will be appended at the end of tohead.
 * fromhead is emptied.
 */
#define ROADMAP_LIST_SPLICE(tohead,fromhead) \
        if ( !ROADMAP_LIST_EMPTY(fromhead) ) {\
                (tohead)->list_last->next = (RoadMapListItem *)(fromhead)->list_first; \
                (fromhead)->list_first->prev = (RoadMapListItem *)(tohead)->list_last; \
                (fromhead)->list_last->next = (RoadMapListItem *)(tohead); \
                (tohead)->list_last = (fromhead)->list_last; \
        } \
        ROADMAP_LIST_INIT(fromhead)

#define ROADMAP_LIST_FOR_EACH(listhead, element, tmp) \
        for ((element) = ROADMAP_LIST_FIRST(listhead); \
                (tmp) = ROADMAP_LIST_NEXT(element), \
                (element) != (RoadMapListItem *)(listhead); \
                (element) = (tmp))

#define ROADMAP_LIST_FOR_EACH_FROM_TO(from, to, element, tmp) \
        for ((element) = (from); \
                (tmp) = ROADMAP_LIST_NEXT(element), \
                (element) != (RoadMapListItem *)(to); \
                (element) = (tmp))


/* The rest of the definitions below are to retain source
 * compatibility for the GPX files, which came originally from
 * gpsbabel
 */

typedef struct roadmap_list_link queue;
#if ROADMAP_LISTS_TYPESAFE
typedef struct roadmap_list_head queue_head;
#else
typedef struct roadmap_list_link queue_head;
#endif

#define QUEUE_INIT(head)        ROADMAP_LIST_INIT(head)
#define QUEUE_FIRST(head)       ROADMAP_LIST_FIRST(head) 
#define QUEUE_LAST(head)        ROADMAP_LIST_LAST(head)  
#define QUEUE_EMPTY(head)       ROADMAP_LIST_EMPTY(head)  
#define QUEUE_NEXT(element)     ROADMAP_LIST_NEXT(element) 
#define QUEUE_PREV(element)     ROADMAP_LIST_PREV(element)

#define ENQUEUE_TAIL(listhead, newelement) \
                roadmap_list_append(listhead, newelement)
#define ENQUEUE_HEAD(listhead, newelement) \
                roadmap_list_insert(listhead, newelement)
#define ENQUEUE_AFTER(element, newelement) \
                roadmap_list_put_after(element, newelement)
#define ENQUEUE_BEFORE(element, newelement) \
                roadmap_list_put_before(element, newelement)

#define QUEUE_MOVE(newhead,oldhead) ROADMAP_LIST_MOVE(newhead,oldhead) 
#define QUEUE_SPLICE(tohead,fromhead) ROADMAP_LIST_SPLICE(tohead,fromhead)

#define QUEUE_FOR_EACH(listhead, element, tmp) \
                ROADMAP_LIST_FOR_EACH(listhead, element, tmp)
#define QUEUE_FOR_EACH_FROM_TO(from, to, element, tmp) \
                ROADMAP_LIST_FOR_EACH_FROM_TO(from, to, element, tmp)



#endif // INCLUDE__ROADMAP_LIST__H
