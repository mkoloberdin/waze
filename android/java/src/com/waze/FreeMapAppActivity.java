/**  
 * FreeMapAppActivity.java
 * This class represents the android layer activity wrapper for the activity. Android based applications and interfaces 
 * should interact with this class. Inherits FreeMapNativeActivity representing the native layer of the activity
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
 *   @see FreeMapNativeActivity_JNI.h/c
 */

package com.waze;

import dalvik.system.VMRuntime;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.media.AudioManager;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.ViewAnimator;

public final class FreeMapAppActivity extends Activity
{

    /*************************************************************************************************
     *================================= Public interface section =================================
     * 
     */
    static
    {
        // Create the waze log singleton
        WazeLog.create();      
    	// Eliminate extra GCs during startup by setting the initial heap size to 4MB.
    	VMRuntime.getRuntime().setMinimumHeapSize( FreeMapAppActivity.INITIAL_HEAP_SIZE );
    }


    
    @Override
    public void onCreate( Bundle savedInstanceState )
    {
        /** Called when the activity is first created. */
        super.onCreate(savedInstanceState);
        // Window without the title and status bar
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        // Sets the volume context to be media ( ringer is the default ) 
        setVolumeControlStream( AudioManager.STREAM_MUSIC ); 
        
        // Log.w( "Waze", "Current process PID " + Process.myPid() + " TID " + Process.myTid() );
        // First time activity creation
        if ( savedInstanceState == null || !FreeMapAppService.isInitialized() )
        {           
            // View animator for the view switching
            mViewAnimator = new ViewAnimator( this );
            // Construct and show the view
            mAppView = new FreeMapAppView( this );
            mViewAnimator.addView( mAppView, WAZE_VIEW_INDEX_MAIN );
            // FFU: Edit text view
            mEditTextView = new WazeEditText( this );
            mViewAnimator.addView( mEditTextView, WAZE_VIEW_INDEX_EDIT );
    
	            // Starting the service
	            Intent intent = new Intent( this, FreeMapAppService.class );
	            startService( intent );
	            FreeMapAppService.StartApp( this, mAppView );
        }
        else
        {
            FreeMapAppService.setMainContext( this );
            mAppView = FreeMapAppService.getAppView();
            if ( mAppView.getParent() != null )
                ( (ViewGroup) mAppView.getParent() ).removeView( mAppView );
        }
        
        setContentView( mViewAnimator );
        mViewAnimator.setDisplayedChild( WAZE_VIEW_INDEX_MAIN );
        
        mNativeManager = FreeMapAppService.getNativeManager();
        mNativeManager.setAppActivity( this );
        mPowerManager = FreeMapAppService.getPowerManager();
    }

    /*************************************************************************************************
     *Returns the status of the battery at the moment
     * 
     */
    public int getBatteryLevel()
    {
        return mPowerManager.getCurrentLevel();
    }

    /*************************************************************************************************
     * Resume event override
     * 
     */
    @Override
    public void onResume()
    {
        super.onResume(); 
        
        mAppView.HideSoftInput();
        // Register on the intent providing the battery level inspection
        registerReceiver(mPowerManager, new IntentFilter(
                Intent.ACTION_BATTERY_CHANGED));
        // Set the screen timeout
        if ( mNativeManager.getInitializedStatus() )
        {
            mNativeManager.LoadSystemSettings();
            mNativeManager.SetBackLightOn();
        }
    }

    /*************************************************************************************************
     * Pause event override
     * 
     */
    @Override
    public void onPause()
    {
        super.onPause();
        if ( mNativeManager.getInitializedStatus() )
        {
            unregisterReceiver(mPowerManager);
            
            // Restore the system settings        
            mNativeManager.RestoreSystemSettings();
            mNativeManager.StopBackLightOn();
        }
    }
    /*************************************************************************************************
     * This event override allows control over the keyboard sliding
     * 
     */
    @Override
    public void onConfigurationChanged( Configuration newConfig )
    {
//        Log.i("FreeMapAppActivity", "onConfigurationChanged");

        // Stop the screen draw until the new buffer is ready
        mAppView.setWillNotDraw(true);
        mNativeManager.setCanvasBufReady(false);
        super.onConfigurationChanged(newConfig);
    }    
    
    /*************************************************************************************************
     * Creates the menu items
     * 
     *
    @Override    
    public boolean onCreateOptionsMenu( Menu aMenu ) 
    {
    	if ( mNativeManager.getInitializedStatus() )
    	{
    		WazeMenuManager menuMgr = mNativeManager.getMenuManager();
    		menuMgr.BuildOptionsMenu( aMenu );
    	}
    	return true;
    }
    */
    /*************************************************************************************************
     * Handles the menu selections
     * 
     */
    @Override
    public boolean onOptionsItemSelected( MenuItem item ) 
    {
    	if ( mNativeManager.getInitializedStatus() )
    	{
    		WazeMenuManager menuMgr = mNativeManager.getMenuManager();
    		menuMgr.OnMenuButtonPressed( item.getItemId() );
    	}
        return true;
    }
    
    /*************************************************************************************************
     * Prepares the menu for display
     * 
     */
    @Override
    public boolean onPrepareOptionsMenu( Menu aMenu )
    {
    	boolean res = false;
    	if ( mNativeManager.getInitializedStatus() )
    	{
    		WazeMenuManager menuMgr = mNativeManager.getMenuManager();
    		menuMgr.BuildOptionsMenu( aMenu );
    		res = true;
    	}
    	return res;
    }

    /*************************************************************************************************
     * Returns the View animator
     * 
     */
    ViewAnimator getAnimator()
    {
        return mViewAnimator;
    }
    
    /*************************************************************************************************
     * Returns the current view reference
     * 
     */
    View getCurrentView()
    {
    	return mViewAnimator.getCurrentView();
    }
    /*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */
    /*************************************************************************************************
     * Callback class for the sdcard warning message
     * 
     */
    private static class SDCardWarnClickListener implements 
    DialogInterface.OnClickListener
    {
    	public void onClick( DialogInterface dialog, int which )
    	{
    		
			dialog.cancel(); 
			
			Activity ctx = (Activity) ( (AlertDialog) dialog ).getContext();
			ctx.finish();
    	}
    }
    /*************************************************************************************************
     * Sdcard availability checker - shows the appropriate message if not available
     * 
     */
    private boolean IsSdCardAvailable()
    {
        boolean bRes = true;
        AlertDialog.Builder dlgBuilder = new AlertDialog.Builder( this );
        dlgBuilder.setMessage( "The sdcard is not available - probably it is not present or mounted. \n" +
        		"To run Waze you need sdcard available (not mounted) in your phone!" );
        dlgBuilder.setTitle( "Warning" );
        dlgBuilder.setCancelable( false );
        
        if ( !android.os.Environment.getExternalStorageState().equals(
                android.os.Environment.MEDIA_MOUNTED ) )
        {
	        dlgBuilder.setNeutralButton( "Ok", new SDCardWarnClickListener() );
	        AlertDialog dlg = dlgBuilder.create();
	        dlg.show();
	
	        
	        bRes = false;
        }  
    	return bRes;
    }
    
    /*************************************************************************************************
     * This event override allows control over the keyboard sliding
     * 
     */

    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */
    private FreeMapAppView       mAppView = null;          // Application view
    private WazeEditText         mEditTextView = null;     // Edit text view
    private FreeMapNativeManager mNativeManager = null;    // Native manager
    private FreeMapPowerManager  mPowerManager = null;     // Power manager

    public static final long INITIAL_HEAP_SIZE = 4096L;
    
    public static final int WAZE_VIEW_INDEX_MAIN = 0;
    public static final int WAZE_VIEW_INDEX_EDIT = 1;
    
    private ViewAnimator mViewAnimator = null;
    
//    private static final String LOG_TAG = "FreeMapAppActivity";

//	  FFU: For the view animator 
//    private static final String STATE_NATIVE_MANAGER = "com.waze.FreeMapAppActivity.mNativeManager";
//    private static final String STATE_POWER_MANAGER = "com.waze.FreeMapAppActivity.mPowerManager";
//    private static final String STATE_APP_VIEW = "com.waze.FreeMapAppActivity.mAppView";
    
    public static final boolean TEST_PNG = false; 
    
}
