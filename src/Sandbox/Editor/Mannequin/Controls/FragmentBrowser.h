#ifndef __FRAGMENT_BROWSER__H__
#define __FRAGMENT_BROWSER__H__

#include <ICryMannequin.h>
#include "afxwin.h"
#include "Controls/ImageButton.h"

class CMannFragmentEditor;
struct SMannequinContexts;

class CFragmentBrowser
: public CXTResizeFormView
{
	typedef CXTResizeFormView PARENT;

	DECLARE_DYNAMIC(CFragmentBrowser)

public:
	CFragmentBrowser(CMannFragmentEditor &fragEditor, CWnd *pParent, UINT nId);
	virtual ~CFragmentBrowser();

	enum { IDD = IDD_FRAGMENT_BROWSER };

	void Update(void);

	void SetContext(SMannequinContexts &context);

	void SetScopeContext(int scopeContextID);
	bool SelectFragment(FragmentID fragmentID, const SFragTagState &tagState, uint32 option = 0);
	bool SetTagState(const SFragTagState &newTags);

	typedef Functor2< FragmentID, CString& > OnSelectedItemEditCallback;
	void SetOnSelectedItemEditCallback( OnSelectedItemEditCallback onSelectedItemEditCallback );

	typedef Functor0 OnScopeContextChangedCallback;
	void SetOnScopeContextChangedCallback( OnScopeContextChangedCallback onScopeContextChangedCallback );

	FragmentID GetSelectedFragmentId() const;
	CString GetSelectedNodeText() const;

	void SetADBFileNameTextToCurrent();

	int GetScopeContextId() const
	{
		return m_cbContext.GetCurSel();
	}

	void RebuildAll();

protected:
	virtual BOOL OnInitDialog();
	virtual void OnDestroy();
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual void OnOK();
	virtual void OnCancel() {}
	
	void EditSelectedItem();
	void SetEditItem(HTREEITEM editedItem, FragmentID fragmentID, const SFragTagState &tagState, uint32 option);
	void SetDatabase(IAnimationDatabase *animDB);
	
	HTREEITEM FindFragmentItem(FragmentID fragmentID, const SFragTagState &tagState, uint32 option) const;

	void CleanFragmentID(FragmentID fragID);
	void CleanFragment(FragmentID fragID);
	void BuildFragment(FragmentID fragID);
	HTREEITEM FindInChildren ( const char* szTagStr, HTREEITEM parent );


	// Drag / drop helpers
	friend class CFragmentBrowserBaseDropTarget;
	FragmentID GetValidFragIDForAnim(const CPoint &point, COleDataObject* pDataObject);
	bool AddAnimToFragment(FragmentID fragID, COleDataObject* pDataObject);

	IAnimationDatabase *FindHostDatabase(uint32 contextID, const SFragTagState &tagState) const;

	CString GetItemText(HTREEITEM item) const;

	struct STreeFragmentData
	{
		FragmentID fragID;
		SFragTagState tagState;
		uint32 option;
		HTREEITEM item;
		bool tagSet;
	};

	COleDataSource* CreateFragmentDescriptionDataSource(STreeFragmentData *fragmentData ) const;

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBeginDrag(NMHDR* pNMHdr, LRESULT* pResult);
	afx_msg void OnBeginRDrag(NMHDR* pNMHdr, LRESULT* pResult);

	afx_msg void OnTVClick(NMHDR*, LRESULT*);
	afx_msg void OnTVRightClick(NMHDR*, LRESULT*);
	afx_msg void OnTVEditSel(NMHDR*, LRESULT*);
	afx_msg void OnTreeKeyDown(NMHDR*, LRESULT*);
	afx_msg void OnTVSelChanged(NMHDR*, LRESULT*);
	afx_msg void OnChangeFilterTags();
	afx_msg void OnChangeFilterFragments();
	afx_msg void OnChangeFilterAnimClip();
	afx_msg void OnToggleSubfolders();
	afx_msg void OnToggleShowEmpty();
	afx_msg void OnNewBtn();
	afx_msg void OnEditBtn();
	afx_msg void OnDeleteBtn();
	afx_msg void OnTagDefEditorBtn();
	afx_msg void OnShowWindow(BOOL, UINT);
	afx_msg void OnChangeContext();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnNewDefinitionBtn();
	afx_msg void OnDeleteDefinitionBtn();
	afx_msg void OnRenameDefinitionBtn();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);

	void SelectFirstKey();

	uint32 AddNewFragment(FragmentID fragID, const SFragTagState& newTagState, const string &sAnimName);
private:

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void UpdateControlEnabledStatus();
	void OnSelectionChanged();

private:
	UINT m_nFragmentClipboardFormat;

	CTreeCtrl m_TreeCtrl;
	CComboBox m_cbContext;
	CEdit m_editFilterTags;
	CEdit m_editFilterFragmentIDs;
	CEdit m_editAnimClipFilter;
	CButton m_chkShowSubFolders;

	CButton m_chkShowEmptyFolders;
	CImageButton m_newEntry;
	CImageButton m_deleteEntry;
	CImageButton m_editEntry;
	CImageButton m_newID;
	CImageButton m_deleteID;
	CImageButton m_renameID;
	CImageButton m_tagDefEditor;
	CImageList m_buttonImages;
	CToolTipCtrl m_toolTip;

	bool m_showSubFolders;
	bool m_showEmptyFolders;

	CString m_filterText;
	std::vector<CString> m_filters;

	CString m_filterFragmentIDText;

	CString m_filterAnimClipText;

	IAnimationDatabase *m_animDB;
	CMannFragmentEditor &m_fragEditor;

	HTREEITEM m_editedItem;

	bool m_rightDrag;
	
	CImageList *m_draggingImage;
	HTREEITEM m_dragItem;

	struct SCopyItem
	{
		SCopyItem(const CFragment& _fragment, const SFragTagState _tagState) : fragment(_fragment), tagState(_tagState){}
		CFragment fragment;
		SFragTagState tagState;
	};
	typedef std::vector<SCopyItem> TCopiedItems;
	TCopiedItems m_copiedItems;
	FragmentID m_copiedFragmentID;

	CImageList	m_imageList;
	CEdit				m_CurrFile;

	SMannequinContexts *m_context;
	int m_scopeContext;

	FragmentID m_fragmentID;
	SFragTagState m_tagState;
	uint32 m_option;

	typedef std::vector<STreeFragmentData*> TFragmentData;
	TFragmentData m_fragmentData;

	std::vector<HTREEITEM> m_fragmentItems;

	float m_filterDelay;

	OnSelectedItemEditCallback m_onSelectedItemEditCallback;

	// Drag / Drop
	COleDropTarget* m_pDropTarget;
	OnScopeContextChangedCallback m_onScopeContextChangedCallback;
};

#endif
