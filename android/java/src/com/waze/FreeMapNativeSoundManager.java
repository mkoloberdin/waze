/**  
 * FreeMapNativeSoundManager.java
 * This class is responsible for the sound management. Playing sound and control
 * Provides both synchronous and asynchronous interfaces for the sound file playing
 *   
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
 *   @see FreeMapNativeActvity.java
 */

package com.waze;

import java.io.File;
import java.io.FileInputStream;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;
import java.util.LinkedHashMap;

import android.media.MediaPlayer;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer.OnPreparedListener;
import android.util.Log;


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

        mPlayersTable = new PlayersCache<String, MediaPlayerQueued>( MAX_PLAYERS_NUMBER );
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
    	String delim = new String( "/" );
    	mSoundsDir = new String( aDataDir, 0, aDataDir.length ); // Null termination is unnecessary
    	
    	// Get the list of files
    	File dir = new File( mSoundsDir );
    	if ( !dir.isDirectory() )
    	{
    		WazeLog.w( "The path " + dir + " is not a directory!" );
    		return;
    	}
    	String[] files = dir.list();
    	// Load the file and add to the map
    	for ( int i = 0; i < files.length && i < MAX_PLAYERS_NUMBER; ++i )
    	{
    		String path = dir.getAbsolutePath() + delim + files[i];
    		
            // Add the initialized player to the table
    		MediaPlayerQueued player = new MediaPlayerQueued( path );
            mPlayersTable.put( files[i], player );
            player.Prepare();
    	}    	
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
     * Add file to the list and try to play
     * 
     * @param aFileName
     *            - the full path to the file
     */
    public void PlayFile( byte aFileName[] )
    {
        String fileName = new String( aFileName, 0, aFileName.length );
//    	File dir = new File( mSoundsDir );
//		String path = dir.getAbsolutePath() + "/" + fileName;
        
    	mPendingPlayersList.add( fileName );
        PlayNext();
    }
    
    private void PlayNext()
    {
    	// Check if we have one in cache
    	if ( mPendingPlayersList.size() > 0 )
    	{
	    	if ( mAudioIdle )
	        {
	    		MediaPlayerQueued player = PrepareNext();
	        	mPendingPlayersList.remove( 0 );
	        	player.Play();
	        }
	    	
	    	// Prepare the next
	    	PrepareNext();
    	}
    }

    public MediaPlayerQueued PrepareNext()
    {
    	MediaPlayerQueued player = null;
        // Check if we have one in cache
        if ( mPendingPlayersList.size() > 0 )
        {    
        	String nextFile = mPendingPlayersList.get( 0 );
	    	player = mPlayersTable.get( nextFile );                
	        if ( player == null )
	        {                	
	        	// Replace
	        	player = new MediaPlayerQueued( nextFile );
	    		mPlayersTable.put( nextFile, player );
	    		player.Prepare();            		
	        }
        }
        return player;
    }
    
    public void ShutDown()
    {
    	/*
    	 * Explicitly release the resources     	 
    	 */
    	Collection<MediaPlayerQueued> players = mPlayersTable.values();    	
    	Iterator<MediaPlayerQueued> it = players.iterator();
    	MediaPlayerQueued val;
    	while( it.hasNext() ) 
    	{ 
    		val = it.next();
    		if ( val != null )
    		{
    			val.release();
    		}
		} 
    	mPlayersTable.clear();
    }
    /*
     * LRU cache implementation for the players table
     * 
     */
    private class PlayersCache<K, V> extends LinkedHashMap<K, V>
    {
    	public PlayersCache( int aMaxCapacity )
    	{
    		/*
    		 * 1 element more because the element inserted before removing
    		 * 1.1 load factor - rehashing is unnecessary - constant size hash implementation
    		 */
    		super( aMaxCapacity + 1, 1.1F, true );
    		this.mMaxCapacity = aMaxCapacity;
    	}
    	@Override
    	public V put(K key, V value)
    	{
    		// AGA DEBUG
    		return super.put( key, value );
    	}
    	@Override  
    	protected boolean removeEldestEntry( Entry<K,V> eldest )
    	{
    		if ( size() >= ( mMaxCapacity + 1 ) )
    		{
    			// Release the resources
    			MediaPlayerQueued mp = ( MediaPlayerQueued ) eldest.getValue();
    			
    			return releaseEntry( mp );
    		}
    		return false;
    	}
    	protected boolean releaseEntry( MediaPlayerQueued aMP )
    	{
    		boolean res = true;
    		
    		if ( aMP.isPlaying() )
    		{
    	    	Collection<MediaPlayerQueued> players = mPlayersTable.values();    	
    	    	Iterator<MediaPlayerQueued> it = players.iterator();
    	    	MediaPlayerQueued val;
    	    	while( it.hasNext() ) 
    	    	{ 
    	    		val = it.next();
    	    		if ( val != null )
    	    		{
    	    			if ( !val.isPlaying() )
    	    			{
    	    				this.remove( val.mFileName );
    	    				val.release();
    	    			}
    	    		}
    			} 
    		}
    		else
    		{
    			aMP.release();
    		}
    		return res;
    	}
    	
    	private int mMaxCapacity = 0;
    }
    
    private class MediaPlayerQueued extends MediaPlayer implements 
                            OnCompletionListener, OnPreparedListener
    {
        public MediaPlayerQueued( String aFileName )
        {
            InitListeners();
            
            mFileName = new String( aFileName );
        }
        
        public void Prepare()
        {
            try
            {
                // Load the file
                final FileInputStream fileInStream = new FileInputStream( mFileName );
                // Prepare the media player
                setDataSource( fileInStream.getFD() );
                // Set the volume to the maximum. Adjustment is performed through
                // the general media volume setting
                setVolume(1, 1);
                mEnabled = true;
                /*
                 * Prepare the data file
                 */
                prepare();
            }
            catch( Exception ex )
            {
                WazeLog.e( "MediaPlayerQueued Prepare: Error in loading sound file "
                        + mFileName, ex );
                ex.printStackTrace();
                
            }
        }
        
        public void Play()
        {
            try
            {
                if ( mEnabled )
                {
                    mAudioIdle = false;
                    mPlaying = true;
                    start(); // Just play
                }
            }
            catch( Exception ex )
            {
                WazeLog.e( "MediaPlayerQueued: Error in preparing sound file "
                        + mFileName, ex );
                ex.printStackTrace();
            }
        }
        @Override
        public boolean isPlaying()
        {
        	return mPlaying;
        }
        public void onPrepared( MediaPlayer aMP )
        {
            mPrepared = true;
        }
        public void onCompletion( MediaPlayer aMP )
        {
            try 
            {
                mAudioIdle = true;
                seekTo( 0 );
                mPlaying = false;
                PlayNext();                    
            }
            catch( Exception ex )
            {
                WazeLog.e( "MediaPlayerQueued: Error in preparing sound file "
                        + mFileName, ex );
                ex.printStackTrace();
            }
        }
        private void InitListeners()
        {
            setOnCompletionListener( this );
            setOnPreparedListener( this );
        }
        
        private String mFileName = null;
        private boolean mEnabled = false;
        private boolean mPrepared = false;
        private boolean mPlaying = false;
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
	
	private PlayersCache<String, MediaPlayerQueued> mPlayersTable;
    private ArrayList<String> mPendingPlayersList;
    
    private boolean mAudioIdle = true;
    
    private String mSoundsDir = null;
    
    private final static int  MAX_PLAYERS_NUMBER = 5;
}
