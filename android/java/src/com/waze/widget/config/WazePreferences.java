package com.waze.widget.config;

import java.io.File;

import android.os.Environment;


public class WazePreferences {
	
	
	/***********************************************
	 * load
	 * @return
	 */
	public static void load()
	{
		if (mConfig == null){
			File sdcard = Environment.getExternalStorageDirectory();
			mConfig = new WazeConfig(sdcard+mDirName+mFileName);
			mConfig.load();
		}
	
    }
	
	/***********************************************
	 * getProperty
	 * @param name
	 * @return
	 */
	public static String getProperty(String name){
		if (mConfig == null)
			load();
		
		return mConfig.getProperty(name);
	}
	
	/***********************************************
	 * getProperty
	 * @param name
	 * @param default_value
	 * @return
	 */
	public static String getProperty(String name, String default_value){
		if (mConfig == null)
			load();
		
		if (mConfig != null)
			return mConfig.getProperty(name, default_value);
		else
			return default_value;

	}
	
	
	private static WazeConfig mConfig = null;
	private static String mDirName = "/waze/";
	private static String mFileName ="preferences";

}
