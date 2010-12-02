/**  
 * WazeCPUProfiler - Provides information regarding the cpu load of the entire system  
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

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Timer;
import java.util.TimerTask;

import android.os.Process;
import android.util.Log;

public final class WazeCPUProfiler extends TimerTask {
	
    /*************************************************************************************************
     *================================= Public interface section =================================
     */

	@Override
	public void run() {

		// Read CPU statistics
		if ( ReadCPU() )
		{
			if ( ( mCurUsageCPUSys + mCurUsageCPUUser ) >= USAGE_THRESHOLD )
			{
				// Post string
				PostResultToLog();
			}
		}
	}
	
    public static void Start()
    {
    	mInstance = new WazeCPUProfiler();
    	Timer timer = FreeMapAppService.getNativeManager().getTimer();
    	timer.scheduleAtFixedRate( mInstance, 0, SAMPLE_PERIOD );
    }
	
	
    /*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */
	
	private WazeCPUProfiler()
	{}
	
	
	private boolean ReadCPU()
	{
		try
		{
			
			BufferedReader reader = new BufferedReader( new InputStreamReader( new FileInputStream( "/proc/stat" ) ), 1000 );
	        String load = reader.readLine();
	        reader.close();     
	
	        String[] toks = load.split( " " );
	
	        long currTotal = Long.parseLong( toks[OVERALL_USER_IDX] ) + Long.parseLong(toks[OVERALL_SYSTEM_IDX]) + 
	        Long.parseLong(toks[OVERALL_IDLE_IDX]);
	        
	        long currUser = Long.parseLong(toks[OVERALL_USER_IDX]);
	        long currSystem = Long.parseLong(toks[OVERALL_SYSTEM_IDX]);
	        long currIdle = Long.parseLong(toks[OVERALL_IDLE_IDX]);
	
	        long totalDelta = currTotal - mLastTotal;
	        
	        // Prevent divide by zero
	        if ( totalDelta == 0 )
	        	return false;
	        
	        // Overall data
	        mCurUsageCPUIdle = ( 100 * ( currIdle - mLastIdleCPU ) )/ totalDelta;   
	        mCurUsageCPUUser = ( 100 * ( currUser - mLastUserCPU ) )/ totalDelta;
	        mCurUsageCPUSys = ( 100 * ( currSystem - mLastSysCPU ) ) / totalDelta;
	        
	        // Process wise data
	        int pid = Process.myPid();
			reader = new BufferedReader( new InputStreamReader( new FileInputStream( "/proc/" + pid + "/stat" ) ), 100 );
	        String loadProcess = reader.readLine();
	        reader.close();     
	
	        toks = loadProcess.split( " " );
	        long currProcessCPU = Long.parseLong( toks[PROCESS_USER_IDX] ) + Long.parseLong( toks[PROCESS_KERNEL_IDX] ); 
	        mCurUsageCPUProcess = ( 100 * ( currProcessCPU - mLastProcessCPU ) ) / totalDelta;
	        
	        
	        // Update the last samples state
	        mLastTotal = currTotal;
	        mLastUserCPU = currUser;
	        mLastSysCPU = currSystem;
	        mLastIdleCPU = currIdle;
	        mLastProcessCPU = currProcessCPU;
		}
		catch( Exception ex )
		{
			WazeLog.e( WAZE_CPU_PROFILER, ex );
			return false;
		}
		return true;
	}
	
	
	private void PostResultToLog()
	{
		final String resStr = FormatResultString();
		Runnable logTask = new Runnable() {			
			public void run() {
				WazeLog.w( resStr );
			}
		};
		FreeMapNativeManager mgr = FreeMapAppService.getNativeManager();
		mgr.PostRunnable( logTask );
		
		// Show in the logcat
		if ( SHOW_LOGCAT )
		{
			Log.w( "WAZE", resStr );
		}

	}
    
	private String FormatResultString()
	{
		final SimpleDateFormat formatter = new SimpleDateFormat( "hh:mm:ss" );
		final Date now = new Date();
		final String fmtTime = formatter.format( now );
		final String res = 
			WAZE_CPU_PROFILER + "(percents). User: " + mCurUsageCPUUser + ". System: " + mCurUsageCPUSys +
						". Idle: " + mCurUsageCPUIdle + ". WAZE: " + mCurUsageCPUProcess +
						". Post time: " + fmtTime;
		return res;
	}
    /*************************************************************************************************
     *================================= Data members section =================================
     */
	
	// Overall statistics
	private long mLastTotal = 0;
	private long mLastUserCPU = 0;
	private long mLastSysCPU = 0;
	private long mLastIdleCPU = 0;
	
	// Process wise statistics
	private long mLastProcessCPU = 0;
	private long mCurUsageCPUProcess = 0;	// In percents
	
	
	private long mCurUsageCPUUser = 0;		// In percents
	private long mCurUsageCPUSys = 0;	// In percents
	private long mCurUsageCPUIdle = 0;	// In percents

	private static WazeCPUProfiler mInstance = null; 	// The singelton instance
	
	/*************************************************************************************************
     *================================= Constants section =================================
     */
	private final static long SAMPLE_PERIOD = 5000L; // In milli seconds
	
	private final static float USAGE_THRESHOLD = 75; // Threshold if print the usage (percents)
	
	private final boolean SHOW_LOGCAT = false;
	
	private final static int OVERALL_USER_IDX = 2;
	private final static int OVERALL_SYSTEM_IDX = 4;
	private final static int OVERALL_IDLE_IDX = 5;
	
	private final static int PROCESS_USER_IDX = 13;
	private final static int PROCESS_KERNEL_IDX = 14;

	private final static String WAZE_CPU_PROFILER = "WAZE CPU PROFILER"; 
}
