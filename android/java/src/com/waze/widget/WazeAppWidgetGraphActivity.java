/**  
 * WazeAppWidgetGraphActivity - Activity displaying the graph
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


import com.waze.R;
import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;

public class WazeAppWidgetGraphActivity extends Activity {
	
	@Override
	public void onCreate( Bundle savedInstanceState )
    {
        /** Called when the activity is first created. */
        super.onCreate(savedInstanceState);
        setContentView( R.layout.app_widget_graph );
        /*
         * Set callbacks    
         */
        ImageButton btnClose = (ImageButton) findViewById( R.id.btn_graph_close );
        btnClose.setOnClickListener( new OnCloseListener() );
    }	
	
	private final class OnCloseListener implements OnClickListener
	{
		public void onClick( View aView )
		{
			finish();
		}
	}	
}
