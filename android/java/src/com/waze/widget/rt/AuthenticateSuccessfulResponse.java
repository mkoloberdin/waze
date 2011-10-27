package com.waze.widget.rt;

import com.waze.widget.WazeAppWidgetLog;


public class AuthenticateSuccessfulResponse {
	private String mSessionId;
	private String mCookie;
	
	/*************************************************************************************************
     * getCookie
     * 
     */
	public String getCookie(){
		return mCookie;
	}
	
	/*************************************************************************************************
     * getSessionId
     * 
     */
	public String getSessionId(){
		return mSessionId;
	}
	
	/*************************************************************************************************
     * AuthenticateSuccessfulResponse
     * 
     * @param responseStr
     */
	public AuthenticateSuccessfulResponse(String responseStr){
		String[] lines = responseStr.split("\n");
		String[] params = lines[0].split(",");
		String status = params[1];
		if (status.equalsIgnoreCase("200")){
			WazeAppWidgetLog.d( "Got AuthenticateSuccessful response [" +   lines[1] +"]" );	
			String[] responseParams = lines[1].split(",");
			if (responseParams[0].equalsIgnoreCase("LoginError")){
				WazeAppWidgetLog.e( "Authenticate failed status =" + responseParams[1]);
				return;
			}
			else{
				mSessionId = responseParams[1];
				mCookie = responseParams[2];
			}
		}else{
			WazeAppWidgetLog.e( "Authenticate failed status =" + params[1] +" details=" + params[2]);
		}
	}
}
