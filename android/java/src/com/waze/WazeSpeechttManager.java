/**  
 * WazeSpeechttManager - Manages android speech to text engine. Extends basic functionality of WazeSpeechttManagerBase  
 *            Supports both native UI and separate SpeechRecognizer implementations
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

import com.waze.WazeSpeechttManagerBase.Callback;

import android.content.Intent;
import android.os.Bundle;
import android.speech.*;
import android.util.Log;

public class WazeSpeechttManager extends WazeSpeechttManagerBase {
	
	/*************************************************************************************************
	 *================================= Public interface section =================================
	 */

	/*************************************************************************************************
	 * WazeSpeechttManager - Constructor   
	 */
	public WazeSpeechttManager()
	{
	}
	
	/*************************************************************************************************
	 * Start - Main entry point  
	 * Initializes new listener and starts it
	 */
	@Override
	public void Start( final Callback aCb, final long aCbContext, final int aTimeout, final byte[] aLangTag, final byte[] aExtraPrompt, final int aFlags )
	{		
		if ( mBusy )
		{
			WazeLog.w( "Cannot start speech recognition - the engine is busy" );
			return;
		}

		mExternalRecognizer = ( ( aFlags & SPEECHTT_FLAG_EXTERNAL_RECOGNIZER ) > 0 );
		
		if ( mExternalRecognizer )
		{
			super.Start( aCb, aCbContext, aTimeout, aLangTag, aExtraPrompt, aFlags );
		}
		else
		{			
			final Intent intent = Prepare( aLangTag, aExtraPrompt, aFlags );
			intent.putExtra( RecognizerIntent.EXTRA_CALLING_PACKAGE, FreeMapAppService.getAppContext().getPackageName() );
		
			Runnable startEvent = new Runnable() {			
				public void run() {
						// Should be destroyed upon finishing speech recognition
						SpeechRecognizer recognizer = SpeechRecognizer.createSpeechRecognizer( FreeMapAppService.getAppContext() );
						
						mListener = new SpeechttListener( recognizer );				
						recognizer.setRecognitionListener( mListener );
						recognizer.startListening( intent );
				}
			};
			// Run on the main thread
			FreeMapAppService.Post( startEvent );
		}
		mBusy = true;
	}
	
	/*************************************************************************************************
	 * Stop - stops the speech recognition process  
	 * @param: aIsCallback - if not zero callback should be called 
	 */
	@Override
	public void Stop()
	{	
		if ( !mExternalRecognizer && mListener == null )
			return;

		Log.i( "WAZE", "Stopping the recognition service" );

		if ( mExternalRecognizer )
		{
			super.Stop();
		}
		else
		{
			final Runnable stopEvent = new Runnable() {			
				public void run() {
						SpeechRecognizer recognizer = mListener.getRecognizer();
						recognizer.stopListening();
				}
			};
			FreeMapAppService.Post( stopEvent );
		}
	}
	
	/*************************************************************************************************
	 * IsAvailable - returns true if the recognition available  
	 * @param: none  
	 */
	@Override
	public boolean IsAvailable()
	{
		return SpeechRecognizer.isRecognitionAvailable( FreeMapAppService.getAppContext() );
	}
	/*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */
	
	/*************************************************************************************************
	 * Reset - resets the state of the manager. Makes it available for the next recognition request
	 * Should be thread safe  
	 * @param: none  
	 */
	@Override
	protected void Reset()
	{
		mBusy = false;
		mListener = null;
		mExternalRecognizer = true;
	}

	/*************************************************************************************************
	 * SpeechttListener - listener interface implementation providing callback for the SpeechRecognizer   
	 *   
	 * @notes: Used only in the non-external implementation
	 */
	final private class SpeechttListener implements RecognitionListener {	
		public SpeechttListener( final SpeechRecognizer aRecognizer )
		{
			mRecognizer = aRecognizer;			
		}
		
		public void onEvent( int eventType, Bundle params ) 
		{
			final int eventType_ = eventType;
			final Runnable eventRun = new Runnable() {
				
				public void run() {
					// AGA DEBUG - reduce level
					WazeLog.w( "Speech to text event: " + eventType_ );
				}
			};
			FreeMapNativeManager.Post( eventRun );			
		}
		
		public void onResults( Bundle results ) 
		{
			ArrayList<String> resultsArray = results.getStringArrayList( SpeechRecognizer.RESULTS_RECOGNITION );
			
			ResultsHandler( resultsArray );
			
			// Finalizing
			mRecognizer.destroy();						
		}
		
		public void onBeginningOfSpeech()
		{
			Log.i( "WAZE", "Recognizer listener: starting speech" );
		}
		public void onEndOfSpeech() 
		{
			Log.i( "WAZE", "Recognizer listener: end of speech" );
		}
		
		public void onPartialResults( Bundle partialResults ) 
		{
			String resStr = new String();
			ArrayList<String> resultsArray = partialResults.getStringArrayList( SpeechRecognizer.RESULTS_RECOGNITION );
			/*
			 * Build new string
			 */
			for( int i = 0; i < resultsArray.size(); ++i )
			{
				resStr += resultsArray.get( i );
				if ( i < resultsArray.size() - 1 )
					resStr += " ";
			}
			Log.i( "WAZE", "Recognizer partial result: " + resStr );
		}
		public void onError( int error ) 
		{
	
			WazeLog.w( "Error (" + error + ") is reported while recognition process" );
			// Finalizing
			mRecognizer.destroy();
			Reset();			
		}
		
		public void onReadyForSpeech( Bundle params ) {}
		public void onRmsChanged( float rmsdB ) {}
		public void onBufferReceived( byte[] buffer ) {}
		public SpeechRecognizer getRecognizer() 
		{
			return mRecognizer;
		}
		/*
		 * 
		 */
		private final SpeechRecognizer mRecognizer;
	}
	
	/*************************************************************************************************
     *================================= Members section =================================
     */
	private volatile SpeechttListener mListener = null;	
	private volatile boolean mExternalRecognizer = true;		// If true, we don't need to instantiate the SpeechRecognizer
													// Just request for the external activity
	
	/*************************************************************************************************
     *================================= Constants section =================================
     */
	
}

