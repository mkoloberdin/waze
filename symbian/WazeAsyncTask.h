/* WazeAsyncTask.h
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
 *
 *
 *   The interface for the symbian async task class 
 *
 */

#ifndef __WAZE_ASYNC_TASK_H__
#define __WAZE_ASYNC_TASK_H__

#include <e32base.h>

extern "C" {
#include "roadmap_thread.h"
}

class CWazeAsyncTask : public CActive
	{
	public: // New methods

		static CWazeAsyncTask* NewL( RMThreadFunc aCallback, TAny* aContext, TInt aPriority );
		static CWazeAsyncTask* NewLC( RMThreadFunc aCallback, TAny* aContext, TInt aPriority );
		
		/**
		* Post the asynchronous request the task
		*
		* @param aFrame - the frame to be drawn
		* @return void
		*/
		void Post();
		
		virtual ~CWazeAsyncTask();
		
	protected:
		
		  /**
		   * From CActive.
		   * Called when an asynchronous request has completed.
		   */
		  virtual void RunL();
		  /**
		   * From CActive.
		   * Cancels the Active object. Empty.
		   */
		  virtual void DoCancel();
		  /**
		   * From CActive.
		   * Called upon error in the async request
		   */
		  virtual TInt RunError( TInt aError );

		  void ConstructL( RMThreadFunc aCallback, TAny* aContext );

		  
		  CWazeAsyncTask( TInt aPriority  );
		  

	protected:

		TAny* iContext;
		RMThreadFunc iCallback;
};

#endif // __WAZE_ASYNC_TASK_H__

// End of File
