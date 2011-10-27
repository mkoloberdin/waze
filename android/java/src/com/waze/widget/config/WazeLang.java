package com.waze.widget.config;

import java.io.File;
import java.io.FileReader;
import java.util.HashMap;
import java.util.Scanner;

import com.waze.widget.WazeAppWidgetLog;

import android.os.Environment;


public class WazeLang {

	
	/*************************************************************************************************
     * load 
     * 
     */
	public static void load(String lang)
	{
		if (mMap != null)
			return;

		try{
    		File sdcard = Environment.getExternalStorageDirectory();
    		WazeAppWidgetLog.d( "Loading lang file" + sdcard +"," + mDirName + "," + mFileName +lang );
			
			File file = new File(sdcard, mDirName+mFileName+lang);
    		Scanner scanner = new Scanner(new FileReader(file));
	        mMap = new HashMap<String, String>();

	        while (scanner.hasNextLine()) {
	            String[] columns = scanner.nextLine().split("=");
	            mMap.put(columns[0], columns[1]);
	        }
    	}
    	catch (Exception ex) {
    		WazeAppWidgetLog.e( "Failed to load lang file [" + ex.getMessage()+"]");
    	}
	}
	
	/*************************************************************************************************
     * getLang
     * 
     */
	public static String getLang(String name){
		if (mMap != null){
			String val = mMap.get(name);
			if (val == null)
				return name;
			else
				return val;
		}else
			return name;
	}
	
	private static HashMap<String, String> mMap = null;
	private static String mDirName = "/waze/";
	private static String mFileName ="lang.";

}
