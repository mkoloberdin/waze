/**  
 * WazeAppWidgetNoDataActivity - Activity displaying no data information
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


import com.waze.FreeMapAppActivity;
import com.waze.R;
import com.waze.WazeResManager;
import com.waze.WazeTypefaceSpan;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Typeface;
import android.os.Bundle;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageButton;
import android.widget.TextView;

public class WazeAppWidgetNoDataActivity extends Activity {
	
	@Override
	public void onCreate( Bundle savedInstanceState )
    {
		
		WazeAppWidgetLog.d("WazeAppWidgetNoDataActivity.onCreate" );
		
		
        /** Called when the activity is first created. */
        super.onCreate(savedInstanceState);
        
        mInstance = this;
        
        setContentView( R.layout.app_widget_nodata );
            
        /*
         * Format texts
         */
        TextView txtTitle = (TextView) findViewById( R.id.text_title );
        txtTitle.setTypeface( WazeResManager.getFaceVagBold( getApplicationContext() ) );

        TextView txtBtnEnter = (TextView) findViewById( R.id.text_btn_enter );
        txtBtnEnter.setTypeface( WazeResManager.getFaceVagBold( getApplicationContext() ) );
        
        formatBodyText();
        
        /*
         * Set callbacks    
         */
        ImageButton btnClose = (ImageButton) findViewById( R.id.btn_nodata_close );
        btnClose.setOnClickListener( new OnCloseListener() );
        TextView btnTextEnter = (TextView) findViewById( R.id.text_btn_enter );
        btnTextEnter.setOnClickListener( new OnEnterListener() );
        
    }
	
	
	private final class OnCloseListener implements OnClickListener
	{
		public void onClick( View aView )
		{
			mInstance.finish();
		}
	}

	private final class OnEnterListener implements OnClickListener
	{
		public void onClick( View aView )
		{
			Context context = mInstance.getApplicationContext();
			Intent intent = new Intent( context, FreeMapAppActivity.class );
			intent.setFlags( Intent.FLAG_ACTIVITY_NEW_TASK|Intent.FLAG_ACTIVITY_MULTIPLE_TASK );
			context.startActivity( intent );
			mInstance.finish();
		}
	}

	private void formatBodyText()
	{
		Typeface faceBold = WazeResManager.getFaceVagBlck( getApplicationContext() );
		/*
		 * First paragraph
		 */
        TextView txtInfo1 = (TextView) findViewById( R.id.text_info1 );
        txtInfo1.setTypeface( WazeResManager.getFaceVagReg( getApplicationContext() ) );

		/*
		 * Second paragraph
		 */
        TextView txtInfo2 = (TextView) findViewById( R.id.text_info2 );
        txtInfo2.setTypeface( WazeResManager.getFaceVagReg( getApplicationContext() ) );
        CharSequence text = txtInfo2.getText();
        final String homeStr = "'Home'";
        final String workStr = "'Work'";
        
        SpannableStringBuilder spanBuilder = new SpannableStringBuilder( text );
        String textString = text.toString();
        
        int start = textString.indexOf( homeStr );
        spanBuilder.setSpan( new WazeTypefaceSpan( faceBold ), 
        			start, start + homeStr.length(), Spanned.SPAN_INCLUSIVE_INCLUSIVE );
        start = textString.indexOf( workStr );
        spanBuilder.setSpan( new WazeTypefaceSpan( faceBold ), 
        			start, start + workStr.length(), Spanned.SPAN_INCLUSIVE_INCLUSIVE );
        
        txtInfo2.setText( spanBuilder );
        
		/*
		 * Third paragraph
		 */
        TextView txtInfo3 = (TextView) findViewById( R.id.text_info3 );
        txtInfo3.setTypeface( WazeResManager.getFaceVagReg( getApplicationContext() ) );
        text = txtInfo3.getText();
        
        final String driveToStr = "'Drive to'";
        final String favsStr = "'Save to Favorites'";        
        
        spanBuilder = new SpannableStringBuilder( text );
        textString = text.toString();
        // Drive to 
        start = textString.indexOf( driveToStr );
        spanBuilder.setSpan( new WazeTypefaceSpan( faceBold ), 
        			start, start + driveToStr.length(), Spanned.SPAN_INCLUSIVE_INCLUSIVE );
        // Favorites 
        start = textString.indexOf( favsStr );
        spanBuilder.setSpan( new WazeTypefaceSpan( faceBold ), 
        			start, start + favsStr.length(), Spanned.SPAN_INCLUSIVE_INCLUSIVE );
        // Home
        textString.indexOf( homeStr );
        spanBuilder.setSpan( new WazeTypefaceSpan( faceBold ), 
        			start, start + homeStr.length(), Spanned.SPAN_INCLUSIVE_INCLUSIVE );
        // Work
        start = textString.indexOf( workStr );
        spanBuilder.setSpan( new WazeTypefaceSpan( faceBold ), 
        			start, start + workStr.length(), Spanned.SPAN_INCLUSIVE_INCLUSIVE );
        
        txtInfo3.setText( spanBuilder );
		
	}
	private WazeAppWidgetNoDataActivity mInstance;
}

