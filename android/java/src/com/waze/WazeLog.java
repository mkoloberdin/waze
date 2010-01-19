/**  
 * WazeLog.java
 *   
 * 
 * 
 * LICENSE:
 *
 *   Copyright 2009     @author Alex Agranovich
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
 *   @see 
 */

package com.waze;

import java.io.BufferedReader;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.io.Writer;

import android.util.Log;

public class WazeLog
{

    /*************************************************************************************************
     *================================= Public interface section =================================
     */
    public static void create()
    {
       if ( mInstance == null )
       {
           mInstance = new WazeLog();
       }
    }
    /*************************************************************************************************
     * Returns the string representation of the stack provided by the throwable parameter object
     */
     public static String getStackStr( Throwable aThrowable )
     {     
        final Writer result = new StringWriter();
        final PrintWriter printWriter = new PrintWriter( result );
        aThrowable.printStackTrace( printWriter );
        return result.toString();
     }

    /*************************************************************************************************
     * Debug level log
     */
     public static void d( String aStr )
     {   
         if ( mLogAndroidEnabled )
             Log.d( "Waze Logger", aStr );
         if ( mLogFileEnabled )
             LogData( DEBUG_LVL, aStr );
     }
     
     public static void d( String aStr,  Throwable aThrowable )
     {
         WazeLog.d( aStr + " " + aThrowable.getMessage() + " " + getStackStr( aThrowable ) );
     }
    /*************************************************************************************************
    * Info level log
    */
    public static void i( String aStr )
    {
        if ( mLogAndroidEnabled )
            Log.i( "Waze Logger", aStr );
        if ( mLogFileEnabled )
            LogData( INFO_LVL, aStr );
    }
    public static void i( String aStr,  Throwable aThrowable )
    {
        WazeLog.i( aStr + " " + aThrowable.getMessage() + " " + getStackStr( aThrowable ) );
    }

    /*************************************************************************************************
     * Warning level log
     */
    public static void w( String aStr )
    {
        if ( mLogAndroidEnabled )
            Log.w( "Waze Logger", aStr );
        if ( mLogFileEnabled )
            LogData( WARNING_LVL, aStr );
    }
    public static void w( String aStr,  Throwable aThrowable )
    {
        WazeLog.w( aStr + " " + aThrowable.getMessage() + " " + getStackStr( aThrowable ) );
    }

    /*************************************************************************************************
     * Error level log
     */
    public static void e( String aStr )
    {
        if ( mLogAndroidEnabled )
            Log.e( "Waze Logger", aStr );
        if ( mLogFileEnabled )
            LogData( ERROR_LVL, aStr );
    }
    public static void e( String aStr,  Throwable aThrowable  )
    {
        WazeLog.e( aStr + " " + aThrowable.getMessage() + " " + getStackStr( aThrowable ) );
    }

    /*************************************************************************************************
     * Fatal level log
     */
    public static void f( String aStr )
    {
        LogData( FATAL_LVL, aStr );
    }
    
     
    /*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */
    
    /*************************************************************************************************
     * This class represents the thread following the Android log
     */
     public static class LogCatMonitor extends Thread
     {
         public LogCatMonitor()
         {
             setPriority( Thread.MIN_PRIORITY );           
             setName( "Logcat Monitor" );             
         }
         @Override public void run()
         {
             
             BufferedReader reader = null;
             FileOutputStream writer = null;
             String line;
             StringBuilder log;
             String separator;
             try
             {
                 // Start the process
                 mLogcatProc = Runtime.getRuntime().exec( new String[]
                         {"logcat", "-v" ,"time", "*:W" } );
                 
                 // Start the reader and writer
                 reader = new BufferedReader(new InputStreamReader( mLogcatProc.getInputStream()) );
                 log = new StringBuilder();
                 separator = System.getProperty( "line.separator" );
                 
                 mActive = true;
                 mOutFileValid = false;
                 
                 // Write loop
                 while ( mActive && ( ( line = reader.readLine()) != null ) )
                 {
                     // Reinit the output stream in case of the output file removal
                     if ( !mOutFileValid )
                     {
                         writer = new FileOutputStream( FreeMapResources.mAppDir + FreeMapResources.mLogCatFile, true /*Append*/ );
                         mOutFileValid = true;
                     }
                     
                     // Read the data 
                     log.append( line );
                     log.append( separator );
                     // Write the data
                     writer.write( line.getBytes() );
                     writer.write( separator.getBytes() );
                 }
             }
             catch ( Exception ex )
             {
                 Log.e( "Waze", "Error in Logcat thread: " + ex.getMessage() );
                 ex.printStackTrace();
             }
             finally
             {
         
                 try
                 {  
                     synchronized( mLogcatProc )
                     {              
                         mLogcatProc.destroy();
                     }
                     if ( writer != null )
                         writer.close();
                     if ( reader != null )
                         reader.close();                     
                 }
                 catch ( IOException e )
                 {
                     Log.e( "Waze", "Error closing streams in Logcat thread" );
                 }
             } 
             Log.e( "Waze", "Logcat thread ended" );
         }
         
         public void setOutFileInvalid() { mOutFileValid = false; }
         
         public void Destroy()
         {
             mActive= false;
             try
             {  
                 synchronized( mLogcatProc )
                 {              
                     mLogcatProc.destroy();
                 }
             }
             catch ( Exception e )
             {
                 Log.e( "Waze", "Error closing streams in Logcat thread" );
             }
             
         }
         private Process mLogcatProc = null;
         private boolean mActive = false; 
         private boolean mOutFileValid = false;
     }
    
    
     private WazeLog()
     {}
    
     private static void LogData( int aLevel, String aStr )
     {
         if ( mInstance == null )
             return;
        synchronized ( mInstance )
        {
             mInstance.WazeLogNTV( aLevel, LOG_PREFIX + aStr );            
        }

     }
     
     private native void WazeLogNTV( int level, String aStr );

    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */
     static private WazeLog mInstance = null;   
     
     static private final int DEBUG_LVL = 1; 
     static private final int INFO_LVL = 2;
     static private final int WARNING_LVL = 3;
     static private final int ERROR_LVL = 4;
     static private final int FATAL_LVL = 5;   
      
     static public boolean mLogAndroidEnabled = false;
     static public boolean mLogFileEnabled = true;
     
     static private final String LOG_PREFIX = "Java Layer: ";
}
