package com.waze;

import com.google.android.apps.analytics.GoogleAnalyticsTracker;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class WazeInstallReferrerTracker extends BroadcastReceiver {

	@Override
	public void onReceive( Context context, Intent intent ) {
		
		Log.d( WazeLog.TAG, "Install referrer tracker. Intent: " + intent.getAction() + 
				", " + intent.getDataString() + ", " + intent.getStringExtra( "referrer" ) );
		
		GoogleAnalyticsTracker tracker = GoogleAnalyticsTracker.getInstance();
       // Start the tracker in manual dispatch mode...
        tracker.startNewSession( "UA-24084788-1", context );
        tracker.trackPageView( intent.getStringExtra( "referrer" ) );
        
        tracker.dispatch();
        tracker.stopSession();        
	}
}
