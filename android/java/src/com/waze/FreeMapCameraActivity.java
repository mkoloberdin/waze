/**  
 * FreeMapCameraActivity.java
 * This class represents the android layer activity wrapper for the activity.  
 * This activity represents the camera interaction context
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
 *   @see FreeMapCameraView.java
 */

package com.waze;

import android.app.Activity;
import android.graphics.PixelFormat;
import android.os.Bundle;
import android.view.Window;
import android.view.WindowManager;

public class FreeMapCameraActivity extends Activity
{
    /*************************************************************************************************
     *================================= Public interface section
     * =================================
     * 
     */
    @Override
    public void onCreate( Bundle savedInstanceState )
    {
        /** Called when the activity is first created. */
        super.onCreate(savedInstanceState);
        // Window without the title and status bar
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
        // Creating the preview object
        mCameraPreView = new FreeMapCameraPreView(this);

        getWindow().setFormat(PixelFormat.TRANSLUCENT);
        setContentView(mCameraPreView);
    }

    @Override
    protected void onResume()
    {
        super.onResume();
    };

    @Override
    protected void onPause()
    {
        super.onPause();
    };

    @Override
    protected void onStop()
    {
        super.onPause();
        finish();
    };

    /*************************************************************************************************
     *================================= Private interface section
     * =================================
     * 
     */

    /*************************************************************************************************
     *================================= Data members section
     * =================================
     * 
     */

    FreeMapCameraPreView        mCameraPreView;
    // private static final String LOG_TAG = "FreeMapCameraActivity";
}
