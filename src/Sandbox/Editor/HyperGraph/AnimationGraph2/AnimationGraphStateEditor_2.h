#ifndef __ANIMATIONGRAPHSTATEEDITOR2_H__
#define __ANIMATIONGRAPHSTATEEDITOR2_H__

#pragma once

#include "..\ToolbarDialog.h"
#include "..\Controls\PropertyCtrl.h"

#include "AnimationGraph_2.h"
#include "AnimationGraph2_AnimationNodeDetails.h"

#include "FlatFrameCtrl.h"

class CAnimationGraphDialog2;

enum EStateEditorEvent2
{
	eSEE_InitContainer2,

	eSEE_Delete2,
	eSEE_Clone2,

	eSEE_RebuildCriteria2,

	eSEE_AddExtra2,
	eSEE_RemoveExtra2,
	eSEE_RebuildExtra2,
	eSEE_RebuildExtraPanels2,

	eSEE_RebuildInputPanel2,
	eSEE_RebuildInputTypedProperties2,

	eSEE_UpdateNodeDetailsPane,

	eSEE_AddValue2,
	eSEE_RemoveValue2,
	eSEE_RenameValue2,
};


// CFullSizePropertyCtrl

class CFullSizePropertyCtrl2 : public CPropertyCtrl
{
public:
	CXTPTaskPanelGroup* m_pGroup;
	CXTPTaskPanelGroupItem* m_pItem;
	CFullSizePropertyCtrl2() : m_pGroup(0), m_pItem(0) {}
private:
	DECLARE_MESSAGE_MAP()
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};


// CAnimationGraphStateEditor

class CAnimationGraphStateEditor2
{
public:
	CAnimationGraphStateEditor2() : m_pParent(0), m_updateLock(0) {}
	CXTPDockingPane * Init( CAnimationGraphDialog2 * pParent, UINT id );
	CXTPTaskPanel * GetPanel() { return &m_panel; }

	// panes for details
	//CXTPDockingPane *GetDetailsDockingPane();
	typedef CFlatFramedCtrl2< CAnimationGraphAnimationNodeDetails > TAnimationDetailsPane;
	TAnimationDetailsPane* GetAnimNodeDetailsPane() { return &m_AnimationNodeDetails; }


	void ClearActiveItem();
	void OnCommand( UINT id );

	void StateEvent( EStateEditorEvent2 );

	class UpdateLock
	{
	public:
		UpdateLock( CAnimationGraphStateEditor2 * pSE ) : m_pSE(pSE), m_pos(0)
		{
			if (1 == ++m_pSE->m_updateLock)
			{
				if (m_pSE->m_panel.GetSafeHwnd())
				{
					m_pSE->m_panel.GetParent()->LockWindowUpdate();

					SCROLLINFO si;
					si.cbSize = sizeof( SCROLLINFO );
					m_pSE->m_panel.GetScrollInfo( SB_VERT, &si, SIF_POS );
					m_pos = si.nPos;
				}
			}
		}
		~UpdateLock()
		{
			if (0 == --m_pSE->m_updateLock)
			{
				if (m_pSE->m_panel.GetSafeHwnd())
				{
					m_pSE->m_panel.GetParent()->UnlockWindowUpdate();

					SCROLLINFO si;
					si.cbSize = sizeof( SCROLLINFO );
					si.fMask = SIF_POS;
					si.nPos = m_pos;
					m_pSE->m_panel.SetScrollInfo( SB_VERT, &si );

					// workaround to hide buggy behavior of the panel
					m_pSE->m_panel.SendMessage( WM_VSCROLL, (m_pos<<16)|SB_THUMBTRACK, NULL );
				}
			}
		}
	private:
		UpdateLock( const UpdateLock& );
		UpdateLock& operator=( const UpdateLock& );
		CAnimationGraphStateEditor2 * m_pSE;
		int m_pos;
	};

private:
	enum EPropEditorType
	{
		ePET_State,
		ePET_States,
		ePET_View,
		ePET_Input,
		ePET_Link
	};

	CAnimationGraphDialog2 * m_pParent;

	typedef std::pair<CString, EStateEditorEvent2> Verb;

	class CPropGroup : public _reference_target_t
	{
	public:
		CPropGroup( CAnimationGraphStateEditor2 * pParent, const CString& name, UINT id, const TAGPropMap& props, const Verb * pVerbs );
		CPropGroup( CAnimationGraphStateEditor2 * pParent, const CString& name, UINT id, const std::vector<CString>& props, const Verb * pVerbs );
		~CPropGroup();

		void Reset( const TAGPropMap& props );
		void Reset( const std::vector<CString>& props );
		CPropertyCtrl * GetCtrl() { return m_pProps; }
		CListBox * GetList() { return m_pList; }
		void OnCommand( UINT id );
		void Invalidate() { if (m_pList) m_pList->Invalidate(); if (m_pProps) m_pProps->Invalidate(); }
		void Disable();

	private:
		void CommonInit( CAnimationGraphStateEditor2 * pParent, const CString& name, UINT id, const Verb * pVerbs );

		CAnimationGraphStateEditor2 * m_pParent;
		UINT m_id;
		CFullSizePropertyCtrl2 * m_pProps;
		CListBox * m_pList;
		CXTPTaskPanelGroup * m_pGroup;
		CXTPTaskPanelGroupItem * m_pItem;
		std::vector<EStateEditorEvent2> m_callbacks;
	};
	typedef _smart_ptr<CPropGroup> CPropGroupPtr;

	class CPropEditor : public _reference_target_t
	{
	public:
		CPropEditor( CAnimationGraphStateEditor2 * pSE, EPropEditorType type ) 
			: m_pSE(pSE)
			, m_type(type) 
		{}

		virtual void OnEvent( EStateEditorEvent2 ) = 0;
		bool IsSameEditor( CPropEditor* pEditor )
		{
			if (pEditor->m_type != m_type)
				return false;
			return CheckSameEditor( pEditor );
		}
		virtual CParamsDeclaration2 * GetParamsDeclaration() { return NULL; }
		virtual void ActivateParameterization( const TParameterizationId2* pParamId, const CString& viewName ) {}
		virtual void ResetParameterization() {}
		virtual bool IsParameterized() const { return false; }

		virtual bool AddParamValue( const char* param, const char* value ) { return false; }
		virtual bool DeleteParamValue( const char* param, const char* value ) { return false; }
		virtual bool RenameParamValue( const char* param, const char* oldValue, const char* newValue ) { return false; }

		virtual bool AddParameter( const CString& name ) { return false; }
		virtual bool DeleteParameter( const CString& name ) { return false; }
		virtual bool RenameParameter( const CString& oldName, const CString& newName ) { return false; }

	protected:
		virtual bool CheckSameEditor( CPropEditor* pEditor ) = 0;

		CAnimationGraphStateEditor2 * m_pSE;

		EPropEditorType m_type;
	};

	template <class T> class CPropEditorImpl;

	template <> class CPropEditorImpl<CAGState2Ptr> : public CPropEditor, public IAnimationGraphListener2
	{
	public:
		CPropEditorImpl( CAnimationGraphStateEditor2 * pSE, CAGState2Ptr pCurState );
		~CPropEditorImpl();

		void OnEvent( EStateEditorEvent2 );
		bool CheckSameEditor( CPropEditor* pEditor );
		void OnStateEvent( EAGStateEvent2 event, CAGState2Ptr pState );
		virtual CParamsDeclaration2 * GetParamsDeclaration() { return m_pCurState ? m_pCurState->GetParamsDeclaration() : NULL; }
		virtual void ActivateParameterization( const TParameterizationId2* pParamId, const CString& viewName );
		virtual void ResetParameterization() { if ( m_pCurState ) m_pCurState->ResetParameterization(); }
		virtual bool IsParameterized() const { return m_pCurState ? m_pCurState->IsParameterized() : false; }

		virtual bool AddParamValue( const char* param, const char* value ) { return m_pCurState ? m_pCurState->AddParamValue( param, value ) : false; }
		virtual bool DeleteParamValue( const char* param, const char* value ) { return m_pCurState ? m_pCurState->DeleteParamValue( param, value ) : false; }
		virtual bool RenameParamValue( const char* param, const char* oldValue, const char* newValue ) { return m_pCurState ? m_pCurState->RenameParamValue( param, oldValue, newValue ) : false; }

		virtual bool AddParameter( const CString& name ) { return m_pCurState ? m_pCurState->AddParameter( name ) : false; }
		virtual bool DeleteParameter( const CString& name ) { return m_pCurState ? m_pCurState->DeleteParameter( name ) : false; }
		virtual bool RenameParameter( const CString& oldName, const CString& newName ) { return m_pCurState ? m_pCurState->RenameParameter( oldName, newName ) : false; }

	private:
		void BuildExtras();

		CPropGroupPtr m_pGeneralPanel;
		CPropGroupPtr m_pCriteriaPanel;
		//CPropGroupPtr m_pOverridablePanel;
		CPropGroupPtr m_pExtrasPanel;
		//CPropGroupPtr m_pTemplatePanel;
		CAGState2Ptr m_pCurState;
	};
	template <> class CPropEditorImpl< std::vector< CAGState2Ptr > > : public CPropEditor, public IAnimationGraphListener2
	{
	public:
		CPropEditorImpl( CAnimationGraphStateEditor2 * pSE, std::vector< CAGState2Ptr >& vCurStates );
		~CPropEditorImpl();

		void OnEvent( EStateEditorEvent2 );
		bool CheckSameEditor( CPropEditor* pEditor );

	private:
	//	void BuildExtras();

		CPropGroupPtr m_pGeneralPanel;
		CPropGroupPtr m_pCriteriaPanel;
		//CPropGroupPtr m_pOverridablePanel;
		//CPropGroupPtr m_pExtrasPanel;
		//CPropGroupPtr m_pTemplatePanel;
		std::vector< CAGState2Ptr > m_vCurStates;
	};
	template <> class CPropEditorImpl<CAGView2Ptr> : public CPropEditor
	{
	public:
		CPropEditorImpl( CAnimationGraphStateEditor2 * pSE, CAGView2Ptr pCurView );
		~CPropEditorImpl();

		void OnEvent( EStateEditorEvent2 );
		bool CheckSameEditor( CPropEditor* pEditor );

	private:
		CAGView2Ptr m_pCurView;
	};
	template <> class CPropEditorImpl<CAGInput2Ptr> : public CPropEditor, public IAnimationGraphListener2
	{
	public:
		CPropEditorImpl( CAnimationGraphStateEditor2 * pSE, CAGInput2Ptr pCurView );
		~CPropEditorImpl();

		void OnEvent( EStateEditorEvent2 );
		bool CheckSameEditor( CPropEditor* pEditor );
		void OnInputEvent( EAGInputEvent2 event, CAGInput2Ptr pInput );

	private:
		CAGInput2Ptr m_pCurInput;
	};
	template <> class CPropEditorImpl<CAGLink2Ptr> : public CPropEditor, public IAnimationGraphListener2
	{
	public:
		CPropEditorImpl( CAnimationGraphStateEditor2 * pSE, CAGLink2Ptr pCurView );
		~CPropEditorImpl();

		void OnEvent( EStateEditorEvent2 );
		bool CheckSameEditor( CPropEditor* pEditor );
		void OnLinkEvent( EAGLinkEvent2 event, CAGLink2Ptr pLink );

	private:
		CAGLink2Ptr m_pCurLink;
	};

	CXTPTaskPanel m_panel;
	std::vector<CPropGroupPtr> m_propGroups;
	int m_updateLock;
	_smart_ptr<CPropEditor> m_pCurEdit;


	CPropGroupPtr AddPropertyGroup( const CString& name, const TAGPropMap& props, const Verb * pVerbs = NULL );
	CPropGroupPtr AddPropertyGroup( const CString& name, const std::vector<CString>& props, const Verb * pVerbs = NULL );
	void SetEditor( _smart_ptr<CPropEditor> pImpl );

	// ---- The various properties dialogs -----

	//! End-Animation Nodes
	TAnimationDetailsPane m_AnimationNodeDetails;

public:

	const _smart_ptr<CPropEditor> GetActiveItem() const { return m_pCurEdit; }

	template <class T>
	void EditPropertiesOf( T value )
	{
		if (!value)
			ClearActiveItem();
		else
		{
			_smart_ptr<CPropEditor> pImpl = new CPropEditorImpl<T>( this, value );
			SetEditor( pImpl );
		}
	}

	void EditPropertiesOf( std::vector< CAGState2Ptr > values )
	{
		if ( values.empty() )
			ClearActiveItem();
		else if ( values.size() == 1 )
			EditPropertiesOf( values[0] );
		else
		{
			_smart_ptr<CPropEditor> pImpl = new CPropEditorImpl< std::vector< CAGState2Ptr > >( this, values );
			SetEditor( pImpl );
		}
	}

	void OnStateParamSelChanged( const TParameterizationId2* pParamsId, const CString& viewName );
	void ResetParameterization() { if ( m_pCurEdit ) m_pCurEdit->ResetParameterization(); }
	bool IsParameterized() const { return m_pCurEdit ? m_pCurEdit->IsParameterized() : false; }

	bool AddParamValue( const char* param, const char* value ) { return m_pCurEdit ? m_pCurEdit->AddParamValue( param, value ) : false; }
	bool DeleteParamValue( const char* param, const char* value ) { return m_pCurEdit ? m_pCurEdit->DeleteParamValue( param, value ) : false; }
	bool RenameParamValue( const char* param, const char* oldValue, const char* newValue ) { return m_pCurEdit ? m_pCurEdit->RenameParamValue( param, oldValue, newValue ) : false; }

	bool AddParameter( const CString& name ) { return m_pCurEdit ? m_pCurEdit->AddParameter( name ) : false; }
	bool DeleteParameter( const CString& name ) { return m_pCurEdit ? m_pCurEdit->DeleteParameter( name ) : false; }
	bool RenameParameter( const CString& oldName, const CString& newName ) { return m_pCurEdit ? m_pCurEdit->RenameParameter( oldName, newName ) : false; }
};

#endif
