/**  
 * FreeMapLogTail.java
 * This class allows printing the native log to the java one
 * by polling the native log file
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
 *   @see FreeMapNativeActivity.java
 */

package com.waze;

import java.io.BufferedReader;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;

import android.util.Log;

public final class FreeMapLogTail
{

    /*************************************************************************************************
     *================================= Public interface section
     * =================================
     * 
     */
    // Default constructor
    FreeMapLogTail()
    {
    }

    /*************************************************************************************************
     * Constructor allows to enable/disable logging to the sdcard
     * 
     * @param aSDLogEnabled
     *            - true for enabled
     */
    FreeMapLogTail(boolean aSDLogEnabled)
    {
        mSDLogEnabled = aSDLogEnabled;
    }

    /*************************************************************************************************
     * Opens the native file for polling
     * 
     * @param aPath
     *            - the full path to the file
     */
    public void Open( String aPath )
    {
        mPath = aPath;
        try
        {
            mReader = new BufferedReader(new FileReader(mPath), mBufSize);
            mTailLoop = new TailLoop();
            mTailLoopThread = new Thread(mTailLoop, "Native Log Thread");
            // Open the sdcard log copy
            if (mSDLogEnabled)
            {
                mSDLogStream = new FileOutputStream(mSDLogPath);
            }

        }
        catch (Exception ex)
        {
            Log.e("FreeMapLogTail Open", "Error in opening log file"
                    + ex.getMessage() + ex.getStackTrace());
        }
    }

    /*************************************************************************************************
     * Starting the polling thread
     * 
     */
    public void StartLogTail()
    {
        if (mReader != null)
            mTailLoopThread.start();
    }

    /*************************************************************************************************
     * Stops the polling thread
     * 
     * @param aPath
     *            - the full path to the file
     */
    public void StopLogTail()
    {
        mTailLoopThread.stop();
        // SdCard Log
        if (mSDLogEnabled)
        {
            try
            {
                mSDLogStream.close();
            }
            catch (IOException ex)
            {
                Log.i("Log Tail", "Error closing SD log stream."
                        + ex.getMessage() + ex.getStackTrace());
            }
        }
    }

    /*************************************************************************************************
     * Closes the file and stops the thread
     * 
     */
    public void Close()
    {
        StopLogTail();
        try
        {
            mReader.close();
        }
        catch (IOException ex)
        {

        }
    }

    /*************************************************************************************************
     *================================= Private interface section
     * =================================
     * 
     */

    /*************************************************************************************************
     * Polling loop thread class
     */
    private class TailLoop implements Runnable
    {
        public void run()
        {
            Log.i("TailLoop", "Starting the LogTail");
            String line = null;
            while (true)
            {
                try
                {
                    if ((line = mReader.readLine()) != null)
                    {
                        // Logging to the java log
                        Log.i("NativeLog", line);
                        // Logging to the SD card log
                        if (mSDLogEnabled && (mSDLogStream != null))
                        {
                            mSDLogStream.write((line + "\n").getBytes());
                        }
                        continue;
                    }
                    Thread.sleep(100L);

                }
                catch (Exception ex)
                {
                    Thread.currentThread().interrupt();
                    Log.e("TailLoop", "Error in reading log file"
                            + ex.getMessage() + ex.getStackTrace());
                    break;
                }
            }
        }
    }

    /*************************************************************************************************
     *================================= Data members section
     * =================================
     * 
     */
    private String           mPath;
    private BufferedReader   mReader;
    private TailLoop         mTailLoop;
    private Thread           mTailLoopThread;
    private int              mBufSize      = 65536;
    private final String     mSDLogPath    = "sdcard/waze_log.txt";
    private boolean          mSDLogEnabled = false;
    private FileOutputStream mSDLogStream;
}
