/**  
 * FreeMapResources.java
 * This class is responsible for the resources extraction and management
 * The context ha to be initialized before use
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
 *   @see roadmap_path.c
 */

package com.waze;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.drawable.Drawable;
import android.os.Message;
import android.util.Log;
import android.view.Display;
import android.view.View;

import com.waze.FreeMapNativeManager.FreeMapUIEvent;
import com.waze.WazeUtils.ExceptionEntry;

public final class FreeMapResources
{
    /*************************************************************************************************
     *================================= Public interface section
     * =================================
     * 
     */
    public enum ZipEntryType
    {
        ZIP_ENTRY_FILE, ZIP_ENTRY_DIRECTORY
    }

    /*************************************************************************************************
     * Extraction of the native library and other resources from the APK package
     * 
     */
    public static void InitContext( Activity aContext )
    {
        mContext = aContext;
    }
    /*************************************************************************************************
     * Extraction of the native library and other resources from the APK package
     * 
     */
    public static boolean CheckSDCardAvailable()
    {
    	boolean res = true;
    	File file = new File("/sdcard");
    	if ( !file.exists() || !file.canWrite() )
    	{
    		if ( mMsgSDDialogRunner == null )
    		{
	    		mMsgSDDialogRunner = new UIProgressDlgRunner( null, UIProgressDlgRunner.ACTION_CREATE_MESSAGE_OK|UIProgressDlgRunner.ACTION_SHOW );
	        	View curView = ( (FreeMapAppActivity) mContext ).getCurrentView();        	
	        	curView.post( mMsgSDDialogRunner );
    		}
    		res = false;
    	}
    	return res;
    }
    /*************************************************************************************************
     * Extraction of the native library and other resources from the APK package
     * 
     */
    public static void ExtractResources()
    {
        // Check if we have context to run in
        if (mContext == null)
        {
            Log.e("FreeMapResources", "The context is not initialized");
            return;
        }

        try
        {
            String resDir = mAppDir;
            String libFile = mPkgDir + mLibFile;
            PackageInfo pi = mContext.getPackageManager().getPackageInfo(
                    mContext.getPackageName(), 0 );
            File appDirFile = new File( mAppDir );
            int currentVerCode = GetVersionFromFile();
            int verCode = pi.versionCode;
            ExceptionEntry[] extractExceptions = null;
            ExceptionEntry userException[] = null;
            // Log.d("Waze installation", "Current version: " + verCode + ". Previous version: " + GetVersionFromFile() );
            if ( currentVerCode < verCode || !appDirFile.exists() )
            {
            	View curView = ( (FreeMapAppActivity) mContext ).getCurrentView();
            	UIProgressDlgRunner showRunner = new UIProgressDlgRunner( null, UIProgressDlgRunner.ACTION_CREATE_PROGRESS|UIProgressDlgRunner.ACTION_SHOW );            	
            	curView.postDelayed( showRunner, PROGRESS_DLG_SHOW_TIMEOUT ); 
            	
            	mUpgradeRun = 1;
                // Remove the log files
                ( new File( mSDCardResDir + mLogFile ) ).delete();
                ( new File( mSDCardResDir + mLogCatFile ) ).delete();
                if ( FreeMapAppService.mLogCatMonitor != null )
                {
                	FreeMapAppService.mLogCatMonitor.setOutFileInvalid();
                }
                
                if ( currentVerCode < 0 )       /* Clean installation */
                {                            
                    ExceptionEntry[] list = BuildCleanUpPkgDirExceptions( false );                    
                    WazeUtils.DeleteDir( mPkgDir, true, list );
                    WazeUtils.DeleteDir( resDir, true, null ); 
                    extractExceptions = BuildCleanInstallExceptionList();
                }
                else                            /* Upgrade            */
                {
                	ExceptionEntry[] list = BuildCleanUpPkgDirExceptions( true );
                    extractExceptions = BuildUpgradeExceptionList();
                    userException = new ExceptionEntry[1];
                    userException[0] = new ExceptionEntry( mUserFile, ExceptionEntry.EXCLUDE_IF_EXIST );
                    
                    // Copy the user from sdcard if exists
                    boolean userInResDir = new File( mAppDir + mUserFile ).exists();
                    boolean userInPkgDir = new File( mPkgDir + mUserFile).exists();
                    if (  userInResDir && !userInPkgDir )
                    {
                        WazeUtils.Copy( mAppDir + mUserFile, mPkgDir + mUserFile );
                        WazeUtils.DeleteDir( mAppDir + mUserFile );
                    }                    
                    /* Delete the contents of the resource dir, except the ... */
                    WazeUtils.DeleteDir( resDir, true, extractExceptions );
                    /* Delete the contents of the package dir, except the ... */
                    WazeUtils.DeleteDir( mPkgDir, true, list );
                }
                // Extract the library
                ExtractFromZip("assets/libandroidroadmap.so", libFile,
                        ZipEntryType.ZIP_ENTRY_FILE, null );
                // Extract the resources directory
                ExtractFromZip("assets/freemap", resDir,
                        ZipEntryType.ZIP_ENTRY_DIRECTORY, extractExceptions );
                
                // Extract the HD resources if necessary
                if ( IsHD() )
                {
                    ExtractFromZip("assets/freemapHD", resDir,
                            ZipEntryType.ZIP_ENTRY_DIRECTORY, extractExceptions );
                }
                
                // Extract the user
                ExtractFromZip("assets/freemap/user", mPkgDir + mUserFile,
                        ZipEntryType.ZIP_ENTRY_FILE, userException );
                                
                // Extract the maps directory
//                ExtractFromZip("assets/freemap/maps", mAppDir + mMapsDir,
//                        ZipEntryType.ZIP_ENTRY_DIRECTORY, null );                
                SetVersionToFile(verCode);
                /* Close the progress dialog */
                showRunner.setAction( UIProgressDlgRunner.ACTION_CANCEL );
                curView.post( showRunner );
            }
            else
            {
                Log.d("Freemap installation",
                        "Resources extraction unnecessary");
            }
        }
        catch ( NameNotFoundException ex )
        {
            Log.e("FreeMapNativeActivity", "ExtractResources() failed."
                    + ex.getMessage() + ex.getStackTrace());
        }
    }

    static public boolean IsHD()
    {
    	return IsHD( mContext );
    }

    static public boolean IsHD( Activity aContext )
    {
    	Display display = aContext.getWindowManager().getDefaultDisplay();
    	return ( display.getHeight() >= SCREEN_HD_DIM_PIX || display.getWidth() >= SCREEN_HD_DIM_PIX );
    }

    
    static public String GetBaseDir()
    {
    	String base_dir = mBaseDirSD;
    	if ( IsHD() )
    		base_dir = mBaseDirHD;
    	return base_dir;
    }
    
    static public String GetSplashPath( boolean wide )
    {
    	String splashPath = GetBaseDir();
    	if ( wide )
    		splashPath += mSplashWidePath;
    	else
    		splashPath += mSplashPath;
    	
    	return splashPath;
    }

    static public String GetReportPath()
    {
    	String path = mSDCardResDir;
   		path += mReportPath;    
    	return path;
    }
    static public String GetEventsPath()
    {
    	String path = mSDCardResDir;
   		path += mEventsPath;    
    	return path;
    }
    static public String GetSkinsPath()
    {
    	String path = mSDCardResDir;
   		path += mSkinsPath;    
    	return path;
    }
    
    /*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */
    private static class UIProgressDlgRunner implements Runnable
    {
    	UIProgressDlgRunner( AlertDialog aDlg2Run, int aAction )
    	{
    		mDlg = aDlg2Run;
    		mAction = aAction;
    	}
    	public void run()
    	{
    		if ( ( mAction & ACTION_CREATE_PROGRESS ) > 0)
    		{
            	mDlg = new ProgressDialog( mContext, ProgressDialog.STYLE_SPINNER );
            	mDlg.setTitle( new String( "Please wait" ) );
            	mDlg.setMessage( new String( "waze is preparing to run for the first time. It may take a minute or two" ) );
    		}
    		if ( ( mAction & ACTION_CREATE_MESSAGE_OK ) > 0)
    		{
        		AlertDialog.Builder dlgBuilder = new AlertDialog.Builder( mContext );
        		FreeMapNativeManager mgr = FreeMapAppService.getNativeManager();
        		
        		mDlg = dlgBuilder.create();
        		
        		final String btnLabel = new String( "Ok" );
        		final String title = new String( "Error" );
        		final String msgText = new String( "Waze can't access your SD card. Make sure it isn't mounted." );
        		Message msg = mgr.GetUIMessage( FreeMapUIEvent.UI_EVENT_STARTUP_NOSDCARD );
    			
    			mDlg.setTitle( title );
    			mDlg.setButton( DialogInterface.BUTTON_POSITIVE, btnLabel, msg );
            	mDlg.setMessage( msgText );
            	mDlg.setIcon( android.R.drawable.ic_dialog_alert );
    		}
			if ( ( mAction & ACTION_SHOW ) > 0 )
    		{
    			mDlg.show();	    			
    		}
    		if ( ( mAction & ACTION_CANCEL ) > 0 )
    		{
    			mDlg.dismiss();
    		}
    	}
    	public void setAction( int aAction )
    	{
    		mAction = aAction;
    	}
    	public AlertDialog getDialog()
    	{
    		return mDlg;
    	}
    	public static final int ACTION_CREATE_PROGRESS = 0x1;
    	public static final int ACTION_CREATE_MESSAGE_OK = 0x2;
    	public static final int ACTION_SHOW = 0x100;
    	public static final int ACTION_CANCEL = 0x200;
    	private AlertDialog mDlg = null;
    	private int mAction = ACTION_SHOW;
    }
  
    /*************************************************************************************************
     * Extraction of the resources from the APK package
     * for the upgrade process
     */
    static private ExceptionEntry[] BuildUpgradeExceptionList()
    {
        final int EXCEPTION_LIST_SIZE = 6;

        ExceptionEntry[] list = new ExceptionEntry[EXCEPTION_LIST_SIZE];
        
        list[0] = new ExceptionEntry( mHistoryFile, ExceptionEntry.EXCLUDE_IF_EXIST );
        list[1] = new ExceptionEntry( mSessionFile, ExceptionEntry.EXCLUDE_IF_EXIST );
        list[2] = new ExceptionEntry( mUserFile, ExceptionEntry.EXCLUDE_ALWAYS );
        String maps = mMapsDir.replaceAll( "/", "" ); 
        list[3] = new ExceptionEntry( maps, ExceptionEntry.EXCLUDE_IF_EXIST, ExceptionEntry.COMPARE_START_OF );
        list[4] = new ExceptionEntry( mLangPrefix, ExceptionEntry.EXCLUDE_IF_EXIST, ExceptionEntry.COMPARE_START_OF );
        list[5] = new ExceptionEntry( mPromptsConf, ExceptionEntry.EXCLUDE_IF_EXIST );
        return list;
    }

    /*************************************************************************************************
     * Clean up of the package directory exceptions
     * for the clean install process
     */
    static private ExceptionEntry[] BuildCleanUpPkgDirExceptions( boolean isUpgrade )
    {
    	ArrayList<ExceptionEntry> list = new ArrayList<ExceptionEntry>();

    	list.add( new ExceptionEntry( mPkgCacheDir, ExceptionEntry.EXCLUDE_ALWAYS ) );
    	list.add( new ExceptionEntry( mPkgDatabasesDir, ExceptionEntry.EXCLUDE_ALWAYS ) );
    	list.add( new ExceptionEntry( mPkgLibDir, ExceptionEntry.EXCLUDE_ALWAYS ) );
    	
    	if ( isUpgrade )
    	{
    		list.add( new ExceptionEntry( mUserFile, ExceptionEntry.EXCLUDE_ALWAYS ) );
    	}
    	ExceptionEntry[] array= new ExceptionEntry[list.size()]; 
    	list.toArray( array );
    	list.clear();
    	return array;
    }
    
    /*************************************************************************************************
     * Extraction of the resources from the APK package
     * for the clean install process
     */
    static private ExceptionEntry[] BuildCleanInstallExceptionList()
    {
        final int EXCEPTION_LIST_SIZE = 1;

        ExceptionEntry[] list = new ExceptionEntry[EXCEPTION_LIST_SIZE];
        
        list[0] = new ExceptionEntry( mUserFile, ExceptionEntry.EXCLUDE_ALWAYS );
        return list;
    }

    /*************************************************************************************************
     * Extracts files and folders from the zip archive
     * 
     * @param aZipEntryName
     *            - archive entry to be extracted
     * @param aTargetFullPath
     *            - the full path to be stored in
     * @param aZipEntryType
     *            - the type File/Directory
     * @param aExceptionList
     *            - the list of exception with the special handling             
     */
    private static void ExtractFromZip( String aZipEntryName,
            String aTargetFullPath, ZipEntryType aZipEntryType, ExceptionEntry[] aExceptionList )
    {
        final int bufSize = 1048576;
        // Load the data and dump it to a file
        byte[] buffer = new byte[bufSize]; // 1MB buffer
        ArrayList<ExceptionEntry> exceptList = null;
        ZipFile pkgZip = null;
        if ( aExceptionList != null )
            exceptList = new ArrayList<ExceptionEntry>( Arrays.asList( aExceptionList ) );
        
        try
        {
            boolean stopFlag = false;
            String pkgFile = mContext.getPackageManager().getApplicationInfo(
                    mContext.getPackageName(), 0).sourceDir;
            // Open the zip
            pkgZip = new ZipFile(pkgFile);
            // Zip entries
            Enumeration entries = pkgZip.entries();
            int nBytesRead = 0;
            ZipEntry zipEntry = null;
            String destFilePath = null;

            while ( entries.hasMoreElements() && !stopFlag )
            {
                // For the file read the file and stop
                if (aZipEntryType == ZipEntryType.ZIP_ENTRY_FILE)
                {
                    zipEntry = pkgZip.getEntry(aZipEntryName);
                    destFilePath = aTargetFullPath;
                    stopFlag = true;
                }
                // For the directory get the next entry
                if (aZipEntryType == ZipEntryType.ZIP_ENTRY_DIRECTORY)
                {
                	zipEntry = (ZipEntry) entries.nextElement();
                	boolean startsWith = zipEntry.getName().startsWith( aZipEntryName ) && 
                							zipEntry.getName().charAt( aZipEntryName.length() ) == '/';
                    if ( startsWith )
                    {
                        int nameStartIdx = aZipEntryName.length() + 1;
                        int nameEndIdx = zipEntry.getName().length();
                        String destPathSuffix = zipEntry.getName().substring(
                                nameStartIdx, nameEndIdx);
                        // Destination path suffix
                        destFilePath = aTargetFullPath + destPathSuffix;

                        if ( CheckException( exceptList, destPathSuffix, aTargetFullPath ) )
                            continue;
                    }
                    else
                    // Not our directory
                    {
                        continue;
                    }
                }

                if ( zipEntry != null )
                {
                    // Get the input stream
                    InputStream streamIn = pkgZip.getInputStream(zipEntry);
                    // Output stream
                    File destFile = new File(destFilePath);
                    File destFileParent = destFile.getParentFile();
                    destFileParent.mkdirs(); // make the path directories if necessary
                    FileOutputStream streamOut = new FileOutputStream(destFile);
    
                    // Unzip the file
                    while ((nBytesRead = streamIn.read(buffer)) > 0)
                    {
                        streamOut.write(buffer, 0, nBytesRead);
                    }
                    // Finish off by closing the streams
                    streamOut.close();
                    streamIn.close();
                }
            } // Entries loop
        }
        catch (NameNotFoundException ex)
        {
            Log.e("FreeMapNativeActivity", "Error! Archive not found"
                    + ex.getMessage());
            ex.printStackTrace();
        }
        catch (IOException ex)
        {
            Log.e("FreeMapNativeActivity", "Error! While file I/O"
                    + ex.getMessage());
            ex.printStackTrace();            
        }
        finally
        {
            buffer = null;
        }
        
        try 
        {
            if ( pkgZip != null )
                pkgZip.close();
        }
        catch ( IOException ex )
        {
            Log.e( "Waze", "Error closing the archive" );
        }
    }
    
    static private boolean CheckException( ArrayList<ExceptionEntry> aExceptionList, String aEntry, String aFullPath )
    {
        boolean res = false;
        int index = -1;

        
        if ( aExceptionList != null )
        {
            ExceptionEntry exEntry = new ExceptionEntry( aEntry,
                                                            ExceptionEntry.EXCLUDE_IF_EXIST,
                                                            ExceptionEntry.COMPARE_USE_ANOTHER_SIDE
                                                        );
            index = aExceptionList.indexOf( exEntry );
            if (  index != -1 )
            {
                ExceptionEntry entry = aExceptionList.get( index );
                
                if ( entry.mType == ExceptionEntry.EXCLUDE_IF_EXIST )
                {
                    if ( new File( aFullPath + aEntry ).exists() )
                        res = true;
                }
                else                               
                {
                    res = true;
                }
            }
        }
        
        return res;
    }

    /*************************************************************************************************
     * Getting the version from the version file stored in the resource directory
     * 
     * @return Loaded version code
     * 
     */
    private static int GetVersionFromFile()
    {
        int versionCode = -1;
        String fullPath = mPkgDir + mVersionFile;
        try
        {
            if ( new File( fullPath ).exists() )
            {
                char verCode[] = new char[mVersionCodeSize];
                FileReader fr = new FileReader( fullPath );
                fr.read( verCode, 0, mVersionCodeSize );
                versionCode = Integer.parseInt( new String( verCode ) );
                fr.close();
            }
        }
        catch (Exception ex)
        {
            Log.e("FreeMapNativeActivity", "Error! While file I/O"
                    + ex.getMessage() + ex.getStackTrace());
            versionCode = -1;
        }
        return versionCode;
    }

    /*************************************************************************************************
     * Setting the version from the version file stored in the resource
     * directory
     * 
     * @param Version
     *            to be stored
     * 
     */
    private static void SetVersionToFile( int aVersion )
    {
        String fullPath = mPkgDir + mVersionFile;
        try
        {
            FileWriter fw = new FileWriter(fullPath);
            fw.write(String.valueOf(aVersion));
            fw.close();
        }
        /*************************************************************************************************
         * Setting the version from the version file stored in the resource
         * directory
         * 
         * @param Version
         *            to be stored
         * 
         */
        catch (Exception ex)
        {
            Log
                    .e("FreeMapNativeActivity",
                            "Error! Failed to update version file"
                                    + ex.getStackTrace());
        }
    }

    /*************************************************************************************************
     * Copy log file to the sdcard
     * 
     * 
     */
    public static void CopyLogToSD()
    {
        FileChannel srcChannel = null;
        FileChannel dstChannel = null;
        try
        {
            // Open files
            srcChannel = (new FileInputStream(mAppDir + mLogFile))
                    .getChannel();
            dstChannel = (new FileOutputStream(mSDCardDir + mLogFileCopy))
                    .getChannel();
            // Transfer the data
            dstChannel.transferFrom(srcChannel, 0, srcChannel.size());
            // Close the files
            if (srcChannel != null)
            {
                srcChannel.close();
            }
            if (dstChannel != null)
            {
                dstChannel.close();
            }
        }
        catch (FileNotFoundException ex)
        {
            Log.i("CopyLogToSD", "File not found " + ex.getMessage()
                    + ex.getStackTrace());
        }
        catch (IOException ex)
        {
            Log.i("CopyLogToSD", "Transfer data error " + ex.getMessage()
                    + ex.getStackTrace());
        }
    }

    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */

	static private Activity mContext = null; // The context
	public static byte mUpgradeRun = 0;
	private static UIProgressDlgRunner mMsgSDDialogRunner = null;
	
    // Constants
    public static final String mAppDir          = "/sdcard/waze/";
    public static final String mPkgDir          = "/data/data/com.waze/";
    public static final String mLibFile         = "libandroidroadmap.so";
    public static final String mSoundDir        = "sound/";
    public static final String mMapsDir         = "maps/";
    public static final String mVersionFile     = "version";
    public static final String mLogFile         = "waze_log.txt";
    public static final String mLogCatFile      = "logcat.txt";
    public static final String mLogFileCopy     = "waze_log.txt.copy";
    public static final String mSDCardDir       = "/sdcard/";
    public static final String mSDCardResDir    = "/sdcard/waze/";
    public static final String mMapTilesDir     = "77001";
    public static final String mHistoryFile     = "history";
    public static final String mLangPrefix      = "lang.";
    public static final String mPromptsConf     = "prompts.conf";
    public static final String mSessionFile     = "session";
    public static final String mUserFile        = "user";
    public static final String mPkgCacheDir      = "cache";
    public static final String mPkgDatabasesDir  = "databases";
    public static final String mPkgLibDir    	 = "lib";
    public static final String mBaseDirSD        = "freemap/";
    public static final String mBaseDirHD        = "freemapHD/";
    public static final String mSkinsPath        = "skins/default/";
    public static final String mReportPath       = "skins/default/ReportAndroid.bin";
    public static final String mEventsPath       = "skins/default/EventsAndroid.bin";
    public static final String mSplashPath       = "skins/default/welcome.bin";    
    public static final String mSplashWidePath   = "skins/default/welcome_wide.bin";
	public static final int mVersionCodeSize = 7; // Version code size in bytes
	private static final int PROGRESS_DLG_SHOW_TIMEOUT = 1000; // Milliseconds
	private static final int SCREEN_HD_DIM_PIX	= 640;	// The minimimum dimension size to define the screen as HD (in pixels)
}


