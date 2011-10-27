package com.waze.widget.rt;

public class AuthenticateRequest {
	private String mUserName ;
	private String mPassword ;
	
	
	/*************************************************************************************************
	 * AuthenticateRequest
	 * @param userName
	 * @param passWord
	 * @return 
     */
	AuthenticateRequest(String userName, String passWord){
		mUserName = userName;
		mPassword = passWord;
	}
	
	/*************************************************************************************************
     * buildCmd
	 * @param void
	 * @return String - the cmd string
	 *  
     */
	String buildCmd(){
		
		String cmd = "Authenticate," + RealTimeManager.getProtocol() + "," + mUserName + "," + mPassword ;
		return cmd;
	}
}
