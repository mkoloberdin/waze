package com.waze.widget;

import android.location.Location;
import android.location.LocationListener;
import android.os.Bundle;

public class WazeAppWidgetLocationListener implements LocationListener
{
	
	
	public void onLocationChanged(Location loc)
	{
		WazeAppWidgetLog.d("onLocationChanged called");
		WidgetManager.onLocation(loc);
	}

	
	public void onProviderDisabled(String provider)
	{
//		WazeAppWidgetService.alertUser("Gps Disabled");
	}

	
	public void onProviderEnabled(String provider)
	{
	}

	public void onStatusChanged(String provider, int status, Bundle extras)
	{
		
	}
}/* End of Class WazeAppWidgetLocationListener */