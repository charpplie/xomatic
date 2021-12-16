#include "StdAfx.h"
#include "AboutDialog.h"

/////////////////////////////////////////////////////////////////////////////
// CAboutDialog dialog

CAboutDialog *CAboutDialog::s_pAboutWindow = 0;
// Defining the transparent color
const COLORREF CAboutDialog::kTransparentColor = RGB(255, 0, 255);

CAboutDialog::CAboutDialog(const Version &v, CWnd* pParent /*=NULL*/)
: CDialog(CAboutDialog::IDD, pParent)
{
	m_version = v;
}

CAboutDialog::~CAboutDialog()
{
	s_pAboutWindow = 0;
}

void CAboutDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ABOUT_VERSION, m_TransparentVersion);
	DDX_Control(pDX, IDC_ABOUT_COPYRIGHT , m_TransparentCopyright);
	DDX_Control(pDX, IDC_ABOUT_ALLRIGHTRESERVED, m_TransparentAllRightReserved);
	DDX_Control(pDX, IDC_ABOUT_TRADEMARKS, m_TransparentTrademarks);
	DDX_Control(pDX, IDC_ABOUT_DEVELOPEDBY  , m_TransparentDevelopedBy);
	//{{AFX_DATA_MAP(CStartupLogoDialog)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAboutDialog, CDialog)
	//{{AFX_MSG_MAP(CAboutDialog)
	//}}AFX_MSG_MAP
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_WM_PAINT()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////

void CAboutDialog::OnClose() 
{
	DeleteObject(m_hBitmap);
	CDialog::OnClose();
}

void CAboutDialog::OnPaint() 
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

void CAboutDialog::OnSize(UINT nType, int cx, int cy) 
{
	__super::OnSize(nType, cx, cy);
	// Unloads the image. Important after the first call of this function, when the variable
	// m_hBitmap is still containing a bitmap.
	m_hBitmap.Detach();
	// Loads the splash screen image.
	m_hBitmap.Attach(LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_CRYTEK_LOGO) ) );

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

BOOL CAboutDialog::OnInitDialog() 
{
	s_pAboutWindow = this;

	m_TransparentCopyright.SetTextColor(RGB(255,255,255));
	m_TransparentVersion.SetTextColor(RGB(255,255,255));
	m_TransparentAllRightReserved.SetTextColor(RGB(255,255,255));
	m_TransparentTrademarks.SetTextColor(RGB(255,255,255));
	m_TransparentDevelopedBy.SetTextColor(RGB(255,255,255));
	SetVersion(m_version);

	return CDialog::OnInitDialog();
}

void CAboutDialog::OnLButtonUp(UINT nFlags, CPoint point) 
{
	s_pAboutWindow->EndDialog(IDOK);
}


void CAboutDialog::SetVersion( const Version &v )
{
	if (!m_hWnd)
		return;

	char pVersionText[256];

//Diesel cut
	//_snprintf(pVersionText, 256, "Version %d.%d.%d - Build %d", v[3], v[2], v[1], v[0]);
//Diesel new	
	_snprintf(pVersionText, 256, "Engine %d.%d.%d.%d", v[3], v[2], v[1], v[0]);
	

	SetDlgItemText(IDC_ABOUT_TRADEMARKS, CString(pVersionText));
}