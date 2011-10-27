package com.waze.widget.routing;

import java.io.IOException;

import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.util.EntityUtils;
import org.json.JSONException;

import android.location.Location;

import com.waze.widget.WazeAppWidgetLog;
import com.waze.widget.WazeAppWidgetPreferences;
import com.waze.widget.WazeAppWidgetService;
import com.waze.widget.rt.RealTimeManager;


public class RoutingManager {
	private String mServerUrl;
	
	/*************************************************************************************************
     * RoutingManager
     * 
     */
	public RoutingManager(){
		mServerUrl = WazeAppWidgetPreferences.getRoutingServerUrl();
	}
	
	
	/*************************************************************************************************
     * RoutingRequest
	 * @param from - from location
	 * @param to - to location
	 * @return - routing response
	 */
	public RoutingResponse RoutingRequest(Location from, Location to){
    	RealTimeManager rt = RealTimeManager.getInstance();
    	// Routing request
		RoutingRequest rr = new RoutingRequest(from, to, RoutingUserOptions.routeType(), rt.getSessionId(), rt.getCookie());
	
		rr.addOption(RoutingOption.AVOID_TOLL_ROADS, RoutingUserOptions.avoidTolls());
		rr.addOption(RoutingOption.AVOID_DANGER_ZONES, RoutingUserOptions.avoidPalestinianRoads());
		rr.addOption(RoutingOption.AVOID_PRIMARIES, RoutingUserOptions.avoidPrimaries());
		rr.addOption(RoutingOption.PREFER_SAME_STREET, RoutingUserOptions.preferSameStreet());

		RoutingResponse rrsp = execute(rr);
		
		return rrsp;

	}
	
	
	/*************************************************************************************************
     * execute - execute request
	 * @param rr - routing request
	 * @return - routing response
	 */
	private RoutingResponse execute(RoutingRequest rr){
		try
		{
			if (mServerUrl == null || mServerUrl.length() <= 0){
				WazeAppWidgetLog.e( "Sending routing request [mServerUrl is null]" );
				return null;
			}
			
			WazeAppWidgetLog.d( "Sending routing request [" +mServerUrl+"/routingRequest"+rr.buildCmd()+"]" );
			
			HttpClient httpclient = new DefaultHttpClient();
		    HttpPost httppost = new HttpPost(mServerUrl+"/routingRequest"+rr.buildCmd());
	        HttpResponse rp = httpclient.execute(httppost);

	        if(rp.getStatusLine().getStatusCode() == HttpStatus.SC_OK)
	        {
	        	String str = EntityUtils.toString(rp.getEntity());
	        	WazeAppWidgetLog.d(  "Got routing response [" +str+"]" );
	        	try{
	        		RoutingResponse routingResponse = new RoutingResponse(str);
	        		return routingResponse;
	        	
	        	} catch (JSONException e) {
	        		return null;
	        	}
	        }
	        else{
	        	WazeAppWidgetLog.e("routing request failed code=" +rp.getStatusLine().getStatusCode());
    			WazeAppWidgetService.alertUser("routing request failed (code=" +rp.getStatusLine().getStatusCode()+")");
	        	return null;
	        }
		}catch(IOException e){
			WazeAppWidgetLog.e( "routing request Http post error " + e.getMessage());
			WazeAppWidgetService.alertUser("routing request Http error (" +e.getMessage()+")");

			return null;
		}
		
	}
}
