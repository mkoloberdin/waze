package com.waze.widget.config;



public class WazeUserPreferences {
	
	/***********************************************
	 * load
	 * @return
	 */
	public static void load()
	{
		if (mConfig == null){
			mConfig = new WazeConfig(mDirName+mFileName);
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
	
	/************************************************
	 * getProperty
	 * @param name
	 * @param default_value
	 * @return
	 */
	public static String getProperty(String name, String default_value){
		
		if (mConfig == null)
			load();
		
		if (mConfig == null)
			return default_value;
		else
			return mConfig.getProperty(name, default_value);
	}
	
	private static WazeConfig mConfig = null;
	private static String mDirName = "/data/data/com.waze/";
	private static String mFileName ="user";
}
