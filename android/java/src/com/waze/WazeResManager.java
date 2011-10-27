/**  
 * WazeResManager.java
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
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.AssetManager;
import android.graphics.Typeface;
import android.util.Log;
import android.view.Display;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Toast;
import com.waze.WazeUtils.ExceptionEntry;

public final class WazeResManager
{
    /*************************************************************************************************
     *================================= Public interface section =================================
     * 
     */
    public enum ZipEntryType
    {
        ZIP_ENTRY_FILE, ZIP_ENTRY_DIRECTORY
    }

    static public WazeResManager Create()
    {
    	mInstance = new WazeResManager();
    	return mInstance;
    }

    /*************************************************************************************************
     * Extraction of the native library and other resources from the APK package
     * 
     */
    public static void Prepare()
    {
        // Check if we have context to run in
    	final Context context = FreeMapAppService.getAppContext();
        if ( context == null )
        {
            Log.e("FreeMapResources", "The context is not initialized");
            return;
        }
        try
        {
            String resDir = mAppDir;
            PackageInfo pi = context.getPackageManager().getPackageInfo(
            		context.getPackageName(), 0 );
            File appDirFile = new File( mAppDir );
            int currentVerCode = GetVersionFromFile();
            int verCode = pi.versionCode;
            ExceptionEntry[] extractExceptions = null;
            ExceptionEntry[] skinsExtractExceptions = null;
            ExceptionEntry userException[] = null;
            // Log.d("Waze installation", "Current version: " + verCode + ". Previous version: " + GetVersionFromFile() );
            if ( currentVerCode < verCode || !appDirFile.exists() )
            {
            	final FreeMapAppActivity activity = FreeMapAppService.getMainActivity();
            	if ( activity != null )
            		activity.setSplashProgressVisible();
            	
            	skinsExtractExceptions = BuildSkinsExceptionList();
            	
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
                    /* Delete the editor db files */
                    String files[] = ( new File( mAppDir + mMapsDir ) ).list();
                    for ( int i = 0; i < files.length; ++i )
                    {
                    	if ( files != null && files[i] != null )
                    	{
                    		if ( files[i].endsWith( mEditorDbExt ) )
                    			WazeUtils.DeleteDir(  mAppDir + mMapsDir + files[i] );
                    	}
                    }            
                    /* Delete the contents of the package dir, except the ... */
                    WazeUtils.DeleteDir( mPkgDir, true, list );
                }                
                	
                // Extract the library - obsolete. Was before NDK
                if ( EXTRACT_LIBRARY )
                {
                    String libFile = mPkgDir + mLibFile;
	                ExtractFromZip("assets/libandroidroadmap.so", libFile,
	                        ZipEntryType.ZIP_ENTRY_FILE, null );
                }
                // Extract the resources directory
                ExtractFromZip("assets/freemap", resDir,
                        ZipEntryType.ZIP_ENTRY_DIRECTORY, extractExceptions );

                ExtractFromZip("assets/freemap/skins", resDir + "skins/",
                        ZipEntryType.ZIP_ENTRY_DIRECTORY, skinsExtractExceptions );
                
                // Extract the HD resources if necessary
                if ( IsHD() )
                {
                    ExtractFromZip("assets/freemapHD", resDir,
                            ZipEntryType.ZIP_ENTRY_DIRECTORY, extractExceptions );
                    ExtractFromZip("assets/freemapHD/skins", resDir + "skins/",
                            ZipEntryType.ZIP_ENTRY_DIRECTORY, skinsExtractExceptions );

                }
                // Extract the LD resources if necessary
                if ( IsLD() )
                {
                    ExtractFromZip("assets/freemapLD", resDir,
                            ZipEntryType.ZIP_ENTRY_DIRECTORY, extractExceptions );
                    ExtractFromZip("assets/freemapLD/skins", resDir + "skins/",
                            ZipEntryType.ZIP_ENTRY_DIRECTORY, skinsExtractExceptions );

                }

                // Extract the user
                ExtractFromZip("assets/freemap/user", mPkgDir + mUserFile,
                        ZipEntryType.ZIP_ENTRY_FILE, userException );
                                
                SetVersionToFile(verCode);
                // Create no media file
                File file = new File( mSDCardResDir + "/.nomedia" );
                if ( !file.exists() )
                	file.createNewFile();

            }
            else
            {
                Log.d("Freemap installation",
                        "Resources extraction unnecessary");
            }
        }
        catch ( NameNotFoundException ex )
        {
            WazeLog.e( "Prepare failure", ex );
        }
        catch ( IOException ex )
        {
        	WazeLog.e( "Prepare failure", ex );
        }
    }

    static public boolean IsHD()
    {
    	Context ctx = FreeMapAppService.getAppContext();
    	WindowManager windowManager = ( WindowManager ) ctx.getSystemService( Context.WINDOW_SERVICE );
    	Display display = windowManager.getDefaultDisplay();
    	return ( display.getHeight() >= SCREEN_HD_DIM_PIX || display.getWidth() >= SCREEN_HD_DIM_PIX );
    }
    static public boolean IsLD()
    {
    	Context ctx = FreeMapAppService.getAppContext();
    	WindowManager windowManager = ( WindowManager ) ctx.getSystemService( Context.WINDOW_SERVICE );
    	Display display = windowManager.getDefaultDisplay();
    	return ( display.getHeight() <= SCREEN_LD_DIM_PIX || display.getWidth() <= SCREEN_LD_DIM_PIX );
    }
    
    static public String GetBaseDir()
    {
    	String base_dir = mBaseDirSD;
    	if ( IsHD() )
    		base_dir = mBaseDirHD;
    	if ( IsLD() )
    		base_dir = mBaseDirLD;

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
    static public String GetSplashName( boolean wide )
    {
    	return mSplashName;
    }

    static public String GetSkinsPathCommon()
    {
    	String path = mBaseDirSD;
   		path += mSkinsPath;    
    	return path;
    }
    static public String GetSkinsPathCustom()
    {
    	String path = null;
    	if ( IsHD() )
    		path = mBaseDirHD;
    	if ( IsLD() )
    		path = mBaseDirLD;
    	if ( path != null )
    		path += mSkinsPath;
    	
    	return path;
    }

    static public String GetSkinsPath()
    {
    	String path = mSDCardResDir;
   		path += mSkinsPath;    
    	return path;
    }
    
    /*************************************************************************************************
     * LoadResStream - General resource read function. Loads the requested resource from the disk ( high priority ) or 
     *                  package and returns input stream object
     * @param aResName - resource name
     * @param aFilePath - resource path on the file system 
     * @param aAssetPathList - array of paths in the assets to look for resource in 
     * @return Input stream object
     */
    public static InputStream LoadResStream( final String aResName, String aFilePath, String[] aAssetPathList )
    {    	
    	AssetManager assetMgr = FreeMapAppService.getAppContext().getAssets();
    	InputStream stream = null;    	
    	try
    	{
    		File file = new File( aFilePath + "/" + aResName );
    		// Trying to load from the sdcard
    		if ( file.exists() )
    		{
    			stream = new FileInputStream( file );
    		}
    		else
    		{
    			for ( int i = 0; i < aAssetPathList.length; ++i )
    			{
    				final String path = aAssetPathList[i] + aResName;
    				stream  = LoadAssetEntry( assetMgr, path );
    				if ( stream != null )
    					break;
    			}
    		}
    	}
		catch( Exception ex )
    	{
    		WazeLog.e( "Error loading image from package", ex );
    	}
		return stream;
	}
    /*************************************************************************************************
     * LoadSkin - Skin resource read function. Loads the requested resource from the disk ( high priority ) or 
     *                  package 
     * @param aResName - resource name
     * @return Data object of the resource (ResData instance)
     */
    public ResData LoadSkin( String aResName )
    {    	
    	String filePath = WazeResManager.GetSkinsPath();
    	ResData result = null;
    	final String[] assetPathList = 
    		{ 
    			WazeResManager.GetSkinsPathCustom(),
    			WazeResManager.GetSkinsPathCommon()    		  
    		};   
    	
    	result = LoadResData( aResName, filePath, assetPathList );
    	
    	return result;
    }

    /*************************************************************************************************
     * LoadResData - General resource read function. Loads the requested resource from the disk ( high priority ) or 
     *                  package 
     * @param aResName - resource name
     * @param aFilePath - resource path on the file system 
     * @param aAssetPathList - array of paths in the assets to look for resource in 
     * @return Data object of the resource (ResData instance)
     */
    public ResData LoadResData( String aResName, String aFilePath, String[] aAssetPathList )
    {    	
    	InputStream stream = null;
    	int size = 0;
    	byte[] array = null;
    	ResData data = null;
    	
    	try
    	{
    		stream = LoadResStream( aResName, aFilePath, aAssetPathList );
    		if ( stream != null )
    		{
    			array = WazeUtils.ReadStream( stream );
    			data = new ResData( array, size );
    			stream.close();
    		}
    	}
    	catch( Exception ex )
    	{
    		WazeLog.e( "Error loading image from package", ex );
    	}
    	return data;
    }

    
    /*************************************************************************************************
     * class ResData - Represents resources data object  
     *  
     */
    public static class ResData
    {
    	public ResData() {}
    	public ResData( byte[] aBuf, int aSize ) 
    	{
    		buf = aBuf;
    		size = aSize;
    	}
    	
    	public byte[] buf;
    	public int size;
    }
    
    /*************************************************************************************************
     * LoadSkinStream - Loads the requested skin resource and returns data stream  
     * @param aResName - skin resource name
     * @return stream of resource data 
     */
    public static InputStream LoadSkinStream( String aResName )
    {    	    	
    	String filePath = WazeResManager.GetSkinsPath();
    	InputStream stream = null;
    	final String[] assetPathList = 
    		{ 
    			WazeResManager.GetSkinsPathCustom(),
    			WazeResManager.GetSkinsPathCommon()    		  
    		};   
        stream = WazeResManager.LoadResStream( aResName, filePath, assetPathList );
		return stream;
	}
    
    /*************************************************************************************************
     * LoadResList - Returns the list of the resources under the specified path 
     * @param aPath - path in the resources directories 
     * @return array of the resources names 
     */
    public String[] LoadResList( String aPath )
    {    	
    	String[] resListArray = null;
    	AssetManager assetMgr = FreeMapAppService.getAppContext().getAssets();
    	if ( assetMgr == null )
    	{
    		WazeLog.ee( "Error loading resources list. Can't access asset manager" );
    		return null;
    	}
    	ArrayList<String> resList = new ArrayList<String>();
    	
    	try
    	{
    		// Get the list of the resources under the path. 
    		// AssetManager.list does not accept "double" splash as "single" splash
    		String pathCustom = GetBaseDir() + aPath;
    		pathCustom = pathCustom.replace( "//", "/" );
    		String[] entries = assetMgr.list( pathCustom );
	    	
	    	resList.addAll( Arrays.asList( entries ) );
	    	
	    	String pathCommon = mBaseDirSD + aPath;
	    	pathCommon = pathCommon.replace( "//", "/" );
	    	entries = assetMgr.list( pathCommon );
	    	
	    	// Add only non existing entries from the common
	    	for ( int i = 0; i < entries.length; ++i )
	    	{
	    		if ( !resList.contains( entries[i] ) )
	    		{
	    			resList.add( entries[i] );
	    		}
	    	}
	    	
	    	resListArray = new String[resList.size()];
	    	resListArray = resList.toArray( resListArray );
    	}
    	catch( Exception ex )
    	{
    		WazeLog.e( "Exception while loading list of resources. Path: " + aPath, ex );
    	}
    	return resListArray;
	}
    
    
    /*************************************************************************************************
     * getFaceVagReg - Returns the regular VAG font
     * @param aContext - application context to access the resources 
     * @return Typeface of the font 
     */
    public static Typeface getFaceVagReg( Context aContext )
    {
    	if ( mFaceVagReg == null )
    		mFaceVagReg = Typeface.createFromAsset( aContext.getAssets(), mFontVagRegPath );
    	
    	return mFaceVagReg;
    }
    
    /*************************************************************************************************
     * getFaceVagBlck - Returns the black type of the VAG font
     * @param aContext - application context to access the resources 
     * @return Typeface of the font 
     */
    public static Typeface getFaceVagBlck( Context aContext )
    {
    	if ( mFaceVagBlck == null )
    		mFaceVagBlck = Typeface.createFromAsset( aContext.getAssets(), mFontVagBlckPath );
    	
    	return mFaceVagBlck;
    }

    /*************************************************************************************************
     * getFaceVagBold - Returns the bold type of the VAG font
     * @param aContext - application context to access the resources 
     * @return Typeface of the font 
     */
    public static Typeface getFaceVagBold( Context aContext )
    {
    	if ( mFaceVagBold == null )
    		mFaceVagBold = Typeface.createFromAsset( aContext.getAssets(), mFontVagBoldPath );
    	
    	return mFaceVagBold;
    }

    
    /*************************************************************************************************
     *================================= Native methods section ================================= 
     * These methods are implemented in the native side and should be called after!!! the shared library is loaded
     */
	private native void InitResManagerNTV();
	
    /*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */
    private WazeResManager()
    {    	
    	InitResManagerNTV();
    }
    
    /*************************************************************************************************
     * Tries to load the image from the assets
     * Returns null if fails
     */
    private static InputStream LoadAssetEntry( final AssetManager aAssetMgr, final String aPath )
    {
    	InputStream stream = null;
    	try 
    	{
    		stream = aAssetMgr.open( aPath );
    	}
    	catch( Exception ex )
    	{
    		stream = null; 
    	}
    	
    	return stream;
    }

    /*************************************************************************************************
     * Extraction of the resources from the APK package
     * for the upgrade process
     */
    static private ExceptionEntry[] BuildUpgradeExceptionList()
    {
    	String maps = mMapsDir.replaceAll( "/", "" );
    	String tts = mTtsDir.replaceAll( "/", "" );
        final ExceptionEntry[] list = {   
        						new ExceptionEntry( mHistoryFile, ExceptionEntry.EXCLUDE_IF_EXIST ),
    							new ExceptionEntry( mSessionFile, ExceptionEntry.EXCLUDE_IF_EXIST ),
								new ExceptionEntry( mUserFile, ExceptionEntry.EXCLUDE_ALWAYS ),
						        new ExceptionEntry( maps, ExceptionEntry.EXCLUDE_IF_EXIST, ExceptionEntry.COMPARE_START_OF ),
						        new ExceptionEntry( mLangPrefix, ExceptionEntry.EXCLUDE_IF_EXIST, ExceptionEntry.COMPARE_START_OF ),
					        	new ExceptionEntry( mPromptsConf, ExceptionEntry.EXCLUDE_IF_EXIST ),	
				        		new ExceptionEntry( mSkinsPath, ExceptionEntry.EXCLUDE_ALWAYS, ExceptionEntry.COMPARE_START_OF ), 
        						new ExceptionEntry( tts, ExceptionEntry.EXCLUDE_IF_EXIST, ExceptionEntry.COMPARE_START_OF )
        						};
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
    	ExceptionEntry[] array = new ExceptionEntry[list.size()]; 
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
        final ExceptionEntry[] list = 
                { 
        		  new ExceptionEntry( mUserFile, ExceptionEntry.EXCLUDE_ALWAYS ),
    			  new ExceptionEntry( mSkinsPath, ExceptionEntry.EXCLUDE_ALWAYS, ExceptionEntry.COMPARE_START_OF ) 
                };
        
        return list;
    }

    /*************************************************************************************************
     * Extraction of the skins resources from the APK package exceptions
     * 
     */
    static private ExceptionEntry[] BuildSkinsExceptionList()
    {
        final ExceptionEntry[] list = 
        	{
        		new ExceptionEntry( ".bin", ExceptionEntry.EXCLUDE_ALWAYS, ExceptionEntry.COMPARE_END_OF )
        	};
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
            final Context context = FreeMapAppService.getAppContext();
            String pkgFile = context.getPackageManager().getApplicationInfo(
            		context.getPackageName(), 0).sourceDir;
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
        catch (Exception ex)
        {
            Log.e("FreeMapResources", "Error! Failed to update version file"
                                    + ex.getStackTrace());
        }
    }

    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */
    public static byte mUpgradeRun = 0;
    private static WazeResManager mInstance = null; 
	private static Typeface mFaceVagReg = null;
	private static Typeface mFaceVagBold = null;
	private static Typeface mFaceVagBlck = null;
    
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
    public static final String mEditorDbExt		= ".dat";
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
    public static final String mBaseDirLD        = "freemapLD/";
    public static final String mSkinsPath        = "skins/default/";    
    public static final String mTtsDir        	 = "tts/";
    public static final String mSplashPath       = "skins/default/welcome.bin";    
    public static final String mSplashWidePath   = "skins/default/welcome_wide.bin";
    public static final String mSplashName       = "welcome.bin";
    public static final String mFontVagRegPath   = "fonts/VAGRLigh.TTF";
    public static final String mFontVagBoldPath   = "fonts/VAGRBol.TTF";
    public static final String mFontVagBlckPath   = "fonts/VAGRBLck.TTF";
    
	public static final int mVersionCodeSize = 7; // Version code size in bytes
	private static final int PROGRESS_DLG_SHOW_TIMEOUT = 1000; // Milliseconds
	private static final int SCREEN_HD_DIM_PIX	= 640;	// The minimimum dimension size to define the screen as HD (in pixels)
	private static final int SCREEN_LD_DIM_PIX	= 240;	// The minimimum dimension size to define the screen as HD (in pixels)
	private static final boolean EXTRACT_SKINS_BIN_FILES = false;
	private static final boolean EXTRACT_LIBRARY = false;
	
	
}
