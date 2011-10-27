package com.waze.widget;

import com.waze.widget.routing.RouteScoreType;
import com.waze.widget.routing.RoutingResponse;

public class StatusData
{
	public final static int STATUS_SUCCESS = 0x00000001;
	public final static int STATUS_UNDEFINED = 0x00000002;
	public final static int STATUS_DATA_OK = 0x00000004;
	public final static int STATUS_NEW_WIDGET = 0x00000008;
	public final static int STATUS_REFRESH_TEST_TRUE = 0x00000010;
	public final static int STATUS_REFRESH_REQUEST = 0x00000020;
	public final static int STATUS_REFRESH_REQUEST_INFO = 0x00000040;
	public final static int STATUS_DRIVE_REQUEST = 0x00000080;
	
	public final static int STATUS_ERR_GENERAL = 0x00010000;
	public final static int STATUS_ERR_NO_LOGIN = 0x00020000;
	public final static int STATUS_ERR_NO_LOCATION = 0x00040000;
	public final static int STATUS_ERR_NO_DESTINATION = 0x00080000;
	public final static int STATUS_ERR_ROUTE_SERVER = 0x00100000;
	public final static int STATUS_ERR_REFRESH_TIMEOUT = 0x00200000;
	
	public final static int STATUS_MASK_ERROR = 0x00010000;

	
	/*************************************************************************************************
     * 
     */
	public StatusData() {}
	
	/*************************************************************************************************
     * 
     */
	public StatusData( String aDestDescription, int aTimeToDest, RouteScoreType score , RoutingResponse routingRsp )
	{
		mTimeStamp = System.currentTimeMillis();
		mDestDescription = aDestDescription;
		mTimeToDest = aTimeToDest;
		mScore = score;
		mRoutingResponse = routingRsp;
	}
	
	/*************************************************************************************************
     * 
     */
	public StatusData( String aDestDescription, int aTimeToDest, long aTimeStamp )
	{
		mTimeStamp = aTimeStamp;
		mDestDescription = aDestDescription;
		mTimeToDest = aTimeToDest;
	}

	/*************************************************************************************************
     * 
     */
	public StatusData( final StatusData aStatusData ) { this.copy( aStatusData ); }

	/*************************************************************************************************
     * 
     */
	public void copy( final StatusData aData )
	{
		if ( aData != null )
		{
			mDestDescription = aData.mDestDescription;
			mTimeToDest = aData.mTimeToDest;
			mTimeStamp = aData.mTimeStamp;
			mScore = aData.mScore;
			mRoutingResponse = aData.mRoutingResponse;
		}
	}
	
	/*************************************************************************************************
     * 
     */
	public RoutingResponse  getRoutingRespnse(){ return mRoutingResponse; }

	/*************************************************************************************************
     * 
     */
	public void setTimeStamp( long aTimeStamp ) { mTimeStamp = aTimeStamp; }
	
	/*************************************************************************************************
     * 
     */
	public String destination() { return mDestDescription; }

	/*************************************************************************************************
     * 
     */
	public int timeToDest() { return mTimeToDest; }
	
	/*************************************************************************************************
     * 
     */
	public long timeStamp() { return mTimeStamp; }

	/*************************************************************************************************
     * 
     */
	public RouteScoreType score() {return mScore;} 
	
	
	
	private String mDestDescription = "Home";
	private int mTimeToDest = 0;		// In minutes
	private long mTimeStamp = 0;
	private RoutingResponse mRoutingResponse;
	private RouteScoreType mScore = RouteScoreType.NONE;
}