package com.waze.widget;

import java.util.Calendar;
import java.util.GregorianCalendar;

import android.location.Location;

import com.waze.widget.config.WazeHistory;
import com.waze.widget.config.WazeLang;

public class DestinationSelector {
	
	/*************************************************************************************************
     * getDestination - selects a destination (home/work) according to time of day
	 * @param currentLocation
	 * @return Destination address
	 *  
     */
	public static Destination getDestination(Location currentLocation){
		Calendar calendar = new GregorianCalendar();
		Destination dest = new Destination(DestinationType.NONE, null, null);
		
		
		if (currentLocation == null){
			WazeAppWidgetLog.w( "currentLocation is null" );
			return dest;
		}

		Location locationHome = WazeHistory.getEntry(WazeLang.getLang("Home"));
    	Location locationWork = WazeHistory.getEntry(WazeLang.getLang("Work"));
       	if ((locationHome == null) && (locationWork == null)) {
    		locationHome = WazeHistory.getEntry("home");
        	locationWork = WazeHistory.getEntry("work");
       	}
       	
    	if ((locationHome == null) && (locationWork == null)) {
    		WazeAppWidgetLog.w( "No Home & Work" );
    		dest.setType(DestinationType.NA);
			return dest;
		}
    	
    	if(calendar.get(Calendar.AM_PM) == 0){
			//AM
    		if (locationWork != null){
            	float distanceWork = locationWork.distanceTo(currentLocation);
        		if (distanceWork > 1000){
        			dest.setType(DestinationType.WORK);
        			dest.setLocation(locationWork);
        			dest.setName(WazeLang.getLang("Work"));
        		}else if (locationWork != null){
        			dest.setType(DestinationType.HOME);
        			dest.setLocation(locationHome);
        			dest.setName(WazeLang.getLang("Home"));
        		}
    		}
    		else{
    			WazeAppWidgetLog.w( "No Work" );
        		dest.setType(DestinationType.NA);
    		}
    	}
    	else{
    		//PM
       		if (locationHome != null){
       			float distanceHome= locationHome.distanceTo(currentLocation);
       			if (distanceHome > 1000){
       				dest.setType(DestinationType.HOME);
       				dest.setLocation(locationHome);
       				dest.setName(WazeLang.getLang("Home"));
       			} else if (locationWork != null){
       	   			dest.setType(DestinationType.WORK);
        			dest.setLocation(locationWork);
        			dest.setName(WazeLang.getLang("Work"));
       			}
       		}
      		else{
      			WazeAppWidgetLog.w( "No Home" );
        		dest.setType(DestinationType.NA);
    		}
    	}
		
		return dest;
	}
	
}
