package com.waze.widget.routing;

import com.waze.widget.config.WazePreferences;
import com.waze.widget.config.WazeUserPreferences;

public class RoutingUserOptions {

	/***********************************************
	 * avoidPrimaries
	 * @return
	 */
	public static boolean avoidPrimaries(){
		String avoidPrimaries = WazeUserPreferences.getProperty("Routing.Avoid primaries", "no");
		return avoidPrimaries.equalsIgnoreCase("yes");
	}
	
	/***********************************************
	 * avoidTrails
	 * @return
	 */
	public static int avoidTrails(){
		String avoidPrimaries = WazeUserPreferences.getProperty("Routing.Avoid trails", "Don't allow");
		if (avoidPrimaries.equalsIgnoreCase("Allow"))
			return 1;
		else if (avoidPrimaries.equalsIgnoreCase("Don't allow"))
			return 0;
		else
			return 2;
	}
	
	/***********************************************
	 * preferSameStreet
	 * @return
	 */
	public static boolean preferSameStreet(){
		String preferSameStreet = WazePreferences.getProperty("Routing.Prefer same street","no");
		return preferSameStreet.equalsIgnoreCase("yes");
	}	
	
	
	/***********************************************
	 * userTraffic
	 * @return
	 */
	public static boolean userTraffic(){
		return true;
	}
	
	/***********************************************
	 * allowUnkownDirections
	 * @return
	 */
	public static boolean allowUnkownDirections(){
		String allowUnkown = WazeUserPreferences.getProperty("Routing.Allow unknown directions", "yes");
		return allowUnkown.equalsIgnoreCase("yes");
	}
	
	/***********************************************
	 * avoidTolls
	 * @return
	 */
	public static boolean avoidTolls(){
		String avoidTolls = WazeUserPreferences.getProperty("Routing.Avoid tolls", "no");
		return avoidTolls.equalsIgnoreCase("yes");
	}

	/***********************************************
	 * preferUnkownDirections
	 * @return
	 */
	public static boolean preferUnkownDirections(){
		String preferUnkownDirections = WazeUserPreferences.getProperty("Routing.Prefer unknown directions", "no");
		return preferUnkownDirections.equalsIgnoreCase("yes");
	}
	
	/***********************************************
	 * avoidPalestinianRoads
	 * @return
	 */
	public static boolean avoidPalestinianRoads(){
		String preferUnkownDirections = WazeUserPreferences.getProperty("Routing.Avoid Palestinian Roads", "yes");
		return preferUnkownDirections.equalsIgnoreCase("yes");
	}
	
	/***********************************************
	 * routeType
	 * @return
	 */
	public static RoutingType routeType(){
		String routeType = WazeUserPreferences.getProperty("Routing.Type", "Fastest");
		if (routeType.equalsIgnoreCase("Fastest"))
			return RoutingType.HISTORIC_TIME;
		else
		    return RoutingType.DISTANCE;	
	}
	
}
