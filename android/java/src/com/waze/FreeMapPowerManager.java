/**  
 * FreeMapNativePowerManager.java
 *  Provides the power management for the Android platform. Updates the native layer with the   
 *  current battery status
 *   
 * 
 * LICENSE:
 *
 *   Copyright 2009 	@author Alex Agranovich, Waze Ltd.
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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class FreeMapPowerManager extends BroadcastReceiver
{
    /*************************************************************************************************
     *================================= Public interface section
     * =================================
     * 
     */

    /*************************************************************************************************
     * Event handler for the intent, on changed battery level
     * 
     */
    @Override
    public void onReceive( Context context, Intent intent )
    {
        String action = intent.getAction();
        if (Intent.ACTION_BATTERY_CHANGED.equals(action))
        {

            int level = intent.getIntExtra("level", 0);
            int scale = intent.getIntExtra("scale", 100);
            // Set the level
            synchronized (mCurrentLevel)
            {
                mCurrentLevel = (level * 100) / scale;
            }

        }
    }

    /*************************************************************************************************
     * Battery level inspector
     * 
     */
    public int getCurrentLevel()
    {
        int retVal;
        synchronized (mCurrentLevel)
        {
            retVal = mCurrentLevel;
        }
        return retVal;
    }
    
    /*************************************************************************************************
     *================================= Private interface section =================================
     */

    /*************************************************************************************************
     *================================= Data members section
     * =================================
     * 
     */

	private Integer mCurrentLevel = 100; // Battery level in percents
	//private static final String LOG_TAG = "FreeMapPowerManager"; // Log Tag
};
