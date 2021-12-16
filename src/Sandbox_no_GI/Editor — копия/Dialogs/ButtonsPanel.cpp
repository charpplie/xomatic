// MainTools.cpp : implementation file
//

#include "stdafx.h"
#include "ButtonsPanel.h"
#include "ObjectCreateTool.h"
#include "Controls/ToolButton.h"

/////////////////////////////////////////////////////////////////////////////
// CButtonsPanel dialog
CButtonsPanel::CButtonsPanel()
{
}

CButtonsPanel::~CButtonsPanel()
{
	ReleaseGuiButtons();
}

BEGIN_MESSAGE_MAP(CButtonsPanel, CXTResizeDialog)
	ON_WM_DESTROY()
	ON_WM_SIZE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CButtonsPanel message handlers

BOOL CButtonsPanel::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	// Route the commands to the view
	if (nID == ID_INSERT_ENTITY && AfxGetMainWnd())
	{
		AfxGetMainWnd()->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	}
	//((CFrameWnd *) (AfxGetMainWnd()))->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
	
	return __super::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

//////////////////////////////////////////////////////////////////////////
void CButtonsPanel::AddButton( const SButtonInfo &button )
{
	SButton b;
	b.info = button;
	m_buttons.push_back(b);
}
//////////////////////////////////////////////////////////////////////////
void CButtonsPanel::AddButton( CString name,CString toolClass )
{
	SButtonInfo bi;
	bi.name = name;
	bi.toolClassName = toolClass;
	AddButton(bi);
}
//////////////////////////////////////////////////////////////////////////
void CButtonsPanel::AddButton( CString name,CRuntimeClass *pToolClass )
{
	SButtonInfo bi;
	bi.name = name;
	bi.pToolClass = pToolClass;
	AddButton(bi);
}
//////////////////////////////////////////////////////////////////////////
void CButtonsPanel::ClearButtons()
{
	ReleaseGuiButtons();
	m_buttons.clear();
}

void CButtonsPanel::UncheckAll()
{
	for (int i = 0; i < m_buttons.size(); i++)
	{
		m_buttons[i].pButton->SetChecked( FALSE );
	}
}

BOOL CButtonsPanel::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	// TODO: Add your specialized code here and/or call the base class
	if (HIWORD(wParam) == BN_CLICKED)
	{
		int ctrlId = LOWORD(wParam);

		if (ctrlId >= 0 && ctrlId < (int)m_buttons.size()+1)
		{
			OnButtonPressed( m_buttons[ctrlId].info );
		}
	}
	
	return __super::OnCommand(wParam, lParam);
}

BOOL CButtonsPanel::OnInitDialog() 
{
	__super::OnInitDialog();

	// Create Buttons.
	for (int i = 0; i < m_buttons.size(); i++)
	{
		SButton &b = m_buttons[i];

		b.pButton = new CToolButton;
		b.pButton->Create( b.info.name,WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX | BS_PUSHLIKE,CRect(0,0,1,1),this, i+1 );
		b.pButton->SetFont( CFont::FromHandle( (HFONT)gSettings.gui.hSystemFont) );
		if (b.info.pToolClass)
		{
			b.pButton->SetToolClass( b.info.pToolClass,b.info.toolUserDataKey,(void*)(const char*)b.info.toolUserData );
		}
		else if (!b.info.toolClassName.IsEmpty())
		{
			b.pButton->SetToolName( b.info.toolClassName,b.info.toolUserDataKey,(void*)(const char*)b.info.toolUserData );
		}
	}
	
	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

//////////////////////////////////////////////////////////////////////////
void CButtonsPanel::RecalcButtonsLayout()
{
	int leftButtonsOffset = 5;
	int xOffset = 5;
	int yOffset = 4;

	int ySeprator = 4;

	int m_buttonHeight = 18;

	if (gSettings.gui.bWindowsVista)
	{
		m_buttonHeight = 20;
		ySeprator = 2;
	}

	int row = 0;
	int col = 0;

	CRect rc;
	GetClientRect( rc );

	int buttonWidth = ((rc.right - rc.left) - 3*xOffset)/2;

	int newHeight = 0;

	// Get class categories.
	for (int i = 0; i < m_buttons.size(); i++)
	{
		SButton &b = m_buttons[i];

		CRect brc;

		brc.left = leftButtonsOffset + col*(buttonWidth + xOffset);
		brc.right = brc.left + buttonWidth;

		brc.top = yOffset + row*(m_buttonHeight + ySeprator);
		brc.bottom = brc.top + m_buttonHeight;

		newHeight = brc.bottom + ySeprator;

		if (b.pButton)
			b.pButton->MoveWindow( brc,FALSE );

		col++;
		brc.left = leftButtonsOffset + col*(buttonWidth + xOffset);
		brc.right = brc.left + buttonWidth;
		if (brc.right > rc.right-4)
		{
			col = 0;
			row++;
		}
	}

	SetWindowPos( NULL,0,0,rc.right-rc.left,newHeight+4,SWP_NOMOVE );
}

//////////////////////////////////////////////////////////////////////////
void CButtonsPanel::ReleaseGuiButtons()
{
	for (int i = 0; i < m_buttons.size(); i++)
	{
		delete m_buttons[i].pButton;
		m_buttons[i].pButton = 0;
	}
}

void CButtonsPanel::OnDestroy() 
{
	__super::OnDestroy();
}

void CButtonsPanel::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	if (GetSafeHwnd())
	{
		RecalcButtonsLayout();
	}
}
