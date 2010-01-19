/**  
 * FreeMapAppService.java
 * This class represents the android service controlling different inter-activities interaction.
 * It also increases the importance of the entire application process from the android system view.
 * The implementation is "NO IPC" because the calls are from inside the application 
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
import java.util.TimerTask;

import com.waze.WazeLog.LogCatMonitor;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Debug;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.util.Log;

final public class FreeMapAppService extends Service
{
    @Override
    public void onCreate()
    {
        super.onCreate();
    }

    @Override
    public void onStart( Intent aIntent, int aStartId )
    {
        super.onStart(aIntent, aStartId);
        // Initialize the service instance
        mInstance = this;
        if ( WAZE_LOGCAT_MONITOR_ENABLED )
        {
	        // Create the logcat thread
	        mLogCatMonitor = new LogCatMonitor();
	        // Start the logcat thread
	        mLogCatMonitor.start();
        }
        // Reduce the possibility to be killed 
        setForeground( true );
        
        Log.w( LOG_TAG, "Service started. Instance: " + mInstance );
    }

    @Override
    public void onDestroy()
    {
        if ( mNativeManager != null )
            mNativeManager.RestoreSystemSettings();
        Log.w( LOG_TAG, "Service destroy." );
    }

    /*************************************************************************************************
     * Initializes the managers and starts the application
     * 
     * @param aContext
     *            - reference to the Activity context
     */
    public static void StartApp( FreeMapAppActivity aContext, FreeMapAppView aAppView )
    {
        mAppView = aAppView;
        mMainContext = aContext;
        
        // Power manager
        mPowerManager = new FreeMapPowerManager();
        
        // Start the native manager
        mNativeManager = FreeMapNativeManager.Start( mMainContext, mAppView );
        
        // Set notification
        SetNotification( WAZE_NOTIFICATION_RUNNING );
        
        // AGA DEBUG
        TimerTask task = new TimerTask() {
		        @Override	
		        public void run()
		        {
	                Log.w("WAZE PROFILE", "Native heap. Used: " + Debug.getNativeHeapAllocatedSize() +
    		                ". Free: " + Debug.getNativeHeapFreeSize() +
    		                ". Total: " + Debug.getNativeHeapSize() );
		        }         
		    };
//		 mNativeManager.getTimer().scheduleAtFixedRate( task, 0, 10000 );
        
    }
    
    
    /*************************************************************************************************
     * Returns the native manager reference
     * 
     * @param void
     *            
     */
    public static FreeMapNativeManager getNativeManager()
    {
        return mNativeManager;
    }
    
    /*************************************************************************************************
     * Returns the power manager reference
     * 
     * @param void
     *            
     */
    public static FreeMapPowerManager getPowerManager()
    {
        return mPowerManager;
    }
    /*************************************************************************************************
     * Returns the main context reference
     * 
     * @param void
     *            
     */
    public static FreeMapAppActivity getAppActivity()
    {
        return mMainContext;
    }
    /*************************************************************************************************
     * Returns the application vew reference
     * 
     * @param void
     *            
     */
    public static FreeMapAppView getAppView()
    {
        return mAppView;
    }
    
    /*************************************************************************************************
     * Shows the main activity window with the delay of aDelay msec-s
     * 
     * @param aDelay
     *            - delay in showing the main activity window
     */
    public static void ShowMainActivityWindow( long aDelay )
    {
        // Send message to show the Dialer
        mServiceMsgDispatcher.sendEmptyMessageDelayed(MSG_SHOW_MAIN_ACTIVITY, aDelay);
    }
    
    /*************************************************************************************************
     * 
     * 
     * @param aDelay
     *            - delay in showing the main activity window
     */
    public static void OpenBrowserForUpgrade()
    {
        // Send message to show the Dialer
        mServiceMsgDispatcher.sendEmptyMessage( MSG_OPEN_BROWSER_FOR_UPGRADE );
    }


    /*************************************************************************************************
     * Shows the home window. Waits aDelay msec-s before return to the main
     * context
     * 
     * @param aDelay
     *            - time (in msec-s) to wait before return to the main window.
     *            -1 - no return
     */
    public static void ShowHomeWindow( long aDelay )
    {
        // Send message to show the Dialer
        mServiceMsgDispatcher.sendEmptyMessage(MSG_SHOW_HOME_SCREEN);
        if (aDelay >= 0)
        {
            mServiceMsgDispatcher.sendEmptyMessageDelayed(MSG_SHOW_MAIN_ACTIVITY, aDelay);
        }
    }

    /*************************************************************************************************
     * Shows the dialer window. Waits aDelay msec-s before return to the main
     * context
     * 
     * @param aDelay
     *            - time (in msec-s) to wait before return to the main window.
     *            -1 - no return
     */
    public static void ShowDilerWindow( long aDelay )
    {
//        Log.d(LOG_TAG, "Post message request for Dialer");
        // Send message to show the Dialer
        mServiceMsgDispatcher.sendEmptyMessage(MSG_SHOW_DIALER_SCREEN);
        if (aDelay >= 0)
        {
            mServiceMsgDispatcher.sendEmptyMessageDelayed(MSG_SHOW_MAIN_ACTIVITY, aDelay);
        }
    }

    /*************************************************************************************************
     * Shows the Camer preview window
     * 
     */
    public static void ShowCameraPreviewWindow()
    {
//        Log.d(LOG_TAG, "Post message request for Camera");
        // Send message to show the Camera
        mServiceMsgDispatcher.sendEmptyMessage(MSG_SHOW_CAMERA_PREVIEW);
    }

    /*************************************************************************************************
     * Must be overridden ....
     */
    @Override
    public IBinder onBind( Intent intent )
    {
        // TODO Auto-generated method stub
        return null;
    }

    /*************************************************************************************************
     * This class implements the message handler loop for the service. Can be
     * used inside the application only
     * 
     * @author Alex Agranovich
     */
    private final static class ServiceMsgDispatcher extends Handler
    {
        @Override
        public void handleMessage( Message aMsg )
        {
            switch (aMsg.what)
            {
                // Show the main activity
                case MSG_SHOW_MAIN_ACTIVITY:
                {
                    ShowMainActivityHandler();
                    break;
                }
                    // Show the home screen
                case MSG_SHOW_HOME_SCREEN:
                {
                    ShowHomeHandler();
                    break;
                }
                    // Show the dialer screen
                case MSG_SHOW_DIALER_SCREEN:
                {
                    ShowDialerHandler();
                    break;
                }
                // Show the dialer screen
                case MSG_SHOW_CAMERA_PREVIEW:
                {
                    ShowCameraPreviewHandler();
                    break;
                }
                // Show the dialer screen
                case MSG_OPEN_BROWSER_FOR_UPGRADE:
                {
                    OpenBrowserForUpgradeHandler();
                    break;
                }

            }
        }
    };

    /*************************************************************************************************
     * Opens the browser to download the handler
     */
    private static void OpenBrowserForUpgradeHandler()
    {
        if ( mInstance == null )
            return;
        
        Intent browserIntent = new Intent( Intent.ACTION_VIEW );
        Uri uri = Uri.parse( WAZE_UPGRADE_URL );
        browserIntent.setFlags( Intent.FLAG_ACTIVITY_NEW_TASK );
        browserIntent.setData( uri );
        mInstance.startActivity( browserIntent );
    }
    /*************************************************************************************************
     * Shows the main activity by sending the intent to the main context
     */
    private static void ShowMainActivityHandler()
    {
        if (mInstance == null)
            return;

        Intent mainActivityIntent = new Intent(mMainContext, FreeMapAppActivity.class);
        mainActivityIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK );
        mInstance.startActivity(mainActivityIntent);
    }

    /*************************************************************************************************
     * Shows the home screen by sending the MAIN intent
     */
    private static void ShowHomeHandler()
    {
        if (mInstance == null)
            return;

        Intent homeIntent = new Intent(Intent.ACTION_MAIN);
        homeIntent.addCategory(Intent.CATEGORY_HOME);
        homeIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mInstance.startActivity(homeIntent);
    }

    /*************************************************************************************************
     * Shows the dialer by sending the ACTION_DIAL intent to the system
     */
    private static void ShowDialerHandler()
    {
        if (mInstance == null)
            return;
        Intent dialIntent = new Intent(Intent.ACTION_DIAL);
        dialIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mInstance.startActivity(dialIntent);
    }

    /*************************************************************************************************
     * Updates the notification bar
     */
    private static void SetNotification( int aNotification )
    {
    	int icon = R.drawable.notification;	        // icon from resources
    	
    	if ( FreeMapResources.IsHD( mMainContext ) )
    		icon = R.drawable.notification_hd;		// WVGA - another icon
    	
    	CharSequence tickerText = "waze";              // ticker-text
    	long when = System.currentTimeMillis();         // notification time
    	Context context = (Context) mMainContext;
    	CharSequence contentTitle = "waze";  // expanded message title
    	CharSequence contentText = "return to waze";      // expanded message text

    	Intent notificationIntent = new Intent( mMainContext, FreeMapAppActivity.class );
    	PendingIntent contentIntent = PendingIntent.getActivity( mMainContext, 0, notificationIntent, 0 );

    	Notification notification = new Notification ( icon, tickerText, when );
    	notification.setLatestEventInfo( context, contentTitle, contentText, contentIntent );
    	
    	notification.flags |=  Notification.FLAG_NO_CLEAR | Notification.FLAG_ONGOING_EVENT;    
    	
    	String ns = Context.NOTIFICATION_SERVICE;
    	NotificationManager mNotificationManager = (NotificationManager) context.getSystemService(ns);
    	mNotificationManager.notify( aNotification, notification );
    	mCurrentNotification = aNotification;
    }
    
    /*************************************************************************************************
     * Updates the notification bar
     */
    private static void ClearNotification()
    {
    	String ns = Context.NOTIFICATION_SERVICE;
    	NotificationManager mNotificationManager = (NotificationManager) mMainContext.getSystemService(ns);
    	mNotificationManager.cancel( mCurrentNotification );
    }
    /*************************************************************************************************
     * Shows the camera preview by sending the appropriate intent to the
     * activity
     */
    private static void ShowCameraPreviewHandler()
    {
        if (mInstance == null)
            return;
        Intent cameraIntent = new Intent(mInstance, FreeMapCameraActivity.class);
        cameraIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mInstance.startActivity(cameraIntent);
    }

    /*************************************************************************************************
     * The activity being used as a main activity
     */
    public static void setMainContext( FreeMapAppActivity aContext )
    {
        mMainContext = aContext;
    }

    /*************************************************************************************************
     * Service initialized indicator
     */
    public static boolean isInitialized()
    {
        return ( ( mInstance != null ) && ( mMainContext != null ) );
    }

    
    /*************************************************************************************************
     * The activity being used as a main activity
     */
    public void  onLowMemory()
    {
        Log.w( "Waze Service", "Low memory warning!!!" );        
    }
    
    /*************************************************************************************************
     * Stops the service
     */
    public static void ShutDown()
    {
        if (mInstance == null)
            return;
        if ( mLogCatMonitor != null )
            mLogCatMonitor.Destroy();
        mInstance.stopSelf();
        
        ClearNotification();
        
    }

    public final static int a = 1;
    
	public final static int MSG_SHOW_MAIN_ACTIVITY = 0; // This message will
														// cause the main
														// application window to
														// be shown
	public final static int MSG_SHOW_HOME_SCREEN = 1; // This message will cause
														// the home window to be
														// shown
	public final static int MSG_SHOW_DIALER_SCREEN = 2; // This message will
														// cause the dialer
														// window to be shown
	public final static int MSG_SHOW_CAMERA_PREVIEW = 3; // This message will
															// cause the camera
															// preview window to
															// be shown

   public final static int MSG_OPEN_BROWSER_FOR_UPGRADE = 4; // This message will
                                                               // cause the camera
                                                               // preview window to
                                                               // be shown
   
   public final static String WAZE_UPGRADE_URL = "m.waze.com"; 
   

	private static ServiceMsgDispatcher mServiceMsgDispatcher = new ServiceMsgDispatcher(); // Custom
																							// service
																							// message
																							// dispatcher	
	private static FreeMapAppActivity mMainContext; // Main context created the
												    // service

    private static FreeMapPowerManager mPowerManager;
    
    private static FreeMapAppView mAppView;
    

    private static FreeMapNativeManager mNativeManager = null; // Native manager reference


	
    private static FreeMapAppService    mInstance               = null;

    private final static String         LOG_TAG                 = "Waze Service";

    private static final int WAZE_NOTIFICATION_RUNNING	 = 	1;
    
    private static final boolean WAZE_LOGCAT_MONITOR_ENABLED	 = 	false;
    
    private static int 			mCurrentNotification;
    
    static public LogCatMonitor mLogCatMonitor = null;          // Log cat monitor thread   
}
