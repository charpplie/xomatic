// VisualLogWnd.cpp : implementation file
//

#include "stdafx.h"
#include <cctype>
#include "Resource.h"
#include "VisualLogWnd.h"

#define IDW_SW_TEXT_PANE			AFX_IDW_CONTROLBAR_FIRST+10
#define IDW_SW_CONTROLS_PANE	AFX_IDW_CONTROLBAR_FIRST+11

namespace
{
	const int VisualLogLayoutVersion = 0x0001; // bump this up on every substantial pane layout change
}





/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Class for registering the Visual Log Viewer with the the view menu
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CVisualLogViewClass : public TRefCountBase<IViewPaneClass>
{
	//////////////////////////////////////////////////////////////////////////
	// IClassDesc
	//////////////////////////////////////////////////////////////////////////
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; };
	virtual REFGUID ClassID()
	{
		// {585596D7-0DC8-4770-A365-18CF44E35F4D}
		static const GUID guid = {0x585596D7, 0x0DC8, 0x4770, { 0xA3, 0x65, 0x18, 0xCF, 0x44, 0xE3, 0x5F, 0x4D } };

		return guid;
	}
	virtual const char* ClassName() { return "Visual Log Viewer"; };
	virtual const char* Category() { return "Visual Log Viewer"; };
	//////////////////////////////////////////////////////////////////////////
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CVisualLogWnd); };
	virtual const char* GetPaneTitle() { return _T("Visual Log Viewer"); };
	virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; };
	virtual CRect GetPaneRect() { return CRect(0,0,500,500); };
	virtual bool SinglePane() { return true; };
	virtual bool WantIdleUpdate() { return true; };
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define VISUALLOGWND_CLASSNAME "VisualLogViewerWindow"




/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVisualLogWnd class
IMPLEMENT_DYNCREATE(CVisualLogWnd, CXTPFrameWnd)

BEGIN_MESSAGE_MAP(CVisualLogWnd, CXTPFrameWnd)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_DESTROY()

	ON_MESSAGE(XTPWM_DOCKINGPANE_NOTIFY, OnDockingPaneNotify)
	ON_MESSAGE(UWM_BUTTON_CLICKED, OnDialogNotify)
END_MESSAGE_MAP()



// CVisualLogWnd static functions & data
void CVisualLogWnd::RegisterViewClass()
{
	GetIEditor()->GetClassFactory()->RegisterClass( new CVisualLogViewClass );
}



// CVisualLogWnd construction & destruction
CVisualLogWnd::CVisualLogWnd()
{
	// Allocate the common data structure (initializes values in its constructor)
	m_pCD = new SVLogCommonData;

	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();
	if (!(::GetClassInfo(hInst, VISUALLOGWND_CLASSNAME, &wndcls)))
	{
		// otherwise we need to register a new class
		wndcls.style            = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wndcls.lpfnWndProc      = ::DefWindowProc;
		wndcls.cbClsExtra       = wndcls.cbWndExtra = 0;
		wndcls.hInstance        = hInst;
		wndcls.hIcon            = NULL;
		wndcls.hCursor          = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		wndcls.hbrBackground    = (HBRUSH) (COLOR_3DFACE + 1);
		wndcls.lpszMenuName     = NULL;
		wndcls.lpszClassName    = VISUALLOGWND_CLASSNAME;
		if (!AfxRegisterClass(&wndcls))
		{
			AfxThrowResourceException();
		}
	}

	m_pView					= 0;
	m_pTextView			= 0;
	m_pControlsDlg	= 0;

	BOOL bRes = Create(WS_CHILD, CRect(0,0,0,0), AfxGetMainWnd());
	if (!bRes)
		return;
	ASSERT( bRes );
}

CVisualLogWnd::~CVisualLogWnd()
{
	SAFE_DELETE(m_pControlsDlg);
	SAFE_DELETE(m_pCD);
}



// CVisualLogWnd operations
BOOL CVisualLogWnd::Create(DWORD dwStyle,const RECT& rect,CWnd* pParentWnd)
{
	return CXTPFrameWnd::Create(VISUALLOGWND_CLASSNAME, NULL, dwStyle, rect, pParentWnd);
}

bool CVisualLogWnd::ReadLogFiles()
{
	CString strDir(PathFindFileName(m_pCD->strFolder));
	CString strBase = "", strNum = "";
	strDir = strDir.Left(strDir.GetLength() - 1);	// Remove the last character ('\' or '/')

	// Clear other data
	m_pCD->DestroyFonts();
	m_pCD->arrFrameTxt.RemoveAll();
	m_pCD->nMaxCol	= 0;

	// Check if the string ends with 4 digits
	if (strDir.GetLength() < 6)
		return false;
	for (int i = 4; i > 0; --i)
	{
		TCHAR chTmp = strDir[strDir.GetLength() - i];
		if (!std::isdigit(chTmp))
			return false;
		strNum += chTmp;
	}

	strBase = strDir.Left(strDir.GetLength() - 4);

	// Save the names of the log files
	m_pCD->strLog				= strBase + strNum + ".log";
	m_pCD->strLogParams	= strBase + strNum + "_params.log";

	// Open the log files
	FILE *pfLog = _tfopen(m_pCD->strFolder + m_pCD->strLog, _T("rt"));
	if (0 == pfLog)
		return false;
	FILE *pfLogParams = _tfopen(m_pCD->strFolder + m_pCD->strLogParams, _T("rt"));
	if (0 == pfLogParams)
	{
		fclose(pfLog);
		return false;
	}

	CDC *pDC = m_pTextView->GetBkDC();	// For measuring text height

	// File reading...
	TCHAR szBuffLog[0x400], szBuffParams[0x400], *pRet = 0, *pRet1 = 0;
	bool bRetVal = false;

	// Read the first 2 lines of each file
	pRet = _fgetts(szBuffLog, _countof(szBuffLog), pfLog);
	pRet1 = _fgetts(szBuffParams, _countof(szBuffParams), pfLogParams);
	if (!pRet || !pRet1 || (0 != _tcscmp(szBuffLog, szBuffParams))) goto FN_OUT;

	pRet = _fgetts(szBuffLog, _countof(szBuffLog), pfLog);
	pRet1 = _fgetts(szBuffParams, _countof(szBuffParams), pfLogParams);
	if (!pRet || !pRet1 || (0 != _tcscmp(szBuffLog, szBuffParams))) goto FN_OUT;

	// Read the next 4 lines of m_pCD->strLogParams (the default values)

	// Read the default color
	pRet = _fgetts(szBuffParams, _countof(szBuffParams), pfLogParams);
	if (!pRet) goto FN_OUT;
	while (*pRet != '(') pRet++; pRet++;
	{	// Read the default color
		CString r, g, b, a;
		while (*pRet != ',') { r += *pRet; pRet++; } pRet++;
		while (*pRet != ',') { g += *pRet; pRet++; } pRet++;
		while (*pRet != ',') { b += *pRet; pRet++; } pRet++;
		while (*pRet != ')') { a += *pRet; pRet++; } pRet++;
		float fr = _tstof(r);
		float fg = _tstof(g);
		float fb = _tstof(b);
		float fa = _tstof(a);
		fr *= fa; fg *= fa; fb *= fa;
		m_pCD->defVals.clr = RGB(int(255.f*fr), int(255.f*fg), int(255.f*fb));
	}

	// Read the default fontsize
	pRet = _fgetts(szBuffParams, _countof(szBuffParams), pfLogParams);
	if (!pRet) goto FN_OUT;
	while (*pRet != '(') pRet++; pRet++;
	{ // Read the default fontsize
		CString fs;
		while (*pRet != ')') { fs += *pRet; pRet++; } pRet++;
		float fsize = _tstof(fs);
		m_pCD->defVals.nSize = int (fsize * SVLogCommonData::fFONT_MUL);

		CFont *pFont = new CFont();
		pFont->CreatePointFont(m_pCD->defVals.nSize, SVLogCommonData::strFONT);
		m_pCD->mapFonts[m_pCD->defVals.nSize] = pFont;
		CFont *pOldFont = pDC->SelectObject(pFont);
		m_pCD->mapHeights[m_pCD->defVals.nSize] = (pDC->GetTextExtent("My", 2)).cy;
		pDC->SelectObject(pOldFont);
	}

	// Read the default column (columns are numbered 1, 2, ... N)
	pRet = _fgetts(szBuffParams, _countof(szBuffParams), pfLogParams);
	if (!pRet) goto FN_OUT;
	while (*pRet != '(') pRet++; pRet++;
	{ // Read the default column
		CString col;
		while (*pRet != ')') { col += *pRet; pRet++; } pRet++;
		m_pCD->defVals.nCol = _tstoi(col);
	}

	// The maximum column number
	if (m_pCD->nMaxCol < m_pCD->defVals.nCol) m_pCD->nMaxCol = m_pCD->defVals.nCol;

	// Ignore one line
	pRet = _fgetts(szBuffParams, _countof(szBuffParams), pfLogParams);
	if (!pRet) goto FN_OUT;

	// Initialize the size of m_pCD->arrFrameTxt
	//	m_pCD->arrFrameTxt.SetSize(m_pCD->nLastFrame + 1);

	int nFrameCounter = 0;
	while (true)
	{
		TCHAR *pRet = _fgetts(szBuffLog, _countof(szBuffLog), pfLog);
		// Check if eof has been reached in pfLog, finish reading if so
		if ((0 == pRet) && (feof(pfLog)))
		{
			bRetVal = true;
			break;
		}

		// If eof has been reached in pfLogParams and not in pfLog --> error
		if (0 == _fgetts(szBuffParams, _countof(szBuffParams), pfLogParams))
			break;

		if (szBuffParams[0] == 'F')	// Reading frame info
		{
			if (0 != _tcscmp(szBuffLog, szBuffParams))
				break;	// Error (frame info not equal)

			// Read the frame number
			while (!std::isdigit(*pRet)) pRet++;
			CString strFrameNum;
			while (std::isdigit(*pRet)) { strFrameNum += *pRet; pRet++; }
			int nFrameNum = _tstoi(strFrameNum);

			// Read the frame image string
			CString strFrameName;
			while (*pRet != '[') pRet++; pRet++;
			while (*pRet != ']') { strFrameName += *pRet; pRet++; }

			// Read the frame time
			CString strFrameTime;
			while (*pRet != '[') pRet++; pRet++;
			while (*pRet != ']') { strFrameTime += *pRet; pRet++; }
			float fFrameTime = _tstof(strFrameTime);

			// Exit if the frame number doesn't match
			if (nFrameNum != nFrameCounter)
			{
				CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "Duplicate frame in log files [frame = %d]", nFrameNum);
				goto FN_OUT;
			}
			nFrameCounter++;

			m_pCD->arrFrameTxt.Add(SVLogFrameTxtDataEx());
			m_pCD->arrFrameTxt[nFrameNum].Set(strFrameName, fFrameTime, new CList<SVLogFrameTxtData, SVLogFrameTxtData &>());	// Insert element
		}
		else	// Reading frame data
		{
			szBuffLog[_tcslen(szBuffLog)-1] = '\0';	// Remove the '\n' character

			SVLogFrameTxtData ftd(szBuffLog, m_pCD->defVals.nCol, m_pCD->defVals.clr, m_pCD->defVals.nSize, m_pCD->defVals.bSync);

			// Parse line data
			pRet = szBuffParams;
			if (*pRet != '[') break;
			for (pRet++;;)
			{
				if (*pRet == ']') break;

				CString strParam;
				while (std::isalpha(*pRet) || (*pRet == '_')) { strParam += *pRet; pRet++; } pRet++;

				if (strParam == "color")
				{	// Read the color
					CString r, g, b, a;
					while (*pRet != ',') { r += *pRet; pRet++; } pRet++;
					while (*pRet != ',') { g += *pRet; pRet++; } pRet++;
					while (*pRet != ',') { b += *pRet; pRet++; } pRet++;
					while (*pRet != ')') { a += *pRet; pRet++; } pRet++;
					if (*pRet == ',') pRet++;
					float fr = _tstof(r);
					float fg = _tstof(g);
					float fb = _tstof(b);
					float fa = _tstof(a);
					fr *= fa; fg *= fa; fb *= fa;
					ftd.clr = RGB(int(255.f*fr), int(255.f*fg), int(255.f*fb));
				}
				else if (strParam == "size")
				{	// Read the font size
					CFont *pFont = 0;
					CString fs;
					while (*pRet != ')') { fs += *pRet; pRet++; } pRet++;
					if (*pRet == ',') pRet++;
					float fsize = _tstof(fs);
					ftd.nSize = int (fsize * SVLogCommonData::fFONT_MUL);

					if (!m_pCD->mapFonts.Lookup(ftd.nSize, pFont))
					{
						pFont = new CFont();
						pFont->CreatePointFont(ftd.nSize, SVLogCommonData::strFONT);
						m_pCD->mapFonts[ftd.nSize] = pFont;
						CFont *pOldFont = pDC->SelectObject(pFont);
						m_pCD->mapHeights[ftd.nSize] = (pDC->GetTextExtent("My", 2)).cy;
						pDC->SelectObject(pOldFont);
					}
				}
				else if (strParam == "column")
				{	// Read the column number
					CString col;
					while (*pRet != ')') { col += *pRet; pRet++; } pRet++;
					if (*pRet == ',') pRet++;
					ftd.nCol = _tstoi(col);

					if (ftd.nCol > m_pCD->nMaxCol)
						m_pCD->nMaxCol = ftd.nCol;
				}
				else if (strParam == "align_columns_to_this")
				{
					while (*pRet != ')') pRet++; pRet++;
					if (*pRet == ',') pRet++;

					ftd.bSync = true;
				}
				else goto FN_OUT;
			}

			// Add a line for the frame
			m_pCD->arrFrameTxt[nFrameCounter-1].lst->AddTail(ftd);
		}
	}

FN_OUT:
	// Close the log files
	fclose(pfLog);
	fclose(pfLogParams);

	if (!bRetVal) // Clear data on failure
	{
		m_pCD->DestroyFonts();
		m_pCD->arrFrameTxt.RemoveAll();
		m_pCD->nMaxCol	= 0;
	}
	else
	{
		m_pTextView->ResetColumns();
		m_pCD->bHasText = true;
	}

	return bRetVal;
}


// CVisualLogWnd message handlers
int CVisualLogWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CXTPFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	//////////////////////////////////////////////////////////////////////////
	GetDockingPaneManager()->InstallDockingPanes(this);
	GetDockingPaneManager()->SetTheme(CMainFrame::GetDockingPaneTheme());
	GetDockingPaneManager()->SetThemedFloatingFrames(TRUE);
	if (CMainFrame::GetDockingHelpers())
	{
		GetDockingPaneManager()->SetAlphaDockingContext(TRUE);
		GetDockingPaneManager()->SetShowDockingContextStickers(TRUE);
	}
	//////////////////////////////////////////////////////////////////////////

	// Create the view
	m_pView = new CVLogImageView();
	m_pView->Create(WS_CHILD|WS_CLIPCHILDREN|WS_VISIBLE, CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST);

	// Create the text pane
	CXTPDockingPane *pTextPane = 0;
	{
		m_paneText.Create(CVLogTextPreview::IDD, this);
		pTextPane = GetDockingPaneManager()->CreatePane(IDW_SW_TEXT_PANE, CRect(0,0,550,200), dockBottomOf);
		pTextPane->SetTitle( _T("Text Output") );
		pTextPane->SetOptions( xtpPaneNoCloseable );

		// Create the view within text pane
		m_pTextView = new CVLogTextView();
		m_pTextView->Create(WS_CHILD|WS_CLIPCHILDREN|WS_VISIBLE, CRect(0, 0, 0, 0), &m_paneText, AFX_IDW_PANE_FIRST+1);
	}

	// Create the controls pane
	CXTPDockingPane *pControlsPane = 0;
	{
		m_paneControls.Create(CVisualLogControls::IDD, this);
		pControlsPane = GetDockingPaneManager()->CreatePane(IDW_SW_CONTROLS_PANE, CRect(0,0,550,120), dockTopOf);
		pControlsPane->SetTitle( _T("Controls") );
		pControlsPane->SetOptions( xtpPaneNoCloseable|xtpPaneNoHideable|xtpPaneNoFloatable|xtpPaneNoCaption );

		// Create the dialog within the controls pane
		m_pControlsDlg = new CVisualLogDialog(&m_paneControls);
		m_pControlsDlg->Create(CVisualLogDialog::IDD, &m_paneControls);
		m_pControlsDlg->SetOwner(this);
	}

	// Initialize the command bars
	InitCommandBars();

	// Restore (or set) previous layout
	CXTRegistryManager regMgr;
	int nPaneLayoutVersion = regMgr.GetProfileInt(_T("VisualLog"), _T("VisualLogLayoutVersion"), 0);
	if (nPaneLayoutVersion == VisualLogLayoutVersion)
	{
		CXTPDockingPaneLayout layout(GetDockingPaneManager());
		if (layout.Load(_T("VisualLogLayout"))) 
		{
			if (layout.GetPaneList().GetCount() > 0)
			{
				GetDockingPaneManager()->SetLayout(&layout);	
			}
		}
	}
	else
		regMgr.WriteProfileInt(_T("VisualLog"), _T("VisualLogLayoutVersion"), VisualLogLayoutVersion);

	// Set the common data in the view (m_pView) , the dialog (m_pControlsDlg) and the text output view
	m_pView->SetCommonData(m_pCD);
	m_pControlsDlg->SetCommonData(m_pCD);
	m_pTextView->SetCommonData(m_pCD);

	return 0;
}

BOOL CVisualLogWnd::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CVisualLogWnd::OnDestroy()
{
	CXTPFrameWnd::OnDestroy();

	CXTPDockingPaneLayout layout(GetDockingPaneManager());
	GetDockingPaneManager()->GetLayout( &layout );
	layout.Save(_T("VisualLogLayout"));
}

LRESULT CVisualLogWnd::OnDockingPaneNotify(WPARAM wParam, LPARAM lParam)
{
	if (wParam == XTP_DPN_SHOWWINDOW)
	{
		// get a pointer to the docking pane being shown.
		CXTPDockingPane* pwndDockWindow = (CXTPDockingPane*)lParam;
		if (!pwndDockWindow->IsValid())
		{
			switch (pwndDockWindow->GetID())
			{
			case IDW_SW_CONTROLS_PANE:
				pwndDockWindow->Attach(m_pControlsDlg);
				m_pControlsDlg->ShowWindow(SW_SHOW);
				return TRUE;

			case IDW_SW_TEXT_PANE:
				pwndDockWindow->Attach(m_pTextView);
				m_pTextView->ShowWindow(SW_SHOW);
				return TRUE;

			default:
				return FALSE;
			}
		}
	}
	else if (wParam == XTP_DPN_CLOSEPANE)
	{
		// get a pointer to the docking pane being closed.
		CXTPDockingPane* pwndDockWindow = (CXTPDockingPane*)lParam;
		if (pwndDockWindow->IsValid())
		{
			switch (pwndDockWindow->GetID())
			{
			case IDW_SW_CONTROLS_PANE:
				break;
			case IDW_SW_TEXT_PANE:
				break;
			}
		}
	}

	return FALSE;
}

LRESULT CVisualLogWnd::OnDialogNotify(WPARAM wParam, LPARAM lParam)
{
	switch (lParam)
	{
	case eVLDML_CheckValidFolder:	// Check if the folder is valid (lParam == 1)
		{
			// Always invalidate the view even if the folder is invalid
			m_pView->Invalidate();
			m_pTextView->Invalidate();

			bool bNoImages, bNoTextFiles;

			// Enumerate the image files
			m_pCD->vecFiles.clear();
			CFileEnum::ScanDirectory(m_pCD->strFolder, "Frame*.*", m_pCD->vecFiles, false);
			bNoImages = m_pCD->vecFiles.empty();

			if (!bNoImages)
			{
				m_pCD->img.Destroy();
				if (SUCCEEDED( m_pCD->img.Load(m_pCD->strFolder + m_pCD->vecFiles[0]) ))
					m_pView->InitDestRect();
				else
					bNoImages = true;
			}

			bNoTextFiles = !ReadLogFiles();

			if (!bNoTextFiles && !bNoImages)
			{	// Restructure image vector
				if (m_pCD->arrFrameTxt.GetCount() != m_pCD->vecFiles.size())
				{
					std::vector<CString> vecNew(m_pCD->arrFrameTxt.GetCount());

					std::vector<CString>::iterator itOrig = m_pCD->vecFiles.begin();
					std::vector<CString>::iterator itNew = vecNew.begin();
					int nTextIdx = 0;
					for (;itOrig != m_pCD->vecFiles.end(); ++itOrig)
					{
						while ((nTextIdx < m_pCD->arrFrameTxt.GetCount()) && (m_pCD->arrFrameTxt[nTextIdx].strFileName != *itOrig))
							nTextIdx++;
						if (nTextIdx >= m_pCD->arrFrameTxt.GetCount())
						{
							bNoImages = true;
							break;
						}
						vecNew[nTextIdx] = m_pCD->arrFrameTxt[nTextIdx].strFileName;
					}

					m_pCD->vecFiles.swap(vecNew);
				}
			}

			if (bNoImages)
				m_pCD->vecFiles.clear();

			// Set the common data
			m_pCD->eState			= eVLPS_Active;
			m_pCD->nCurFrame	= 0;
			m_pCD->nLastFrame	= max( int(m_pCD->vecFiles.size()), int(m_pCD->arrFrameTxt.GetCount())) - 1;
			m_pCD->bHasImages	= !bNoImages;

			if (bNoImages && bNoTextFiles)
				return FALSE;
			if (bNoImages)
			{
				CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "Error reading the image files!\r\nNo image preview will be available.");
				m_pCD->vecFiles.clear();
			}
			if (bNoTextFiles)
				CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "Error reading the text (*.log) files!\r\nNo text output will be available.");

			return TRUE;
		}
		break;

	case eVLDML_FrameChanged:	// Frame slider moved
		m_pView->m_nPrevFrame = -1;
		// Fall through

	case eVLDML_Invalidate:	// Invalidate
		if (m_pCD->bUpdateImages && m_pCD->bHasImages)	m_pView->Invalidate();
		if (m_pCD->bUpdateTxt && m_pCD->bHasText)				m_pTextView->Invalidate();
		return TRUE;

	case eVLDML_ForceInvalidate:	// Force Invalidate
		if (wParam & eVLDMW_View) m_pView->Invalidate();
		if (wParam & eVLDMW_Text) m_pTextView->Invalidate();
		return TRUE;
	}

	return FALSE;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVLogImageView class
BEGIN_MESSAGE_MAP(CVLogImageView, CView)
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
END_MESSAGE_MAP()



// CVLogImageView construction & destruction
CVLogImageView::CVLogImageView()
{
	m_pCD = 0;
	m_nPrevFrame = -1;
}

CVLogImageView::~CVLogImageView()
{
}



// CVLogImageView operations
bool CVLogImageView::Create( DWORD dwStyle,const RECT &rect,CWnd *pParentWnd,UINT nID )
{
	if (!m_hWnd)
	{
		LPCTSTR lpClassName = AfxRegisterWndClass(CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW,	AfxGetApp()->LoadStandardCursor(IDC_ARROW), NULL, NULL);
		BOOL bCrt = CreateEx(NULL, lpClassName, "VisualLoggerView", dwStyle, CRect(0,0,100,100), pParentWnd, nID);

		if (!m_hWnd)
			return false;
	}
	return true;
}

void CVLogImageView::InitDestRect()
{
	if (m_pCD && !m_pCD->img.IsNull() && (m_rcClient.right > 4) && (m_rcClient.bottom > 4))
	{
		//if ((m_pCD->img.GetWidth() <= m_rcClient.right) && (m_pCD->img.GetHeight() <= m_rcClient.bottom))
		//{
		//	int nLeft	= (m_rcClient.right - m_pCD->img.GetWidth()) / 2;
		//	int nTop	= (m_rcClient.bottom - m_pCD->img.GetHeight()) / 2;
		//	m_rcDest.SetRect(nLeft, nTop, nLeft + m_pCD->img.GetWidth(), nTop + m_pCD->img.GetHeight());
		//}
		//else
		{
			int nWidth = m_rcClient.right - 4;
			int nHeight = (nWidth * m_pCD->img.GetHeight()) / m_pCD->img.GetWidth();

			if (nHeight < (m_rcClient.bottom - 4))
			{
				m_rcDest.SetRect(m_rcClient.left + 2,
					m_rcClient.top + (m_rcClient.bottom - nHeight)/2,
					m_rcClient.right - 2,
					m_rcClient.top + (m_rcClient.bottom + nHeight)/2);
			}
			else
			{
				nHeight = m_rcClient.bottom - 4;
				nWidth = (nHeight * m_pCD->img.GetWidth()) / m_pCD->img.GetHeight();

				m_rcDest.SetRect(m_rcClient.left + (m_rcClient.right - nWidth)/2,
					m_rcClient.top + 2,
					m_rcClient.left + (m_rcClient.right + nWidth)/2,
					m_rcClient.bottom - 2);
			}
		}
	}
}



// CVLogImageView virtual overrides
void CVLogImageView::OnDraw(CDC *pDC)
{
	if ((m_pCD->eState == eVLPS_Uninitialized) || !m_pCD->bHasImages || (m_rcClient.right < 5) || (m_rcClient.bottom < 5) ||
		!m_pCD || (m_rcDest.Width() < 1) || (m_rcDest.Height() < 1))
		pDC->FillSolidRect(m_rcClient, m_pCD->clrBack);
	else
	{
		if (!m_pCD->bUpdateImages || (m_pCD->vecFiles[m_pCD->nCurFrame] == ""))
		{
			pDC->FillSolidRect(m_pCD->bKeepAspect  ?  m_rcDest : m_rcClient, RGB(127,127,127));
			if (m_pCD->bKeepAspect)
			{
				pDC->FillSolidRect(0, 0, m_rcClient.right, m_rcDest.top, m_pCD->clrBack);
				pDC->FillSolidRect(0, m_rcDest.top, m_rcDest.left, m_rcClient.bottom - m_rcDest.top, m_pCD->clrBack);
				pDC->FillSolidRect(m_rcDest.right, m_rcDest.top, m_rcClient.right - m_rcDest.right, m_rcClient.bottom - m_rcDest.top, m_pCD->clrBack);
				pDC->FillSolidRect(m_rcDest.left, m_rcDest.bottom, m_rcDest.Width(), m_rcClient.bottom - m_rcDest.bottom, m_pCD->clrBack);
			}
		}
		else
		{
			if (m_nPrevFrame != m_pCD->nCurFrame)
			{
				m_pCD->img.Destroy();
				m_pCD->img.Load(m_pCD->strFolder + m_pCD->vecFiles[m_pCD->nCurFrame]);
				m_nPrevFrame = m_pCD->nCurFrame;
			}

			pDC->SetStretchBltMode(HALFTONE);
			if (m_pCD->bKeepAspect)
			{
				if (!m_pCD->img.IsNull())
					m_pCD->img.Draw(pDC->m_hDC, m_rcDest.left, m_rcDest.top, m_rcDest.Width(), m_rcDest.Height(),
					0, 0, m_pCD->img.GetWidth(), m_pCD->img.GetHeight());
				else
					pDC->FillSolidRect(m_rcDest, RGB(127,127,127));

				pDC->FillSolidRect(0, 0, m_rcClient.right, m_rcDest.top, m_pCD->clrBack);
				pDC->FillSolidRect(0, m_rcDest.top, m_rcDest.left, m_rcClient.bottom - m_rcDest.top, m_pCD->clrBack);
				pDC->FillSolidRect(m_rcDest.right, m_rcDest.top, m_rcClient.right - m_rcDest.right, m_rcClient.bottom - m_rcDest.top, m_pCD->clrBack);
				pDC->FillSolidRect(m_rcDest.left, m_rcDest.bottom, m_rcDest.Width(), m_rcClient.bottom - m_rcDest.bottom, m_pCD->clrBack);
			}
			else
				m_pCD->img.Draw(pDC->m_hDC, 0, 0, m_rcClient.Width(), m_rcClient.Height(),
				0, 0, m_pCD->img.GetWidth(), m_pCD->img.GetHeight());
		}
	}
}

void CVLogImageView::PostNcDestroy()
{
	delete this;
}



// CVLogImageView message handlers
BOOL CVLogImageView::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CVLogImageView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	// Calculate image display position
	m_rcClient.SetRect(0, 0, cx, cy);

	InitDestRect();
	Invalidate();
}





/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CVLogTextView class
BEGIN_MESSAGE_MAP(CVLogTextView, CView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_DESTROY()
END_MESSAGE_MAP()



// CVLogTextView construction & destruction
CVLogTextView::CVLogTextView()
{
	m_pCD = 0;
	m_fntInfo.CreatePointFont(100, "Courier New");
}

CVLogTextView::~CVLogTextView()
{
}



// CVLogTextView operations
bool CVLogTextView::Create( DWORD dwStyle,const RECT &rect,CWnd *pParentWnd,UINT nID )
{
	if (!m_hWnd)
	{
		LPCTSTR lpClassName = AfxRegisterWndClass(CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW,	AfxGetApp()->LoadStandardCursor(IDC_ARROW), NULL, NULL);
		BOOL bCrt = CreateEx(NULL, lpClassName, "VisualLoggerTextView", dwStyle, CRect(0,0,100,100), pParentWnd, nID);

		if (!m_hWnd)
			return false;
	}
	return true;
}

void CVLogTextView::ResetColumns()
{
	if (m_pCD && m_pCD->nMaxCol)
	{
		m_nColumnWidth = m_rcClient.right / m_pCD->nMaxCol;

		m_arrColHeights.SetSize(m_pCD->nMaxCol);
		for (int i = 0; i < m_pCD->nMaxCol; ++i)
			m_arrColHeights[i] = 20;
	}
}



// CVLogTextView virtual overrides
void CVLogTextView::OnDraw(CDC *pDC)
{
	CBitmap *pBmTmp = m_dcBack.SelectObject(&m_bmBack);
	m_dcBack.PatBlt(0, 0, m_rcClient.right, m_rcClient.bottom, BLACKNESS);

	// Drawing code goes here
	if ((!m_pCD->bUpdateTxt) || (m_pCD->eState == eVLPS_Uninitialized) || !m_pCD->bHasText)
	{
		m_dcBack.PatBlt(0, 0, m_rcClient.right, m_rcClient.bottom, BLACKNESS);
		m_dcBack.SetTextColor(RGB(255,255,255));
		m_dcBack.TextOut(10, 10, _T("No Data / No Update"));
	}
	else
	{
		m_dcBack.FillSolidRect(0, 0, m_rcClient.right, 20, RGB(25,25,40));
		m_dcBack.PatBlt(0, 20, m_rcClient.right, m_rcClient.bottom-20, BLACKNESS);
		SVLogFrameTxtDataEx &refFTX = m_pCD->arrFrameTxt[m_pCD->nCurFrame];
		int nMaxColHeight = 20;

		// Draw frame info first
		static CRect rc1(15, 0, 45, 20), rc2(48, 0, 198, 20), rc3(250, 0, 290, 20);
		CString strTime;
		m_dcBack.SelectObject(m_fntInfo);
		m_dcBack.SetTextColor(RGB(100,255,100));
		m_dcBack.DrawText("File:", 5, rc1, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
		m_dcBack.DrawText(refFTX.strFileName, rc2, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
		m_dcBack.SetTextColor(RGB(140,140,255));
		strTime.Format(_T("Time: [%8.3f]"), refFTX.fFrameTime);
		m_dcBack.DrawText(strTime, rc3, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);

		// Draw frame data
		for (POSITION pos = refFTX.lst->GetHeadPosition(); pos != NULL;)
		{
			SVLogFrameTxtData &refFT = refFTX.lst->GetNext(pos);
			m_dcBack.SetTextColor(refFT.clr);
			m_dcBack.SelectObject(m_pCD->mapFonts[refFT.nSize]);
			if (refFT.bSync)
			{
				for (int i = 0; i < m_pCD->nMaxCol; ++i)
					m_arrColHeights[i] = nMaxColHeight;
			}
			int x = m_arrColHeights[refFT.nCol-1];
			m_dcBack.TextOut(1 + m_nColumnWidth*(refFT.nCol-1), m_arrColHeights[refFT.nCol-1], refFT.str);
			m_arrColHeights[refFT.nCol-1] += m_pCD->mapHeights[refFT.nSize];
			if (nMaxColHeight < m_arrColHeights[refFT.nCol-1])
				nMaxColHeight = m_arrColHeights[refFT.nCol-1];
		}

		m_dcBack.SelectStockObject(ANSI_FIXED_FONT);
	}

	pDC->BitBlt(0, 0, m_rcClient.right, m_rcClient.bottom, &m_dcBack, 0, 0, SRCCOPY);
	m_dcBack.SelectObject(pBmTmp);
}

void CVLogTextView::PostNcDestroy()
{
	delete this;
}



// CVLogTextView message handlers
int CVLogTextView::OnCreate(LPCREATESTRUCT lpcs)
{
	if (CView::OnCreate(lpcs) == -1)
		return -1;

	// Create the backbuffer DC
	CClientDC dc(this);
	m_dcBack.CreateCompatibleDC(&dc);
	m_dcBack.SetBkMode(TRANSPARENT);
	m_dcBack.SetTextAlign(TA_LEFT | TA_TOP);

	return 0;
}

void CVLogTextView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	// Calculate image display position
	m_rcClient.SetRect(0, 0, cx, cy);

	// Create the backbuffer bitmap
	CClientDC dc(this);
	m_bmBack.DeleteObject();
	m_bmBack.CreateCompatibleBitmap(&dc, cx, cy);

	ResetColumns();
}

BOOL CVLogTextView::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

void CVLogTextView::OnDestroy()
{
	CView::OnDestroy();
}
