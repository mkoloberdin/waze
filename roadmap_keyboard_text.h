
/*
 * LICENSE:
 *
 *   Copyright 2008 PazO
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
 */

#ifndef __ROADMAP_KEYBOARD_TEXT_H__
#define __ROADMAP_KEYBOARD_TEXT_H__

#include "roadmap_input_type.h"

BOOL  is_valid_key   ( const char* utf8char, uint16_t input_type);
BOOL  is_alphabetic	( unsigned char key);
BOOL  is_numeric		( unsigned char key);
BOOL  is_white_space	( unsigned char key);
BOOL  is_punctuation	( unsigned char key);
BOOL  is_symbol		( unsigned char key);

#endif // __ROADMAP_KEYBOARD_TEXT_H__

