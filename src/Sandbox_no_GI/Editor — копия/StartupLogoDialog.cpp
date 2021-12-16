// StartupLogoDialog.cpp : implementation file
//

#include "StdAfx.h"
#include "StartupLogoDialog.h"

/////////////////////////////////////////////////////////////////////////////
// CStartupLogoDialog dialog

CStartupLogoDialog *CStartupLogoDialog::s_pLogoWindow = 0;
// Defining the transparent color

const COLORREF CStartupLogoDialog::kTransparentColor = RGB(255, 0, 255);


CStartupLogoDialog::CStartupLogoDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CStartupLogoDialog::IDD, pParent)
{

}

CStartupLogoDialog::~CStartupLogoDialog()
{
	s_pLogoWindow = 0;
}

void CStartupLogoDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_INFO_TEXT, m_TransparentText);
	DDX_Control(pDX, IDC_VERSION, m_TransparentVersion);
	//{{AFX_DATA_MAP(CStartupLogoDialog)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStartupLogoDialog, CDialog)
	//{{AFX_MSG_MAP(CStartupLogoDialog)
	//}}AFX_MSG_MAP
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_WM_PAINT()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStartupLogoDialog message handlers+


void CStartupLogoDialog::SetVersion( const Version &v )
{
	if (!m_hWnd)
		return;

	char pVersionText[256];
//Diesel cut
	//_snprintf(pVersionText, 256, "Version %d.%d.%d - Build %d", v[3], v[2], v[1], v[0]);
//Diesel new
	_snprintf(pVersionText, 256, "Engine %d.%d.%d.%d", v[3], v[2], v[1], v[0]);
	
	SetDlgItemText(IDC_VERSION, CString(pVersionText));
}


void CStartupLogoDialog::SetInfo( const char *text )
{
	if (m_hWnd != NULL)
	{
		m_TransparentText.SetWindowText(text);
	}
}

void CStartupLogoDialog::SetText( const char *text )
{
	if (s_pLogoWindow)
		s_pLogoWindow->SetInfo( text );
}

void CStartupLogoDialog::SetInfoText( const char *text )
{
	SetInfo( text );
}

void CStartupLogoDialog::OnClose() 
{
	DeleteObject(m_hBitmap);
	CDialog::OnClose();
}

void CStartupLogoDialog::OnPaint() 
{
	// Device context for painting
	CPaintDC dc(this); 
	// Creates a memory DC
	HDC hMemDC = ::CreateCompatibleDC(NULL);
	// Selects the bitmap in the memory dc.
	SelectObject(hMemDC, m_hBitmap);
	// Copies the memory dc into the screen dc
	::BitBlt(dc.m_hDC, 0, 0, m_Bitmap.bmWidth, m_Bitmap.bmHeight, hMemDC, 0, 0, SRCCOPY);
	// Deletes the memory DC and the bitmap
	::DeleteDC(hMemDC);
	CDialog::OnPaint();
}

void CStartupLogoDialog::OnSize(UINT nType, int cx, int cy) 
{
	__super::OnSize(nType, cx, cy);
	// Unloads the image. Important after the first call of this function, when the variable
	// m_hBitmap is still containing a bitmap.
	m_hBitmap.Detach();
	// Loads the splash screen image.
	m_hBitmap.Attach(LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_SPLASH) ) );

	// Get info about the bitmap 
	m_hBitmap.GetObject(sizeof(BITMAP),&m_Bitmap);
	// Puts the bitmap into a memory device context
	CPaintDC dc(this);
	// Gets a memory dc object
	CDC dcMem;
	// Creates a compatible dc
	dcMem.CreateCompatibleDC(&dc);	
	// Selects the bitmap into the dc
	CBitmap* pOldBitmap = dcMem.SelectObject(&m_hBitmap);
	// Creates a couple of region objects.
	CRgn crRgn, crRgnTmp;
	// crRgn is an empty region.
	crRgn.CreateRectRgn(0, 0, 0, 0);
	// Creates a region from a bitmap with transparency colour of Purple
	COLORREF crTransparent = kTransparentColor;	
	int iX = 0, iY = 0;
	for (iY = 0; iY < m_Bitmap.bmHeight; ++iY)
	{
		do
		{
			// Skips over transparent pixels at start of lines.
			while (iX <= m_Bitmap.bmWidth && dcMem.GetPixel(iX, iY) == crTransparent)
				iX++;
			// Remembers this pixel
			int iLeftX = iX;
			// Now finds first non transparent pixel
			while (iX <= m_Bitmap.bmWidth && dcMem.GetPixel(iX, iY) != crTransparent)
				++iX;
			// Creates a temp region on this info
			crRgnTmp.CreateRectRgn(iLeftX, iY, iX, iY+1);
			// Combines into main region.
			crRgn.CombineRgn(&crRgn, &crRgnTmp, RGN_OR);
			// Deletes the temp region for next pass (otherwise you'll get an ASSERT)
			crRgnTmp.DeleteObject();
		}while(iX < m_Bitmap.bmWidth);
		iX = 0;
	}
	// Put the image in the center of the current desktop
	SetWindowRgn(crRgn, TRUE);
	iX = (GetSystemMetrics(SM_CXSCREEN)) / 2 - (m_Bitmap.bmWidth / 2);
	iY = (GetSystemMetrics(SM_CYSCREEN)) / 2 - (m_Bitmap.bmHeight / 2);
	SetWindowPos(&wndTopMost, iX, iY, m_Bitmap.bmWidth, m_Bitmap.bmHeight, NULL); 

	// Frees resources.
	dcMem.SelectObject(pOldBitmap);	// Put the original bitmap back (prevents memory leaks)
	dcMem.DeleteDC();
	crRgn.DeleteObject();
}

BOOL CStartupLogoDialog::OnInitDialog() 
{
	s_pLogoWindow = this;

	CDialog::OnInitDialog();

	CRect rcBmp, rcDlg, rcTxt;
	GetWindowRect(rcDlg);
	GetDlgItem(IDC_VERSION)->GetWindowRect(rcTxt);
	GetDlgItem(IDC_STATIC)->GetWindowRect(rcBmp);
	rcTxt.OffsetRect(rcBmp.Width()+2 - rcDlg.Width(), 0);
	ScreenToClient(rcTxt);
	SetWindowPos(NULL, 0, 0, rcBmp.Width()+2, rcDlg.Height(), SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER);
	GetDlgItem(IDC_VERSION)->SetWindowPos(NULL, rcTxt.left, rcTxt.top, 0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOOWNERZORDER|SWP_NOZORDER);

//Diesel cut
	//SetWindowText( "Starting Crytek Sandbox Editor" );
//Diesel new
	SetWindowText( "Starting Editor" );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
