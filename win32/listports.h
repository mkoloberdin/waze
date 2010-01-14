/* A small library that lists serial ports available on the system, both on
 * 9x/ME and NT4.0/2000/XP platforms.
 * Infrared serial ports are listed as well.
 *
 * LEGAL:
 *
 * (C) 2001-2005 Joaquín Mª López Muñoz (joaquin@tid.es). All rights reserved.
 * (C) 2005 Tony Kmoch, Telematix a.s. [Windows CE version]
 *
 * Permission is granted to use, distribute and modify this code provided that:
 *   · this copyright notice remain unchanged,
 *   · you submit all changes to the copyright holder and properly mark the
 *     changes so they can be told from the original code,
 *   · credits are given to the copyright holder in the documentation of any
 *     software using this code with the following line:
 *       "Portions copyright 2001 Joaquín Mª López Muñoz (joaquin@tid.es)"
 *
 * The author welcomes any suggestions on the code or reportings of actual
 * use of the code. Please send your comments to joaquin@tid.es.
 *
 * The author makes NO WARRANTY or representation, either express or implied,
 * with respect to this code, its quality, accuracy, merchantability, or
 * fitness for a particular purpose.  This software is provided "AS IS", and
 * you, its user, assume the entire risk as to its quality and accuracy.
 *
 * Changes in version 2.0
 *   · Added lpTechnology to LISTPORTS_PORTINFO.
 *   · Windows CE supported (Tony Kmoch).
 *
 * Last modified: August 23rd, 2005
 */

#ifndef LISTPORTS_H
#define LISTPORTS_H

#define VERSION_LISTPORTS 0x00020000

#ifdef __cplusplus
extern "C"{
#endif

#include <windows.h>

typedef struct
{
  LPCTSTR lpPortName;     /* "COM1", etc. */
  LPCTSTR lpFriendlyName; /* Suitable to describe the port, as for  */
                          /* instance "Infrared serial port (COM4)" */
  LPCTSTR lpTechnology;   /* "BIOS","INFRARED","USB", etc.          */
}LISTPORTS_PORTINFO;

typedef BOOL (CALLBACK* LISTPORTS_CALLBACK)(LPVOID              lpCallbackValue,
                                            LISTPORTS_PORTINFO* lpPortInfo);
/* User provided callback funtion that receives the information on each
 * serial port available.
 * The strings provided on the LISTPORTS_INFO are not to be referenced after
 * the callback returns; instead make copies of them for later use.
 * If the callback returns FALSE, port enumeration is aborted.
 */

BOOL ListPorts(LISTPORTS_CALLBACK lpCallback,LPVOID lpCallbackValue);
/* Lists serial ports available on the system, passing the information on
 * each port on succesive calls to lpCallback.
 * lpCallbackValue, treated opaquely by ListPorts(), is intended to carry
 * information internal to the callback routine.
 * Returns TRUE if succesful, otherwise error code can be retrieved via
 * GetLastError().
 */

#ifdef __cplusplus
}
#endif

#elif VERSION_LISTPORTS!=0x00020000
#error You have included two LISTPORTS.H with different version numbers
#endif

