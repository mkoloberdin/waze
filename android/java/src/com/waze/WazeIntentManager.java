/**  
 * WazeIntentManager - Handles in/out intents   
 *            
 * TODO:: Pass the intent functionality from the service   
 * 
 * 
 * LICENSE:
 *
 *   Copyright 2010     @author Alex Agranovich (AGA),	Waze Mobile Ltd
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

import android.app.Activity;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ContentUris;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Looper;
import android.os.SystemClock;
import android.provider.Contacts;
import android.util.Log;


public final class WazeIntentManager 
{

    /*************************************************************************************************
     *================================= Public interface section =================================
     */
    /*************************************************************************************************
     * Create - Intent manager instance creator  
     * @param aContext - activity reference
     * @return WazeIntentManager instance 
     */
	static WazeIntentManager Create()
	{
		mInstance = new WazeIntentManager();
		return mInstance;
	}
	
	/*************************************************************************************************
     * HandleUri - Intent manager instance creator  
     * @param aUrl - uri of the intent
     * @param aContext - the target activity
     * @return true if the parameters are valid and query is in decoding process 
     */
	public static boolean HandleUri( Uri aUrl, Activity aContext )
	{
		boolean res = false;
		FreeMapNativeManager nativeMgr = FreeMapAppService.getNativeManager();
		String resUrl = null;
		
    	if ( aUrl == null ) return res;
    	
    	String scheme = aUrl.getScheme();
    	if ( scheme.equals( "waze" ) )
    	{
    		resUrl = aUrl.toString();;
    	}

    	if ( scheme.equals( "geo" ) )
    	{
    		resUrl = ConvertGeoUri( aUrl.toString() );
    	}

    	if ( scheme.equals( "content" ) )
    	{
    		String address = FetchContactAddress( aUrl, aContext );
    		if ( address != null )
    		{
    			resUrl = "waze://?q=" + address;
    		}
    	}
    	/*
    	 * If the string is ready try to handle it
    	 */
    	if ( ( nativeMgr != null ) && ( nativeMgr.IsAppStarted() ) )
		{
    		nativeMgr.UrlHandler( resUrl );    		
		}
    	else
    	{
    		FreeMapAppService.setUrl( resUrl );
    	}
    	
    	res = ( resUrl != null );
		return res;
	}
	
	/*************************************************************************************************
     * RequestRestart - One shot alarm for application execution  
     * @param aContext - current context
     * @return void 
     */
	public static void RequestRestart( Context aContext )
	{
		AlarmManager mgr = ( AlarmManager ) aContext.getSystemService( Context.ALARM_SERVICE );
		Intent i = new Intent( aContext, FreeMapAppActivity.class );
		PendingIntent pi = PendingIntent.getBroadcast( aContext, 0, i, 0 );

		mgr.set( AlarmManager.ELAPSED_REALTIME_WAKEUP, SystemClock.elapsedRealtime() + 5000, pi );
	}
	public static class WazeSDCardManager extends BroadcastReceiver
	{
	    /*************************************************************************************************
	     * Event handler for the intent
	     * 
	     */
	    @Override
	    public void onReceive( Context context, Intent intent )
	    {
	        String action = intent.getAction();
	        if ( Intent.ACTION_MEDIA_REMOVED.equals( action ) || 
	        		Intent.ACTION_MEDIA_UNMOUNTED.equals( action ) ) 
	        {
	        	FreeMapNativeManager mgr = FreeMapAppService.getNativeManager();
	        	mgr.CheckSDCardAvailable();
	        }
	    }
	}
    /*************************************************************************************************
     *================================= Native methods section ================================= 
     * These methods are implemented in the native side and should be called after!!! the shared library is loaded
     */

	/*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */
	private WazeIntentManager()
	{		
	}
	
    /*************************************************************************************************
     * FetchContactAddress - Extracts the address from the database
     * 
     * @param aUri - uri from the intent
     * @return Address if found or null otherwise
     * TODO:: Check if the scheme is content           
     */
    private static String FetchContactAddress( Uri aUri, Activity aContext )
    {
    	String address = null;
    	/* Prepare the projection to extract from the database */
        String[] projection = new String[] {                        
                Contacts.ContactMethods._ID,
                Contacts.ContactMethods.KIND,
                Contacts.ContactMethods.DATA };
        /* Run the query 
         * Note: where clause is not working in this case
         * */
        Cursor cur = aContext.managedQuery( Contacts.ContactMethods.CONTENT_URI, projection, null, null, null );
        if ( cur == null )
        {
        	Log.w("WAZE", "No data for uri: " + aUri.toString() );
        }
        else
        {
        	if (cur.moveToFirst()) 
        	{
        		long personId = ContentUris.parseId( aUri );	
        		int indexID = cur.getColumnIndex( Contacts.ContactMethods._ID );
        		int indexDATA = cur.getColumnIndex( Contacts.ContactMethods.DATA );
        		int indexKIND = cur.getColumnIndex( Contacts.ContactMethods.KIND );
	            do 
	            {
	            	long nextId = cur.getLong( indexID );
	            	int kind = cur.getInt( indexKIND );
	            	if ( nextId == personId && kind == Contacts.KIND_POSTAL )
	            	{
	            		address = cur.getString( indexDATA );
	            		break;
	            	}
	            } while (cur.moveToNext());
        	}
        }
        return address;
    }
		
    /*************************************************************************************************
     * Converts geo scheme to the native scheme
     * @param void
     */
    private static String ConvertGeoUri( String aGeoUri )
    {
    	String res = null;
    	String scheme = "geo:";
    	String dummy_loc = "0,0?";
//    	char zoom_chr = 'z';
//    	char query_chr = 'q';
    	
    	String waze_prefix = "waze://?";
    	String ll_prefix = "ll=";
    	if ( aGeoUri.startsWith( scheme ) )
    	{
    		String token = aGeoUri.substring( scheme.length() );

//    		Log.w( "WAZE DEBUG", "token : " + token );
    		// If non zero digits - add "ll=" prefix, otherwise suppose
    		// q or z are is the start of the token 
    		if ( token.startsWith( dummy_loc ) )
    		{
    			res = waze_prefix + token.substring( dummy_loc.length() );
    		}    		
    		else
    		{
    			token = token.replace( '?', '&' );
    			res = waze_prefix + ll_prefix + token;
    		}
    	}
//    	Log.w( "WAZE DEBUG", "res : " + res );
        return res;
    }
	/*************************************************************************************************
     *================================= Members section =================================
     */
	static WazeIntentManager mInstance = null;					// Instance reference
	
	/*************************************************************************************************
     *================================= Constants section =================================
     */
	
}
