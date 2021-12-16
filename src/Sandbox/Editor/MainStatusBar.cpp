#include "StdAfx.h"
#include "MainStatusBar.h"
#include "ISourceControl.h"


static UINT s_statusBarIndicators[] =
{
	ID_SEPARATOR,
	ID_INDICATOR_SOURCE_CONTROL,
	ID_INDICATOR_BACKGROUNDACTIVITY,
	ID_INDICATOR_GAME_INFO,
	ID_INDICATOR_MEMORY
};


static const char* s_statusBarIndicatorNames[] =
{
	"",
	"source_control",
	"background_activity",
	"game_info",
	"memory"
};


BEGIN_MESSAGE_MAP(CMainStatusBar, CXTPStatusBar)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


void CMainStatusBar::Init(CWnd* pParentWnd)
{
	VERIFY(Create(pParentWnd));

	int numIndicators = sizeof(s_statusBarIndicators)/sizeof(UINT);
	VERIFY(SetIndicators(s_statusBarIndicators, numIndicators));

	GetImageManager()->SetIcon(IDI_BALL_DISABLED, IDI_BALL_DISABLED);
	GetImageManager()->SetIcon(IDI_BALL_PENDING, IDI_BALL_PENDING);
	GetImageManager()->SetIcon(IDI_BALL_OFFLINE, IDI_BALL_OFFLINE);
	GetImageManager()->SetIcon(IDI_SOURCE_CONTROL, IDI_SOURCE_CONTROL);

	for (int i=0; i<numIndicators; ++i)
	{
		if (s_statusBarIndicators[i]!=ID_SEPARATOR)
		{
			int nIndex = CommandToIndex(s_statusBarIndicators[i]);
			if (nIndex>0)
				m_indexMap[s_statusBarIndicatorNames[i]] = nIndex;
		}
	}

	Set("source_control", "", "No source control provided", IDI_SOURCE_CONTROL);

	CString strGameInfo;
	ICVar* pCVar = gEnv->pConsole->GetCVar( "sys_game_folder" );
	if (pCVar)
	{
		strGameInfo = "GameFolder: '" + ((CString)pCVar->GetString()) + "'";
	}
	pCVar = gEnv->pConsole->GetCVar( "sys_dll_game" );
	if (pCVar)
	{
		strGameInfo += " - GameDLL: '" + ((CString)pCVar->GetString()) + "'";
	}
	SetItem("game_info", strGameInfo , "Game Info", 0);
}


/////////////////////////////////////////////////////////////////////////
void CMainStatusBar::SetStatusText(const char* text)
{
	CXTPStatusBarPane* pPane = GetPane(0);
	if (!pPane)
		return;

	pPane->SetText(text);
	pPane->Redraw();
};


/////////////////////////////////////////////////////////////////////////
int CMainStatusBar::SetItem(const char* indicatorName, const char* text, const char* tip, void* pIcon)
{
	auto index = m_indexMap.find(indicatorName);
	if (index == m_indexMap.end())
		return -1;

	CXTPStatusBarPane* pPane = GetPane(index->second);
	if(!pPane)
		return -1;

	if (text)
	{
		pPane->SetText(text);
	}

	if (tip)
	{
		pPane->SetTooltip(tip);
	}

	if (pIcon)
	{
		HICON hIcon = (HICON) pIcon;

		CXTPImageManagerIcon* pImageManIcon = pPane->GetImage();
		int nWidth = pImageManIcon->GetWidth();
		CSize srcIconSize = CXTPImageManagerIcon::GetExtent(hIcon);
		if (srcIconSize.cx != nWidth)
		{
			hIcon = CXTPImageManagerIcon::ScaleToFit(hIcon, srcIconSize, nWidth);
		}
		pImageManIcon->SetIcon(hIcon);
		pPane->Redraw();
	}
	
	return index->second;
}


/////////////////////////////////////////////////////////////////////////
int CMainStatusBar::Set(const char* indicatorName, const char* text, const char* tip, int nIconIndex)
{
	int nIndex = SetItem(indicatorName, text, tip, 0);
	if(nIndex>=0)
	{
		GetPane(nIndex)->SetIconIndex(nIconIndex);
	}

	return nIndex;
}


/////////////////////////////////////////////////////////////////////////
void CMainStatusBar::OnLButtonDown(UINT nFlags, CPoint pt)
{
	__super::OnLButtonDown(nFlags, pt);

	CRect rect;
	CXTPStatusBarPane* pPane = HitTest(pt, &rect);
	if (!pPane)
		return;

	CRect wndRect;
	GetWindowRect(&wndRect);

	if (ID_INDICATOR_SOURCE_CONTROL == pPane->GetID())
	{
		enum
		{
			eEnable = 1,
			eSettings
		};

		CMenu menu;
		menu.CreatePopupMenu();
		menu.AppendMenu(MF_STRING, eEnable, gSettings.enableSourceControl ? "Disable" : "Enable");
		menu.AppendMenu(MF_STRING | (GetIEditor()->IsSourceControlAvailable() ? 0 : MF_GRAYED), eSettings, "Settings");
		int cmd = menu.TrackPopupMenu(TPM_LEFTALIGN|TPM_BOTTOMALIGN|TPM_RETURNCMD, wndRect.left + pt.x, wndRect.top + pt.y, this);
		switch(cmd)
		{
		case eEnable:
			gSettings.enableSourceControl = !gSettings.enableSourceControl;
			break;
		case eSettings:
			if(GetIEditor()->IsSourceControlAvailable())
			{
				GetIEditor()->GetSourceControl()->ShowSettings();
			}
			break;
		default:
			break;
		}
	}

}

