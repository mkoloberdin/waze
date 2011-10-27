/**  
 * WazeSpeechttManager - Manages android speech to text engine. Basic implementation of sending 
 * intent to the external recognizer activity  
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
 *   @see WazeSpeechttManager_JNI.c
 */
package com.waze;

import java.util.ArrayList;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.DialogInterface;
import android.content.Intent;
import android.speech.*;
import android.util.Log;

public class WazeSpeechttManagerBase {
	
	/*************************************************************************************************
	 *================================= Public interface section =================================
	 */
	/*************************************************************************************************
	 * InitNativeLayer - Initialization of the core level for the message box  
	 * At this stage the library should be loaded
	 * @return WazeMsgBox instance 
	 */
	public void InitNativeLayer()
	{
		InitSpeechttManagerNTV();	
	}
	/*************************************************************************************************
	 * WazeSpeechttManager - Constructor   
	 */
	public WazeSpeechttManagerBase()
	{
	}
	
	/*************************************************************************************************
	 * Start - Main entry point  
	 * Initializes new listener and starts it
	 */
	public void Start( final Callback aCb, final long aCbContext, final int aTimeout, final byte[] aLangTag, final byte[] aExtraPrompt, final int aFlags )
	{		
		if ( mBusy )
		{
			WazeLog.w( "Cannot start speech recognition - the engine is busy" );
			return;
		}

		setCallback( aCb );		
		
		final Intent intent = Prepare( aLangTag, aExtraPrompt, aFlags );
		
		Runnable startEvent = new Runnable() {			
			public void run() {
				final Activity activity = FreeMapAppService.getMainActivity();
				if ( activity != null )
				{
					try
					{
						activity.startActivityForResult( intent, FreeMapAppActivity.SPEECHTT_EXTERNAL_REQUEST_CODE );						
					}
					catch( ActivityNotFoundException ex )
					{
						WazeLog.e( "Error. Speech to text service is not available", ex );						
						WazeMsgBox.ShowOk( "Error", "Speech to text service is not available", "Ok", new OnOkMsgNoService() );
					}
				}
				else
				{
					WazeLog.e( "Cannot start speech recognizer. Main activity is not available" );
				}
			}
		};
		// Run on the main thread
		FreeMapAppService.Post( startEvent );
		mBusy = true;
	}
	
	/*************************************************************************************************
	 * StartNative - Running the engine from the native side. Uses predefined native callback in this case  
	 * Initializes new listener and starts it
	 */
	public void StartNative( final long aCbContext, final int aTimeout, final byte[] aLangTag, final byte[] aExtraPrompt, final int aFlags )
	{
		setNativeLayerCallback( aCbContext );
		Start( mCallback, aCbContext, aTimeout, aLangTag, aExtraPrompt, aFlags );
	}
	
	/*************************************************************************************************
	 * Stop - stops the speech recognition process  
	 * @param: aIsCallback - if not zero callback should be called 
	 */
	public void Stop()
	{	
		Log.i( "WAZE", "Stopping the recognition service" );
		
		final Runnable stopEvent = new Runnable() {			
			public void run() {
					final Activity activity = FreeMapAppService.getMainActivity();
					if ( activity != null )
					{
						activity.finishActivity( FreeMapAppActivity.SPEECHTT_EXTERNAL_REQUEST_CODE );
					}
					else
					{
						WazeLog.e( "Cannot stop speech recognizer. Main activity is not available" );
					}
			}
		};		
		FreeMapAppService.Post( stopEvent );				
	}
	
	public void OnResultsExternal( int aResultCode, Intent aIntent )
	{
		if ( aResultCode == Activity.RESULT_OK )
		{
            ArrayList<String> matches = aIntent.getStringArrayListExtra( RecognizerIntent.EXTRA_RESULTS );
            ResultsHandler( matches );
		}
		else 
		{
		
			if ( aResultCode == Activity.RESULT_CANCELED )
			{
				Log.w( "WAZE", "Recognition is canceled: " + aResultCode );
				WazeLog.w( "Recognition process is canceled!" );				
			}
			else
			{
				Log.w( "WAZE", "Error result is reported: " + aResultCode );
				WazeLog.w( "Not valid result code (" + aResultCode + ") is reported while recognition process" );
			}			
			ResultsHandler( null );			
		}
	}
	/*************************************************************************************************
	 * IsAvailable - returns true if the recognition available  
	 * @param: none  
	 */
	public boolean IsAvailable()
	{
		return true;
	}
	
	/*************************************************************************************************
     * Callback interface. Called upon results arrival
     * 
     */
	public static abstract class Callback
	{
		public Callback()
		{
			mCbContext = CONTEXT_NULL;			
		}
		public Callback( long aCbContext )
		{
			mCbContext = aCbContext;
		}
		private void run( final String aResult, final int aStatus )
		{
			onResults( mCbContext, aResult, aStatus );
		}
		
		protected abstract void onResults( final long aCbContext, final String aResult, final int aStatus );
		public final long CONTEXT_NULL = 0x0;
		public volatile long mCbContext = CONTEXT_NULL;
	}
	
	/*************************************************************************************************
	 * setCallback - Sets the callback for the results handling 
	 * @param: aCb - callback object  
	 */
	public void setCallback( Callback aCb )
	{
		mCallback = aCb;
	}
	
	/*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */
	protected native void InitSpeechttManagerNTV();
	protected native void SpeechttManagerCallbackNTV( final long aCbContext, final String aRes, final int aResStatus );
	
	/*************************************************************************************************
	 * Prepare the intent for the voice recognition   
	 * Auxiliary  
	 */
	protected final Intent Prepare( final byte[] aLangTag, final byte[] aExtraPrompt, int aFlags )
	{
		/*
		 * Build an intent
		 */ 
		final Intent intent = new Intent( RecognizerIntent.ACTION_RECOGNIZE_SPEECH );
		intent.putExtra( RecognizerIntent.EXTRA_LANGUAGE_MODEL, GetModel( aFlags ) );
		intent.putExtra( RecognizerIntent.EXTRA_MAX_RESULTS, SPEECHTT_MAX_RESULTS );
		
		if ( aLangTag != null )
		{
			final String langTag =  new String( aLangTag, 0, aLangTag.length );
			intent.putExtra( RecognizerIntent.EXTRA_LANGUAGE, langTag );
		}
				
		if ( aExtraPrompt != null )
		{
			final String prompt = new String( aExtraPrompt, 0, aExtraPrompt.length );
			intent.putExtra(RecognizerIntent.EXTRA_PROMPT, prompt );
		}		
		return intent;
	}
	/*************************************************************************************************
	 * GetModel - returns the requested recognition model  
	 * @param: none  
	 */
	protected String GetModel( int flags )
	{
		String model = RecognizerIntent.LANGUAGE_MODEL_FREE_FORM;
		if ( ( flags & SPEECHTT_FLAG_INPUT_SEARCH ) > 0 )
		{
			model = RecognizerIntent.LANGUAGE_MODEL_WEB_SEARCH;
		}		
		return model;
	}
	/*************************************************************************************************
	 * Reset - resets the state of the manager. Makes it available for the next recognition request
	 * Should be thread safe  
	 * @param: none  
	 */
	protected void Reset()
	{
		mBusy = false;
	}

	/*************************************************************************************************
	 * setNativeLayerCallback - Sets the callback to be the native layer one
	 * Should be thread safe  
	 * @param: none  
	 */
	protected void setNativeLayerCallback( long aCbContext )
	{
		mCallback = new Callback( aCbContext ) {
			@Override
			protected void onResults( final long aCbContext, final String aResult, final int aStatus ) {
				// TODO Auto-generated method stub
				final Runnable eventCb = new Runnable() {
					public void run() {
						SpeechttManagerCallbackNTV( aCbContext, aResult, aStatus );						
					}
				};
				FreeMapNativeManager.Post( eventCb );
			}
		};
	}
	/*************************************************************************************************
	 * ResultsHandler - builds the resulting phrase and calls core callback   
	 * @param: aMatchesList - array list of the matched words  
	 * @notes: Auxiliary
	 */
	protected void ResultsHandler( final ArrayList<String> aMatchesList )
	{
		String resStr = null;
		final int resStatus;
		
		if ( aMatchesList != null )
		{
			resStr = new String();
			/*
			 * Build new string
			 */
			for( int i = 0; i < aMatchesList.size(); ++i )
			{
				resStr += aMatchesList.get( i );
				if ( i < aMatchesList.size() - 1 )
					resStr += " ";
			}
			Log.i( "WAZE", "Recognizer result: " + resStr );
			
			resStatus = SPEECHTT_RES_STATUS_SUCCESS;
		}
		else
		{
			Log.i( "WAZE", "There are no results..." );
			resStatus = SPEECHTT_RES_STATUS_NO_RESULTS;
		}
		

		/*
		 * Callback event on the native thread
		 */
		final String resStrParam = resStr;
		if ( mCallback != null )
			mCallback.run( resStrParam, resStatus );
		Reset();
	}
	
	
	protected class OnOkMsgNoService implements DialogInterface.OnClickListener
	{
		public void onClick( DialogInterface dialog, int which ) {
			dialog.cancel();
			ResultsHandler( null );
		}
	}
	
	
	/*************************************************************************************************
     *================================= Members section =================================
     */
	protected volatile boolean mBusy = false;

	protected Callback mCallback = null;
	/*************************************************************************************************
     *================================= Constants section =================================
     */
	private static final int SPEECHTT_MAX_RESULTS						 =	  1;
	
	public static final int SPEECHTT_FLAG_TIMEOUT_ENABLED                =    0x00000001;
	public static final int SPEECHTT_FLAG_INPUT_SEARCH                   =    0x00000002;
	public static final int SPEECHTT_FLAG_INPUT_TEXT                     =    0x00000004;
	public static final int SPEECHTT_FLAG_INPUT_CMD                      =    0x00000008;	
	public static final int SPEECHTT_FLAG_EXTERNAL_RECOGNIZER            =    0x00000010;
	
	public static final int SPEECHTT_RES_STATUS_ERROR                    =    0x00000000;
	public static final int SPEECHTT_RES_STATUS_SUCCESS                  =    0x00000001;
	public static final int SPEECHTT_RES_STATUS_NO_RESULTS               =    0x00000002;
}

