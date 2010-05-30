/*
============================================================================
 Name		: RoadmapQueryDialog.h
 Author	  	: AGA
 Copyright  : GPL
 Description: Query dialog class customization
============================================================================
*/
#include <aknquerydialog.h>
#include <coeinput.h>
// Extension of the query dialog class
class CRoadMapQueryDialog : public CAknTextQueryDialog
{
public:
	CRoadMapQueryDialog (TDes &aDataText, const TTone &aTone=ENoTone) :
	CAknTextQueryDialog(aDataText, aTone), iLeftSoftKeyVisible( ETrue ),
	m_InputCapabilities( TCoeInputCapabilities::EAllText|TCoeInputCapabilities::ENavigation ) {}

	static CRoadMapQueryDialog* NewL(TDes& aDataText, const TTone& aTone = ENoTone)
	{
		CRoadMapQueryDialog* self = new (ELeave) CRoadMapQueryDialog(aDataText, aTone);
		return self;
	}

	virtual void UpdateLeftSoftKeyL ()
	{
		TBool leftSoftKeyVisible = ( iLeftSoftKeyVisible || CheckIfEntryTextOk() );
		MakeLeftSoftkeyVisible ( leftSoftKeyVisible );
	}

	void SetLeftSoftKeyVisible( TBool aVisible )
	{
		iLeftSoftKeyVisible = aVisible;
	}

	virtual TCoeInputCapabilities InputCapabilities() const
	{
		TCoeInputCapabilities caps( CAknTextQueryDialog::InputCapabilities() );
		caps.SetCapabilities( caps.Capabilities() | m_InputCapabilities );
		return caps;
	}
	void SetInputCapabilities( TUint aCapabilities )
	{
		m_InputCapabilities = aCapabilities;
	}

protected:
	TBool iLeftSoftKeyVisible;
	TUint m_InputCapabilities;
};
