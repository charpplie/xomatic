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

#include "StdAfx.h"

#include "Schematyc_TreeCtrl.h"

#pragma warning(disable: 4355)

namespace Schematyc
{
	namespace
	{
		int CALLBACK SortCustomTreeCtrlItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
		{
			if(CCustomTreeCtrl* pTreeCtrl = (CCustomTreeCtrl*)lParamSort)
			{
				int	lhsImage = 0;
				int	lhsSelectedImage = 0;
				pTreeCtrl->GetItemImage((HTREEITEM)lParam1, lhsImage, lhsSelectedImage);

				int	rhsImage = 0;
				int	rhsSelectedImage = 0;
				pTreeCtrl->GetItemImage((HTREEITEM)lParam2, rhsImage, rhsSelectedImage);

				CString	lhsText = pTreeCtrl->GetItemText((HTREEITEM)lParam1);
				CString	rhsText = pTreeCtrl->GetItemText((HTREEITEM)lParam2);

				if(lhsImage == rhsImage)
				{
					return lhsText.Compare(rhsText);
				}
				else
				{
					return lhsImage - rhsImage;
				}
			}
			return 0;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	CCustomTreeCtrlItem::CCustomTreeCtrlItem(const char* text, size_t icon, const CCustomTreeCtrlItemPtr& pParent)
		: m_text(text)
		, m_icon(icon)
		, m_pParent(pParent)
		, m_hTreeItem(NULL)
	{}

	//////////////////////////////////////////////////////////////////////////
	CCustomTreeCtrlItem::~CCustomTreeCtrlItem() {}

	//////////////////////////////////////////////////////////////////////////
	const char* CCustomTreeCtrlItem::GetText() const
	{
		return m_text.c_str();
	}

	//////////////////////////////////////////////////////////////////////////
	size_t CCustomTreeCtrlItem::GetIcon() const
	{
		return m_icon;
	}

	//////////////////////////////////////////////////////////////////////////
	CCustomTreeCtrlItemPtr CCustomTreeCtrlItem::GetParent() const
	{
		return m_pParent;
	}

	//////////////////////////////////////////////////////////////////////////
	bool CCustomTreeCtrlItem::HasAncestor(CCustomTreeCtrlItemPtr pItem) const
	{
		for(CCustomTreeCtrlItemPtr pAncestor = GetParent(); pAncestor; pAncestor = pAncestor->GetParent())
		{
			if(pAncestor == pItem)
			{
				return true;
			}
		}
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	CCustomTreeCtrlItemPtr CCustomTreeCtrlItem::FindAncestor(const char* text) const
	{
		CRY_ASSERT(text);
		if(text)
		{
			for(CCustomTreeCtrlItemPtr pAncestor = m_pParent; pAncestor; pAncestor = pAncestor->GetParent())
			{
				if(pAncestor->GetText() == text)
				{
					return pAncestor;
				}
			}
		}
		return CCustomTreeCtrlItemPtr();
	}

	//////////////////////////////////////////////////////////////////////////
	CCustomTreeCtrlItemPtr CCustomTreeCtrlItem::FindAncestor(size_t icon) const
	{
		for(CCustomTreeCtrlItemPtr pAncestor = m_pParent; pAncestor; pAncestor = pAncestor->GetParent())
		{
			if(pAncestor->GetIcon() == icon)
			{
				return pAncestor;
			}
		}
		return CCustomTreeCtrlItemPtr();
	}

	//////////////////////////////////////////////////////////////////////////
	CCustomTreeCtrlItemPtr CCustomTreeCtrlItem::FindAncestor(const TSizeTConstArray& icons) const
	{
		const size_t	*pBeginIcon = icons.begin();
		const size_t	*pEndIcon = icons.end();
		for(CCustomTreeCtrlItemPtr pAncestor = m_pParent; pAncestor; pAncestor = pAncestor->GetParent())
		{
			const size_t	ancestorIcon = pAncestor->GetIcon();
			for(const size_t* pIcon = pBeginIcon; pIcon != pEndIcon; ++ pIcon)
			{
				if(*pIcon == ancestorIcon)
				{
					return pAncestor;
				}
			}
		}
		return CCustomTreeCtrlItemPtr();
	}

	//////////////////////////////////////////////////////////////////////////
	void CCustomTreeCtrlItem::SetHTreeItem(HTREEITEM hTreeItem)
	{
		m_hTreeItem = hTreeItem;
	}

	//////////////////////////////////////////////////////////////////////////
	HTREEITEM CCustomTreeCtrlItem::GetHTreeItem() const
	{
		return m_hTreeItem;
	}

	//////////////////////////////////////////////////////////////////////////
	BOOL CCustomTreeCtrl::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID)
	{
		if(__super::Create(dwStyle | WS_BORDER | TVS_HASBUTTONS | TVS_NOTOOLTIPS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS, rect, pParentWnd, nID))
		{
			// Register drop target.
			m_dropTarget.Register(this);
			// Load image list.
			LoadImageList(m_imageList);
			return true;
		}
		else
		{
			return false;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CCustomTreeCtrl::ExpandItem(const CCustomTreeCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem);
		if(pItem)
		{
			CXTTreeCtrl::Expand(pItem->GetHTreeItem(), TVE_EXPAND);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CCustomTreeCtrl::EnsureVisible(const CCustomTreeCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem);
		if(pItem)
		{
			CXTTreeCtrl::EnsureVisible(pItem->GetHTreeItem());
		}
	}

	//////////////////////////////////////////////////////////////////////////
	CCustomTreeCtrl::CCustomTreeCtrl()
		: m_dropTarget(*this)
	{}

	//////////////////////////////////////////////////////////////////////////
	CCustomTreeCtrl::~CCustomTreeCtrl() {}

	//////////////////////////////////////////////////////////////////////////
	void CCustomTreeCtrl::AddItem(const CCustomTreeCtrlItemPtr& pItem)
	{
		CRY_ASSERT(pItem);
		if(pItem)
		{
			CCustomTreeCtrlItemPtr	pParentItem = pItem->GetParent();
			HTREEITEM								hParentTreeItem = pParentItem ? pParentItem->GetHTreeItem() : NULL;
			HTREEITEM								hTreeItem = CXTTreeCtrl::InsertItem(pItem->GetText(), pItem->GetIcon(), pItem->GetIcon(), hParentTreeItem, TVI_LAST);
			CXTTreeCtrl::SetItemData(hTreeItem, (DWORD_PTR)hTreeItem);
			pItem->SetHTreeItem(hTreeItem);
			m_items.push_back(pItem);
			if(hParentTreeItem)
			{
				TVSORTCB sortCallback;
				sortCallback.hParent			= hParentTreeItem;
				sortCallback.lpfnCompare	= SortCustomTreeCtrlItems;
				sortCallback.lParam				= (LPARAM)this;
				CXTTreeCtrl::SortChildrenCB(&sortCallback);
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CCustomTreeCtrl::RemoveItem(const CCustomTreeCtrlItemPtr& pItem)
	{
		for(TCustomTreeCtrlItemPtrVector::iterator iItem = m_items.begin(), iEndItem = m_items.end(); iItem != iEndItem; ++ iItem)
		{
			const CCustomTreeCtrlItemPtr& _pItem = *iItem;
			if(_pItem == pItem)
			{
				CXTTreeCtrl::DeleteItem(_pItem->GetHTreeItem());
				if(iItem != (iEndItem - 1))
				{
					*iItem = m_items.back();
				}
				m_items.pop_back();
				break;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	void CCustomTreeCtrl::RemoveItemChildren(const CCustomTreeCtrlItemPtr& pItem)
	{
		TCustomTreeCtrlItemPtrVector::iterator	iEndItem = m_items.end();
		TCustomTreeCtrlItemPtrVector::iterator	iLastItem = iEndItem - 1;
		for(TCustomTreeCtrlItemPtrVector::iterator iItem = m_items.begin(); iItem <= iLastItem; )
		{
			const CCustomTreeCtrlItemPtr& _pItem = *iItem;
			if(_pItem->HasAncestor(pItem))
			{
				CXTTreeCtrl::DeleteItem(_pItem->GetHTreeItem());
				if(iItem != iLastItem)
				{
					*iItem = *iLastItem;
				}
				-- iLastItem;
			}
			else
			{
				++ iItem;
			}
		}
		m_items.erase(iLastItem + 1, iEndItem);
	}

	//////////////////////////////////////////////////////////////////////////
	CCustomTreeCtrlItemPtr CCustomTreeCtrl::FindItem(const char* text, const CCustomTreeCtrlItemPtr& pParentItem)
	{
		for(TCustomTreeCtrlItemPtrVector::iterator iItem = m_items.begin(), iEndItem = m_items.end(); iItem != iEndItem; ++ iItem)
		{
			const CCustomTreeCtrlItemPtr&	pItem = *iItem;
			if(!pParentItem || (pItem->GetParent() == pParentItem))
			{
				if(strcmp(pItem->GetText(), text) == 0)
				{
					return pItem;
				}
			}
		}
		return CCustomTreeCtrlItemPtr();
	}

	//////////////////////////////////////////////////////////////////////////
	CCustomTreeCtrlItemPtr CCustomTreeCtrl::FindItem(CPoint pos)
	{
		UINT			flags = 0;
		HTREEITEM	hTreeItem = CXTTreeCtrl::HitTest(pos, &flags);
		if(hTreeItem && (flags & TVHT_ONITEM))
		{
			return FindItem(hTreeItem);
		}
		return CCustomTreeCtrlItemPtr(); 
	}

	///////////////////////////////////////////////////////////////////////
	CCustomTreeCtrlItemPtr CCustomTreeCtrl::FindItem(HTREEITEM hTreeItem)
	{
		for(TCustomTreeCtrlItemPtrVector::iterator iItem = m_items.begin(), iEndItem = m_items.end(); iItem != iEndItem; ++ iItem)
		{
			const CCustomTreeCtrlItemPtr&	pItem = *iItem;
			if(pItem->GetHTreeItem() == hTreeItem)
			{
				return pItem;
			}
		}
		return CCustomTreeCtrlItemPtr();
	}

	//////////////////////////////////////////////////////////////////////////
	TCustomTreeCtrlItemPtrVector& CCustomTreeCtrl::GetItems()
	{
		return m_items;
	}

	//////////////////////////////////////////////////////////////////////////
	const TCustomTreeCtrlItemPtrVector& CCustomTreeCtrl::GetItems() const
	{
		return m_items;
	}

	//////////////////////////////////////////////////////////////////////////
	void CCustomTreeCtrl::Reset()
	{
		m_items.clear();
		CXTTreeCtrl::DeleteAllItems();
	}

	//////////////////////////////////////////////////////////////////////////
	void CCustomTreeCtrl::LoadImageList(CImageList& imageList) {}

	//////////////////////////////////////////////////////////////////////////
	DROPEFFECT CCustomTreeCtrl::OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
	{
		return DROPEFFECT_NONE;
	}

	//////////////////////////////////////////////////////////////////////////
	BOOL CCustomTreeCtrl::OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
	{
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	CCustomTreeCtrl::CDropTarget::CDropTarget(CCustomTreeCtrl& customTreeCtrl)
		: m_customTreeCtrl(customTreeCtrl)
	{}

	//////////////////////////////////////////////////////////////////////////
	DROPEFFECT CCustomTreeCtrl::CDropTarget::OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
	{
		return m_customTreeCtrl.OnDragOver(pWnd, pDataObject, dwKeyState, point);
	}

	//////////////////////////////////////////////////////////////////////////
	BOOL CCustomTreeCtrl::CDropTarget::OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
	{
		return m_customTreeCtrl.OnDrop(pWnd, pDataObject, dropEffect, point);
	}
}