package com.waze;

import java.io.File;
import java.io.FileOutputStream;

import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;

public class FreeMapScreenShot extends Thread
{
    static
    {
        // Getting the max index from the file system ons startup
        mMaxIndex = GetMaxFileIndex();
    }
    
    public FreeMapScreenShot( Bitmap aBM )
    {
        setBitmap( aBM );
    }

//    public synchronized void start( Bitmap aBM )
//    {
//        setBitmap( aBM );
//        super.start();
//    }
    
    @Override
    public void run()
    {
        if ( mBitMap != null )
        {
            mMaxIndex++;
             
            String fileName = mScrnShotsNamePrefix + String.valueOf( mMaxIndex ) + mScrnShotsNameSuffix;
            String fullPath = FreeMapResources.mSDCardResDir + mScrnShotsDir + fileName;
            File file = new File( fullPath );
            
            file.getParentFile().mkdirs();
            try
            {
                FileOutputStream fos = new FileOutputStream( file );
                mBitMap.compress( CompressFormat.JPEG, 80, fos );
                mBitMap = null;
            }
            catch( Exception ex )
            {
                WazeLog.w( "FreeMapScreenShot: File writing error for " + fileName, ex );
                ex.printStackTrace();
            }
        }
    }
    
    public void setBitmap( Bitmap aBM )
    {
        mBitMap = aBM;
    }
    
    private static int GetMaxFileIndex()
    {
        int maxIndex = -1;
        String dirName = FreeMapResources.mSDCardResDir + mScrnShotsDir;
        File dir = new File( dirName );
        // Pass through the list of files
        String[] fileNames = dir.list();
        if ( fileNames != null )
        {
            for( int i = 0; i < fileNames.length; i++ )
            {
                // Check if the file is a screenshot
                if ( fileNames[i].startsWith( mScrnShotsNamePrefix ) )
                {
                    // Get the maximum index of the screenshot
                    int start = mScrnShotsNamePrefix.length();
                    int end = fileNames[i].length() - mScrnShotsNameSuffix.length(); 
                    String nextNumString = fileNames[i].substring( start, end );
                    if ( Integer.decode( nextNumString ) > maxIndex )
                        maxIndex =  Integer.decode( nextNumString );  
                }
            }
        }
        return maxIndex;
    }
    
    private Bitmap mBitMap;
    private static int mMaxIndex;
    public final static String mScrnShotsDir = "screenshots/"; 
    public final static String mScrnShotsNamePrefix = "screenshot_";
    public final static String mScrnShotsNameSuffix = ".jpg";
}
