package com.waze;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.view.SurfaceView;

public class WazeStandbyManager extends BroadcastReceiver {

	/*************************************************************************************************
     *================================= Public interface section =================================
     */
	
	public WazeStandbyManager( FreeMapNativeManager aNativeMgr )
	{	
		mNativeManager = aNativeMgr;
		mScreenManager = FreeMapAppService.getScreenManager();
	}
	
	@Override
	public void onReceive( Context aContext, Intent aIntent ) 
	{
		String action = aIntent.getAction();
		Log.w( "WAZE", "Screen broadcast receiver got action: " + action );
		WazeLog.w( "Screen broadcast receiver got action: " + action );
		if ( Intent.ACTION_SCREEN_ON.equals( action ) )
		{
			mScreenManager.onPause();
		}
		if ( Intent.ACTION_SCREEN_OFF.equals( action ) )
		{
			mScreenManager.onResume();
		}
	}

	/*************************************************************************************************
     *================================= Private interface section =================================
     * 
    /*************************************************************************************************
     *================================= Native methods section ================================= 
     * These methods are implemented in the
     * native side and should be called after!!! the shared library is loaded
     */

    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */
	FreeMapNativeManager mNativeManager = null;
	WazeScreenManager mScreenManager = null;
}
