// SoundBrowserDialog.cpp : implementation file
//

#include "stdafx.h"
#include "SoundBrowserDialog.h"
#include "CustomFileDialog.h"
#include <ISound.h>
#include <IMusicSystem.h>
#include "SoundEventParameterDialog.h"
#include "Clipboard.h"

bool CSoundBrowserDialog::m_isAutoPlay = true;
bool CSoundBrowserDialog::m_isCached = false;
std::vector<SCachElem> CSoundBrowserDialog::m_cachElems;

//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNAMIC(CSoundBrowserDialog, CDialog)
CSoundBrowserDialog::CSoundBrowserDialog(CWnd* pParent /*=NULL*/)
:	CDialog(CSoundBrowserDialog::IDD, pParent),
	m_iTreeOffset(0),
	m_iOriginalSizeX(0),
	m_iOriginalSizeY(0),
	m_isBrowse(false),
	m_pSound(NULL),
	m_ListenerID(LISTENERID_INVALID)
{
	//m_propWnd = 0;
}

//////////////////////////////////////////////////////////////////////////
CSoundBrowserDialog::~CSoundBrowserDialog()
{
	if (m_pSound)
		m_pSound->Stop();
	
	if (m_ListenerID != LISTENERID_INVALID)
		gEnv->pSoundSystem->RemoveListener(m_ListenerID);

	// UnPause all other sounds
	gEnv->pSoundSystem->Pause(false);

	// UnPause Music
	gEnv->pMusicSystem->Pause(false);
}

void CSoundBrowserDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SOUNDSTREE, m_soundsTree);
}


BEGIN_MESSAGE_MAP(CSoundBrowserDialog, CDialog)
	ON_NOTIFY(TVN_SELCHANGED, IDC_SOUNDSTREE, OnTvnSelchangedTree)
	ON_NOTIFY(NM_DBLCLK, IDC_SOUNDSTREE, OnTvnDoubleClick)
	ON_NOTIFY(NM_RCLICK , IDC_SOUNDSTREE, OnTvnRightClick)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowseBtn)
	ON_BN_CLICKED(IDC_AUTOPLAY, OnAutoPlayBtn)
	ON_BN_CLICKED(IDC_PLAY, OnPlayBtn)
	ON_BN_CLICKED(IDC_REFRESH, OnRefreshBtn)
	ON_WM_SIZE()
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
void CSoundBrowserDialog::Init(CString const& name)
{
	m_initName = name;
	
	if (m_ListenerID == LISTENERID_INVALID)
	{
		m_ListenerID = gEnv->pSoundSystem->CreateListener();
		IListener *pListener = gEnv->pSoundSystem->GetListener(m_ListenerID);
		pListener->SetRecordLevel(1.0f);
		pListener->SetActive(true);
		gEnv->pSoundSystem->Update(eSoundUpdateMode_All);
	}

	// Pause all other sounds
	gEnv->pSoundSystem->Pause(true);

	// Pause Music
	gEnv->pMusicSystem->Pause(true);

}

//////////////////////////////////////////////////////////////////////////
void CSoundBrowserDialog::CollectCachEventgroup(XmlNodeRef & gr, const CString & Block, const CString & Path, int level)
{
	const char * pNameGr="";
	XmlNodeRef name = gr->findChild("name");
	if(name)
		pNameGr = name->getContent();
	CString NewPath = Path + pNameGr;
	m_cachElems.push_back(SCachElem(1, level, pNameGr, ""));
	for(int j=0; j<gr->getChildCount();j++)
	{
		XmlNodeRef ev= gr->getChild(j);
		if(!strcmp(ev->getTag(), "event"))
		{
			const char * pNameEv="";
			XmlNodeRef name = ev->findChild("name");
			if(name)
				pNameEv = name->getContent();

			m_cachElems.push_back(SCachElem(2, level+1, pNameEv, Block + ":" + NewPath+ ":" + pNameEv));
		}
		else if(!strcmp(ev->getTag(), "eventgroup"))
		{
			CollectCachEventgroup(ev, Block, NewPath+"/", level+1);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSoundBrowserDialog::CollectCach()
{
	BeginWaitCursor();

	m_cachElems.clear();

	CFileUtil::FileArray fa;
	CFileUtil::ScanDirectory("", "*.fdp", fa, true);

	for(int f = 0; f<fa.size(); f++)
	{
		XmlParser parser;
		XmlNodeRef root = parser.parse( fa[f].filename );
		char path[_MAX_PATH];
		strcpy(path, fa[f].filename);
		char * ch;
		while(ch = strchr(path,'\\'))
			*ch='/';
		if(ch = strrchr(path,'/'))
			*ch = 0;

		if(root)
		{
			const char * pName="";
			XmlNodeRef name = root->findChild("name");
			if(name)
				pName = name->getContent();

			m_cachElems.push_back(SCachElem(0, 0, path, ""));
			for(int i=0; i<root->getChildCount();i++)
			{
				XmlNodeRef gr= root->getChild(i);
				if(!strcmp(gr->getTag(), "eventgroup"))
					CollectCachEventgroup(gr, path, "", 1);
			}
		}
	}

	EndWaitCursor();
	m_isCached = true;
}

//////////////////////////////////////////////////////////////////////////
void CSoundBrowserDialog::FillSoundsTree()
{
	m_soundsTree.DeleteAllItems();
	m_treeMap.clear();

	HTREEITEM hProjItem=0;
	HTREEITEM hGroupItem = 0;
	HTREEITEM hEventItem;
	int level = 0;
	std::vector<HTREEITEM> projItems;

	for(int i = 0; i<m_cachElems.size(); i++)
	{
		if(m_cachElems[i].m_type==0)
		{
			if(hProjItem)
				m_soundsTree.SortChildren(hProjItem);
			hProjItem = m_soundsTree.InsertItem( m_cachElems[i].m_name, 0, 0, TVI_ROOT);
			projItems.resize(0);
			projItems.push_back( hProjItem );
			level = 1;
		}
		else
		{
			if(level != m_cachElems[i].m_level)
			{
				if(m_cachElems[i].m_level > level)
					projItems.push_back( hGroupItem );
				else
					projItems.resize(m_cachElems[i].m_level);
			}
			level = m_cachElems[i].m_level;
		}

		if(m_cachElems[i].m_type==1)
		{
			if(hGroupItem)
				m_soundsTree.SortChildren(hGroupItem);
			hGroupItem = m_soundsTree.InsertItem( m_cachElems[i].m_name, 2, 2, projItems[projItems.size()-1]);
		}
		if(m_cachElems[i].m_type==2)
		{
			hEventItem = m_soundsTree.InsertItem( m_cachElems[i].m_name, 9, 9, projItems[projItems.size()-1]);
			m_treeMap[hEventItem] = m_cachElems[i].m_path;

			if(!stricmp(m_initName, m_cachElems[i].m_path))
			{
				//m_soundsTree.Expand(hProjItem, TVE_EXPAND);
				m_soundsTree.Expand(hGroupItem, TVE_EXPAND);
				m_soundsTree.SelectItem(hEventItem);
			}
		}
	}
	if(hProjItem)
		m_soundsTree.SortChildren(hProjItem);

	if(hGroupItem)
		m_soundsTree.SortChildren(hGroupItem);
}

//////////////////////////////////////////////////////////////////////////
BOOL CSoundBrowserDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	if(CCustomFileDialog::m_bForceBrowse)
	{
		m_isBrowse = true;
		EndDialog( IDOK );
		return TRUE;
	}

	CButton * rb = (CButton *)GetDlgItem(IDC_AUTOPLAY);
	if(rb)
		rb->SetCheck(m_isAutoPlay);

	CMFCUtils::LoadTrueColorImageList( m_imageListFiles,IDB_FILES_IMAGE,16,RGB(255,0,255) );

	if(!m_isCached)
		CollectCach();

	//m_imageListFiles.Create( MAKEINTRESOURCE(IDB_FILES_IMAGE),16,1,RGB(255,255,255) );
	m_imageListFiles.SetOverlayImage( 1,1 );
	m_soundsTree.SetImageList(&m_imageListFiles,TVSIL_NORMAL);

	FillSoundsTree();

	WINDOWPLACEMENT wp = {0};
	GetWindowPlacement(&wp);

	m_iOriginalSizeX = wp.rcNormalPosition.right-wp.rcNormalPosition.left;
	m_iOriginalSizeY = wp.rcNormalPosition.bottom-wp.rcNormalPosition.top;

	CWnd * wnd;

	if(wnd = GetDlgItem(IDC_AUTOPLAY))
	{
		WINDOWPLACEMENT wp2 = {0};
		wnd->GetWindowPlacement(&wp2);
		m_buttonLeft = wp.rcNormalPosition.right-wp.rcNormalPosition.left-wp2.rcNormalPosition.left;
	}

	if(wnd = GetDlgItem(IDC_SOUNDSTREE))
	{
		WINDOWPLACEMENT wp2 = {0};
		wnd->GetWindowPlacement(&wp2);
		m_treeBot			= wp.rcNormalPosition.bottom-wp.rcNormalPosition.top-wp2.rcNormalPosition.bottom;
		m_iTreeOffset	= wp.rcNormalPosition.right-wp.rcNormalPosition.left-wp2.rcNormalPosition.right;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

//////////////////////////////////////////////////////////////////////////
void CSoundBrowserDialog::Play()
{
	if(m_pSound)
	{
		// trick to prevent SoundSystem from updating the "distance" parameter
		m_pSound->GetInterfaceExtended()->SetFlags(m_pSound->GetFlags() & ~FLAG_SOUND_RADIUS);
		m_pSound->SetSemantic(eSoundSemantic_Sandbox);
		m_pSound->Play();
	}
}

//////////////////////////////////////////////////////////////////////////
void CSoundBrowserDialog::OnTvnSelchangedTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	m_return = m_treeMap[pNMTreeView->itemNew.hItem];

	// Since the selection changed remove the old slider dialogs from the dialog
	RemoveSliders();

	if(!m_return.IsEmpty())
	{
		// Create new sliders
		HandleParameterSliders(m_return);

		// And play the sound in case the user checked auto-play
		CButton* rb = (CButton*)GetDlgItem(IDC_AUTOPLAY);
		if(rb && rb->GetCheck())
			Play();
	}
	else
	{
		// In case there is no sound name stop an potentially playing/existing one
		if(m_pSound)
		{
			m_pSound->Stop();
			m_pSound = NULL;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSoundBrowserDialog::OnTvnDoubleClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	if(!m_return.IsEmpty())
		EndDialog(IDOK);
}

//////////////////////////////////////////////////////////////////////////
bool CSoundBrowserDialog::IsBrowse()
{
	return m_isBrowse;
}

//////////////////////////////////////////////////////////////////////////
void CSoundBrowserDialog::OnBrowseBtn()
{
	m_isBrowse = true;
	EndDialog( IDOK );
}

//////////////////////////////////////////////////////////////////////////
void CSoundBrowserDialog::OnPlayBtn()
{
	if(!m_return.IsEmpty())
	{
		HandleParameterSliders(m_return);
		Play();
	}
}

//////////////////////////////////////////////////////////////////////////
void CSoundBrowserDialog::OnRefreshBtn()
{
	CollectCach();
	FillSoundsTree();
}

//////////////////////////////////////////////////////////////////////////
void CSoundBrowserDialog::OnAutoPlayBtn()
{
	CButton * rb = (CButton *)GetDlgItem(IDC_AUTOPLAY);
	if(rb)
		m_isAutoPlay = rb->GetCheck();
	if(!m_isAutoPlay)
		if (m_pSound)
			m_pSound->Stop();
}

//////////////////////////////////////////////////////////////////////////
const CString & CSoundBrowserDialog::GetString()
{
	return m_return;
}

//////////////////////////////////////////////////////////////////////////
void CSoundBrowserDialog::OnSize(UINT nType, int cx, int cy) 
{
	CWnd * wnd;
	int Items[] = {IDC_PLAY, IDC_AUTOPLAY, IDOK, IDCANCEL, IDC_BROWSE, IDC_REFRESH};

	WINDOWPLACEMENT wp;
	GetWindowPlacement(&wp);

	for(int i=0; i<sizeof(Items)/sizeof(Items[0]); i++)
	{
		if(wnd = GetDlgItem(Items[i]))
		{
			WINDOWPLACEMENT wp2;
			wnd->GetWindowPlacement(&wp2);
			int cx =wp.rcNormalPosition.right-wp.rcNormalPosition.left-m_buttonLeft - wp2.rcNormalPosition.left;
			wp2.rcNormalPosition.left+=cx;
			wp2.rcNormalPosition.right+=cx;
			wnd->SetWindowPlacement(&wp2);
		}
	}

	if(wnd = GetDlgItem(IDC_SOUNDSTREE))
	{
		WINDOWPLACEMENT wp2;
		wnd->GetWindowPlacement(&wp2);
		wp2.rcNormalPosition.right	= wp.rcNormalPosition.right-wp.rcNormalPosition.left - m_iTreeOffset;
		wp2.rcNormalPosition.bottom	= wp.rcNormalPosition.bottom-wp.rcNormalPosition.top - m_treeBot;
		wnd->SetWindowPlacement(&wp2);
	}

	// Move the parameter sliders as well
	if(!m_apSoundEventParameterDialogs.empty())
	{
		std::vector<CSoundEventParameterDialog*>::const_iterator itBeg(m_apSoundEventParameterDialogs.begin());
		std::vector<CSoundEventParameterDialog*>::const_iterator const itEnd(m_apSoundEventParameterDialogs.end());

		for(; itBeg!=itEnd; ++itBeg)
		{
			WINDOWPLACEMENT wp2;
			(*itBeg)->GetWindowPlacement(&wp2);
			wp2.rcNormalPosition.left		= wp.rcNormalPosition.right - wp.rcNormalPosition.left - m_iTreeOffset;
			wp2.rcNormalPosition.right	= wp.rcNormalPosition.right - wp.rcNormalPosition.left;
			(*itBeg)->SetWindowPlacement(&wp2);
			(*itBeg)->RedrawWindow();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSoundBrowserDialog::HandleParameterSliders(CString const& rsName)
{
	// Stop a previous sound
	if(m_pSound)
		m_pSound->Stop();

	// Create a new one
	m_pSound = gEnv->pSoundSystem->CreateSound(rsName.GetString(), FLAG_SOUND_LOAD_SYNCHRONOUSLY);

	// The slider dialogs align themselves beneath the refresh button
	CWnd* pRefreshButton = GetDlgItem(IDC_REFRESH);
	WINDOWPLACEMENT oRefreshButtonPlacement = {0};
	if(pRefreshButton)
		pRefreshButton->GetWindowPlacement(&oRefreshButtonPlacement);

	int i										= 0;
	float fValue						= 0.0f;
	float fRangeMin					= 0.0f;
	float fRangeMax					= 0.0f;
	char const* pcParamName	= NULL;

	// Only create new slider dialogs if the old ones have been removed
	if(m_apSoundEventParameterDialogs.empty() && m_pSound)
	{
		while(m_pSound->GetParam(i, &fValue, &fRangeMin, &fRangeMax, &pcParamName, false))
		{
			CSoundEventParameterDialog* pSoundEventParameterDialog = new CSoundEventParameterDialog();
			pSoundEventParameterDialog->Create(IDD_SOUND_EVENTPARAMETER, this);
			pSoundEventParameterDialog->ShowWindow(SW_SHOWNOACTIVATE);

			CRect oDialogClientRect;
			GetClientRect(oDialogClientRect);

			WINDOWPLACEMENT oPlacement = {0};
			pSoundEventParameterDialog->GetWindowPlacement(&oPlacement);

			pSoundEventParameterDialog->MoveWindow(	oDialogClientRect.Width()-oPlacement.rcNormalPosition.right,
				oRefreshButtonPlacement.rcNormalPosition.bottom+10+i*(oPlacement.rcNormalPosition.bottom+10),
				oPlacement.rcNormalPosition.right,
				oPlacement.rcNormalPosition.bottom);

			// Get final placement to compare against parent dialog size
			pSoundEventParameterDialog->GetWindowPlacement(&oPlacement);

			pSoundEventParameterDialog->SetSound(m_pSound);
			pSoundEventParameterDialog->SetIndex(i);
			pSoundEventParameterDialog->SetName(pcParamName);
			pSoundEventParameterDialog->SetRange(fRangeMin, fRangeMax);
			pSoundEventParameterDialog->SetCurrentValue(fValue);

			// Resize the dialog in case the slider dialogs grow out of its boundaries
			if(oPlacement.rcNormalPosition.bottom > oDialogClientRect.Height())
			{
				WINDOWPLACEMENT oDialogPlacement = {0};
				GetWindowPlacement(&oDialogPlacement);

				MoveWindow(	oDialogPlacement.rcNormalPosition.left,
					oDialogPlacement.rcNormalPosition.top,
					oDialogPlacement.rcNormalPosition.right-oDialogPlacement.rcNormalPosition.left,
					oDialogPlacement.rcNormalPosition.bottom-oDialogPlacement.rcNormalPosition.top+oPlacement.rcNormalPosition.bottom-oDialogClientRect.Height());
			}

			m_apSoundEventParameterDialogs.push_back(pSoundEventParameterDialog);
			++i;
		}

		SetFocus();
	}
	else
	{
		std::vector<CSoundEventParameterDialog*>::const_iterator itBeg(m_apSoundEventParameterDialogs.begin());
		std::vector<CSoundEventParameterDialog*>::const_iterator const itEnd(m_apSoundEventParameterDialogs.end());

		for(; itBeg!=itEnd; ++itBeg)
			(*itBeg)->SetSound(m_pSound);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSoundBrowserDialog::RemoveSliders()
{
	if(!m_apSoundEventParameterDialogs.empty())
	{
		std::vector<CSoundEventParameterDialog*>::const_iterator itBeg(m_apSoundEventParameterDialogs.begin());
		std::vector<CSoundEventParameterDialog*>::const_iterator const itEnd(m_apSoundEventParameterDialogs.end());

		for(; itBeg!=itEnd; ++itBeg)
		{
			(*itBeg)->DestroyWindow();
			delete *itBeg;
		}

		m_apSoundEventParameterDialogs.clear();
	}
}

//////////////////////////////////////////////////////////////////////////
void CSoundBrowserDialog::OnTvnRightClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	HTREEITEM const hItem1 = m_soundsTree.GetSelectedItem();
	if(hItem1)
	{
		CPoint oPoint;
		::GetCursorPos(&oPoint);
		m_soundsTree.ScreenToClient(&oPoint);
		HTREEITEM	const hItem2 = m_soundsTree.HitTest(oPoint);

		if(hItem1 == hItem2) // Only show the context menu if the user right clicked the actual selected tree item
		{
			char const* const pcPath = m_treeMap[hItem1];
			if(pcPath && pcPath[0])
			{
				CClipboard	oClipboard;
				CMenu				oMenu;

				oMenu.CreatePopupMenu();
				oMenu.AppendMenu( MF_STRING, 1, _T("Copy Path") );

				m_soundsTree.ClientToScreen(&oPoint);
				int const nRes = ::TrackPopupMenuEx( oMenu.GetSafeHmenu(), TPM_LEFTBUTTON|TPM_RETURNCMD, oPoint.x, oPoint.y, GetSafeHwnd(), NULL );
				if( nRes == 1 )
					oClipboard.PutString(pcPath);
			}
		}
	}
}