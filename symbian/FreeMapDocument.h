/* FreeMapDocument.h
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


#ifndef __FREEMAPDOCUMENT_h__
#define __FREEMAPDOCUMENT_h__

// INCLUDES
#include <akndoc.h>

// FORWARD DECLARATIONS
class CFreeMapAppUi;
class CEikApplication;


// CLASS DECLARATION

/**
* CFreeMapDocument application class.
* An instance of class CFreeMapDocument is the Document part of the
* AVKON application framework for the FreeMap example application.
*/
class CFreeMapDocument : public CAknDocument
	{
	public: // Constructors and destructor

		/**
		* NewL.
		* Two-phased constructor.
		* Construct a CFreeMapDocument for the AVKON application aApp
		* using two phase construction, and return a pointer
		* to the created object.
		* @param aApp Application creating this document.
		* @return A pointer to the created instance of CFreeMapDocument.
		*/
		static CFreeMapDocument* NewL( CEikApplication& aApp );

		/**
		* NewLC.
		* Two-phased constructor.
		* Construct a CFreeMapDocument for the AVKON application aApp
		* using two phase construction, and return a pointer
		* to the created object.
		* @param aApp Application creating this document.
		* @return A pointer to the created instance of CFreeMapDocument.
		*/
		static CFreeMapDocument* NewLC( CEikApplication& aApp );

		/**
		* ~CFreeMapDocument
		* Virtual Destructor.
		*/
		virtual ~CFreeMapDocument();

	public: // Functions from base classes

		/**
		* CreateAppUiL
		* From CEikDocument, CreateAppUiL.
		* Create a CFreeMapAppUi object and return a pointer to it.
		* The object returned is owned by the Uikon framework.
		* @return Pointer to created instance of AppUi.
		*/
		CEikAppUi* CreateAppUiL();

	private: // Constructors

		/**
		* ConstructL
		* 2nd phase constructor.
		*/
		void ConstructL();

		/**
		* CFreeMapDocument.
		* C++ default constructor.
		* @param aApp Application creating this document.
		*/
		CFreeMapDocument( CEikApplication& aApp );

	};

#endif // __FREEMAPDOCUMENT_h__

// End of File
