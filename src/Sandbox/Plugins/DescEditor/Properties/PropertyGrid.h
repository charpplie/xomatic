#ifndef __PROPERTYGRID_H__
#define __PROPERTYGRID_H__

#if _MSC_VER > 1000
#pragma once
#endif


namespace CryGame
{
	class CPropertyGrid;
	class CPropertyGridItem;

	//////////////////////////////////////////////////////////////////////////
	// Property Grid Events
	//////////////////////////////////////////////////////////////////////////

	enum EPropertyGridEvents
	{
		ePGE_OnChanged,
		ePGE_OnDirty,
		ePGE_OnClean,
		ePGE_OnCleanAll,
		ePGE_OnElementAdded,
		ePGE_OnElementRemoved,
	};

	//////////////////////////////////////////////////////////////////////////
	// Property Grid Events
	//////////////////////////////////////////////////////////////////////////

	enum EPropertyGridVerbs
	{
		ePGV_EditAnnotation,
		ePGV_CreateGroup,
	};

	//////////////////////////////////////////////////////////////////////////
	// Property Grid Data
	//////////////////////////////////////////////////////////////////////////

	struct CPropertyGridData
	{
		CPropertyGridData(CReflectedObject* pObject = NULL, IProperty* pProperty = NULL, bool isElement = false)
			: m_pObject(pObject)
			, m_pProperty(pProperty)
			, m_isElement(isElement)
		{}

		CReflectedObject* m_pObject;
		IProperty* m_pProperty;
		bool m_isElement;
		Value m_origValue;
	};

	//////////////////////////////////////////////////////////////////////////
	// Property Grid
	//////////////////////////////////////////////////////////////////////////

	struct SPropertyGridItemState
	{
	public:
		SPropertyGridItemState()
			: ID(0)
			, DirtyRefCount(0)
			, Expanded(false)
			, Selected(false)
		{}

		void ResetDirtyStatus(bool resetOriginalValue = false)
		{
			DirtyRefCount = 0;

			if (resetOriginalValue)
			{
				// In this case, we want to clear the original
				// value so it isn't applied when the state is restored.
				OriginalValue = Value();
			}

			for (int i = 0; i < ChildStates.size(); ++i)
			{
				ChildStates[i].ResetDirtyStatus(resetOriginalValue);
			}
		}

	public:
		int ID;
		int DirtyRefCount;
		Value OriginalValue;

		bool Expanded;
		bool Selected;

		string Name;

		std::vector<SPropertyGridItemState> ChildStates;
	};

	struct SPropertyGridState
	{
	public:
		SPropertyGridState()
			: TopIndex(-1)
			, Count(0)
		{}

		void ResetDirtyStatus(bool resetOriginalValue = false)
		{
			RootState.ResetDirtyStatus(resetOriginalValue);
		}

	public:
		int TopIndex;
		int Count;
		SPropertyGridItemState RootState;
	};

	enum EPropertyGridFlags
	{
		ePGF_ShowObjects    = 1 << 0,
		ePFG_ShowArrays     = 1 << 1,
		ePFG_ShowAll        = 0xFFFF
	};

	typedef std::vector<CPropertyGridItem*> PropertyGridItemList;

	DECLARE_DISPATCHER(Property, CPropertyGrid, const CPropertyGridData);
	class CPropertyGrid : public CXTPPropertyGrid
						, public IPropertyDispatcher
	{
	public:
		CPropertyGrid();

		virtual BOOL Create(const RECT& rect, CWnd* pParentWnd, UINT nID, DWORD dwListStyle = 0) override;
		virtual CXTPPropertyGridView* CreateView() const override;
		virtual void OnNavigate(XTPPropertyGridUI nUIElement, BOOL bForward, CXTPPropertyGridItem* pItem) override;

		CPropertyGridItem* Populate(CReflectedObject* pObject, IProperty* pProperty = NULL);
		void Repopulate();
		void PopulateGroups();

		void ProcessProperty(CPropertyGridItem* pParentProperty, CReflectedObject* pObject);
		CPropertyGridItem* ProcessProperty(CPropertyGridItem* pParentProperty, CReflectedObject* o, IProperty* p);
		CPropertyGridItem* ProcessProperty(CPropertyGridItem* pParentProperty, CReflectedObject* o, IProperty* p, bool isElement, Value& value, const char* name);

		CPropertyGridItem* CreateProperty(CRuntimeClass* pClass, CPropertyGridItem* pParentProperty, CReflectedObject* o, IProperty* p, bool isElement, const char* name, const char* variation = NULL);
		void SavePropertyProfiles(CXTPPropertyGridItem* pRoot);
		void SaveChildPropertyProfiles(CXTPPropertyGridItems* pChildren);

		CPropertyGridItem* AddGroup(CPropertyGridItem* pParentProperty, const char* name);
		void MoveSelectedToGroup(PropertyGridItemList& itemList, CPropertyGridItem* pGroup);

		void SetFlag(const EPropertyGridFlags& nFlag) { m_nFlags |= nFlag; }
		void ClearFlag(const EPropertyGridFlags& nFlag) { m_nFlags &= ~nFlag; }
		uint16 GetFlags() const { return m_nFlags; }

		int GetSelectedProperties(PropertyGridItemList& out);

		void UpdateDescriptionText(CPropertyGridItem* pProperty);
		void UpdateItemIDs(CXTPPropertyGridItem* pProperty = NULL, int nID = 0);

		void SaveState(SPropertyGridState& state);
		void RestoreState(const SPropertyGridState& state);

		COLORREF GetBorderBgColor() const { return m_borderBgColor; }
		void SetBorderBgColor(COLORREF value) { m_borderBgColor = value; }

		void ExpandOrCollapseAll(CXTPPropertyGridItem* pItem, bool expand);
		void Reset(CXTPPropertyGridItem* pItem = NULL);

		CXTPPropertyGridItem* FindGroupItem(const CXTPPropertyGridItems* pItemList, const char* pCaption) const;

	private:
		virtual void OnSelectionChanged(CXTPPropertyGridItem* pItem) override;

	private:
		uint16 m_nFlags;
		COLORREF m_borderBgColor;
		std::vector<CPropertyGridItem*> m_groups;
		CImageList m_icons;
	};

	class CPropertyGridView : public CXTPPropertyGridView
	{
	public:
		CPropertyGridView();

	protected:
		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnTimer(UINT_PTR nIDEvent);
		DECLARE_MESSAGE_MAP()

		void HandleScrolling(CPoint Point, CRect ClientRect);
		void UpdateLines(int nIndex);
		void DrawLine(int nIndex, COLORREF color);

	private:
		PropertyGridItemList m_draggingItems;

		bool m_dragging;
		int m_nDragToIndex;
		int m_nDrawLineIndex;
		DWORD m_nScrollInterval;
	};
}

#endif
