/* roadmap_androidbrowser.c - Android recommend functionality
 *
 * LICENSE:
 *
  *   Copyright 2010 Alex Agranovich (AGA), Waze Ltd
 *
 *   This file is part of RoadMap.
 *
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
 */

#include "roadmap_recommend.h"
#include "JNI/FreeMapJNI.h"

static void roadmap_androidrecommend_launcher( void );

/***********************************************************/
/*  Name        : void roadmap_androidrecommend_init()
 *  Purpose     : Initializes the recommend dialog
 *  Params      : void
 */
void roadmap_androidrecommend_init( void )
{
   /*
    * Launcher registration.
    */
   roadmap_recommend_register_launcher( (RMRateLauncherCb) roadmap_androidrecommend_launcher );
}

/*************************************************************************************************
 * void roadmap_androidrecommend_launcher( void )
 * Shows the market page for waze application
 *
 */
static void roadmap_androidrecommend_launcher( void )
{
   FreeMapNativeManager_MarketRate();
}
