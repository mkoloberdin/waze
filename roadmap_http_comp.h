/* roadmap_http_comp.h
 *
 * LICENSE:
 *
 *   Copyright 2010 Ehud Shabtai
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
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

#ifndef  __ROADMAP_HTTP_COMP_H__
#define  __ROADMAP_HTTP_COMP_H__

#include "roadmap.h"

struct roadmap_http_comp_t;
typedef struct roadmap_http_comp_t *RoadMapHttpCompCtx;

RoadMapHttpCompCtx roadmap_http_comp_init (void);
void roadmap_http_comp_add_data (RoadMapHttpCompCtx ctx, int received);
int  roadmap_http_comp_read (RoadMapHttpCompCtx ctx, void *data, int size);
void roadmap_http_comp_close(RoadMapHttpCompCtx ctx);

int roadmap_http_comp_avail (RoadMapHttpCompCtx ctx);
void roadmap_http_comp_get_buffer (RoadMapHttpCompCtx ctx,
      void **ctx_buffer, int *ctx_buffer_size);
#endif   // __ROADMAP_HTTP_COMP_H__
