/* WazeAsyncTask.cpp - Waze asynchronous task implementation
 *			* 
 *			* 
 *			*
 *
 * LICENSE:
 *
 *   Copyright 2009, Waze Ltd, Alex Agranovich (AGA)
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

#include "WazeAsyncTask.h"
#include <e32const.h>

CWazeAsyncTask* CWazeAsyncTask::NewL( RMThreadFunc aCallback, TAny* aContext, TInt aPriority )
{
	CWazeAsyncTask *self = NewLC( aCallback, aContext, aPriority );
	CleanupStack::Pop( self );
	return self;
}

CWazeAsyncTask* CWazeAsyncTask::NewLC( RMThreadFunc aCallback, TAny* aContext, TInt aPriority )
{
	CWazeAsyncTask *self = new (ELeave) CWazeAsyncTask( aPriority );
	CleanupStack::PushL( self );
	self->ConstructL( aCallback, aContext );
	return self;
}

CWazeAsyncTask::CWazeAsyncTask( TInt aPriority ) : CActive( aPriority )
{
}

CWazeAsyncTask::~CWazeAsyncTask()
{
	Cancel();
}

inline void CWazeAsyncTask::RunL()
{
	User::LeaveIfError( iStatus.Int() );
	
	if ( iCallback )
	{
		iCallback( iContext );
	}
	// Must be the last statement
	delete this;
}

inline void CWazeAsyncTask::ConstructL( RMThreadFunc aCallback, TAny* aContext )
{
	iCallback = aCallback;
	iContext = aContext;
	CActiveScheduler::Add( this );
}

inline void CWazeAsyncTask::DoCancel()
{	
}

inline TInt CWazeAsyncTask::RunError( TInt aError )
{
	roadmap_log( ROADMAP_WARNING, "Error handling async request: %d", aError );
	return KErrNone;
}

void CWazeAsyncTask::Post()
{
	TRequestStatus* status = &iStatus;
	User::RequestComplete( status, KErrNone );
	SetActive();
}


