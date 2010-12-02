/**  
 * FreeMapNativeSoundManager.java
 * This class is responsible for the sound management. Implements asynchronous playing, playlist management
 * and pre-buffering.  
 *   
 * 
 * LICENSE:
 *
 *   Copyright 2010 	@author Alex Agranovich
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
 *   @see FreeMapNativeActvity.java
 */

package com.waze;

import java.io.FileInputStream;
import java.util.ArrayList;

import android.media.MediaPlayer;
import android.media.MediaPlayer.OnCompletionListener;
import android.util.Log;
/**
 * TODO: Manage failures in terms of media player release
 * TODO: Check logger from the different thread
 * 
 */

public final class FreeMapNativeSoundManager
{

    /*************************************************************************************************
     *================================= Public interface section =================================
     * 
     */

    public FreeMapNativeSoundManager(FreeMapNativeManager aNativeManager)
    {
        mNativeManager = aNativeManager;

        // At this stage the native library has to be loaded
        InitSoundManagerNTV(); 

        mPendingPlayersList = new ArrayList<String>();
        
    }

    /*************************************************************************************************
     * Init the sound pool. Build the hash map
     * 
     * @param aDataDir
     *            - the full path to the directory containing only the sound files 
     */
    public void LoadSoundData( byte[] aDataDir )
    {
    }

    /*************************************************************************************************
     * Add file to the list and try to play
     * 
     * @param aFileName
     *            - the full path to the file
     */
    public void PlayFile( byte aFileName[] )
    {
        String fileName = new String( aFileName, 0, aFileName.length );
    	mPendingPlayersList.add( fileName );
    	PlayNext();
    }

    /*************************************************************************************************
     * Playing the sound file for the SDK supported formats. The overloaded
     * function for the more convenient call from the native layer
     * 
     * @param aFileName
     *            - the full path to the file
     */

    /*************************************************************************************************
     *================================= Private interface section =================================
     */

    /*************************************************************************************************
     * PlayNext() - Main logic of the manager. Plays the file and tries to buffer the next file in the list
     * 
     * @param void
     * @return void           
     */
    private void PlayNext()
    {
    	
		
		if ( mPlaying ) 
		{
			// If already playing - try to buffer the next
			BufferNext();
		}
		else
		{
			// If not playing - play the buffered file or play the new one
			if ( mBufferedPlayer == null )
			{
				if ( mPendingPlayersList.size() > 0 )
				{
					String nextFile = mPendingPlayersList.remove( 0 );
					mCurrentPlayer = new WazeAudioPlayer( nextFile ); 
					mCurrentPlayer.Play();
				}
			}
			else
			{
				mCurrentPlayer = mBufferedPlayer;
				mCurrentPlayer.Play();
				
				mBufferedPlayer = null;
			}
			// Try to buffer next file
			BufferNext();
		}
    }
    
    /*************************************************************************************************
     * BufferNext() - Creates thread with buffering request. Will be played later 
     * 
     * @param void
     * @return void           
     */
    private void BufferNext()
    {
		if ( mBufferedPlayer == null )
		{
			if ( mPendingPlayersList.size() > 0 )
			{
				String nextFile = mPendingPlayersList.get( 0 );
				
				/*
				 * Avoid audio lock on playing the same file
				 */
				if ( mCurrentPlayer != null && nextFile.equals( mCurrentPlayer.getFileName() ) )
				{
					return;
				}

				mPendingPlayersList.remove( 0 );
				mBufferedPlayer = new WazeAudioPlayer( nextFile );
				mBufferedPlayer.Buffer();
			}
		}
    }
    
    public void ShutDown()
    {
    	/*
    	 * Explicitly release the resources     	 
    	 */
    }
   
    
    /*************************************************************************************************
     * This class implements the logic of the player thread 
     * 
     * @author Alex Agranovich
     */
    private final class WazeAudioPlayer extends Thread
    {
    	WazeAudioPlayer( String aFileName )
    	{
    		mFileName = aFileName;
    	}
    	public void Play()
    	{
    		if ( mBuffering && !mBuffered ) // In the buffer process
    			return;
    		
    		mPlaying = true;
    		if ( mBuffered )
    		{
    			Log.w( "WAZE", "Playing buffered file " + mFileName );
    			try
                {
	    			synchronized( this ) {
	    				notify();
	    			}
                }
                catch( Exception ex )
                {
                	WazeLog.e( "Audio Player. Error notifying the thread", ex );
                }
    		}
    		else
    		{  
    			Log.w( "WAZE", "Playing not buffered file " + mFileName );
    			start();
    		}
    	}
    	public void Buffer()
    	{
    		mBuffering = true;
    		start();
    	}
    	public void run()
    	{
            try
            {
            	/*
            	 * Always buffer first
            	 */
            	if ( !mBuffered )
            		BufferInternal();
            	
            	/*
            	 * if buffering request - wait for the play request
            	 */
            	if ( mBuffering )
            	{
	    			synchronized( this ) {
	    				wait();
	    			}
            	}
            	/*
            	 * Start playing
            	 */
	            mMP.start();
            }
            catch( Exception ex )
            {
            	WazeLog.e( "Audio Player. Error playing the file " + mFileName, ex );
            }
    	}
    	public String getFileName()
    	{
    		return mFileName;
    	}
    	private void BufferInternal()
    	{
            try
            {
	    		mMP = new MediaPlayer();
	    		mMP.setOnCompletionListener( new CompletionListener() );
	            // Load the file
	            final FileInputStream fileInStream = new FileInputStream( mFileName );
	            // Prepare the media player
	            mMP.setDataSource( fileInStream.getFD() );
	            // Set the volume to the maximum. Adjustment is performed through
	            // the general media volume setting
	            mMP.setVolume(1, 1);                     
	            /*
	             * Prepare the data file
	             */
	            mMP.prepare();
	            
	            mBuffered = true;
            }
            catch( Exception ex )
            {
            	Log.e( "WAZE", "Exception occurred: " + ex.getMessage() );
            }

    	}
    	/*
    	 * Completion  listener - indicate the next file can be played. Post request for the next file playing 
    	 */
    	private final class CompletionListener implements OnCompletionListener
    	{
            public void onCompletion( MediaPlayer aMP )
            {            	
            	aMP.release();            	
            	mPlaying = false;
            	
            	Runnable playNext = new Runnable() {
					public void run() {
						PlayNext();
					}
				};
				mNativeManager.PostRunnable( playNext );
            }            
    	}
    	private String mFileName;
    	private boolean mBuffered = false;			// Indicates whether the prepare is finished
    	private boolean mBuffering = false;			// Indicates whether the buffering is requested  
    	private MediaPlayer mMP = null;
    }
    
    
    
    /*************************************************************************************************
     *================================= Native methods section ================================= These methods are implemented in the
     * native side and should be called after!!! the shared library is loaded
     */
    private native void InitSoundManagerNTV();

    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */
    private FreeMapNativeManager mNativeManager;
	
    private ArrayList<String> mPendingPlayersList;
    private volatile boolean mPlaying = false;
    private WazeAudioPlayer mBufferedPlayer = null;
    private WazeAudioPlayer mCurrentPlayer = null;
//    private final static int  MAX_PLAYERS_NUMBER = 5;
}
