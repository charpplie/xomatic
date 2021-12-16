////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:   SelectionTreeTimestampsView.h
//  Version:     v1.00
//  Created:     02/09/2013 by Michiel Meesters
//  Description: 
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SELECTION_TREE_TIMESTAMPS_VIEW__H__
#define __SELECTION_TREE_TIMESTAMPS_VIEW__H__


#include "SelectionTreeBaseDockView.h"
#include "Util/IXmlHistoryManager.h"

class CSelectionTreeTimestampsView
	: public CSelectionTreeBaseDockView
	, public IXmlHistoryView
	, public IXmlUndoEventHandler
{
	DECLARE_DYNAMIC( CSelectionTreeTimestampsView )

public:
	CSelectionTreeTimestampsView();
	virtual ~CSelectionTreeTimestampsView();

	// IXmlHistoryView
	virtual bool LoadXml( int typeId, const XmlNodeRef& xmlNode, IXmlUndoEventHandler*& pUndoEventHandler, uint32 userindex );
	virtual void UnloadXml( int typeId );

	// IXmlUndoEventHandler
	virtual bool SaveToXml( XmlNodeRef& xmlNode );
	virtual bool LoadFromXml( const XmlNodeRef& xmlNode );
	virtual bool ReloadFromXml( const XmlNodeRef& xmlNode );

protected:
	virtual BOOL OnInitDialog();

	afx_msg void OnTvnDblClick( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnTvnRightClick( NMHDR* pNMHDR, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()

private:
	bool LoadTimestampNodes( const XmlNodeRef& xmlNode, HTREEITEM hItem );
	
	bool CreateSignalItem( const char* stampName, const char* eventName, const char* exclusiveTo, HTREEITEM hItem, HTREEITEM hInsertAfter );
	void DeleteSignalItem( HTREEITEM hItem );
	void DeleteSignalItems( HTREEITEM hItem );

	void AddSignal( HTREEITEM hItem );
	void RemoveSignalVars( HTREEITEM hItem );

	void AddSigVar( HTREEITEM hItem );
	void DeleteSigVar( HTREEITEM hItem );

	void ClearList();

	HTREEITEM GetParentItem(HTREEITEM hItem);

private:
	CTreeCtrl m_signals;
	bool m_bLoaded;

	enum EItemType
	{
		eIT_UNDEFINED = -1,
		eIT_Signal = 0,
		eIT_TimeStamp,
	};

	struct SVarInfo
	{
		SVarInfo( const string& signalname, const string& setOnEventName, const string& exclusiveTo ) : stampName( signalname ), eventName( setOnEventName ), exclusiveToStamp( exclusiveTo ) {}
		static EItemType GetType() { return eIT_TimeStamp; }

		string stampName;
		string eventName;
		string exclusiveToStamp;
	};
	typedef std::map< HTREEITEM, SVarInfo > TVarInfoMap;

	struct SSignalInfo
	{
		SSignalInfo( const string& signalName ) : Name( signalName ) {}
		static EItemType GetType() { return eIT_Signal; }
		string Name;
	};
	typedef std::map< HTREEITEM, SSignalInfo > TSignalInfoMap;


	struct SChildItem
	{
		SChildItem(HTREEITEM item, EItemType type) : Item(item), Type(type) {}
		bool operator==(const HTREEITEM& item) const { return Item == item; }
		HTREEITEM Item;
		EItemType Type;
	};

	typedef std::list< SChildItem > TChildItemList;
	typedef std::map< HTREEITEM, TChildItemList > TChildItemMap;

	struct STreeInfo
	{
		void Clear()
		{
			ChildItems.clear();
			SignalInfo.clear();
			VarInfo.clear();
			ChildItems[TVI_ROOT] = TChildItemList();
		}
		TChildItemMap ChildItems;
		TSignalInfoMap SignalInfo;
		TVarInfoMap VarInfo;
	};

	STreeInfo m_TreeInfo;

	template <class T>
	const T& GetItemInfo(const std::map< HTREEITEM, T >& map, HTREEITEM hItem) const
	{
		std::map< HTREEITEM, T >::const_iterator it = map.find(hItem);
		assert(it != map.end());
		return it->second;
	}

	template <class T>
	T& GetItemInfo(std::map< HTREEITEM, T >& map, HTREEITEM hItem)
	{
		std::map< HTREEITEM, T >::iterator it = map.find(hItem);
		assert(it != map.end());
		return it->second;
	}

	EItemType GetItemType(HTREEITEM hItem)
	{
		if (m_TreeInfo.VarInfo.find(hItem) != m_TreeInfo.VarInfo.end()) return eIT_TimeStamp;
		if (m_TreeInfo.SignalInfo.find(hItem) != m_TreeInfo.SignalInfo.end()) return eIT_Signal;
		return eIT_UNDEFINED;
	}

	template <class T> 
	void DeleteItem(HTREEITEM hItem, T& map)
	{
		HTREEITEM hParent = GetParentItem( hItem );
		assert(m_TreeInfo.ChildItems.find(hParent) != m_TreeInfo.ChildItems.end());
		bool ok = stl::find_and_erase(m_TreeInfo.ChildItems[hParent], hItem);
		assert( ok );

		m_signals.DeleteItem( hItem );
		assert(map.find(hItem) != map.end());
		map.erase(map.find(hItem));
	}

	template <class T>
	HTREEITEM InsertItem(HTREEITEM hParent, HTREEITEM hInsertAfter, const char* displayName, std::map< HTREEITEM, T >& map, const T& item)
	{
		HTREEITEM hNewItem = m_signals.InsertItem( displayName, 0, 0, hParent, hInsertAfter);
		map.insert( std::make_pair(hNewItem, item) );
		m_TreeInfo.ChildItems[ hParent ].push_back( SChildItem(hNewItem, T::GetType()) );
		return hNewItem;
	}

	TChildItemList& GetChildList(HTREEITEM hItem)
	{
// 		assert(m_TreeInfo.ChildItems.find(hItem) != m_TreeInfo.ChildItems.end());
		return m_TreeInfo.ChildItems[ hItem ];
	}

	const TChildItemList& GetChildList(HTREEITEM hItem) const
	{
		assert(m_TreeInfo.ChildItems.find(hItem) != m_TreeInfo.ChildItems.end());
		return m_TreeInfo.ChildItems.find(hItem)->second;
	}

};


#endif