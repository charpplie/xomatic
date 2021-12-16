//////////////////////////////////  CRYTEK  ////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2011.
// -------------------------------------------------------------------------
//  File Name        : ScriptTermDialog.cpp
//  Author           : Jaewon Jung
//  Time of creation : 4/19/2011   15:16
//  Compilers        : VS2008
//  Description      : Dialog for python script terminal
// -------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ScriptTermDialog.h"
#include "ScriptHelpDialog.h"
#include "Util/BoostPythonHelpers.h"

IMPLEMENT_DYNCREATE(CScriptTermDialog, CXTResizeDialog)

class CScriptTerminalViewClass : public TRefCountBase<IViewPaneClass>
{
	//////////////////////////////////////////////////////////////////////////
	// IClassDesc
	//////////////////////////////////////////////////////////////////////////
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
	virtual REFGUID ClassID()
	{
		// {403A9051-20FA-4140-B297-2B4A2AAFB9E5}
		static const GUID guid = 
		{ 0x403a9051, 0x20fa, 0x4140, { 0xb2, 0x97, 0x2b, 0x4a, 0x2a, 0xaf, 0xb9, 0xe5 } };

		return guid;
	}
	virtual const char* ClassName() { return SCRIPT_TERM_WINDOW_NAME; };
	virtual const char* Category() { return "Editor"; };
	//////////////////////////////////////////////////////////////////////////
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CScriptTermDialog); };
	virtual const char* GetPaneTitle() { return SCRIPT_TERM_WINDOW_NAME; };
	virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; };
	virtual CRect GetPaneRect() { return CRect(0,0,600,200); };
	virtual bool SinglePane() { return true; };
	virtual bool WantIdleUpdate() { return true; };
};

REGISTER_CLASS_DESC(CScriptTerminalViewClass)

BEGIN_MESSAGE_MAP(CScriptTermDialog, CXTResizeDialog)
	ON_BN_CLICKED(IDC_SCRIPT_HELP, OnScriptHelp)
END_MESSAGE_MAP()

CScriptTermDialog::CScriptTermDialog(CWnd* pParent)
	: CXTResizeDialog(CScriptTermDialog::IDD, pParent)
{
	Create(IDD, pParent);
	PyScript::RegisterListener(this);
}

CScriptTermDialog::~CScriptTermDialog()
{
	PyScript::RemoveListener(this);
}

void CScriptTermDialog::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SCRIPT_INPUT, m_inputCtrl);
	DDX_Control(pDX, IDC_SCRIPT_OUTPUT, m_outputCtrl);
}

BOOL CScriptTermDialog::OnInitDialog()
{
	__super::OnInitDialog();

	m_inputCtrl.Init();
	m_inputCtrl.SetMode(_MODE_STANDARD_|_MODE_HISTORY_|_MODE_FIND_ALL_);

	// Add module names to the auto-completion list.
	const CAutoRegisterPythonModuleHelper::ModuleList modules
		= CAutoRegisterPythonModuleHelper::s_modules;
	for(size_t i=0; i<modules.size(); ++i)
	{
		m_inputCtrl.AddSearchString(modules[i].name.c_str());
	}

	// Add full command names to the auto-completion list.
	CAutoRegisterPythonCommandHelper *pCurrent = CAutoRegisterPythonCommandHelper::s_pFirst;
	while(pCurrent)
	{
		CString command = pCurrent->m_name.c_str(); 
		CString fullCmd = CAutoRegisterPythonModuleHelper::s_modules[pCurrent->m_moduleIndex].name.c_str();
		fullCmd += ".";
		fullCmd += command;
		fullCmd += "()";

		m_inputCtrl.AddSearchString(fullCmd);

		pCurrent = pCurrent->m_pNext;
	}

	SetResize(IDC_SCRIPT_OUTPUT, SZ_RESIZE(1));
	SetResize(IDC_SCRIPT_INPUT, CXTResizeRect(0,1,1,1));
	SetResize(IDC_SCRIPT_HELP, SZ_REPOS(1));

	if(m_outputFont.CreateFont(
		14,                        // nHeight
		0,                         // nWidth
		0,                         // nEscapement
		0,                         // nOrientation
		FW_NORMAL,                 // nWeight
		FALSE,                     // bItalic
		FALSE,                     // bUnderline
		0,                         // cStrikeOut
		ANSI_CHARSET,              // nCharSet
		OUT_DEFAULT_PRECIS,        // nOutPrecision
		CLIP_DEFAULT_PRECIS,       // nClipPrecision
		DEFAULT_QUALITY,           // nQuality
		DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
		"Consolas") != 0)
	{
		m_outputCtrl.SetFont( &m_outputFont, true );
	}
	else if(m_outputFont.CreateFont(
		14,                        // nHeight
		0,                         // nWidth
		0,                         // nEscapement
		0,                         // nOrientation
		FW_NORMAL,                 // nWeight
		FALSE,                     // bItalic
		FALSE,                     // bUnderline
		0,                         // cStrikeOut
		ANSI_CHARSET,              // nCharSet
		OUT_DEFAULT_PRECIS,        // nOutPrecision
		CLIP_DEFAULT_PRECIS,       // nClipPrecision
		DEFAULT_QUALITY,           // nQuality
		DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
		"Courier New") != 0)
	{
		m_outputCtrl.SetFont( &m_outputFont, true );
	}

	return TRUE;
}

void CScriptTermDialog::OnOK()
{
	CString command, command2;
	// Echo the command.
	m_inputCtrl.GetWindowText(command);
	command2 = "] ";
	command2 += command;
	command2 += "\r\n";
	AppendText(command2);
	
	// Add the command to the history.
	m_inputCtrl.AddHistoryString(command);

	ExecuteAndPrint(command);
	m_inputCtrl.SetWindowText("");
}

void CScriptTermDialog::OnCancel()
{
	// Do nothing; do not close the dialog when 'ESC' pressed, for instance.
}

void CScriptTermDialog::OnScriptHelp()
{
	CScriptHelpDialog::GetInstance().ShowWindow(SW_SHOW);
}

void CScriptTermDialog::PostNcDestroy()
{
	__super::PostNcDestroy();
	delete this;
}

void CScriptTermDialog::ExecuteAndPrint(const char *cmd)
{
	// Execute the given script command.
	PyRun_SimpleString(cmd);

	PyErr_Print(); // Make python print any errors.
}

void CScriptTermDialog::AppendText(const char *pText)
{
	AppendToConsole(pText, RGB(0, 0, 0));
}

void CScriptTermDialog::AppendError( const char *pText )
{
	AppendToConsole(pText, RGB(150, 0, 0));
}

void CScriptTermDialog::AppendToConsole(CString text, COLORREF color)
{
	CHARFORMAT charFormat;
	// Initialize character format structure
	charFormat.cbSize = sizeof(CHARFORMAT);
	charFormat.dwMask = CFM_COLOR;
	charFormat.dwEffects = 0; // To disable CFE_AUTOCOLOR
	charFormat.crTextColor = color;

	// Get the initial text length.
	const int length = m_outputCtrl.GetWindowTextLength();
	// Put the selection at the end of text.
	m_outputCtrl.SetSel(length, -1);

	//  Set the character format
	m_outputCtrl.SetSelectionCharFormat(charFormat);

	// Replace the selection.
	m_outputCtrl.ReplaceSel(text);

	const int visibleLines = GetNumVisibleLines();
	if (&m_outputCtrl != m_outputCtrl.GetFocus())
	{
		m_outputCtrl.LineScroll(INT_MAX);
		m_outputCtrl.LineScroll(1 - visibleLines);
	}
}

int CScriptTermDialog::GetNumVisibleLines()
{
	CRect rect;
	long firstChar, lastChar;
	long firstLine, lastLine;

	// Get client rect of rich edit control
	m_outputCtrl.GetClientRect(rect);

	// Get character index close to upper left corner
	firstChar = m_outputCtrl.CharFromPos(CPoint(0, 0));

	// Get character index close to lower right corner
	lastChar = m_outputCtrl.CharFromPos(CPoint(rect.right, rect.bottom));
	if (lastChar < 0)
	{
		lastChar = m_outputCtrl.GetTextLength();
	}

	// Convert to lines
	firstLine = m_outputCtrl.LineFromChar(firstChar);
	lastLine  = m_outputCtrl.LineFromChar(lastChar);

	return (lastLine - firstLine);
}