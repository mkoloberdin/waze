package com.waze.widget.routing;

public enum RoutingOption {

	NONE(false), 
	AVOID_PRIMARIES(false), 
	AVOID_TRAILS(false), 
	AVOID_LONG_TRAILS(true), 
	PREFER_SAME_STREET(false), 
	IGNORE_REALTIME_INFO(false), 
	ALLOW_UNKNOWN_DIRECTIONS(false), 
	ALLOW_NEAR_BY_SOURCE(true), 
	ALLOW_NEAR_BY_TARGET(true),
	IGNORE_SEGMENT_INFO(false),
	AVOID_TOLL_ROADS(false),
	IGNORE_PENALTIES(false),
	PREFER_UNKNOWN_DIRECTIONS(false),
	AVOID_DANGER_ZONES(false),
	PREFER_LESS_SEGMENTS(true),
	IGNORE_HISTORIC_INFO(false),
	USE_EXTENDED_INSTRUCTIONS(false),
	AVOID_LANDMARKS(false),
	PREFER_COMMON_ROUTES(false),
	;
	
	private boolean defaultValue;
	
	/*************************************************************************************************
     * RoutingOption
     * 
     */
	private RoutingOption(boolean defaultValue)
	{
		this.defaultValue = defaultValue;
	}
	
	/*************************************************************************************************
     * getDefault
     * 
     */
	public boolean getDefault()
	{
		return defaultValue;
	}
}

