// SmartFileOpenDialog.cpp : implementation file
//

#include "stdafx.h"
#include "SmartFileOpenDialog.h"
#include "Util/IndexedFiles.h"

#include <ICryPak.h>

#define FILETYPE_CGF			3
#define FILETYPE_CHR_CGA	4
#define FILETYPE_JPG			5
#define FILETYPE_DDS			6
#define FILETYPE_TGA			7
#define FILETYPE_BMP			8
#define FILETYPE_SND			9

#define PREVIEW_DELAY_TIMER 1
#define TIME_OF_PREVIEW_DELAY 500

bool CSmartFileOpenDialog::m_bPreviewOn = true;

static struct FileIconsMapping
{
	char ext[8];
	int nIconID;
} s_FileIconsMapping[] = 
{
	{ "cgf",FILETYPE_CGF },
	{ "chr",FILETYPE_CHR_CGA },
	{ "cga",FILETYPE_CHR_CGA },
	{ "jpg",FILETYPE_JPG },
	{ "dds",FILETYPE_DDS },
	{ "tga",FILETYPE_TGA },
	{ "bmp",FILETYPE_BMP },
	{ "wav",FILETYPE_SND },
	{ "ogg",FILETYPE_SND },
	{ "mp2",FILETYPE_SND },
	{ "mp3",FILETYPE_SND },
};

IMPLEMENT_DYNAMIC(CSmartFileOpenDialog, CXTResizeDialog)

CSmartFileOpenDialog::CSmartFileOpenDialog(const CString& initialSearchTerm, CWnd* pParent)
	: CXTResizeDialog(CSmartFileOpenDialog::IDD, pParent),
	m_initialSearchTerm(initialSearchTerm), m_pSound(NULL), m_nTimer(0),
	m_previewItem(-1), m_iNextTagOfPrefix(0)
{
	// Fill icons mapping.
	for (int i = 0; i < sizeof(s_FileIconsMapping)/sizeof(s_FileIconsMapping[0]); i++)
	{
		m_extToIcons[s_FileIconsMapping[i].ext] = s_FileIconsMapping[i].nIconID;
	}
}

CSmartFileOpenDialog::~CSmartFileOpenDialog()
{
}

void CSmartFileOpenDialog::DoDataExchange(CDataExchange* pDX)
{
	CXTResizeDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILELIST, m_outputListCtrl);
	DDX_Control(pDX, IDC_SEARCHTAGS, m_inputCtrl);
	DDX_Control(pDX, IDC_PLAY, m_playSound);
	DDX_Control(pDX, IDC_FILEINFO, m_fileInfoCtrl);
	DDX_Control(pDX, IDC_PREVIEW_ON, m_previewOn);
	DDX_Radio(pDX, IDC_SHOW_RGB, m_previewRGBA);
}

BEGIN_MESSAGE_MAP(CSmartFileOpenDialog, CXTResizeDialog)
	ON_BN_CLICKED(IDC_REFRESH, OnBnClickedRefresh)
	ON_NOTIFY(NM_CLICK, IDC_FILELIST, OnNMClickFilelist)
	ON_NOTIFY(NM_DBLCLK, IDC_FILELIST, OnNMDblclkFilelist)
	ON_NOTIFY(NM_RCLICK, IDC_FILELIST, OnNMRclkFilelist)
	ON_NOTIFY(LVN_KEYDOWN, IDC_FILELIST, OnLvnKeydownFilelist)
	ON_EN_CHANGE(IDC_SEARCHTAGS, OnEnChangeSearchtags)
	ON_BN_CLICKED(IDC_SWITCH_TO_NORMAL, OnBnClickedNormal)
	ON_BN_CLICKED(IDC_PLAY, OnBnClickedPlay)
	ON_BN_CLICKED(IDC_PREVIEW_ON, OnBnClickedPreviewOn)
	ON_BN_CLICKED(IDC_SHOW_RGB, OnBnClickedShowAlpha)
	ON_BN_CLICKED(IDC_SHOW_ALPHA, OnBnClickedShowAlpha)
	ON_BN_CLICKED(IDC_SHOW_RGBA, OnBnClickedShowAlpha)
	ON_WM_TIMER()
	ON_NOTIFY(LVN_GETDISPINFO, IDC_FILELIST, GetDispInfo)
END_MESSAGE_MAP()

BOOL CSmartFileOpenDialog::OnInitDialog()
{
	CXTResizeDialog::OnInitDialog();

	CMFCUtils::LoadTrueColorImageList( m_imageListFiles,IDB_FILES_IMAGE,16,RGB(255,0,255) );
	m_imageListFiles.SetOverlayImage( 1,1 );

	m_outputListCtrl.SetImageList( &m_imageListFiles,LVSIL_SMALL );
	m_outputListCtrl.SetExtendedStyle( LVS_EX_BORDERSELECT|LVS_EX_DOUBLEBUFFER );
	m_outputListCtrl.ModifyStyle( 0,LVS_SINGLESEL );

	// Create columns for the report view of the list control.
	LVCOLUMN lvc; 
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
	lvc.iSubItem = 0;
	lvc.pszText = "File path";	
	lvc.cx = 300;     // width of column in pixels
	lvc.fmt = LVCFMT_LEFT;  // left-aligned column
	m_outputListCtrl.InsertColumn(0, &lvc);
	lvc.iSubItem = 1;
	lvc.pszText = "Modified";	
	lvc.cx = 150;     // width of column in pixels
	lvc.fmt = LVCFMT_LEFT;  // left-aligned column
	m_outputListCtrl.InsertColumn(1, &lvc);
	lvc.iSubItem = 2;
	lvc.pszText = "Size";	
	lvc.cx = 100;     // width of column in pixels
	lvc.fmt = LVCFMT_RIGHT;  // left-aligned column
	m_outputListCtrl.InsertColumn(2, &lvc);

	m_previewOn.SetCheck( (m_bPreviewOn)?BST_CHECKED:BST_UNCHECKED );
	m_previewRGBA = ESHOW_RGB;

	// Set up the control for the geometry preview.
	m_previewCtrl.SubclassDlgItem( IDC_PREVIEW_GEOMETRY,this );
	m_previewCtrl.ModifyStyle( SS_BITMAP,SS_OWNERDRAW );

	// Set up the control for the texture preview.
	m_previewImageCtrl.SubclassDlgItem( IDC_PREVIEW_TEXTURE,this );
	m_previewImageCtrl.ModifyStyle( SS_OWNERDRAW|SS_GRAYFRAME,SS_BITMAP );
	m_previewImageCtrl.GetClientRect(m_rcPreview);
	m_previewImageCtrl.SetBitmap( NULL );

	// Initially hides all preview controls.
	m_previewCtrl.ShowWindow(SW_HIDE);
	m_previewImageCtrl.ShowWindow(SW_HIDE);
	m_playSound.ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SHOW_RGB)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SHOW_ALPHA)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SHOW_RGBA)->ShowWindow(SW_HIDE);

	// Make dialog resizable.
	SetResize( IDOK,SZ_REPOS(1) );
	SetResize( IDCANCEL,SZ_REPOS(1) );
	SetResize( IDC_REFRESH,SZ_REPOS(1) );
	SetResize( IDC_FILELIST,SZ_RESIZE(1) );
	SetResize( IDC_SEARCHTAGS,CXTResizeRect(0,1,1,1) );
	SetResize( IDC_SWITCH_TO_NORMAL,SZ_REPOS(1) );
	SetResize( IDC_PREVIEW_GEOMETRY,SZ_HORREPOS(1) );
	SetResize( IDC_PREVIEW_TEXTURE,SZ_HORREPOS(1) );
	SetResize( IDC_FILEINFO,SZ_HORREPOS(1) );
	SetResize( IDC_PLAY,SZ_REPOS(1) );
	SetResize( IDC_PREVIEW_ON,SZ_REPOS(1) );
	SetResize( IDC_SHOW_ALPHA,SZ_REPOS(1) );

	m_inputCtrl.SetWindowText(m_initialSearchTerm);
	m_inputCtrl.SetFocus();

	UpdateData(FALSE);

	return FALSE;
}

void CSmartFileOpenDialog::OnOK()
{
	POSITION pos = m_outputListCtrl.GetFirstSelectedItemPosition();
	assert(pos != NULL);
	int nItem = m_outputListCtrl.GetNextSelectedItem(pos);
	m_currentFile = m_outputListCtrl.GetItemText(nItem, 0);
	if(CFileUtil::FileExists(m_currentFile) == false)
	// It can be gone while it is selected, so check again.
	{
		CIndexedFiles::GetDB().RemoveFile(m_currentFile);
		GetDlgItem( IDOK )->EnableWindow( FALSE );
		m_currentFile = "";
		return;
	}

	CXTResizeDialog::OnOK();
}

void CSmartFileOpenDialog::OnCancel()
{
	m_currentFile = "";

	CXTResizeDialog::OnCancel();
}

void CSmartFileOpenDialog::OnBnClickedRefresh()
{
	POSITION pos = m_outputListCtrl.GetFirstSelectedItemPosition();
	assert(pos != NULL);
	int nItem = m_outputListCtrl.GetNextSelectedItem(pos);
	CString dirPath = m_outputListCtrl.GetItemText(nItem, 0);
	if(CFileUtil::PathExists(dirPath) == false)
	// It can be gone while it is selected, so check again.
	{
		CIndexedFiles::GetDB().RemoveFile(dirPath);
		GetDlgItem( IDC_REFRESH )->EnableWindow( FALSE );
		return;
	}
	CIndexedFiles::GetDB().Refresh(dirPath);
	OnEnChangeSearchtags();
}

int CSmartFileOpenDialog::CheckSelectedItem(bool delay)
{
	POSITION pos = m_outputListCtrl.GetFirstSelectedItemPosition();
	if(pos == NULL)
		return 0;
	int nItem = m_outputListCtrl.GetNextSelectedItem(pos);
	CString path = m_outputListCtrl.GetItemText(nItem, 0);
	PreviewOff();
	if(CFileUtil::FileExists(path))
	{
		GetDlgItem( IDOK )->EnableWindow( TRUE );
		GetDlgItem( IDC_REFRESH )->EnableWindow( FALSE );
		SetDefID(IDOK);
		if(delay)
			SchedulePreview(nItem);
		else
			PreviewItem(nItem);
		return 2;
	}
	else if(CFileUtil::PathExists(path))
	{
		GetDlgItem( IDOK )->EnableWindow( FALSE );
		GetDlgItem( IDC_REFRESH )->EnableWindow( TRUE );
		SetDefID(IDC_REFRESH);
		return 1;
	}
	else
	{
		GetDlgItem( IDOK )->EnableWindow( FALSE );
		GetDlgItem( IDC_REFRESH )->EnableWindow( FALSE );
		CIndexedFiles::GetDB().RemoveFile(path);
	}

	return 0;
}

void CSmartFileOpenDialog::OnNMClickFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;

	CheckSelectedItem(false);
}

void CSmartFileOpenDialog::OnNMRclkFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;

	int selectedItemType = CheckSelectedItem(false);
	if(selectedItemType != 2) // Not a file
		return;

	POSITION pos = m_outputListCtrl.GetFirstSelectedItemPosition();
	assert(pos != NULL);
	int nItem = m_outputListCtrl.GetNextSelectedItem(pos);
	CString path = m_outputListCtrl.GetItemText(nItem, 0);
	
	bool isSelected;
	CFileUtil::PopupMenu(PathUtil::GetFile(path.GetString()), 
											PathUtil::GetPath(path.GetString()), this, &isSelected);
	if(isSelected)
	{
		OnOK();
	}
}

void CSmartFileOpenDialog::OnNMDblclkFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 1;

	int selectedItemType = CheckSelectedItem(false);
	if(selectedItemType == 2) // file
		OnOK();
	else if(selectedItemType == 1) // dir
		OnBnClickedRefresh();
}

void CSmartFileOpenDialog::OnLvnKeydownFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);

	int wKey = pLVKeyDow->wVKey;

	*pResult = 0;

	if (wKey == VK_RETURN)
	{
		*pResult = 1;

		int selectedItemType = CheckSelectedItem(false);
		if(selectedItemType == 2) // file
			OnOK();
		else if(selectedItemType == 1) // dir
			OnBnClickedRefresh();
	}
}

void CSmartFileOpenDialog::OnEnChangeSearchtags()
{
	// Prepare tags to search for.
	CString searchTerm;
	m_inputCtrl.GetWindowText(searchTerm);

	std::vector<CString> tags;
	CString tag;
	int curPos = 0;
	tag = searchTerm.Tokenize(" .", curPos);
	while(tag != "")
	{
		const int MIN_CHARS_AS_A_SEARCH_TERM = 3;
		// For efficiency, don't take a too short word into consideration. 
		if(tag.GetLength() >= MIN_CHARS_AS_A_SEARCH_TERM)
			tags.push_back(tag.MakeLower());
		tag = searchTerm.Tokenize(" .", curPos);
	}

	m_filesToList.clear();
	// Get matching files from the database.
	SetWindowText("Getting matching files from the database...");
	CIndexedFiles::GetDB().GetFilesWithTags(m_filesToList, tags);

	m_outputListCtrl.DeleteAllItems();

	int numfiles = m_filesToList.size();
	if(numfiles == 0)
	{
		GetDlgItem( IDOK )->EnableWindow( FALSE );
		GetDlgItem( IDC_REFRESH )->EnableWindow( FALSE );
	}

	m_outputListCtrl.SetItemCountEx(numfiles, LVSICF_NOSCROLL|LVSICF_NOINVALIDATEALL);

	// Select the first item by default.
	m_outputListCtrl.SetItemState( 0,LVIS_SELECTED,LVIS_SELECTED );
	CheckSelectedItem(true);

	// Display the info of file counts in the dialog title.
	CString newTitle;
	newTitle.Format("Smart Open [%d of %d] : Use backtick(`) key for tag auto-completion.", 
									numfiles, CIndexedFiles::GetDB().GetTotalCount());
	SetWindowText(newTitle);
}

static CSmartFileOpenDialog *_this = NULL;

BOOL CSmartFileOpenDialog::PreTranslateMessage(MSG* pMsg) 
{
	// If not done yet, abort.
	if(CIndexedFiles::HasFileIndexingDone() == false)
	{
		m_currentFile = "";
		EndDialog(IDRETRY);
		return TRUE;
	}

	if(pMsg->message == WM_KEYDOWN)
	{
		WPARAM nChar = pMsg->wParam;

		switch(nChar)
		{
		case VK_UP:
		case VK_DOWN:
			// Delegate up/down key messages to the list control
			// so that the items there can be selected even without a focus.
			m_outputListCtrl.SendMessage(pMsg->message, pMsg->wParam, pMsg->lParam);
			CheckSelectedItem(true);
			return TRUE;
		case VK_F5:
			// Rebuild the database.
			_this = this;
			CIndexedFiles::GetDB().Initialize(PathUtil::GetGameFolder().c_str(), CSmartFileOpenDialog::BuildProgressCallBack);
			OnEnChangeSearchtags();
			return TRUE;
		case VK_F1:
		case VK_F2:
		case VK_F3:
			if(m_previewImageCtrl.IsWindowVisible())
			{
				if (nChar == VK_F1)
					m_previewRGBA = ESHOW_RGB;
				else if (nChar == VK_F2)
					m_previewRGBA = ESHOW_ALPHA;
				else if (nChar == VK_F3)
					m_previewRGBA = ESHOW_RGBA;
				UpdateData(FALSE);
				OnBnClickedShowAlpha();
				return TRUE;
			}
			break;
		case VK_OEM_3: // backtick(`) key
			{
			CString prefix = GetPrefixBeforeCaret();
			if(prefix.GetLength() > 0)
			{
				if(prefix != m_prefix)
				{
					m_prefix = prefix;
					CIndexedFiles::GetDB().GetTagsOfPrefix(m_tagsOfPrefix, m_prefix);
					m_iNextTagOfPrefix = 0;
				}
				if(m_tagsOfPrefix.size() > 0)
				{
					CString candidate = m_tagsOfPrefix[m_iNextTagOfPrefix];
					int lengthToReplace = candidate.GetLength() - m_prefix.GetLength();
					int start, end;
					m_inputCtrl.GetSel(start, end);
					m_inputCtrl.ReplaceSel(m_tagsOfPrefix[m_iNextTagOfPrefix].Right(lengthToReplace), FALSE);
					m_inputCtrl.SetSel(start, start + lengthToReplace);
					++m_iNextTagOfPrefix;
					if(m_iNextTagOfPrefix >= m_tagsOfPrefix.size())
						m_iNextTagOfPrefix = 0;
				}
			}
			}
			return TRUE;	
		default:
			// If other keys pushed, set the focus to the edit control.
			m_inputCtrl.SetFocus();
			break;
		}
	}
	
	return CXTResizeDialog::PreTranslateMessage(pMsg);
}

bool CSmartFileOpenDialog::BuildProgressCallBack(const CString& msg)
{
	CString title;
	title.Format("Smart Open (Push ESC to cancel.) [%s]", msg);
	_this->SetWindowText(title);
	_this->RedrawWindow();
	if(GetAsyncKeyState(VK_ESCAPE) & (1<<15))
		return false;
	return true;
}

void CSmartFileOpenDialog::OnBnClickedNormal()
{
	m_currentFile = "";
	EndDialog(IDRETRY);
}

void CSmartFileOpenDialog::OnBnClickedPlay()
{
	if(m_pSound)
	{
		m_pSound->GetInterfaceExtended()->SetFlags(m_pSound->GetFlags() & ~FLAG_SOUND_RADIUS);
		m_pSound->SetSemantic(eSoundSemantic_Sandbox);
		m_pSound->Play();
	}
}

void CSmartFileOpenDialog::CancelPreviewTimer()
{
	if(m_nTimer > 0)
	{
		KillTimer(m_nTimer);
		m_nTimer = 0;
	}
	m_previewItem = -1;
}

void CSmartFileOpenDialog::SchedulePreview(int item)
{
	CancelPreviewTimer();

	m_previewItem = item;
	m_nTimer = SetTimer(PREVIEW_DELAY_TIMER, TIME_OF_PREVIEW_DELAY, NULL);
}

void CSmartFileOpenDialog::PreviewItem(int item)
{
	if(m_bPreviewOn == false)
		return;

	CString filepath = m_outputListCtrl.GetItemText(item, 0);
	LVITEM lvI;
	ZeroStruct(lvI);
	lvI.iItem = item;
	lvI.mask = LVIF_IMAGE;
	m_outputListCtrl.GetItem(&lvI);
	CString fileInfo;
	
	fileInfo.Format( "%s\r\nFile Size: %dK",
									PathUtil::GetFile((const char*)filepath),
									m_filesToList[item].size/1024 );
	
	CString ext = Path::GetExt(filepath);
	ext.MakeLower();

	bool geometryFile = lvI.iImage == FILETYPE_CGF || 
											lvI.iImage == FILETYPE_CHR_CGA ||
											ext == "cdf";
	bool textureFile = lvI.iImage == FILETYPE_DDS ||
											lvI.iImage == FILETYPE_JPG ||
											lvI.iImage == FILETYPE_BMP ||
											lvI.iImage == FILETYPE_TGA ||
											ext == "tif";

	bool soundFile = lvI.iImage == FILETYPE_SND;

	if(geometryFile)
	{
		if (m_previewCtrl.m_hWnd)
		{
			m_previewCtrl.LoadFile( filepath );
		}
		int nVertexCount = m_previewCtrl.GetVertexCount();
		int nFaceCount = m_previewCtrl.GetFaceCount();
		int nLods = m_previewCtrl.GetLodCount();
		int nMtls = m_previewCtrl.GetMtlCount();
		if (nFaceCount > 0)
		{
			CString info;
			info.Format( "\r\n%d Faces\r\n%d Verts\r\n%d Lods\r\n%d Materials",nFaceCount,nVertexCount,nLods,nMtls );
			fileInfo += info;
		}

		m_previewCtrl.ShowWindow(SW_SHOW);
	}
	else if(textureFile)
	{
		if (m_previewImageCtrl.m_hWnd)
		{
			CImage	image;
			CString strLoadFilename;
			strLoadFilename=Path::GamePathToFullPath(filepath,false);

			if(CImageUtil::LoadImage( strLoadFilename,image ))
			{
				CString imginfo;
				imginfo.Format( "\r\n%dx%d\r\n%s",image.GetWidth(),image.GetHeight(),(const char*)image.GetFormatDescription() );
				fileInfo += imginfo;

				CImage scaledImage;
				scaledImage.Allocate( m_rcPreview.Width(),m_rcPreview.Height() );
				CImageUtil::ScaleToFit( image,scaledImage );

				if (m_previewRGBA == ESHOW_RGB)
				{
					scaledImage.SwapRedAndBlue();
					scaledImage.FillAlpha();
				}
				else if(m_previewRGBA == ESHOW_ALPHA)
				{
					for (int h = 0; h < scaledImage.GetHeight(); h++)
						for (int w = 0; w < scaledImage.GetWidth(); w++)
						{
							int a = scaledImage.ValueAt(w,h) >> 24;
							scaledImage.ValueAt(w,h) = RGB(a,a,a);
						}
				}
				else //if (m_previewRGBA == ESHOW_RGBA)
					scaledImage.SwapRedAndBlue();

				if(m_previewBitmap.m_hObject)
					m_previewBitmap.DeleteObject();
				m_previewBitmap.CreateBitmap( scaledImage.GetWidth(),scaledImage.GetHeight(), 1, 32, scaledImage.GetData() );
				m_previewImageCtrl.SetBitmap( m_previewBitmap );
			}
			else
			{
				m_previewImageCtrl.SetBitmap( NULL );
			}

			m_previewImageCtrl.ShowWindow(SW_SHOW);
			GetDlgItem(IDC_SHOW_RGB)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_SHOW_ALPHA)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_SHOW_RGBA)->ShowWindow(SW_SHOW);
		}
	}
	else if(soundFile)
	{
		// Stop the previous sound.
		if(m_pSound)
		{
			m_pSound->Stop();
			m_pSound = NULL;
		}

		// Start the new one.
		m_pSound = gEnv->pSoundSystem->CreateSound(filepath, FLAG_SOUND_LOAD_SYNCHRONOUSLY);

		if(m_pSound)
		{
				m_playSound.ShowWindow(SW_SHOW);
		}
	}
	else
	{
		PreviewOff();
	}

	m_fileInfoCtrl.SetWindowText( fileInfo );
}

void CSmartFileOpenDialog::PreviewOff()
{
	CancelPreviewTimer();

	m_fileInfoCtrl.SetWindowText( "" );
	if(m_pSound)
		m_pSound->Stop();
	m_pSound = 0;
	m_previewCtrl.ReleaseObject();
	m_previewImageCtrl.SetBitmap( NULL );

	m_previewCtrl.ShowWindow(SW_HIDE);
	m_previewImageCtrl.ShowWindow(SW_HIDE);
	m_playSound.ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SHOW_RGB)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SHOW_ALPHA)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_SHOW_RGBA)->ShowWindow(SW_HIDE);

	m_previewItem = -1;
}

void CSmartFileOpenDialog::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == PREVIEW_DELAY_TIMER)
	{
		if(m_previewItem >= 0)
			PreviewItem(m_previewItem);
		CancelPreviewTimer();
	}

	CXTResizeDialog::OnTimer(nIDEvent);
}

void CSmartFileOpenDialog::OnDestroy()
{
	PreviewOff();

	CXTResizeDialog::OnDestroy();
}

void CSmartFileOpenDialog::OnBnClickedPreviewOn()
{
	bool bPreviewOn =  m_previewOn.GetCheck() == BST_CHECKED;
	if(!bPreviewOn)
	{
		PreviewOff();
		m_bPreviewOn = bPreviewOn;
	}
	else
	{
		m_bPreviewOn = bPreviewOn;
		CheckSelectedItem(false);
	}
}

void CSmartFileOpenDialog::OnBnClickedShowAlpha()
{
	UpdateData(TRUE);
	CheckSelectedItem(false);
}

CString CSmartFileOpenDialog::GetPrefixBeforeCaret() const
{
	CString inputString, stringBeforeCaret;
	int start, end;
	m_inputCtrl.GetSel(start, end);
	m_inputCtrl.GetWindowText(inputString);
	if(inputString.GetLength() ==0)
		return "";
	stringBeforeCaret = inputString.Left(start);
	if(stringBeforeCaret.GetLength() == 0
	|| stringBeforeCaret[stringBeforeCaret.GetLength()-1] == ' ')
		return "";
	return stringBeforeCaret.Right(stringBeforeCaret.GetLength()
																	- (stringBeforeCaret.ReverseFind(' ') + 1)).MakeLower();
}

void CSmartFileOpenDialog::GetDispInfo(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	//Create a pointer to the item
	LV_ITEM* pItem= &(pDispInfo)->item;

	//Which item number?
	int nItem = pItem->iItem;

	int numfiles = m_filesToList.size();
	if(nItem > numfiles-1)
		return; // Just to be safe

	CFileUtil::FileDesc& fileDesc = m_filesToList[nItem];

	//Do we need text information?
	if(pItem->mask & LVIF_TEXT) 
	{
		CString csText;

		//Which column?
		if(pItem->iSubItem == 0)
			csText = fileDesc.filename;
		else if (pItem->iSubItem == 1) // File modification time
			csText = fileDesc.time_write>0 ? 
									ctime(&fileDesc.time_write) :
									"Unknown";
		else if (pItem->iSubItem == 2) // File Size
			csText.Format("%dK", (int32)(fileDesc.size/1024));

		//Copy the text to the LV_ITEM structure
		//Maximum number of characters is in pItem->cchTextMax
		lstrcpyn(pItem->pszText, csText, pItem->cchTextMax);
	}

	//Does the list need image information?
	if(pItem->mask & LVIF_IMAGE) 
	{
		// Find icon id associated with this extension.
		UINT nImage = 2;
		if(fileDesc.attrib & _A_SUBDIR)
			nImage = 0;
		else
		{
			CString ext = Path::GetExt(fileDesc.filename);
			ext.MakeLower();
			nImage = stl::find_in_map( m_extToIcons,ext,nImage );
		}
		pItem->iImage = nImage; // Always use 0 since only one element

		// In-Pak-files have a different color.
		if(fileDesc.attrib & _A_IN_CRYPAK)
		{
			pItem->mask |= LVIF_STATE; 
			pItem->state = INDEXTOOVERLAYMASK(1);
			pItem->stateMask = LVIS_OVERLAYMASK;
		}
	}
	
	*pResult = 0;
}