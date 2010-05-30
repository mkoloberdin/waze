// setupdll.cpp : Defines the entry point for the DLL application.
//

#include <windows.h>
#include <notify.h>
#include "ce_setup.h"
#include "setupdll.h"

///////////////////////////////////////////////////////////
//PURPOSE : HANDLES TASKS DONE AT START OF INSTALLATION
///////////////////////////////////////////////////////////
codeINSTALL_INIT Install_Init(HWND hwndparent,
  BOOL ffirstcall,BOOL fpreviouslyinstalled,LPCTSTR pszinstalldir)
{
   if (!ffirstcall || !fpreviouslyinstalled) return codeINSTALL_INIT_CONTINUE;

   const int NUM_FILES = 2;
   const WCHAR *FILES[] = {
         L"edt77001.rdm",
         L"usc77001.dgl"
   };

   
   WCHAR maps_dir[255];
   wcscpy(maps_dir, pszinstalldir);
   wcscat(maps_dir, L"\\maps");

   for (int i=0; i<NUM_FILES; i++) {
      WCHAR file[300];

      _sntprintf(file, sizeof(file), L"%s\\%s", maps_dir, FILES[i]);
      DeleteFile(file);
   }
    
   return codeINSTALL_INIT_CONTINUE;
}

///////////////////////////////////////////////////////////
//PURPOSE : HANDLES TASKS DONE AT END OF INSTALLATION
///////////////////////////////////////////////////////////
codeINSTALL_EXIT Install_Exit(
    HWND hwndparent,LPCTSTR pszinstalldir,
    WORD cfaileddirs,WORD cfailedfiles,WORD cfailedregkeys,
    WORD cfailedregvals,
    WORD cfailedshortcuts)
{
   WCHAR roadmap_exe[255];
   wcscpy(roadmap_exe, pszinstalldir);
   wcscat(roadmap_exe, L"\\FreeMap.exe");

   BOOL res = CeRunAppAtEvent(roadmap_exe, NOTIFICATION_EVENT_NONE);
   res = CeRunAppAtEvent(roadmap_exe, NOTIFICATION_EVENT_RS232_DETECTED);
   res = CeRunAppAtEvent(roadmap_exe, NOTIFICATION_EVENT_SYNC_END);
   
	PROCESS_INFORMATION pi;
	STARTUPINFO si;

	memset(&pi, 0, sizeof(pi));
	memset(&si, 0, sizeof(si));

	si.cb = sizeof(STARTUPINFO);

	if (CreateProcess(roadmap_exe, APP_RUN_AT_RS232_DETECT, NULL,
      NULL, FALSE, 0, NULL, NULL, &si, &pi)) {

	   CloseHandle(pi.hThread);
   }

   return codeINSTALL_EXIT_DONE;
}

///////////////////////////////////////////////////////////////
//PURPOSE : HANDLES TASKS DONE AT BEGINNING OF UNINSTALLATION
///////////////////////////////////////////////////////////////
codeUNINSTALL_INIT Uninstall_Init(
  HWND hwndparent,LPCTSTR pszinstalldir)
{
    //local variables    
    WIN32_FIND_DATA findfiledata;
    HANDLE hfind;
    TCHAR pszfilepath[50];
 
    WCHAR roadmap_exe[255];
    wcscpy(roadmap_exe, pszinstalldir);
    wcscat(roadmap_exe, L"\\FreeMap.exe");

    BOOL res = CeRunAppAtEvent(roadmap_exe, NOTIFICATION_EVENT_NONE);

/*
    //initialize character array
    memset(pszfilepath,0,sizeof(pszfilepath));


    //copy database path to character array
    //pszinstalldir variable will contain the 
    //application path (eg : \Program Files\TestApp)
    wcscpy(pszfilepath,pszinstalldir);
    wcscat(pszfilepath,TEXT("\\test.sdf"));

    //trying to find whether database file exists or not
    hfind = FindFirstFile(pszfilepath,&findfiledata);

    if(hfind != INVALID_HANDLE_VALUE) //database file exists
    {
        //delete database
        DeleteFile(pszfilepath);
    }

*/
    //return value
    return codeUNINSTALL_INIT_CONTINUE;
}


///////////////////////////////////////////////////////////
//PURPOSE : HANDLES TASKS DONE AT END OF UNINSTALLATION
///////////////////////////////////////////////////////////
codeUNINSTALL_EXIT Uninstall_Exit(HWND hwndparent)
{
    //do nothing
    //return value
    return codeUNINSTALL_EXIT_DONE;
}

