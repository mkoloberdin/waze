package com.waze;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.util.Log;
import android.view.View;

public final class WazeSplashView extends View {
    /*************************************************************************************************
     *================================= Public interface section =================================
     */
	
	
	public WazeSplashView( Activity aContext )
	{	
		super( aContext );
		mContext = aContext;
		
	}
	
	/*************************************************************************************************
     *  Initialize the bitmap of the splash
     */
	private void InitSplashBitmap()
	{
		mSplashBmp = FreeMapNativeCanvas.GetSplashBmp( mContext, this );
	}
    @Override
    protected void onMeasure( int aWidthMeasureSpec, int aHeightMeasureSpec )
    {
        super.onMeasure(aWidthMeasureSpec, aHeightMeasureSpec);
        InitSplashBitmap();
    }
    /*************************************************************************************************
     * Callback for the screen update Runs the application draw state machine
     * step Consequent calls update the canvas
     */
    @Override
    protected void onDraw( Canvas aCanvas )
    {
//    	Bitmap bmp = FreeMapNativeCanvas.GetSplashBmp( mContext, this  );
    	aCanvas.drawBitmap( mSplashBmp, 0, 0, null );
//    	bmp.recycle();
    }
	
	
    /*************************************************************************************************
     * 
     * 
     */
    @Override
    protected void  onDetachedFromWindow  ()
    {
    	Log.w("WAZE DEBUG", "Splash view detached" );
    	mSplashBmp.recycle();
    }
	
	/*************************************************************************************************
     *================================= Private interface section =================================
     * 
    /*************************************************************************************************
     *================================= Native methods section ================================= 
     * These methods are implemented in the
     * native side and should be called after!!! the shared library is loaded
     */

    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */
    
	private Activity			 mContext = null;
	private Bitmap 		 mSplashBmp = null;
}
