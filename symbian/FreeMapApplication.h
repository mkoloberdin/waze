/* FreeMapApplication.h
 *
 * LICENSE:
 *
 *   Copyright 2008 Ehud Shabtai
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

#ifndef __FREEMAPAPPLICATION_H__
#define __FREEMAPAPPLICATION_H__

// INCLUDES
#include <aknapp.h>
#include "FreeMap.hrh"


// UID for the application;
// this should correspond to the uid defined in the mmp file
const TUid KUidFreeMapApp = { _UID3 };


// CLASS DECLARATION

/**
* CFreeMapApplication application class.
* Provides factory to create concrete document object.
* An instance of CFreeMapApplication is the application part of the
* AVKON application framework for the FreeMap example application.
*/
class CFreeMapApplication : public CAknApplication
	{
	public: // Functions from base classes

		/**
		* From CApaApplication, AppDllUid.
		* @return Application's UID (KUidFreeMapApp).
		*/
		TUid AppDllUid() const;

	protected: // Functions from base classes

		/**
		* From CApaApplication, CreateDocumentL.
		* Creates CFreeMapDocument document object. The returned
		* pointer in not owned by the CFreeMapApplication object.
		* @return A pointer to the created document object.
		*/
		CApaDocument* CreateDocumentL();
	};

#endif // __FREEMAPAPPLICATION_H__

// End of File
