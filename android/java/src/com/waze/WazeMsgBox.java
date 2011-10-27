/**  
 * WazeMsgBox - Provides interface for the native message box presentation  
 *            
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
 *   @see WazeMsgBox_JNI.c
 */
package com.waze;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.text.util.Linkify;
import android.util.Log;
import android.widget.TextView;

public final class WazeMsgBox 
{
    /*************************************************************************************************
     *================================= Public interface section =================================
     */
    /*************************************************************************************************
     * InitNativeLayer - Initialization of the core level for the message box  
     * At this stage the library should be loaded
     * @return WazeMsgBox instance 
     */
	static void InitNativeLayer()
	{
		WazeMsgBox instance = getInstance();
		instance.InitMsgBoxNTV();		
	}
	
    /*************************************************************************************************
     * Create - Msg box instance getter  
     * @return WazeMsgBox instance 
     */
	static WazeMsgBox getInstance()
	{
		if ( mInstance == null )
		{
			mInstance = new WazeMsgBox();
		}
		return mInstance;
	}
	
	/*************************************************************************************************
     * setBlocking - sets the messagebox to be blocking  
     * @param aValue - true message box blocks the native thread  
     */
	void setBlocking( boolean aValue )
	{
		mInstance.mIsBlocking = aValue;
	}
	
    /*************************************************************************************************
     * Show - Shows native message box  (for use from the library side) 
     * @param aTitle - title at the top of the box
     * @param aMessage - message at the body of the box
     * @param aLabelOk - label of the ok button
     * @param aLabelCancel - label of the cancel button
     * @param aCbContext - the context of the buttons callback 
     * @return void 
     */
	public void Show( final byte[] aTitle, final byte[] aMessage, final byte[] aLabelOk, final byte[] aLabelCancel, final long aCbContext )
	{
		final FreeMapNativeManager mgr = FreeMapAppService.getNativeManager();
		final FreeMapAppActivity activity = FreeMapAppService.getMainActivity();
		
		if ( activity == null || mgr == null )
			return;

		if ( mgr.IsNativeThread() )
		{
			final Runnable runMsgBox = new Runnable() {
				public void run() {					
					ShowRun( aTitle, aMessage, aLabelOk, aLabelCancel, aCbContext );
				}
			};
			activity.runOnUiThread( runMsgBox );
			// Wait on native thread 
			if ( mIsBlocking )
			{
				try 
				{
		    		synchronized( mInstance ) {
		    			mInstance.wait();
		    			mIsBlocking = false;
		    		}				
					Log.w( "WAZE", "Continue running the native thread" );
				}
				catch ( Exception ex ) {
					WazeLog.w( "Error waiting for the message to finish" );
				}
			}

		}
		else
		{
			ShowRun( aTitle, aMessage, aLabelOk, aLabelCancel, aCbContext );
		}
	}
	 /*************************************************************************************************
     * Show - Shows native message box   
     * @param aTitle - title at the top of the box
     * @param aMessage - message at the body of the box
     * @param aLabelOk - label of the ok button
     * @param aLabelCancel - label of the cancel button
     * @param aOkCb - callback for Ok button
     * @param aCancelCb - callback for the cancel button
     * @return void 
     */
	public void Show( final String aTitle, final String aMessage, final String aLabelOk, final String aLabelCancel, 
			final DialogInterface.OnClickListener aOkCb, final DialogInterface.OnClickListener aCancelCb )
	{
		final FreeMapNativeManager mgr = FreeMapAppService.getNativeManager();
		final FreeMapAppActivity activity = FreeMapAppService.getMainActivity();
		
		if ( activity == null )
			return;
		
		if ( mgr != null && mgr.IsNativeThread() )
		{
			final Runnable runMsgBox = new Runnable() {
				public void run() {					
					ShowRun( aTitle, aMessage, aLabelOk, aLabelCancel, aOkCb, aCancelCb );					
				}
			};
			activity.runOnUiThread( runMsgBox );
			// Wait on native thread 
			if ( mIsBlocking )
			{
				try 
				{
		    		synchronized( mInstance ) {
		    			mInstance.wait();
		    			mIsBlocking = false;
		    		}				
					Log.w( "WAZE", "Continue running the native thread" );
				}
				catch ( Exception ex ) {
					WazeLog.w( "Error waiting for the message to finish" );
				}
			}
	
		}
		else
		{
			ShowRun( aTitle, aMessage, aLabelOk, aLabelCancel, aOkCb, aCancelCb );
		}
	}
	
	 /*************************************************************************************************
     * Notify - notifies the message box to stop blocking the thread    
     * 
     */
	 public static void Notify()
	 {
			try 
			{
	    		synchronized( mInstance ) {
	    			mInstance.notify();
	    		}				
			}
			catch ( Exception ex ) {
				WazeLog.w( "Error notifying the message box: " + ex.getMessage() );
			}
	 }
	 
	 /*************************************************************************************************
     * ShowOk - Shows native message box with Ok button only. Static wrapper   
     * 
     */
	public static void ShowOk( final String aTitle, final String aMessage, final String aLabelOk, final DialogInterface.OnClickListener aOkCb )
	{
		mInstance.Show( aTitle, aMessage, aLabelOk, null, aOkCb, null );
	}

	 /*************************************************************************************************
     * ShowRun - Shows native message box   
     * Auxiliary - see matching Show call
     */
	public void ShowRun( final String aTitle, final String aMessage, final String aLabelOk, final String aLabelCancel, 
			final DialogInterface.OnClickListener aOkCb, final DialogInterface.OnClickListener aCancelCb )
	{
		AlertDialog dlg = builder().create();
        dlg.setCancelable( false );
        
        if ( ( aLabelOk != null ) && ( aOkCb != null ) )
        {
        	dlg.setButton( new String( aLabelOk ), aOkCb );
        }
        if ( ( aLabelCancel != null ) && ( aCancelCb != null ) )
        {
        	dlg.setButton2( new String( aLabelCancel ), aCancelCb );
        }

        dlg.setTitle( aTitle );
        dlg.setMessage( aMessage );
        
        dlg.show();
        
        // Makes http links clickable
        TextView msgView = (TextView) dlg.findViewById(android.R.id.message);
        if ( msgView != null )
        {
        	Linkify.addLinks( msgView, Linkify.WEB_URLS );
        }
	}
	
    /*************************************************************************************************
     * ShowRun - Thread independent body of the show function  
     * @param see Show(...)
     * @return void 
     */
	private void ShowRun( byte[] aTitle, byte[] aMessage, byte[] aLabelOk, byte[] aLabelCancel, long aCbContext )
	{
        AlertDialog dlg = builder().create();
        dlg.setCancelable( false );
        
        if ( aLabelOk != null )
        {
        	dlg.setButton( new String( aLabelOk ), new MsgBoxOnClick( WAZE_MSG_BOX_RES_OK, aCbContext ) );
        }
        if ( aLabelCancel != null )
        {
        	dlg.setButton2( new String( aLabelCancel ), new MsgBoxOnClick( WAZE_MSG_BOX_RES_CANCEL, aCbContext ) );
        }
        dlg.setTitle( new String( aTitle ) );
        dlg.setMessage( new String( aMessage ) );
        
        dlg.show();
        
        // Makes http links clickable       
        TextView msgView = (TextView) dlg.findViewById(android.R.id.message);
        if ( msgView != null )
        {
        	Linkify.addLinks( msgView, Linkify.WEB_URLS );
        }
	}
	
    /*************************************************************************************************
     * builder() - builder creator  
     * @param void
     * @return dialog builder 
     */
	private AlertDialog.Builder builder()
	{
		FreeMapAppActivity activity = FreeMapAppService.getMainActivity();
		return ( new AlertDialog.Builder( activity ) );
	}
	
    /*************************************************************************************************
     *================================= Native methods section ================================= 
     * These methods are implemented in the native side and should be called after!!! the shared library is loaded
     */
	private native void InitMsgBoxNTV();									// Object initializer
	private native void MsgBoxCallbackNTV( int aRes, long aCbContext );		// Callback wrapper on the native layer
	
	/*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */
    /*************************************************************************************************
     * WazeMsgBox - private constructor  
     * @return void 
     */
	private WazeMsgBox()
	{		
	}
	
    /*************************************************************************************************
     * class MsgBoxOnClick -click interface   
     */
	private class MsgBoxOnClick implements DialogInterface.OnClickListener
	{
		public MsgBoxOnClick( int aRes, long aCbContext )
		{
			mCbRes = aRes;
			mCbContext = aCbContext;
		}
    	public void onClick( DialogInterface dialog, int which )
    	{   
    		/* Cancel the dialog in any case */
    		dialog.cancel();
    		
    		FreeMapNativeManager mgr = FreeMapAppService.getNativeManager();
    		Runnable clickRun = new Runnable() 
    		{
				public void run() {

					MsgBoxCallbackNTV( mCbRes, mCbContext );
				}
			};
			
    		mgr.PostRunnable( clickRun );
    		if ( mIsBlocking )
    		{
	    		synchronized( mInstance ) {
	    			mInstance.notify();
	    		}
    		}
    	}
    	private volatile long mCbContext;
    	private volatile int mCbRes;
	}
	
	
	/*************************************************************************************************
     *================================= Members section =================================
     */
	static private WazeMsgBox mInstance = null;					// Instance reference	
	private volatile boolean mIsBlocking = false;
	
	/*************************************************************************************************
     *================================= Constants section =================================
     */
	public static final int WAZE_MSG_BOX_RES_CANCEL = 0;
	public static final int WAZE_MSG_BOX_RES_OK = 1;
	
}
