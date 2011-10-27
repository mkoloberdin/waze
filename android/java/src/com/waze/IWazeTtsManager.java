/**  
 * IWazeTtsManager - Tts manager interface
 *   
 * 
 * 
 * LICENSE:
 *
 *   Copyright 2011      Waze Ltd
 *   
 *   @author Alex Agranovich (AGA)
 *   
 * 
 */

package com.waze;

import android.content.Intent;

public interface IWazeTtsManager 
{
	public void InitNativeLayer();
	public void Submit( final String aText, final String aFullPath, final long aCbContext );
	public void Prepare();
	public void onDataCheckResult( int resultCode, Intent data );
	public void onDataInstallResult( int resultCode, Intent data );
	public void onInit( int status );
}

