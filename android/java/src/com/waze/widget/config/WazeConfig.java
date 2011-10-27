package com.waze.widget.config;

import java.io.File;
import java.io.FileReader;
import java.util.HashMap;
import java.util.Scanner;

import com.waze.widget.WazeAppWidgetLog;


public class WazeConfig {
	
	/***********************************************
	 * WazeConfig
	 * @param directory
	 * @param fileName
	 */
	WazeConfig (String fileName){
		mFileName = fileName;
		mMap = new HashMap<String, String>();
	}
	
	/***********************************************
	 * load
	 * 
	 */
    void load()
	{
		try{
			
    		File file = new File(mFileName);
    		Scanner scanner = new Scanner(new FileReader(file));
	        mMap = new HashMap<String, String>(); 

	        while (scanner.hasNextLine()) {
	            String[] columns = scanner.nextLine().split(": ");
	            if (columns.length > 1)
	            	mMap.put(columns[0], columns[1]);
	        }
	        
	        WazeAppWidgetLog.d( "config file " +  mFileName  + " Loaded");
				
    	}
    	catch (Exception ex) {
    		mMap = null;
    		WazeAppWidgetLog.e( "Failed to load config file " +  mFileName);
    	}
    }
	
	/***********************************************
	 * getProperty
	 * @param name
	 * @return
	 */
    String getProperty(String name){
		if (mMap == null)
			load();
		
		if (mMap != null)
			return (mMap.get(name));
		else
			return null;
	}
	
	/************************************************
	 * getProperty
	 * @param name
	 * @param default_value
	 * @return
	 */
	String getProperty(String name, String default_value){
		String val = getProperty(name);
		if (val == null)
			return default_value;
		else
			return val;
	}
	
	
	private String mFileName;
	private HashMap<String, String> mMap = null;
	
}
