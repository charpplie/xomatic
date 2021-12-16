// ConsoleSCB.cpp: implementation of the CConsoleSCB class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ConsoleSCB.h"
#include "..\PropertiesDialog.h"

static CPropertiesDialog *gPropertiesDlg = 0;

static CConsoleSCB *s_pConsoleWnd = 0;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CConsoleEdit, CEdit)
	ON_WM_GETDLGCODE()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CConsoleButton, CButton)
	ON_CONTROL_REFLECT(BN_CLICKED, &CConsoleButton::OnBnClicked)
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CConsoleSCB, CWnd)
    //{{AFX_MSG_MAP(CConsoleSCB)
    ON_WM_CREATE()
		ON_WM_SIZE()
		ON_WM_DESTROY()
		ON_WM_SETFOCUS()
		ON_EN_SETFOCUS( IDC_EDIT,OnEditSetFocus )
		ON_EN_KILLFOCUS( IDC_EDIT,OnEditKillFocus )
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

static CVarBlock* VarBlockFromConsoleVars()
{
	IConsole *console = GetIEditor()->GetSystem()->GetIConsole();
	std::vector<const char*> cmds;
	cmds.resize( console->GetNumVars() );
	size_t cmdCount = console->GetSortedVars( &cmds[0],cmds.size() );

	CVarBlock *vb = new CVarBlock;
	IVariable* pVariable = 0;
	for (int i = 0; i < cmdCount; i++)
	{
		ICVar *pCVar = console->GetCVar(cmds[i]);
		if (!pCVar)
			continue;
		int varType = pCVar->GetType();

		switch(varType) 
		{
			case CVAR_INT:
				pVariable = new CVariable<int>;
				pVariable->Set( pCVar->GetIVal() );
				break;
			case CVAR_FLOAT:
				pVariable = new CVariable<float>;
				pVariable->Set( pCVar->GetFVal() );
				break;
			case CVAR_STRING:
				pVariable = new CVariable<CString>;
				pVariable->Set( pCVar->GetString() );
				break;
			default: 
				assert(0);
		}

		pVariable->SetDescription(pCVar->GetHelp());
		pVariable->SetName(cmds[i]);

		if (pVariable)
			vb->AddVariable(pVariable);
	}
	return vb;
}

static void OnConsoleVariableUpdated( IVariable *pVar )
{
	if (!pVar)
		return;
	CString varName = pVar->GetName();
	ICVar *pCVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar(varName);
	if (!pCVar)
		return;
	if (pVar->GetType() == IVariable::INT)
	{
		int val;
		pVar->Get(val);
		pCVar->Set(val);
	}
	else if (pVar->GetType() == IVariable::FLOAT)
	{
		float val;
		pVar->Get(val);
		pCVar->Set(val);
	}
	else if (pVar->GetType() == IVariable::STRING)
	{
		CString val;
		pVar->Get(val);
		pCVar->Set(val);
	}
}

//////////////////////////////////////////////////////////////////////////
static CString popup_helper( HWND hwnd,int x,int y )
{
	IConsole *console = GetIEditor()->GetSystem()->GetIConsole();

	TSmartPtr<CVarBlock> vb = VarBlockFromConsoleVars();
	XmlNodeRef node;
	if (!gPropertiesDlg)
		gPropertiesDlg = new CPropertiesDialog( "Console Variables",node,AfxGetMainWnd(), true );
	if (!gPropertiesDlg->m_hWnd)
	{
		gPropertiesDlg->Create( CPropertiesDialog::IDD,AfxGetMainWnd() );
		gPropertiesDlg->SetUpdateCallback( functor(OnConsoleVariableUpdated) );
	}
	gPropertiesDlg->ShowWindow( SW_SHOW );
	gPropertiesDlg->GetPropertyCtrl()->AddVarBlock( vb );

	/*
	HMENU hm = CreatePopupMenu();
	for (int i = 0; i < cmdCount; i++) {
		AppendMenu( hm,MF_STRING,i+1,cmds[i] );
	}
	int res = TrackPopupMenuEx( hm,TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RETURNCMD,x,y,hwnd,NULL );
	if (res > 0) {
		return cmds[res-1];
	}
	*/
	return "";
}

void CConsoleButton::OnBnClicked()
{
	CPoint pnt;
	GetCursorPos(&pnt);
	CString str = popup_helper( GetSafeHwnd(),pnt.x,pnt.y );
	if (!str.IsEmpty())
	{
		SetWindowText( str+" " );
	}	
}

UINT CConsoleEdit::OnGetDlgCode()
{
	UINT code = CEdit::OnGetDlgCode();
	code |= DLGC_WANTMESSAGE; 
	return code; 
}

BOOL CConsoleEdit::PreTranslateMessage(MSG* msg)
{
	if (msg->message == WM_LBUTTONDBLCLK || msg->message == WM_RBUTTONDOWN)
	{
		CPoint pnt;
		GetCursorPos(&pnt);
		CString str = popup_helper( GetSafeHwnd(),pnt.x,pnt.y );
		if (!str.IsEmpty())
		{
			SetWindowText( str+" " );
		}
	}
	else if (msg->message == WM_CHAR)
	{
		switch (msg->wParam)
		{
		case '~':
		case '`':
			// disable log.
			GetIEditor()->ShowConsole( false );
			SetWindowText( "" );
			break;
		}
	}	else if (msg->message == WM_KEYDOWN)
	{
		IConsole *console = GetIEditor()->GetSystem()->GetIConsole();

		if(msg->wParam!=VK_TAB)
			console->ResetAutoCompletion();

		switch (msg->wParam)
		{
		case VK_RETURN:
			{
				CString str;
				GetWindowText( str );
				// Execute this string as command.
				if (!str.IsEmpty())
				{
					CLogFile::WriteLine( str );
					GetIEditor()->GetSystem()->GetIConsole()->ExecuteString( str );
					m_history.erase( std::remove(m_history.begin(),m_history.end(),str),m_history.end() );
					m_history.push_back(str);
				}
				SetWindowText( "" );
				return 0;
			}
			return TRUE;
			break;

		case VK_TAB:
			{
				GetAsyncKeyState(VK_CONTROL);
				bool bCtrl = GetAsyncKeyState(VK_CONTROL) != 0;
			
				CString inputStr,newStr;
				GetWindowText( inputStr );
				inputStr = inputStr.SpanExcluding( " " );

				if (!bCtrl)
				{
					newStr = console->ProcessCompletion(inputStr);
					newStr = console->AutoComplete( inputStr );
				}
				else
				{
					newStr = console->AutoCompletePrev( inputStr );
				}

				if (!newStr.IsEmpty()) 
				{
					newStr += " ";
					SetWindowText( newStr );
				}
				CString str;
				GetWindowText( str );
				SetSel( str.GetLength(),str.GetLength() );
				return TRUE;
			}
			return TRUE;
			break;

		case VK_UP:
		case VK_DOWN:
			{
				CString str;
				CMenu menu;
				menu.CreatePopupMenu();
				for (int i = 0; i < m_history.size(); i++) {
					menu.AppendMenu( MF_STRING,i+1,m_history[i] );
				}
				CPoint pnt;
				GetCursorPos(&pnt);
				int res = ::TrackPopupMenuEx( menu.GetSafeHmenu(),TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RETURNCMD,pnt.x,pnt.y,GetSafeHwnd(),NULL );
				if (res > 0) {
					str = m_history[res-1];
				}
				if (!str.IsEmpty())
				{
					SetWindowText( str );
					SetSel( str.GetLength(),str.GetLength() );
				}
			}
			return TRUE;

		case VK_ESCAPE:
			// disable log.
			GetIEditor()->ShowConsole( false );
			return TRUE;
			break;
		}
	}

	return CEdit::PreTranslateMessage(msg);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CConsoleSCB* CConsoleSCB::GetCreatedInstance()
{
	return s_pConsoleWnd;
}

CConsoleSCB::CConsoleSCB()
{
	s_pConsoleWnd = this;
}

CConsoleSCB::~CConsoleSCB()
{
	s_pConsoleWnd =0;
	if (gPropertiesDlg)
		delete gPropertiesDlg;
	gPropertiesDlg = 0;
}

int CConsoleSCB::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	////////////////////////////////////////////////////////////////////////
	// Create the edit control and register it
	////////////////////////////////////////////////////////////////////////

	RECT rcEdit;
	CFont cListBoxFont;

	if (CWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

	AfxInitRichEdit2();

	/*
	// Create the edit control
	VERIFY(m_cListBox.Create(WS_CHILD | WS_VISIBLE | WS_VSCROLL | 
		LBS_NOSEL, rcEdit, this, NULL));
	m_cListBox.ShowWindow( SW_HIDE );
	*/
	
	m_dialog.Create( CConsoleSCB_InternalDialog::IDD,this );
	m_dialog.ShowWindow(SW_SHOW);

	if (GetIEditor()->IsInConsolewMode())
	{
		m_edit.Create( WS_CHILD|WS_VISIBLE|ES_MULTILINE|WS_VSCROLL|ES_AUTOVSCROLL|ES_READONLY, rcEdit, &m_dialog, NULL );
		m_edit.SetFont( CFont::FromHandle( (HFONT)gSettings.gui.hSystemFont) );
		m_edit.ModifyStyleEx( 0,WS_EX_STATICEDGE );
		m_edit.SetLimitText(0);
		// Attach / register edit box
		CLogFile::AttachEditBox(m_edit.GetSafeHwnd());
	}
	else
	{
		m_richEdit.Create( WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_AUTOVSCROLL|ES_MULTILINE, rcEdit, &m_dialog, NULL );
		m_richEdit.SetFont( CFont::FromHandle( (HFONT)gSettings.gui.hSystemFont) );
		m_richEdit.ModifyStyleEx( 0,WS_EX_STATICEDGE );
		UpdateWindow();
		//m_richEdit.SetLimitText(0);
	}
	
	m_input.Create( WS_CHILD|WS_VISIBLE|ES_WANTRETURN|ES_AUTOHSCROLL|WS_TABSTOP, rcEdit, &m_dialog, IDC_EDIT );
	m_input.SetFont( CFont::FromHandle( (HFONT)::GetStockObject(SYSTEM_FONT)) );
	m_input.ModifyStyleEx( 0,WS_EX_STATICEDGE );
	m_input.SetWindowText( "" );

	m_button.Create("...",WS_CHILD|WS_VISIBLE, rcEdit,&m_dialog, IDC_BUTTON1);
	m_button.SetFont( CFont::FromHandle( (HFONT)::GetStockObject(SYSTEM_FONT)) );

	return 0;
}

void CConsoleSCB::OnSize(UINT nType, int cx, int cy)
{
	////////////////////////////////////////////////////////////////////////
	// Resize the edit control
	////////////////////////////////////////////////////////////////////////

	RECT rcEdit;

	CWnd::OnSize(nType, cx, cy);

	// Get the size of the client window
	GetClientRect(&rcEdit);
/*
	// Set the position of the listbox
	m_cListBox.SetWindowPos(NULL, rcEdit.left + 3, rcEdit.top + 3, rcEdit.right - 6, 
		rcEdit.bottom - 6, SWP_NOZORDER);
		*/
	int inputH = 18;
	int btnWidth = 30;

	m_dialog.MoveWindow( rcEdit.left,rcEdit.top,rcEdit.right,rcEdit.bottom );

	if (m_edit.GetSafeHwnd())
		m_edit.SetWindowPos(NULL, rcEdit.left, rcEdit.top + 2, rcEdit.right, rcEdit.bottom - 1 - inputH, SWP_NOZORDER);

	if (m_richEdit.GetSafeHwnd())
		m_richEdit.SetWindowPos(NULL, rcEdit.left, rcEdit.top + 2, rcEdit.right, rcEdit.bottom - 1 - inputH, SWP_NOZORDER);


	m_input.SetWindowPos(NULL, rcEdit.left, rcEdit.bottom-inputH, rcEdit.right - btnWidth, rcEdit.bottom - 2, SWP_NOZORDER);
	m_button.SetWindowPos(NULL, rcEdit.right - btnWidth, rcEdit.bottom-inputH, btnWidth, inputH , SWP_NOZORDER);
}

void CConsoleSCB::OnDestroy()
{
	////////////////////////////////////////////////////////////////////////
	// Unregister the edit control
	////////////////////////////////////////////////////////////////////////
	CLogFile::AttachEditBox(NULL);
}

void CConsoleSCB::OnSetFocus( CWnd* pOldWnd )
{
	m_input.SetFocus();
}

BOOL CConsoleSCB::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	return TRUE;
}

void CConsoleSCB::OnEditSetFocus()
{
	// Disable accelerators when Edit gets focus.
	GetIEditor()->EnableAcceleratos( false );
}
	
void CConsoleSCB::OnEditKillFocus()
{
	// Enable accelerators when Edit loose focus.
	GetIEditor()->EnableAcceleratos( true );
}

void CConsoleSCB::SetInputFocus()
{
	m_input.SetFocus();
	m_input.SetWindowText( "" );
}

static CString CopyAndRemoveColorCode( const char *sText,int &iColorCode )
{
	CString ret=sText;		// alloc and copy

	char *s,*d;

	s=ret.GetBuffer();d=s;

	// remove color code in place
	while(*s!=0)
	{
		if(*s=='$' && isdigit(*(s+1)))
		{
			if (iColorCode == 0)
				iColorCode = *(s+1) - '0';
			s+=2;
			continue;
		}
		if (*s == '\r' || *s == '\n')
		{
			s++;
			continue;
		}

		*d++=*s++;
	}

	ret.ReleaseBuffer(d-ret.GetBuffer());

	return ret;
}

//////////////////////////////////////////////////////////////////////////
void CConsoleSCB::AddToConsole( const char *sText,bool bNewLine )
{
	HWND hWndRichEdit = m_richEdit.GetSafeHwnd();
	if (!hWndRichEdit)
		return;

	uint32 m_vColorTable[10];

	m_vColorTable[0]=RGB(0,0,0);
	m_vColorTable[1]=RGB(255,255,255);
	m_vColorTable[2]=RGB(0,0,200);// blue
	m_vColorTable[3]=RGB(0,200,0);// green
	m_vColorTable[4]=RGB(200,0,0);// red
	m_vColorTable[5]=RGB(0,200,200);// cyan
	m_vColorTable[6]=RGB(200,180,0);// yellow
	m_vColorTable[7]=RGB(200,0,200);// red+blue
	m_vColorTable[8]=0x000080ff;
	m_vColorTable[9]=0x008f8f8f;

	int iColor = 0;
	CString tempStr = CopyAndRemoveColorCode(sText,iColor);
	if (iColor < 0 || iColor > 9)
		iColor = 0;

	//tempStr = sText;

	// remember selection and the top row
	int len = ::GetWindowTextLength( hWndRichEdit );
	int top, from, to;
	::SendMessage( hWndRichEdit, EM_GETSEL, (WPARAM)&from, (LPARAM)&to );
	bool keepPos = false;
	bool locking = false;

	if ( from!=len || to!=len )
	{
		keepPos = ::GetFocus() == hWndRichEdit;
		if ( keepPos )
		{
			top = ::SendMessage( hWndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0 );
			locking = bNewLine && ::IsWindowVisible( hWndRichEdit );
			if ( locking )
				m_richEdit.LockWindowUpdate();
		}
		::SendMessage( hWndRichEdit, EM_SETSEL, len, len );
	}
	if ( bNewLine )
	{
		//str = CString("\r\n") + str.TrimLeft();
		//str = CString("\r\n") + str;
		//str = CString("\r") + str;
		tempStr.TrimRight();
		tempStr = CString("\r\n") + tempStr;
	}

	CHARFORMAT2 chrfmt;
	// set text color
	chrfmt.cbSize = sizeof(chrfmt);
	chrfmt.dwMask = CFM_COLOR|CFM_BOLD;
	chrfmt.dwEffects = 0;
	chrfmt.crTextColor = m_vColorTable[iColor];
	if (chrfmt.crTextColor != 0)
	{
		chrfmt.dwEffects |= CFE_BOLD;
	}
	m_richEdit.SetSelectionCharFormat( chrfmt );

	const char *szStr = tempStr;
	::SendMessage( hWndRichEdit, EM_REPLACESEL, FALSE, (LPARAM)szStr );

	// restore selection and the top line
	if ( keepPos )
	{
		::SendMessage( hWndRichEdit, EM_SETSEL, from, to );
		top -= ::SendMessage( hWndRichEdit, EM_GETFIRSTVISIBLELINE, 0, 0 );
		::SendMessage( hWndRichEdit, EM_LINESCROLL, 0, (LPARAM)top );
	}
	else
	{
		// Scroll to bottom
		::SendMessage(hWndRichEdit,WM_VSCROLL, /*SB_LINEDOWN*/ SB_BOTTOM, (LPARAM)NULL);
	}

	if ( locking )
		m_richEdit.UnlockWindowUpdate();
}


void CConsoleSCB::UpdateWindow()
{
	if (!GetIEditor()->IsInConsolewMode() && CConsoleSCB::GetCreatedInstance())
	{
		COLORREF colorBG = RGB(GetRValue(gSettings.colorConsoleBG), GetGValue(gSettings.colorConsoleBG), GetBValue(gSettings.colorConsoleBG));
		CConsoleSCB::GetCreatedInstance()->m_richEdit.SetBackgroundColor(FALSE, colorBG);
	}
}