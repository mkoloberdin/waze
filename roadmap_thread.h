/* roadmap_thread.h - basic thread interface for the per OS implementation
 *
 * LICENSE:
 *
 *   Copyright 2009 Alex Agranovich (AGA)
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
#ifndef ROADMAP_THREAD_H_
#define ROADMAP_THREAD_H_

#include "roadmap.h"
#include "roadmap_types.h"

#define RM_THREAD_MAX_THREAD_NAME 256

typedef enum
{
	_priority_idle = 0,
	_priority_low,
	_priority_normal,
	_priority_high,
	_priority_realtime
} RMThreadPriority;

typedef int (*RMThreadFunc) ( void* context );

typedef struct
{
	RMThreadFunc func;
	void* context;
} RMThreadContext;



/*
 * Executes the thread function with the given priority in separate thread or async request
 * running in the same thread. OS specific implementation
 * @param func: [in] - the function pointer to the thread body function
 * @param context: [in] - the context pointer to be passed to the thread function
 * @param priority: [in] - thread priority
 * @param name:  [in] - thread/task name (usually must be unique)
 * @param separate thread:  [in] - TRUE - execute in the separate thread, FALSE - execute as asynchronous event running in the same thread
 */
EXTERN_C BOOL roadmap_thread_run ( RMThreadFunc func, void* context, RMThreadPriority priority, const char* name, BOOL separate_thread );

/*
 * Executes the thread function as an asynchronous task in the same thread. OS specific implementation
 * @param func: [in] - the function pointer to the thread body function
 * @param context: [in] - the contedifferentaxt pointer to be passed to the thread function
 * @param priority: [in] - task priority
 * @param name:  [in] - thread/task name (usually must be unique)
 */
EXTERN_C BOOL roadmap_thread_run_same( RMThreadFunc func, void* context, RMThreadPriority priority, const char* name );

/*
 * Executes the thread function as an asynchronous task in the same thread. OS specific implementation
 * @param func: [in] - the function pointer to the thread body function
 * @param context: [in] - the contedifferentaxt pointer to be passed to the thread function
 * @param priority: [in] - task priority
 * @param name:  [in] - thread/task name (usually must be unique)
 */
EXTERN_C BOOL roadmap_thread_run_separate( RMThreadFunc func, void* context, RMThreadPriority priority, const char* name );


#endif /* ROADMAP_THREAD_H_ */
