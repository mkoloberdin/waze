/* roadmap_thread.cpp - Basic asynchronous task execution interface implementation in Symbian 
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
extern "C" {
	#include "roadmap.h"
}
#include <e32base.h>
#include <e32std.h>
#include "GSConvert.h"
#include "roadmap_thread.h"
#include "WazeAsyncTask.h"

_LIT( KErrThreadPanic, "Thread panic" );

typedef struct SYMThreadContext
{
	RMThreadFunc func;
	void* context;
};

static TInt roadmap_thread_func_wrapper( TAny* aContext );

EXTERN_C BOOL roadmap_thread_run ( RMThreadFunc func, void* context, RMThreadPriority priority, const char* name, BOOL separate_thread )
{
	BOOL res = TRUE;
	if ( separate_thread )
	{
		res = roadmap_thread_run_separate( func, context, priority, name );
	}
	else
	{
		res = roadmap_thread_run_same( func, context, priority, name );	
	}
	return res;
}
EXTERN_C BOOL roadmap_thread_run_separate( RMThreadFunc func, void* context, RMThreadPriority priority, const char* name )
{
	RThread thread;
	const TInt KHeapSize = 0x800;
	TBuf<128> buf;
	TThreadPriority sym_priority;
	BOOL res = TRUE;
	
	
	if ( name == NULL )
	{
		roadmap_log( ROADMAP_ERROR, "Error creating thread: name must be supplied! " );
		return FALSE;
	}
	GSConvert::CharPtrToTDes16( name, buf );
	
	SYMThreadContext* sym_thread_ctx = (SYMThreadContext*) User::Alloc( sizeof( SYMThreadContext ) );
	sym_thread_ctx->func = func;
	sym_thread_ctx->context = context;
	TInt err = thread.Create( buf, (TThreadFunction) roadmap_thread_func_wrapper, KDefaultStackSize, KMinHeapSize, KHeapSize, 
			(TAny*) sym_thread_ctx, EOwnerThread );
	
	if ( err != KErrNone )
	{
		roadmap_log( ROADMAP_ERROR, "Error creating thread: %s. Error: %d", name, err );
		res = FALSE;
	}
	else
	{
		switch( priority )
		{
			case _priority_idle:
			{
				sym_priority = EPriorityAbsoluteVeryLow;
			}
			case _priority_low:
			{
				sym_priority = EPriorityLess;
			}
			case _priority_normal:
			{
				sym_priority = EPriorityNormal;
			}
			case _priority_high:
			{
				sym_priority = EPriorityMore;
			}
			case _priority_realtime:
			{
				sym_priority = EPriorityRealTime;
			}
			default:
			{
				sym_priority = EPriorityNormal;
			}
		}
		thread.SetPriority( sym_priority );
		thread.Resume();
	}
	return res;
}

/*
 * The wrapper for the thread function
 */
static TInt roadmap_thread_func_wrapper( TAny* aContext )
{
	SYMThreadContext* ctx = (SYMThreadContext*) aContext;
	CTrapCleanup* cleanup = CTrapCleanup::New();
	
	TRAPD( error, ctx->func( ctx->context ) );
	
	User::Free( ctx );
	
	delete cleanup;
	return error;
}

EXTERN_C BOOL roadmap_thread_run_same( RMThreadFunc func, void* context, RMThreadPriority priority, const char* name )
{
	TInt async_task_priority;
	switch( priority )
	{
		case _priority_idle:
		{
			async_task_priority = CActive::EPriorityIdle;
		}
		case _priority_low:
		{
			async_task_priority = CActive::EPriorityLow;
		}
		case _priority_normal:
		{
			async_task_priority = CActive::EPriorityStandard;
		}
		case _priority_high:
		{
			async_task_priority = CActive::EPriorityHigh;
		}
		case _priority_realtime:
		{
			async_task_priority = CActive::EPriorityUserInput;
		}
		default:
		{
			async_task_priority = CActive::EPriorityStandard;
		}
	}
	CWazeAsyncTask* task = CWazeAsyncTask::NewL( func, context, async_task_priority );
	/*
	 * Self destroying object
	 */
	task->Post();
	return TRUE;
}
