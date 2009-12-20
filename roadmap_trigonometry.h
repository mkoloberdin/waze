/* roadmap_trigonometry.h - All we need when it comes to trigonometric table.
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
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
 *  This file contains a limited set of trigonometric data for sine and
 *  cosine functions. The result is multiplied by 32768 to get the most
 *  accuracy using short integers.
 *
 *  The data represents the (x,y) position of 45 points on a 32768 radius
 *  1/8 of a circle that is centered at the origin (0,0). The origin and
 *  direction are defined such that point (0,-1) would rotate clockwise.
 *
 *  Extending this table to cover the full 360 degree range involves two
 *  simple transformations:
 *  - compute the modulo 90. If the modulo is greater than 45, substract
 *    it to 90 and exchange sine and cosine.
 *  - change sign and exchange sin and cosine depending on the quadrant.
 */

#ifndef _ROADMAP_TRIGONOMETRY__H_
#define _ROADMAP_TRIGONOMETRY__H_

#include "roadmap_gui.h"

static RoadMapGuiPoint RoadMapTrigonometricTable[] = {

   {    0,32767},          /* 0 degrees. */
   {  572,32763},          /* 1 degrees. */
   { 1144,32748},          /* 2 degrees. */
   { 1715,32723},          /* 3 degrees. */
   { 2286,32688},          /* 4 degrees. */
   { 2856,32643},          /* 5 degrees. */
   { 3425,32588},          /* 6 degrees. */
   { 3993,32524},          /* 7 degrees. */
   { 4560,32449},          /* 8 degrees. */
   { 5126,32365},          /* 9 degrees. */
   { 5690,32270},          /* 10 degrees. */
   { 6252,32166},          /* 11 degrees. */
   { 6813,32052},          /* 12 degrees. */
   { 7371,31928},          /* 13 degrees. */
   { 7927,31795},          /* 14 degrees. */
   { 8481,31651},          /* 15 degrees. */
   { 9032,31499},          /* 16 degrees. */
   { 9580,31336},          /* 17 degrees. */
   {10126,31164},          /* 18 degrees. */
   {10668,30983},          /* 19 degrees. */
   {11207,30792},          /* 20 degrees. */
   {11743,30592},          /* 21 degrees. */
   {12275,30382},          /* 22 degrees. */
   {12803,30163},          /* 23 degrees. */
   {13328,29935},          /* 24 degrees. */
   {13848,29698},          /* 25 degrees. */
   {14365,29452},          /* 26 degrees. */
   {14876,29197},          /* 27 degrees. */
   {15384,28932},          /* 28 degrees. */
   {15886,28660},          /* 29 degrees. */
   {16384,28378},          /* 30 degrees. */
   {16877,28088},          /* 31 degrees. */
   {17364,27789},          /* 32 degrees. */
   {17847,27482},          /* 33 degrees. */
   {18324,27166},          /* 34 degrees. */
   {18795,26842},          /* 35 degrees. */
   {19261,26510},          /* 36 degrees. */
   {19720,26170},          /* 37 degrees. */
   {20174,25822},          /* 38 degrees. */
   {20622,25466},          /* 39 degrees. */
   {21063,25102},          /* 40 degrees. */
   {21498,24730},          /* 41 degrees. */
   {21926,24351},          /* 42 degrees. */
   {22348,23965},          /* 43 degrees. */
   {22763,23571},          /* 44 degrees. */
   {23170,23170}           /* 45 degrees. */
};

#endif // _ROADMAP_TRIGONOMETRY__H_

