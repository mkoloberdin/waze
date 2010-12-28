package com.waze;

import java.io.File;
import java.io.FileOutputStream;
import java.nio.IntBuffer;

import javax.microedition.khronos.opengles.GL10;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.drawable.BitmapDrawable;
import android.view.View;
import android.view.ViewGroup;

public class WazeScreenShotManager extends Thread
{
    static
    {
        // Getting the max index from the file system ons startup
        mMaxIndex = GetMaxFileIndex();
    }
    
    /*************************************************************************************************
     *================================= Public interface section =================================
     */
    /*************************************************************************************************
     * WazeScreenShotManager 
     * Constructor
     */
    public WazeScreenShotManager()
    {
    }
    
    
    /*************************************************************************************************
     * void Capture() - should be called on the rendering thread !
     * @param aGL - OpenGL reference 
     * @return void 
     */
    public void Capture( View aView, GL10 aGL )
    {
    	final int width = aView.getMeasuredWidth(); 
    	final int height = aView.getMeasuredHeight();

    	int glDataArray[]=new int[width*height];
		int pixelsRGBA[]=new int[width*height];
		
		IntBuffer glDataBuf=IntBuffer.wrap( glDataArray );
		glDataBuf.position( 0 );
		
		aGL.glReadPixels( 0, 0, width, height, GL10.GL_RGBA, GL10.GL_UNSIGNED_BYTE, glDataBuf );
		for( int i = 0; i < height; ++i )
		{
			for( int j = 0; j < width; j++ )
            {
				int pix=glDataArray[i*width+j];
				int pb= (pix>>16) & 0xff;
				int pr= (pix<<16) & 0x00ff0000;
				int pixRes=(pix&0xff00ff00) | pr | pb;
				pixelsRGBA[(height-i-1)*width+j] = pixRes;
        	}
         }                  
         mBitMap = Bitmap.createBitmap( pixelsRGBA, width, height, Bitmap.Config.ARGB_8888 );
         
    	 DrawScaledImage( aGL, mBitMap );
    	
    	( new ImageWriter() ).run();
    }
    
	/*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */
    private void DrawScaledImage( GL10 aGL, Bitmap aBmp )
    {    	
    	final FreeMapAppActivity context = FreeMapAppService.getMainActivity();
    	final int viewWidth = aBmp.getWidth();
    	final int viewHeight = aBmp.getHeight();
   	 	final Bitmap bmp = Bitmap.createBitmap( viewWidth, viewHeight, Bitmap.Config.ARGB_8888 ); 
   	 	
   	 	Canvas canvas = new Canvas( bmp );
   	 	canvas.drawColor( Color.DKGRAY );
   	 
   	 	Matrix mtx = new Matrix();
   	 	mtx.setScale( 0.75F, 0.75F );
   	 	mtx.postTranslate( 0.25F * viewWidth / 2, 0.25F * viewHeight / 2 );   	 	
    	canvas.drawBitmap( aBmp, mtx, null );
   	 	
    	final View view = new ScaleEffectView( context, bmp );
        
        
        Runnable showView = new Runnable()
        {
        	public void run() {
        		context.addContentView( view, new ViewGroup.LayoutParams
        				(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT) );
        		view.invalidate();
			}
		};
		context.runOnUiThread( showView );
        
		Runnable hideView = new Runnable()
        {
        	public void run() {
        		view.setVisibility( View.GONE );
        		bmp.recycle();
			}
		};
		view.postDelayed( hideView, SCRN_SHOT_EFFECT_TIMEOUT );
    }
    
    /*************************************************************************************************
     * WazeScreenShotManager 
     * @param aContext - activity reference
     * @return maximum file index in the screen shots folder 
     */
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
    
    /*************************************************************************************************
     * ImageWriter 
     * Separate thread writing and compression of the image
     */
    private final class ImageWriter extends Thread
    {
	    @Override
	    public void run()
	    {
	    	/*
	    	 * Synchronization is unnecessary because every time new
	    	 * bitmap is created 
	    	 */
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
	                mBitMap.compress( CompressFormat.JPEG, 50, fos );
	                fos.flush();
	                fos.close();
	                mBitMap.recycle();
	                mBitMap = null;	                
	            }
	            catch( Exception ex )
	            {
	                WazeLog.w( "FreeMapScreenShot: File writing error for " + fileName, ex );
	                ex.printStackTrace();
	            }
	        }
	    }
    }
    /*************************************************************************************************
     * Scale effect view 
     * 
     */
    private final class ScaleEffectView extends View
    {
    	public ScaleEffectView( Context aContext, Bitmap aBitmap )
    	{
    		super( aContext );
    		setFocusable( true );
//    		setBackgroundColor( Color.BLUE );
    		setBackgroundDrawable( new BitmapDrawable( aBitmap ) );
    	}    	
    }
    
	/*************************************************************************************************
     *================================= Members section =================================
     */

    private Bitmap mBitMap;
    private static volatile int mMaxIndex;
    
	/*************************************************************************************************
     *================================= Constants section =================================
     */

    public final static String mScrnShotsDir = "screenshots/"; 
    public final static String mScrnShotsNamePrefix = "screenshot_";
    public final static String mScrnShotsNameSuffix = ".jpg";
    
    private final static long SCRN_SHOT_EFFECT_TIMEOUT = 1200L;		// milli seconds 
}
