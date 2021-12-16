#ifndef __ANIMATIONGRAPHSTATEQUERY2_H__
#define __ANIMATIONGRAPHSTATEQUERY2_H__

#pragma once

#include "Controls\PropertyCtrl.h"

#include "AnimationGraph_2.h"
#include "IAnimationGraph.h"
#include "IAnimationGraphSystem.h"

class CAnimationGraphDialog2;

class CAnimationGraphStateQuery2 : public CXTPTaskPanel
{
public:
	CAnimationGraphStateQuery2();
	~CAnimationGraphStateQuery2();

	void Init( CAnimationGraphDialog2 * pParent );
	void Reload();
	void Reload( bool bGraphChangedSinceSaved );
	void RunUnitTests( bool bMessageOnSuccess=true, bool bMessageOnLoadFail=true );

	void OnCommand( int cmd, bool bSkipConfirmations=false );

	CVarBlockPtr CreateCriteriaVariables( CAGInput2Ptr pInput );
	void SetSelection( CAGInput2Ptr pInput, ECriteriaType2 sType );
	ECriteriaType2 GetSelection( CAGInput2Ptr pInput );
	
	class UpdateLock
	{
	public:
		UpdateLock( CAnimationGraphStateQuery2 * pSQ );
		~UpdateLock();
		
	private:
		UpdateLock( const UpdateLock& );
		UpdateLock& operator=( const UpdateLock& );
		CAnimationGraphStateQuery2 * m_pSQ;
		int m_pos;
	};

private:
	class CPropGroup : public _reference_target_t
	{
	public:
		CPropGroup( CAnimationGraphStateQuery2 *pParent, const CString& name, CXTPTaskPanelGroup *group, const TAGPropMap& props );
		~CPropGroup();

		void Reset( const TAGPropMap& props, CXTPTaskPanelGroup *group );
		CPropertyCtrl * GetCtrl() { return m_pProps; }
		void Invalidate() { if (m_pProps) m_pProps->Invalidate(); }

	private:
		CAnimationGraphStateQuery2 *m_pParent;
		CPropertyCtrl *m_pProps;
		CXTPTaskPanelGroup *m_pGroup;
		CXTPTaskPanelGroupItem *m_pItem;
		CString m_sName;
	};
	typedef _smart_ptr<CPropGroup> CPropGroupPtr;
	typedef std::map<CAGInput2Ptr, SCriteria2> TSelectionMap;

	CAnimationGraphDialog2 *m_pParent;
	CXTPTaskPanelGroup *m_pGroupLoading;
	CXTPTaskPanelGroup *m_pGroupConditions;
	CXTPTaskPanelGroup *m_pGroupStates;
	CXTPTaskPanelGroup *m_pGroupPreselection;
	int m_iUpdateLock;
	CPropGroupPtr m_pCriteria;
	CPropGroupPtr m_pPreselections;
	TAGPropMap m_pMap;
	TAGPropMap m_pPreselectionMap;
	TSelectionMap m_SelectionMap;
	CVarBlockPtr m_pPreselectionLists;
	XmlNodeRef m_preselections;

	bool m_bGraphLoaded;
	bool m_bGraphChangedSinceSaved;
	bool m_bHideUpperBodyResults;
	bool m_bHideRankings;
	CString m_sCurrentGraphWorkingName;
	IAnimationGraphQueryResultsPtr m_pResults;

	void ReloadGroupLoading();
	void ReloadGroupConditions();
	void ReloadGroupStates();
	void ReloadGroupPreselection();
	void AddGroupLoadingText();
	void ResetPreselections();

	void AddVerb( CXTPTaskPanelGroup *pGroup, int id, const CString& name );
	void RemoveVerb( CXTPTaskPanelGroup *pGroup, int id );
	void SetVerbEnabled( CXTPTaskPanelGroup *pGroup, int id, bool bEnabled );
	void AddText( CXTPTaskPanelGroup *pGroup, const CString& name, bool bBold=false );
	void AddModifiableText( CXTPTaskPanelGroup *pGroup, int id, const CString& name, bool bBold=false );
	void UpdateModifiableText( CXTPTaskPanelGroup *pGroup, int id, const CString& name, bool bBold=false );

	CString GetGraphWorkingName(); // Get the name to use in IAnimationGraphSystem calls for the current graph
	CString GetGraphFileName(); // Get the filename of the current graph

	void DoQuery();
	void BuildPropMap();
	void BuildPreselectionLists();
	void ChangePreselection();
};

#endif
