package com.waze.widget.routing;


import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.waze.widget.WazeAppWidgetLog;
import com.waze.widget.WazeAppWidgetService;


public class RoutingResponse {
	private JSONObject mJson;
	private JSONArray mValuesArray;
	private int mAverageTime;
	private int mMin;
	private int mMax;
	
	/*************************************************************************************************
     * RoutingResponse
     * 
     */
	public RoutingResponse(String fromString) throws JSONException {
		int total = 0;
		try {
			mMin = Integer.MAX_VALUE;
			mMax = 0;
			mAverageTime = 0;
			mJson = new JSONObject(fromString);
			mValuesArray = mJson.getJSONArray("response");
			for (int i = 0; i < mValuesArray.length(); i++){
				String val = mValuesArray.getString(i);
				total += Integer.parseInt(val);
				if (Integer.parseInt(val) < mMin) mMin = Integer.parseInt(val);
				if (Integer.parseInt(val) > mMax) mMax = Integer.parseInt(val);
			}
		} catch (JSONException e) {
			WazeAppWidgetLog.e("JSONException parsing routing result [" +fromString+ "]" + e.getMessage());
    		WazeAppWidgetService.alertUser(fromString);

			throw e;
		}
		
		mAverageTime = total / mValuesArray.length();
	
	}
	
	/*************************************************************************************************
     * getList
     * 
     */
	public double[] getList(){ 
		if (mValuesArray != null) { 
			double[] dArray = new double[mValuesArray.length()];  
			for (int i=0;i<mValuesArray.length();i++){ 
				String val;
				try {
					val = mValuesArray.getString(i);
					dArray[i] =Double.parseDouble(val)/60; 
				} catch (JSONException e) {

				}
			}
			return dArray;
		}
		return null;
	}
	   
	/*************************************************************************************************
     * getNumResults
     */
	public int getNumResults(){
		return mValuesArray.length();
	}
	
	/*************************************************************************************************
     * getAveragetTime
     * 
     */
	public int getAveragetTime(){
		return mAverageTime;
	}

	/*************************************************************************************************
     * getMinValue
     * 
     */
	public int getMinValue(){
		return mMin;
	}

	/*************************************************************************************************
     * getMaxValue
     * 
     */
	public int getMaxValue(){
		return mMax;
	}

	/*************************************************************************************************
     * toString
     * 
     */
	public String toString(){
		if (mJson != null)
			return mJson.toString();
		else
			return null;
	}
	
	/*************************************************************************************************
     * getTime
     * 
     */
	public int getTime(){
		String val;
		try {
			if (mValuesArray == null)
				return 0;
			val = mValuesArray.getString(RoutingRequest.getNowLocation());
			return Integer.parseInt(val);
		} catch (JSONException e) {
			// TODO Auto-generated catch block
			return 0;
		}
	}	
}
