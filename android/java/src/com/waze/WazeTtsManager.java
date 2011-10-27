package com.waze;
import java.util.HashMap;

import com.waze.FreeMapAppActivity;
import com.waze.FreeMapAppService;
import com.waze.FreeMapNativeCanvas;
import com.waze.FreeMapNativeManager;
import com.waze.WazeLog;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.speech.SpeechRecognizer;
import android.speech.tts.TextToSpeech;
import android.util.Log;
import android.widget.MultiAutoCompleteTextView;

/**  
 * WazeTtsManager - Manages android TTS engine. Inovked from the native library.
 * Callback notifies the completion
 *            
 *   
 * 
 * 
 * LICENSE:
 *
 *   Copyright 2011     @author Alex Agranovich (AGA),	Waze Mobile Ltd
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
 *   @see WazeTtsManager_JNI.c
 */


public final class WazeTtsManager implements TextToSpeech.OnInitListener, IWazeTtsManager {
	
	/*************************************************************************************************
	 *================================= Public interface section =================================
	 */
	
	WazeTtsManager()
	{
	}
	
	/*************************************************************************************************
	 * InitNativeLayer - Initialization of the   
	 * At this stage the library should be loaded
	 * @return void 
	 */
	public void InitNativeLayer()
	{
		InitTtsManagerNTV();	
	}

	
	/*************************************************************************************************
	 * Submit - Main entry point  
	 * Submits the tts request
	 * @param aText - text to synthesize
	 * @param aCbContext - context to pass upon completion
	 */
	public void Submit( final String aText, final String aFullPath, final long aCbContext )
	{		
		if ( mTtsInitilized )
		{
			// AGA DEBUG
			Log.w( WazeLog.TAG, "Received request for " + aText + ". Path: " + aFullPath );

			HashMap<String,String> params = new HashMap<String,String>();
			params.put( TextToSpeech.Engine.KEY_PARAM_UTTERANCE_ID, aText );
			int result = mTts.synthesizeToFile( aText, params, aFullPath );
			
			if ( result == TextToSpeech.ERROR )
				TtsManagerCallbackNTV( aCbContext, TTS_RES_STATUS_ERROR );
			else
				mRequestMap.put( aText, new Long( aCbContext ) );
		}
	}
	
	/*************************************************************************************************
	 * Prepare
	 * Prepares the provider to the requests
	 * @param void
	 */
	public void Prepare()
	{
		checkData();
	}	
	
	public void onDataCheckResult( int resultCode, Intent data )
	{
		
		if ( resultCode == TextToSpeech.Engine.CHECK_VOICE_DATA_PASS ) 
		{
			final Runnable ttsCreateEvent = new Runnable() {
				public void run() {
					Context ctx = FreeMapAppService.getAppContext();
		            // success, create the TTS instance
		            mTts = new TextToSpeech( ctx, WazeTtsManager.this );
				}
			};
			FreeMapNativeManager.Post( ttsCreateEvent );
        } 
		else 
		{
			Log.w( WazeLog.TAG, "TTS Data doesn't present - installing" );
			Activity activity = FreeMapAppService.getMainActivity();
			if ( activity != null )
			{
			    // Missing data, install it
	            Intent installIntent = new Intent();
	            installIntent.setAction( TextToSpeech.Engine.ACTION_INSTALL_TTS_DATA );
	            activity.startActivityForResult( installIntent, FreeMapAppActivity.TTS_DATA_INSTALL_CODE );
			}
        } 		
	}
	
	public void onDataInstallResult( int resultCode, Intent data )
	{
		checkData();
	}
	
	public void onInit( int status )
	{
		if ( status == TextToSpeech.SUCCESS )
		{
			mTtsInitilized = true;
			mRequestMap = new HashMap<String, Long>();
			mTts.setOnUtteranceCompletedListener( new UtteranceCompletedListener() );
		}
	}
	/*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */
	private final class UtteranceCompletedListener implements TextToSpeech.OnUtteranceCompletedListener
	{
		public void onUtteranceCompleted( String aUtteranceId )
		{
			// AGA DEBUG
			Log.w( WazeLog.TAG, "Request completed for " + aUtteranceId );
			Long cbContext = mRequestMap.get( aUtteranceId );
			if ( cbContext == null )
			{
				WazeLog.ee( "WazeTttManager Error. There is no request for " + aUtteranceId );
			}
			// Todo : Check thread
			TtsManagerCallbackNTV( cbContext.longValue(), TTS_RES_STATUS_SUCCESS );
		}
	}

	private void checkData()
	{
		Activity activity = FreeMapAppService.getMainActivity();
		if ( activity != null )
		{
			Intent checkIntent = new Intent();
			checkIntent.setAction( TextToSpeech.Engine.ACTION_CHECK_TTS_DATA );
			activity.startActivityForResult( checkIntent, FreeMapAppActivity.TTS_DATA_CHECK_CODE );
		}
	}

	/*************************************************************************************************
     *================================= Native methods section ================================= 
     * These methods are implemented in the
     * native side and should be called after!!! the shared library is loaded
     */
	 private native void InitTtsManagerNTV();
	 private native void TtsManagerCallbackNTV( final long aCbContext, final int aResStatus );
	 
	/*************************************************************************************************
     *================================= Members section =================================
     */
	
	private volatile boolean mTtsInitilized = false;
	private TextToSpeech mTts = null;
	private HashMap<String,Long> mRequestMap;
	/*************************************************************************************************
     *================================= Constants section =================================
     */
	
	private final int TTS_RES_STATUS_ERROR                     =   0x00000001;
	private final int TTS_RES_STATUS_PARTIAL_SUCCESS           =   0x00000002;
	private final int TTS_RES_STATUS_SUCCESS                   =   0x00000004;
	private final int TTS_RES_STATUS_NO_RESULTS                =   0x00000008;
}

