/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2014.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Schematyc tree control.
-------------------------------------------------------------------------
History:
- 01:01:2013: Created by Paul Slinger
*************************************************************************/

#ifndef __SCHEMATYC_TREECTRL_H__
#define __SCHEMATYC_TREECTRL_H__

#include "BoostHelpers.h"

namespace Schematyc
{
	typedef TemplateUtils::CArrayProxy<const size_t> TSizeTConstArray;

	class CCustomTreeCtrlItem;

	DECLARE_BOOST_POINTERS(CCustomTreeCtrlItem)

	class CCustomTreeCtrlItem
	{
	public:

		CCustomTreeCtrlItem(const char* text, size_t icon, const CCustomTreeCtrlItemPtr& pParent);
		virtual ~CCustomTreeCtrlItem();

		const char* GetText() const;
		size_t GetIcon() const;
		CCustomTreeCtrlItemPtr GetParent() const;
		bool HasAncestor(CCustomTreeCtrlItemPtr pItem) const;
		CCustomTreeCtrlItemPtr FindAncestor(const char* text) const;
		CCustomTreeCtrlItemPtr FindAncestor(size_t icon) const;
		CCustomTreeCtrlItemPtr FindAncestor(const TSizeTConstArray& icons) const;
		void SetHTreeItem(HTREEITEM hTreeItem);
		HTREEITEM GetHTreeItem() const;

	private:

		string									m_text;
		size_t									m_icon;
		CCustomTreeCtrlItemPtr	m_pParent;		// TODO : Shouldn't this be a weak pointer?
		HTREEITEM								m_hTreeItem;
	};

	typedef std::vector<CCustomTreeCtrlItemPtr> TCustomTreeCtrlItemPtrVector;

	class CCustomTreeCtrl : public CXTTreeCtrl
	{
	public:

		virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
		void ExpandItem(const CCustomTreeCtrlItemPtr& pItem);
		void EnsureVisible(const CCustomTreeCtrlItemPtr& pItem);

	protected:

		CCustomTreeCtrl();
		virtual ~CCustomTreeCtrl();
		
		void AddItem(const CCustomTreeCtrlItemPtr& pItem);
		void RemoveItem(const CCustomTreeCtrlItemPtr& pItem);
		void RemoveItemChildren(const CCustomTreeCtrlItemPtr& pItem);
		CCustomTreeCtrlItemPtr FindItem(const char* text, const CCustomTreeCtrlItemPtr& pParentItem = CCustomTreeCtrlItemPtr());
		CCustomTreeCtrlItemPtr FindItem(CPoint pos);
		CCustomTreeCtrlItemPtr FindItem(HTREEITEM hTreeItem);
		TCustomTreeCtrlItemPtrVector& GetItems();
		const TCustomTreeCtrlItemPtrVector& GetItems() const;
		void Reset();

		virtual void LoadImageList(CImageList& imageList);
		virtual DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
		virtual BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);

	private:

		class CDropTarget : public COleDropTarget
		{
		public:

			CDropTarget(CCustomTreeCtrl& customTreeCtrl);

			// COleDropTarget
			virtual DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
			virtual BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
			// ~COleDropTarget

		private:

			CCustomTreeCtrl&	m_customTreeCtrl;
		};

		CImageList										m_imageList;
		TCustomTreeCtrlItemPtrVector	m_items;
		CDropTarget										m_dropTarget;
	};
}

#endif //__SCHEMATYC_TREECTRL_H__
