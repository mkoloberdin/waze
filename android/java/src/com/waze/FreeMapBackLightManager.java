/**  
 * FreeMapBackLightController.java
 * Controls the backlight of the application by using timer wake lock    
 * 
 *   
 * 
 * LICENSE:
 *
 *   Copyright 2009, Waze Ltd     @author Alex Agranovich
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
 *   @see FreeMapNativeManager.java
 */

package com.waze;

import java.util.Timer;
import java.util.TimerTask;

import android.content.Context;
import android.os.PowerManager;

public final class FreeMapBackLightManager
{

    /*************************************************************************************************
     *================================= Public interface section  =================================
     * 
     */
    FreeMapBackLightManager( FreeMapNativeManager aManager)
    {
        mNativeManager = aManager;
        mTimer = mNativeManager.getTimer();
        
        Context context = FreeMapAppService.getAppContext();
        
        final PowerManager pm = ( PowerManager ) context.getSystemService( Context.POWER_SERVICE );
        mWakeLock = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK     // Causes stay in the same state 
                                   |PowerManager.ON_AFTER_RELEASE,          // Resets user's activity timer
                                   "FreeMap Wake" );
        
    }

    /*************************************************************************************************
     * Starts waking up with the supplied period
     * Resets the previous task if scheduled 
     * @param aRefreshPeriod - the period in milli seconds
     */
    public void StartWakeMonitor( long aRefreshPeriod )
    {
        if ( mWakeTask != null )
            StopWakerMonitor();
        
        mRefreshPeriod = aRefreshPeriod;
        mWakeTask = new TimerTask()
        {
            @Override
            public void run()
            {
                mWakeLock.acquire();
                mWakeLock.release(); // On after release resets the activity timer 
            }         
        };       
        
        mTimer.scheduleAtFixedRate( mWakeTask, 0, mRefreshPeriod );
    }

    public void StopWakerMonitor()
    {        
        if ( mWakeTask != null )
        {
            mWakeTask.cancel();
            mWakeTask = null;
        }
    }

    /*************************************************************************************************
     *================================= Private interface section =================================
     */

    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */
    
    public static final long WAKE_REFRESH_PERIOD_DEFAULT = 9999L;		// Assume the 10 sec is the minimal screen timeout
    
    private long mRefreshPeriod = WAKE_REFRESH_PERIOD_DEFAULT;

    protected PowerManager.WakeLock mWakeLock;
    
    private TimerTask mWakeTask = null;

    private Timer mTimer = null;
    
    private FreeMapNativeManager mNativeManager;
}
