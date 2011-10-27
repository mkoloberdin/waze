package com.waze.widget;


import com.waze.widget.config.WazePreferences;

public class WazeAppWidgetPreferences {

	/*************************************************************************************************
     * RoutingRequest
	 * @return - Authentication is needed
	 */
	static public Boolean RoutingServerAuthenticationNeeded(){
		String  value= WazePreferences.getProperty("Widget.Authentication", "no");
		if ((value !=  null) && (value.equalsIgnoreCase("yes")))
			return true;
		else
			return false;
	} 
	
	/*************************************************************************************************
     * AllowRefreshTimer
	 * @return - Number of minutes before users can request a refresh
	 */
	static public long  AllowRefreshTimer(){
		Long defaultTimer = (long) (10); 
		String  value= WazePreferences.getProperty("Widget.Allow Refresh Timer", defaultTimer.toString());
		if (value == null)
			return defaultTimer;
		
		long val = Long.parseLong(value)*60*1000;
		return val;
	}
	
	/*************************************************************************************************
     * RoutingRequest
	 * @return - Routing server URL
	 */
	static public String getRoutingServerUrl(){
		String  value= WazePreferences.getProperty("Widget.Routing Server URL", "");
		return value;
	}
	
	/*************************************************************************************************
     * debugEnabled
	 * @return - debug level on/off
	 */
	static public Boolean debugEnabled(){
		String  value= WazePreferences.getProperty("General.Log level", "1");
		if (value.equalsIgnoreCase("1"))
			return true;
		else
			return false;
		
	}
	
	/*************************************************************************************************
     * SecuredServerUrl
	 * @return - RT sercured url
	 */
	static public String SecuredServerUrl(){
		return WazePreferences.getProperty("Realtime.Web-Service Secured Address");
	}
	
	/*************************************************************************************************
     * ServerUrl
	 * @return - RT  url
	 */
	static public String ServerUrl(){
		return WazePreferences.getProperty("Realtime.Web-Service Address");
	}

	
	/*************************************************************************************************
     * isWebServiceSecuredEnabled
	 * @return - SSL true/false
	 */
	static public boolean isWebServiceSecuredEnabled(){
		String value = WazePreferences.getProperty("Realtime.Web-Service Secure Enabled", "yes");	
		if ((value !=  null) && (value.equalsIgnoreCase("yes")))
			return true;
		else
			return false;
	}
	
	
	/*************************************************************************************************
     * getStartRange
	 * @return - Request start range fro gragh
	 */
	static public int getStartRange(){
		String  value= WazePreferences.getProperty("Widget.Start Range", "-60");
		return Integer.parseInt(value);
	}
	
	/*************************************************************************************************
     * getEndRange
	 * @return - Request end range fro gragh
	 */
	static public int getEndRange(){
		String  value= WazePreferences.getProperty("Widget.End Range", "60");
		return Integer.parseInt(value);
	}
}
