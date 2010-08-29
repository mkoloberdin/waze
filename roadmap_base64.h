/* roadmap_base64.h - Base 64 encoding/decoding
 *
 * LICENSE:
 *
 *   Copyright 2010 Avi R.
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

#ifndef INCLUDE__ROADMAP_BASE64__H
#define INCLUDE__ROADMAP_BASE64__H

int roadmap_base64_get_buffer_size (int binary_length);
int roadmap_base64_encode (const void* inData, int inLength, char** outText, int bufferSize);
int roadmap_base64_decode (char* inText, void** outData);

#endif // INCLUDE__ROADMAP_BASE64__H
