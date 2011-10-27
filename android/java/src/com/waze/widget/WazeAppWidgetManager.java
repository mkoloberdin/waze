package com.waze.widget;



import android.app.ProgressDialog;
import android.content.Context;

public class WazeAppWidgetManager {
	
	
	/*************************************************************************************************
     *================================= Public interface section =================================
     */
	
	public static WazeAppWidgetManager getInstance()
	{		
		return mInstance;
	}
	public static WazeAppWidgetManager create( Context aContext )
	{		
		if ( mInstance == null )
		{
			mInstance = new WazeAppWidgetManager( aContext );
		}
		return mInstance;
	}
	
    /*************************************************************************************************
     * refreshHandler - Handler for the refresh button - starts the application if not started
     *                    posts the route request on the native thread 
     * @param void
     * @return void 
     */
    public static void refreshHandler( final Context aContext )
    {
        mRequestInProcess = true;
        
        WazeAppWidgetLog.d( "refreshHandler" );
        
        WidgetManager.RouteRequest();
       
        
    }

    /*************************************************************************************************
     * RouteRequestCallback - Callback for the routing request ( will be called from the native thread )
     *                    
     * @param aDestDescription - description of the destination
     * @param aTimeToDest - time to destination in minutes
     * @return void 
     */
    public void RouteRequestCallback( final int aStatusCode, final String aErrDesc, final String aDestDescription, final int aTimeToDest )
    {
    	
 	}
    
    public static void setRequestInProcess( boolean aValue )
    {
    	mRequestInProcess = aValue;    	
    }
    public static void shutDownApp()
    {
    }
    public void RequestRefresh()
    {
    	WazeAppWidgetService.requestRefresh( mContext );
    }
    

    

	/*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */
	private WazeAppWidgetManager( Context aContext )
	{
		mContext = aContext;
	}	

	
	/*************************************************************************************************
     *================================= Members section =================================
     */
	private Context mContext = null;
	private static WazeAppWidgetManager mInstance = null;
	private static volatile boolean mRequestInProcess = false;
	private static ProgressDialog mProgressDlg = null;
	/*************************************************************************************************
     *================================= Constants section =================================
     */
	
}
