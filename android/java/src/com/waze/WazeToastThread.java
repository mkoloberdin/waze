/**  
 * WazeToastThread - Toast views abstraction layer.
 *                   UI thread different from the main thread
 *   
 * 
 * 
 * LICENSE:
 *
 *   Copyright 2010     @author Alex Agranovich (AGA),	Waze Mobile Ltd
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
 *   
 */
package com.waze;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Process;
import android.util.Log;
import android.widget.Toast;

/*************************************************************************************************
 * Thread running the Toast shows the toast in its own thread receiving the 
 * events from its view.
 * stopProgress() cancels the toast and stops the thread
 * Auxiliary   
 */
public abstract class WazeToastThread extends HandlerThread
{
	public WazeToastThread( String aName )
	{
		super( aName, Process.THREAD_PRIORITY_URGENT_DISPLAY );
	}
	final public void stopToast() 
	{
		try
		{
			synchronized ( this )
			{
				if ( mHandler == null )
					return;
				
				// Cancel toast view with small delay to allow the screen to refresh
				mHandler.postDelayed( new Runnable() {
					public void run() {
						if ( mToast != null )
						{
							cancel();
							mToast = null;
						}
					}
				}, TOAST_CANCEL_DELAY );
				// Cancel the thread.  Small delay to hold the thread alive ( for events) until the toast view is dead 
				mHandler.postDelayed( new Runnable() {
					public void run() {
						getLooper().quit();
					}
				}, TOAST_CANCEL_DELAY + LOOPER_CANCEL_DELAY );
				
				mHandler.removeCallbacks( mDurationTestCallback );
				mHandler = null;
			} // synchronized
		}
		catch ( Exception ex )
		{
			Log.e( WazeLog.TAG, "Error stopping toast: " + ex.getMessage() );
			ex.printStackTrace();
		}
	}
	final public void stopToastImmediately() 
	{
		try
		{
			synchronized ( this )
			{ 
				final Toast toast = mToast;
				
				if ( mHandler == null )
					return;
				
				mHandler.removeCallbacks( mDurationTestCallback );
				
				// Cancel toast view with small delay to allow the screen to refresh
				mHandler.post( new Runnable() {
					public void run() {
						if ( mToast != null )
						{
							cancel();
							mToast = null;
						}
					}
				} );
				// Cancel the thread.  Small delay to hold the thread alive ( for events) until the toast view is dead 
				mHandler.postDelayed( new Runnable() {
					public void run() {
						getLooper().quit();
					}
				},  LOOPER_CANCEL_DELAY );
				
				
				mHandler = null;
			} // synchronized
		}
		catch ( Exception ex )
		{
			Log.e( WazeLog.TAG, "Error stopping toast: " + ex.getMessage() );
			ex.printStackTrace();
		}
	}
	/*************************************************************************************************
    * Shows the toast view and returns the created Toast. Should be overridden
    */
	public abstract Toast show();

	@Override protected void onLooperPrepared()
	{
		mHandler = new Handler();
		// Show the view
		mToast = show();
		
		if ( mToast == null ) 
		{
			Log.e( WazeLog.TAG, "Toast is not initialized properly: stopping the thread" );
			getLooper().quit();
		}
	}
	
	/*************************************************************************************************
    * Cancels the toast view. Should be refined only when overridden.
    * Called on the Toast thread!!!
    */
	protected void cancel()
	{
		mToast.cancel();
	}
	
	protected void setCustomDuration( long aDuration )
	{
		mDuration = aDuration;
		if ( mHandler != null )
		{
			mDurationTestCallback = new Runnable() {
				public void run() {
					try
					{
						synchronized ( WazeToastThread.this )
						{
							if ( ( mDuration > 0 ) && ( mToast != null ) && ( mHandler != null ) )
							{
								if ( Thread.currentThread().isAlive() )
								{
									mToast.show();
									mHandler.postDelayed( mDurationTestCallback, TOAST_DURATION_TEST_PERIOD );
									mDuration -= TOAST_DURATION_TEST_PERIOD;
								}
								
							}
						} // synchronized
					}
					catch ( Exception ex )
					{
						WazeLog.ee( "Error stopping toast", ex );
					}
							
				}
			};
			mHandler.postDelayed( mDurationTestCallback, TOAST_DURATION_TEST_PERIOD );
		}
	}
	
	protected long mDuration;		// Duration in milliseconds
	protected final long TOAST_CANCEL_DELAY = 1000L;
	protected final long LOOPER_CANCEL_DELAY = 10000L;
	protected volatile Handler mHandler;
	protected Toast mToast = null;	
	
	private final long TOAST_DURATION_TEST_PERIOD = TOAST_CANCEL_DELAY/2;
	private Runnable mDurationTestCallback = null;
}