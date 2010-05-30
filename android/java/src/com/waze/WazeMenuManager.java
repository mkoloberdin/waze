/**  
 * WazeMenuBuilder.java
 *   
 * 
 * 
 * LICENSE:
 *
 *   Copyright 2009     @author Alex Agranovich
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

import java.lang.reflect.Field;

import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;

public final class WazeMenuManager
{

    /*************************************************************************************************
     *================================= Public interface section =================================
     */
	
	public WazeMenuManager( FreeMapNativeManager aMgr )
	{
		mNativeManager = aMgr;
		mMenuItems = new WazeMenuItem[WAZE_OPT_MENU_ITEMS_MAXNUM];
		mMenuItemCount = 0;
		
		InitMenuManagerNTV();		
	}
	
    /*************************************************************************************************
     * Builds the options menu layout
     */
	public void BuildOptionsMenu( Menu aMenu, boolean aIsPortrait )
	{
		MenuItem item = null;		
		int order = 0;
		aMenu.clear();
		
		try 
		{
			if ( ( mMenuItemCount > 0 ) /* && !mIsInitialized */ )
			{
				int i;
				for ( i = 0; i < mMenuItemCount; ++i )
				{
					order = aIsPortrait ? mMenuItems[i].portrait_order : mMenuItems[i].landscape_order;
					item = aMenu.add( 0, mMenuItems[i].item_id, order, mMenuItems[i].item_label );
					if ( mMenuItems[i].icon_name != null )					
					{
						item.setIcon( mMenuItems[i].icon_id );
					}
				}
				mIsInitialized = true;	
			}	
			
		}
		catch( Exception ex )
		{
			Log.w( "WAZE", "Error while building the menu" + ex.getMessage() );
			ex.printStackTrace();
		}
	}
	
    /*************************************************************************************************
     * Handles the button pressed event
     */
	public void OnMenuButtonPressed( int aButtonId )
	{
		int msg = NATIVE_MSG_CATEGORY_MENU | aButtonId;
		FreeMapAppService.getNativeManager().PostNativeMessage( msg );
	}

    /*************************************************************************************************
     * Adds a new item to menu 
     */
	public void AddOptionsMenuItem( int aItemId, byte[] aLabel, byte[] aIcon, int is_icon_native, int portrait_order, int landscape_order )
	{
		WazeMenuItem item = new WazeMenuItem();
		
		item.item_id = aItemId;
		item.item_label = new String ( aLabel );
		item.portrait_order = portrait_order;
		item.landscape_order = landscape_order;
		item.is_native = is_icon_native;
		
		try 
		{
			if ( aIcon != null )
			{
				Field icon_field;
				item.icon_name = new String( aIcon );
				if ( item.is_native == 1 )
				{
					icon_field = android.R.drawable.class.getField( item.icon_name );
				}
				else
				{
					icon_field = R.drawable.class.getField( item.icon_name );
//					Drawable icon = BitmapDrawable.createFromPath( FreeMapResources.GetSkinsPath() + item.icon_name );
//					item.icon = icon;
				}
				int icon_id = icon_field.getInt( null );
				item.icon_id = icon_id;
			}
			else
			{
				item.icon_name = null;
			}
		}
		catch( Exception ex )
		{
			Log.w( "WAZE", "Error while building the menu" + ex.getMessage() );
			ex.printStackTrace();
		}
	    mMenuItems[mMenuItemCount] = item;
		
		mMenuItemCount++;
	}

	
	/*************************************************************************************************
     *================================= Private interface section =================================
     * 
     */
    private static class WazeMenuItem
    {
    	public int item_id;
    	public String item_label;
    	public String icon_name;
//    	Drawable icon;		// For custom
    	int icon_id;		// For native 
    	public int is_native;    	
    	public int portrait_order;
    	public int landscape_order;
    }
	
	
	
    /*************************************************************************************************
     *================================= Native methods section ================================= 
     * These methods are implemented in the
     * native side and should be called after!!! the shared library is loaded
     */
    private native void InitMenuManagerNTV();

    
    /*************************************************************************************************
     *================================= Data members section =================================
     * 
     */
    
    private static final int WAZE_OPT_MENU_ITEMS_MAXNUM = 10;
    
    private WazeMenuItem mMenuItems[];
	private int mMenuItemCount;
	
	private boolean mIsInitialized = false;
	
	public static final int NATIVE_MSG_CATEGORY_MENU = 0x080000;
	
	private FreeMapNativeManager mNativeManager;

}
