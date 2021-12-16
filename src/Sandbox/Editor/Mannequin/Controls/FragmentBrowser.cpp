#include "stdafx.h"
#include "FragmentBrowser.h"

#include "../FragmentEditor.h"
#include "../MannequinBase.h"

#include "../MannTagEditorDialog.h"
#include "../MannAdvancedPasteDialog.h"
#include "../MannequinDialog.h"
#include "../SequencerDopeSheetBase.h"
#include "StringDlg.h"
#include <IGameFramework.h>
#include <ICryMannequinEditor.h>
#include "Clipboard.h"

#define FILTER_DELAY_SECONDS (0.5f)

IMPLEMENT_DYNAMIC(CFragmentBrowser, CXTResizeFormView)

BEGIN_MESSAGE_MAP(CFragmentBrowser, CXTResizeFormView)
	ON_BN_CLICKED(IDC_NEW, OnNewBtn)
	ON_BN_CLICKED(IDEDIT, OnEditBtn)
	ON_BN_CLICKED(IDDELETE, OnDeleteBtn)
	ON_BN_CLICKED(IDC_NEW_DEFINITION, OnNewDefinitionBtn)
	ON_BN_CLICKED(IDC_DELETE_DEFINITION, OnDeleteDefinitionBtn)
	ON_BN_CLICKED(IDC_RENAME_DEFINITION, OnRenameDefinitionBtn)
	ON_BN_CLICKED(IDC_FRAGMENT_SUBFOLDERS_CHK, OnToggleSubfolders)
	ON_BN_CLICKED(IDC_FRAGMENT_SHOWEMPTY_CHK, OnToggleShowEmpty)
	ON_BN_CLICKED(IDC_TAG_DEF_EDITOR, OnTagDefEditorBtn)
	ON_EN_CHANGE(IDC_FILTER_TAGS, OnChangeFilterTags)
	ON_EN_CHANGE(IDC_FILTER_FRAGMENT, OnChangeFilterFragments)
	ON_EN_CHANGE(IDC_FILTER_ANIM_CLIP, OnChangeFilterAnimClip)

	ON_NOTIFY(TVN_BEGINDRAG, IDC_TREE_FB, OnBeginDrag)
	ON_NOTIFY(TVN_BEGINRDRAG, IDC_TREE_FB, OnBeginRDrag)

	ON_CBN_SELCHANGE(IDC_CONTEXT, OnChangeContext)

	ON_WM_MOUSEMOVE()

	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_NOTIFY(NM_CLICK, IDC_TREE_FB, OnTVClick)
	ON_NOTIFY(NM_RCLICK, IDC_TREE_FB, OnTVRightClick)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_FB, OnTVEditSel)
	ON_NOTIFY(NM_RETURN, IDC_TREE_FB, OnTVEditSel)

	ON_NOTIFY(TVN_KEYDOWN, IDC_TREE_FB, OnTreeKeyDown)

	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_FB, OnTVSelChanged)
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()




class CFragmentBrowserBaseDropTarget : public COleDropTarget
{
public:

	CFragmentBrowserBaseDropTarget(CFragmentBrowser* pDlg)
	: m_pDlg(pDlg) 
	{}

	virtual DROPEFFECT OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
	{
		SetCapture(*pWnd);

		// Do the same as dragover
		return OnDragOver(pWnd, pDataObject, dwKeyState, point);
	}

	virtual DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
	{
		// Check if there's a fragment ID in the browser under the mouse
		if (FRAGMENT_ID_INVALID != m_pDlg->GetValidFragIDForAnim(point, pDataObject))
		{
			return DROPEFFECT_MOVE;
		}

		return DROPEFFECT_NONE;

	}

	virtual void OnDragLeave(CWnd* pWnd)
	{
		m_pDlg->m_TreeCtrl.SelectDropTarget(0);
		ReleaseCapture();
	}

	virtual BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
	{
		// Get the fragment ID 
		FragmentID fragID = m_pDlg->GetValidFragIDForAnim(point, pDataObject);
		if (FRAGMENT_ID_INVALID == fragID)
		{
			return FALSE;
		}

		m_pDlg->m_TreeCtrl.SelectDropTarget(0);
		ReleaseCapture();

		// Add the animation to the animation database through the browser, and rebuild the ID entry
		return m_pDlg->AddAnimToFragment(fragID, pDataObject);
	}

	virtual DROPEFFECT OnDragScroll(CWnd* pWnd, DWORD dwKeyState, CPoint point)
	{
		CRect rect; 
		m_pDlg->m_TreeCtrl.GetClientRect(&rect); 
		rect.NormalizeRect(); 

		m_pDlg->ClientToScreen(&point);
		m_pDlg->m_TreeCtrl.ScreenToClient(&point);

		//determine if we are close to top or bottom of tree control 
		if (point.y > (rect.Height() - 5)) 
		{ 
			//close to bottom, scroll down 1 line 
			m_pDlg->m_TreeCtrl.SendMessage(WM_VSCROLL,SB_LINEDOWN,0l); 
			return DROPEFFECT_SCROLL;
		}
		else if (point.y < 5) 
		{ 
			//close to top, scroll up 1 line 
			m_pDlg->m_TreeCtrl.SendMessage(WM_VSCROLL,SB_LINEUP,0l); 
			return DROPEFFECT_SCROLL;
		}
		return DROPEFFECT_MOVE;
	}
	
private:

	CFragmentBrowser* m_pDlg;
};

const int IMAGE_FOLDER = 1;
const int IMAGE_ANIM = 6;
const CString DEFAULT_FRAGMENT_ID_FILTER_TEXT("<FragmentID Filter>");
const CString DEFAULT_TAG_FILTER_TEXT("<Tag Filter>");
const CString DEFAULT_ANIM_CLIP_FILTER_TEXT("<Anim Filter>");

CFragmentBrowser::CFragmentBrowser(CMannFragmentEditor &fragEditor, CWnd *pParent, UINT nId)
: CXTResizeFormView(CFragmentBrowser::IDD)
, m_animDB(NULL)
, m_fragEditor(fragEditor)
, m_editedItem(0)
, m_rightDrag(false)
, m_draggingImage(0)
, m_dragItem(0)
, m_context(NULL)
, m_scopeContext(-1)
, m_fragmentID(FRAGMENT_ID_INVALID)
, m_option(0)
, m_copiedFragmentID(FRAGMENT_ID_INVALID)
, m_showSubFolders(true)
, m_showEmptyFolders(true)
, m_filterDelay(0.0f)
{
	Create("ClassName", "FragmentBrowser", 0, CRect(0, 0, 100, 100), pParent, nId, NULL);
	m_toolTip.Create(this);
	m_toolTip.AddTool(&m_newEntry, "New Fragment Entry");
	m_toolTip.AddTool(&m_deleteEntry, "Delete Fragment Entry");
	m_toolTip.AddTool(&m_editEntry, "Edit Fragment Entry");
	m_toolTip.AddTool(&m_newID, "New FragmentID");
	m_toolTip.AddTool(&m_deleteID, "Delete FragmentID");
	m_toolTip.AddTool(&m_renameID, "Rename FragmentID");
	m_toolTip.AddTool(&m_tagDefEditor, "Launch Tag Definition Editor");
	m_toolTip.Activate(true);
}

CFragmentBrowser::~CFragmentBrowser()
{
}

void CFragmentBrowser::Update(void)
{
	static CTimeValue timeLast = gEnv->pTimer->GetAsyncTime();

	CTimeValue timeNow = gEnv->pTimer->GetAsyncTime();
	CTimeValue deltaTime = timeNow - timeLast;
	timeLast = timeNow;

	if (m_filterDelay > 0.0f)
	{
		m_filterDelay -= deltaTime.GetSeconds();

		if (m_filterDelay <= 0.0f)
		{
			m_filterDelay = 0.0f;
			RebuildAll();
		}
	}
}

void CFragmentBrowser::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE_FB, m_TreeCtrl);
	DDX_Control(pDX, IDC_CONTEXT, m_cbContext);
	DDX_Control(pDX, IDC_EDIT_ADBFILE, m_CurrFile);
	DDX_Control(pDX, IDC_FILTER_TAGS, m_editFilterTags);
	DDX_Control(pDX, IDC_FILTER_FRAGMENT, m_editFilterFragmentIDs);
	DDX_Control(pDX, IDC_FILTER_ANIM_CLIP, m_editAnimClipFilter);
	DDX_Control(pDX, IDC_FRAGMENT_SUBFOLDERS_CHK, m_chkShowSubFolders);
	DDX_Control(pDX, IDC_FRAGMENT_SHOWEMPTY_CHK, m_chkShowEmptyFolders);
	m_chkShowSubFolders.SetCheck(m_showSubFolders);
	m_chkShowEmptyFolders.SetCheck(m_showEmptyFolders);
	DDX_Control(pDX, IDC_NEW, m_newEntry);
	DDX_Control(pDX, IDDELETE, m_deleteEntry);
	DDX_Control(pDX, IDEDIT, m_editEntry);
	DDX_Control(pDX, IDC_NEW_DEFINITION, m_newID);
	DDX_Control(pDX, IDC_DELETE_DEFINITION, m_deleteID);
	DDX_Control(pDX, IDC_RENAME_DEFINITION, m_renameID);
	DDX_Control(pDX, IDC_TAG_DEF_EDITOR, m_tagDefEditor);
}

BOOL CFragmentBrowser::OnInitDialog()
{
	__super::OnInitDialog();

	ModifyStyleEx( 0, WS_EX_CLIENTEDGE );

	SetWindowText( "Fragment Browser" );

	if (CMFCUtils::LoadTrueColorImageList(m_imageList, IDB_ANIMATIONS_TREE, 16, RGB(255,0,255) ))
	{
		m_TreeCtrl.SetImageList(&m_imageList, TVSIL_NORMAL);
	}

	CMFCUtils::LoadTrueColorImageList(m_buttonImages, IDB_MANN_TAGDEF_TOOLBAR, 16, RGB(255, 0, 255));

	m_pDropTarget = new CFragmentBrowserBaseDropTarget(this);
	m_pDropTarget->Register(this);

	m_newEntry.SetImage(m_buttonImages, 8);
	m_deleteEntry.SetImage(m_buttonImages, 10);
	m_editEntry.SetImage(m_buttonImages, 9);
	m_newID.SetImage(m_buttonImages, 2);
	m_deleteID.SetImage(m_buttonImages, 4);
	m_renameID.SetImage(m_buttonImages, 3);
	m_tagDefEditor.SetImage(m_buttonImages, 6);

	CStatic* pStatic = (CStatic*)GetDlgItem(IDC_FRAGMENT_FILTER_ICON);
	if (pStatic != NULL)
	{
		CImage img;
		img.Load("Editor\\UI\\Icons\\mann_folder.png");
		pStatic->SetBitmap(img.Detach());
	}
	pStatic = (CStatic*)GetDlgItem(IDC_TAG_FILTER_ICON);
	if (pStatic != NULL)
	{
		CImage img;
		img.Load("Editor\\UI\\Icons\\mann_tag.png");
		pStatic->SetBitmap(img.Detach());
	}
	pStatic = (CStatic*)GetDlgItem(IDC_ANIM_CLIP_FILTER_ICON);
	if (pStatic != NULL)
	{
		CImage img;
		img.Load("Editor\\Icons\\animation\\animation.png");
		pStatic->SetBitmap(img.Detach());
	}

	SetResize( IDC_CONTEXT, SZ_TOP_LEFT, SZ_TOP_RIGHT );
	SetResize( IDC_TREE_FB, SZ_TOP_LEFT, SZ_BOTTOM_RIGHT );
	SetResize( IDC_NEW, SZ_BOTTOM_LEFT, CXTResizePoint(1.f/3, 1) );
	SetResize( IDDELETE, CXTResizePoint(1.f/3, 1), CXTResizePoint(2.f/3, 1) );
	SetResize( IDEDIT, CXTResizePoint(2.f/3, 1), SZ_BOTTOM_RIGHT );
	SetResize( IDC_NEW_DEFINITION, SZ_BOTTOM_LEFT, CXTResizePoint(1.f/3, 1) );
	SetResize( IDC_DELETE_DEFINITION, CXTResizePoint(1.f/3, 1), CXTResizePoint(2.f/3, 1) );
	SetResize( IDC_RENAME_DEFINITION, CXTResizePoint(2.f/3, 1), SZ_BOTTOM_RIGHT );
	SetResize( IDC_TAG_DEF_EDITOR, SZ_BOTTOM_LEFT, SZ_BOTTOM_RIGHT );
	SetResize( IDC_STATIC, SZ_TOP_LEFT, SZ_TOP_LEFT );
	SetResize( IDC_EDIT_ADBFILE, SZ_TOP_LEFT, SZ_TOP_RIGHT );
	SetResize( IDC_FRAGMENT_SUBFOLDERS_CHK, SZ_TOP_RIGHT, SZ_TOP_RIGHT );
	SetResize( IDC_FRAGMENT_SHOWEMPTY_CHK, SZ_TOP_RIGHT, SZ_TOP_RIGHT );
	SetResize( IDC_FILTER_TAGS, SZ_TOP_LEFT, SZ_TOP_RIGHT );
	SetResize( IDC_FILTER_FRAGMENT, SZ_TOP_LEFT, SZ_TOP_RIGHT );
	SetResize( IDC_FILTER_ANIM_CLIP, SZ_TOP_LEFT, SZ_TOP_RIGHT );
	SetResize( IDC_FRAGMENT_FILTER_ICON, SZ_TOP_LEFT, SZ_TOP_LEFT );
	SetResize( IDC_TAG_FILTER_ICON, SZ_TOP_LEFT, SZ_TOP_LEFT );
	SetResize( IDC_ANIM_CLIP_FILTER_ICON, SZ_TOP_LEFT, SZ_TOP_LEFT );

	m_editFilterTags.SetWindowText( DEFAULT_TAG_FILTER_TEXT );
	m_editFilterTags.SetSel(0,DEFAULT_TAG_FILTER_TEXT.GetLength());
	m_editFilterFragmentIDs.SetWindowText( DEFAULT_FRAGMENT_ID_FILTER_TEXT );
	m_editFilterFragmentIDs.SetSel(0,DEFAULT_FRAGMENT_ID_FILTER_TEXT.GetLength());
	m_editAnimClipFilter.SetWindowText( DEFAULT_ANIM_CLIP_FILTER_TEXT );
	m_editAnimClipFilter.SetSel(0,DEFAULT_ANIM_CLIP_FILTER_TEXT.GetLength());

	UpdateControlEnabledStatus();

	return TRUE;
}

void CFragmentBrowser::OnDestroy()
{
	if ( m_pDropTarget )
	{
		delete m_pDropTarget;
	}
	__super::OnDestroy();
}

void CFragmentBrowser::UpdateControlEnabledStatus()
{
	HTREEITEM item = m_TreeCtrl.GetSelectedItem();

	if ( !item || m_TreeCtrl.ItemHasChildren(item) || !m_TreeCtrl.GetParentItem(item) )
	{
		GetDlgItem( IDDELETE )->EnableWindow( FALSE );
		GetDlgItem( IDEDIT )->EnableWindow( FALSE );
	}
	else
	{
		GetDlgItem( IDDELETE )->EnableWindow( TRUE );
		GetDlgItem( IDEDIT )->EnableWindow( TRUE );
	}

	if ( !item )
	{
		GetDlgItem( IDC_NEW )->EnableWindow( FALSE );
		GetDlgItem( IDC_RENAME_DEFINITION )->EnableWindow( FALSE );
		GetDlgItem( IDC_DELETE_DEFINITION )->EnableWindow( FALSE );
	}
	else
	{
		GetDlgItem( IDC_NEW )->EnableWindow( TRUE );
		GetDlgItem( IDC_RENAME_DEFINITION )->EnableWindow( TRUE );
		GetDlgItem( IDC_DELETE_DEFINITION )->EnableWindow( TRUE );
	}
}

void CFragmentBrowser::SetContext(SMannequinContexts &context)
{
	m_context = &context;

	m_cbContext.ResetContent();
	const uint32 numContexts = context.m_contextData.size();
	for (uint32 i=0; i<numContexts; i++)
	{
		m_cbContext.AddString(context.m_contextData[i].name);
	}
	m_cbContext.SetCurSel(0);

	if (context.m_controllerDef)
	{
		SetScopeContext(0);
	}
}

BOOL CFragmentBrowser::PreTranslateMessage(MSG* pMsg)
{
	m_toolTip.RelayEvent(pMsg);

	return PARENT::PreTranslateMessage(pMsg);
}

void CFragmentBrowser::RebuildAll()
{
	// Clear old data
	m_fragEditor.FlushChanges();

	// WARNING: clear m_fragmentItems before m_TreeCtrl.DeleteAllItems() because
	//   DeleteAllItems() might still send a SelectItem message, which will
	//   do a lookup in the m_fragmentItems and subsequently the context,
	//   which could be switched already
	m_fragmentItems.clear();

	m_TreeCtrl.DeleteAllItems();
	for (uint32 i=0; i<m_fragmentData.size(); i++)
	{
		delete(m_fragmentData[i]);
	}
	m_fragmentData.resize(0);

	// Fill new data & try to reselect currently selected item
	if (m_animDB)
	{
		const CTagDefinition &fragDefs	= m_animDB->GetFragmentDefs();
		const uint32 numFrags = fragDefs.GetNum();

		for (uint32 i=0; i<numFrags; i++)
		{
			const string fragTagName = fragDefs.GetTagName(i);
			HTREEITEM item = m_TreeCtrl.InsertItem( fragTagName, IMAGE_FOLDER, IMAGE_FOLDER, 0, TVI_SORT );
			m_fragmentItems.push_back(item);
		}

		for (uint32 i=0; i<numFrags; i++)
		{
			BuildFragment(i);
		}

		if (!m_showEmptyFolders)
		{
			for (uint32 i=0; i<numFrags; i++)
			{
				if (!m_TreeCtrl.ItemHasChildren(m_fragmentItems[i]))
				{
					m_TreeCtrl.DeleteItem(m_fragmentItems[i]);
					m_fragmentItems[i] = 0;
				}
			}
		}

		if (m_filterFragmentIDText.GetLength()>0)
		{
			for (uint32 i=0; i<numFrags; i++)
			{
				if (m_fragmentItems[i]!=0)
				{
					string fragTagName = fragDefs.GetTagName(i);
					fragTagName.MakeLower();
					if (fragTagName.find(m_filterFragmentIDText)==string::npos)
					{
						m_TreeCtrl.DeleteItem(m_fragmentItems[i]);
						m_fragmentItems[i] = 0;
					}
				}
			}
		}
	}

	OnSelectionChanged();
}

void CFragmentBrowser::SetDatabase(IAnimationDatabase *animDB)
{
	if (m_animDB != animDB)
	{
		m_animDB = animDB;

		RebuildAll();

		if (m_animDB)
		{
			if (m_fragmentID != FRAGMENT_ID_INVALID)
			{
				m_editedItem = 0;
				SelectFragment(m_fragmentID, m_tagState, m_option);
			}
		}
		else
		{
			m_editedItem = 0;
			m_fragmentID = FRAGMENT_ID_INVALID;
			m_tagState = SFragTagState();
			m_option = 0;
		}
	}
}

HTREEITEM CFragmentBrowser::FindFragmentItem(FragmentID fragmentID, const SFragTagState &tagState, uint32 option) const
{
	for (uint32 i=0; i<m_fragmentData.size(); i++)
	{
		STreeFragmentData &fragData = *m_fragmentData[i];
		if ((fragData.fragID == fragmentID) && (fragData.tagState == tagState) && (fragData.option == option))
		{
			return fragData.item;
		}
	}

	return NULL;
}

bool CFragmentBrowser::SelectFragment(FragmentID fragmentID, const SFragTagState &tagState, uint32 option)
{
	SFragTagState bestTagState;
	uint32 numOptions = m_animDB->FindBestMatchingTag(SFragmentQuery(fragmentID, tagState), &bestTagState);
	if (numOptions > 0)
	{
		option = option%numOptions;
		HTREEITEM selItem = FindFragmentItem(fragmentID, bestTagState, option);

		if (selItem)
		{
			//--- Found tag node
			HTREEITEM fragItem = m_TreeCtrl.GetParentItem(selItem);
			m_TreeCtrl.Expand(fragItem, TVE_EXPAND);
			m_TreeCtrl.SelectItem(selItem);

			SetEditItem(selItem, fragmentID, bestTagState, option);
			m_onSelectedItemEditCallback(fragmentID, GetItemText(fragItem));
		}

		m_fragEditor.SetFragment(fragmentID, bestTagState, option);
			
		CMannequinDialog::GetCurrentInstance()->GetDockingPaneManager()->ShowPane(CMannequinDialog::IDW_FRAGMENT_EDITOR_PANE);
		m_fragEditor.RedrawWindow();

		return true;
	}

	return false;
}

IAnimationDatabase *CFragmentBrowser::FindHostDatabase(uint32 contextID, const SFragTagState &tagState) const
{
	const SScopeContextData *contextData = m_context->GetContextDataForID(tagState.globalTags, contextID, eMEM_FragmentEditor);
	if (contextData && contextData->database)
	{
		return contextData->database;
	}

	return m_animDB;
}

bool CFragmentBrowser::SetTagState(const SFragTagState &newTags)
{
	if ((m_fragmentID != FRAGMENT_ID_INVALID) && (newTags != m_tagState))
	{
		//--- Ensure edited fragment is stored
		m_fragEditor.FlushChanges();

		SFragmentQuery query(m_fragmentID, m_tagState, m_option);
		const CFragment *fragment = m_animDB->GetBestEntry(query);

		uint32 newOption = m_option;
		if (fragment)
		{
			IMannequin &mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
			newOption = mannequinSys.GetMannequinEditorManager()->AddFragmentEntry(m_animDB, m_fragmentID, newTags, *fragment);
			mannequinSys.GetMannequinEditorManager()->DeleteFragmentEntry(m_animDB, m_fragmentID, m_tagState, m_option);
		}

		CleanFragment(m_fragmentID);
		BuildFragment(m_fragmentID);

		return SelectFragment(m_fragmentID, newTags, newOption);
	}
	return false;
}

// Left Drag helper
COleDataSource* CFragmentBrowser::CreateFragmentDescriptionDataSource(STreeFragmentData *fragmentData) const
{
	if (fragmentData)
	{
		CSequencerDopeSheetBase::SDropFragmentData fData = 
		{
			fragmentData->fragID, 
			fragmentData->tagState,
			fragmentData->option
		};
		CSharedFile	sf(GMEM_MOVEABLE|GMEM_DDESHARE|GMEM_ZEROINIT);
		sf.Write(&fData, sizeof(CSequencerDopeSheetBase::SDropFragmentData));
		HGLOBAL hMem = sf.Detach();
		if (!hMem)
			return 0;

		COleDataSource* dataSource = new COleDataSource();
		dataSource->CacheGlobalData(EditorDragDropHelpers::GetFragmentClipboardFormat(), hMem);

		return dataSource;
	}

	return 0;
}

void CFragmentBrowser::OnBeginDrag(NMHDR* pNMHdr, LRESULT* pResult)
{
	NMTREEVIEWA* nmtv = (NMTREEVIEWA*) pNMHdr;

	HTREEITEM dragItem = nmtv->itemNew.hItem;
	if ( dragItem )
	{
		STreeFragmentData *fragmentData = (STreeFragmentData *)m_TreeCtrl.GetItemData(dragItem);
		if (fragmentData && !fragmentData->tagSet)
		{
			COleDataSource* dataSource = CreateFragmentDescriptionDataSource(fragmentData);
			if (dataSource)
			{
				DROPEFFECT dropEffect = dataSource->DoDragDrop(DROPEFFECT_MOVE | DROPEFFECT_LINK);
			}
		}
	}
}


void CFragmentBrowser::OnBeginRDrag(NMHDR* pNMHdr, LRESULT* pResult)
{
	NMTREEVIEWA* nmtv = (NMTREEVIEWA*) pNMHdr;
	m_dragItem = nmtv->itemNew.hItem;
	if ( m_dragItem )
	{
		STreeFragmentData *fragmentData = (STreeFragmentData *)m_TreeCtrl.GetItemData(m_dragItem);
		if (fragmentData && !fragmentData->tagSet)
		{
			RECT rcItem;        // bounding rectangle of item 
			m_draggingImage = m_TreeCtrl.CreateDragImage(m_dragItem);

			// Get the bounding rectangle of the item being dragged. 
			m_TreeCtrl.GetItemRect(m_dragItem, &rcItem, false);

			if (m_draggingImage)
			{
				// Start the drag operation. 
				CPoint pos = nmtv->ptDrag;
				//ClientToScreen(&pos);
				m_draggingImage->BeginDrag(0, CPoint(0, 0));
				m_draggingImage->DragEnter(this, pos);
				//ImageList_BeginDrag(m_draggingImage, 0, 0, 0);
				//ImageList_DragEnter(nmtv->hwndFrom, nmtv->ptDrag.x, nmtv->ptDrag.x);
			}

			// Hide the mouse pointer, and direct mouse input to the 
			// parent window. 
			ShowCursor(FALSE); 
			SetCapture(); 

			m_rightDrag = true;
		}
	}
}


void CFragmentBrowser::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (m_rightDrag)
	{
		POINT dragPt = point;
		ClientToScreen(&dragPt);
		m_TreeCtrl.ScreenToClient(&dragPt);

		HTREEITEM hitTarget = m_TreeCtrl.HitTest(dragPt);

		if (m_draggingImage)
		{
			CPoint pos = point;
			//ClientToScreen(&pos);
			m_draggingImage->DragMove(pos);
			//ImageList_DragMove(point.x, point.y);
			// Turn off the dragged image so the background can be refreshed.
			m_draggingImage->DragShowNolock(FALSE);
			//ImageList_DragShowNolock(FALSE); 

			if (hitTarget)
			{
				m_TreeCtrl.EnsureVisible(m_TreeCtrl.GetNextVisibleItem(hitTarget));
				m_TreeCtrl.EnsureVisible(m_TreeCtrl.GetPrevVisibleItem(hitTarget));
			}
		}

		if (hitTarget) 
		{ 
			HTREEITEM itemParent = m_TreeCtrl.GetParentItem(hitTarget);

			if ( itemParent )
			{
				hitTarget = itemParent;
			}

			m_TreeCtrl.SelectDropTarget(hitTarget);
		} 
		if (m_draggingImage)
		{
			//ImageList_DragShowNolock(TRUE);
			m_draggingImage->DragShowNolock(TRUE);
		}
	}
}

void CFragmentBrowser::OnLButtonUp(UINT nFlags, CPoint point) 
{
}

void CFragmentBrowser::OnRButtonUp(UINT nFlags, CPoint point) 
{
	if (m_rightDrag)
	{
		// Get destination item.
		HTREEITEM hitDest = m_TreeCtrl.GetDropHilightItem();
		if (hitDest != NULL)
		{
			STreeFragmentData *fragmentDataFrom = (STreeFragmentData *)m_TreeCtrl.GetItemData(m_dragItem);
			if (fragmentDataFrom)
			{
				STreeFragmentData *fragmentDataTo = (STreeFragmentData *)m_TreeCtrl.GetItemData(hitDest);
				FragmentID fragID;
				SFragTagState newTagState = fragmentDataFrom->tagState;
				if (fragmentDataTo)
				{
					fragID = fragmentDataTo->fragID;
					newTagState = fragmentDataTo->tagState;
				}
				else
				{
					CString szFragID = m_TreeCtrl.GetItemText( hitDest );
					fragID = m_animDB->GetFragmentID(szFragID);

					if (fragID == FRAGMENT_ID_INVALID)
						return;
				}

				m_fragEditor.FlushChanges();

				SFragmentQuery query(fragmentDataFrom->fragID, fragmentDataFrom->tagState, fragmentDataFrom->option);
				const CFragment *cloneFrag = m_animDB->GetBestEntry(query);
				IMannequin &mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
				uint32 newOption = mannequinSys.GetMannequinEditorManager()->AddFragmentEntry(m_animDB, fragID, newTagState, *cloneFrag);
				
				CleanFragment(fragID);
				BuildFragment(fragID);

				SelectFragment(fragID, newTagState, newOption);
			}
		}
		if (m_draggingImage)
		{
			m_draggingImage->EndDrag();
		}

		m_TreeCtrl.SelectDropTarget(NULL);
		ReleaseCapture(); 
		ShowCursor(TRUE); 
		m_rightDrag = false;
	}
}

void CFragmentBrowser::OnTVClick(NMHDR* pNMHdr, LRESULT* pResult)
{
}

#define CMD_COPY_SELECTED_TEXT						1
#define CMD_FIND_FRAGMENT_REFERENCES				2
#define CMD_FIND_TAG_REFERENCES						3

void CFragmentBrowser::OnTVRightClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	if ( HTREEITEM item = m_TreeCtrl.GetSelectedItem() )
	{
		CMenu menu;
		menu.CreatePopupMenu();

		CString	selectedNodeText = GetSelectedNodeText();

		// Copy functionality
		if (selectedNodeText.GetLength() > 0)
		{
			menu.AppendMenu( MF_STRING, CMD_COPY_SELECTED_TEXT, "Copy selected text");
			menu.AppendMenu(MF_SEPARATOR);
		}

		// Search functionality
		HTREEITEM root = m_TreeCtrl.GetParentItem(item);
		if (root != NULL)
		{
			menu.AppendMenu(MF_STRING, CMD_FIND_TAG_REFERENCES, "Find tag transitions");
		}
		menu.AppendMenu(MF_STRING, CMD_FIND_FRAGMENT_REFERENCES, "Find fragment transitions");


		CPoint p;
		::GetCursorPos( &p );
		int res = ::TrackPopupMenuEx( menu.GetSafeHmenu(),TPM_LEFTBUTTON|TPM_RETURNCMD,p.x,p.y,GetSafeHwnd(),NULL );

		CMannequinDialog* pMannequinDialog = CMannequinDialog::GetCurrentInstance();

		switch (res)
		{
		case CMD_COPY_SELECTED_TEXT:
			{
				CClipboard clipboard;
				clipboard.PutString(selectedNodeText);
				break;
			}
		case CMD_FIND_FRAGMENT_REFERENCES:
			{
				// Find highest root (the fragment)
				while (root != NULL)
				{
					item = root;
					root = m_TreeCtrl.GetParentItem(item);
				}

				CString fragmentName = m_TreeCtrl.GetItemText(item);

				if (pMannequinDialog != NULL)
				{
					pMannequinDialog->FindFragmentReferences(fragmentName);
				}
			} break;
		case CMD_FIND_TAG_REFERENCES:
			{
				// GetSelectedNodeText (used earlier) doesn't actually get the text on the node...
				CString tagName = m_TreeCtrl.GetItemText(item);

				if (pMannequinDialog != NULL)
				{
					pMannequinDialog->FindTagReferences(tagName);
				}
			} break;
		}
	}
}

void CFragmentBrowser::OnTVEditSel(NMHDR*, LRESULT*)
{
	EditSelectedItem();
	m_TreeCtrl.SetFocus();
}


void CFragmentBrowser::OnTreeKeyDown(NMHDR *pNotifyStruct, LRESULT*)
{
	if (!m_animDB)
	{
		return;
	}

	const LPNMTVKEYDOWN lpNMKey = (LPNMTVKEYDOWN)pNotifyStruct;
	switch (lpNMKey->wVKey)
	{	
	case 'C':
		if (GetAsyncKeyState(VK_CONTROL))
		{
			//--- Copy
			m_fragEditor.FlushChanges();

			const HTREEITEM copySourceItem = m_TreeCtrl.GetSelectedItem();
			const STreeFragmentData *const pFragmentDataCopy = (STreeFragmentData *)m_TreeCtrl.GetItemData(copySourceItem);
			const FragmentID fragmentID = pFragmentDataCopy ? pFragmentDataCopy->fragID : m_animDB->GetFragmentID(m_TreeCtrl.GetItemText(copySourceItem));

			m_copiedItems.clear();
			m_copiedFragmentID = fragmentID;

			if (!pFragmentDataCopy || pFragmentDataCopy->tagSet)
			{
				// Copy all fragments matching the given tag set or FragmentID
				const SFragTagState& sharedTagState = pFragmentDataCopy ? pFragmentDataCopy->tagState : SFragTagState();
				const CTagDefinition& globalTagDef = m_animDB->GetTagDefs();
				const CTagDefinition* pFragTagDef = m_animDB->GetFragmentDefs().GetSubTagDefinition(fragmentID);
				const uint32 numTagSets = m_animDB->GetTotalTagSets(fragmentID);
				for (uint32 itTagSets = 0; itTagSets < numTagSets; ++itTagSets)
				{
					SFragTagState entryTagState;
					const uint32 numOptions = m_animDB->GetTagSetInfo(fragmentID, itTagSets, entryTagState);
					const bool globalTagsMatch = globalTagDef.Contains(entryTagState.globalTags, sharedTagState.globalTags);
					const bool fragmentTagsMatch = !pFragTagDef || pFragTagDef->Contains(entryTagState.fragmentTags, sharedTagState.fragmentTags);
					const bool subFolders = m_chkShowSubFolders.GetCheck() != 0;
					const bool tagsMatch = subFolders ? globalTagsMatch && fragmentTagsMatch : sharedTagState == entryTagState;
					if (!tagsMatch)
					{
						continue;
					}

					// Strip parent tags out
					const HTREEITEM parentItem = m_TreeCtrl.GetParentItem(copySourceItem);
					const STreeFragmentData *const pParentViewData = parentItem ? (STreeFragmentData*)m_TreeCtrl.GetItemData(parentItem) : 0;
					if (pParentViewData)
					{
						entryTagState.globalTags = globalTagDef.GetDifference(entryTagState.globalTags, pParentViewData->tagState.globalTags);
						entryTagState.fragmentTags = pFragTagDef ? pFragTagDef->GetDifference(entryTagState.fragmentTags, pParentViewData->tagState.fragmentTags) : entryTagState.fragmentTags;
					}

					// Copy fragments
					for (uint32 itOptions = 0; itOptions < numOptions; ++itOptions)
					{
						const CFragment* const pFragment = m_animDB->GetEntry(fragmentID, itTagSets, itOptions);
						CRY_ASSERT(pFragment);
						m_copiedItems.push_back(SCopyItem(*pFragment, entryTagState));
					}
				}
			}
			else
			{
				// Copy selected fragment
				SFragmentQuery query(pFragmentDataCopy->fragID, pFragmentDataCopy->tagState, pFragmentDataCopy->option);
				const CFragment *cloneFrag = m_animDB->GetBestEntry(query);
				if (cloneFrag!=NULL)
				{
					m_copiedItems.push_back(SCopyItem(*cloneFrag, SFragTagState()));
				}
			}
		}
		break;
	case 'V':
		if (GetAsyncKeyState(VK_CONTROL))
		{
			//--- Paste
			if (!m_copiedItems.empty())
			{
				m_fragEditor.FlushChanges();

				const HTREEITEM targetItem = m_TreeCtrl.GetSelectedItem();
				const STreeFragmentData *const pFragmentDataTo = (STreeFragmentData *)m_TreeCtrl.GetItemData(targetItem);

				IMannequin &mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
				const FragmentID destFragmentID = pFragmentDataTo ? pFragmentDataTo->fragID : m_animDB->GetFragmentID(m_TreeCtrl.GetItemText(targetItem));
				SFragTagState copyBaseTagState = pFragmentDataTo ? pFragmentDataTo->tagState : SFragTagState();

				if (GetAsyncKeyState(VK_SHIFT))
				{
					// Advanced pasting: let the user change the destination base tag state
					CMannAdvancedPasteDialog advancedPasteDialog(this, *m_animDB, destFragmentID, copyBaseTagState);
					if (advancedPasteDialog.DoModal() == IDOK)
					{
						copyBaseTagState = advancedPasteDialog.GetTagState();
					}
					else
					{
						break;
					}
				}

				for (TCopiedItems::const_iterator itCopiedItems = m_copiedItems.begin(); itCopiedItems != m_copiedItems.end(); ++itCopiedItems)
				{
					if (destFragmentID != FRAGMENT_ID_INVALID)
					{
						// Convert tag state
						// For fragment tags, we need to do a CRC look-up since the tag definitions might not be the exact same
						SFragTagState insertionTagState = copyBaseTagState;
						const CTagDefinition& globalTagDef = m_animDB->GetTagDefs();
						const CTagDefinition* pFragTagDefDest = m_animDB->GetFragmentDefs().GetSubTagDefinition(destFragmentID);
						const CTagDefinition* pFragTagDefSource = m_animDB->GetFragmentDefs().GetSubTagDefinition(destFragmentID);

						const auto transferTags = [](const CTagDefinition& sourceDef, const CTagDefinition& destDef, const TagState& sourceTagState, TagState& destTagState)
						{
							const TagID numSourceTags = sourceDef.GetNum();
							for (TagID itSourceTags = 0; itSourceTags < numSourceTags; ++itSourceTags)
							{
								if (sourceDef.IsSet(sourceTagState, itSourceTags))
								{
									// Tag set in the source state, set it in the destination only if there's no tag set within the same group
									const TagID destTagID = destDef.Find(sourceDef.GetTagCRC(itSourceTags));
									const TagGroupID groupId = destTagID != TAG_ID_INVALID ? destDef.GetGroupID(destTagID) : GROUP_ID_NONE;
									const bool nonExclusive = groupId == GROUP_ID_NONE || !destDef.IsGroupSet(destTagState, groupId);
									if (destTagID != TAG_ID_INVALID && nonExclusive)
									{
										destDef.Set(destTagState, destTagID, true);
									}
								}
							}
						};
						transferTags(globalTagDef, globalTagDef, itCopiedItems->tagState.globalTags, insertionTagState.globalTags);
						if (pFragTagDefSource && pFragTagDefDest)
						{
							transferTags(*pFragTagDefSource, *pFragTagDefDest, itCopiedItems->tagState.fragmentTags, insertionTagState.fragmentTags);
						}

						const uint32 newOption = mannequinSys.GetMannequinEditorManager()->AddFragmentEntry(m_animDB, destFragmentID, insertionTagState, itCopiedItems->fragment);

						const bool isLast = itCopiedItems + 1 == m_copiedItems.end();
						if (isLast)
						{
							CleanFragment(destFragmentID);
							BuildFragment(destFragmentID);
							SelectFragment(destFragmentID, insertionTagState, newOption);
						}
					}
				}
			}
		}
		break;
	}
}

void CFragmentBrowser::SetEditItem(HTREEITEM editedItem, FragmentID fragmentID, const SFragTagState &tagState, uint32 option)
{
	if (m_editedItem)
	{
		m_TreeCtrl.SetItemState(m_editedItem, 0, TVIS_BOLD);
	}

	m_editedItem = editedItem;
	if (editedItem)
	{
		m_TreeCtrl.SetItemState(m_editedItem, TVIS_BOLD, TVIS_BOLD);
	}
	m_fragmentID = fragmentID;
	m_tagState = tagState;
	m_option = option;
}

void CFragmentBrowser::SetADBFileNameTextToCurrent()
{
	HTREEITEM rootItem = m_TreeCtrl.GetSelectedItem();
	if ( rootItem )
	{
		while ( m_TreeCtrl.GetParentItem(rootItem) != NULL )
			rootItem = m_TreeCtrl.GetParentItem(rootItem);

		CString IDName = m_TreeCtrl.GetItemText(rootItem);
		const char* filename = m_animDB == NULL ? "" : m_animDB->FindSubADBFilenameForID(m_animDB->GetFragmentID(IDName.GetString()));

		CString adbName = Path::GetFile(filename);
		m_CurrFile.SetWindowTextA (adbName.GetString());
	}
} 


void CFragmentBrowser::OnTVSelChanged(NMHDR*, LRESULT*)
{
	OnSelectionChanged();
}


void CFragmentBrowser::OnSelectionChanged()
{
	UpdateControlEnabledStatus();
	SetADBFileNameTextToCurrent();
}

void CFragmentBrowser::OnChangeFilterTags()
{
	CString filter;
	m_editFilterTags.GetWindowText(filter);

	if (filter[0] != DEFAULT_TAG_FILTER_TEXT[0] && filter != m_filterText)
	{
		m_filterText = filter;
		m_filterDelay = FILTER_DELAY_SECONDS;

		int Position = 0;
		CString token;

		m_filters.clear();
		token = filter.Tokenize(" ", Position);
		while(!token.IsEmpty())
		{
			m_filters.push_back(token.MakeLower());
			token = filter.Tokenize(" ", Position);
		} 

		// The RebuildAll() is now called in Update() when the filter delay reaches 0
	}

	if (filter.GetLength()==0) {
		m_editFilterTags.SetWindowText( DEFAULT_TAG_FILTER_TEXT );
		m_editFilterTags.SetSel(0,DEFAULT_TAG_FILTER_TEXT.GetLength());
	}
}

void CFragmentBrowser::OnChangeFilterFragments()
{
	CString filter;
	m_editFilterFragmentIDs.GetWindowText(filter);

	if (filter[0] != DEFAULT_FRAGMENT_ID_FILTER_TEXT[0] && filter != m_filterFragmentIDText)
	{
		m_filterFragmentIDText = filter.MakeLower();
		m_filterDelay = FILTER_DELAY_SECONDS;

		// The RebuildAll() is now called in Update() when the filter delay reaches 0
	}

	if (filter.GetLength()==0) {
		m_editFilterFragmentIDs.SetWindowText( DEFAULT_FRAGMENT_ID_FILTER_TEXT );
		m_editFilterFragmentIDs.SetSel(0,DEFAULT_FRAGMENT_ID_FILTER_TEXT.GetLength());
	}
}

void CFragmentBrowser::OnChangeFilterAnimClip()
{
	CString filter;
	m_editAnimClipFilter.GetWindowText(filter);

	if (filter[0] != DEFAULT_ANIM_CLIP_FILTER_TEXT[0] && filter != m_filterAnimClipText)
	{
		m_filterAnimClipText = filter.MakeLower();
		m_filterDelay = FILTER_DELAY_SECONDS;

		// The RebuildAll() is now called in Update() when the filter delay reaches 0
	}

	if (filter.GetLength()==0) {
		m_editAnimClipFilter.SetWindowText( DEFAULT_ANIM_CLIP_FILTER_TEXT );
		m_editAnimClipFilter.SetSel(0,DEFAULT_ANIM_CLIP_FILTER_TEXT.GetLength());
	}
}

void CFragmentBrowser::OnToggleSubfolders()
{
	bool newCheck = (m_chkShowSubFolders.GetCheck() != 0);

	if (newCheck != m_showSubFolders)
	{
		m_showSubFolders = newCheck;
		RebuildAll();
	}
}

void CFragmentBrowser::OnToggleShowEmpty()
{
	bool newCheck = (m_chkShowEmptyFolders.GetCheck() != 0);

	if (newCheck != m_showEmptyFolders)
	{
		m_showEmptyFolders = newCheck;
		RebuildAll();
	}
}

void CFragmentBrowser::OnTagDefEditorBtn()
{
	CMannequinDialog::GetCurrentInstance()->LaunchTagDefinitionEditor();
}

FragmentID CFragmentBrowser::GetSelectedFragmentId() const
{
	const HTREEITEM item = m_TreeCtrl.GetSelectedItem();
	if ( !item )
	{
		return FRAGMENT_ID_INVALID;
	}

	const STreeFragmentData* pFragData = reinterpret_cast< STreeFragmentData* >( m_TreeCtrl.GetItemData( item ) );
	if ( !pFragData )
	{
		const size_t fragmentItemsCount = m_fragmentItems.size();
		for ( size_t i = 0; i < fragmentItemsCount; ++i )
		{
			if ( m_fragmentItems[ i ] == item )
			{
				const FragmentID fragmentId = static_cast< FragmentID >( i );
				return fragmentId;
			}
		}
		return FRAGMENT_ID_INVALID;
	}

	const FragmentID fragmentId = pFragData->fragID;
	return fragmentId;
}

CString CFragmentBrowser::GetSelectedNodeText() const
{
	if (HTREEITEM item = m_TreeCtrl.GetSelectedItem())
		return GetItemText(item);
	else	
		return "";
}

void CFragmentBrowser::OnShowWindow(BOOL bShow, UINT nStatus)
{
}

void CFragmentBrowser::OnChangeContext()
{
	SetScopeContext(m_cbContext.GetCurSel());
}

void CFragmentBrowser::SetScopeContext(int scopeContextID)
{
	const bool rangeValid = (0 <= scopeContextID) && (scopeContextID < m_context->m_contextData.size());
	const int modifiedScopeContextID = rangeValid ? scopeContextID : -1;

	if (modifiedScopeContextID != m_scopeContext)
	{
		m_scopeContext = modifiedScopeContextID;

		IAnimationDatabase* const pAnimationDatabase = rangeValid ? m_context->m_contextData[modifiedScopeContextID].database : NULL;

		SetDatabase(pAnimationDatabase);

		m_cbContext.SetCurSel(modifiedScopeContextID);

		if (m_onScopeContextChangedCallback)
		{
			m_onScopeContextChangedCallback();
		}
	}
}

void CFragmentBrowser::OnDeleteBtn()
{
	if (!m_animDB)
		return;

	const FragmentID selectedFragmentId = GetSelectedFragmentId();
	if (selectedFragmentId == FRAGMENT_ID_INVALID)
		return;

	if ( HTREEITEM item = m_TreeCtrl.GetSelectedItem() )
	{
		CString message;
		const char *fragmentName = m_animDB->GetFragmentDefs().GetTagName(selectedFragmentId);
		message.Format("Delete the fragment '%s [%s]'?", fragmentName, GetSelectedNodeText().GetBuffer());

		if (MessageBox(message.GetBuffer(), "CryMannequin", MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			STreeFragmentData *fragData = (STreeFragmentData *)m_TreeCtrl.GetItemData(item);
			if (fragData && !fragData->tagSet)
			{
				m_fragEditor.FlushChanges();

				FragmentID fragID = fragData->fragID;
				IMannequin &mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
				mannequinSys.GetMannequinEditorManager()->DeleteFragmentEntry(m_animDB, fragID, fragData->tagState, fragData->option);

				CleanFragment(fragID);
				BuildFragment(fragID);

				SelectFragment(m_fragmentID, m_tagState, m_option);

				EditSelectedItem();
			}
		}
	}
}

void CFragmentBrowser::EditSelectedItem()
{
	if ( HTREEITEM item = m_TreeCtrl.GetSelectedItem() )
	{
		STreeFragmentData *pFragData = (STreeFragmentData *)m_TreeCtrl.GetItemData(item);

		if (pFragData)
		{
			SetEditItem(item, pFragData->fragID, pFragData->tagState, pFragData->option);
			m_onSelectedItemEditCallback(pFragData->fragID, GetItemText(item));
			CMannequinDialog::GetCurrentInstance()->GetDockingPaneManager()->ShowPane(CMannequinDialog::IDW_FRAGMENT_EDITOR_PANE);
			m_fragEditor.SetFragment(pFragData->fragID, pFragData->tagState, pFragData->option);
			SelectFirstKey();
			m_fragEditor.RedrawWindow();
			return;
		}
	}

}

CString CFragmentBrowser::GetItemText(HTREEITEM item) const
{
	if (m_showSubFolders)
	{
		// We don't want the "Option 1" bit...
		item = m_TreeCtrl.GetParentItem(item);
	}

	// Get the tag name
	CString tags = m_TreeCtrl.GetItemText(item);
	item = m_TreeCtrl.GetParentItem(item);

	// Build a string of "+" separated tag names (omitting the root parent)
	while (m_TreeCtrl.GetParentItem(item) != NULL)
	{
		tags = m_TreeCtrl.GetItemText(item) + "+" + tags;
		item = m_TreeCtrl.GetParentItem(item);
	}

	return tags;
}

void CFragmentBrowser::OnEditBtn()
{
	EditSelectedItem();
}

void CFragmentBrowser::CleanFragmentID(FragmentID fragID)
{	
	// First remove the child fragment nodes
	CleanFragment(fragID);

	// Now the parent FragmentID
	HTREEITEM fragmentDefItem = m_fragmentItems[fragID];
	m_TreeCtrl.DeleteItem(fragmentDefItem);
	stl::find_and_erase(m_fragmentItems, fragmentDefItem);
	if (m_fragmentID == fragID)
		m_fragmentID = FRAGMENT_ID_INVALID;

	for (size_t i = 0; i < m_fragmentData.size(); i++)
	{
		STreeFragmentData *treeFragData = m_fragmentData[i];
		if (treeFragData->fragID > fragID)
			treeFragData->fragID--;
	}

	OnSelectionChanged(); // deleting the last item in the treeview doesn't call OnTVSelectChange
}

void CFragmentBrowser::CleanFragment(FragmentID fragID)
{
	//--- Remove old data
	const int numFragDatas = m_fragmentData.size();
	for (int i=numFragDatas-1; i>=0; i--)
	{
		STreeFragmentData *treeFragData = m_fragmentData[i];
		if (treeFragData->fragID == fragID)
		{
			m_TreeCtrl.DeleteItem(treeFragData->item);

			delete treeFragData;
			m_fragmentData.erase(m_fragmentData.begin()+i);
		}
	}
}

HTREEITEM CFragmentBrowser::FindInChildren( const char* szTagStr, HTREEITEM hCurrItem )
{
	if ( m_TreeCtrl.ItemHasChildren(hCurrItem))
	{
		HTREEITEM hChildItem = m_TreeCtrl.GetChildItem(hCurrItem);
		while (hChildItem != NULL)
		{
			
			CString name = m_TreeCtrl.GetItemText(hChildItem);
			if (!strcmp (name.GetString(), szTagStr))
				return hChildItem; 

			hChildItem = m_TreeCtrl.GetNextItem(hChildItem, TVGN_NEXT);
		}
	}
	return NULL;
}


// helper function 
struct sCompareTagPrio
{
	typedef std::pair<int, string> TTagPair; 
	const bool operator() ( const TTagPair &p1, const TTagPair &p2 ) const { return p1.first < p2.first; }
};


void CFragmentBrowser::BuildFragment(FragmentID fragID)
{
	HTREEITEM fragItem = m_fragmentItems[fragID];

	const uint32 numFilters = m_filters.size();

	//--- Insert new data
	char szTagStateBuff [512];
	const uint32 numTagSets = m_animDB->GetTotalTagSets(fragID);
	for (uint32 t=0; t<numTagSets; t++)
	{
		SFragTagState tagState;
		uint32 numOptions = m_animDB->GetTagSetInfo(fragID, t, tagState);

		MannUtils::FlagsToTagList(szTagStateBuff, 512, tagState, fragID, *m_context->m_controllerDef);
		CString sTagState ( szTagStateBuff );

		bool include = true;
		if (numFilters > 0)
		{
			CString sTagStateLC = sTagState.MakeLower();

			for (uint32 f=0; f<numFilters; f++)
			{
				if (sTagStateLC.Find(m_filters[f]) < 0)
				{
					include = false;
					break;
				}
			}
		}

		if (m_filterAnimClipText.GetLength() > 0)
		{
			for (uint32 o = 0; o < numOptions; o++)
			{
				const CFragment *fragment = m_animDB->GetEntry(fragID, t, o);

				bool bMatch = false;
				uint32 numALayers = fragment->m_animLayers.size();
				for (uint32 ali = 0; ali < numALayers; ali++)
				{
					const TAnimClipSequence& animLayer = fragment->m_animLayers[ali];
					const uint32 numClips = animLayer.size();
					for (uint32 c = 0; c < numClips; c++)
					{
						const SAnimClip& animClip = animLayer[c];
						if (strstr(animClip.animation.animRef.c_str(), m_filterAnimClipText))
						{
							bMatch = true;
							break;
						}
					}
				}
				if (!bMatch)
				{
					// Anim clip filter was specified, but didn't match anything in this fragment,
					// so don't add fragment to tree hierarchy.
					include = false;
				}
			}
		}

		if (!include)
			continue;

		HTREEITEM parent = fragItem;

		typedef std::vector < sCompareTagPrio::TTagPair > TTags;
		TTags vTags; 

		const bool noFolderForOptions = !m_showSubFolders && (numOptions < 2);

		if (m_showSubFolders)
		{
			bool bDone = sTagState.IsEmpty();
			while ( !bDone ) 
			{
				// Separate the tag strings by '+'
				int nPlusIdx = sTagState.Find('+');

				string sSingleTagString;
				// None found - just copy the string into the buffer
				if ( -1 == nPlusIdx )
				{
					sSingleTagString = sTagState;
					bDone = true;
				}
				// Found: split the string, copy the first name into the buffer
				else
				{
					sSingleTagString = sTagState.Left(nPlusIdx);
					sTagState = sTagState.Right(sTagState.GetLength() - nPlusIdx - 1);
				}

				const CTagDefinition &tagDefs = m_animDB->GetTagDefs();
				TagID id = tagDefs.Find(sSingleTagString);
				if ( TAG_ID_INVALID == id )
				{
					const CTagDefinition *fragTagDefs = m_context->m_controllerDef->GetFragmentTagDef(fragID);
					if ( fragTagDefs ) 
					{
						id = fragTagDefs->Find(sSingleTagString);
						if (id != TAG_ID_INVALID)
						{
							vTags.push_back(std::make_pair(fragTagDefs->GetPriority(id), sSingleTagString));
						}
					}
				}
				else
				{
						vTags.push_back(std::make_pair(tagDefs.GetPriority(id), sSingleTagString));
				}
			}
		}
		else if (!sTagState.IsEmpty() && !noFolderForOptions)
		{
			vTags.push_back(std::make_pair(0, string(sTagState)));
		}

		if ( !vTags.empty())
		{
			std::sort ( vTags.rbegin(), vTags.rend(), sCompareTagPrio());

			for (TTags::const_iterator it = vTags.begin(); it != vTags.end(); ++it )
			{
				// Search for the tag string under the current node. 
				// If found, use that node as a parent. If not, create a new one for that tag
				HTREEITEM newParent = FindInChildren ( it->second.c_str(), parent );
				if ( newParent ) 
				{
					parent = newParent;
				}
				else
				{
					const HTREEITEM oldParent = parent;
					parent = m_TreeCtrl.InsertItem( it->second.c_str(), IMAGE_FOLDER, IMAGE_FOLDER, parent );

					const STreeFragmentData* const pOldParentData = (STreeFragmentData*)m_TreeCtrl.GetItemData(oldParent);
					SFragTagState itemTagState = pOldParentData ? pOldParentData->tagState : SFragTagState();

					const string& tagName = it->second;
					const CTagDefinition& globalTagDef = m_animDB->GetTagDefs();
					const CTagDefinition* pFragTagDef = m_animDB->GetFragmentDefs().GetSubTagDefinition(fragID);
					const TagID globalTagId = globalTagDef.Find(tagName.c_str());
					const TagID fragmentTagID = pFragTagDef ? pFragTagDef->Find(tagName.c_str()) : TAG_ID_INVALID;
					if (fragmentTagID != TAG_ID_INVALID)
					{
						pFragTagDef->Set(itemTagState.fragmentTags, fragmentTagID, true);
					}
					else
					{
						globalTagDef.Set(itemTagState.globalTags, globalTagId, true);
					}

					STreeFragmentData *fragmentData = new STreeFragmentData();
					fragmentData->fragID				  = fragID;
					fragmentData->tagState			  = itemTagState;
					fragmentData->item					  = parent;
					fragmentData->tagSet					= true;
					m_fragmentData.push_back(fragmentData);
					m_TreeCtrl.SetItemData(parent, (DWORD_PTR)fragmentData);
				}
			}
		}
		else if (!noFolderForOptions)
		{
			parent = m_TreeCtrl.InsertItem( MannUtils::GetEmptyTagString(), IMAGE_FOLDER, IMAGE_FOLDER, parent );
			STreeFragmentData *fragmentData = new STreeFragmentData();
			fragmentData->fragID				  = fragID;
			fragmentData->tagState			  = tagState;
			fragmentData->item					  = parent;
			fragmentData->tagSet					= true;
			m_fragmentData.push_back(fragmentData);
			m_TreeCtrl.SetItemData(parent, (DWORD_PTR)fragmentData);
		}

		for (uint32 o=0; o<numOptions; o++)
		{
			const CFragment *fragment = m_animDB->GetEntry(fragID, t, o);

			char subFragNameBuffer[128];
			const char* nodeName;

			if (noFolderForOptions)
			{
				nodeName = szTagStateBuff;
			}
			else
			{
				sprintf(subFragNameBuffer, "Option %d", o+1);
				nodeName = subFragNameBuffer;
			}

			HTREEITEM entry = m_TreeCtrl.InsertItem( nodeName, IMAGE_ANIM, IMAGE_ANIM, parent, TVI_LAST );

			STreeFragmentData *fragmentData = new STreeFragmentData();
			fragmentData->fragID				  = fragID;
			fragmentData->tagState			  = tagState;
			fragmentData->option					= o;
			fragmentData->item					  = entry;
			fragmentData->tagSet					= false;
			m_fragmentData.push_back(fragmentData);

			m_TreeCtrl.SetItemData(entry, (DWORD_PTR)fragmentData);
		}
	}
}


uint32 CFragmentBrowser::AddNewFragment(FragmentID fragID, const SFragTagState& newTagState, const string &sAnimName)
{
	IMannequin &mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();

	ActionScopes scopeMask = m_context->m_controllerDef->GetScopeMask(fragID, newTagState);
	uint32 numScopes = m_context->m_controllerDef->m_scopeDef.size();
	uint32 newOption = OPTION_IDX_INVALID;

	// Add fragment on root scope (only)
	for (uint32 itScopes = 0; itScopes < numScopes; ++itScopes)
	{
		if (scopeMask & (1 << itScopes))
		{
			const SScopeDef &scopeDef = m_context->m_controllerDef->m_scopeDef[itScopes];
			SFragTagState tagState = newTagState;
			tagState.globalTags = m_context->m_controllerDef->m_tags.GetUnion(tagState.globalTags, scopeDef.additionalTags);

			IAnimationDatabase *pDatabase = FindHostDatabase(scopeDef.context, tagState);
			if ( !sAnimName.empty() )
			{
				CFragment newFrag;
				newFrag.m_animLayers.push_back(TAnimClipSequence());
				TAnimClipSequence &animSeq = newFrag.m_animLayers.back();

				animSeq.push_back(SAnimClip());
				SAnimClip &animClp = animSeq.back();
				animClp.animation.animRef.SetByString(sAnimName);

				newOption = mannequinSys.GetMannequinEditorManager()->AddFragmentEntry(pDatabase, fragID, tagState, newFrag);
			}
			else
			{
				newOption = mannequinSys.GetMannequinEditorManager()->AddFragmentEntry(pDatabase, fragID, tagState, CFragment());
			}

			break;
		}
	}

	CleanFragment(fragID);
	BuildFragment(fragID);

	SelectFragment(fragID, newTagState, newOption);
	return newOption;
}


void CFragmentBrowser::OnNewBtn()
{
	if ( HTREEITEM item = m_TreeCtrl.GetSelectedItem() )
	{
		STreeFragmentData *pFragData = (STreeFragmentData *)m_TreeCtrl.GetItemData(item);

		HTREEITEM parentItemID = item;

		FragmentID fragID;
		SFragTagState newTagState;
		if (pFragData)
		{
			fragID = pFragData->fragID;
			newTagState = pFragData->tagState;
		}
		else
		{
			CString szFragID = m_TreeCtrl.GetItemText( item );
			fragID = m_animDB->GetFragmentID(szFragID);

			if (fragID == FRAGMENT_ID_INVALID)
				return;
		}
		IMannequin &mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
		uint32 newOption = mannequinSys.GetMannequinEditorManager()->AddFragmentEntry(m_animDB, fragID, newTagState, CFragment());

		CleanFragment(fragID);
		BuildFragment(fragID);

		SelectFragment(fragID, newTagState, newOption);
	}
}

void CFragmentBrowser::OnNewDefinitionBtn()
{
	if (!m_animDB)
		return;

	IMannequin &mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
	IMannequinEditorManager* pManEditMan = mannequinSys.GetMannequinEditorManager();
	assert(pManEditMan);

	CString fragmentName("");
	EModifyFragmentIdResult result = eMFIR_Success;

	// Keep retrying until we get a valid name or the user gives up
	do 
	{
		CStringDlg fragmentNameDialog("New FragmentID Name", this);
		fragmentNameDialog.SetString(fragmentName);
		if (fragmentNameDialog.DoModal() != IDOK)
			return;

		fragmentName = fragmentNameDialog.GetString();

		result = pManEditMan->CreateFragmentID(m_animDB->GetFragmentDefs(), fragmentName);
		if (result != eMFIR_Success)
		{
			const CString commonErrorMessage = "Failed to create FragmentID with name '" + fragmentName + "'.\n  Reason:\n\n  ";
			switch (result)
			{
			case eMFIR_DuplicateName:
				AfxMessageBox(commonErrorMessage + "There is already a FragmentID with this name.");
				break;

			case eMFIR_InvalidNameIdentifier:
				AfxMessageBox(commonErrorMessage + "Invalid name identifier for a FragmentID.\n  A valid name can only use a-Z, A-Z, 0-9 and _ characters and cannot start with a digit.");
				break;

			case eMFIR_UnknownInputTagDefinition:
				AfxMessageBox(commonErrorMessage + "Unknown input tag definition. Your changes cannot be saved.");
				break;

			default:
				assert(false);
				return;
			}
		}
	} while (result != eMFIR_Success);

	const FragmentID fragID = m_animDB->GetFragmentID(fragmentName);

	// Edit the newly-created Fragment ID
	CMannTagEditorDialog tagEditorDialog(this, m_animDB, fragID);
	if (tagEditorDialog.DoModal() != IDOK)
	{
		pManEditMan->DeleteFragmentID(m_animDB->GetFragmentDefs(), fragID);
		return;
	}

	fragmentName = tagEditorDialog.FragmentName();

	HTREEITEM item = m_TreeCtrl.InsertItem(fragmentName, 1, 1, 0, TVI_SORT);
	m_fragmentItems.push_back(item);

	CleanFragment(fragID);
	BuildFragment(fragID);

	//--- Autoselect the new node
	m_TreeCtrl.Expand(item, TVE_EXPAND);
	m_TreeCtrl.SelectItem(item);
}

void CFragmentBrowser::OnDeleteDefinitionBtn()
{
	if (!m_animDB)
		return;

	const FragmentID selectedFragmentId = GetSelectedFragmentId();
	if (selectedFragmentId == FRAGMENT_ID_INVALID)
		return;

	IMannequin &mannequinSys = gEnv->pGame->GetIGameFramework()->GetMannequinInterface();
	IMannequinEditorManager* pManEditMan = mannequinSys.GetMannequinEditorManager();
	assert(pManEditMan);

	CString message;
	CString fragmentName = m_animDB->GetFragmentDefs().GetTagName(selectedFragmentId);
	message.Format("Delete the FragmentID '%s'?", fragmentName.GetBuffer());

	if (MessageBox(message.GetBuffer(), "CryMannequin", MB_YESNO | MB_ICONQUESTION) == IDYES)
	{
		m_fragEditor.FlushChanges();

		// Save the current previewer sequence to a temp file
		char szTempFileName[MAX_PATH] = "Mannequin_Temp.tmp";
		::GetTempFileName(".", "MNQ", 0, szTempFileName);
		CMannequinDialog::GetCurrentInstance()->Previewer()->SaveSequence(szTempFileName);

		// Delete the FragmentID
		EModifyFragmentIdResult result = pManEditMan->DeleteFragmentID(m_animDB->GetFragmentDefs(), selectedFragmentId);

		if (result != eMFIR_Success)
		{
			const CString commonErrorMessage = "Failed to delete FragmentID with name '" + fragmentName + "'.\n  Reason:\n\n  ";
			switch (result)
			{
			case eMFIR_UnknownInputTagDefinition:
				AfxMessageBox(commonErrorMessage + "Unknown input tag definition. Your changes cannot be saved.");
				break;

			case eMFIR_InvalidFragmentId:
				AfxMessageBox(commonErrorMessage + "Invalid FragmentID. Your changes cannot be saved.");
				break;

			default:
				assert(false);
				break;
			}

			// Clean up the temp file
			::DeleteFile(szTempFileName);
			return;
		}

		CleanFragmentID(selectedFragmentId);

		// Reload the previewer sequence from the temp file and clean up
		CMannequinDialog::GetCurrentInstance()->Previewer()->LoadSequence(szTempFileName);
		::DeleteFile(szTempFileName);

		CMannequinDialog::GetCurrentInstance()->Previewer()->KeyProperties()->ForceUpdate();

		// Only force a selection change if we've deleted the FragmentID we were editing
		if (m_fragmentID == FRAGMENT_ID_INVALID)
			EditSelectedItem();
	}
}

void CFragmentBrowser::OnRenameDefinitionBtn()
{
	if (!m_animDB)
		return;

	const FragmentID selectedFragmentId = GetSelectedFragmentId();
	if (selectedFragmentId == FRAGMENT_ID_INVALID)
		return;

	CMannTagEditorDialog tagEditorDialog(this, m_animDB, selectedFragmentId);
	if (tagEditorDialog.DoModal() != IDOK)
		return;

	const CString& fragmentName = tagEditorDialog.FragmentName();

	const HTREEITEM item = m_TreeCtrl.GetSelectedItem();
	m_TreeCtrl.SetItemText(item, fragmentName);

	CleanFragment(selectedFragmentId);
	BuildFragment(selectedFragmentId);

	OnSelectionChanged();
}

void CFragmentBrowser::SetOnSelectedItemEditCallback( OnSelectedItemEditCallback onSelectedItemEditCallback )
{
	m_onSelectedItemEditCallback = onSelectedItemEditCallback;
}

void CFragmentBrowser::SetOnScopeContextChangedCallback( OnScopeContextChangedCallback onScopeContextChangedCallback )
{
	m_onScopeContextChangedCallback = onScopeContextChangedCallback;
}

void CFragmentBrowser::OnOK()
{
	EditSelectedItem();
	SetFocus();
}

void CFragmentBrowser::SelectFirstKey()
{
	if (!m_fragEditor.SelectFirstKey(SEQUENCER_PARAM_ANIMLAYER))
	{
		if (!m_fragEditor.SelectFirstKey(SEQUENCER_PARAM_PROCLAYER))
		{
			m_fragEditor.SelectFirstKey();
		}
	}
}

// 
FragmentID CFragmentBrowser::GetValidFragIDForAnim(const CPoint &point, COleDataObject* pDataObject)
{
	UINT clipFormat = EditorDragDropHelpers::GetAnimationNameClipboardFormat();
	HGLOBAL hData = pDataObject->GetGlobalData(clipFormat);

	if ( nullptr == hData )
	{
		return FRAGMENT_ID_INVALID;
	}

	POINT dragPt = point;
	ClientToScreen(&dragPt);
	m_TreeCtrl.ScreenToClient(&dragPt);

	HTREEITEM hitTarget = m_TreeCtrl.HitTest(dragPt);
	if ( nullptr == hitTarget )
	{
		return FRAGMENT_ID_INVALID;
	}

	string animName = static_cast<char*>(GlobalLock(hData));
	GlobalUnlock(hData);

	if ( animName.empty() )
	{
		return FRAGMENT_ID_INVALID;
	}

	std::vector<SScopeContextData>::const_iterator itCtx = m_context->m_contextData.begin();
	for (; itCtx != m_context->m_contextData.end(); ++itCtx )
	{
		const SScopeContextData &ctxData = *itCtx;
		int nAnimID = ctxData.animSet->GetAnimIDByName(animName);
		if ( -1 != nAnimID )
		{
			break;
		}
	}

	if ( m_context->m_contextData.end() == itCtx )
	{
		return FRAGMENT_ID_INVALID;
	}

	// Find the root item (Fragment ID)
	HTREEITEM rootItem = hitTarget;

	while ( nullptr != m_TreeCtrl.GetParentItem(rootItem) )
	{
		rootItem = m_TreeCtrl.GetParentItem(rootItem);
	}
	CString IDName = m_TreeCtrl.GetItemText(rootItem);
	FragmentID fragID = m_animDB->GetFragmentID(IDName.GetString());

	// HILITE MENU ITEM HERE 
	m_TreeCtrl.SelectDropTarget(rootItem);
	return fragID;
}

// 
bool CFragmentBrowser::AddAnimToFragment(FragmentID fragID, COleDataObject* pDataObject)
{
	UINT clipFormat = EditorDragDropHelpers::GetAnimationNameClipboardFormat(); 
	HGLOBAL hData = pDataObject->GetGlobalData(clipFormat);

	if (hData == NULL)
	{
		return false;
	}

	string animName = static_cast<char*>(GlobalLock(hData));
	GlobalUnlock(hData);

	SFragTagState newTagState;
	MannUtils::AppendTagsFromAnimName(animName, newTagState);
	AddNewFragment(fragID, newTagState, animName);
	
	return true;
}

