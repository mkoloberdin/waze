/**  
 * WazeTypefaceSpan - Typeface span class. Allows using custom fonts in spans
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
 *   @see 
 */

package com.waze;

import android.text.style.TypefaceSpan;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.text.TextPaint;
;

public class WazeTypefaceSpan extends TypefaceSpan 
{
	   public WazeTypefaceSpan( Typeface aType ) 
	    {
	        super( "sans" );
	        mNewType = aType;
	    }
	   
    public WazeTypefaceSpan( String aFamily, Typeface aType ) 
    {
        super( aFamily );
        mNewType = aType;
    }

    @Override
    public void updateDrawState( TextPaint aTextPaint ) 
    {
        applyTypeFace( aTextPaint, mNewType );
    }

    @Override
    public void updateMeasureState( TextPaint aTextPaint ) 
    {
        applyTypeFace( aTextPaint, mNewType );
    }

    private static void applyTypeFace( Paint aPaint, Typeface aType ) 
    {
        int oldStyle;
        Typeface old = aPaint.getTypeface();
        if (old == null) {
            oldStyle = 0;
        } else {
            oldStyle = old.getStyle();
        }

        int fake = oldStyle & ~aType.getStyle();
        if ( ( fake & Typeface.BOLD ) != 0 ) 
        {
        	aPaint.setFakeBoldText( true );
        }

        if ( ( fake & Typeface.ITALIC ) != 0 ) 
        {
        	aPaint.setTextSkewX(-0.25f);
        }

        aPaint.setTypeface( aType );
    }
    
    private final Typeface mNewType;
}