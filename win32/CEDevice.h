/* ScummVM - Scumm Interpreter
 * LICENSE:
 * Copyright (C) 2001-2006 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Header: /home/ehud/tmp/sf_cvs/roadmap_editor/src/win32/CEDevice.h,v 1.1 2006/08/09 07:43:03 eshabtai Exp $
 *
 */

#ifndef CEDEVICE
#define CEDEVICE

class CEDevice {
	public:
		static void init();
		static void end();
		static void wakeUp();
		static bool hasPocketPCResolution();
		static bool hasDesktopResolution();
		static bool hasWideResolution();
		static bool hasSmartphoneResolution();
		static bool isSmartphone();
};

#endif
