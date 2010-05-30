/* roadmap_thread.c - Basic asynchronous task execution interface implementation in Android
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
#include "roadmap_thread.h"
#include <pthread.h>
#include <utils/threads.h>
#include "roadmap_androidmain.h"
#include "errno.h"

static void* roadmap_thread_func_wrapper( void* context );
static int android_thread_priority( RMThreadPriority priority );

BOOL roadmap_thread_run ( RMThreadFunc func, void* context, RMThreadPriority priority, const char* name, BOOL separate_thread )
{
	pthread_t thread_id;

	if ( separate_thread )
	{
	   struct sched_param params;
	   pthread_attr_t attr;
	   int retVal;
	   RMThreadContext* thread_context;

	   /*
	   int min_priority;
	   int cur_priority;
	   int cur_policy;

	   retVal = pthread_getschedparam( pthread_self(), &cur_policy, &cur_param );
	   LogResult( retVal, "pthread_getschedparam. ", ROADMAP_ERROR );

	   min_priority = sched_get_priority_min( cur_policy );
	   LogResult( retVal, "sched_get_priority_min. ", ROADMAP_ERROR );

	   cur_priority = cur_param.sched_priority;
	   cur_param.sched_priority = min_priority;

	   pthread_attr_init ( &attr );
	   LogResult( retVal, "pthread_attr_init. ", ROADMAP_ERROR );
	   pthread_attr_setschedparam( &attr, &cur_param );
	   LogResult( retVal, "pthread_attr_setschedparam. ", ROADMAP_ERROR );
	    */


	   /*
	    * Set the thread attributes to match the requested priority
	    */
	   retVal = pthread_attr_init ( &attr );
	   LogResult( retVal, "pthread_attr_init. ", ROADMAP_ERROR );

	   params.sched_priority = android_thread_priority( priority );

	   retVal = pthread_attr_setschedparam( &attr, &params );
	   LogResult( retVal, "pthread_attr_setschedparam. ", ROADMAP_ERROR );

	   /*
	    * Preparing the thread context
	    */
	   thread_context = malloc( sizeof( RMThreadContext ) );
	   roadmap_check_allocated( thread_context );
	   thread_context->func = func;
	   thread_context->context = context;
	   /*
	    * Starting the thread
	    */
	   retVal = pthread_create( &thread_id, &attr, ( void *(*)(void *) ) roadmap_thread_func_wrapper, thread_context );
	   LogResult( retVal, "pthread_create. ", ROADMAP_ERROR );
	}
	else
	{
		/*
		 * There is no async implementation meanwhile just execute synchronously
		 */
		func( context );
	}
	return TRUE;
}

/*
 * Start routine wrapper in order to allow customization of the return code to the OS
 */
static void* roadmap_thread_func_wrapper( void* context )
{
	RMThreadContext* thread_context = (RMThreadContext*) context;
	int retVal = 0;

	retVal = thread_context->func( thread_context->context );

	free( thread_context );

	return NULL;
}
/*
 * Dispatches the internal priority to the values of the android priorities
 */
static int android_thread_priority( RMThreadPriority priority )
{
   int droid_priority;
   switch ( priority )
   {
		case _priority_idle:
		{
		   droid_priority = ANDROID_PRIORITY_LOWEST;
		   break;
		}
		case _priority_low:
		{
		   droid_priority = ANDROID_PRIORITY_BACKGROUND;
		   break;
		}
		case _priority_normal:
		{
		   droid_priority = ANDROID_PRIORITY_NORMAL;
		   break;
		}
		case _priority_high:
		{
		   droid_priority = ANDROID_PRIORITY_DISPLAY;
		   break;
		}
		case _priority_realtime:
		{
		   droid_priority = ANDROID_PRIORITY_HIGHEST;
		   break;
		}
		default:
		{
		   droid_priority = ANDROID_PRIORITY_NORMAL;
		   break;
		}
   }
   return droid_priority;
}

