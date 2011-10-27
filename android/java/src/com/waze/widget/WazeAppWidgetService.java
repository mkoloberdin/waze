/**  

 * WazeAppWidgetService - Service management for the widget
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
 *   @see WazeAppWidgetProvider.java
 */

package com.waze.widget;

import java.io.File;
import java.util.Date;
import java.util.Timer;
import java.util.TimerTask;

import org.json.JSONException;

import com.waze.FreeMapAppActivity;
import com.waze.widget.StatusData;
import com.waze.widget.routing.RouteScore;
import com.waze.widget.routing.RouteScoreType;
import com.waze.widget.routing.RoutingResponse;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.IBinder;
import android.os.SystemClock;
import android.widget.Toast;

public class WazeAppWidgetService extends Service {

	/*************************************************************************************************
	 * ================================= Public interface section
	 * =================================
	 */

	@Override
	public void onCreate() {
		WazeAppWidgetLog.d("Widget service instance is created: " + this);
		super.onCreate();
//		mThreadId = Thread.currentThread().getId();
		mInstance = this;
		loadState();
	}

	/*************************************************************************************************
	 * onStart - Starts the service instance. Handles the intents commands
	 * 
	 * @param void
	 * @return void
	 */
	@Override
	public void onStart(Intent aIntent, int aStartId) {
		super.onStart(aIntent, aStartId);
		
		if (aIntent == null){
			return;
		}
		WazeAppWidgetLog.d("Widget service instance is started. Intent: "
				+ aIntent.getAction());
		String command = aIntent.getAction();
		/*
		 * This context switch ensures the context is updated with the new
		 * widget instance
		 */
		android.os.SystemClock.sleep(100L);

		WidgetManager.init(this);

		/*
		 * Handle commands
		 */
		if (command.equals(APPWIDGET_ACTION_CMD_ENABLE)) {
			enableCmdHandler();
		}
		if (command.equals(APPWIDGET_ACTION_CMD_UPDATE)) {
			updateCmdHandler();
		}
		if (command.equals(APPWIDGET_ACTION_CMD_REFRESH)) {
			refreshCmdHandler(false);
		}
		if (command.equals(APPWIDGET_ACTION_CMD_REFRESH_INFO)) {
			WazeAppWidgetLog.d("APPWIDGET_ACTION_CMD_REFRESH_INFO command");
			refreshCmdHandler(true);
		}

		if (command.equals(APPWIDGET_ACTION_CMD_NODATA)) {
			if (WidgetManager.hasHomeWork()) {
				refreshCmdHandler(true);
			} else {
				noDataCmdHandler();
			}
		}
		if (command.equals(APPWIDGET_ACTION_CMD_REFRESH_TEST)) {
			refreshTestCmdHandler();
		}
		if (command.equals(APPWIDGET_ACTION_CMD_DRIVE)) {
			driveCmdHandler();
		}
		if (command.equals(APPWIDGET_ACTION_CMD_GRAPH)) {
			graphCmdHandler();
		}
		if (command.equals(APPWIDGET_ACTION_CMD_NONE)) {

		}
	}

	@Override
	public void onDestroy() {
		WazeAppWidgetLog.d("Widget service instance is destroyed: " + this);
		super.onDestroy();
	}

	@Override
	public IBinder onBind(Intent arg0) {
		// TODO Auto-generated method stub
		return null;
	}

	/*************************************************************************************************
	 * setState - Public interface for state testing trigger.
	 * 
	 * @param aStatusCode
	 *            - status code ( Main input for the state transition)
	 * @param aStatusData
	 *            - status data ( Secondary input for the state transition.
	 *            State information )
	 * @return void
	 */
	public static void setState(final int aStatusCode,
			final StatusData aStatusData) {
//		if (Thread.currentThread().getId() == mThreadId) {
			stateHandler(aStatusCode, aStatusData);
//		} else {
//			final Runnable stateEvent = new Runnable() {
//				public void run() {
//					stateHandler(aStatusCode, aStatusData);
//				}
//			};
//			mHandler.post(stateEvent);
//		}
	}

	public static void alertUser(String str){
		Toast toast = Toast.makeText(mInstance.getApplicationContext(), str, Toast.LENGTH_LONG);
		toast.show();


	}
	/*************************************************************************************************
	 * destroy - Posts a request for the service destruction. Saves the state
	 * 
	 * @param none
	 * @return void
	 */
	public static void destroy() {
		if (mInstance != null) {
			mInstance.saveState();
			Context context = mInstance.getApplicationContext();
			Intent destroyIntent = new Intent(context,
					WazeAppWidgetService.class);
			context.stopService(destroyIntent);
		}
	}

	/*************************************************************************************************
	 * requestRefresh - Posts a request for widget refreshing.
	 * 
	 * @param none
	 * @return void
	 */
	public static void requestRefresh(Context aContext) {
		Intent requestRefreshIntent = new Intent(aContext,
				WazeAppWidgetService.class);
		requestRefreshIntent.setAction(APPWIDGET_ACTION_CMD_REFRESH);
		aContext.startService(requestRefreshIntent);
	}

	/*************************************************************************************************
	 * startRefresh - invokes the refresh process
	 * 
	 * @param none
	 * @return void
	 */
	private static void startRefresh() {

		if (mRefreshMonitor != null) {
			WazeAppWidgetLog.d("startRefresh - Refresh timer is active - cancelling");
			mRefreshMonitor.cancel();
		}
		mRefreshMonitor = new Timer();
		WazeAppWidgetLog.d("startRefresh timer");
		final class Task extends TimerTask {
			@Override
			public void run() {
				if (mInstance != null)
					mInstance.onRefreshingTimeout();
				mRefreshMonitor.cancel();
				mRefreshMonitor = null;
			}
		}
		mRefreshMonitor.schedule(new Task(), REFRESHING_TIMEOUT);

		WazeAppWidgetProvider.setRefreshingState(mInstance
				.getApplicationContext());
		WazeAppWidgetManager.refreshHandler(mInstance.getApplicationContext());
	}

	/*************************************************************************************************
	 * stopRefreshMonitor - cancels the refresh monitor
	 * 
	 * @param none
	 * @return void
	 */
	public static void stopRefreshMonitor() {
		if (mRefreshMonitor != null) {
			WazeAppWidgetLog.d("stopRefreshMonitor - Refresh timer is active - cancelling");
			mRefreshMonitor.cancel();
			mRefreshMonitor = null;
		}
	}

	private void onRefreshingTimeout() {
		WazeAppWidgetLog.d("Refresh timeout. Reset state.");
		setState(StatusData.STATUS_ERR_REFRESH_TIMEOUT, null);
//		final Runnable shutDownEvent = new Runnable() {
//			public void run() {
//				WazeAppWidgetManager.setRequestInProcess(false);
//				WazeAppWidgetManager.shutDownApp();
//			}
//		};
//		mHandler.post(shutDownEvent);
	}

	/*************************************************************************************************
	 * ================================= Native methods section
	 * ================================= These methods are implemented in the
	 * native side and should be called after!!! the shared library is loaded
	 */

	/*************************************************************************************************
	 * ================================= Private interface section
	 * =================================
	 * 
	 */
	/*************************************************************************************************
	 * enableCmdHandler - Called when the first widget is added. Resets the
	 * state.
	 * 
	 * @param none
	 * @return void
	 */
	private void enableCmdHandler() {
		WazeAppWidgetLog.d("enable command handler");
		// Reset and save initial state
		
		// Check sdcard availability every 30 sec
		final File file = new File("/sdcard/");
		if (file.exists() && file.canWrite()) {
			WazeAppWidgetLog
					.d("SD Card is available. Setting state to the STATUS_NEW_WIDGET.");
			setState(StatusData.STATUS_NEW_WIDGET, null);
		} else {
			if (mSDCardChecker == null) {
				mSDCardChecker = new Timer();
				final TimerTask task = new TimerTask() {
					@Override
					public void run() {
						if (file.exists() && file.canWrite()) {
							WazeAppWidgetLog
									.d("SD Card is available. Setting state to the STATUS_NEW_WIDGET. Cancelling the timer.");
							setState(StatusData.STATUS_NEW_WIDGET, null);
							mSDCardChecker.cancel();
							WidgetManager.loadWazeConfig();
							mWidgetState = STATE_NONE;
							mStatusData.copy(new StatusData());
							mSDCardChecker = null;
						} else {
							WazeAppWidgetLog
									.w("SD Card is not available. Scheduling next check in 30 seconds");
							mSDCardChecker.schedule(this, 30000L);
						}
					}
				};
				mSDCardChecker.schedule(task, 30000L);
			}
		}

	}

	/*************************************************************************************************
	 * updateCmdHandler - Handles the new widget addition
	 * 
	 * @param none
	 * @return void
	 */
	private void updateCmdHandler() {
		WazeAppWidgetProvider.setNoDataState(getApplicationContext(),
				INITIAL_DESTINATION);
		WazeAppWidgetLog.d("Update command handler");

		// Check sdcard availability every 30 sec
		final File file = new File("/sdcard/");
		if (file.exists() && file.canWrite()) {
			WazeAppWidgetLog
					.d("SD Card is available. Setting state to the STATUS_NEW_WIDGET.");
			setState(StatusData.STATUS_NEW_WIDGET, null);
		} else {
			if (mSDCardChecker == null) {
				mSDCardChecker = new Timer();
				final TimerTask task = new TimerTask() {
					@Override
					public void run() {
						if (file.exists() && file.canWrite()) {
							WazeAppWidgetLog
									.d("SD Card is available. Setting state to the STATUS_NEW_WIDGET. Cancelling the timer.");
							setState(StatusData.STATUS_NEW_WIDGET, null);
							mSDCardChecker.cancel();
							WidgetManager.loadWazeConfig();
							mSDCardChecker = null;
						} else {
							WazeAppWidgetLog
									.w("SD Card is not available. Scheduling next check in 30 seconds");
							mSDCardChecker.schedule(this, 30000L);
						}
					}
				};
				mSDCardChecker.schedule(task, 30000L);
			}
		}

	}

	/*************************************************************************************************
	 * refreshCmdHandler - Handles the new refresh request command
	 * 
	 * @param none
	 * @return void
	 */
	private void refreshCmdHandler(boolean aShowNoDataInfo) {
		WazeAppWidgetLog.d("Refresh command handler");

		if (aShowNoDataInfo)
			setState(StatusData.STATUS_REFRESH_REQUEST_INFO, null);
		else
			setState(StatusData.STATUS_REFRESH_REQUEST, null);
	}

	/*************************************************************************************************
	 * refreshTestCmdHandler - Handles the new refresh test positive result.
	 * Called upon every test period expiration.
	 * 
	 * @param none
	 * @return void
	 */
	private void refreshTestCmdHandler() {
		WazeAppWidgetLog.d("RefreshTest command handler");

		if (isDataExpired()) {
			setState(StatusData.STATUS_REFRESH_TEST_TRUE, null);
		}
	}

	/*************************************************************************************************
	 * refreshTestCmdHandler - Handles the new refresh test positive result.
	 * Called upon every test period expiration.
	 * 
	 * @param none
	 * @return void
	 */
	private void graphCmdHandler() {
		WazeAppWidgetLog.d("Graph command handler");

		if (mWidgetState == STATE_CURRENT_DATA_UPTODATE
				|| mWidgetState == STATE_CURRENT_DATA_NEED_REFRESH) {
			Intent intent = new WazeAppWidgetChart().execute(this,
					mStatusData.getRoutingRespnse(), mStatusData.timeStamp());
			// Intent intent = new Intent( getApplicationContext(),
			// WazeAppWidgetGraphActivity.class );
			intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK
					| Intent.FLAG_ACTIVITY_MULTIPLE_TASK);
			getApplicationContext().startActivity(intent);
		} else {
			WazeAppWidgetLog.d("Graph command handler called but state is ="
					+ mWidgetState);
		}
	}

	/*************************************************************************************************
	 * noDataCmdHandler - Handles the no data command. Opens the appropriate
	 * activity
	 * 
	 * @param none
	 * @return void
	 */
	private void noDataCmdHandler() {
		WazeAppWidgetLog.d("NoData command handler");

		Intent intent = new Intent(getApplicationContext(),
				WazeAppWidgetNoDataActivity.class);
		intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK
				| Intent.FLAG_ACTIVITY_MULTIPLE_TASK);
		getApplicationContext().startActivity(intent);
	}

	/*************************************************************************************************
	 * driveCmdHandler - Handles the drive command. Opens waze in the navigation
	 * mode
	 * 
	 * @param none
	 * @return void
	 */
	private void driveCmdHandler() {
		WazeAppWidgetLog.d("Drive command handler");

		if (mWidgetState == STATE_CURRENT_DATA_UPTODATE
				|| mWidgetState == STATE_CURRENT_DATA_NEED_REFRESH) {
			
			WazeAppWidgetLog.d("Starting waze waze://?favorite=" + mStatusData.destination());
			Intent intent = new Intent(getApplicationContext(),
					FreeMapAppActivity.class);
			Uri uri = Uri
					.parse("waze://?favorite=" + mStatusData.destination());
			intent.setAction(Intent.ACTION_VIEW);
			intent.setData(uri);
			intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK
					| Intent.FLAG_ACTIVITY_MULTIPLE_TASK);
			WazeAppWidgetLog.d("driveCmdHandler - starting waze");
			getApplicationContext().startActivity(intent);
		}
		else{
			WazeAppWidgetLog.d("driveCmdHandler invalid state" + mWidgetState);
		}
	}

	/*************************************************************************************************
	 * stateHandler - Main state machine logic
	 * 
	 * @param aStatusCode
	 *            - status code. Main input to change the state
	 * @param aStatusData
	 *            - status data coming together with the code. Represents the
	 *            state information
	 * @return void
	 */
	private static void stateHandler(final int aStatusCode,
			final StatusData aStatusData) {
		if (aStatusData != null)
			mStatusData.copy(aStatusData);

		switch (mWidgetState) {
		case STATE_NONE: {
			printDbg("STATE_NONE: ", aStatusCode);

			// New widget - Initiate refresh request
			if (aStatusCode == StatusData.STATUS_NEW_WIDGET) {
				mWidgetState = STATE_REFRESHING;
				startRefresh();
				break;
			}
				mWidgetState = STATE_REFRESHING;
				startRefresh();
			
//			WazeAppWidgetLog.e("Illegal status: " + aStatusCode
//					+ " for the state: STATE_NONE");
			break;
		}
		case STATE_NO_DATA: {
			printDbg("STATE_NO_DATA: ", aStatusCode);
			if (aStatusCode == StatusData.STATUS_REFRESH_REQUEST) {
				mWidgetState = STATE_REFRESHING;
				startRefresh();
			}
			if (aStatusCode == StatusData.STATUS_REFRESH_REQUEST_INFO) {
				mWidgetState = STATE_REFRESHING_INFO;
				startRefresh();
			}

			break;
		}
		case STATE_CURRENT_DATA_UPTODATE: {
			printDbg("STATE_CURRENT_DATA_UPTODATE: ", aStatusCode);
			if (aStatusCode == StatusData.STATUS_NEW_WIDGET) {
				WazeAppWidgetProvider.setUptodateState(
						mInstance.getApplicationContext(),
						mStatusData.destination(), mStatusData.timeToDest(),
						mStatusData.score());
			}
			if (aStatusCode == StatusData.STATUS_REFRESH_TEST_TRUE) {
				mWidgetState = STATE_CURRENT_DATA_NEED_REFRESH;
				boolean isOldData = ((new Date().getTime()) - mStatusData.timeStamp()) > 1000*60*120;
				WazeAppWidgetProvider.setNeedRefreshState(
						mInstance.getApplicationContext(),
						mStatusData.destination(), mStatusData.timeToDest(), isOldData);
			}

			break;
		}
		case STATE_CURRENT_DATA_NEED_REFRESH: {
			printDbg("STATE_CURRENT_DATA_NEED_REFRESH: ", aStatusCode);
			if ((aStatusCode == StatusData.STATUS_REFRESH_REQUEST)
					|| (aStatusCode == StatusData.STATUS_REFRESH_REQUEST_INFO)) {
				mWidgetState = STATE_REFRESHING;
				startRefresh();
			}
			break;
		}
		case STATE_REFRESHING:
		case STATE_REFRESHING_INFO: {
			printDbg("STATE_REFRESHING: ", aStatusCode);
			if (aStatusCode == StatusData.STATUS_SUCCESS) {
				mWidgetState = STATE_CURRENT_DATA_UPTODATE;
				WazeAppWidgetProvider.setUptodateState(
						mInstance.getApplicationContext(),
						mStatusData.destination(), mStatusData.timeToDest(),
						mStatusData.score());
				if (mInstance != null)
					mInstance.saveState();

				break;
			}
			if ((aStatusCode == StatusData.STATUS_ERR_REFRESH_TIMEOUT) || (aStatusCode == StatusData.STATUS_ERR_ROUTE_SERVER) || (aStatusCode == StatusData.STATUS_ERR_NO_LOCATION)){
				mWidgetState = STATE_NO_DATA;
				WazeAppWidgetProvider.setNoDataState(
						mInstance.getApplicationContext(), "No Data");
				break;
			}
			
			
			// No destination or no login - no data state. Force user to enter
			// the application
			if (testStatus(aStatusCode,
					StatusData.STATUS_ERR_NO_DESTINATION)
					|| testStatus(aStatusCode,
							StatusData.STATUS_ERR_NO_LOGIN)) {
				if (mWidgetState == STATE_REFRESHING_INFO)
					startNoDataActivity();
				mWidgetState = STATE_NO_DATA;
				WazeAppWidgetProvider.setNoDataState(
						mInstance.getApplicationContext(),
						mStatusData.destination());
				break;
			}
			if (aStatusCode == StatusData.STATUS_REFRESH_REQUEST_INFO) {
				mWidgetState = STATE_REFRESHING_INFO;
				startRefresh();
			}

			// TODO:: Check this
			WazeAppWidgetLog.e("Illegal status for STATE_REFRESHING: "
					+ aStatusCode);
			break;
		}
		default: {
			break;
		}
		}
	}

	/*************************************************************************************************
	 * saveState - Saves state for the case if the service is destroyed
	 * 
	 * @param none
	 * @return void
	 */
	private void saveState() {
		WazeAppWidgetLog.d("saveState ");
		SharedPreferences prefs = getApplicationContext().getSharedPreferences(
				PREFS_DB, 0);
		SharedPreferences.Editor editor = prefs.edit();
		editor.putInt("State", mWidgetState);
		editor.putString("Destination", mStatusData.destination());
		editor.putInt("TimeToDestination", mStatusData.timeToDest());
		editor.putLong("TimeStamp", mStatusData.timeStamp());

		if (mStatusData.getRoutingRespnse() != null)
			editor.putString("TimesArray", mStatusData.getRoutingRespnse()
					.toString());
		WazeAppWidgetLog.d("Saving last Routing Reposne: "
				+ mStatusData.getRoutingRespnse().toString());
		WazeAppWidgetLog.d("Last saved state: " + mWidgetState +  " timestamp= " + mStatusData.timeStamp());

		editor.commit();
	}

	/*************************************************************************************************
	 * loadState - Restoring state saved by saveState
	 * 
	 * @param none
	 * @return void
	 */
	private void loadState() {
		WazeAppWidgetLog.d("loadState ");
		SharedPreferences prefs = getApplicationContext().getSharedPreferences(
				PREFS_DB, 0);
		mWidgetState = prefs.getInt("State", mWidgetState);
		String destination = prefs.getString("Destination",
				mStatusData.destination());
		int timeToDest = prefs.getInt("TimeToDestination",
				mStatusData.timeToDest());
		long timeStamp = prefs.getLong("TimeStamp", mStatusData.timeStamp());
		String routingResultStr = prefs.getString("TimesArray", "");
		if (routingResultStr != null && routingResultStr.length() > 0) {
			try{
				RoutingResponse rrsp = new RoutingResponse(routingResultStr);
				RouteScoreType score = RouteScore.getScore(timeToDest,
					rrsp.getAveragetTime());
				mStatusData.copy(new StatusData(destination, timeToDest, score,
					rrsp));
				mStatusData.setTimeStamp(timeStamp);
				WazeAppWidgetLog.d("Last loaded Routing Reposne: "
						+ rrsp.toString() + " timestamp= "+timeStamp);
			} catch (JSONException e) {

			}

		} else {
			mStatusData
					.copy(new StatusData(destination, timeToDest, timeStamp));
		}
		
		if (mWidgetState == STATE_CURRENT_DATA_UPTODATE){
			if (!isDataExpired()){
				WazeAppWidgetLog.d("loaded state: data is not expired");
			}
			else{
				WazeAppWidgetLog.d("loaded state: data is expired");
				setState(StatusData.STATUS_REFRESH_TEST_TRUE, null);
			}
		}
		else{
			WazeAppWidgetLog.d("Last loaded state was: " + mWidgetState);
			mWidgetState = STATE_CURRENT_DATA_UPTODATE;
			setState(StatusData.STATUS_REFRESH_TEST_TRUE, null);
		}
		
		WazeAppWidgetLog.d("Last loaded state: " + mWidgetState);
	}

	/*************************************************************************************************
	 * startNoDataActivity - Starts the NO DATA activity using the alarm if
	 * service is died
	 * 
	 * @param none
	 * @return void
	 */
	private static void startNoDataActivity() {
		WazeAppWidgetLog.d("Request to show NO DATA Activity");

		Context ctx = mInstance.getApplicationContext();

		Intent intent = new Intent(ctx, WazeAppWidgetNoDataActivity.class);
		intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK
				| Intent.FLAG_ACTIVITY_MULTIPLE_TASK);
		PendingIntent pendingIntent = PendingIntent.getActivity(ctx, 0, intent,
				PendingIntent.FLAG_ONE_SHOT);
		AlarmManager alarms = (AlarmManager) ctx
				.getSystemService(Context.ALARM_SERVICE);
		alarms.set(AlarmManager.ELAPSED_REALTIME,
				SystemClock.elapsedRealtime() + 300L, pendingIntent);
	}

	/*************************************************************************************************
	 * 
	 */
	private static boolean isDataExpired() {
		return (System.currentTimeMillis() - mStatusData.timeStamp()) > WazeAppWidgetPreferences
				.AllowRefreshTimer();
	}

	/*************************************************************************************************
	 * 
	 */
	private static void printDbg(String aStr, int aStatus) {
		WazeAppWidgetLog.d("DEBUG PRINT. " + aStr + "(" + mWidgetState + "). "
				+ "Status: " + Integer.toString(aStatus, 16)
				+ ". Current status data: ( " + mStatusData.destination()
				+ " , " + mStatusData.timeToDest() + ", "
				+ mStatusData.score().name() + ")");
	}

	/*************************************************************************************************
	 * 
	 */
	private static boolean testStatus(int aLhs, int aRhs) {
		return ((aLhs & aRhs) > 0);
	}

	/*************************************************************************************************
	 * ================================= Constants section
	 * =================================
	 */
	public final static String APPWIDGET_ACTION_CMD_NONE 	= "AppWidget Action Command None";
	public final static String APPWIDGET_ACTION_CMD_GRAPH 	= "AppWidget Action Command Graph";
	public final static String APPWIDGET_ACTION_CMD_NODATA 	= "AppWidget Action Command No Data";
	public final static String APPWIDGET_ACTION_CMD_ENABLE 	= "AppWidget Action Command Enable";
	public final static String APPWIDGET_ACTION_CMD_UPDATE 	= "AppWidget Action Command Update";
	public final static String APPWIDGET_ACTION_CMD_REFRESH = "AppWidget Action Command Refresh";
	public final static String APPWIDGET_ACTION_CMD_REFRESH_INFO = "AppWidget Action Command Refresh Info";
	public final static String APPWIDGET_ACTION_CMD_REFRESH_TEST = "AppWidget Action Command Refresh Test";
	public final static String APPWIDGET_ACTION_CMD_DRIVE 	= "AppWidget Action Command Drive";
	public final static String APPWIDGET_ACTION_UPDATE		= "AppWidget Action Update";

	public final static int STATE_NONE = 0;
	public final static int STATE_NO_DATA = 1;
	public final static int STATE_CURRENT_DATA_UPTODATE = 2;
	public final static int STATE_CURRENT_DATA_NEED_REFRESH = 3;
	public final static int STATE_REFRESHING = 4;
	public final static int STATE_REFRESHING_INFO = 5;

	private final static String INITIAL_DESTINATION = "Home";

	private final static String PREFS_DB = "WAZE WIDGET PREFS";

	private final static long REFRESHING_TIMEOUT = 30 * 1000; 

	/*************************************************************************************************
	 * ================================= Members section
	 * =================================
	 */

//	private static final Handler mHandler = new Handler();
	private static WazeAppWidgetService mInstance = null;
	// Current status data
	private static final StatusData mStatusData = new StatusData();

	private static Timer mSDCardChecker = null;
	private static Timer mRefreshMonitor = null;

//	private static long mThreadId;

	private static volatile int mWidgetState = STATE_NONE;
	
}
