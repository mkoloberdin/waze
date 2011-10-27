package com.waze.widget.config;

import java.io.File;
import java.io.FileReader;
import java.util.HashMap;
import java.util.Scanner;

import com.waze.widget.WazeAppWidgetLog;

import android.location.Location;
import android.os.Environment;

public class WazeHistory {
	
	/***********************************************
	 * load
	 * 
	 */
	public static void load()
	{
		if (mMap != null)
			return;

		try{
    		File sdcard = Environment.getExternalStorageDirectory();
    		WazeAppWidgetLog.d(  "Loading history file" + sdcard +"," + mDirName  + mFileName  );
			
			File file = new File(sdcard, mDirName+mFileName);
    		Scanner scanner = new Scanner(new FileReader(file));
	        mMap = new HashMap<String, Location>();

	        while (scanner.hasNextLine()) {
	            String[] columns = scanner.nextLine().split(",");
	            Double latitude = new Double(columns[6]) / 1000000;
	            Double longitude = new Double(columns[7]) / 1000000;
	            Location location = new Location("History");
	            location.setLatitude(latitude);
	            location.setLongitude(longitude);
	            mMap.put(columns[5].toLowerCase(), location);
	        }
    	}
    	catch (Exception ex) {
    		WazeAppWidgetLog.e( "Failed to load history file [" + ex.getMessage()+"]");
    	}
	}
	
	/***********************************************
	 * getEntry
	 * @param name
	 * @return
	 */
	public static Location getEntry(String name){
		if (mMap != null)
			return (mMap.get(name.toLowerCase()));
		else
			return null;
	}
	
	private static HashMap<String, Location> mMap = null;
	private static String mDirName = "/waze/";
	private static String mFileName ="history";
}
