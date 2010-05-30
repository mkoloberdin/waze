/* roadmap_main.c - The main function of the RoadMap application.
 *
 * LICENSE:
 *
 *   Copyright 2005 Ehud Shabtai
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
 *   roadmap_main.h
 */

#include <windows.h>
#include <commctrl.h>
#include "resource.h"
#include <winsock.h>
#include <time.h>
#include <comutil.h>

#ifdef UNDER_CE
#include <aygshell.h>
#include <notify.h>
#include "CEException.h"
#include "CEDevice.h"
#endif

#ifndef I_IMAGENONE
#define I_IMAGENONE (-2)
#endif

#define MIN_SYNC_TIME 1*3600

#ifdef WIN32_PROFILE
#include <C:\Program Files\Windows CE Tools\Common\Platman\sdk\wce500\include\cecap.h>
#endif

extern "C" {
#include "../roadmap.h"
#include "../roadmap_path.h"
#include "../roadmap_start.h"
#include "../roadmap_config.h"
#include "../roadmap_history.h"
#include "../roadmap_canvas.h"
#include "../roadmap_io.h"
#include "../roadmap_main.h"
#include "../roadmap_res.h"
#include "../roadmap_serial.h"
#include "../roadmap_messagebox.h"
#include "../roadmap_screen.h"
#include "../roadmap_download.h"
#include "../roadmap_lang.h"
#include "../roadmap_dialog.h"
#include "../roadmap_gps.h"
#include "../roadmap_keyboard.h"
#include "../roadmap_device_events.h"
#include "wince_input_mon.h"
#include "win32_serial.h"
#include "roadmap_wincecanvas.h"
#include "../editor/editor_main.h"
#include "../editor/export/editor_sync.h"
#include "../Realtime/RealtimeAlerts.h"
#include "../roadmap_utf8.h"
#include "../roadmap_camera.h"
#include "../ssd/ssd_dialog.h"
#include "../ssd/ssd_confirm_dialog.h"
#include "../roadmap_urlscheme.h"
#ifdef SSD
#include "roadmap_pointer.h"
#include "roadmap_screen.h"
#endif
}

#ifdef _WIN32
   #ifdef   TOUCH_SCREEN
      #pragma message("    Target device type:    TOUCH-SCREEN")
   #else
      #pragma message("    Target device type:    MENU ONLY (NO TOUCH-SCREEN)")
   #endif   // TOUCH_SCREEN
#endif   // _WIN32

///[BOOKMARK]:[NOTE]:[PAZ] - Ability to force the usage of the phone keyboard (0..9,'*','#')
///#define  FORCE_PHONE_KEYBOARD_USAGE


///[BOOKMARK]:[NOTE]:[PAZ] - For testing
///#define  TESTING_BUILD

#ifdef   FORCE_PHONE_KEYBOARD_USAGE
   #pragma message("    NOTE: Forcing the usage of phone-keyboard!")
   extern "C" int USING_PHONE_KEYPAD = TRUE;
#else
   extern "C" int USING_PHONE_KEYPAD = FALSE;
#endif   // FORCE_PHONE_KEYBOARD_USAGE

#ifdef UNDER_CE
   #ifndef  KBDI_KEYBOARD_PRESENT
      // Indicates whether or not the system has keyboard hardware.
      #define  KBDI_KEYBOARD_PRESENT         (0x01)
   #endif   // KBDI_KEYBOARD_PRESENT

   #ifndef  KBDI_KEYBOARD_ENABLED
      // Indicates whether or not the keyboard hardware is enabled.
      #define  KBDI_KEYBOARD_ENABLED         (0x02)
   #endif   // KBDI_KEYBOARD_ENABLED

   #ifndef  KBDI_KEYBOARD_ENTER_ESC
      // Indicates whether or not the keyboard hardware has ENTER and ESC keys.
      #define  KBDI_KEYBOARD_ENTER_ESC       (0x04)
   #endif   // KBDI_KEYBOARD_ENTER_ESC

   #ifndef  KBDI_KEYBOARD_ALPHA_NUM
      // Indicates whether or not the keyboard hardware has alphanumeric keys.
      #define  KBDI_KEYBOARD_ALPHA_NUM       (0x08)
   #endif   // KBDI_KEYBOARD_ALPHA_NUM

   // All flags:
   #define  ROADMAP_FULL_KEYBOARD            (KBDI_KEYBOARD_PRESENT|KBDI_KEYBOARD_ENABLED|KBDI_KEYBOARD_ENTER_ESC|KBDI_KEYBOARD_ALPHA_NUM)
#endif   // UNDER_CE


#ifdef   TESTING_BUILD
   #pragma message("    WARNING: TEST-BUILD")
   void  TestingFeature( POINT* pPoint);

   ///#define  SHOW_RECT_ON_SCREEN
#endif   //    TESTING_BUILD
/////////////////////////////////////////////////////////////////////////////////////////////////
BOOL   OnKeyDown  ( WORD key);   //   WM_KEYDOWN
BOOL   OnKeyUp    ( WORD key);   //   WM_KEYUP
BOOL   OnChar     ( WORD key);   //   WM_CHAR
/////////////////////////////////////////////////////////////////////////////////////////////////

#include "ConnectionThread.h"
///#include "ReceiverThread.h"


// Menu & toolbar defines

struct tb_icon {
   char *name;
   int id;
};
static   RoadMapCallback menu_callbacks[MAX_MENU_ITEMS] = {0};
static   RoadMapCallback tool_callbacks[MAX_TOOL_ITEMS] = {0};

// timer stuff
#define ROADMAP_MAX_TIMER 20
struct roadmap_main_timer {
   unsigned int id;
   RoadMapCallback callback;
   HWND hWnd;
};
static struct roadmap_main_timer RoadMapMainPeriodicTimer[ROADMAP_MAX_TIMER];

// IO stuff
#define ROADMAP_MAX_IO 16
static roadmap_main_io *RoadMapMainIo[ROADMAP_MAX_IO] = {0};

static   HANDLE      VirtualSerialHandle      = 0;
static   HWND        g_hMainWnd               = NULL;
static   bool        RoadMapMainSync         = false;
static   BOOL        full_screen             = FALSE;
static   const char* RoadMapMainVirtualSerial;

#ifdef   UNDER_CE
static   HWND        g_hWndDialogMenuBar       = NULL;
static   BOOL        ignore_first_keyup_event  = TRUE;
#else
static   HMENU      g_hWndDialogMenuBar      = NULL;
#endif   //   UNDER_CE
static CConnectionThreadMgr   g_ConnectionThreadMgr;

// Global Variables used by other C modules:
extern "C"
{
   HINSTANCE   g_hInst            = NULL;
   HWND        RoadMapMainWindow   = NULL;
   HANDLE      eventShuttingDown = NULL;
   BOOL        shutting_down     = FALSE;
}

// Forward declarations of functions included in this code module:
static ATOM                  MyRegisterClass(HINSTANCE, LPTSTR);
static BOOL                  InitInstance(HINSTANCE, LPTSTR);
static LRESULT CALLBACK      WndProc(HWND, UINT, WPARAM, LPARAM);
static INT_PTR CALLBACK      About(HWND, UINT, WPARAM, LPARAM);

#define MAX_LOADSTRING 100

// class name definition
#ifdef _ROADGPS
static WCHAR g_szWindowClass[]      = L"RoadGPSClass";
static WCHAR g_szOtherWindowClass[] = L"RoadMapClass";
#else
static WCHAR g_szWindowClass[]      = L"RoadMapClass";
static WCHAR g_szOtherWindowClass[] = L"RoadGPSClass";
#endif

static RoadMapConfigDescriptor RoadMapConfigGPSVirtual =
                        ROADMAP_CONFIG_ITEM("GPS", "Virtual");

static RoadMapConfigDescriptor RoadMapConfigMenuBar =
                        ROADMAP_CONFIG_ITEM("General", "Menu bar");

#ifdef UNDER_CE
static RoadMapConfigDescriptor RoadMapConfigAutoSync =
                                  ROADMAP_CONFIG_ITEM("FreeMap", "Auto sync");
static RoadMapConfigDescriptor RoadMapConfigLastSync =
                                  ROADMAP_CONFIG_ITEM("FreeMap", "Last sync");
#endif

static RoadMapConfigDescriptor RoadMapConfigFirstTime =
                                  ROADMAP_CONFIG_ITEM("FreeMap", "Welcome wizard");

static RoadMapConfigDescriptor RoadMapConfigUser =
                                  ROADMAP_CONFIG_ITEM("FreeMap", "User Name");

static RoadMapConfigDescriptor RoadMapConfigPassword =
                                  ROADMAP_CONFIG_ITEM("FreeMap", "Password");

static void first_time_wizard (void);

static HANDLE g_hMutexAppRunning = NULL;

static int VK_FUNC_MOTOROLA = 17;
static int VK_FUNC_SAMSUNG = 144;
static BOOL FUNC_PRESSED = 0;
static const int HASH_KEY = 120;
static const int STAR_KEY = 119;
static WORD reMapNumbers(WORD key){ // patch to fix strange keyboard behaviour on moto q.	
	switch(key){
		case ('1') :
			return 'e';
		case ('2') :
			return 'r';
		case ('3') :
			return 't';
		case ('4') :
			return 'd';
		case ('5') :
			return 'f';
		case ('6') :
			return 'g';
		case ('7') :
			return 'x';
		case ('8') :
			return 'c';
		case ('9') :
			return 'v';

		default:
			return key;
	}	
}

static BOOL AppInstanceExists()
{
   BOOL bAppRunning = FALSE;

   assert( !g_hMutexAppRunning);
   g_hMutexAppRunning = CreateMutex( NULL, FALSE,
                                     L"Global\\FreeMap RoadMap");
   if( !g_hMutexAppRunning)
   {
      assert(0);
      return TRUE;
   }

   if( ERROR_ALREADY_EXISTS == ::GetLastError())
   {
      ::CloseHandle( g_hMutexAppRunning);
      g_hMutexAppRunning = NULL;
   }

   return ( g_hMutexAppRunning == NULL );
}


static void roadmap_start_event (int event) {
   switch (event) {
   case ROADMAP_START_INIT:
#ifdef FREEMAP_IL
      editor_main_check_map ();
#endif
      break;
   }
}

static void CALLBACK AvoidSuspend (HWND hwnd, UINT uMsg, UINT idEvent,
                                   DWORD dwTime) {
#ifdef UNDER_CE
   CEDevice::wakeUp();
#endif
}

#ifndef _ROADGPS
#ifdef UNDER_CE
static BOOL roadmap_main_should_sync (void) {

   roadmap_config_declare
      ("session", &RoadMapConfigLastSync, "0", NULL);

   if (roadmap_config_match(&RoadMapConfigAutoSync, "No")) {
      return FALSE;
   } else {
      unsigned int last_sync_time =
         roadmap_config_get_integer(&RoadMapConfigLastSync);

      if (last_sync_time &&
         ((last_sync_time + MIN_SYNC_TIME) > time(NULL))) {

         return FALSE;
      }

      roadmap_config_set_integer (&RoadMapConfigLastSync, time(NULL));
      roadmap_config_save (0);
   }

   return TRUE;
}
#endif

static void roadmap_main_start_sync (void) {

   struct hostent *h;
   int i;

   if (RoadMapMainSync) return;

   RoadMapMainSync = true;

   Sleep(1000);

   for (i=0; i<5; i++) {
      if ((h = gethostbyname ("ppp_peer")) != NULL) {
         export_sync ();
         break;
      }
      Sleep(1000);
   }

   RoadMapMainSync = false;
}

static void perform_sync()
{
#ifdef UNDER_CE
   if( !RoadMapMainSync && roadmap_main_should_sync())
#endif   // UNDER_CE
      roadmap_main_start_sync();
}

#endif

static void setup_virtual_serial (void) {

#ifdef UNDER_CE
   DWORD index;
   DWORD resp;
   HKEY key;

   const char *virtual_port = roadmap_config_get (&RoadMapConfigGPSVirtual);

   if (strlen(virtual_port) < 5) return;
   if (strncmp(virtual_port, "COM", 3)) return;

   index = atoi (virtual_port + 3);

   if ((index < 0) || (index > 9)) return;

#ifdef _ROADGPS
   if (FindWindow(g_szOtherWindowClass, NULL) != NULL) {
      /* RoadMap or RoadGPS is already running */
      RoadMapMainVirtualSerial = virtual_port;
      return;
   }
#endif

   RegCreateKeyEx(HKEY_LOCAL_MACHINE, L"Drivers\\RoadMap",
      0, NULL, REG_OPTION_NON_VOLATILE, 0, NULL, &key, &resp);
   RegSetValueEx(key, L"Dll", 0, REG_SZ, (unsigned char*)L"ComSplit.dll", 26);
   RegSetValueEx(key, L"Prefix", 0, REG_SZ, (unsigned char*)L"COM", 8);
   RegSetValueEx(key, L"Index", 0, REG_DWORD, (unsigned char*)&index, sizeof(DWORD));

   RegCloseKey(key);

   //res = RegisterDevice(L"COM", 4, L"ComSplit.dll", 0);
   VirtualSerialHandle = ActivateDevice(L"Drivers\\RoadMap", NULL);

   if (VirtualSerialHandle == 0) {
      roadmap_messagebox ("Virtual comm Error!", "Can't setup virtual serial port.");
   }
#endif
}


const char *roadmap_main_get_virtual_serial (void) {
   return RoadMapMainVirtualSerial;
}

int handleException(EXCEPTION_POINTERS *exceptionPointers) {

//#ifdef UNDER_CE
        //CEException::writeException(TEXT("\\roadmapCrash"), exceptionPointers);
//#endif
	roadmap_log( ROADMAP_FATAL, "roadmap_main.cpp::handleException() code=%x, address=%x ", exceptionPointers->ExceptionRecord->ExceptionCode,exceptionPointers->ExceptionRecord->ExceptionAddress);

        return EXCEPTION_EXECUTE_HANDLER;


/* wchar_t     FileName[MAX_PATH+1];
   HANDLE      hFile = INVALID_HANDLE_VALUE;
   SYSTEMTIME  ST;
   MINIDUMP_EXCEPTION_INFORMATION
               EI;
   HANDLE      hThread     = ::GetCurrentThread();
   HANDLE      hProcess    = ::GetCurrentProcess();
   DWORD       dwProcessID = ::GetCurrentProcessId();

   EI.ThreadId          = ::GetCurrentThreadId();
   EI.ExceptionPointers = exceptionPointers;
   EI.ClientPointers    = TRUE;

   ::GetLocalTime( &ST);


   wsprintf( FileName, L"WazerDumpFile - %02d.%02d.%02d - %02d.%02d.%02d.dmp",
                        ST.wYear, ST.wMonth, ST.wDay,
                        ST.wHour, ST.wMinute, ST.wSecond);

   hFile = ::CreateFile(FileName,
                        GENERIC_READ|GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

   if( INVALID_HANDLE_VALUE == hFile)
      return EXCEPTION_EXECUTE_HANDLER;

   ::MiniDumpWriteDump( hProcess,
                        dwProcessID,
                        hFile,
                        MiniDumpNormal,
                        &EI,
                        NULL,
                        NULL);

   ::CloseHandle( hFile);
   hFile = INVALID_HANDLE_VALUE;*/
}

extern "C"
BOOL Freemap_ShowMenuBar();
BOOL Freemap_ShowMenuBar()
{
#ifdef   UNDER_CE
   SHMENUBARINFO   MBI;

   ignore_first_keyup_event = FALSE;

   if( g_hWndDialogMenuBar)
   {
      ::MessageBox( g_hMainWnd, L"Error: Menu bar is already active?", L"Freemap_ShowMenuBar()", MB_OK);
      return FALSE;
   }

   memset(&MBI, 0, sizeof(SHMENUBARINFO));
   MBI.cbSize      = sizeof(SHMENUBARINFO);
   MBI.hwndParent= g_hMainWnd;
   MBI.nToolBarId= IDM_MENU;
   MBI.hInstRes   = g_hInst;
   MBI.dwFlags   = SHCMBF_HMENU;

   if( ::SHCreateMenuBar( &MBI))
      g_hWndDialogMenuBar   = MBI.hwndMB;
   else
   {
      ::MessageBox( g_hMainWnd, L"Error: Failed to create menu bar...", L"Freemap_ShowMenuBar()", MB_OK);
      return FALSE;
   }


#else
   //   Windows 32
   if( g_hWndDialogMenuBar)
      return TRUE;

/*   For windows build - fix the menu issue (RC file)
   Maybe build menu dynamically  */

   g_hWndDialogMenuBar = ::LoadMenu( g_hInst, MAKEINTRESOURCE(IDM_MENU));
   if( g_hWndDialogMenuBar)
      ::SetMenu( g_hMainWnd, g_hWndDialogMenuBar);

#endif   //   UNDER_CE

   return TRUE;
}

extern "C"
BOOL Freemap_HideMenuBar();
BOOL Freemap_HideMenuBar()
{
   if( !g_hWndDialogMenuBar)
      return FALSE;

#ifdef   UNDER_CE
   ignore_first_keyup_event = TRUE;
   ::DestroyWindow( g_hWndDialogMenuBar);
#else
   ::SetMenu( g_hMainWnd, NULL);
   ::DestroyMenu( g_hWndDialogMenuBar);
#endif   //   UNDER_CE

   g_hWndDialogMenuBar = NULL;

   return TRUE;
}

// our main function
#ifdef UNDER_CE
int WINAPI WinMain(HINSTANCE hInstance,
               HINSTANCE hPrevInstance,
               LPTSTR    lpCmdLine,
               int       nCmdShow)
#else
int WINAPI WinMain(HINSTANCE hInstance,
               HINSTANCE hPrevInstance,
               LPSTR     lpCmdLine,
               int       nCmdShow)
#endif
{
   MSG      msg;
   LPTSTR   cmd_line = L"";
#ifdef UNDER_CE
#ifndef  FORCE_PHONE_KEYBOARD_USAGE
   DWORD dwKeyboardStatus  = ::GetKeyboardStatus();
   DWORD dwKyeboardCaps    = (dwKeyboardStatus&ROADMAP_FULL_KEYBOARD);
   if( ROADMAP_FULL_KEYBOARD == dwKyeboardCaps)
      USING_PHONE_KEYPAD = FALSE;
   else
      USING_PHONE_KEYPAD = TRUE;
#endif   // FORCE_PHONE_KEYBOARD_USAGE

   cmd_line = lpCmdLine;
#endif

#ifdef WIN32_PROFILE
   //SuspendCAPAll();
#endif

   eventShuttingDown = ::CreateEvent( NULL, TRUE, FALSE, NULL);

   if(FAILED( g_ConnectionThreadMgr.Create()))
      return -1;

   __try
   {
      // Perform application initialization:
      if (!InitInstance(hInstance, cmd_line))
         return -1;
      setup_virtual_serial ();
   }
   __except (handleException(GetExceptionInformation())) {}

   ShowWindow( RoadMapMainWindow, nCmdShow);
   UpdateWindow( RoadMapMainWindow);

   HACCEL hAccelTable;
   hAccelTable = 0;

#ifdef UNDER_CE
   ///[BOOKMARK]:[NOTE]:[PAZ] - Set keyboard input mode:
   //  on WM2003 ( not working!) EDIT_SETINPUTMODE(RoadMapMainWindow, EIM_TEXT)
   ///   On WM6      -  SHSetImeMode( g_hMainWnd, SHIME_MODE_SPELL);
#endif   // UNDER_CE
   // Main message loop:
   while (GetMessage(&msg, NULL, 0, 0))
   {
      if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
      {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }

   if( !shutting_down)
   {
      ::SetEvent( eventShuttingDown);
      shutting_down = TRUE;
      roadmap_start_exit();
   }

   g_ConnectionThreadMgr.Destroy();

#ifdef UNDER_CE
   if (VirtualSerialHandle != 0) {
      DeactivateDevice (VirtualSerialHandle);
   }
#endif

   if( g_hMutexAppRunning)
   {
      ::CloseHandle( g_hMutexAppRunning);
      g_hMutexAppRunning = NULL;
   }

   WSACleanup();
   return (int) msg.wParam;
}


ATOM MyRegisterClass(HINSTANCE hInstance, LPTSTR szWindowClass)
{
   WNDCLASS wc;

   wc.style         = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc   = WndProc;
   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = hInstance;
#ifdef _ROADGPS
   wc.hIcon         = 0;
#else
   wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ROADMAP));
#endif
   wc.hCursor       = 0;
   wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
   wc.lpszMenuName  = 0;
   wc.lpszClassName = szWindowClass;

   return RegisterClass(&wc);
}

#ifdef   UNDER_CE
void CheckIfOrientationChanged( bool force = false)
{
   int   iScreenWidth   = ::GetSystemMetrics(SM_CXFULLSCREEN);
   int   iScreenHeight  = ::GetSystemMetrics(SM_CYFULLSCREEN);
   BOOL  bWideScreen    = (iScreenHeight < iScreenWidth);
   static
   BOOL  s_bWideScreen  = bWideScreen;

   if( force || (s_bWideScreen != bWideScreen))
   {
      s_bWideScreen  = bWideScreen;

      roadmap_device_event_notification( device_event_window_orientation_changed);
   }
}
#endif   // UNDER_CE

#ifdef UNDER_CE
// Get RS232 connection event
static   BOOL g_EnableAutoSync  = FALSE;

static void SetAutoSync( BOOL bActivate)
{}

BOOL GetAutoSyncSetting()
{
   if( roadmap_config_match( &RoadMapConfigAutoSync, "No"))
      return FALSE;

   return TRUE;
}

void OnSettingsChanged_EnableAutoSync(void)
{
   BOOL bEnabled = GetAutoSyncSetting();

   if( bEnabled)
   {
      if( !g_EnableAutoSync)
         SetAutoSync( TRUE);
   }
   else
   {
      if( g_EnableAutoSync)
         SetAutoSync( FALSE);
   }
}
#endif   // UNDER_CE

static int urldecode(char *src, char *last, char *dest){ 
  int code; 
  for (; src != last; src++, dest++){ 
    if (*src == '+') *dest = ' '; 
    else if(*src == '%') { 
      if(sscanf(src+1, "%2x", &code) != 1) code = '?'; 
      *dest = code; 
      src +=2; 
    } 
    else *dest = *src; 
  } 
  *dest   = '\n'; 
  *++dest = '\0'; 
  return 0; 
} 

extern "C" void ssd_dialog_wait ();
BOOL InitInstance(HINSTANCE hInstance, LPTSTR lpCmdLine)
{
   WSADATA  wsaData;
   DWORD    dwProcessID = ::GetCurrentProcessId();
   bool     do_sync     = false;

   g_hInst = hInstance; // Store instance handle in our global variable

#ifdef UNDER_CE
   SHInitExtraControls();

   if (!wcscmp(lpCmdLine, APP_RUN_AT_RS232_DETECT) ||
       !wcscmp(lpCmdLine, APP_RUN_AFTER_SYNC)) {
   return FALSE;
   }
#endif   // UNDER_CE

#ifndef _ROADGPS
   if( AppInstanceExists())
   {
      roadmap_log( ROADMAP_INFO, "InitInstance(PID: 0x%08X) - Other 'waze.exe' instance is already running", dwProcessID);

      HWND hWnd = FindWindow(g_szWindowClass, NULL);
      if (hWnd)
         SetForegroundWindow((HWND)((ULONG) hWnd | 0x00000001));
      else
         roadmap_log( ROADMAP_WARNING, "InitInstance(PID: 0x%08X) - Failed to find other instance window handle (maybe this is a secondery event)", dwProcessID);

      return FALSE;
   }
#endif   // !_ROADGPS

   //If it is already running, then focus on the window, and exit

   if (!MyRegisterClass(hInstance, g_szWindowClass))
      return FALSE;
   
   if(WSAStartup(MAKEWORD(1,1), &wsaData) != 0)
      roadmap_log (ROADMAP_FATAL, "Can't initialize network");

	if (lpCmdLine && *lpCmdLine)
	{
	   char url[URL_MAX_LENGTH];
	   char query[URL_MAX_LENGTH];
	   wcstombs(query,lpCmdLine,URL_MAX_LENGTH);
	   urldecode(query, query+strlen(query), url);
	   roadmap_urlscheme_remove_prefix( query, url );
	   roadmap_urlscheme_init( query );
	}

	char *args[1] = {0};

   roadmap_start_subscribe (roadmap_start_event);
   roadmap_start(0, args);

#if(!defined _ROADGPS && defined FREEMAP_IL)

   roadmap_config_declare_enumeration
               ("preferences", &RoadMapConfigFirstTime, NULL, "Yes", "No", NULL);

   if (roadmap_config_match(&RoadMapConfigFirstTime, "Yes")) {
#ifdef UNDER_CE
      first_time_wizard();
#else
      roadmap_main_start_sync ();
      roadmap_config_set (&RoadMapConfigFirstTime, "No");
#endif
   }

#endif

#ifdef   UNDER_CE
   CheckIfOrientationChanged( true /* force */);
#endif   // UNDER_CE

   return TRUE;
}

extern "C" void roadmap_start_quick_menu (void);



LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   int wmId, wmEvent;
   PAINTSTRUCT ps;
   HDC hdc;

#ifdef UNDER_CE
   static SHACTIVATEINFO s_sai;
#endif

   __try {
   switch (message)
   {
   case WM_COMMAND:
      wmId    = LOWORD(wParam);
      wmEvent = HIWORD(wParam);
      {
         if ((wmId >= MENU_ID_START) &&
            (wmId < (MENU_ID_START + MAX_MENU_ITEMS))) {
            RoadMapCallback callback =
               menu_callbacks[wmId - MENU_ID_START];
            if (callback == NULL) {
               roadmap_log (ROADMAP_ERROR,
                  "Can't find callback for menu item:%d",
                  wmId);
               break;
            }
            (*callback)();
         } else if ((wmId >= TOOL_ID_START) &&
            (wmId < (TOOL_ID_START + MAX_TOOL_ITEMS))) {
            RoadMapCallback callback =
               tool_callbacks[wmId - TOOL_ID_START];
            if (callback == NULL) {
               roadmap_log (ROADMAP_ERROR,
                  "Can't find callback for tool item:%d",
                  wmId);
               break;
            }
            (*callback)();
         } else {
            return DefWindowProc(hWnd, message, wParam, lParam);
         }
      }
      break;

   case WM_CREATE:
   {
#ifdef UNDER_CE
      // Initialize the shell activate info structure
      memset(&s_sai, 0, sizeof (s_sai));
      s_sai.cbSize = sizeof (s_sai);
      CEDevice::init();

      SetTimer (NULL, 0, 50000, AvoidSuspend);

      int   iMaxX = GetSystemMetrics(SM_CXSCREEN);
      int   iMaxY = GetSystemMetrics(SM_CYSCREEN);
      MoveWindow( hWnd, 0, 0, iMaxX, iMaxY, TRUE);

#endif   //   UNDER_CE
      g_ConnectionThreadMgr.SetWindow( hWnd);
      g_hMainWnd = hWnd;

#ifndef UNDER_CE
      ::SetWindowPos( hWnd, 0, 0, 0, 480, 700, SWP_NOMOVE|SWP_NOREPOSITION|SWP_NOZORDER|SWP_SHOWWINDOW);
#endif   //   UNDER_CE

      break;
   }

   case WM_PAINT:
      hdc = BeginPaint(hWnd, &ps);
      roadmap_canvas_refresh();
      EndPaint(hWnd, &ps);
      break;

#ifdef   UNDER_CE
   case WM_WINDOWPOSCHANGED:
      CheckIfOrientationChanged();
      return DefWindowProc(hWnd, message, wParam, lParam);
#endif   // UNDER_CE

   case WM_SIZE:
      roadmap_canvas_new(RoadMapMainWindow, NULL);
      ssd_dialog_resort_tab_order ();
      break;

   case WM_DESTROY:
#ifdef UNDER_CE
      CEDevice::end();
#endif
      PostQuitMessage(0);
      break;

   case WM_LBUTTONDOWN:
      {
         POINT point;
         point.x = LOWORD(lParam);
         point.y = HIWORD(lParam);

#ifdef   TESTING_BUILD
         //TestingFeature( &point);
         roadmap_canvas_button_pressed(&point);
#else
         roadmap_canvas_button_pressed(&point);

#endif   // TESTING_BUILD
      }
      break;

   case WM_LBUTTONUP:
      {
         POINT point;
         point.x = LOWORD(lParam);
         point.y = HIWORD(lParam);
         roadmap_canvas_button_released(&point);
      }
      break;

   case WM_MOUSEMOVE:
      {
         POINT point;
         point.x = LOWORD(lParam);
         point.y = HIWORD(lParam);
         roadmap_canvas_mouse_moved(&point);
      }
      break;

   case WM_KEYDOWN:
   {
      WORD  Code           = (WORD)wParam;
      int   iRepeatTimes   = (lParam & 0x0000FFFF);
      int   iScanCode      = (lParam & 0x00FF0000) >> 16;
      BOOL  bALT_IsDown    = (lParam & 0x20000000)? TRUE: FALSE;
      BOOL  bAlreadyPressed= (lParam & 0x40000000)? TRUE: FALSE;
      BOOL  bNowReleased   = (lParam & 0x80000000)? TRUE: FALSE;
	  if ( (wParam==VK_FUNC_MOTOROLA)||(wParam==VK_FUNC_SAMSUNG)){
		 FUNC_PRESSED = TRUE;
	  }
      if( OnKeyDown( Code))
         return 0;

      return DefWindowProc(hWnd, message, wParam, lParam);
   }

   case WM_KEYUP:
   {
      WORD  Code           = (WORD)wParam;
      int   iRepeatTimes   = (lParam & 0x0000FFFF);
      int   iScanCode      = (lParam & 0x00FF0000) >> 16;
      BOOL  bALT_IsDown    = (lParam & 0x20000000)? TRUE: FALSE;
      BOOL  bAlreadyPressed= (lParam & 0x40000000)? TRUE: FALSE;
      BOOL  bNowReleased   = (lParam & 0x80000000)? TRUE: FALSE;
	  
	  if ( (wParam==VK_FUNC_MOTOROLA)||(wParam==VK_FUNC_SAMSUNG)){
		 FUNC_PRESSED = FALSE;
	  }
      if( OnKeyUp( Code))
         return 0;

      return DefWindowProc(hWnd, message, wParam, lParam);
   }

   case WM_CHAR:
   {
      WORD  Key            = (WORD)wParam;
      int   iRepeatTimes   = (lParam & 0x0000FFFF);
      BOOL  bALT_IsDown    = (lParam & 0x20000000)? TRUE: FALSE;
      BOOL  bAlreadyPressed= (lParam & 0x40000000)? TRUE: FALSE;
      BOOL  bNowReleased   = (lParam & 0x80000000)? TRUE: FALSE;
#ifndef TOUCH_SCREEN  
	  if (!FUNC_PRESSED) // fix problem where key codes aren't received correctly
		  Key = reMapNumbers(Key);
#endif
	  if( OnChar( Key))
         return 0;

      return DefWindowProc(hWnd, message, wParam, lParam);
   }

#if((defined UNDER_CE) && (!defined SIMPLE_SCREEN))
   case WM_ACTIVATE:
      if( full_screen && (WA_INACTIVE != LOWORD(wParam)))
      {
         SHFullScreen(RoadMapMainWindow, SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON);
         MoveWindow(RoadMapMainWindow, 0, 0,GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),TRUE);
      } else {
         // Notify shell of our activate message
         SHHandleWMActivate(hWnd, wParam, lParam, &s_sai, FALSE);
      }
      break;

#endif

#ifndef _ROADGPS
   case WM_FREEMAP_SYNC:
      roadmap_log( ROADMAP_DEBUG, "WndProc(PID: 0x%08X) - Received 'WM_FREEMAP_SYNC' message - !PATH DISABLED!", ::GetCurrentProcessId());
      break;

#endif

#ifdef   UNDER_CE
   case WM_FREEMAP_DEVICE_EVENT:
      roadmap_device_event_notification( (device_event)wParam);
      return 0;
#endif   // UNDER_CE

   case WM_FREEMAP_READ:
      {
         roadmap_main_io *context = (roadmap_main_io *) wParam;
         if (!context->is_valid) break;

         if (lParam != 1) {
            Win32SerialConn *conn = (Win32SerialConn *) lParam;

            if (!ROADMAP_SERIAL_IS_VALID (conn)) {
               /* An old input which was removed */
               break;
            }
         }

         (*context->callback) (context->io);
         break;
      }

   case WM_FREEMAP_SOCKET:
   {
      LPConnectionInfo  pCI   = (LPConnectionInfo)wParam;
      RoadMapSocket     Socket= (RoadMapSocket)lParam;

      pCI->pfnOnNewSocket( Socket, pCI->pContext, pCI->rc);

      break;
   }

   default:
      return DefWindowProc(hWnd, message, wParam, lParam);
   }
}
__except (handleException(GetExceptionInformation())) {}
   return 0;
}


static struct tb_icon RoadMapIcons[] = {
   {"destination", IDB_RM_DESTINATION},
   {"location", IDB_RM_LOCATION},
   {"gps", IDB_RM_GPS},
   {"hold", IDB_RM_HOLD},
   {"counterclockwise", IDB_RM_COUNTERCLOCKWISE},
   {"clockwise", IDB_RM_CLOCKWISE},
   {"zoomin", IDB_RM_ZOOMIN},
   {"zoomout", IDB_RM_ZOOMOUT},
   {"zoom1", IDB_RM_ZOOM1},
   {"full", IDB_RM_FULL},
   {"record", IDB_RM_RECORD},
   {"stop", IDB_RM_STOP},
   {"quit", IDB_RM_QUIT},
   {"down", IDB_RM_DOWN},
   {"up", IDB_RM_UP},
   {"right", IDB_RM_RIGHT},
   {"left", IDB_RM_LEFT},
   {NULL, 0}
};

#if(0)
static void config_auto_sync () {
   WCHAR roadmap_exe[255];
   LPWSTR dir_unicode = ConvertToWideChar(roadmap_path_user (), CP_UTF8);
   BOOL res;

   wcscpy(roadmap_exe, dir_unicode);
   free (dir_unicode);

   wcscat(roadmap_exe, L"\\waze.exe");

   if (roadmap_config_match(&RoadMapConfigAutoSync, "No")) {
      res = CeRunAppAtEvent(roadmap_exe, NOTIFICATION_EVENT_NONE);
      if (!res) roadmap_log (ROADMAP_ERROR, "Can't reset WinCE event notification.");
   } else {
      CeRunAppAtEvent(roadmap_exe, NOTIFICATION_EVENT_NONE);
      res = CeRunAppAtEvent(roadmap_exe, NOTIFICATION_EVENT_RS232_DETECTED);
      res |= CeRunAppAtEvent(roadmap_exe, NOTIFICATION_EVENT_SYNC_END);

      if (!res) roadmap_log (ROADMAP_ERROR, "Can't set WinCE event notification.");
   }

   roadmap_config_save (1);
}
#endif


static int roadmap_main_toolbar_icon (const char *icon)
{
   if (icon == NULL) return NULL;

   tb_icon *itr = RoadMapIcons;

   while (itr->name != NULL) {
      if (!strcasecmp(icon, itr->name)) {
         return itr->id;
      }
      itr++;
   }

   return -1;
}


extern "C" {

void roadmap_main_toggle_full_screen (void)
{
#ifdef UNDER_CE

#ifndef TOUCH_SCREEN
   static BOOL already_set = FALSE;

   if( already_set)
      return;

   already_set = TRUE;
   SHFullScreen(RoadMapMainWindow, SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON|SHFS_HIDESTARTICON);
   MoveWindow(RoadMapMainWindow, 0, 0,GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),TRUE);
#else
   RECT rc2;

   full_screen = !full_screen;

   GetWindowRect(RoadMapMainWindow, &rc2);

   if( full_screen)
   {
      SHFullScreen(RoadMapMainWindow, SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON);
      MoveWindow(RoadMapMainWindow, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), TRUE);
   }
   else
   {
      RECT rc;

      SHFullScreen(RoadMapMainWindow, SHFS_SHOWTASKBAR);

      SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, FALSE);
      MoveWindow(RoadMapMainWindow, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, TRUE);
   }


#endif   //TOUCH_SCREEN
#endif
}

void roadmap_main_new (const char *title, int width, int height)
{
   LPWSTR szTitle = ConvertToWideChar(title, CP_UTF8);
   DWORD style = WS_VISIBLE;

   roadmap_config_declare_enumeration
    ("preferences", &RoadMapConfigMenuBar, NULL, "No", "Yes", NULL);
#ifdef UNDER_CE

   roadmap_config_declare_enumeration
    ("preferences", &RoadMapConfigAutoSync, OnSettingsChanged_EnableAutoSync, "Yes", "No", NULL);

   SetAutoSync( GetAutoSyncSetting());

   roadmap_config_declare
    ("session", &RoadMapConfigLastSync, "0", NULL);
#else
   style = WS_OVERLAPPEDWINDOW;
#endif

      if (width == -1) width = CW_USEDEFAULT;
      if (height == -1) height = CW_USEDEFAULT;
	  
      RoadMapMainWindow = CreateWindow(g_szWindowClass, szTitle, style,
         CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL,
         NULL, g_hInst, NULL);


   free(szTitle);
   if (!RoadMapMainWindow)
   {
      roadmap_log (ROADMAP_FATAL, "Can't create main window");
      return;
   }

#ifdef FREEMAP_IL
   editor_main_set (1);
#endif
}


   void roadmap_main_set_input (RoadMapIO *io, RoadMapInput callback)
   {
      int i;

      for (i = 0; i < ROADMAP_MAX_IO; ++i) {
         if (RoadMapMainIo[i] == NULL) {
            RoadMapMainIo[i] = (roadmap_main_io *) malloc (sizeof(roadmap_main_io));
            RoadMapMainIo[i]->io = io;
            RoadMapMainIo[i]->callback = callback;
            RoadMapMainIo[i]->is_valid = 1;
            break;
         }
      }

      if (i == ROADMAP_MAX_IO) {
         roadmap_log (ROADMAP_FATAL, "Too many set input calls");
         return;
      }

      HANDLE monitor_thread = NULL;
      switch (io->subsystem) {
      case ROADMAP_IO_SERIAL:
         io->os.serial->ref_count++;
         monitor_thread = CreateThread(NULL, 0,
            SerialMonThread, (void*)RoadMapMainIo[i], 0, NULL);
         break;
      case ROADMAP_IO_NET:
         monitor_thread = CreateThread(NULL, 0,
            SocketMonThread, (void*)RoadMapMainIo[i], 0, NULL);
         break;
      case ROADMAP_IO_FILE:
         monitor_thread = CreateThread(NULL, 0,
            FileMonThread, (void*)RoadMapMainIo[i], 0, NULL);
         break;
      }

      if(monitor_thread == NULL)
      {
         roadmap_log (ROADMAP_FATAL, "Can't create monitor thread");
         roadmap_io_close(io);
         return;
      }
      else {
         CloseHandle(monitor_thread);
      }
   }


   void roadmap_main_remove_input (RoadMapIO *io)
   {
      int i;

      for (i = 0; i < ROADMAP_MAX_IO; ++i) {
         if (RoadMapMainIo[i] && RoadMapMainIo[i]->io == io) {

            if (RoadMapMainIo[i]->is_valid) {
               RoadMapMainIo[i]->is_valid = 0;
            } else {
               free (RoadMapMainIo[i]);
            }

            RoadMapMainIo[i] = NULL;
            break;
         }
      }
   }


   static void roadmap_main_timeout (HWND hwnd, UINT uMsg, UINT idEvent,
      DWORD dwTime)
   {
      int i;
      struct roadmap_main_timer *timer = NULL;

      for( i=0; i<ROADMAP_MAX_TIMER; i++)
         if( RoadMapMainPeriodicTimer[i].id == idEvent)
         {
            timer = RoadMapMainPeriodicTimer + i;
            break;
         }

      if( timer)
      {
         RoadMapCallback callback = (RoadMapCallback) timer->callback;

         if (callback != NULL) {
            (*callback) ();
         }
      }
      else
         assert(0);
   }


   void roadmap_main_set_periodic (int interval, RoadMapCallback callback)
   {
      int   index;
      struct roadmap_main_timer *timer = NULL;

      assert(callback);

      for (index = 0; index < ROADMAP_MAX_TIMER; ++index) {

         if (RoadMapMainPeriodicTimer[index].callback == callback) {
            return;
         }

         if( !timer && !RoadMapMainPeriodicTimer[index].callback)
         {
            timer    = RoadMapMainPeriodicTimer + index;
            timer->id= index + 1;
         }
      }

      if( timer)
      {
         timer->callback= callback;
         timer->hWnd    = RoadMapMainWindow;
         timer->id      = SetTimer( RoadMapMainWindow,
                                    timer->id,
                                    interval,
                                    (TIMERPROC)roadmap_main_timeout);
      }
      else
         roadmap_log (ROADMAP_FATAL, "WIN32::roadmap_main_set_periodic() - Timer table saturated");
   }


   void roadmap_main_remove_periodic (RoadMapCallback callback)
   {
      int index;
      bool found = false;

      for (index = 0; index < ROADMAP_MAX_TIMER; ++index) {

         if (RoadMapMainPeriodicTimer[index].callback == callback) {

            assert( !found);

            KillTimer(  RoadMapMainPeriodicTimer[index].hWnd,
                        RoadMapMainPeriodicTimer[index].id);

            RoadMapMainPeriodicTimer[index].callback  = NULL;
            RoadMapMainPeriodicTimer[index].id        = index+1;
            RoadMapMainPeriodicTimer[index].hWnd      = NULL;

            found = true;
         }
      }

      if( !found)
         roadmap_log (ROADMAP_ERROR, "timer 0x%08x not found", callback);
   }


   void roadmap_main_set_status (const char *text) {}

   void roadmap_main_flush (void)
   {
      HWND w = GetFocus();
      MSG msg;

      while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }

      Sleep (50);
      //UpdateWindow(w);
   }


   void roadmap_main_exit (void)
   {
      if( !shutting_down)
      {
         ::SetEvent( eventShuttingDown);
         shutting_down = TRUE;
      }

      roadmap_start_exit ();
      SendMessage(RoadMapMainWindow, WM_CLOSE, 0, 0);
   }

   void roadmap_main_set_cursor (int cursor)
   {
     switch (cursor) {

     case ROADMAP_CURSOR_NORMAL:
       SetCursor(NULL);
       break;

     case ROADMAP_CURSOR_WAIT:
       SetCursor(LoadCursor(NULL, IDC_WAIT));
       break;
     }
   }
} // extern "C"

/* first time wizard */

static void wizard_close (const char *name, void *context) {

//   roadmap_config_set (&RoadMapConfigUser,
//      (char *)roadmap_dialog_get_data ("main", "User Name"));

//   roadmap_config_set (&RoadMapConfigPassword,
//      (char *)roadmap_dialog_get_data ("main", "Password"));

   roadmap_config_set (&RoadMapConfigFirstTime, "No");

   roadmap_dialog_hide (name);
}

static void setup_wizard_callback(int exit_code, void *context){
   roadmap_config_set (&RoadMapConfigFirstTime, "No");

    if (exit_code != dec_yes)
         return;

    roadmap_gps_detect_receiver();
}


void first_time_wizard (void) {
    //roadmap_gps_detect_receiver();
//     ssd_confirm_dialog ("Welcome to Waze!", "Detect GPS", TRUE, setup_wizard_callback , NULL);
}
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
typedef enum tagEMSVirtualKey
{
   MSVK_Invalid        = 0x00,

   MSVK_Back           = 0x12,
   MSVK_Escape         = 0x1B,
   MSVK_Softkey_left   = 0x70,
   MSVK_Softkey_right  = 0x71,
   MSVK_Arrow_up       = 0x26,
   MSVK_Arrow_down     = 0x28,
   MSVK_Arrow_left     = 0x25,
   MSVK_Arrow_right    = 0x27

}  EMSVirtualKey;


//#define  TESTING_KEYBOARD
#ifdef   TESTING_KEYBOARD
#define  roadmap_keyboard_handler__key_pressed     show_key_pressed
BOOL show_key_pressed( WORD key, BOOL virtual_key)
{
   char msg[120];

   if( virtual_key)
      sprintf( msg, "Virtual key: %s", roadmap_keyboard_virtual_key_name( (EVirtualKey)key));
   else
      sprintf( msg, "key: %c", key);

#ifdef   UNDER_CE
   roadmap_messagebox( "Keyboard test", msg);
#else
   MessageBoxA( NULL, msg, "show_key_pressed()", MB_OK);
#endif   // UNDER_CE

   return TRUE;
}
#endif   // TESTING_KEYBOARD

BOOL HandleKeyboardKey( WORD key, uint32_t flags)
{
   char  vk[2];
   char* utf8char = NULL;
   char* out      = NULL;
   BOOL  res;


#ifndef UNDER_CE
   if( (96 == key) || (59 == key /* Hebrew... */))
   {
      key    = VK_Softkey_left;
      flags |= KEYBOARD_VIRTUAL_KEY;
   }
   else if( 126 == key)
   {
      key    = VK_Softkey_right;
      flags |= KEYBOARD_VIRTUAL_KEY;
   }
#endif   // UNDER_CE

#ifdef   TESTING_BUILD
   if( VK_Softkey_left == key)
   {
      POINT pt = {20,20};
      TestingFeature(&pt);
      return TRUE;
   }
#endif   // TESTING_BUILD

   if( KEYBOARD_VIRTUAL_KEY & flags)
   {
      vk[0] = (char)key;
      vk[1] = '\0';

      out   = vk;
   }
   else
   {
#ifdef   _DEBUG
      if( !key)
      {
         assert(0);
         return FALSE;
      }
#endif   // _DEBUG

      utf8char = utf8_char_from_utf16( key);
      if( !utf8char || !(*utf8char))
      {
         assert(0);
         return FALSE;
      }

      if( !utf8char[1])
         flags |= KEYBOARD_ASCII;

      out = utf8char;
   }

   res = roadmap_keyboard_handler__key_pressed( out, flags);

   FREE(utf8char)

   return res;
}

static WORD s_KeyHandled = 0;

BOOL OnKeyDown( WORD key)
{
   EVirtualKey vk = VK_None;

   s_KeyHandled = 0;

   switch(key)
   {
      case MSVK_Arrow_up     : vk = VK_Arrow_up;      break;
      case MSVK_Arrow_down   : vk = VK_Arrow_down;    break;
      case MSVK_Arrow_left   : vk = VK_Arrow_left;    break;
      case MSVK_Arrow_right  : vk = VK_Arrow_right;   break;
      case (HASH_KEY):
		if(FUNC_PRESSED)
			return OnChar('#');
		else
			return OnChar('b');
	  case (STAR_KEY):
		if(FUNC_PRESSED)
			return OnChar('*');
		else
			return OnChar('z');
   }
   


   if( VK_None == vk)
      return FALSE;

   s_KeyHandled = vk;
   return HandleKeyboardKey( vk, KEYBOARD_VIRTUAL_KEY);
}

BOOL OnKeyUp( WORD key)
{
   EVirtualKey vk       = VK_None;
   BOOL        bEscape  = FALSE;
   BOOL        bRes     = FALSE;

   switch(key)
   {
      case MSVK_Back         : vk = VK_Back;          break;
      case MSVK_Softkey_left : vk = VK_Softkey_left;  break;
      case MSVK_Softkey_right: vk = VK_Softkey_right; break;
      case MSVK_Escape       : bEscape = TRUE;        break;
   }

   if( VK_None == vk)
   {
      if( bEscape && (ESCAPE_KEY != s_KeyHandled))
         bRes = HandleKeyboardKey( ESCAPE_KEY, 0);
   }
   else
   {
      if( vk != s_KeyHandled)
         bRes = HandleKeyboardKey( vk, KEYBOARD_VIRTUAL_KEY);
   }

   s_KeyHandled = 0;
   return bRes;
}

BOOL OnChar( WORD key)
{
#if(defined UNDER_CE && defined TESTING_BUILD)
   POINT p = {2,2};
   TestingFeature( &p);
#endif   //   (UNDER_CE && TESTING_BUILD)

   s_KeyHandled = key;
   return HandleKeyboardKey( key, 0);
}
/////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_main_show (void)
{
   if (RoadMapMainWindow != NULL) {
      roadmap_canvas_new(RoadMapMainWindow, NULL);
   }
}

void      roadmap_main_add_status()                        {}
void      roadmap_main_popup_menu(RoadMapMenu,int,int)      {}
void      roadmap_main_add_tool(   const char *label,
                           const char *icon,
                           const char *tip,
                           RoadMapCallback callback)      {}
void      roadmap_main_add_tool_space()   {}
void      roadmap_main_add_menu_item(   RoadMapMenu menu,
                              const char *label,
                              const char *tip,
                              RoadMapCallback callback)   {}
void      roadmap_main_add_menu      (RoadMapMenu menu,
                              const char *label)         {}
RoadMapMenu   roadmap_main_new_menu      ()   { return NULL;}
void      roadmap_main_add_separator(   RoadMapMenu menu)   {}
void      roadmap_main_set_keyboard(   struct RoadMapFactoryKeyMap *bindings,
                              RoadMapKeyInput callback)   {}
/////////////////////////////////////////////////////////////////////////////////////////////////


extern "C"
{

BOOL roadmap_horizontal_screen_orientation()
{
   int   iScreenWidth   = ::GetSystemMetrics(SM_CXFULLSCREEN);
   int   iScreenHeight  = ::GetSystemMetrics(SM_CYFULLSCREEN);
   BOOL  bWideScreen    = (iScreenHeight < iScreenWidth);

   return bWideScreen;
}

BOOL win32_roadmap_net_async_connect(  const char*                protocol,
                                       const char*                name,
                                       time_t                     update_time,
                                       int                        default_port,
                                       int                        flags,
                                       RoadMapNetConnectCallback  on_net_connected,
                                       void*                      context)
{
   ConnectionInfo CI;

   if( !protocol || !(*protocol) || !name || !(*name) || !on_net_connected)
      return FALSE;

   ConnectionInfo_Init( &CI);
   strncpy( CI.Protocol,protocol, CI_STRING_MAX_SIZE);
   strncpy( CI.Name,    name,     CI_STRING_MAX_SIZE);
   CI.iDefaultPort   = default_port;
   CI.pfnOnNewSocket = on_net_connected;
   CI.pContext       = context;
   CI.tUpdate        = update_time;

   if( SUCCEEDED( g_ConnectionThreadMgr.QueueConnectionRequest( &CI)))
      return TRUE;
   // Else
   return FALSE;
}


void roadmap_main_maximize()
{
   ::ShowWindow         ( g_hMainWnd, SW_MAXIMIZE);
   ::SetForegroundWindow( g_hMainWnd);
#ifdef UNDER_CE
   ::SHFullScreen       ( g_hMainWnd, SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON|SHFS_HIDESTARTICON);
#endif   // UNDER_CE
   ::MoveWindow         ( g_hMainWnd, 0, 0,GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),TRUE);
}

// 1. Takes camera capture and saves the image according to the attributes in image_file
// 2. Creates and returns thumbnail buffer according to the attributes in image_thumbnail.
//    The pointer to the buffer is passed back in the image_thumbnail ( see roadmap_camera_defs.h
BOOL roadmap_camera_take_picture(CameraImageFile*  image_file,
                                 CameraImageBuf*   image_thumbnail)
{
#ifdef UNDER_CE
   SHCAMERACAPTURE   CCI;
   _bstr_t           w_output_dir( image_file->folder);
   _bstr_t           w_output_file(image_file->file);
   CAMERACAPTURE_STILLQUALITY
                     win32_quality = CAMERACAPTURE_STILLQUALITY_LOW;

   ::memset( &CCI, 0, sizeof(SHCAMERACAPTURE));

   switch( image_file->quality)
   {
      case quality_low:
         win32_quality = CAMERACAPTURE_STILLQUALITY_LOW;
         break;

      case quality_medium:
         win32_quality = CAMERACAPTURE_STILLQUALITY_NORMAL;
         break;

      case quality_high:
         win32_quality = CAMERACAPTURE_STILLQUALITY_HIGH;
         break;

      default:
         assert(0);
   }

   if( !image_file->folder || !(*image_file->folder))
   {
      if( image_file->folder)
         free( image_file->folder);

      image_file->folder = (char*)malloc(64);
      sprintf( image_file->folder, roadmap_path_user());
   }
   w_output_dir = image_file->folder;

   if( !image_file->file || !(*image_file->file))
   {
      if( image_file->file)
         free( image_file->file);

      image_file->file = (char*)malloc(64);
      sprintf( image_file->file, "WazeAlert_TempPicture.jpg");
   }
   w_output_file = image_file->file;

   CCI.cbSize              = sizeof(SHCAMERACAPTURE);
   CCI.hwndOwner           = g_hMainWnd;
   CCI.pszInitialDir       = w_output_dir;
   CCI.pszDefaultFileName  = w_output_file;
   CCI.pszTitle            = L"Waze";
   CCI.StillQuality        = win32_quality;
   CCI.VideoTypes          = CAMERACAPTURE_VIDEOTYPE_STANDARD;
   CCI.nResolutionWidth    = image_file->width;
   CCI.nResolutionHeight   = image_file->height;
   CCI.nVideoTimeLimit     = 12; // 12 seconds
   CCI.Mode                = CAMERACAPTURE_MODE_STILL;

   HRESULT hr = ::SHCameraCapture( &CCI);

   if( SUCCEEDED( hr))
      roadmap_log(ROADMAP_DEBUG, "roadmap_main::shell_camera_capture() - Succeeded");
   else
      roadmap_log(ROADMAP_DEBUG, "roadmap_main::shell_camera_capture() - Failed with HR: 0x%08X", hr);

   return SUCCEEDED( hr);

#else
   assert(0);
   return E_NOTIMPL;

#endif   // WINDOWS_CE
}

int roadmap_device_get_battery_level()
{
#ifdef UNDER_CE
   SYSTEM_POWER_STATUS_EX  SPS;
   const BOOL              realtime = FALSE;

   if( !::GetSystemPowerStatusEx( &SPS, realtime))
      return -1;

   if( ((int)(SPS.BatteryLifePercent) < 0) || (100 < SPS.BatteryLifePercent))
   {
      assert(0);
      return -1;
   }

   return SPS.BatteryLifePercent;
#else
   return -1;
#endif   // UNDER_CE
}
}  // extern "C"

/////////////////////////////////////////////////////////////////////////////////////////////////
void roadmap_main_add_canvas (void)
{
#if((!defined _ROADGPS) && (!defined SIMPLE_SCREEN))
   roadmap_main_toggle_full_screen ();
#endif

   roadmap_canvas_new(RoadMapMainWindow, NULL);
}

extern "C"
{
   #include "../auto_hide_dlg.h"
}

extern "C" void roadmap_gui_minimize()
{  ::ShowWindow( g_hMainWnd, SW_MINIMIZE);}

extern "C" void roadmap_gui_maximize()
{
   ::ShowWindow         ( g_hMainWnd, SW_MAXIMIZE);
   ::SetForegroundWindow( g_hMainWnd);
#ifdef UNDER_CE
   ::SHFullScreen       ( g_hMainWnd, SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON|SHFS_HIDESTARTICON);
#endif // UNDER_CE
   ::MoveWindow         ( g_hMainWnd, 0, 0,GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),TRUE);
}

static int  dlg_active = false;

static void on_dlg_closed( int exit_code, void* context)
{  dlg_active = false;}

void roadmap_main_minimize()
{
   if( dlg_active)
      return;
   dlg_active = true;

   auto_hide_dlg(on_dlg_closed);
}


#ifdef   TESTING_BUILD

#ifdef SHOW_RECT_ON_SCREEN
#include "..\roadmap_canvas.h"
extern "C" void draw_rect( int x, int y, int width, int height)
{
   RoadMapGuiPoint points[] =
   {
      { x,   y},
      { x + width,y},
      { x + width,y + height +1},
      { x,   y + height+1},
      { x,   y + 10}
   };
   int count = sizeof(points)/sizeof(RoadMapGuiPoint);;

   roadmap_canvas_set_foreground("#FF3322");
   roadmap_canvas_set_thickness (1);
   roadmap_canvas_draw_multiple_lines( 1, &count, points, 0);
   roadmap_canvas_refresh ();
}
#endif   // SHOW_RECT_ON_SCREEN

void TestingFeature( POINT* pPoint)
{
}

#endif   //    TESTING_BUILD
