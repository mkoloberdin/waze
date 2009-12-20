/* roadmap_httpcopy_async.h - Download RoadMap maps using the HTTP protocol asynchornously.
 *
 * LICENSE:
 *
 *   Copyright 2003 Pascal Martin.
 *   Copyright 2009 Israel Disatnik.
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

#ifndef INCLUDED__ROADMAP_HTTPCOPY__H
#define INCLUDED__ROADMAP_HTTPCOPY__H

#include <time.h>

typedef int  (*RoadMapHttpAsyncCallbackSize)     (void *context, size_t size);
typedef void (*RoadMapHttpAsyncCallbackProgress) (void *context, char *data, size_t size);
typedef void (*RoadMapHttpAsyncCallbackError)    (void *context, int connection_failure, const char *format, ...);
typedef void (*RoadMapHttpAsyncCallbackDone)     (void *context, char *last_modified);

typedef struct {
   RoadMapHttpAsyncCallbackSize     size;
   RoadMapHttpAsyncCallbackProgress progress;
   RoadMapHttpAsyncCallbackError    error;
   RoadMapHttpAsyncCallbackDone		done;
} RoadMapHttpAsyncCallbacks;

struct HttpAsyncContext_st;
typedef struct HttpAsyncContext_st HttpAsyncContext;

HttpAsyncContext *roadmap_http_async_copy (RoadMapHttpAsyncCallbacks *callbacks,
									  void *context,
                             const char *source,
                             time_t update_time);
void	roadmap_http_async_copy_abort (HttpAsyncContext *context);


#endif // INCLUDED__ROADMAP_HTTPCOPY__H
