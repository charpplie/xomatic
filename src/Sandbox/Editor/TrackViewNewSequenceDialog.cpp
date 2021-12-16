#include "StdAfx.h"
#include "TrackViewNewSequenceDialog.h"
#include "TrackView/TrackViewSequenceManager.h"

class CPanelDisplayLayer;

// TrackViewNewSequenceDialog dialog
IMPLEMENT_DYNAMIC(CTVNewSequenceDialog, CDialog)
CTVNewSequenceDialog::CTVNewSequenceDialog(CWnd* pParent /* = NULL */ ) : CDialog(CTVNewSequenceDialog::IDD, pParent)
{
}

CTVNewSequenceDialog::~CTVNewSequenceDialog()
{
}

void CTVNewSequenceDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_NAME, m_nameEdit);
	DDX_Control(pDX, IDC_TV_CREATE_NEW_LAYER_EDIT, m_newLayerEdit);
	DDX_Control(pDX, IDC_TV_CREATE_NEW_LAYER_CHKBOX, m_createNewLayerChk);
}


BEGIN_MESSAGE_MAP(CTVNewSequenceDialog, CDialog)
	ON_BN_CLICKED(IDC_TV_CREATE_NEW_LAYER_CHKBOX, OnAddNewLayerCheckbox)

END_MESSAGE_MAP()


// CTVNewSequenceDialog message handlers

BOOL CTVNewSequenceDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_newLayerEdit.EnableWindow(false);
	m_createNewLayerChk.SetCheck(false);

	return TRUE;
}


void CTVNewSequenceDialog::OnOK()
{
	m_nameEdit.GetWindowText( m_sequenceName );

	if (m_sequenceName.IsEmpty())
	{
		AfxMessageBox( "A sequence name cannot be empty!",MB_OK|MB_ICONEXCLAMATION );
		return;
	}
	else if (m_sequenceName.Find('/') != -1)
	{
		AfxMessageBox( "A sequence name cannot contain a '/' character!",MB_OK|MB_ICONEXCLAMATION );
		return;
	}

	for(int k=0; k<GetIEditor()->GetSequenceManager()->GetCount(); ++k)
	{
		CTrackViewSequence *pSequence = GetIEditor()->GetSequenceManager()->GetSequenceByIndex(k);
		CString fullname = pSequence->GetName();

		if ( fullname.CompareNoCase(m_sequenceName) == 0)
		{
			AfxMessageBox( "Sequence with this name already exists!",MB_OK|MB_ICONEXCLAMATION );
			return;
		}
	}

	if (m_createNewLayerChk.GetCheck())
	{
		m_newLayerEdit.GetWindowText(m_layerName);
	}

	CDialog::OnOK();
}

void CTVNewSequenceDialog::OnAddNewLayerCheckbox()
{
	m_newLayerEdit.EnableWindow(m_createNewLayerChk.GetCheck());
}