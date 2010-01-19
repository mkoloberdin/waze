/**  
 * FreeMapNativeLocListener.java
 * This class is responsible for collecting the GPS data from the device
 * and its forwarding to the native layer for the further handling
 *   
 * 
 * LICENSE:
 *
 *   Copyright 2008 	@author Alex Agranovich
 *   
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * SYNOPSYS:
 *
 *   @see FreeMapNativeActvity.java
 */

package com.waze;

import java.util.Iterator;

import android.location.GpsSatellite;
import android.location.Location;
import android.location.GpsStatus;
import android.location.LocationListener;
import android.location.LocationManager;
import android.location.LocationProvider;
import android.os.Bundle;
import android.util.Log;


public final class FreeMapNativeLocListener implements LocationListener
{

    /*************************************************************************************************
     *================================= Public interface section
     * =================================
     * 
     */

    public class NativeLocation
    {
        int mGpsTime;
        int mLatitude;
        int mLongtitude;
        int mAltitude;
        int mSpeed;
        int mSteering;
    }

    public FreeMapNativeLocListener(LocationManager aManager)
    {
        mLocationManager = aManager;
        mLocationManager.addGpsStatusListener( new GpsStatusListener() );
    }

    /*************************************************************************************************
     * 
     * 
     */
    public void onLocationChanged( Location location )
    {
        UpdateNativeLayer( mStatusAvailable, location );
//        mLocationManager.removeUpdates( this );
//        mLocationManager.requestLocationUpdates( LocationManager.GPS_PROVIDER, 0, 0,
//                this );
    }

    /*************************************************************************************************
     * 
     * 
     */
    public void onProviderDisabled( String provider )
    {
        // Covered in onStatusChanged
        // // Adjustment of the parameters for the native layer
        // NativeLocation nativeLocation = GetNativeLocation(
        // mLocationManager.getLastKnownLocation( provider ) );
        // UpdateNativeLayer( mStatusNotAvailable, nativeLocation );

    }

    /*************************************************************************************************
     * 
     * 
     */
    public void onProviderEnabled( String provider )
    {
        // Covered in onStatusChanged
        // // Adjustment of the parameters for the native layer
        // NativeLocation nativeLocation = GetNativeLocation(
        // mLocationManager.getLastKnownLocation( provider ) );
        // UpdateNativeLayer( mStatusAvailable, nativeLocation );
    }

    /*************************************************************************************************
     * 
     * 
     */
    public void onStatusChanged( String provider, int status, Bundle extras )
    {
        byte lStatus;
        Location lastLoc;
        if (status == LocationProvider.AVAILABLE)
        {
            lStatus = mStatusAvailable;
        }
        else
        {
            lStatus = mStatusNotAvailable;
        }

        // Adjustment of the parameters for the native layer
        lastLoc = mLocationManager.getLastKnownLocation(provider);
        if (lastLoc != null)
        {
            UpdateNativeLayer( lStatus, lastLoc );
        }
    }

    /*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */

    /*************************************************************************************************
     * Convert the location data to the format that is necessary for the native
     * layer
     * 
     */
    private NativeLocation GetNativeLocation( Location aLocation )
    {
        NativeLocation lNativeLoc = new NativeLocation();

        lNativeLoc.mLongtitude = (int) Math.round(aLocation.getLongitude()
                * (double) mFixedPointFactor);

        lNativeLoc.mLatitude = (int) Math.round(aLocation.getLatitude()
                * (double) mFixedPointFactor);

        lNativeLoc.mAltitude = (int) Math.round(aLocation.getAltitude()
                * (double) mFixedPointFactor);

        lNativeLoc.mGpsTime = (int) (aLocation.getTime() / 1000); // Time in
        // seconds

        lNativeLoc.mSpeed = (int) (aLocation.getSpeed() * 1.944); // m/s to
        // knots

        lNativeLoc.mSteering = (int) aLocation.getBearing();

        return lNativeLoc;
    }

    private void UpdateNativeLayer( byte aStatus, Location aLoc )
    {
        // Log the update
        int isCellData = 0;
        double hdop;
        if ( aLoc == null )
            return;
        
        NativeLocation loc = GetNativeLocation( aLoc );
        
        // No GPS - use cell information
        if ( aLoc.getProvider().equals( LocationManager.NETWORK_PROVIDER ) )
        {
            isCellData = 1;
        }
       
        // GPS Available - handle trough the satellites
        if ( aLoc.getProvider().equals( LocationManager.GPS_PROVIDER ) )
        {
        	int satelliteNumber = 0;
            // Update native layer wit the satellite information
//            Bundle extraParams = aLoc.getExtras();
//            if ( extraParams != null )
//            {
//                satelliteNumber = extraParams.getInt( "satellites");
//                SatteliteListenerCallbackNTV( satelliteNumber );
//            }
            
            // Why this???? Taken from the iphone implementation
            //Dilution data
            if ( aLoc.getAccuracy() <= 20) 
            {
               hdop = 1;
            } 
            else if (aLoc.getAccuracy() <= 50) 
            {
               hdop = 2;
            } 
            else if (aLoc.getAccuracy() <= 100) 
            {
               hdop = 3;
            } 
            else 
            {
               hdop = 4;
            }
            // Update native layer wit the satellite information
            DilutionListenerCallbackNTV( 3, 0, hdop, 0 );
        }

        // Update the native layer with location information
        LocListenerCallbackNTV( mStatusAvailable, loc.mGpsTime, loc.mLatitude,
                loc.mLongtitude, loc.mAltitude, loc.mSpeed, loc.mSteering, isCellData );

    }

    
    private class GpsStatusListener implements GpsStatus.Listener
    {
    	public void onGpsStatusChanged( int aEvent )
    	{
    		int satelliteNumber = 0;
    		switch ( aEvent )
    		{
    			case GpsStatus.GPS_EVENT_SATELLITE_STATUS:
    			{
    	            GpsStatus status = mLocationManager.getGpsStatus( null );
    	            Iterator<GpsSatellite> iter = status.getSatellites().iterator();
    	            // Count the satellites
    	            while( iter.hasNext() )
    	            {
    	            	satelliteNumber++;
    	            	iter.next();
    	            }
    	            // Update the native layer
    	        	SatteliteListenerCallbackNTV( satelliteNumber );
    				break;
    			}
    			
    		}
    	}
    }
    
    /*************************************************************************************************
     *================================= Native methods section ================================= 
     * These methods are implemented in the
     * native side and should be called after!!! the shared library is loaded
     */
    private native void LocListenerCallbackNTV( byte aStatus, int aGpsTime,
            int aLatitude, int aLongtitude, int aAltitude, int aSpeed,
            int aSteering, int aIsCellData );
    private native void SatteliteListenerCallbackNTV( int aSatteliteNumber );
    private native void DilutionListenerCallbackNTV( int aDim, double aPdop, double aHdop, double aVdop );

    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */
    private LocationManager mLocationManager;

    // ===== Constants ====
    private final int       mFixedPointFactor   = 1000000;
    private final byte      mStatusAvailable    = 'A';
    private final byte      mStatusNotAvailable = 'V';
    private int mSatelliteNumber = 0;
}
