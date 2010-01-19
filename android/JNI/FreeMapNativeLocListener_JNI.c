/* FreeMapNativeLocListener_JNI.c - JNI STUB for the FreeMapNativeLocListener class
 *
 *
 * LICENSE:
 *
 *   Copyright 2008 Alex Agranovich
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
 *   See FreeMapNativeLocListener_JNI.h
 */


#include "FreeMapJNI.h"
#include "roadmap.h"
#include "roadmap_trip.h"
#include "roadmap_screen.h"
#include "roadmap_androidgps.h"

#include <string.h>
// The JNI object representing the current class
static android_jni_obj_type gJniObj;

/*************************************************************************************************
 * Java_com_waze_FreeMapNativeLocListener_LocListenerCallbackNTV()
 * The callback for the GPS update service. Called from the Java layer
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeLocListener_LocListenerCallbackNTV
  (JNIEnv *aJNIEnv, jobject aJObj, jbyte aStatus, jint aGpsTime, jint aLatitude,
					  jint aLongitude, jint aAltitude, jint aSpeed, jint aSteering, jint aIsCellData )
{
	static int location_set = 0;

    JNI_LOG( ROADMAP_INFO, "Update the GPS. Status: %c, Time %d, Latitude %d, \n\
					Longtitude %d, Altitude %d, Speed %d, Steering %d, Is Cell Data %d", aStatus, aGpsTime, aLatitude,
					aLongitude, aAltitude, aSpeed, aSteering, aIsCellData );

    AndroidPositionMode mode = aIsCellData ? _andr_pos_cell : _andr_pos_gps;

    roadmap_gpsandroid_set_position( mode, aStatus, aGpsTime, aLatitude, aLongitude, aAltitude, aSpeed, aSteering );
}

/*************************************************************************************************
 * Java_com_waze_FreeMapNativeLocListener_LocListenerCallbackNTV()
 * The callback for the GPS sattelites update
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeLocListener_SatteliteListenerCallbackNTV
(JNIEnv *aJNIEnv, jobject aJObj, jint aNum )
{
	int i;
	JNI_LOG( ROADMAP_INFO, "Update the Sattelites. Number: %d", aNum );

	if (!GpsdSatelliteCallback) return;

    for (i = 1; i <= aNum; i++)
    {
          (*GpsdSatelliteCallback)( i, 0, 0, 0, 0, 1 );
    }
    (*GpsdSatelliteCallback)( 0, 0, 0, 0, 0, 0 );

}

/*************************************************************************************************
 * Java_com_waze_FreeMapNativeLocListener_LocListenerCallbackNTV()
 * The callback for the GPS dilution update
 *
 */
JNIEXPORT void JNICALL Java_com_waze_FreeMapNativeLocListener_DilutionListenerCallbackNTV
(JNIEnv *aJNIEnv, jobject aJObj, jint aDim, jdouble aPdop, jdouble aHdop, jdouble aVdop )
{
	JNI_LOG( ROADMAP_INFO, "Update the Dilution. Dimension: %d. Position: %f. Horizontal: %f. Vertical: %f",
			aDim, aPdop, aHdop, aVdop );
	if (!GpsdDilutionCallback) return;

	(*GpsdDilutionCallback)( aDim, aPdop, aHdop, aVdop );
}
