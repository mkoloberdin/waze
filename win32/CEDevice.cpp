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
 * $Header: /home/ehud/tmp/sf_cvs/roadmap_editor/src/win32/CEDevice.cpp,v 1.2 2007/11/19 20:08:33 eshabtai Exp $
 *
 */

//#define SIMU_SMARTPHONE 1
//#define SIMU_SMARTPHONE_2005 1

#include "windows.h"
#include "CEDevice.h"

#define KEY_CALENDAR 0xc1
#define KEY_CONTACTS 0xc2
#define KEY_INBOX 0xc3
#define KEY_TASK 0xc4

static void (WINAPI* _SHIdleTimerReset)(void) = NULL;
static HANDLE (WINAPI* _SetPowerRequirement)(PVOID,int,ULONG,PVOID,ULONG) = NULL;
static DWORD (WINAPI* _ReleasePowerRequirement)(HANDLE) = NULL;
static HANDLE _hPowerManagement = NULL;
static DWORD _lastTime = 0;

#define TIMER_TRIGGER 9000

// Power management code borrowed from MoDaCo & Betaplayer. Thanks !
void CEDevice::init() {
	HINSTANCE dll = LoadLibrary(TEXT("aygshell.dll"));
	if (dll) {
		*(FARPROC*)&_SHIdleTimerReset = GetProcAddress(dll, MAKEINTRESOURCE(2006));
	}
	dll = LoadLibrary(TEXT("coredll.dll"));
	if (dll) {
		*(FARPROC*)&_SetPowerRequirement = GetProcAddress(dll, TEXT("SetPowerRequirement"));
		*(FARPROC*)&_ReleasePowerRequirement = GetProcAddress(dll, TEXT("ReleasePowerRequirement"));

	}
	if (_SetPowerRequirement)
		_hPowerManagement = _SetPowerRequirement(TEXT("BKL1:"), 0, 1, NULL, 0);
	_lastTime = GetTickCount();
}

void CEDevice::end() {
	if (_ReleasePowerRequirement && _hPowerManagement) {
		_ReleasePowerRequirement(_hPowerManagement);
	}
}

void CEDevice::wakeUp() {
	DWORD currentTime = GetTickCount();
	if (currentTime > _lastTime + TIMER_TRIGGER) {
		_lastTime = currentTime;
		SystemIdleTimerReset();
		if (_SHIdleTimerReset)
			_SHIdleTimerReset();
	}
}


bool CEDevice::isSmartphone() {
#ifdef SIMU_SMARTPHONE
	return true;
#else
	TCHAR platformType[100];
	BOOL result = SystemParametersInfo(SPI_GETPLATFORMTYPE, sizeof(platformType), platformType, 0);
	if (!result && GetLastError() == ERROR_ACCESS_DENIED)
		return true;
	return (wcsnicmp(platformType, TEXT("SmartPhone"), 10) == 0);
#endif
}

