// DialogBrowserDialog.cpp : implementation file
//

#include "stdafx.h"
#include "DialogBrowserDialog.h"
#include "CustomFileDialog.h"
#include "Clipboard.h"
#include <ISound.h>
#include <IMusicSystem.h>
#include <ILocalizationManager.h>


bool CDialogBrowserDialog::m_isAutoPlay = true;
bool CDialogBrowserDialog::m_isDSPEffect = true;
bool CDialogBrowserDialog::m_isCached = false;
std::vector<SDialogCachElem> CDialogBrowserDialog::m_cachElems;

IMPLEMENT_DYNAMIC(CDialogBrowserDialog, CDialog)
CDialogBrowserDialog::CDialogBrowserDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CDialogBrowserDialog::IDD, pParent)
{
	//m_propWnd = 0;
	m_isBrowse = false;
	m_bClearingSoundTree = false;
	m_pSound = NULL;
	m_ListenerID = LISTENERID_INVALID;
	m_SortingRule1 = eDBD_Sorting_NONE;
	m_SortingRule2 = eDBD_Sorting_NONE;

}

CDialogBrowserDialog::~CDialogBrowserDialog()
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

void CDialogBrowserDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DIALOGTREE, m_soundsTree);
}


BEGIN_MESSAGE_MAP(CDialogBrowserDialog, CDialog)
	ON_NOTIFY(TVN_SELCHANGED, IDC_DIALOGTREE, OnTvnSelchangedTree)
	ON_NOTIFY(NM_DBLCLK, IDC_DIALOGTREE, OnTvnDoubleClick)
	ON_NOTIFY(NM_RCLICK , IDC_DIALOGTREE, OnTvnRightClick)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowseBtn)
	ON_BN_CLICKED(IDC_AUTOPLAY, OnAutoPlayBtn)
	ON_BN_CLICKED(IDC_DSP_EFFECT, OnDSPEffectBtn)
	ON_BN_CLICKED(IDC_PLAY, OnPlayBtn)
	ON_BN_CLICKED(IDC_REFRESH, OnRefreshBtn)
	ON_CBN_SELCHANGE(IDC_SOUNDBROWSER_SORT1, OnSortChange)
	ON_CBN_SELCHANGE(IDC_SOUNDBROWSER_SORT2, OnSortChange)
	ON_WM_SIZE()
END_MESSAGE_MAP()


void CDialogBrowserDialog::Init(const CString & name)
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


void CDialogBrowserDialog::CollectCach()
{
	BeginWaitCursor();

	m_cachElems.clear();

	ILocalizationManager* pLocMan = gEnv->pSystem->GetLocalizationManager();
	int nNum = pLocMan->GetLocalizedStringCount();
	m_cachElems.reserve(nNum);

	for (int i = 0; i<nNum; ++i)
	{
		SLocalizedInfoEditor LocInfo;
		memset( &LocInfo,0,sizeof(SLocalizedInfoEditor) );
		bool bResult = pLocMan->GetLocalizedInfoByIndex(i, LocInfo);

		string sFolder;	
		string sKey = LocInfo.sKey;
		const char *pFirstSlash = strstr(LocInfo.sKey, "/");

		if (pFirstSlash)
			sFolder = sKey.substr(0, pFirstSlash - LocInfo.sKey);

		if (LocInfo.sKey[0] != 0 && LocInfo.sOriginalActorLine[0] != 0 && LocInfo.sOriginalCharacterName[0] != 0)
			m_cachElems.push_back(SDialogCachElem(0, LocInfo.nRow, LocInfo.sKey, LocInfo.sOriginalActorLine, sFolder.c_str(), LocInfo.sOriginalCharacterName));
		//type and level unused for now
	}

	EndWaitCursor();
	m_isCached = true;
}


void CDialogBrowserDialog::FillSoundsTree()
{
	m_bClearingSoundTree = true;
	m_soundsTree.DeleteAllItems();
	m_treeMap.clear();
	m_bClearingSoundTree = false;

	//if (!bSortByFolder && !bSortByIndex && rb1)
	//{
	//	rb1->SetCheck(1);
	//	bSortByFolder = true;
	//}

	//for(int i = 0; i<m_cachElems.size(); i++)
	//{
	//	mapFolder[m_cachElems[i].m_sFolder] = 0;
	//	mapCharacterPerFolder[m_cachElems[i].m_sFolder][m_cachElems[i].m_sEnglishWho] = 0;
	//}

	HTREEITEM hSort1Item = 0;
	HTREEITEM hSort2Item = 0;
	HTREEITEM hLineItem;
	int level = 0;
	std::vector<HTREEITEM> projItems;

	typedef std::map<CString, HTREEITEM> tHTreeItemMap;
	tHTreeItemMap mapSort1;
	std::map<CString, tHTreeItemMap> mapSort2PerSort1;

	CString sSort2ItemText;

	for(int i = 0; i<m_cachElems.size(); i++)
	{
		CString sRow;
		sRow.Format("%d", m_cachElems[i].m_nRow); //int to CString

		// first sort
		switch (m_SortingRule1)
		{
		case eDBD_Sorting_SUBFOLDER:
			{
				hSort1Item = mapSort1[m_cachElems[i].m_sFolder];

				if (!hSort1Item) // not added to TreeCtrl yet
				{
					if (!m_cachElems[i].m_sFolder.IsEmpty()) // dont add empty folder name
					{
						hSort1Item = m_soundsTree.InsertItem( m_cachElems[i].m_sFolder, 0, 0, TVI_ROOT);
						m_soundsTree.SortChildren(hSort1Item);
						mapSort1[m_cachElems[i].m_sFolder] = hSort1Item; // added
					}
				}
				sSort2ItemText = m_cachElems[i].m_sEnglishWho;

				break;
			}
		case eDBD_Sorting_CHARACTER:
			{
				hSort1Item = mapSort1[m_cachElems[i].m_sEnglishWho];

				if (!hSort1Item) // not added to TreeCtrl yet
				{
					if (!m_cachElems[i].m_sEnglishWho.IsEmpty()) // dont add empty folder name
					{
						hSort1Item = m_soundsTree.InsertItem( m_cachElems[i].m_sEnglishWho, 0, 0, TVI_ROOT);
						m_soundsTree.SortChildren(hSort1Item);
						mapSort1[m_cachElems[i].m_sEnglishWho] = hSort1Item; // added
					}
				}
				sSort2ItemText = m_cachElems[i].m_sFolder;

				break;
			}
		case eDBD_Sorting_INDEX:
			{
				CString sText = CString(sRow) + CString(" [") + CString(m_cachElems[i].m_sEnglishWho) + CString("] ") + CString(m_cachElems[i].m_sText);
				hLineItem = m_soundsTree.InsertItem( sText, 9, 9, TVI_ROOT);
				m_treeMap[hLineItem] = m_cachElems[i].m_sKey;
				continue;
				break;
			}
		default:
			break;
		}


		// second sort
		switch (m_SortingRule2)
		{
		case eDBD_Sorting_SUBFOLDER:
			{
				hSort2Item = mapSort2PerSort1[m_cachElems[i].m_sEnglishWho][m_cachElems[i].m_sFolder];

				if (!hSort2Item) // not added to TreeCtrl yet
				{
					if (!m_cachElems[i].m_sEnglishWho.IsEmpty())
						hSort2Item = m_soundsTree.InsertItem( m_cachElems[i].m_sFolder, 2, 2, mapSort1[m_cachElems[i].m_sEnglishWho]);
					else
						hSort2Item = m_soundsTree.InsertItem( m_cachElems[i].m_sFolder, 2, 2, TVI_ROOT);

					m_soundsTree.SortChildren(hSort2Item);
					mapSort2PerSort1[m_cachElems[i].m_sEnglishWho][m_cachElems[i].m_sFolder] = hSort2Item; // added
				}

				break;
			}
		case eDBD_Sorting_CHARACTER:
			{
				hSort2Item = mapSort2PerSort1[m_cachElems[i].m_sFolder][m_cachElems[i].m_sEnglishWho];

				if (!hSort2Item) // not added to TreeCtrl yet
				{
					if (!m_cachElems[i].m_sFolder.IsEmpty())
						hSort2Item = m_soundsTree.InsertItem( m_cachElems[i].m_sEnglishWho, 2, 2, mapSort1[m_cachElems[i].m_sFolder]);
					else
						hSort2Item = m_soundsTree.InsertItem( m_cachElems[i].m_sEnglishWho, 2, 2, TVI_ROOT);

					m_soundsTree.SortChildren(hSort2Item);
					mapSort2PerSort1[m_cachElems[i].m_sFolder][m_cachElems[i].m_sEnglishWho] = hSort2Item; // added
				}


				break;
			}
		case eDBD_Sorting_INDEX:
			break;
		default:
			break;
		}


		// insert real line
		if (hSort2Item)
		{
			CString sText = CString(sRow) + CString(" - ") + CString(m_cachElems[i].m_sText);
			hLineItem = m_soundsTree.InsertItem( sText, 9, 9, hSort2Item);
		}
		else
		{
			if (hSort1Item)
			{
				CString sText = CString(sRow) + CString(" [") + CString(sSort2ItemText) + CString("] ") + CString(m_cachElems[i].m_sText);
				hLineItem = m_soundsTree.InsertItem( sText, 9, 9, hSort1Item);
			}
			else
			{
				CString sText = CString(sRow) + CString(" - ") + CString(m_cachElems[i].m_sText);
				hLineItem = m_soundsTree.InsertItem( sText, 9, 9, TVI_ROOT);
			}
		}

		m_treeMap[hLineItem] = m_cachElems[i].m_sKey;
	}
}

BOOL CDialogBrowserDialog::OnInitDialog()
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

	CButton * rbdsp = (CButton *)GetDlgItem(IDC_DSP_EFFECT);
	if(rbdsp)
		rbdsp->SetCheck(m_isDSPEffect);

	CMFCUtils::LoadTrueColorImageList( m_imageListFiles,IDB_FILES_IMAGE,16,RGB(255,0,255) );

	if(!m_isCached)
		CollectCach();

	//m_imageListFiles.Create( MAKEINTRESOURCE(IDB_FILES_IMAGE),16,1,RGB(255,255,255) );
	m_imageListFiles.SetOverlayImage( 1,1 );
	m_soundsTree.SetImageList(&m_imageListFiles,TVSIL_NORMAL);

	// set ComboBoxes with default values
	OnSortChange();

	FillSoundsTree();

	WINDOWPLACEMENT wp;
	GetWindowPlacement(&wp);

	CWnd * wnd;

	if(wnd = GetDlgItem(IDC_AUTOPLAY))
	{
		WINDOWPLACEMENT wp2;
		wnd->GetWindowPlacement(&wp2);
		m_buttonLeft = wp.rcNormalPosition.right-wp.rcNormalPosition.left-wp2.rcNormalPosition.left;
	}

	if(wnd = GetDlgItem(IDC_DIALOGTREE))
	{
		WINDOWPLACEMENT wp2;
		wnd->GetWindowPlacement(&wp2);
		m_treeBot= wp.rcNormalPosition.bottom-wp.rcNormalPosition.top-wp2.rcNormalPosition.bottom;
	}



	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CDialogBrowserDialog::Play(CString name)
{
	// prevent to play all sounds via callback on clearing the soundtree
	if (m_bClearingSoundTree)
		return;

	// Play preview sound.
	// stop the previous sound
	if (m_pSound)
		m_pSound->Stop();

	// start new one
	if (m_isDSPEffect)
		m_pSound = gEnv->pSoundSystem->CreateSound( name.GetString(),FLAG_SOUND_VOICE|FLAG_SOUND_LOAD_SYNCHRONOUSLY );
	else
		m_pSound = gEnv->pSoundSystem->CreateSound( name.GetString(),FLAG_SOUND_2D|FLAG_SOUND_STEREO|FLAG_SOUND_16BITS|FLAG_SOUND_LOAD_SYNCHRONOUSLY );

	if (m_pSound)
	{
		m_pSound->SetSemantic(eSoundSemantic_Sandbox);
		m_pSound->Play();
	}
}


void CDialogBrowserDialog::OnTvnSelchangedTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	m_return = m_treeMap[pNMTreeView->itemNew.hItem];

	CButton * rb = (CButton *)GetDlgItem(IDC_AUTOPLAY);
	if(rb && rb->GetCheck())
		if (!m_return.IsEmpty())
			Play(m_return);
}

//////////////////////////////////////////////////////////////////////////
void CDialogBrowserDialog::OnTvnDoubleClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	if(!m_return.IsEmpty())
		EndDialog(IDOK);
}

bool CDialogBrowserDialog::IsBrowse()
{
	return m_isBrowse;
}

void CDialogBrowserDialog::OnBrowseBtn()
{
	m_isBrowse = true;
	EndDialog( IDOK );
}

void CDialogBrowserDialog::OnPlayBtn()
{
	if (!m_return.IsEmpty())
		Play(m_return);
}


void CDialogBrowserDialog::OnRefreshBtn()
{
	CollectCach();
	FillSoundsTree();
}


void CDialogBrowserDialog::OnAutoPlayBtn()
{
	CButton * rb = (CButton *)GetDlgItem(IDC_AUTOPLAY);
	if(rb)
		m_isAutoPlay = rb->GetCheck();
	if(!m_isAutoPlay)
		if (m_pSound)
			m_pSound->Stop();
}

void CDialogBrowserDialog::OnDSPEffectBtn()
{
	CButton * rb = (CButton *)GetDlgItem(IDC_DSP_EFFECT);
	if(rb)
		m_isDSPEffect = rb->GetCheck();

	if (m_pSound)
		m_pSound->Stop();
}

void CDialogBrowserDialog::OnSortChange()
{
	// set ComboBoxes with correct values
	CComboBox * LB1 = (CComboBox *)GetDlgItem(IDC_SOUNDBROWSER_SORT1); // rule 1
	CComboBox * LB2 = (CComboBox *)GetDlgItem(IDC_SOUNDBROWSER_SORT2); // rule 2
	
	eDBD_Sorting m_NewSortingRule1 = (eDBD_Sorting)LB1->GetCurSel();
	eDBD_Sorting m_NewSortingRule2 = (eDBD_Sorting)LB2->GetCurSel();

	if (m_NewSortingRule1 != eDBD_Sorting_NONE && (m_NewSortingRule1 == m_NewSortingRule2 || m_NewSortingRule2 == eDBD_Sorting_NONE))
		LB2->SetCurSel(eDBD_Sorting_INDEX);

	if (m_NewSortingRule1 == eDBD_Sorting_NONE)
	{
		LB1->SetCurSel(eDBD_Sorting_SUBFOLDER);
		LB2->SetCurSel(eDBD_Sorting_CHARACTER);
	}

	if (m_NewSortingRule1 == eDBD_Sorting_INDEX)
	{
		LB2->SetCurSel(eDBD_Sorting_NONE);
	}

	m_NewSortingRule1 = m_SortingRule1;
	m_NewSortingRule2 = m_SortingRule2;

	m_SortingRule1 = (eDBD_Sorting)LB1->GetCurSel();
	m_SortingRule2 = (eDBD_Sorting)LB2->GetCurSel();

	if (m_NewSortingRule1 != LB1->GetCurSel() || m_NewSortingRule2 != LB2->GetCurSel())
		FillSoundsTree();
}


const CString & CDialogBrowserDialog::GetString()
{
	return m_return;
}

void CDialogBrowserDialog::OnSize(UINT nType, int cx, int cy) 
{
	CWnd * wnd;
	int Items[] = {IDC_PLAY, IDC_AUTOPLAY, IDC_DSP_EFFECT, IDOK, IDCANCEL, IDC_BROWSE, IDC_REFRESH, IDC_STATIC, IDC_SOUNDBROWSER_SORT1, IDC_SOUNDBROWSER_SORT2};

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

	if(wnd = GetDlgItem(IDC_DIALOGTREE))
	{
		WINDOWPLACEMENT wp2;
		wnd->GetWindowPlacement(&wp2);
		wp2.rcNormalPosition.right = wp.rcNormalPosition.right-wp.rcNormalPosition.left-m_buttonLeft-5;
		wp2.rcNormalPosition.bottom = wp.rcNormalPosition.bottom-wp.rcNormalPosition.top - m_treeBot;
		wnd->SetWindowPlacement(&wp2);
	}

}

//////////////////////////////////////////////////////////////////////////
void CDialogBrowserDialog::OnTvnRightClick(NMHDR *pNMHDR, LRESULT *pResult)
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
				oMenu.AppendMenu( MF_STRING, 1, _T("Copy Key") );

				m_soundsTree.ClientToScreen(&oPoint);
				int const nRes = ::TrackPopupMenuEx( oMenu.GetSafeHmenu(), TPM_LEFTBUTTON|TPM_RETURNCMD, oPoint.x, oPoint.y, GetSafeHwnd(), NULL );
				if( nRes == 1 )
					oClipboard.PutString(pcPath);
			}
		}
	}
}