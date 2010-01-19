/* roadmap_wchar.h - Wide char support interface
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
 */

#ifndef INCLUDE__ROADMAP_WCHAR__H
#define INCLUDE__ROADMAP_WCHAR__H

#define WCHAR_SIZE 4		/* Actual wide char size in opposite to sizeof ( wchar_t ) that can be just 1 byte */

typedef wchar_t RM_WCHAR_T; 	// To prevent compatibility problems of wchar_t of different sizes
							// int here is 32 bit

typedef size_t RM_SIZE_T; 	// To prevent compatibility problems

/*
RM_SIZE_T mbstowcs( RM_WCHAR_T *pwcs, const char *s, RM_SIZE_T n );

int mbtowc( RM_WCHAR_T *pwc, const char *s, RM_SIZE_T n );

RM_SIZE_T wcslen( const RM_WCHAR_T *ws );
*/

#endif // INCLUDE__ROADMAP_WCHAR__H
