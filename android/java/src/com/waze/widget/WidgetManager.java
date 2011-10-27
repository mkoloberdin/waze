package com.waze.widget;

import android.app.Service;
import android.content.Context;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;

import com.waze.widget.StatusData;
import com.waze.widget.WazeAppWidgetService;
import com.waze.widget.config.WazeHistory;
import com.waze.widget.config.WazeLang;
import com.waze.widget.config.WazePreferences;
import com.waze.widget.config.WazeUserPreferences;
import com.waze.widget.routing.RouteScore;
import com.waze.widget.routing.RouteScoreType;
import com.waze.widget.routing.RoutingManager;
import com.waze.widget.routing.RoutingResponse;
import com.waze.widget.rt.RealTimeManager;

public class WidgetManager {
	static LocationManager mLocationManager;
	private static Service mService;
	static LocationListener mGpslocListener = null;
	static LocationListener mNetlocListener = null;
	
	/*************************************************************************************************
	 * init
	 * @param service
	 */
	public static void init(Service service){
		mService = service;
	   	mLocationManager = (LocationManager)service.getSystemService(Context.LOCATION_SERVICE);
	   	loadWazeConfig();
	}
	
	/*************************************************************************************************
	 * loadWazeConfig
	 * 
	 */
	public static void loadWazeConfig(){
		WazeAppWidgetLog.d("loadWazeConfig" );
    	WazePreferences.load();
    	WazeUserPreferences.load();
    	WazeHistory.load();
    	WazeLang.load(WazeUserPreferences.getProperty("System.Language"));
	}
	
	/*************************************************************************************************
	 * hasHomeWork
	 * @return - user has home & work defined
	 */
	public static Boolean hasHomeWork(){
		loadWazeConfig();
		Location locationHome = WazeHistory.getEntry(WazeLang.getLang("Home"));
    	Location locationWork = WazeHistory.getEntry(WazeLang.getLang("Work"));
    	
    	if ((locationHome == null) && (locationWork == null)) {
    		locationHome = WazeHistory.getEntry("home");
        	locationWork = WazeHistory.getEntry("work");
        	if ((locationHome == null) && (locationWork == null)) {
        		return false;
        	}
        	else{
        		return true;
        	}    	
        }
    	else{
    		return true;
    	}
	}
	
	/*************************************************************************************************
	 * executeRequest
	 * @param from
	 */
	public static void executeRequest(Location from){
		loadWazeConfig();
	
    	if (from == null){
    		WazeAppWidgetService.alertUser("No Location");
    		WazeAppWidgetService.setState( StatusData.STATUS_ERR_NO_LOCATION, new StatusData( "No Location", -1 ,RouteScoreType.NONE,null ));
    		WazeAppWidgetLog.w( "last Known Location is null" );
    		return;
    	}
    	
    	
	   	Destination dest = DestinationSelector.getDestination(from);
		WazeAppWidgetLog.d( "DestinationSelector selected " +dest.getName() );

		if (dest.getType() == DestinationType.NA){
  	    		WazeAppWidgetService.setState( StatusData.STATUS_ERR_NO_DESTINATION, new StatusData( "No Destination", -1 , RouteScoreType.NONE,null) );
    	}
    	else if (dest.getType() == DestinationType.NONE){
    		
    	}
    	else{
    		// Login 
    		if (WazeAppWidgetPreferences.RoutingServerAuthenticationNeeded())
    			RealTimeManager.getInstance().authenticate();

   	
    		RoutingManager rm = new RoutingManager();
    		RoutingResponse rrsp = rm.RoutingRequest(from, dest.getLocation());
           
    		if (rrsp == null){
    			WazeAppWidgetService.setState( StatusData.STATUS_ERR_ROUTE_SERVER, new StatusData( dest.getName(), -1 , RouteScoreType.NONE,null) );
    		}
    		else{
        		RouteScoreType score = RouteScore.getScore(rrsp.getTime(), rrsp.getAveragetTime());
        		WazeAppWidgetService.setState( StatusData.STATUS_SUCCESS, new StatusData(dest.getName(), rrsp.getTime()/60,  score, rrsp));
    		}
    	}
	}
	
	/*************************************************************************************************
	 * onLocation
	 * @param loc
	 */
	public static void onLocation(Location loc){
		if (mGpslocListener != null){
			mLocationManager.removeUpdates(mGpslocListener);
			mGpslocListener = null;
		}
		
		if (mNetlocListener != null){
			mLocationManager.removeUpdates(mNetlocListener);
			mNetlocListener = null;
		}
		
        WazeAppWidgetService.stopRefreshMonitor();
		executeRequest(loc);
		
	}
	
	/*************************************************************************************************
	 * RouteRequest
	 * 
	 */
	public static void RouteRequest(){
		WazeAppWidgetLog.d("RouteRequest..." );
		loadWazeConfig();
		
    	RealTimeManager rt = RealTimeManager.getInstance();
    	if (rt.hasUserName() == false){
    		WazeAppWidgetLog.e("No valide user credentials found");
			WazeAppWidgetService.setState( StatusData.STATUS_ERR_NO_DESTINATION, new StatusData( "No Destination", -1 ,RouteScoreType.NONE ,null) );
            WazeAppWidgetService.stopRefreshMonitor();
    		return;
    	}
    	
    	if (mService == null){
    		WazeAppWidgetLog.w( "service is null" );
    		WazeAppWidgetService.setState( StatusData.STATUS_ERR_NO_LOCATION, new StatusData( "No Location", -1 ,RouteScoreType.NONE,null ));
            WazeAppWidgetService.stopRefreshMonitor();
    		return;
    	}
    	
    	if (mLocationManager == null  )
    	   	mLocationManager = (LocationManager)mService.getSystemService(Context.LOCATION_SERVICE);

    	if (mLocationManager == null){
    		WazeAppWidgetService.alertUser("No Location");
    		WazeAppWidgetService.setState( StatusData.STATUS_ERR_NO_LOCATION, new StatusData( "No Location", -1 ,RouteScoreType.NONE,null ));
    		WazeAppWidgetLog.w( "locationManager is null" );
            WazeAppWidgetService.stopRefreshMonitor();
    		return;
    	}
    	
    	Location lastKnownLocation = mLocationManager.getLastKnownLocation( LocationManager.NETWORK_PROVIDER);
    	if (lastKnownLocation == null){
    		if (mGpslocListener ==null){
    			WazeAppWidgetLog.d("lastKnowLocation is null, activating GPS");
    			WazeAppWidgetService.alertUser(WazeLang.getLang("Current location cannot be determined, activating GPS"));
    			
    			if (mGpslocListener == null){
    				mGpslocListener = new WazeAppWidgetLocationListener();
    				mLocationManager.requestLocationUpdates( LocationManager.GPS_PROVIDER, 0, 0, mGpslocListener);
    			}
    			
    			if (mNetlocListener == null){
    				mNetlocListener = new WazeAppWidgetLocationListener();
    				mLocationManager.requestLocationUpdates( LocationManager.NETWORK_PROVIDER, 0, 0, mNetlocListener);
    			}
    		}
    		else{
    			WazeAppWidgetLog.d("lastKnowLocation is null, GPS already activated");
    		}
    	}
    	else{
    		executeRequest(lastKnownLocation);	
            WazeAppWidgetService.stopRefreshMonitor();

    	}
     	
	}
}
