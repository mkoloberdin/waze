/**  
 * ConnEventReceiver.java - Responsible for receiving the connectivity events.  
 *   
 * 
 * 
 * LICENSE:
 *
 *   Copyright 2011     @author Alex Agranovich (AGA)
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
 *   @see 
 */
package com.waze;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

final public class ConnEventReceiver extends BroadcastReceiver
{
	/*************************************************************************************************
     *================================= Public interface section =================================
     */
	public ConnEventReceiver()
	{	
	}

    /*************************************************************************************************
     * Notification event 
     */
	@Override
	public void onReceive( Context aContext, Intent aIntent )
	{
		Boolean connectivity = !aIntent.getBooleanExtra( ConnectivityManager.EXTRA_NO_CONNECTIVITY, false );
		NetworkInfo netInfo = aIntent.getParcelableExtra( ConnectivityManager.EXTRA_NETWORK_INFO ); 
		
		Log.i( WazeLog.TAG( "ConnEventReceiver" ), "Received event: " + aIntent.getAction() + ". Connectivity: " + connectivity + 
				". Type: " + netInfo.getTypeName() + " ( " + netInfo.getType() + " )" + 
				". State: " + netInfo.getState().toString() + 
				". Connected: " + netInfo.isConnected() );
		
		FreeMapNativeManager mgr = FreeMapAppService.getNativeManager();
		
		if ( mgr != null )
			mgr.ConnectivityChanged( netInfo.isConnected(), netInfo.getType(),  netInfo.getTypeName() );
		
	}

	
	/*************************************************************************************************
     *================================= Private interface section =================================
     */
    

    
    /*************************************************************************************************
     *================================= Native methods section ================================= 
     * These methods are implemented in the native side and should be called after!!! the shared library is loaded
     */

    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */	
    
}
