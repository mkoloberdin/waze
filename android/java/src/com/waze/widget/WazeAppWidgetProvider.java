/**  
 * WazeAppWidgetProvider - Provider interface for the waze widget
 *            			
 *   
 * 
 * 
 * LICENSE:
 *
 *   Copyright 2011     @author Alex Agranovich (AGA),	Waze Mobile Ltd
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
 *   @see WazeAppWidgetService.java
 */
package com.waze.widget;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.net.Uri;
import android.os.SystemClock;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.style.TextAppearanceSpan;
import android.view.View;
import android.widget.RemoteViews;

import com.waze.R;
import com.waze.widget.config.WazeLang;
import com.waze.widget.routing.RouteScoreType;

public class WazeAppWidgetProvider extends AppWidgetProvider
{
    /*************************************************************************************************
     *================================= Public interface section =================================
     */
    /*************************************************************************************************
     * Create - Creates sound recorder engine and initializes the native layer object
     * 				Should be call when library is loaded
     * @param void
     * @return void 
     */

    @Override
    public void onUpdate( Context aContext, AppWidgetManager aAppWidgetManager, int[] aAppWidgetIds ) 
    {
    	super.onUpdate( aContext, aAppWidgetManager, aAppWidgetIds );
    	
//    	WazeAppWidgetLog.d( "ON UPDATE" );
//    	
    	Intent updateIntent = new Intent( aContext, WazeAppWidgetService.class );
        updateIntent.setAction( WazeAppWidgetService.APPWIDGET_ACTION_CMD_UPDATE );
    	aContext.startService( updateIntent );    	
    }
    
    
    @Override
    public void onEnabled(Context aContext) {
    	super.onEnabled( aContext );
    	WazeAppWidgetLog.d( "ON ENABLE" );
    	Intent enableIntent = new Intent( aContext, WazeAppWidgetService.class );
    	enableIntent.setAction( WazeAppWidgetService.APPWIDGET_ACTION_CMD_ENABLE );
    	aContext.startService( enableIntent );
    	setAlarm( aContext, false );
    }

    @Override
    public void onDisabled(Context aContext) {
    	super.onDisabled( aContext );
    	WazeAppWidgetLog.d( "ON DISABLE" );
    	// Stop service
    	Intent updateIntent = new Intent( aContext, WazeAppWidgetService.class );
    	aContext.stopService( updateIntent );
    	// Stop alarm
    	setAlarm( aContext, true );
    }

    
    /*************************************************************************************************
     * setAlarm - Defines the alarm providing the scheduled periodic widget update requests 
     * @param aContext - application context
     * @param aAppWidgetId - widget id
     * 
     * @return void 
     */
    private static void setAlarm( Context aContext, boolean aCancel ) 
	{
    	// TODO:: Should be disabled when last widget is removed
        PendingIntent refreshTestIntent = getControlIntent( aContext, WazeAppWidgetService.APPWIDGET_ACTION_CMD_REFRESH_TEST );
        AlarmManager alarms = (AlarmManager) aContext.getSystemService( Context.ALARM_SERVICE );
        if ( aCancel )
        {
        	WazeAppWidgetLog.d( "Disable Alarm" );
        	alarms.cancel( refreshTestIntent );
        }
        else
        {
        	WazeAppWidgetLog.d( "Setting Alarm for " + WazeAppWidgetPreferences.AllowRefreshTimer()/1000 + " seconds" );
        	alarms.cancel( refreshTestIntent );
        	alarms.setRepeating( AlarmManager.ELAPSED_REALTIME, SystemClock.elapsedRealtime(), 
        			WazeAppWidgetPreferences.AllowRefreshTimer(), refreshTestIntent );
        }
    }
    
    /*************************************************************************************************
     * getControlgIntent - Returns pending intent defining the control action to be posted to the 
     * widget service 
     * @param aContext - application context
     * @param aCommand - string describing the command
     * @param aAppWidgetId - widget id
     * @return void 
     */
    public static PendingIntent getControlIntent( Context aContext, int aAppWidgetId, String aCommand ) 
	{
        Intent commandIntent = new Intent( aContext, WazeAppWidgetService.class );
        commandIntent.setAction( aCommand );
        commandIntent.putExtra( AppWidgetManager.EXTRA_APPWIDGET_ID, aAppWidgetId );
        // Force overriding by making unique field FLAG_UPDATE_CURRENT
        Uri data = Uri.withAppendedPath( Uri.parse("WazeAppWidget://widget/id/#" + aCommand + aAppWidgetId), 
        						String.valueOf( aAppWidgetId ) );
        commandIntent.setData(data);
        PendingIntent pendingIntent = PendingIntent.getService( aContext, 0, commandIntent, PendingIntent.FLAG_UPDATE_CURRENT );
        return pendingIntent;
    }
    
    public static Context _CONTEXT = null;

    /*************************************************************************************************
     * getControlgIntent - Returns pending intent defining the control action to be posted to the 
     * widget service - to be set for all widgets 
     * @param aContext - application context
     * @param aCommand - string describing the command
     * @return void 
     */
    public static PendingIntent getControlIntent( Context aContext, String aCommand ) 
	{
        Intent commandIntent = new Intent( aContext, WazeAppWidgetService.class );
        commandIntent.setAction( aCommand );
        PendingIntent pendingIntent = PendingIntent.getService( aContext, 0, commandIntent, PendingIntent.FLAG_UPDATE_CURRENT );
        return pendingIntent;
    }
    
    public static void setNeedRefreshState( Context aContext, String aDestDesc, int aTimeToDest, boolean OldData )
	{

		WazeAppWidgetLog.d("setNeedRefreshState isOld=" +OldData);
		/*
		 * Set widget UI
		 */
		RemoteViews views = new RemoteViews( aContext.getPackageName(), R.layout.app_widget );
		AppWidgetManager appWidgetManager = AppWidgetManager.getInstance( aContext );
		Resources res = aContext.getResources();
		
		// Background - normal
		// Not supported in the earlier OS versions
//		views.setInt( R.id.app_widget_root, "setBackgroundResource", R.drawable.widget_bg_main );
		
		views.setViewVisibility( R.id.image_status_bg, View.INVISIBLE );
		// Progress - invisible 
		views.setViewVisibility( R.id.widget_progress, View.INVISIBLE );
		// Icon - visible
		views.setViewVisibility( R.id.image_status, View.VISIBLE );
		
		// Icon - red ??
		if (OldData)
			views.setImageViewResource( R.id.image_status, R.drawable.widget_icon_no_data);
		
		// Refresh - visible
		//views.setViewVisibility(R.id.layout_refresh, View.VISIBLE );
		//views.setTextViewText(R.id.refresh_text, WazeLang.getLang("Click to refresh") );
		
		//ABS1 
		 views.setTextColor( R.id.text_view_action, res.getColor( R.color.solid_white ) );
		views.setTextViewText(R.id.text_view_action, WazeLang.getLang("Refresh") );
		views.setViewVisibility( R.id.text_view_action, View.VISIBLE );
		views.setImageViewResource( R.id.image_action, R.drawable.widget_bt_refresh_idle );

		// Text
		views.setTextViewText( R.id.text_view_destination, formatDestination( aContext, aDestDesc ) );
		views.setTextViewText( R.id.text_view_time, formatTime( aContext, aTimeToDest ) );
		// Callbacks
		setRootIntent( aContext, views, WazeAppWidgetService.APPWIDGET_ACTION_CMD_REFRESH );
		setActionIntent( aContext, views, WazeAppWidgetService.APPWIDGET_ACTION_CMD_REFRESH );
		if (OldData)
			setStatusIntent( aContext, views, WazeAppWidgetService.APPWIDGET_ACTION_CMD_REFRESH );
		else
			setStatusIntent( aContext, views, WazeAppWidgetService.APPWIDGET_ACTION_CMD_GRAPH );
		// Update all widgets
		appWidgetManager.updateAppWidget( new ComponentName( aContext, WazeAppWidgetProvider.class), views );
		
		setAlarm( aContext, true );
	}
    
    public static void setRefreshingState( Context aContext )
	{
		/*
		 * Set widget UI
		 */
    	WazeAppWidgetLog.d("setRefreshingState");
		RemoteViews views = new RemoteViews( aContext.getPackageName(), R.layout.app_widget );
		AppWidgetManager appWidgetManager = AppWidgetManager.getInstance( aContext );
		Resources res = aContext.getResources();
		
		// Background - normal
		// Not supported in the earlier OS versions
//		views.setInt( R.id.app_widget_root, "setBackgroundResource", R.drawable.widget_bg_main );
		views.setViewVisibility( R.id.image_status_bg, View.INVISIBLE );
		// Progress - visible 
		views.setViewVisibility( R.id.widget_progress, View.VISIBLE );
		// Icon - invisible
		views.setViewVisibility( R.id.image_status, View.VISIBLE );
		views.setImageViewResource( R.id.image_status, R.drawable.widget_icon_refresh_bg1 );
//		views.setImageViewResource( R.id.image_status, R.drawable.widget_icon_refresh );
		// Refresh - visible
		views.setViewVisibility(R.id.layout_refresh, View.INVISIBLE );
		// Action - enabled
		views.setTextColor( R.id.text_view_action, res.getColor( R.color.disabled_white ) );
		views.setImageViewResource( R.id.image_action, R.drawable.widget_bt_drive_disabled );
		views.setTextViewText(R.id.text_view_action, WazeLang.getLang("Drive")+"!" );
		views.setViewVisibility( R.id.text_view_action, View.INVISIBLE );
		// Text
		views.setTextViewText( R.id.text_view_destination, WazeLang.getLang("Please wait...") );
		views.setTextViewText( R.id.text_view_time, formatTime( aContext, 0 ) );
		// Update all widgets
		setAlarm( aContext, false );
		appWidgetManager.updateAppWidget( new ComponentName( aContext, WazeAppWidgetProvider.class), views );
	}
    
    public static void setUptodateState( Context aContext, String aDestDesc, int aTimeToDest, RouteScoreType score )
	{
		/*
		 * Set widget UI
		 */
		WazeAppWidgetLog.d( "setUptodateState "+ score.name() );
		
		RemoteViews views = new RemoteViews( aContext.getPackageName(), R.layout.app_widget );
		AppWidgetManager appWidgetManager = AppWidgetManager.getInstance( aContext );
		Resources res = aContext.getResources();
		
		// Background - normal
		// Not supported in the earlier OS versions
//		views.setInt( R.id.app_widget_root, "setBackgroundResource", R.drawable.widget_bg_main );
		views.setViewVisibility( R.id.image_status_bg, View.INVISIBLE );
		// Progress - invisible 
		views.setViewVisibility( R.id.widget_progress, View.INVISIBLE );
		// Icon - visible
		views.setViewVisibility( R.id.image_status, View.VISIBLE );
		switch (score){
		 	case ROUTE_BAD:
		 		// 	Icon - red ??
		 		views.setImageViewResource( R.id.image_status, R.drawable.widget_icon_red );
		 		break;
		 		
		 	case ROUTE_GOOD:
		 		views.setImageViewResource( R.id.image_status, R.drawable.widget_icon_green );
		 		break;
		 		
		 	case ROUTE_OK:
		 		views.setImageViewResource( R.id.image_status, R.drawable.widget_icon_orange );
		 		break;

		 	case NONE:
		 		views.setImageViewResource( R.id.image_status, R.drawable.widget_icon_no_data );
		 		break;
		}
		views.setTextViewText(R.id.text_view_action, WazeLang.getLang("Drive")+"!" );
		views.setViewVisibility( R.id.text_view_action, View.VISIBLE );
		
		// Refresh - visible
		views.setViewVisibility(R.id.layout_refresh, View.INVISIBLE );
		// Action - enabled
		views.setTextColor( R.id.text_view_action, res.getColor( R.color.solid_white ) );
		views.setImageViewResource( R.id.image_action, R.drawable.widget_bt_drive_idle );
		
		// Text
		views.setTextViewText( R.id.text_view_destination, formatDestination( aContext, aDestDesc ) );
		views.setTextViewText( R.id.text_view_time, formatTime( aContext, aTimeToDest ) );
		// Callbacks
		setRootIntent( aContext, views, WazeAppWidgetService.APPWIDGET_ACTION_CMD_NONE );
		setActionIntent( aContext, views, WazeAppWidgetService.APPWIDGET_ACTION_CMD_DRIVE );
		setStatusIntent( aContext, views, WazeAppWidgetService.APPWIDGET_ACTION_CMD_GRAPH );
		// Update all widgets
		
		setAlarm( aContext, false );
		appWidgetManager.updateAppWidget( new ComponentName( aContext, WazeAppWidgetProvider.class), views );
	}
    
    public static void setNoDataState( Context aContext, String aDestDesc )
	{
		/*
		 * Set widget UI
		 */
    	WazeAppWidgetLog.d("setNoDataState");
		RemoteViews views = new RemoteViews( aContext.getPackageName(), R.layout.app_widget );
		AppWidgetManager appWidgetManager = AppWidgetManager.getInstance( aContext );
		Resources res = aContext.getResources();
		
		// Background - normal
		// Not supported in the earlier OS versions
//		views.setInt( R.id.app_widget_root, "setBackgroundResource", R.drawable.widget_bg_main_no_data );
//		views.setViewVisibility( R.id.image_status_bg, View.VISIBLE );
		// Progress - invisible 
		views.setViewVisibility( R.id.widget_progress, View.INVISIBLE );
		// Icon - visible
		views.setViewVisibility( R.id.image_status, View.VISIBLE );
		// Icon - red ??
		views.setImageViewResource( R.id.image_status, R.drawable.widget_icon_no_data);
		// Refresh - visible
		views.setViewVisibility( R.id.layout_refresh, View.INVISIBLE );
		// Action - enabled
		//views.setTextColor( R.id.text_view_action, res.getColor( R.color.disabled_white ) );
		//views.setImageViewResource( R.id.image_action, R.drawable.widget_bt_drive_disabled );
		//views.setTextViewText(R.id.text_view_action, WazeLang.getLang("Drive")+"!" );

		 views.setTextColor( R.id.text_view_action, res.getColor( R.color.solid_white ) );
		views.setTextViewText(R.id.text_view_action, WazeLang.getLang("Refresh") );
		views.setViewVisibility( R.id.text_view_action, View.VISIBLE );
		views.setImageViewResource( R.id.image_action, R.drawable.widget_bt_refresh_idle );
		
		// Text
		views.setTextViewText( R.id.text_view_destination, "No Data" ); 
		views.setTextViewText( R.id.text_view_time, formatTime( aContext, 0 ) );
		
		// Refresh - visible
		//views.setViewVisibility(R.id.layout_refresh, View.VISIBLE );
		//views.setTextViewText(R.id.refresh_text, WazeLang.getLang("Click to refresh") );
		
		// Callback
		setRootIntent( aContext, views, WazeAppWidgetService.APPWIDGET_ACTION_CMD_REFRESH_INFO );
		setStatusIntent( aContext, views, WazeAppWidgetService.APPWIDGET_ACTION_CMD_REFRESH_INFO );
		//ABS1
		setActionIntent( aContext, views, WazeAppWidgetService.APPWIDGET_ACTION_CMD_REFRESH_INFO );
		setAlarm( aContext, false );
		// Update all widgets
		appWidgetManager.updateAppWidget( new ComponentName( aContext, WazeAppWidgetProvider.class), views );
	}
    
    private static CharSequence formatDestination( Context aContext, String aDestDesc )
    {
    	String destFmtString = "@ " + aDestDesc + " " + WazeLang.getLang("in:");
    	return destFmtString;
    }
    private static CharSequence formatTime( Context aContext, int aTimeToDest )
    {
    	int hours = aTimeToDest / 60;
    	int mins = aTimeToDest - hours * 60;
    	String hoursString = "", minsString;
    	int spanStart = 0, spanEnd = 0;
    	String hoursValue = null;
    	String hoursUnit = "h.";
    	String hoursSep = " ";
		String sepString = "   ";
		TextAppearanceSpan spanValues, spanUnits;
		SpannableStringBuilder spanBuilder = new SpannableStringBuilder();
		
    	/*
    	 * Hours
    	 */
    	if ( ( hours != 0 ) || ( aTimeToDest == 0 ) )
    	{
    		hoursValue = String.valueOf( hours );
    		hoursString = hoursValue + hoursSep + hoursUnit + sepString;    		
    	}
    	/*
    	 * Minutes
    	 */
    	String minsValue = String.valueOf( mins );
    	if ( aTimeToDest == 0 )
    		minsValue += "0";
    		
		String minsUnit = "min.";
		String minsSep = " ";
		minsString = minsValue + minsSep + minsUnit;
		
		// Add to builder
		String strFinal = hoursString + minsString;
		spanBuilder.append( strFinal );
		
		/*
		 *  Set spans
		 */
		if ( hoursString.length() > 0 )
		{
			spanValues = new TextAppearanceSpan( aContext, R.style.TextAppearance_TimeValue );
	    	spanUnits = new TextAppearanceSpan( aContext, R.style.TextAppearance_TimeUnit );
	    	// Hours value
			spanStart = 0;
			spanEnd = hoursValue.length() - 1;
			spanBuilder.setSpan( spanValues, spanStart, spanEnd, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE );
			// Hours units + separators
			spanStart = spanEnd + 1;
			spanEnd = hoursString.length();
			spanBuilder.setSpan( spanUnits, spanStart, spanEnd, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE );			
		}

		spanValues = new TextAppearanceSpan( aContext, R.style.TextAppearance_TimeValue );
    	spanUnits = new TextAppearanceSpan( aContext, R.style.TextAppearance_TimeUnit );
    	spanStart = hoursString.length();
		// Minutes values + separator format
		spanEnd = spanStart + minsValue.length();   
		spanBuilder.setSpan( spanValues, spanStart, spanEnd, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE );
		// Minutes units format
		spanStart = spanEnd;
		spanEnd = spanStart + minsSep.length() + minsUnit.length();	// ??? Check why length-1 is not working 
		spanBuilder.setSpan( spanUnits, spanStart, spanEnd, Spannable.SPAN_EXCLUSIVE_EXCLUSIVE );

		return spanBuilder;
    	
    }

	static void updateCallbacks( Context aContext ) 
	{
		RemoteViews views = new RemoteViews( aContext.getPackageName(), R.layout.app_widget );
		
		AppWidgetManager appWidgetManager = AppWidgetManager.getInstance( aContext );
		
		setRootIntent( aContext, views, WazeAppWidgetService.APPWIDGET_ACTION_CMD_REFRESH );
		
		setActionIntent( aContext, views, WazeAppWidgetService.APPWIDGET_ACTION_CMD_DRIVE );

		// Update all widgets
		appWidgetManager.updateAppWidget( new ComponentName( aContext, WazeAppWidgetProvider.class ), views );
	}

 
	
    /*************************************************************************************************
     *================================= Native methods section ================================= 
     * These methods are implemented in the native side and should be called after!!! the shared library is loaded
     */
	
	/*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */
	
    private static void setRootIntent( Context aContext, RemoteViews aViews, String aAction )
    {
    	PendingIntent intentCmd = WazeAppWidgetProvider.getControlIntent( aContext, aAction );
    	aViews.setOnClickPendingIntent( R.id.app_widget_root, intentCmd );
    }
    
    private static void setActionIntent( Context aContext, RemoteViews aViews, String aAction )
    {
    	PendingIntent intentCmd = WazeAppWidgetProvider.getControlIntent( aContext, aAction );
    	aViews.setOnClickPendingIntent( R.id.layout_action, intentCmd );
    }

    private static void setStatusIntent( Context aContext, RemoteViews aViews, String aAction )
    {
    	PendingIntent intentCmd = WazeAppWidgetProvider.getControlIntent( aContext, aAction );
    	aViews.setOnClickPendingIntent( R.id.layout_status_image, intentCmd );
    }
	
	/*************************************************************************************************
     *================================= Members section =================================
     */

    /*************************************************************************************************
     *================================= Constants section =================================
     */
		
}

