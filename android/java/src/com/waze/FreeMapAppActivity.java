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

import com.waze.WazeScreenManager.WazeScreenManagerCallback;

import dalvik.system.VMRuntime;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.media.AudioManager;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
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
//         Log.w( "WAZE DEBUG", "ON CREATE" );
        // Window without the title and status bar
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        // Sets the volume context to be media ( ringer is the default ) 
        setVolumeControlStream( AudioManager.STREAM_MUSIC );
        // Restrict orientation change until full initialization
        setRequestedOrientation( ActivityInfo.SCREEN_ORIENTATION_PORTRAIT );

        // Set the current configuration
        mOrientation = getResources().getConfiguration().orientation;
        
        // Every time activity creation - new manager should be created
    	mLayoutMgr = new WazeLayoutManager( this );
        setContentView( mLayoutMgr.getMainLayout() );
        
        if ( !FreeMapAppService.IsAppRunning() )
        {
	            // Starting the service
                WazeIntentManager.HandleUri( getIntent().getData(), this );
                if ( !FreeMapAppService.IsInitialized() )
                {
                	Intent intent = new Intent( this, FreeMapAppService.class );
                	/*
                	 * Pay attention that onStart event of the service comes later than 
                	 * onResume of activity. In the future the start flow should be in the onStart
                	 * of the service 
                	 */
	            	startService( intent );
                }
	            FreeMapAppService.StartApp( this );
        }
        else
        {
            FreeMapAppService.setMainContext( this );
        }

        mNativeManager = FreeMapAppService.getNativeManager();        
    }
    
    /*************************************************************************************************
     * Resume event override
     * 
     */
    @Override
    public void onResume()
    {
    	super.onResume();
    	
//    	 Log.w( "WAZE DEBUG", "ON RESUME" );
    	/*
         * Allows the system to destroy the activity 
         */
        FreeMapAppService.setMainContext( this );
        
        mIsRunning = true;
        
        /*
         * NOT WORKING!!! The application doesn't receive the intents
         * More investigation is necessary
         *
         RegisterMediaBCReceivers();
         */
        
        // Register on the intent providing the battery level inspection
        registerReceiver( FreeMapAppService.getPowerManager(), new IntentFilter(
                Intent.ACTION_BATTERY_CHANGED));  

        // Set the screen timeout
        if ( mNativeManager.getInitializedStatus() )
        {
        	mNativeManager.SaveSystemSettings();
            mNativeManager.RestoreAppSettings();
            OnResumeScreen();
        }
        /*
         * Check if the intent contains the url query
         */
    	final Intent intent = getIntent();
        WazeIntentManager.HandleUri( intent.getData(), this );    		
    }

    /*************************************************************************************************
     * Pause event override
     * 
     */
    @Override
    public void onPause()
    {
        super.onPause();
        // Log.w( "WAZE DEBUG", "ON PAUSE" );
        /*
         * Allows the system to destroy the activity 
         */
        FreeMapAppService.setMainContext( null );
        
        mIsRunning = false;
        if ( mNativeManager.getInitializedStatus() )
        {
            WazeScreenManager screenMgr = FreeMapAppService.getScreenManager();
            screenMgr.onPause();
        	unregisterReceiver( FreeMapAppService.getPowerManager() );               	
            // Restore the system settings        
            mNativeManager.RestoreSystemSettings();  
        }
    }

    protected void onNewIntent ( Intent aIntent )
    {
    	setIntent( aIntent );
    }   
    
    protected void onStart ()
    {
    	super.onStart();
    }
    /*************************************************************************************************
     * Stop event override
     * 
     */
    @Override
    public void onStop()
    {
    	super.onStop();
    }
    /*************************************************************************************************
     * This event override allows control over the keyboard sliding
     *  
     */
    @Override 
    public void onConfigurationChanged( Configuration newConfig )
    {
    	
    	// AGA DEBUG
    	Log.i("WAZE", "Configuration changed: " + newConfig.orientation );
    	
    	/* Orientation changed event */
    	if ( ( mOrientation != newConfig.orientation ) && 
    			( getRequestedOrientation() == ActivityInfo.SCREEN_ORIENTATION_USER ) )
    	{
    		mIsMenuRebuild = true;
    		mOrientation = newConfig.orientation;
    		mLayoutMgr.getMainLayout().requestLayout();
    	}
        // Stop the screen draw until the new buffer is ready
    	super.onConfigurationChanged(newConfig);        
    }    

    /*************************************************************************************************
     * Handles the menu selections
     * 
     */
    @Override
    public boolean onOptionsItemSelected( MenuItem item ) 
    { 
    	boolean res = true;
    	if ( mNativeManager.getInitializedStatus() )
    	{
    		WazeMenuManager menuMgr = mNativeManager.getMenuManager();
    		res = menuMgr.OnMenuButtonPressed( item.getItemId() );
    	}
        return res;
    }
    
    /*************************************************************************************************
     * Prepares the menu for display
     * 
     */
    @Override
    public boolean onPrepareOptionsMenu( Menu aMenu )
    {    	
    	if ( mIsMenuRebuild )
    	{    		
    		// Rebuild the menu
    		WazeMenuManager menuMgr = mNativeManager.getMenuManager();
    		menuMgr.BuildOptionsMenu( aMenu, mOrientation == Configuration.ORIENTATION_PORTRAIT );
    		mIsMenuRebuild = false;
    	}    	
    	return true;
    }

    
    /*************************************************************************************************
     * Creates the menu for display
     * 
     */
    public boolean  onCreateOptionsMenu ( Menu aMenu )
    {
    	boolean res = false;
    	mOptMenu = aMenu;		// Save the reference 
    	if ( mNativeManager.getInitializedStatus() )
    	{
    		WazeMenuManager menuMgr = mNativeManager.getMenuManager();
    		menuMgr.BuildOptionsMenu( aMenu, mOrientation == Configuration.ORIENTATION_PORTRAIT );
    		res = true;
    	}
    	return res;
    }
    /*************************************************************************************************
     * Is the activity is running
     * 
     */
    public boolean IsRunning()
    {
        return mIsRunning;
    }
    /*************************************************************************************************
     * Returns the Main view
     * 
     */
    WazeMainView getMainView()
    {
        return mLayoutMgr.getAppView();
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
     * Returns the Layout manager
     * 
     */
    WazeLayoutManager getLayoutMgr()
    {
        return mLayoutMgr;
    }
    /*************************************************************************************************
     * Returns the current view reference
     * 
     */
    View getCurrentView()
    {
    	return mAppView;
    }
    /*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */
    
    private void OnResumeScreen()
    {
        final WazeScreenManager screenMgr = FreeMapAppService.getScreenManager();
        
        /*
         * Progress hide event 
         */
        final Runnable hideProgressEvent = new Runnable() {				
			public void run() {
				mLayoutMgr.HideProgressView();
			}
		};
    	/*
    	 * Build callback to hide the progress bar
    	 */
//    	final WazeScreenManagerCallback progressViewCb = new WazeScreenManagerCallback() {    			
//			public void Callback() {
//				runOnUiThread( hideProgressEvent );
//				
//			}
//		};

		final Runnable showProgressEvent = new Runnable() {				
			public void run() {
				//Log.w( "WAZE DEBUG", " Running showProgressEvent" );
		        // Show progress view 		         
		        mLayoutMgr.ShowProgressView();
			}
        };

        final Runnable scrMgrResumeEvent = new Runnable() {				
			public void run() {
				//Log.w( "WAZE DEBUG", " Running scrMgrResumeEvent" );
		        // Screen manager event
				screenMgr.onResume( null );				
			}
		};
		
		// Place the event on the UI event queue ( Delayed to make sure execution in the different stack )
		getMainView().postDelayed( showProgressEvent, 10 );

		// Place the event on the UI event queue ( Delayed to make sure execution in the different stack and after progress is shown )
		getMainView().postDelayed( scrMgrResumeEvent, 20 );
    }
    
    private void RegisterMediaBCReceivers()
    {
        // Register the sdcard receiver
        IntentFilter intentFilter = new IntentFilter( Intent.ACTION_MEDIA_REMOVED );
        intentFilter.addAction( Intent.ACTION_MEDIA_UNMOUNTED );
        intentFilter.addDataScheme( "file" );        
        WazeIntentManager.WazeSDCardManager sdcardMgr = new WazeIntentManager.WazeSDCardManager(); 
        registerReceiver( sdcardMgr, intentFilter );
        
        intentFilter = new IntentFilter( Intent.ACTION_EXTERNAL_APPLICATIONS_UNAVAILABLE );
        intentFilter.addAction( Intent.ACTION_MEDIA_UNMOUNTED );
        intentFilter.addDataScheme( "file" );        
		  sdcardMgr = new WazeIntentManager.WazeSDCardManager(); 
        registerReceiver( sdcardMgr, intentFilter );
    }
    
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

    private WazeLayoutManager	 mLayoutMgr = null;		   // Main layout manager
    private WazeMainView       mAppView = null;          // Application view
    private FreeMapNativeManager mNativeManager = null;    // Native manager    

    private int 				 mOrientation;			   // Screen orientation in that the application works now   
    private boolean				 mIsMenuRebuild = false;   // Indicates that the menu rebuild is necessary on prepare
    private Menu				 mOptMenu = null;		   // Reference to the options menu of the application			 
    
    
	private volatile boolean 	mIsRunning = false;		  // Indicates if the activity is running.

    private ViewAnimator mViewAnimator = null;
    
	/*************************************************************************************************
     *================================= Constants section =================================
     */
    public static final long INITIAL_HEAP_SIZE = 4096L;
    public static final boolean TEST_PNG = false; 
    
}
