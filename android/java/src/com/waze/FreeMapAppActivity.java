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

//import dalvik.system.VMRuntime;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.media.AudioManager;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.widget.ImageView; 
import android.widget.Toast;
import android.widget.ViewAnimator;
import com.google.android.apps.analytics.GoogleAnalyticsTracker;

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
//    	VMRuntime.getRuntime().setMinimumHeapSize( FreeMapAppActivity.INITIAL_HEAP_SIZE );
    }


    
    @Override
    public void onCreate( Bundle savedInstanceState )
    {
        /** Called when the activity is first created. */
        super.onCreate(savedInstanceState);
        // Log.w( "WAZE DEBUG", "ON CREATE" );
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
        
//        mGoogleAnalyticsTracker = GoogleAnalyticsTracker.getInstance();;
     // Start the tracker in manual dispatch mode...
//        mGoogleAnalyticsTracker.startNewSession( "UA-24084788-1", this );        
    }
    
    
    /*************************************************************************************************
     * Resume event override
     * 
     */
    @Override
    public void onResume()
    {
    	ProgressToast progressToast = null;    	
    	// Log.w( "WAZE DEBUG", "ON RESUME" );
    	super.onResume();
    	/*
         * Allows the system to destroy the activity 
         */
    	if ( !mIsRunning )
    	{
	        FreeMapAppService.setAppActivity( this );
	        
	        mIsRunning = true;
	
	        /*
	         * Show error message box if sdcard is not accessible
	         */
	        if ( !FreeMapNativeManager.CheckSDCardAvailable() )
	        	return;
	        
	        // Show splash or progress toast
	        if ( mIsSplashShown )
	        {
		        if ( mResumeProgressShow )
		        {	
		        	( progressToast = new ProgressToast() ).start();	        	
		        }
		    	// Just context switch to allow the progress thread to start
	        	android.os.SystemClock.sleep( 20L );
	        }
	        else
	        {
	        	( mSplashToast = new SplashToast() ).start();
	        	mIsSplashShown = true;
	        	 
	        	// Just context switch to allow splash to show
		        android.os.SystemClock.sleep( SPLASH_DISPLAY_TIMEOUT );
	        }
	        
	        if ( FreeMapAppService.IsAppRunning() )
	        {
	        	OnResumeHandler();				        
	    	}
	        else 
	        {
	        	if ( !FreeMapAppService.IsStarting() )
	        	{
		            // Starting the service
		            WazeIntentManager.HandleUri( getIntent().getData(), this );
		            final Runnable resumeEvent = new Runnable() {				
						public void run() {
							OnResumeHandler();					
						}
					};
		            FreeMapAppService.StartApp( this, resumeEvent );
	        	}
	        }
	        
	        final WazeScreenManager screenMgr = FreeMapAppService.getScreenManager();
	        if ( screenMgr != null )
	        	screenMgr.onResume();
	
	        
	        // Register on the intent providing the battery level inspection
	        registerReceiver( FreeMapAppService.getPowerManager(), 
	        					new IntentFilter( Intent.ACTION_BATTERY_CHANGED ) );  
	    	
	        /*
	         * NOT WORKING!!! The application doesn't receive the intents
	         * More investigation is necessary
	         *
	         RegisterMediaBCReceivers();
	         */
	        // Stop progress view thread
	        if ( mResumeProgressShow && progressToast != null )
	        {
	        	progressToast.stopToast();
	        }
    	} // mIsRunning
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
        if ( mIsRunning )
        {
	        FreeMapAppService.setAppActivity( null );
	        
	        mIsRunning = false;
	        
	        FreeMapNativeManager nativeManager = FreeMapAppService.getNativeManager();
	        
	        if ( !nativeManager.IsAppStarted() )
	        {
	            // Special case. Notify the manager that the application pause - don't wait for view to be ready
	        	FreeMapNativeManager.Notify( 0 );
	        	CancelSplash();
	        }
	        else
	        {
	        	unregisterReceiver( FreeMapAppService.getPowerManager() );               	
	            // Restore the system settings        
	        	nativeManager.RestoreSystemSettings();  
	        }
	        WazeScreenManager screenMgr = FreeMapAppService.getScreenManager();
	        screenMgr.onPause();
        } // mIsRunning
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
     */
    @Override
    public void onDestroy() {
      super.onDestroy();
      // Stop the tracker when it is no longer needed.
//      mGoogleAnalyticsTracker.stopSession();
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
    	FreeMapNativeManager nativeManager = FreeMapAppService.getNativeManager();
    	
    	if ( nativeManager.getInitializedStatus() )
    	{
    		WazeMenuManager menuMgr = nativeManager.getMenuManager();
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
    	boolean res = false;
    	FreeMapNativeManager nativeManager = FreeMapAppService.getNativeManager();
    	
    	if ( nativeManager != null && nativeManager.getInitializedStatus() )
    	{
    		WazeMenuManager menuMgr = nativeManager.getMenuManager();
    		res = menuMgr.getInitialized();
    	}
    	
    	if ( mIsMenuRebuild )
    	{    		
    		if ( rebuildOptionsMenu( aMenu ) )
    			mIsMenuRebuild = false;
    	}    	
    	return res;
    }

    
    /*************************************************************************************************
     * Creates the menu for display
     * 
     */
    public boolean  onCreateOptionsMenu ( Menu aMenu )
    {
    	mOptMenu = aMenu;		// Save the reference 
    	
    	boolean res = rebuildOptionsMenu( aMenu );
    	
    	return res;
    }
    @Override
    protected void onActivityResult( int requestCode, int resultCode, Intent data )
    {
    	FreeMapNativeManager nativeManager = FreeMapAppService.getNativeManager();
    	
    	if ( requestCode == SPEECHTT_EXTERNAL_REQUEST_CODE ) {
        	final WazeSpeechttManagerBase speechttManager = nativeManager.getSpeechttManager();
        	speechttManager.OnResultsExternal( resultCode, data );	        	
        }

    	if ( requestCode == TTS_DATA_CHECK_CODE ) {
        	final IWazeTtsManager ttsManager = nativeManager.getTtsManager();
        	if ( ttsManager != null )
        		ttsManager.onDataCheckResult( resultCode, data );	        	
        }

    	if ( requestCode == TTS_DATA_INSTALL_CODE ) {
        	final IWazeTtsManager ttsManager = nativeManager.getTtsManager();
        	if ( ttsManager != null )
        		ttsManager.onDataInstallResult( resultCode, data );	        	
        }
    	
        super.onActivityResult(requestCode, resultCode, data);
    }
    
    
    
    
    /*************************************************************************************************
     * Is the activity is running
     */
    public boolean IsRunning()
    {
        return mIsRunning;
    }
    /*************************************************************************************************
     * Is the activity is running
     */
    public void setResumeProgressShow( boolean aValue )
    {
    	mResumeProgressShow = aValue;        
    }
    /*************************************************************************************************
     * Returns the Main view
     * 
     */
    public WazeMainView getMainView()
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
    public WazeLayoutManager getLayoutMgr()
    {
        return mLayoutMgr;
    }
    /*************************************************************************************************
     * Returns the current view reference
     * 
     */
    public View getCurrentView()
    {
    	return mAppView;
    }
    /*************************************************************************************************
     * CancelSplash - stops displaying splash view
     * 
     */
    public void CancelSplash()
    {
    	if ( mSplashToast != null )
    	{
    		mSplashToast.stopToast(); 
    	}
    	
    }
    /*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */
    
    private boolean rebuildOptionsMenu( Menu aMenu )
    {
    	boolean res = false;
    	FreeMapNativeManager nativeManager = FreeMapAppService.getNativeManager();
    	
    	if ( nativeManager != null && nativeManager.getInitializedStatus() )
    	{
    		WazeMenuManager menuMgr = nativeManager.getMenuManager();
    		if ( menuMgr.BuildOptionsMenu( aMenu, mOrientation == Configuration.ORIENTATION_PORTRAIT ) )
    			res = true;
    	}
    	
    	return res;
    }
    
    private void OnResumeHandler()
    {
    	FreeMapNativeManager nativeManager = FreeMapAppService.getNativeManager();
    	
        // Set the screen timeout
        if ( nativeManager.getInitializedStatus() )
        {
        	nativeManager.SaveSystemSettings();
        	nativeManager.RestoreAppSettings();            
        }
        /*
         * Check if the intent contains the url query
         */
    	final Intent intent = getIntent();
        WazeIntentManager.HandleUri( intent.getData(), this );  
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
	public void setSplashProgressVisible()
	{
		mSplashToast.setProgressVisible();
	}
    /*************************************************************************************************
     * Thread running the Toast showing the progress spin 
     * Auxiliary   
     */
    private final class ProgressToast extends WazeToastThread
    {
    	public ProgressToast()
    	{ 
    		super( "Progress Toast" );
    	}
    	
    	/*************************************************************************************************
         * Shows the progress toast
         */
        @Override public Toast show()
        {
        	Activity activity = FreeMapAppActivity.this;
        	LayoutInflater inflater = activity.getLayoutInflater();
        	View layout = inflater.inflate( R.layout.progress_bar,
        	                               ( ViewGroup ) activity.findViewById( R.id.progress_layout_root ) );

        	Toast toast = new Toast( activity ); 
        	toast.setGravity( Gravity.CENTER_VERTICAL, 0, 0 );
        	toast.setDuration( Toast.LENGTH_LONG );
        	toast.setView( layout );
        	toast.show();
        	return toast;
        }    	
    }
    /*************************************************************************************************
     * Thread running the Toast showing the splash 
     * Auxiliary   
     */
    private final class SplashToast extends WazeToastThread
    {
    	public SplashToast()
    	{ 
    		super( "Splash Toast" );
    	}
    	
    	/*************************************************************************************************
         * Shows the progress toast
         */
    	@Override public Toast show()
        {
        	Activity activity = FreeMapAppActivity.this;
        	LayoutInflater inflater = activity.getLayoutInflater();
        	View layout = inflater.inflate( R.layout.splash_view,
        	                               ( ViewGroup ) activity.findViewById( R.id.splash_layout_root ) );
        	ImageView image = (ImageView) layout.findViewById( R.id.splash_image );
        	mSplashBitmap = FreeMapNativeCanvas.GetSplashBmp( image );
        	image.setImageBitmap( mSplashBitmap );
        	
        	Toast toast = new Toast( activity ); 
        	toast.setGravity( Gravity.CENTER_VERTICAL, 0, 0 );
        	toast.setDuration( Toast.LENGTH_LONG );
        	setCustomDuration( 15000L );
        	toast.setView( layout );        	 
        	toast.show();        	
        	return toast;
        }
    	public void setProgressVisible()
    	{
    		final Runnable event = new Runnable() {				
				public void run() {
					if ( mToast != null )
					{
						final View layout = mToast.getView();
			    		final View progressBar = layout.findViewById( R.id.splash_progress_large );
			    		progressBar.setVisibility( View.VISIBLE );
					}
				}
			};
			if ( mHandler != null )
				mHandler.post( event );
    	}
    	@Override protected void cancel()
        {
    		ImageView image = (ImageView) mToast.getView().findViewById( R.id.splash_image );
    		image.setImageBitmap( null );
        	
        	super.cancel();
        	
    		if ( mSplashBitmap != null )
    		{
	    		mSplashBitmap.recycle();
				mSplashBitmap = null;
    		}
    		
			final Activity activity = FreeMapAppActivity.this;
			activity.setRequestedOrientation( ActivityInfo.SCREEN_ORIENTATION_USER );
        }
    	private Bitmap mSplashBitmap;
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
    private SplashToast 		mSplashToast = null;		// Splash viewer
    private int 				 mOrientation;			   // Screen orientation in that the application works now   
    private boolean				 mIsMenuRebuild = false;   // Indicates that the menu rebuild is necessary on prepare
    private Menu				 mOptMenu = null;		   // Reference to the options menu of the application			 
    
    
	private volatile boolean 	mIsRunning = false;		  // Indicates if the activity is running.
	private volatile boolean 	mResumeProgressShow = true;	  // Indicates if the resume activity progress should be shown
	
    private ViewAnimator mViewAnimator = null;
    
//    private GoogleAnalyticsTracker mGoogleAnalyticsTracker = null;
    public static volatile boolean mIsSplashShown = false;							// Indicates if the splash was already shown

	/*************************************************************************************************
     *================================= Constants section =================================
     */
    public static final long INITIAL_HEAP_SIZE = 4096L;
    public static final boolean TEST_PNG = false; 
    public static final long SPLASH_DISPLAY_TIMEOUT = 1500L;		// In milliseconds

    public static final int SPEECHTT_EXTERNAL_REQUEST_CODE               =    0x00001000;
	public static final int TTS_DATA_CHECK_CODE               			 =    0x00002000;
	public static final int TTS_DATA_INSTALL_CODE               	     =    0x00004000;
}


