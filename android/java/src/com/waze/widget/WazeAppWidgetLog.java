package com.waze.widget;

import java.util.Date;

import android.util.Log;

public class WazeAppWidgetLog {
    public final static String LOG_TAG = "WAZE WIDGET";
    
	/*************************************************************************************************
	 * d - debug
	 * 
	 */
    public static void d(String msg){
		if (WazeAppWidgetPreferences.debugEnabled())
			Log.d(LOG_TAG ,"[" + (new Date()).toLocaleString() + "] - " + "[DEBUG] - "+ msg);
	}
	
	/*************************************************************************************************
	 * w - warning
	 * 
	 */
    public static void w(String msg){
//		if (WazeAppWidgetPreferences.debugEnabled())
//			WazeAppWidgetService.alertUser("[WARNING] - " + msg);
		Log.w(LOG_TAG, "[" + (new Date()).toLocaleString() + "] - " + "[WARNING] - " + msg);
	}

	/*************************************************************************************************
	 * e - error
	 * 
	 */
    public static void e(String msg){
//		if (WazeAppWidgetPreferences.debugEnabled())
//			WazeAppWidgetService.alertUser("[ERROR] - " + msg);
		
		Log.e(LOG_TAG, "[" + (new Date()).toLocaleString() + "] - " + "[ERROR] - " + msg);
	}
	
}
