package com.waze.widget;

import android.location.Location;

public class Destination {
	private DestinationType mDestType;
	private String 			mDestName;
	private Location		mDestLocation;
	
	/*************************************************************************************************
     * Destination
	 * @param type
	 * @param name
	 * @param location
	 */
	public Destination(	DestinationType type, String 	name, Location location){
		mDestType = type;
		mDestName = name;
		mDestLocation = location;
	}
	
	/*************************************************************************************************
     * getName
	 * @return
	 */
	public String getName(){
		return mDestName;
	}
	
	/*************************************************************************************************
     * getType
	 * @return
     */
	public DestinationType getType(){
		return mDestType;
	}
	
	/*************************************************************************************************
     * getLocation
	 * @return
     */
	public Location getLocation(){
		return mDestLocation;
	}

	/*************************************************************************************************
     * setName
	 * @param name
	 */
	public void setName(String name){
	    mDestName = name;
	}
	
	/*************************************************************************************************
     * setType
	 * @param type
	 */
	public void setType(DestinationType type){
		mDestType = type;
	}
	
	/*************************************************************************************************
     * setLocation
	 * @param location
	 */
	public void setLocation(Location location){
		mDestLocation = location;
	}
}
