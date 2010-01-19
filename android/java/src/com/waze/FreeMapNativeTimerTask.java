/**  
 * FreeMapNativeTimerTask.java
 * Basic timer task class extends the TimerTask abstract class    
 *  providing the implementation for the run()
 *   
 * 
 * LICENSE:
 *
 *   Copyright 2008 	@author Alex Agranovich
 *   
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
 * SYNOPSYS:
 *
 *   @see FreeMapNativeActvity.java
 */

package com.waze;

import java.util.TimerTask;

public final class FreeMapNativeTimerTask extends TimerTask implements IFreeMapMessageParam 
{

	/*************************************************************************************************
     *================================= Public interface section =================================
     * 
     */
    FreeMapNativeTimerTask(int aTaskId, int aTaskNativeMsg,
            int interval, FreeMapNativeManager aNativeManager)
    {
        mNativeManager = aNativeManager;
        mTaskId = aTaskId;
        mInterval = interval;
        mTaskNativeMsg = aTaskNativeMsg;
        mIsActive = true;
    }

    /*************************************************************************************************
     * Message parameter active inspector (interface override)
     */
    public boolean IsActive()
    {
        return mIsActive;
    }
    
    /*************************************************************************************************
     * Posting the message to the activity thread upon timeout
     * 
     */
    @Override
    synchronized public void run()
    {
        // Log.i("TimerTask Timeout", "Now: " + String.valueOf( (new
        // Date()).getTime() ) + " Id: " + String.valueOf( mTaskId ) );
    	if (mInterval < 100) {
    		mNativeManager.PostPriorityNativeMessage( mTaskNativeMsg, this );
    	} else {
    		mNativeManager.PostNativeMessage( mTaskNativeMsg, this );
    	}
    }

    /*************************************************************************************************
     * Task id modifier
     * 
     */
    public void setTaskId( int aTaskId )
    {
        mTaskId = aTaskId;
    }

    /*************************************************************************************************
     * Task id inspector
     * 
     */
    public int getTaskId()
    {
        return mTaskId;
    }
    /*************************************************************************************************
     * task cancel routine override
     * 
     */
    public boolean cancel()
    {
        mIsActive = false;
        return super.cancel();
    }

    /*************************************************************************************************
     * Synchronized task cancel routine
     * 
     */
    synchronized public void CancelSync()
    {
        this.cancel();
    }

    /*************************************************************************************************
     *================================= Private interface section
     * =================================
     */

    /*************************************************************************************************
     *================================= Data members section
     * =================================
     * 
     */
    private int mTaskId; // The id of the task
	private int mTaskNativeMsg; // The message to be posted to the native layer
    // on timer expiration
    private int mInterval; // timer interval
    private FreeMapNativeManager mNativeManager;
    
    private boolean mIsActive = false;
}
