// VisualLogCommon.h : Common data for the Visual Log Viewer tool
#pragma once

#include <atlimage.h>



enum EVLogPreviewState
{
	eVLPS_Uninitialized,
	eVLPS_Active,
	eVLPS_Playing,
};

enum EVLogDlgMsgLparam
{
	eVLDML_CheckValidFolder = 1,
	eVLDML_FrameChanged,
	eVLDML_Invalidate,
	eVLDML_ForceInvalidate,
	eVLDML_ForceDword = 0xFFFFFFFF,
};

enum EVLogDlgMsgWparam
{
	eVLDMW_View	= 0x01,
	eVLDMW_Text = 0x02,
	eVLDMW_Both = 0x0F,
};

struct SVLogFrameTxtData
{
	SVLogFrameTxtData(CString str = "", int nCol = 0, COLORREF clr = RGB(0,0,0), int nSize = 0, bool bSync = false)
	{
		this->str		= str;
		this->nCol	= nCol;
		this->clr		= clr;
		this->nSize	= nSize;
		this->bSync	= bSync;
	}

	CString		str;		// One line of text
	int				nCol;		// The column to display the text in
	COLORREF	clr;		// Color of the display text
	int				nSize;	// The font size (identifier)
	bool			bSync;	// Sync column heights after displaying this element
};


struct SVLogFrameTxtDataEx
{
	SVLogFrameTxtDataEx():strFileName(""), fFrameTime(0.f), lst(NULL)
	{
	}

	~SVLogFrameTxtDataEx()
	{
		delete lst;
	}

	void Set(const CString &str, float f, CList<SVLogFrameTxtData, SVLogFrameTxtData &> *l)
	{
		strFileName	= str;
		fFrameTime	= f;
		lst					= l;
	}

	CList<SVLogFrameTxtData, SVLogFrameTxtData &>	*lst;	// list of the strings to display for this frame
	CString	strFileName;
	float		fFrameTime;
};


struct SVLogCommonData
{
	static const float fFONT_MUL;
	static const CString strFONT;

	// Construction & destruction
	SVLogCommonData(void)
	{
		eState				= eVLPS_Uninitialized;
		strFolder			= "";
		nCurFrame			= -1;
		nLastFrame		= -1;
		bUpdateTxt		= true;
		bUpdateImages	= true;
		bKeepAspect		= true;
		clrBack				= RGB(0,0,0);
		strLog				= "";
		strLogParams	= "";
		nMaxCol				= 0;
		bHasText			= false;
		bHasImages		= false;

		defVals.str = "defaults";
	}

	~SVLogCommonData(void)
	{
		DestroyFonts();
	}

	// Operations
	void DestroyFonts(void)
	{
		bHasText = false;

		for (POSITION pos = mapFonts.GetStartPosition(); pos != NULL;)
		{
			int nSize;
			CFont *pFnt;
			mapFonts.GetNextAssoc(pos, nSize, pFnt);
			delete pFnt;
		}
		mapFonts.RemoveAll();
		mapHeights.RemoveAll();
	}

	// Member data
	EVLogPreviewState						eState;					// State of the visual log viewer
	CString											strFolder;			// Working folder
	int													nCurFrame;			// Current frame
	int													nLastFrame;			// Last frame
	std::vector<CString>				vecFiles;				// Image files in the directory
	bool												bUpdateTxt;			// Indicates whether text output should be updated as frames change
	bool												bUpdateImages;	// Indicates whether image view should be updated as frames change
	bool												bKeepAspect;		// Indicates whether frames should be displayed using their original aspect ratio
	ATL::CImage									img;						// The image being displayed currently
	COLORREF										clrBack;				// The background color of the view
	// Logs
	CString											strLog;					// The log file
	CString											strLogParams;		// The log_params file
	int													nMaxCol;				// The maximum column number

	SVLogFrameTxtData						defVals;				// Default values for text display

	bool																bHasImages;		// Images have been successfully read
	bool																bHasText;			// Text has been successfully read
	CMap<int, int, CFont *, CFont *>		mapFonts;			// Fonts of various sizes (precreated for use)
	CMap<int, int, int, int>						mapHeights;		// Heights of the fonts
	CArray<SVLogFrameTxtDataEx, SVLogFrameTxtDataEx &>	arrFrameTxt;	// <SVLogFrameTxtDataEx-text_data_for_the_frame>
};



class CFixedToolTip : public CToolTipCtrl
{
	DECLARE_DYNAMIC(CFixedToolTip)
public:		// Construction & destruction
	CFixedToolTip();
	virtual ~CFixedToolTip();

public:		// Operations
	bool AddWindowTool(CWnd *pWnd, LPCTSTR szText);
	bool AddRectTool(CWnd *pWnd, LPCTSTR szText, LPRECT pRect, UINT nIDTool);

protected:	// Message handlers
	DECLARE_MESSAGE_MAP()
};
