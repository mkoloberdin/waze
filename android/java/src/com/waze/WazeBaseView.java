package com.waze;

import android.app.Activity;
import android.view.View;

public final class WazeBaseView {
    /*************************************************************************************************
     *================================= Public interface section =================================
     */
	
	
	public WazeBaseView( Activity aContext, int aViewType  )
	{	
		mContext = aContext;

		switch ( aViewType )
		{
			case WAZE_VIEW_TYPE_RGBA_BUFFER_VIEW:
			{
//				mView = new FreeMapAppView( mContext, this );
				mView = null;
				break;
			}
			
			case WAZE_VIEW_TYPE_RGBA_OPENGL_SURFACE:
			{
				// TODO :: Add opengl surface here
				mView = null;
				break;
			}
			default:
			{
				mView = null;
				break;
			}		
		}
	}
	/*************************************************************************************************
     * Returns the currently used view 
     */
	public View getView() { return mView; }
	
	
	
	
	
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
    
	private Activity			 mContext;
	private final View		     mView;
	
	public static final int WAZE_VIEW_TYPE_RGBA_BUFFER_VIEW		= 0;
	public static final int WAZE_VIEW_TYPE_RGBA_OPENGL_SURFACE	= 1;
	
}
