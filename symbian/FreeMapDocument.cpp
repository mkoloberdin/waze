/* FreeMapDocument.cpp
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


// INCLUDE FILES
#include "FreeMapAppUi.h"
#include "FreeMapDocument.h"

// ============================ MEMBER FUNCTIONS ===============================

// -----------------------------------------------------------------------------
// CFreeMapDocument::NewL()
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CFreeMapDocument* CFreeMapDocument::NewL( CEikApplication& aApp )
	{
	CFreeMapDocument* self = NewLC( aApp );
	CleanupStack::Pop( self );
	return self;
	}

// -----------------------------------------------------------------------------
// CFreeMapDocument::NewLC()
// Two-phased constructor.
// -----------------------------------------------------------------------------
//
CFreeMapDocument* CFreeMapDocument::NewLC( CEikApplication& aApp )
	{
	CFreeMapDocument* self =
		new ( ELeave ) CFreeMapDocument( aApp );

	CleanupStack::PushL( self );
	self->ConstructL();
	return self;
	}

// -----------------------------------------------------------------------------
// CFreeMapDocument::ConstructL()
// Symbian 2nd phase constructor can leave.
// -----------------------------------------------------------------------------
//
void CFreeMapDocument::ConstructL()
	{
	// No implementation required
	}

// -----------------------------------------------------------------------------
// CFreeMapDocument::CFreeMapDocument()
// C++ default constructor can NOT contain any code, that might leave.
// -----------------------------------------------------------------------------
//
CFreeMapDocument::CFreeMapDocument( CEikApplication& aApp )
	: CAknDocument( aApp )
	{
	// No implementation required
	}

// ---------------------------------------------------------------------------
// CFreeMapDocument::~CFreeMapDocument()
// Destructor.
// ---------------------------------------------------------------------------
//
CFreeMapDocument::~CFreeMapDocument()
	{
	// No implementation required
	}

// ---------------------------------------------------------------------------
// CFreeMapDocument::CreateAppUiL()
// Constructs CreateAppUi.
// ---------------------------------------------------------------------------
//
CEikAppUi* CFreeMapDocument::CreateAppUiL()
	{
	// Create the application user interface, and return a pointer to it;
	// the framework takes ownership of this object
	return ( static_cast <CEikAppUi*> ( new ( ELeave )
										CFreeMapAppUi ) );
	}

// End of File
