/**  
 * WazeSoundRecorder - Provides interface for the basic sound recording  
 *            			Singelton
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
 *   @see WazeSoundRecorder_JNI.c
 */
package com.waze;

import android.media.MediaRecorder;
import android.view.MotionEvent;

public class WazeSoundRecorder
{
    /*************************************************************************************************
     *================================= Public interface section =================================
     */
    /*************************************************************************************************
     * Create - Creates sound recorder engine and initializes the native layer object
     * 				Should be call when library is loaded
     * @param void
     * @return void 
     */
    public static void Create()
    {
    	mInstance = new WazeSoundRecorder();    	
    }
    /*************************************************************************************************
     * Start - Starts recording of the audio 
     * @param aPath - the path to store the
     * @param aTimeout - the timeout to stop after  
     * @return -1 - If Error, 0 - Success 
     */
	public int Start( String aPath, int aTimeout )
	{
		int res = 0;
		
		if ( mMR != null )
		{
			WazeLog.e( "The recorder is already initialized!!" );
			return -1;
		}
		
		WazeLog.d( "Recoridng file to: " + aPath ); 
		
		mMR = new MediaRecorder();

		mMR.setAudioSource( MediaRecorder.AudioSource.MIC );
		mMR.setOutputFormat( MediaRecorder.OutputFormat.MPEG_4 );
		mMR.setAudioEncoder( MediaRecorder.AudioEncoder.AMR_NB );
		mMR.setOutputFile( aPath );
		
		mMR.setOnErrorListener( new OnErrorListener() );
		mMR.setOnInfoListener( new OnInfoListener() );
		mMR.setMaxDuration( (int) ( aTimeout * 1.05 ) );	// 5 Percent more
		mMR.setMaxFileSize( MAX_FILE_SIZE_BYTES );
		
		/*
		 * Increase sampling rate if possible
		 */
//		final int sdkBuildVersion = Integer.parseInt( android.os.Build.VERSION.SDK );
//		if ( sdkBuildVersion >= android.os.Build.VERSION_CODES.FROYO )
//		{ 
//			CompatabilityWrapper.setSamplingRate( mMR, SAMPLING_RATE );
//		}

		try 
		{
			mMR.prepare();
			mMR.start();
		} 
		catch ( Exception ex ) 
		{
			WazeLog.ee( "Error starting media recorder", ex );
			res = -1;
		} 
		
		return res;
	}
	
	  /*************************************************************************************************
     * Stop - Stops recording the audio 
     * @param None  
     * @return void 
     */
	public void Stop()
	{
		try 
		{
			WazeLog.d( "Stop recording file" );
			if ( mMR != null )
			{
				mMR.stop();
				mMR.reset();
				mMR.release();
				mMR = null;
			}
		} 
		catch ( Exception ex ) 
		{
			WazeLog.ee( "Error stopping media recorder", ex );			
		} 
	}
	
	
    /*************************************************************************************************
     * CompatabilityWrapper the compatability wrapper class used to overcome dalvik delayed  
     * class loading. Supposed to be loaded only for android 2.2+.  
     */
    private static final class CompatabilityWrapper
    {
    	public static void setSamplingRate ( MediaRecorder aMR, int aSamplingRate )
    	{
    		aMR.setAudioSamplingRate( aSamplingRate );
    	}

    }

	
    /*************************************************************************************************
     *================================= Native methods section ================================= 
     * These methods are implemented in the native side and should be called after!!! the shared library is loaded
     */
	private native void InitSoundRecorderNTV();
	
	/*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */
	
	private WazeSoundRecorder()
	{		
		InitSoundRecorderNTV();
	}
	
	private static class OnErrorListener implements MediaRecorder.OnErrorListener
	{
		public void onError( MediaRecorder aMR, int aWhat, int aExtra )
		{
			WazeLog.e( "Error: " + aWhat + " in recording process. Extra " + aExtra );
		}
	}
	private static class OnInfoListener implements MediaRecorder.OnInfoListener
	{
		public void onInfo( MediaRecorder aMR, int aWhat, int aExtra )
		{
			WazeLog.ii( "Info: " + aWhat + " in recording process. Extra " + aExtra );
		}
	}
	
	/*************************************************************************************************
     *================================= Members section =================================
     */
	
	private MediaRecorder mMR = null;
	private static WazeSoundRecorder mInstance = null;
	/*************************************************************************************************
     *================================= Constants section =================================
     */
	private final static long MAX_FILE_SIZE_BYTES = 100000L;
	private final static int SAMPLING_RATE = 16000;
}

