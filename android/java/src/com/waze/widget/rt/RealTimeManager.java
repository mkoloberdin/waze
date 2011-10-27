package com.waze.widget.rt;

import java.io.IOException;

import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.util.EntityUtils;

import com.waze.widget.WazeAppWidgetLog;
import com.waze.widget.WazeAppWidgetPreferences;
import com.waze.widget.config.WazePreferences;
import com.waze.widget.config.WazeUserPreferences;


public class RealTimeManager {
	
	/*************************************************************************************************
     * getInstance
     * 
     */
	public static RealTimeManager getInstance()
	{	
		if (mInstance != null)
			return mInstance;
		else{
			mInstance = new RealTimeManager();
			return mInstance;
		}
	}
	
	/*************************************************************************************************
     * loadRealTimeParams
     * 
     */
	private void loadRealTimeParams(){
		mUserName  = WazeUserPreferences.getProperty("Realtime.Name");
		mPassword  = WazeUserPreferences.getProperty("Realtime.Password");
		mServerUrl = WazeAppWidgetPreferences.ServerUrl();
		mSecuredServerUrl =  WazeAppWidgetPreferences.SecuredServerUrl();
		mIsSecuredConnection = WazeAppWidgetPreferences.isWebServiceSecuredEnabled();
	}
	
	/*************************************************************************************************
     * RealTimeManager
	 * @return 
     */
	private RealTimeManager(){
		loadRealTimeParams();
	}
	
	
	/*************************************************************************************************
     * getUserName
	 * @return 
     */
	public String getUserName(){
		return mUserName;
	}
	
	/*************************************************************************************************
     * getPassword
	 * @return 
     */
	public String getPassword(){
		return mPassword;
	}
	
	/*************************************************************************************************
     * hasUserName
	 * @return 
     */
	public Boolean hasUserName(){
		if ((getUserName() == null) || (getPassword() == null))
			return false;
		else
			return true;
	}
	
	/*************************************************************************************************
     * getSessionId
	 * @return 
     */
	public String getSessionId(){
		if (mAuthenticationRsp == null)
			return null;
		return mAuthenticationRsp.getSessionId();
	}
	
	/*************************************************************************************************
     * getCookie
	 * @return 
     */
	public String getCookie(){
		if (mAuthenticationRsp == null)
			return null;
		return mAuthenticationRsp.getCookie();
	}
	
	/*************************************************************************************************
     * authenticate
     * 
     */
	public void authenticate(){
		if ((mUserName == null) || (mPassword == null)){
			WazeAppWidgetLog.w( "login aborted [" + mUserName + "]");
			return;
		}
		
		if (((mIsSecuredConnection == true) && (mSecuredServerUrl == null)) || ((mIsSecuredConnection == false) && (mServerUrl == null))){
			WazeAppWidgetLog.w( "login aborted [mIsSecuredConnection=" + mIsSecuredConnection + " mSecuredServerUrl=" + mSecuredServerUrl  +",mServerUrl="+mServerUrl+"]");
			return;
		}

		
		AuthenticateRequest authReq = new AuthenticateRequest(mUserName, mPassword);
		try
		{
			WazeAppWidgetLog.d( "Sending Authenticate request"  );
			HttpClient httpclient = new DefaultHttpClient();
			String url;
			
			if (mIsSecuredConnection)
				url = mSecuredServerUrl;
			else
				url = mServerUrl;
		    HttpPost httppost = new HttpPost(url+"/login");

		    httppost.addHeader("Content-Type", "binary/octet-stream"); 
		    String cmd = authReq.buildCmd();
		    httppost.setEntity(new StringEntity(cmd)); 
	        HttpResponse rp = httpclient.execute(httppost);

		if(rp.getStatusLine().getStatusCode() == HttpStatus.SC_OK)
		{
			String str = EntityUtils.toString(rp.getEntity());
			mAuthenticationRsp = new AuthenticateSuccessfulResponse(str);
			
		}
		else{
			WazeAppWidgetLog.e("Authenticate failed [error =" + rp.getStatusLine().getStatusCode()+ "]");
		}
		}catch(IOException e){
			WazeAppWidgetLog.e("Authenticate error [error=" + e.getMessage()+ "]");
		}

	}
	
	/*************************************************************************************************
     * getProtocol
	 * @return
	 */
	public static String getProtocol(){
		return Integer.toString(PROTOCL_VERSION);
	}
	
	
	private final static int PROTOCL_VERSION = 146;
	
	private static RealTimeManager mInstance = null;
	private AuthenticateSuccessfulResponse mAuthenticationRsp; 
	private String mUserName;
	private String mPassword;
	private String mServerUrl;
	private String mSecuredServerUrl;
	private boolean mIsSecuredConnection; 
}
