/* roadmap_androidmath.c - Math functions implementations for the android system
 *
 * LICENSE:
 *
  *   Copyright 2008 Alex Agranovich
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
 *  See roadmap_androidmath.h
 */

#include "roadmap_androidmath.h"

/*
/*************************************************************************************************
 * c-style div implementation (absent in the android bionic libc
 *
 */
/*
div_t div( int num, int denom )
{
	div_t retVal;
	retVal.quot = num/denom;
	retVal.rem =  num - denom*retVal.quot;
	return retVal;
}
*/
