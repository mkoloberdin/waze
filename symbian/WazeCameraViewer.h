/* WazeCameraViewer.h
 *
 * LICENSE:
 *
 *   Copyright 2009, Alex Agranovich
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
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
 *
 *   The interface for the object drawing the frames coming from the viewfinder
 *
 */

#ifndef __WAZE_CAMERA_VIEWER_H__
#define __WAZE_CAMERA_VIEWER_H__

#include <fbs.h>

class MWazeCameraViewer
	{
	public: // New methods

		/**
		* Draws the frame on the screen
		*
		* @param aFrame - the frame to be drawn
		* @return void
		*/
		virtual void DrawImage( const CFbsBitmap& aFrame ) = 0;

		/**
		* Returns the display mode of the system
		*
		* @param 
		* @return TDisplayMode
		*/
		virtual TDisplayMode DisplayMode() const = 0;
		/**
		* Returns the current view rectangle
		*
		* @param  void
		* @return TRect
		*/
		virtual TRect Rect() const = 0;
};

#endif // __WAZE_CAMERA_VIEWER_H__

// End of File
