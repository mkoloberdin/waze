package com.waze.widget.routing;

import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

import com.waze.widget.WazeAppWidgetPreferences;

import android.location.Location;

public class RoutingRequest {
	private Location mFrom;
	private Location mTo;
	private RoutingType	 mRouteType;
	private String   mKey;
	private String   mSession;
	private Map<RoutingOption, Boolean> options;
	private static int fromRange = -1;
	private static int toRange = -1;
	
	/*************************************************************************************************
     * RoutingRequest
     * 
     */
	public RoutingRequest(Location from, Location to, RoutingType type, String sessionId, String key){
		mFrom = from;
		mTo = to;
		mRouteType = type;
		mSession = sessionId;
		mKey = key;
		fromRange = WazeAppWidgetPreferences.getStartRange();
		toRange = WazeAppWidgetPreferences.getEndRange();
	}
	
	/*************************************************************************************************
     * setRouteType
     * 
     */
	public void setRouteType(RoutingType type){
		mRouteType = type;
	}
	
	/*************************************************************************************************
     * addOption
     * 
     */
	public void addOption(RoutingOption key, boolean value){
		if (options == null)
			options = new HashMap<RoutingOption, Boolean>();
		
		options.put(key, value);
	}
	
	static public int getNumberOfRecords(){
		if (fromRange == -1)
			fromRange = WazeAppWidgetPreferences.getStartRange();
		
		if (toRange == -1)
			toRange = WazeAppWidgetPreferences.getEndRange();

		return Math.abs(fromRange)/10 + Math.abs(toRange)/10;
	}
	
	static public int getNowLocation(){
		if (fromRange == -1)
			fromRange = WazeAppWidgetPreferences.getEndRange();
		return Math.abs(fromRange)/10;
	}
	
	/*************************************************************************************************
     * buildCmd
     * 
     */
	public String buildCmd(){
		String cmd =  "?from=x:" + mFrom.getLongitude() + "+y:"+mFrom.getLatitude() + "+bd:true"+
			   			"&to=x:" + mTo.getLongitude() + "+y:" + mTo.getLatitude() + "+bd:true"+
			   			"&type=" + mRouteType.name() +
			   			"&returnGeometries=false" + 
			   			"&returnInstructions=false" + 
			   			"&timeout=60000" + 
			   			"&paths=3" + 
			   			"&returnJSON=true" +
			   			"&graph="+fromRange + "," + toRange;
		// Write session id
		if (mSession != null)
			cmd += "&session=" + mSession;
		
		// Write key
		if (mKey != null)
			cmd += "&token=" + mKey;
		
		// Write options
		if (options != null){
			cmd += "&options=";
			for (Entry<RoutingOption, Boolean> e : options.entrySet()){
				cmd += e.getKey().name() + ":";
				if (e.getValue() == true)
					cmd += "T";
				else
					cmd += "F";

				cmd += ",";
			}
		}
		
		return cmd;
		
	}
}
