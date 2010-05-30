/* (C) 2001-2005 Joaqu�n M� L�pez Mu�oz (joaquin@tid.es). All rights reserved.
 * (C) 2005 Tony Kmoch, Telematix a.s. [Windows CE version]
 *
 * Permission is granted to use, distribute and modify this code provided that:
 *   � this copyright notice remain unchanged,
 *   � you submit all changes to the copyright holder and properly mark the
 *     changes so they can be told from the original code,
 *   � credits are given to the copyright holder in the documentation of any
 *     software using this code with the following line:
 *       "Portions copyright 2001 Joaqu�n M� L�pez Mu�oz (joaquin@tid.es)"
 *
 * The author welcomes any suggestions on the code or reportings of actual
 * use of the code. Please send your comments to joaquin@tid.es.
 *
 * The author makes NO WARRANTY or representation, either express or implied,
 * with respect to this code, its quality, accuracy, merchantability, or
 * fitness for a particular purpose.  This software is provided "AS IS", and
 * you, its user, assume the entire risk as to its quality and accuracy.
 *
 * Last modified: August 23rd, 2005
 */

#include "listports.h"
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static BOOL Win9xListPorts(LISTPORTS_CALLBACK lpCbk,LPVOID lpCbkValue);
static BOOL WinNT40ListPorts(LISTPORTS_CALLBACK lpCbk,LPVOID lpCbkValue);
static BOOL Win2000ListPorts(LISTPORTS_CALLBACK lpCbk,LPVOID lpCbkValue);
static BOOL WinCEListPorts(LISTPORTS_CALLBACK lpCbk,LPVOID lpCbkValue);
static BOOL ScanEnumTree(LPCTSTR lpEnumPath,LISTPORTS_CALLBACK lpCbk,LPVOID lpCbkValue);
static LONG OpenSubKeyByIndex(
              HKEY hKey,DWORD dwIndex,REGSAM samDesired,PHKEY phkResult,LPTSTR* lppSubKeyName); 
static LONG QueryStringValue(HKEY hKey,LPCTSTR lpValueName, LPTSTR* lppStringValue);

BOOL ListPorts(LISTPORTS_CALLBACK lpCbk,LPVOID lpCbkValue)
{
  OSVERSIONINFO osvinfo;

  /* check parameters */

  if(!lpCbk){
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  /* determine which platform we're running on and forward
   * to the appropriate routine
   */

  ZeroMemory(&osvinfo,sizeof(osvinfo));
  osvinfo.dwOSVersionInfoSize=sizeof(osvinfo);

  GetVersionEx(&osvinfo);

  switch(osvinfo.dwPlatformId){
    case VER_PLATFORM_WIN32_WINDOWS:
      return Win9xListPorts(lpCbk,lpCbkValue);
      break;
    case VER_PLATFORM_WIN32_NT:
      if(osvinfo.dwMajorVersion<4){
        SetLastError(ERROR_OLD_WIN_VERSION);
        return FALSE;
      }
      else if(osvinfo.dwMajorVersion==4){
        return WinNT40ListPorts(lpCbk,lpCbkValue);
      }
      else{
        return Win2000ListPorts(lpCbk,lpCbkValue); /* hopefully it'll also work for XP */
      }
      break;
#ifdef _WIN32_WCE
    case VER_PLATFORM_WIN32_CE:
      return WinCEListPorts(lpCbk,lpCbkValue);
#endif
    default:
      SetLastError(ERROR_OLD_WIN_VERSION);
      return FALSE;
      break;
  }
}

BOOL Win9xListPorts(LISTPORTS_CALLBACK lpCbk,LPVOID lpCbkValue)
/* Information on communications ports is stored in various places in the registry under
 * the HKEY_LOCAL_MACHINE\ENUM key. The structure of the HKLM\ENUM key, where information
 * on all hw devices present on the system is stored, is as follows
 *
 * HKLM\ENUM
 *   |-LEVEL1
 *     |-LEVEL2
 *       |-LEVEL3
 *         � CLASS
 *
 * LEVEL1 keys indicate the type of technology (vg., "PCI", "BIOS", "SCSI"). LEVEL2
 * keys refer to the type of device. So, for instance, "*PNP0501" is for 16550A UARTs.
 * LEVEL3 keys enumerate actual devices installed on the system. The value "CLASS",
 * present in all LEVEL3 keys, informs about the type of driver. For our purposes, we must
 * pay attention to the Class "Ports", that includes serial as well as parallel ports.
 * In these cases, another two values, "PORTNAME" and "FRIENDLYNAME" are expected.
 * An example follows. Typically, COM1 is recognized as a PnP device by the system BIOS
 * and as such placed on the registry like this:
 *
 * HKLM\ENUM
 *   |-BIOS
 *     |-*PNP0501
 *       |-0D (or any other value, this is not important for us)
 *         � CLASS=        "Ports"
 *         � PORTNAME=     "COM1"
 *         � FRIENDLYNAME= "Communications Port (COM1)"
 *
 * Win9xListPorts() scans all LEVEL3 keys under HKLM\ENUM searching for the Class
 * for ports, and when this is found determines, by means of "PORTNAME", whether the
 * device is a serial port or not.
 */
{
  return ScanEnumTree(TEXT("ENUM"),lpCbk,lpCbkValue);
}

static BOOL WinNT40ListPorts(LISTPORTS_CALLBACK lpCbk,LPVOID lpCbkValue)
/* Unlike Win9x, information on serial ports registry storing in NT 4.0 is
 * scarce. HKEY_LOCAL_MACHINE\HARDWARE\DEVICEMAP\SERIALCOMM is reliably
 * documented to hold the names of all installed serial ports, but I haven't
 * confirmed this for infrared ports and other nonstandard drivers.
 * Descriptions stored under SERIALCOMM lack the "FRIENDLYNAME" found
 * in Win9x, there's only the bare names of the ports.
 */
{
  DWORD  dwError=0;
  HKEY   hKey=NULL;
  DWORD  dwIndex;
  LPTSTR lpValueName=NULL;
  LPTSTR lpPortName=NULL;

  if(dwError=RegOpenKeyEx(HKEY_LOCAL_MACHINE,TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"),
                          0,KEY_READ,&hKey)){
    /* it is really strange that this key does not exist, but could happen in theory */
    if(dwError==ERROR_FILE_NOT_FOUND)dwError=0;
    goto end;
  }

  for(dwIndex=0;;++dwIndex){
    DWORD              cbValueName=32*sizeof(TCHAR);
    DWORD              cbPortName=32*sizeof(TCHAR);
    LISTPORTS_PORTINFO portinfo;

    /* loop asking for the value data til we allocated enough memory */

    for(;;){
      free(lpValueName);
      if(!(lpValueName=(LPTSTR)malloc(cbValueName))){
        dwError=ERROR_NOT_ENOUGH_MEMORY;
        goto end;
      }
      free(lpPortName);
      if(!(lpPortName=(LPTSTR)malloc(cbPortName))){
        dwError=ERROR_NOT_ENOUGH_MEMORY;
        goto end;
      }
      if(!(dwError=RegEnumValue(hKey,dwIndex,lpValueName,&cbValueName,
                                NULL,NULL,(LPBYTE)lpPortName,&cbPortName))){
        break; /* we did it */
      }
      else if(dwError==ERROR_MORE_DATA){ /* not enough space */
        dwError=0;
        /* no indication of space required, we try doubling */
        cbValueName=(cbValueName+sizeof(TCHAR))*2;
      }
      else goto end;
    }

    portinfo.lpPortName=lpPortName;
    portinfo.lpFriendlyName=lpPortName; /* no friendly name in NT 4.0 */
    portinfo.lpTechnology=TEXT(""); /* this information is not available */
    if(!lpCbk(lpCbkValue,&portinfo)){
      goto end; /* listing aborted by callback */
    }
  } 

end:
  free(lpValueName);
  free(lpPortName);
  if(hKey!=NULL)RegCloseKey(hKey);
  if(dwError!=0){
    SetLastError(dwError);
    return FALSE;
  }
  else return TRUE;
}

BOOL Win2000ListPorts(LISTPORTS_CALLBACK lpCbk,LPVOID lpCbkValue)
/* Information on serial ports is stored in the Win2000 registry in a manner very
 * similar to that of Win9x, with three differences:
 *   � The ENUM tree is not located under HKLM, but under HKLM\SYSTEM\CURRENTCONTROLSET.
 *   � The parameter "PORTNAME" is not a LEVEL3 value like "CLASS" and "FRIENDLYNAME";
 *     instead, it is located under an additional subkey named "DEVICE PARAMETERS"
 *   � "CLASS" seems to be deprecated in favor of "CLASSGUID" wich contains a GUID
 *     identifying each type of device. I've seen Win200 installations with and
 *     without "CLASS" values. Moreover, I've seen Win9x installations containing
 *     "CLASSGUID" values, so, to be as robust as possible, ScanEnumTree() accept either
 *     type of value.
 *
 * An example follows:
 *
 * HKLM\SYSTEM\CURRENTCONTROLSET
 *   |-BIOS
 *     |-*PNP0501
 *       |-0D (or any other value, this is not important for us)
 *         � CLASSGUID=    "{4D36E978-E325-11CE-BFC1-08002BE10318}"
 *         � FRIENDLYNAME= "Communications Port (COM1)"
 *         |-DEVICE PARAMETERS
 *           � PORTNAME=   "COM1"
 */
{
  return ScanEnumTree(TEXT("SYSTEM\\CURRENTCONTROLSET\\ENUM"),lpCbk,lpCbkValue);
}

static BOOL WinCEListPorts(LISTPORTS_CALLBACK lpCbk,LPVOID lpCbkValue)
/* Available COM ports on Pocket PC/Windows CE devices are stored in registry under
 * key: HKLM\Drivers\BuiltIn. Note, that also other stuff like native Bluetooth ports
 * are written in this part of registry, so we must skip them using condition
 * (_tcsncicmp(lpPortName,TEXT("COM"),3)!=0) below.
 */
{
  DWORD               dwError=0;
  HKEY                hKey=NULL;
  DWORD               dwIndex;
  LPTSTR              lpPortName=NULL;
  HKEY                hkLevel1=NULL;
  LPTSTR              lpFriendlyName=NULL;
  LISTPORTS_PORTINFO  portinfo;
  DWORD               index;
  DWORD               wordSize = sizeof(DWORD);
  BYTE                found[20] = {0};       

  portinfo.lpPortName = (LPTSTR)malloc(64);

  if(dwError=RegOpenKeyEx(HKEY_LOCAL_MACHINE,TEXT("Drivers\\Active"),
                          0,KEY_READ,&hKey)){
    /* it is really strange that this key does not exist, but could happen in theory */
    if(dwError==ERROR_FILE_NOT_FOUND)dwError=0;
    goto end;
  }

  for(dwIndex=0;;++dwIndex) {
    dwError = 0;
    if (dwError=OpenSubKeyByIndex(hKey,dwIndex,KEY_ENUMERATE_SUB_KEYS,&hkLevel1,NULL)) {
      if (dwError==ERROR_NO_MORE_ITEMS) dwError=0;
      break;
    }

    if (dwError=QueryStringValue(hkLevel1,TEXT("Name"),&lpPortName)) {
      if(dwError==ERROR_FILE_NOT_FOUND) continue; else break;
    }

    if (_wcsnicmp(lpPortName,TEXT("COM"),3)!=0) continue; // We want only COM serial ports

    //if (dwError=RegQueryValueEx(hkLevel1, TEXT("INDEX"), NULL, NULL, (LPBYTE)&index, &wordSize)) {
      //if(dwError==ERROR_FILE_NOT_FOUND) continue; else break;
    //}

    // Now "index" contains serial port number, we put it together with "COM"
    // to format like "COM<index>"
    //_stprintf((LPTSTR)portinfo.lpPortName, TEXT("COM%u"), index);
    wcscpy ((LPTSTR)portinfo.lpPortName, lpPortName);

    // Get friendly name
    //dwError = QueryStringValue(hkLevel1,TEXT("FRIENDLYNAME"),&lpFriendlyName);
    //portinfo.lpFriendlyName = dwError ? TEXT("") : lpFriendlyName;

    portinfo.lpTechnology=TEXT(""); /* this information is not available */

    if (swscanf(portinfo.lpPortName, L"COM%d:", &index)) {

       if (index<sizeof(found)/sizeof(found[0])) found[index] = 1;
    }

    if(!lpCbk(lpCbkValue, &portinfo)){
      break;
    }
  } 

  if(hKey!=NULL) {
     RegCloseKey(hKey);
     hKey = NULL;
  }

  if(dwError=RegOpenKeyEx(HKEY_LOCAL_MACHINE,TEXT("Drivers\\Builtin"),
     0,KEY_READ,&hKey)){
     goto end;
  } else {
     
     DWORD dwIndex = 0;
     
     for(;;){
        TCHAR SubKeyName[20+1];
        DWORD cbSubkeyName = 20;
        FILETIME           filetime;
        
        if(!(dwError=RegEnumKeyEx(hKey,dwIndex,SubKeyName,&cbSubkeyName,
           0,NULL,NULL,&filetime))){
           
           if (!wcsncmp(SubKeyName, L"Serial", 6)) {
              DWORD index = 0;
              
              if (swscanf(SubKeyName, L"Serial%d:", &index)) {
                 
                 if (index && (index<sizeof(found)/sizeof(found[0]))) {
                    found[index] = 1;

                    _stprintf((LPTSTR)portinfo.lpPortName, L"COM%u", index);
                    if(!lpCbk(lpCbkValue, &portinfo)){
                       break;
                    }
                 }
              }
           }
        }
        else if(dwError!=ERROR_MORE_DATA){ /* not enough space */
           goto end;
        }

        dwIndex++;
     }
  }


end:
  free((LPTSTR)portinfo.lpPortName);
  free(lpFriendlyName);
  free(lpPortName);
  if(hKey!=NULL) RegCloseKey(hKey);
  if(hkLevel1!=NULL) RegCloseKey(hkLevel1);
  if(dwError!=0){
    SetLastError(dwError);
    return FALSE;
  }
  else return TRUE;
}

BOOL ScanEnumTree(LPCTSTR lpEnumPath,LISTPORTS_CALLBACK lpCbk,LPVOID lpCbkValue)
{
  static const TCHAR lpstrPortsClass[]=    TEXT("PORTS");
  static const TCHAR lpstrPortsClassGUID[]=TEXT("{4D36E978-E325-11CE-BFC1-08002BE10318}");

  DWORD  dwError=0;
  HKEY   hkEnum=NULL;
  DWORD  dwIndex1;
  HKEY   hkLevel1=NULL;
  DWORD  dwIndex2;
  HKEY   hkLevel2=NULL;
  DWORD  dwIndex3;
  HKEY   hkLevel3=NULL;
  HKEY   hkDeviceParameters=NULL;
  TCHAR  lpClass[sizeof(lpstrPortsClass)/sizeof(lpstrPortsClass[0])];
  DWORD  cbClass;
  TCHAR  lpClassGUID[sizeof(lpstrPortsClassGUID)/sizeof(lpstrPortsClassGUID[0])];
  DWORD  cbClassGUID;
  LPTSTR lpPortName=NULL;
  LPTSTR lpFriendlyName=NULL;
  LPTSTR lpTechnology=NULL;

  if(dwError=RegOpenKeyEx(HKEY_LOCAL_MACHINE,lpEnumPath,0,KEY_ENUMERATE_SUB_KEYS,&hkEnum)){
    goto end;
  }

  for(dwIndex1=0;;++dwIndex1){
    if(hkLevel1!=NULL){
      RegCloseKey(hkLevel1);
      hkLevel1=NULL;
    }
    if(dwError=OpenSubKeyByIndex(
                hkEnum,dwIndex1,KEY_ENUMERATE_SUB_KEYS,&hkLevel1,&lpTechnology)){
      if(dwError==ERROR_NO_MORE_ITEMS){
        dwError=0;
        break;
      }
      else goto end;
    }

    for(dwIndex2=0;;++dwIndex2){
      if(hkLevel2!=NULL){
        RegCloseKey(hkLevel2);
        hkLevel2=NULL;
      }
      if(dwError=OpenSubKeyByIndex(
                   hkLevel1,dwIndex2,KEY_ENUMERATE_SUB_KEYS,&hkLevel2,NULL)){
        if(dwError==ERROR_NO_MORE_ITEMS){
          dwError=0;
          break;
        }
        else goto end;
      }

      for(dwIndex3=0;;++dwIndex3){
        BOOL               bFriendlyNameNotFound=FALSE;
        LISTPORTS_PORTINFO portinfo;

        if(hkLevel3!=NULL){
          RegCloseKey(hkLevel3);
          hkLevel3=NULL;
        }
        if(dwError=OpenSubKeyByIndex(hkLevel2,dwIndex3,KEY_READ,&hkLevel3,NULL)){
          if(dwError==ERROR_NO_MORE_ITEMS){
            dwError=0;
            break;
          }
          else goto end;
        }

        /* Look if the driver class is the one we're looking for.
         * We accept either "CLASS" or "CLASSGUID" as identifiers.
         * No need to dynamically arrange for space to retrieve the values,
         * they must have the same length as the strings they're compared to
         * if the comparison is to be succesful.
         */

        cbClass=sizeof(lpClass);
        if(RegQueryValueEx(hkLevel3,TEXT("CLASS"),NULL,NULL,
                           (LPBYTE)lpClass,&cbClass)==ERROR_SUCCESS&&
           _tcsicmp(lpClass,lpstrPortsClass)==0){
          /* ok */
        }
        else{
          cbClassGUID=sizeof(lpClassGUID);
          if(RegQueryValueEx(hkLevel3,TEXT("CLASSGUID"),NULL,NULL,
                             (LPBYTE)lpClassGUID,&cbClassGUID)==ERROR_SUCCESS&&
             _tcsicmp(lpClassGUID,lpstrPortsClassGUID)==0){
            /* ok */
          }
          else continue;
        }

        /* get "PORTNAME" */

        dwError=QueryStringValue(hkLevel3,TEXT("PORTNAME"),&lpPortName);
        if(dwError==ERROR_FILE_NOT_FOUND){
          /* In Win200, "PORTNAME" is located under the subkey "DEVICE PARAMETERS".
           * Try and look there.
           */

          if(hkDeviceParameters!=NULL){
            RegCloseKey(hkDeviceParameters);
            hkDeviceParameters=NULL;
          }
          if(RegOpenKeyEx(hkLevel3,TEXT("DEVICE PARAMETERS"),0,KEY_READ,
                          &hkDeviceParameters)==ERROR_SUCCESS){
             dwError=QueryStringValue(hkDeviceParameters,TEXT("PORTNAME"),&lpPortName);
          }
        }
        if(dwError){
          if(dwError==ERROR_FILE_NOT_FOUND){ 
            /* boy that was strange, we better skip this device */
            dwError=0;
            continue;
          }
          else goto end;
        }

        /* check if it is a serial port (instead of, say, a parallel port) */

        if(_wcsnicmp(lpPortName,TEXT("COM"),3)!=0)continue;

        /* now go for "FRIENDLYNAME" */

        if(dwError=QueryStringValue(hkLevel3,TEXT("FRIENDLYNAME"),&lpFriendlyName)){
          if(dwError==ERROR_FILE_NOT_FOUND){
            bFriendlyNameNotFound=TRUE;
            dwError=0;
          }
          else goto end;
        }

        /* Assemble the information and pass it on to the callback.
         * In the unlikely case there's no friendly name available,
         * use port name instead.
         */
        portinfo.lpPortName=lpPortName;
        portinfo.lpFriendlyName=bFriendlyNameNotFound?lpPortName:lpFriendlyName;
        portinfo.lpTechnology=lpTechnology;
        if(!lpCbk(lpCbkValue,&portinfo)){
          goto end; /* listing aborted by callback */
        }
      }
    }
  }

end:
  free(lpTechnology);
  free(lpFriendlyName);
  free(lpPortName);
  if(hkDeviceParameters!=NULL)RegCloseKey(hkDeviceParameters);
  if(hkLevel3!=NULL)          RegCloseKey(hkLevel3);
  if(hkLevel2!=NULL)          RegCloseKey(hkLevel2);
  if(hkLevel1!=NULL)          RegCloseKey(hkLevel1);
  if(hkEnum!=NULL)            RegCloseKey(hkEnum);
  if(dwError!=0){
    SetLastError(dwError);
    return FALSE;
  }
  else return TRUE;
}

LONG OpenSubKeyByIndex(
  HKEY hKey,DWORD dwIndex,REGSAM samDesired,PHKEY phkResult,LPTSTR* lppSubKeyName)
{
  DWORD              dwError=0;
  BOOL               bLocalSubkeyName=FALSE;
  LPTSTR             lpSubkeyName=NULL;
  DWORD              cbSubkeyName=128*sizeof(TCHAR); /* an initial guess */
  FILETIME           filetime;

  if(lppSubKeyName==NULL){
    bLocalSubkeyName=TRUE;
    lppSubKeyName=&lpSubkeyName;
  }
  /* loop asking for the subkey name til we allocated enough memory */

  for(;;){
    free(*lppSubKeyName);
    if(!(*lppSubKeyName=(LPTSTR)malloc(cbSubkeyName))){
       dwError=ERROR_NOT_ENOUGH_MEMORY;
       goto end;
    }
    if(!(dwError=RegEnumKeyEx(hKey,dwIndex,*lppSubKeyName,&cbSubkeyName,
                              0,NULL,NULL,&filetime))){
      break; /* we did it */
    }
    else if(dwError==ERROR_MORE_DATA){ /* not enough space */
      dwError=0;
      /* no indication of space required, we try doubling */
      cbSubkeyName=(cbSubkeyName+sizeof(TCHAR))*2;
    }
    else goto end;
  }

  dwError=RegOpenKeyEx(hKey,*lppSubKeyName,0,samDesired,phkResult);

end:
  if(bLocalSubkeyName)free(*lppSubKeyName);
  return dwError;
}

LONG QueryStringValue(HKEY hKey,LPCTSTR lpValueName,LPTSTR* lppStringValue)
{
  DWORD cbStringValue=128*sizeof(TCHAR); /* an initial guess */

  for(;;){
    DWORD dwError;

    free(*lppStringValue);
    if(!(*lppStringValue=(LPTSTR)malloc(cbStringValue))){
      return ERROR_NOT_ENOUGH_MEMORY;
    }
    if(!(dwError=RegQueryValueEx(hKey,lpValueName,NULL,NULL,
                                 (LPBYTE)*lppStringValue,&cbStringValue))){
      return ERROR_SUCCESS;
    }
    else if(dwError==ERROR_MORE_DATA){
      /* not enough space, keep looping */
    }
    else return dwError;
  }
}
