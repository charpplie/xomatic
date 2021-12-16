/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: TextureBrowserPanel.cpp,v 1.0 2008/08/13 11:52:18 PauloZaffari Exp wwwrun $
$DateTime$
Description:  MFC class implementing the texture browser dialog.
-------------------------------------------------------------------------
History:
- 13:08:2008   11:52 : Created by Paulo Zaffari

*************************************************************************/

#include "StdAfx.h"
#include "TextureBrowserPanel.h"
#include "../Controls/TextureViewer.h"
#include "TextureDatabaseItem.h"

//BEGIN_MESSAGE_MAP(CTextureBrowserPanel::CXTPTaskPanelSpecific,CXTResizeDialog)
//	ON_WM_HSCROLL()
//END_MESSAGE_MAP()

//CXTPCalendarReminderForOccurrence::CXTPCalendarReminderForOccurrence()
//{
//
//}
//
//CXTPCalendarReminderForOccurrence::~CXTPCalendarReminderForOccurrence()
//{
//
//}

BEGIN_MESSAGE_MAP(CTextureBrowserPanel, CXTResizeDialog)
	ON_WM_CLOSE()
	ON_WM_HSCROLL()
	ON_MESSAGE(					XTPWM_TASKPANEL_NOTIFY			, &CTextureBrowserPanel::OnTaskPanelNotify)

	ON_EN_CHANGE(				eFilenameEdit								,	&CTextureBrowserPanel::OnFilenameChangeEdit)

	ON_CBN_SELCHANGE(		eTextureSizeHorizontalMinimum			,	&CTextureBrowserPanel::OnCbnSelchangeTextureBrowserTextureSizeXMinimumCombo)
	ON_CBN_EDITCHANGE(	eTextureSizeHorizontalMinimum			, &CTextureBrowserPanel::OnCbnEditchangeTextureBrowserTextureSizeXMinimumCombo)
	ON_CBN_SELCHANGE(		eTextureSizeHorizontalMaximum			,	&CTextureBrowserPanel::OnCbnSelchangeTextureBrowserTextureSizeXMaximumCombo)
	ON_CBN_EDITCHANGE(	eTextureSizeHorizontalMaximum			, &CTextureBrowserPanel::OnCbnEditchangeTextureBrowserTextureSizeXMaximumCombo)

	ON_CBN_SELCHANGE(		eTextureSizeVerticalMinimum				,	&CTextureBrowserPanel::OnCbnSelchangeTextureBrowserTextureSizeYMinimumCombo)
	ON_CBN_EDITCHANGE(	eTextureSizeVerticalMinimum				,	&CTextureBrowserPanel::OnCbnEditchangeTextureBrowserTextureSizeYMinimumCombo)
	ON_CBN_SELCHANGE(		eTextureSizeVerticalMaximum				,	&CTextureBrowserPanel::OnCbnSelchangeTextureBrowserTextureSizeYMaximumCombo)
	ON_CBN_EDITCHANGE(	eTextureSizeVerticalMaximum				,	&CTextureBrowserPanel::OnCbnEditchangeTextureBrowserTextureSizeYMaximumCombo)

	//ON_BN_CLICKED(			eTextureSemanticsDiffuse		,	&CTextureBrowserPanel::OnBnClickedTextureBrowserSemanticsDiffuseCheck)
	//ON_BN_CLICKED(			eTextureSemanticsSpecular		,	&CTextureBrowserPanel::OnBnClickedTextureBrowserSemanticsSpecularCheck)
	//ON_BN_CLICKED(			eTextureSemanticsBump				,	&CTextureBrowserPanel::OnBnClickedTextureBrowserSemanticsBumpCheck)
	//ON_BN_CLICKED(			eTextureSemanticsCubemap		,	&CTextureBrowserPanel::OnBnClickedTextureBrowserSemanticsCubemapCheck)
	//ON_BN_CLICKED(			eTextureFileDDS							,	&CTextureBrowserPanel::OnBnClickedTextureBrowserFiletypeDDSCheck)
	//ON_BN_CLICKED(			eTextureFileTIF							,	&CTextureBrowserPanel::OnBnClickedTextureBrowserFiletypeTIFCheck)
	//ON_BN_CLICKED(			eTextureFileUsedInLevel			,	&CTextureBrowserPanel::OnBnClickedTextureBrowserUsedInLevelCheck)
END_MESSAGE_MAP()

BOOL CTextureBrowserPanel::CXTPTaskPanelSpecific::OnCommand(WPARAM wParam,LPARAM lParam)
{
	switch(LOWORD(wParam))
	{
	case eTextureSemanticsDiffuse :
		{

			m_poMyOwner->OnBnClickedTextureBrowserSemanticsDiffuseCheck();
			return TRUE;
		}
		break;

	case eTextureSemanticsSpecular:
		{
			m_poMyOwner->OnBnClickedTextureBrowserSemanticsSpecularCheck();
			return TRUE;
		}
		break;

	case eTextureSemanticsBump:
		{
			m_poMyOwner->OnBnClickedTextureBrowserSemanticsBumpCheck();
			return TRUE;
		}
		break;

	case eTextureSemanticsCubemap:
		{
			m_poMyOwner->OnBnClickedTextureBrowserSemanticsCubemapCheck();
			return TRUE;
		}
		break;

	case eTextureFileDDS:
		{
			m_poMyOwner->OnBnClickedTextureBrowserFiletypeDDSCheck();
			return TRUE;
		}
		break;

	case eTextureFileTIF:
		{
			m_poMyOwner->OnBnClickedTextureBrowserFiletypeTIFCheck();
			return TRUE;
		}
		break;

	case eTextureFileUsedInLevel:
		{
			m_poMyOwner->OnBnClickedTextureBrowserUsedInLevelCheck();
			return TRUE;
		}
		break;

		case eTextureHasAlphaChannel:
		{
			m_poMyOwner->OnBnClickedTextureBrowserHasAlphaChanelCheck();
			return TRUE;
		}
		break;

		case eTextureAlphaChannel:
			{
				m_poMyOwner->OnBnClickedTextureBrowserShowAlphaChanelCheck();
				return TRUE;
			}
		break;
	}

	return __super::OnCommand(wParam,lParam);
}

LRESULT CTextureBrowserPanel::CXTPTaskPanelSpecific::WindowProc(UINT message,WPARAM wParam,LPARAM lParam)
{
	switch(message)
	{
		case WM_HSCROLL:
			{
					CScrollBar	oScrollBar,*poScrollBarPointer(NULL);
					if (lParam!=NULL)
					{
						oScrollBar.FromHandle((HWND)lParam);
						poScrollBarPointer=&oScrollBar;
					}					
					//__super::OnHScroll(HIWORD(wParam),LOWORD(wParam),poScrollBarPointer);
					m_poMyOwner->OnHScroll(HIWORD(wParam),LOWORD(wParam),poScrollBarPointer);
					return 0;
			}
		break;
	}
	return __super::WindowProc(message,wParam,lParam);
}

CTextureBrowserPanel::CTextureBrowserPanel(CTextureViewer* poSettingsOwner,CWnd* pParent)
:CXTResizeDialog(CTextureBrowserPanel::IDD, pParent),m_poSettingsOwner(poSettingsOwner)
{
	Create( IDD,pParent );
	m_wndTaskPanel.SetOwner(this);
}

CTextureBrowserPanel::~CTextureBrowserPanel()
{

}

void CTextureBrowserPanel::SetFilenameDisplay(const char* szFilename)
{
	string strOutputString("");
	string strHelperString(szFilename);

	// This will guarantee we will keep the same size.
	if (strHelperString.empty())
	{
		strHelperString="No selection";
	}

	strOutputString+=strHelperString;
	m_apoDisplayItems[EGroupItemFilename]->SetCaption(strOutputString.c_str());
}

void CTextureBrowserPanel::SetSurfaceTypeDisplay(const char* szSurfaceType)
{
	string strOutputString("Type: ");
	string strHelperString(szSurfaceType);

	strOutputString+=strHelperString;
	m_apoDisplayItems[EGroupItemSurfaceType]->SetCaption(strOutputString.c_str());
}

void CTextureBrowserPanel::SetNumberOfMipsDisplay( const char* szNumberOfMips )
{
	string strOutputString("Mips: ");
	string strHelperString(szNumberOfMips);

	strOutputString+=strHelperString;
	m_apoDisplayItems[EGroupItemNumberOfMips]->SetCaption(strOutputString.c_str());
}

void CTextureBrowserPanel::SetFileSizeDisplay(const char*  szFileSizeDisplay)
{
	string strOutputString("Size: ");
	string strHelperString(szFileSizeDisplay);

	strOutputString+=strHelperString;
	m_apoDisplayItems[EGroupItemFileSize]->SetCaption(strOutputString.c_str());
}

void CTextureBrowserPanel::SetTextureResolutionDisplay(const char*  szResolution)
{
	string strOutputString("Resolution: ");
	string strHelperString(szResolution);

	strOutputString+=strHelperString;
	m_apoDisplayItems[EGroupItemTextureResolution]->SetCaption(strOutputString.c_str());
}

void CTextureBrowserPanel::ClearFilters()
{
	m_aoTextureSemanticsCheckBoxes[EShowOnlyDiffuseSemantics].SetCheck(FALSE);
	m_poSettingsOwner->FilterShowSemanticsDiffuseOnly(false);

	m_aoTextureSemanticsCheckBoxes[EShowOnlySpecularSemantics].SetCheck(FALSE);
	m_poSettingsOwner->FilterShowSemanticsSpecularOnly(false);

	m_aoTextureSemanticsCheckBoxes[EShowOnlyBumpSemantics].SetCheck(FALSE);
	m_poSettingsOwner->FilterShowSemanticsBumpOnly(false);

	m_aoTextureSemanticsCheckBoxes[EShowOnlyCubemapSemantics].SetCheck(FALSE);
	m_poSettingsOwner->FilterShowSemanticsCubemapOnly(false);

	m_aoTextureSizeComboBoxes[EMinimumWidth].SelectString(0,"Any");
	m_poSettingsOwner->FilterWithMinimum(0);

	m_aoTextureSizeComboBoxes[EMaximumWidth].SelectString(0,"Any");
	m_poSettingsOwner->FilterWithMaximum(INT_MAX);

	m_aoTextureSizeComboBoxes[EMinimumHeight].SelectString(0,"Any");
	m_poSettingsOwner->FilterHeightMinimum(0);

	m_aoTextureSizeComboBoxes[EMaximumHeight].SelectString(0,"Any");
	m_poSettingsOwner->FilterHeightMaximum(INT_MAX);

	m_oFilenameEdit.SetWindowText("");
	m_poSettingsOwner->FilterByName((string)"");
}

void CTextureBrowserPanel::DoDataExchange(CDataExchange* pDX)    // DDX/DDV support
{
	CXTResizeDialog::DoDataExchange(pDX);
}

BOOL CTextureBrowserPanel::OnInitDialog()
{
	BOOL bnReturnValue=__super::OnInitDialog();

	CRect rc(0,0,200,1100);
	m_wndTaskPanel.Create( WS_CHILD|WS_BORDER|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,rc,this,eTaskPanel );
	m_wndTaskPanel.SetBehaviour(xtpTaskPanelBehaviourExplorer);
	m_wndTaskPanel.SetTheme(xtpTaskPanelThemeNativeWinXP);
	m_wndTaskPanel.SetSelectItemOnFocus(TRUE);
	m_wndTaskPanel.AllowDrag(TRUE);
	m_wndTaskPanel.SetAnimation(xtpTaskPanelAnimationNo);

	////////////////////////////////////////////////////////////////////////////
	m_aoTextureSemanticsCheckBoxes[EShowOnlyDiffuseSemantics].Create( "Show Only Diffuse",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,CRect(0,0,60,16),this,eTextureSemanticsDiffuse);
	m_aoTextureSemanticsCheckBoxes[EShowOnlyDiffuseSemantics].SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
	m_aoTextureSemanticsCheckBoxes[EShowOnlyDiffuseSemantics].SetParent(&m_wndTaskPanel); m_aoTextureSemanticsCheckBoxes[EShowOnlyDiffuseSemantics].SetOwner(this); // Akward but needed to route msgs to this dialog
	m_aoTextureSemanticsCheckBoxes[EShowOnlyDiffuseSemantics].SetCheck(FALSE);

	m_aoTextureSemanticsCheckBoxes[EShowOnlySpecularSemantics].Create( "Show Only Specular",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,CRect(0,0,60,16),this,eTextureSemanticsSpecular);
	m_aoTextureSemanticsCheckBoxes[EShowOnlySpecularSemantics].SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
	m_aoTextureSemanticsCheckBoxes[EShowOnlySpecularSemantics].SetParent(&m_wndTaskPanel); m_aoTextureSemanticsCheckBoxes[EShowOnlySpecularSemantics].SetOwner(this); // Akward but needed to route msgs to this dialog
	m_aoTextureSemanticsCheckBoxes[EShowOnlySpecularSemantics].SetCheck(FALSE);

	m_aoTextureSemanticsCheckBoxes[EShowOnlyBumpSemantics].Create( "Show Only Bump",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,CRect(0,0,60,16),this,eTextureSemanticsBump);
	m_aoTextureSemanticsCheckBoxes[EShowOnlyBumpSemantics].SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
	m_aoTextureSemanticsCheckBoxes[EShowOnlyBumpSemantics].SetParent(&m_wndTaskPanel); m_aoTextureSemanticsCheckBoxes[EShowOnlyBumpSemantics].SetOwner(this); // Akward but needed to route msgs to this dialog
	m_aoTextureSemanticsCheckBoxes[EShowOnlyBumpSemantics].SetCheck(FALSE);

	m_aoTextureSemanticsCheckBoxes[EShowOnlyCubemapSemantics].Create( "Show Only Cube Map",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,CRect(0,0,60,16),this,eTextureSemanticsCubemap);
	m_aoTextureSemanticsCheckBoxes[EShowOnlyCubemapSemantics].SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
	m_aoTextureSemanticsCheckBoxes[EShowOnlyCubemapSemantics].SetParent(&m_wndTaskPanel); m_aoTextureSemanticsCheckBoxes[EShowOnlyCubemapSemantics].SetOwner(this); // Akward but needed to route msgs to this dialog
	m_aoTextureSemanticsCheckBoxes[EShowOnlyCubemapSemantics].SetCheck(FALSE);

	////////////////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////////////////
	m_aoTextureFileCheckBoxes[EShowOnlyDDSFiles].Create( "Show Only DDS",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,CRect(0,0,60,16),this,eTextureFileDDS);
	m_aoTextureFileCheckBoxes[EShowOnlyDDSFiles].SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
	m_aoTextureFileCheckBoxes[EShowOnlyDDSFiles].SetParent(&m_wndTaskPanel); m_aoTextureFileCheckBoxes[EShowOnlyDDSFiles].SetOwner(this); // Akward but needed to route msgs to this dialog
	m_aoTextureFileCheckBoxes[EShowOnlyDDSFiles].SetCheck(FALSE);

	m_aoTextureFileCheckBoxes[EShowOnlyTIFFiles].Create( "Show Only TIF",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,CRect(0,0,60,16),this,eTextureFileTIF);
	m_aoTextureFileCheckBoxes[EShowOnlyTIFFiles].SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
	m_aoTextureFileCheckBoxes[EShowOnlyTIFFiles].SetParent(&m_wndTaskPanel); m_aoTextureFileCheckBoxes[EShowOnlyTIFFiles].SetOwner(this); // Akward but needed to route msgs to this dialog
	m_aoTextureFileCheckBoxes[EShowOnlyTIFFiles].SetCheck(FALSE);

	m_aoTextureFileCheckBoxes[EShowOnlyUsedInLevelFiles].Create( "Used in Level",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,CRect(0,0,60,16),this,eTextureFileUsedInLevel);
	m_aoTextureFileCheckBoxes[EShowOnlyUsedInLevelFiles].SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
	m_aoTextureFileCheckBoxes[EShowOnlyUsedInLevelFiles].SetParent(&m_wndTaskPanel); m_aoTextureSemanticsCheckBoxes[EShowOnlyUsedInLevelFiles].SetOwner(this); // Akward but needed to route msgs to this dialog
	m_aoTextureFileCheckBoxes[EShowOnlyUsedInLevelFiles].SetCheck(FALSE);

	m_aoTextureFileCheckBoxes[EShowOnlyIfHasAlphaChannel].Create( "Has Alpha Channel",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,CRect(0,0,60,16),this,eTextureHasAlphaChannel);
	m_aoTextureFileCheckBoxes[EShowOnlyIfHasAlphaChannel].SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
	m_aoTextureFileCheckBoxes[EShowOnlyIfHasAlphaChannel].SetParent(&m_wndTaskPanel); m_aoTextureSemanticsCheckBoxes[EShowOnlyIfHasAlphaChannel].SetOwner(this); // Akward but needed to route msgs to this dialog
	m_aoTextureFileCheckBoxes[EShowOnlyIfHasAlphaChannel].SetCheck(FALSE);


	m_aoTextureFileCheckBoxes[EShowOnlyAlphaChannel].Create( "Show Alpha Channel",WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,CRect(0,0,60,16),this,eTextureAlphaChannel);
	m_aoTextureFileCheckBoxes[EShowOnlyAlphaChannel].SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
	m_aoTextureFileCheckBoxes[EShowOnlyAlphaChannel].SetParent(&m_wndTaskPanel); m_aoTextureSemanticsCheckBoxes[EShowOnlyAlphaChannel].SetOwner(this); // Akward but needed to route msgs to this dialog
	m_aoTextureFileCheckBoxes[EShowOnlyAlphaChannel].SetCheck(FALSE);
	////////////////////////////////////////////////////////////////////////////	

	////////////////////////////////////////////////////////////////////////////	
	m_oFilenameEdit.Create( WS_CHILD|WS_VISIBLE,CRect(0,0,60,16),this,eFilenameEdit);
	m_oFilenameEdit.SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
	m_oFilenameEdit.SetParent(&m_wndTaskPanel); m_oFilenameEdit.SetOwner(this); // Akward but needed to route msgs to this dialog
	////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////	
	m_aoTextureSizeComboBoxes[EMinimumWidth].Create( WS_CHILD|WS_VISIBLE|CBS_DROPDOWN,CRect(0,0,60,16),this,eTextureSizeHorizontalMinimum);
	m_aoTextureSizeComboBoxes[EMinimumWidth].SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
	m_aoTextureSizeComboBoxes[EMinimumWidth].SetParent(&m_wndTaskPanel); m_aoTextureSizeComboBoxes[EMinimumWidth].SetOwner(this); // Akward but needed to route msgs to this dialog
	m_aoTextureSizeComboBoxes[EMinimumWidth].AddString("Any");
	m_aoTextureSizeComboBoxes[EMinimumWidth].AddString("1");
	m_aoTextureSizeComboBoxes[EMinimumWidth].AddString("2");
	m_aoTextureSizeComboBoxes[EMinimumWidth].AddString("4");
	m_aoTextureSizeComboBoxes[EMinimumWidth].AddString("8");
	m_aoTextureSizeComboBoxes[EMinimumWidth].AddString("16");
	m_aoTextureSizeComboBoxes[EMinimumWidth].AddString("32");
	m_aoTextureSizeComboBoxes[EMinimumWidth].AddString("64");
	m_aoTextureSizeComboBoxes[EMinimumWidth].AddString("128");
	m_aoTextureSizeComboBoxes[EMinimumWidth].AddString("256");
	m_aoTextureSizeComboBoxes[EMinimumWidth].AddString("512");
	m_aoTextureSizeComboBoxes[EMinimumWidth].AddString("1024");
	m_aoTextureSizeComboBoxes[EMinimumWidth].AddString("2048");
	m_aoTextureSizeComboBoxes[EMinimumWidth].AddString("4096");
	m_aoTextureSizeComboBoxes[EMinimumWidth].AddString("8192");
	m_aoTextureSizeComboBoxes[EMinimumWidth].SelectString(0,"Any");

	m_aoTextureSizeComboBoxes[EMaximumWidth].Create( WS_CHILD|WS_VISIBLE|CBS_DROPDOWN,CRect(0,0,60,16),this,eTextureSizeHorizontalMaximum);
	m_aoTextureSizeComboBoxes[EMaximumWidth].SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
	m_aoTextureSizeComboBoxes[EMaximumWidth].SetParent(&m_wndTaskPanel); m_aoTextureSizeComboBoxes[EMaximumWidth].SetOwner(this); // Akward but needed to route msgs to this dialog
	m_aoTextureSizeComboBoxes[EMaximumWidth].AddString("Any");
	m_aoTextureSizeComboBoxes[EMaximumWidth].AddString("1");
	m_aoTextureSizeComboBoxes[EMaximumWidth].AddString("2");
	m_aoTextureSizeComboBoxes[EMaximumWidth].AddString("4");
	m_aoTextureSizeComboBoxes[EMaximumWidth].AddString("8");
	m_aoTextureSizeComboBoxes[EMaximumWidth].AddString("16");
	m_aoTextureSizeComboBoxes[EMaximumWidth].AddString("32");
	m_aoTextureSizeComboBoxes[EMaximumWidth].AddString("64");
	m_aoTextureSizeComboBoxes[EMaximumWidth].AddString("128");
	m_aoTextureSizeComboBoxes[EMaximumWidth].AddString("256");
	m_aoTextureSizeComboBoxes[EMaximumWidth].AddString("512");
	m_aoTextureSizeComboBoxes[EMaximumWidth].AddString("1024");
	m_aoTextureSizeComboBoxes[EMaximumWidth].AddString("2048");
	m_aoTextureSizeComboBoxes[EMaximumWidth].AddString("4096");
	m_aoTextureSizeComboBoxes[EMaximumWidth].AddString("8192");
	m_aoTextureSizeComboBoxes[EMaximumWidth].SelectString(0,"Any");

	m_aoTextureSizeComboBoxes[EMinimumHeight].Create( WS_CHILD|WS_VISIBLE|CBS_DROPDOWN,CRect(0,0,60,16),this,eTextureSizeVerticalMinimum);
	m_aoTextureSizeComboBoxes[EMinimumHeight].SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
	m_aoTextureSizeComboBoxes[EMinimumHeight].SetParent(&m_wndTaskPanel); m_aoTextureSizeComboBoxes[EMinimumHeight].SetOwner(this); // Akward but needed to route msgs to this dialog
	m_aoTextureSizeComboBoxes[EMinimumHeight].AddString("Any");
	m_aoTextureSizeComboBoxes[EMinimumHeight].AddString("1");
	m_aoTextureSizeComboBoxes[EMinimumHeight].AddString("2");
	m_aoTextureSizeComboBoxes[EMinimumHeight].AddString("4");
	m_aoTextureSizeComboBoxes[EMinimumHeight].AddString("8");
	m_aoTextureSizeComboBoxes[EMinimumHeight].AddString("16");
	m_aoTextureSizeComboBoxes[EMinimumHeight].AddString("32");
	m_aoTextureSizeComboBoxes[EMinimumHeight].AddString("64");
	m_aoTextureSizeComboBoxes[EMinimumHeight].AddString("128");
	m_aoTextureSizeComboBoxes[EMinimumHeight].AddString("256");
	m_aoTextureSizeComboBoxes[EMinimumHeight].AddString("512");
	m_aoTextureSizeComboBoxes[EMinimumHeight].AddString("1024");
	m_aoTextureSizeComboBoxes[EMinimumHeight].AddString("2048");
	m_aoTextureSizeComboBoxes[EMinimumHeight].AddString("4096");
	m_aoTextureSizeComboBoxes[EMinimumHeight].AddString("8192");
	m_aoTextureSizeComboBoxes[EMinimumHeight].SelectString(0,"Any");

	m_aoTextureSizeComboBoxes[EMaximumHeight].Create( WS_CHILD|WS_VISIBLE|CBS_DROPDOWN,CRect(0,0,60,16),this,eTextureSizeVerticalMaximum);
	m_aoTextureSizeComboBoxes[EMaximumHeight].SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
	m_aoTextureSizeComboBoxes[EMaximumHeight].SetParent(&m_wndTaskPanel); m_aoTextureSizeComboBoxes[EMaximumHeight].SetOwner(this); // Akward but needed to route msgs to this dialog
	m_aoTextureSizeComboBoxes[EMaximumHeight].AddString("Any");
	m_aoTextureSizeComboBoxes[EMaximumHeight].AddString("1");
	m_aoTextureSizeComboBoxes[EMaximumHeight].AddString("2");
	m_aoTextureSizeComboBoxes[EMaximumHeight].AddString("4");
	m_aoTextureSizeComboBoxes[EMaximumHeight].AddString("8");
	m_aoTextureSizeComboBoxes[EMaximumHeight].AddString("16");
	m_aoTextureSizeComboBoxes[EMaximumHeight].AddString("32");
	m_aoTextureSizeComboBoxes[EMaximumHeight].AddString("64");
	m_aoTextureSizeComboBoxes[EMaximumHeight].AddString("128");
	m_aoTextureSizeComboBoxes[EMaximumHeight].AddString("256");
	m_aoTextureSizeComboBoxes[EMaximumHeight].AddString("512");
	m_aoTextureSizeComboBoxes[EMaximumHeight].AddString("1024");
	m_aoTextureSizeComboBoxes[EMaximumHeight].AddString("2048");
	m_aoTextureSizeComboBoxes[EMaximumHeight].AddString("4096");
	m_aoTextureSizeComboBoxes[EMaximumHeight].AddString("8192");
	m_aoTextureSizeComboBoxes[EMaximumHeight].SelectString(0,"Any");
	////////////////////////////////////////////////////////////////////////////	

	m_oTextureCellSizeSlider.Create(WS_CHILD|WS_VISIBLE|TBS_HORZ,CRect(0,0,60,16),this,eTextureCellSizeSlider);
	m_oTextureCellSizeSlider.SetFont( CFont::FromHandle(gSettings.gui.hSystemFont) );
	m_oTextureCellSizeSlider.SetParent(&m_wndTaskPanel); m_aoTextureFileCheckBoxes[EShowOnlyDDSFiles].SetOwner(this); // Akward but needed to route msgs to this dialog
	m_oTextureCellSizeSlider.SetRange(0,100);
	// The cell size works as a porcentage. This is why we have to multiply it by 100.
	m_oTextureCellSizeSlider.SetPos(((gSettings.sTextureBrowserSettings.nCellSize-MINIMUNCELLSIZE)/(float)MAXIMUMCELLSIZEMULTIPLIER)*100.0f);

	// Add default tasks.
	CXTPTaskPanelGroupItem	*pItem =  NULL;
	CXTPTaskPanelGroup			*pGroup = NULL;

	{
		pGroup = m_wndTaskPanel.AddGroup(1);
		pGroup->SetCaption( "Texture Information" );

		// Only for filenames we use 2 lines since filenames can be long strings...
		// ...(in a display point of view)...in order to avoid having to run an
		// algorithm to cut the strings ourselves, which could be not so simple
		// with the XTP Toolkit, we use a new lne which will do everything for us.
		pItem =  pGroup->AddTextItem("Filename:");
		pItem =  pGroup->AddTextItem("No selection");// Filaname...
		m_apoDisplayItems[EGroupItemFilename]=pItem;

		pItem =  pGroup->AddTextItem("Type:");
		//pItem =  pGroup->AddTextItem(" ");// DXT1,etc...
		m_apoDisplayItems[EGroupItemSurfaceType]=pItem;

		pItem =  pGroup->AddTextItem("Mips:");
		//pItem =  pGroup->AddTextItem(" "); // 9
		m_apoDisplayItems[EGroupItemNumberOfMips]=pItem;

		pItem =  pGroup->AddTextItem("Size:");
		//pItem =  pGroup->AddTextItem(" "); // File size
		m_apoDisplayItems[EGroupItemFileSize]=pItem;

		pItem =  pGroup->AddTextItem("Resolution:");
		//pItem =  pGroup->AddTextItem(" "); // Resolution 512 x 512 
		m_apoDisplayItems[EGroupItemTextureResolution]=pItem;
	}

	{
		pGroup = m_wndTaskPanel.AddGroup(2);
		pGroup->SetCaption("Filename");
		pItem =  pGroup->AddControlItem(m_oFilenameEdit);
	}

	{
		pGroup = m_wndTaskPanel.AddGroup(3);
		pGroup->SetCaption( "Texture Dimensions" );

		pItem =  pGroup->AddTextItem("Minimum Width:");
		pItem =  pGroup->AddControlItem(m_aoTextureSizeComboBoxes[EMinimumWidth]);

		pItem =  pGroup->AddTextItem("Maximum Width:");
		pItem =  pGroup->AddControlItem(m_aoTextureSizeComboBoxes[EMaximumWidth]);

		pItem =  pGroup->AddTextItem("Minimum Height:");
		pItem =  pGroup->AddControlItem(m_aoTextureSizeComboBoxes[EMinimumHeight]);

		pItem =  pGroup->AddTextItem("Maximum Height:");
		pItem =  pGroup->AddControlItem(m_aoTextureSizeComboBoxes[EMaximumHeight]);
	}

	{
		pGroup = m_wndTaskPanel.AddGroup(4);
		pGroup->SetCaption( "Texture Semantics" );

		pItem =  pGroup->AddControlItem(m_aoTextureSemanticsCheckBoxes[EShowOnlyDiffuseSemantics]);
		pItem =  pGroup->AddControlItem(m_aoTextureSemanticsCheckBoxes[EShowOnlySpecularSemantics]);
		pItem =  pGroup->AddControlItem(m_aoTextureSemanticsCheckBoxes[EShowOnlyBumpSemantics]);
		pItem =  pGroup->AddControlItem(m_aoTextureSemanticsCheckBoxes[EShowOnlyCubemapSemantics]);
	}

	{
		pGroup = m_wndTaskPanel.AddGroup(5);
		pGroup->SetCaption( "Texture Files" );

		pItem =  pGroup->AddControlItem(m_aoTextureFileCheckBoxes[EShowOnlyDDSFiles]);
		pItem =  pGroup->AddControlItem(m_aoTextureFileCheckBoxes[EShowOnlyTIFFiles]);
		pItem =  pGroup->AddControlItem(m_aoTextureFileCheckBoxes[EShowOnlyUsedInLevelFiles]);
		pItem =  pGroup->AddControlItem(m_aoTextureFileCheckBoxes[EShowOnlyIfHasAlphaChannel]);
		pItem =  pGroup->AddControlItem(m_aoTextureFileCheckBoxes[EShowOnlyAlphaChannel]);
	}

	{
		pGroup = m_wndTaskPanel.AddGroup(6);
		pGroup->SetCaption( "Display Options" );

		pItem =  pGroup->AddTextItem("Texture Cell Size");
		pItem =  pGroup->AddControlItem(m_oTextureCellSizeSlider);		
	}

	{
		pGroup = m_wndTaskPanel.AddGroup(7);
		pGroup->SetCaption( "Operations" );

		pItem =  pGroup->AddLinkItem(eOpenFolderLink); pItem->SetType(xtpTaskItemTypeLink); pItem->SetCaption( "Open Folder" );
		pItem =  pGroup->AddLinkItem(ePreviewLink); pItem->SetType(xtpTaskItemTypeLink); pItem->SetCaption( "Preview" );
	}

	return bnReturnValue;
}

void CTextureBrowserPanel::OnClose()
{	
	DestroyWindow();
}

void CTextureBrowserPanel::OnHScroll( UINT nSBCode,UINT nPos,CScrollBar* pScrollBar )
{
	__super::OnHScroll(nSBCode,nPos,pScrollBar);

  // Get the current position of scroll bar / trackbark (Slider).
	int curpos = m_oTextureCellSizeSlider.GetPos();

	m_poSettingsOwner->SetCellSize(curpos);
}

void CTextureBrowserPanel::OnFilenameChangeEdit()
{
	if (!m_poSettingsOwner)
	{
		return;
	}

	CString strWindowText;
	string  strWindowTextCopy;
	m_oFilenameEdit.GetWindowText(strWindowText);

	strWindowTextCopy=strWindowText;

	if (!strWindowTextCopy.empty())
	{
		if (strWindowTextCopy[0]!='*')
		{
			strWindowTextCopy.insert(0,'*');
		}
		if (strWindowTextCopy[strWindowTextCopy.size()-1]!='*')
		{
			strWindowTextCopy.push_back('*');
		}
	}

	m_poSettingsOwner->FilterByName(strWindowTextCopy);
}

void CTextureBrowserPanel::OnBnClickedTextureBrowserOpenFolderButton()
{
	CTextureViewer::TDTextureDatabase	cpoSelectedItems;
	// As we want to open each folder containing selected textures once
	// and only once, this set guarantees we will have them stored once and only once.
	std::set<string>									cstrFolderSet;
	std::set<string>::iterator				itFirstIterador,itEndIterator;
	string														strExplorer("explorer ");
	string														strCommandLine;
	CString														strTargetDirectory(".\\");
	string                       strFinalTargetDirectory;

	STARTUPINFO												si;
	PROCESS_INFORMATION								pi;

	BOOL                              bnFailedToOpenAFolder(FALSE);

	intptr_t													hFindHandle(NULL);
	_finddata_t												stFileInfo;
	bool															boFailedToOpenFiles(false);
 

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);

	ZeroMemory( &pi, sizeof(pi) );

	m_poSettingsOwner->GetSelectedItems(cpoSelectedItems);

	int				nTotalTextures(cpoSelectedItems.size());
	int				nCurrentTexture(0);
	string		strFolerToOpen;
	int       nStringLenght(0);

	if (cpoSelectedItems.empty())
	{
		return;
	}

	for (nCurrentTexture=0;nCurrentTexture<nTotalTextures;++nCurrentTexture)
	{
		CTextureDatabaseItem*&	rpoTextureDatabaseItem=cpoSelectedItems[nCurrentTexture];

		// Preparing folder serach... so we won't try to open invalid folders.
		rpoTextureDatabaseItem->GetRelativePath(strFolerToOpen);
		strTargetDirectory=strFolerToOpen;
		strFinalTargetDirectory=Path::GamePathToFullPath(strTargetDirectory,true);

		CString	strTemporaryFinalTargetDirectory(strFinalTargetDirectory);
		Path::ConvertSlashToBackSlash(strTemporaryFinalTargetDirectory);
		strFinalTargetDirectory=strTemporaryFinalTargetDirectory;

		nStringLenght=strFinalTargetDirectory.size();
		// Find first will not find a directory unless it has no backslash...
		// so in case there is one, we remove it.
		if (nStringLenght>0)
		{
			if (strFinalTargetDirectory[nStringLenght-1]=='\\')
			{
				strFinalTargetDirectory.erase(nStringLenght-1);
			}		
		}


		// We have to check for file existence BEFORE adding the quotes to the filename or
		// find first will fail.
		hFindHandle=_findfirst(strFinalTargetDirectory.c_str(),&stFileInfo);
		if (hFindHandle==-1)
		{
			boFailedToOpenFiles=true;
			continue;
		}
		else
		{
			_findclose(hFindHandle);
		}

		cstrFolderSet.insert(strFolerToOpen);
	}	

	if (cstrFolderSet.size()>10)
	{
		if (MessageBox("Are you sure you want to open more than 10 folders at a single time?","Trying to open many folders",MB_YESNO)==IDNO)
		{
			return;
		}
	}

	itEndIterator=cstrFolderSet.end();
	for (itFirstIterador=cstrFolderSet.begin();itFirstIterador!=itEndIterator;itFirstIterador++)
	{
		strCommandLine=strExplorer;
		strTargetDirectory=*itFirstIterador;
		Path::AddPathSlash(strTargetDirectory);
		strFinalTargetDirectory=Path::GamePathToFullPath(strTargetDirectory,true);

		CString	strTemporaryFinalTargetDirectory(strFinalTargetDirectory);
		Path::ConvertSlashToBackSlash(strTemporaryFinalTargetDirectory);
		Path::SurroundWithQuotes(strTemporaryFinalTargetDirectory);
		strFinalTargetDirectory=strTemporaryFinalTargetDirectory;

		strCommandLine+=strFinalTargetDirectory.c_str();

		bnFailedToOpenAFolder=
			bnFailedToOpenAFolder
			||
			(!CreateProcess( NULL,// No module name (use command line). 
				(char*)strCommandLine.c_str(),				// Command line. 
				NULL,																	// Process handle not inheritable. 
				NULL,																	// Thread handle not inheritable. 
				TRUE,																	// Set handle inheritance to TRUE. 
				0,																		// creation flags. 
				NULL,																	// Use parent's environment block. 
				NULL,//szWorkingDirectory?szWorkingDirectory:szRemoteDirectory,			// Set starting directory. 
				&si,																	// Pointer to STARTUPINFO structure.
				&pi )
			);																			// Pointer to PROCESS_INFORMATION structure.		
	}

	if ((boFailedToOpenFiles)||(bnFailedToOpenAFolder))
	{
		MessageBox("Could not open all the folders for the selected textures.","Warning",MB_OK);
	}
}

void CTextureBrowserPanel::OnCbnSelchangeTextureBrowserTextureSizeXMinimumCombo()
{
	if (!m_poSettingsOwner)
	{
		return;
	}

	// Initialized with the invalid filter.
	int         nTextureWidthFilter(-1);
	int         nCurrentSelection(-1);
	int         nOperationResult(0);
	CString			strCurrentText;
	char				*szNumericString(NULL);

	nCurrentSelection=m_aoTextureSizeComboBoxes[EMinimumWidth].GetCurSel();
	if (nCurrentSelection==CB_ERR)
	{
		return;
	}

	m_aoTextureSizeComboBoxes[EMinimumWidth].GetLBText(nCurrentSelection,strCurrentText);
	szNumericString=strCurrentText.GetBuffer();
	// In case we failed to parse a number, we don't need to 
	// set a new filter.
	if (sscanf(szNumericString,"%d",&nTextureWidthFilter)<=0)
	{
		nTextureWidthFilter=0;
	}
	m_poSettingsOwner->FilterWithMinimum(nTextureWidthFilter);	
}

void CTextureBrowserPanel::OnCbnEditchangeTextureBrowserTextureSizeXMinimumCombo()
{
	if (!m_poSettingsOwner)
	{
		return;
	}

	// Initialized with the invalid filter.
	int         nTextureWidthFilter(-1);
	CString			strCurrentText;
	char				*szNumericString(NULL);


	m_aoTextureSizeComboBoxes[EMinimumWidth].GetWindowText(strCurrentText);
	szNumericString=strCurrentText.GetBuffer();

	// In case we failed to parse a number, we don't need to 
	// set a new filter.
	if (sscanf(szNumericString,"%d",&nTextureWidthFilter)<=0)
	{
		nTextureWidthFilter=0;
	}
	m_poSettingsOwner->FilterWithMinimum(nTextureWidthFilter);
}

void CTextureBrowserPanel::OnCbnSelchangeTextureBrowserTextureSizeXMaximumCombo()
{
	if (!m_poSettingsOwner)
	{
		return;
	}

	// Initialized with the invalid filter.
	int         nTextureWidthFilter(-1);
	int         nCurrentSelection(-1);
	int         nOperationResult(0);
	CString			strCurrentText;
	char				*szNumericString(NULL);

	nCurrentSelection=m_aoTextureSizeComboBoxes[EMaximumWidth].GetCurSel();
	if (nCurrentSelection==CB_ERR)
	{
		return;
	}

	m_aoTextureSizeComboBoxes[EMaximumWidth].GetLBText(nCurrentSelection,strCurrentText);
	szNumericString=strCurrentText.GetBuffer();
	// In case we failed to parse a number, we don't need to 
	// set a new filter.
	if (sscanf(szNumericString,"%d",&nTextureWidthFilter)<=0)
	{
		nTextureWidthFilter=INT_MAX;
	}
	m_poSettingsOwner->FilterWithMaximum(nTextureWidthFilter);	
}

void CTextureBrowserPanel::OnCbnEditchangeTextureBrowserTextureSizeXMaximumCombo()
{
	if (!m_poSettingsOwner)
	{
		return;
	}

	// Initialized with the invalid filter.
	int         nTextureWidthFilter(-1);
	CString			strCurrentText;
	char				*szNumericString(NULL);


	m_aoTextureSizeComboBoxes[EMaximumWidth].GetWindowText(strCurrentText);
	szNumericString=strCurrentText.GetBuffer();

	// In case we failed to parse a number, we don't need to 
	// set a new filter.
	if (sscanf(szNumericString,"%d",&nTextureWidthFilter)<=0)
	{
		nTextureWidthFilter=INT_MAX;
	}
	m_poSettingsOwner->FilterWithMaximum(nTextureWidthFilter);	
}

void CTextureBrowserPanel::OnCbnSelchangeTextureBrowserTextureSizeYMinimumCombo()
{
	if (!m_poSettingsOwner)
	{
		return;
	}

	// Initialized with the invalid filter.
	int         nTextureHeightFilter(-1);
	int         nCurrentSelection(-1);
	int         nOperationResult(0);
	CString			strCurrentText;
	char				*szNumericString(NULL);

	nCurrentSelection=m_aoTextureSizeComboBoxes[EMinimumHeight].GetCurSel();
	if (nCurrentSelection==CB_ERR)
	{
		return;
	}

	m_aoTextureSizeComboBoxes[EMinimumHeight].GetLBText(nCurrentSelection,strCurrentText);
	szNumericString=strCurrentText.GetBuffer();
	// In case we failed to parse a number, we don't need to 
	// set a new filter.
	if (sscanf(szNumericString,"%d",&nTextureHeightFilter)<=0)
	{
		nTextureHeightFilter=0;
	}
	m_poSettingsOwner->FilterHeightMinimum(nTextureHeightFilter);	
}

void CTextureBrowserPanel::OnCbnEditchangeTextureBrowserTextureSizeYMinimumCombo()
{
	if (!m_poSettingsOwner)
	{
		return;
	}

	// Initialized with the invalid filter.
	int         nTextureHeightFilter(-1);
	CString			strCurrentText;
	char				*szNumericString(NULL);


	m_aoTextureSizeComboBoxes[EMinimumHeight].GetWindowText(strCurrentText);
	szNumericString=strCurrentText.GetBuffer();

	// In case we failed to parse a number, we don't need to 
	// set a new filter.
	if (sscanf(szNumericString,"%d",&nTextureHeightFilter)<=0)
	{
		nTextureHeightFilter=0;;
	}
	m_poSettingsOwner->FilterHeightMinimum(nTextureHeightFilter);	
}

void CTextureBrowserPanel::OnCbnSelchangeTextureBrowserTextureSizeYMaximumCombo()
{
	if (!m_poSettingsOwner)
	{
		return;
	}

	// Initialized with the invalid filter.
	int         nTextureHeightFilter(-1);
	int         nCurrentSelection(-1);
	int         nOperationResult(0);
	CString			strCurrentText;
	char				*szNumericString(NULL);

	nCurrentSelection=m_aoTextureSizeComboBoxes[EMaximumHeight].GetCurSel();
	if (nCurrentSelection==CB_ERR)
	{
		return;
	}

	m_aoTextureSizeComboBoxes[EMaximumHeight].GetLBText(nCurrentSelection,strCurrentText);
	szNumericString=strCurrentText.GetBuffer();
	// In case we failed to parse a number, we don't need to 
	// set a new filter.
	if (sscanf(szNumericString,"%d",&nTextureHeightFilter)<=0)
	{
		nTextureHeightFilter=INT_MAX;
	}
	m_poSettingsOwner->FilterHeightMaximum(nTextureHeightFilter);	
}

void CTextureBrowserPanel::OnCbnEditchangeTextureBrowserTextureSizeYMaximumCombo()
{
	if (!m_poSettingsOwner)
	{
		return;
	}

	// Initialized with the invalid filter.
	int         nTextureHeightFilter(-1);
	CString			strCurrentText;
	char				*szNumericString(NULL);


	m_aoTextureSizeComboBoxes[EMaximumHeight].GetWindowText(strCurrentText);
	szNumericString=strCurrentText.GetBuffer();

	// In case we failed to parse a number, we don't need to 
	// set a new filter.
	if (sscanf(szNumericString,"%d",&nTextureHeightFilter)<=0)
	{
		nTextureHeightFilter=INT_MAX;
	}
	m_poSettingsOwner->FilterHeightMaximum(nTextureHeightFilter);	
}

void CTextureBrowserPanel::OnBnClickedTextureBrowserSemanticsDiffuseCheck()
{
	if (!m_poSettingsOwner)
	{
		return;
	}

	if (m_aoTextureSemanticsCheckBoxes[EShowOnlyDiffuseSemantics].GetCheck()==BST_CHECKED)
	{
		m_poSettingsOwner->FilterShowSemanticsDiffuseOnly(true);
	}
	else
	{
		m_poSettingsOwner->FilterShowSemanticsDiffuseOnly(false);
	}
}

void CTextureBrowserPanel::OnBnClickedTextureBrowserSemanticsSpecularCheck()
{
	if (!m_poSettingsOwner)
	{
		return;
	}

	if (m_aoTextureSemanticsCheckBoxes[EShowOnlySpecularSemantics].GetCheck()==BST_CHECKED)
	{
		m_poSettingsOwner->FilterShowSemanticsSpecularOnly(true);
	}
	else
	{
		m_poSettingsOwner->FilterShowSemanticsSpecularOnly(false);
	}
}

void CTextureBrowserPanel::OnBnClickedTextureBrowserSemanticsBumpCheck()
{
	if (!m_poSettingsOwner)
	{
		return;
	}

	if (m_aoTextureSemanticsCheckBoxes[EShowOnlyBumpSemantics].GetCheck()==BST_CHECKED)
	{
		m_poSettingsOwner->FilterShowSemanticsBumpOnly(true);
	}
	else
	{
		m_poSettingsOwner->FilterShowSemanticsBumpOnly(false);
	}
}

void CTextureBrowserPanel::OnBnClickedTextureBrowserSemanticsCubemapCheck()
{
	if (!m_poSettingsOwner)
	{
		return;
	}

	if (m_aoTextureSemanticsCheckBoxes[EShowOnlyCubemapSemantics].GetCheck()==BST_CHECKED)
	{
		m_poSettingsOwner->FilterShowSemanticsCubemapOnly(true);
	}
	else
	{
		m_poSettingsOwner->FilterShowSemanticsCubemapOnly(false);
	}
}

void CTextureBrowserPanel::OnBnClickedTextureBrowserFiletypeDDSCheck()
{
	if (!m_poSettingsOwner)
	{
		return;
	}

	if (m_aoTextureFileCheckBoxes[EShowOnlyDDSFiles].GetCheck()==BST_CHECKED)
	{
		m_poSettingsOwner->FilterShowDDSOnly(true);
	}
	else
	{
		m_poSettingsOwner->FilterShowDDSOnly(false);
	}
}

void CTextureBrowserPanel::OnBnClickedTextureBrowserFiletypeTIFCheck()
{
	if (!m_poSettingsOwner)
	{
		return;
	}

	if (m_aoTextureFileCheckBoxes[EShowOnlyTIFFiles].GetCheck()==BST_CHECKED)
	{
		m_poSettingsOwner->FilterShowTIFOnly(true);
	}
	else
	{
		m_poSettingsOwner->FilterShowTIFOnly(false);
	}
}

void CTextureBrowserPanel::OnBnClickedTextureBrowserUsedInLevelCheck()
{
	if (!m_poSettingsOwner)
	{
		return;
	}

	if (m_aoTextureFileCheckBoxes[EShowOnlyUsedInLevelFiles].GetCheck()==BST_CHECKED)
	{
		m_poSettingsOwner->FilterShowUsedInLevelOnly(true);
	}
	else
	{
		m_poSettingsOwner->FilterShowUsedInLevelOnly(false);
	}
}
//////////////////////////////////////////////////////////////////////////
void CTextureBrowserPanel::OnBnClickedTextureBrowserShowAlphaChanelCheck()
{
	if (!m_poSettingsOwner)
	{
		return;
	}

	if (m_aoTextureFileCheckBoxes[EShowOnlyAlphaChannel].GetCheck()==BST_CHECKED)
	{
		m_poSettingsOwner->ShowAlphaChannel(true);
	}
	else
	{
		m_poSettingsOwner->ShowAlphaChannel(false);
	}

}
//////////////////////////////////////////////////////////////////////////
void CTextureBrowserPanel::OnBnClickedTextureBrowserHasAlphaChanelCheck()
{
	if (!m_poSettingsOwner)
	{
		return;
	}

	if (m_aoTextureFileCheckBoxes[EShowOnlyIfHasAlphaChannel].GetCheck()==BST_CHECKED)
	{
		m_poSettingsOwner->FilterShowIfHasAlphaChannel(true);
	}
	else
	{
		m_poSettingsOwner->FilterShowIfHasAlphaChannel(false);
	}
}
//////////////////////////////////////////////////////////////////////////
void CTextureBrowserPanel::OnBnClickedTextureBrowserPreviewButton()
{
	CTextureViewer::TDTextureDatabase	cpoSelectedItems;
	// As we want to open each folder containing selected textures once
	// and only once, this set guarantees we will have them stored once and only once.
	string														strExplorer("explorer ");
	string														strCommandLine;
	string                            strFilename;
	CString														strTargetDirectory(".\\");
	CString														strFinalTargetDirectory;

	STARTUPINFO												si;
	PROCESS_INFORMATION								pi;

	BOOL                              bnFailedToOpenAFile(FALSE);
	bool                              boFailedToOpenFiles(false);

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);

	ZeroMemory( &pi, sizeof(pi) );

	m_poSettingsOwner->GetSelectedItems(cpoSelectedItems);

	int				nTotalTextures(cpoSelectedItems.size());
	int				nCurrentTexture(0);
	string		strFolerToOpen;

	if (cpoSelectedItems.empty())
	{
		return;
	}

	if (cpoSelectedItems.size()>10)
	{
		if (MessageBox("Are you sure you want to open more than 10 files at a single time?","Trying to open many files",MB_YESNO)==IDNO)
		{
			return;
		}
	}

	intptr_t			hFindHandle(NULL);
	_finddata_t		stFileInfo;

	for (nCurrentTexture=0;nCurrentTexture<nTotalTextures;++nCurrentTexture)
	{
		CTextureDatabaseItem*&	rpoTextureDatabaseItem=cpoSelectedItems[nCurrentTexture];
		rpoTextureDatabaseItem->GetRelativePath(strFolerToOpen);
		rpoTextureDatabaseItem->GetFilename(strFilename);

		strCommandLine=strExplorer;
		strTargetDirectory=strFolerToOpen;
		Path::AddPathSlash(strTargetDirectory);
		strFinalTargetDirectory=Path::GamePathToFullPath(strTargetDirectory,true);
		strFinalTargetDirectory+=strFilename;
		Path::ConvertSlashToBackSlash(strFinalTargetDirectory);

		// We have to check for file existence BEFORE adding the quotes to the filename or
		// find first will fail.
		hFindHandle=_findfirst(strFinalTargetDirectory,&stFileInfo);
		if (hFindHandle==-1)
		{
			boFailedToOpenFiles=true;
			continue;
		}
		else
		{
			_findclose(hFindHandle);
		}

		Path::SurroundWithQuotes(strFinalTargetDirectory);		
		strCommandLine+=strFinalTargetDirectory;


		//// It will happen when the file is inside a packfile.
		//if (!PathFileExists(strFinalTargetDirectory.c_str()))
		//{
		//	boFailedToOpenFiles=true;
		//	continue;
		//}

		bnFailedToOpenAFile=
			bnFailedToOpenAFile
			||
			(!CreateProcess( NULL,// No module name (use command line). 
			(char*)strCommandLine.c_str(),				// Command line. 
			NULL,																	// Process handle not inheritable. 
			NULL,																	// Thread handle not inheritable. 
			TRUE,																	// Set handle inheritance to TRUE. 
			0,																		// creation flags. 
			NULL,																	// Use parent's environment block. 
			NULL,//szWorkingDirectory?szWorkingDirectory:szRemoteDirectory,			// Set starting directory. 
			&si,																	// Pointer to STARTUPINFO structure.
			&pi )
			);																			// Pointer to PROCESS_INFORMATION structure.		
		//if (!bnFailedToOpenAFile)
		//{
		//	boFailedToOpenFiles=true;
		//}
	}	

	if ((boFailedToOpenFiles)||(bnFailedToOpenAFile))
	{
		MessageBox("Could not open all the files for the selected textures.","Warning",MB_OK);
	}
}

//////////////////////////////////////////////////////////////////////////
LRESULT CTextureBrowserPanel::OnTaskPanelNotify( WPARAM wParam, LPARAM lParam )
{

	switch(wParam) 
	{
	case XTP_TPN_CLICK:
		{
			CXTPTaskPanelGroupItem* pItem = (CXTPTaskPanelGroupItem*)lParam;
			UINT nCmdID = pItem->GetID();
			switch (nCmdID)
			{
				case eOpenFolderLink:
				{
					OnBnClickedTextureBrowserOpenFolderButton();
				}
				break;

				case ePreviewLink:
				{
					OnBnClickedTextureBrowserPreviewButton();
				}
				break;
			}
		}
	}
	return 0;
}
//////////////////////////////////////////////////////////////////////////
