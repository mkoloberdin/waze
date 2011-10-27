package com.waze.widget.routing;

import com.waze.widget.WazeAppWidgetLog;

public class RouteScore {

	
	/*************************************************************************************************
     * RouteScoreType - Rate the route compared to an average route time.
	 * @param time - route time
	 * @param avegare - all route avergae time
	 * @return - routeScore
     */
	static public RouteScoreType getScore(int time, int avegare){
		if (time > avegare * 1.10){
			//WazeAppWidgetLog.d("getScore of " + time + " avegarege is " + avegare +" is BAD");
			return RouteScoreType.ROUTE_BAD;
		}else if (time < avegare){
			//WazeAppWidgetLog.d("getScore of " + time + " avegarege is " + avegare +" is GOOD");
			return RouteScoreType.ROUTE_GOOD;
		}else{
			//WazeAppWidgetLog.d("getScore of " + time + " avegarege is " + avegare +" is OK");
			return RouteScoreType.ROUTE_OK;
		}
	}
}
