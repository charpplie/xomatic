#include "StdAfx.h"
#include "AnimationGraph_2.h"
#include "AnimationGraphStateEditor_2.h"
#include <queue>
#include "..\Controls\PropertyCtrl.h"
#include "IAnimationGraphSystem.h"
#include "..\HyperGraphNode.h"
#include "HyperNodePainter_MultiIOBox_2.h"
#include "SpeedReport_2.h"
#include "StringUtils.h"
#include "..\StringDlg.h"
#include "..\Util\FileUtil.h"
#include "AnimationGraph2_Modifier.h"


static const float AG_VERSION = 1.5f;

#define NO_VALUE_STRING "<none>"
#define ADD_KEY_STRING "<Add Key...>"
const CString DIFFERENT_VALUES_STRING = "Different Values Selected";

char* g_szMCMString[eMCM_COUNT] = { "Undefined", "Entity", "Animation", "DecoupledCatchUp", "ClampedEntity", "SmoothedEntity", "AnimationHCollision" };
char* g_szColliderModeString[eColliderMode_COUNT] = { "Undefined", "Disabled", "GroundedOnly", "Pushable", "NonPushable", "PushesPlayersOnly", "Spectator" };

bool CParamsDeclaration2::GuessParamId( const TParameterizationId2& hint, TParameterizationId2& returnId ) const
{
	returnId.clear();
	for ( const_iterator it = begin(); it != end(); ++it )
	{
		CString paramName( it->first );
		paramName.MakeLower();
		TParameterizationId2::const_iterator itHint = hint.find( paramName );
		if ( itHint == hint.end() )
		{
			// there's no hint for this parameter
			returnId.clear();
			return false;
		}
		if ( it->second.find(itHint->second) == it->second.end() )
		{
			// invalid hint
			returnId.clear();
			return false;
		}
		returnId.insert( std::make_pair(it->first, itHint->second) );
	}
	return true;
}


namespace
{

	template <class T>
	class CSetCallback : public Functor1<IVariable*>
	{
	public:
		CSetCallback( T& value ) : Functor1<IVariable*>(thunk, &value, 0, 0, 0) {}

		static void thunk( const FunctorBase& ftor, IVariable * pVar )
		{
			pVar->Get( *(T*)ftor.getCallee() );
		}
	};

	template <class H>
	class CNotifyCallback : public Functor1<IVariable*>
	{
	public:
		CNotifyCallback( H * pHost, void (H::*notification)() ) : Functor1<IVariable*>(thunk, pHost, 0, &notification, sizeof(notification)) {}

		static void thunk( const FunctorBase& ftor, IVariable * )
		{
			typedef void (H::*Notification)();
			Notification notification = *(Notification*)(void*)ftor.getMemFunc();
			H * callee = (H*)ftor.getCallee();
			(callee->*notification)();
		}
	};

	IVariablePtr SetGraph( IVariablePtr var, CAnimationGraph2 * pGraph )
	{
		var->AddOnSetCallback( CNotifyCallback<CAnimationGraph2>(pGraph, &CAnimationGraph2::Modified) );
		return var;
	}

	template <class T>
	IVariablePtr VariableHelper( CVarBlockPtr varBlock, const CString& humanName, T& value )
	{
		IVariablePtr var = new CVariable<T>();
		var->Set(value);
		var->AddOnSetCallback( CSetCallback<T>(value) );
		var->SetHumanName(humanName);
		var->SetFlags(0);
		varBlock->AddVariable(var);
		return var;
	}

	template <class T>
	IVariablePtr ConstVariableHelper( CVarBlockPtr varBlock, const CString& humanName, const T& value )
	{
		IVariablePtr var = new CVariable<T>();
		var->Set(value);
		var->SetHumanName(humanName);
		var->SetFlags( IVariable::UI_DISABLED | IVariable::UI_BOLD );
		varBlock->AddVariable(var);
		return var;
	}

	template <class T, class H>
	IVariablePtr VariableHelper( CVarBlockPtr varBlock, const CString& humanName, T& value, H * pHost, void (H::*notification)() )
	{
		IVariablePtr pVar = VariableHelper( varBlock, humanName, value );
		pVar->AddOnSetCallback( CNotifyCallback<H>(pHost, notification) );
		return pVar;
	}

	class CStateVariable : public CVariableEnum<CString>, public IAnimationGraphListener2
	{
	public:
		CStateVariable(CAnimationGraph2 * pGraph, CAGState2Ptr& ptr, CAGState2Ptr selectedState) : m_pGraph(pGraph), m_ptr(ptr), m_selectedState(selectedState)
		{
			m_pGraph->AddListener(this);
		}
		~CStateVariable()
		{
			m_pGraph->RemoveListener(this);
		}

		void Init()
		{
			Reload();
			AddOnSetCallback( CNotifyCallback<CStateVariable>(this, &CStateVariable::Change) );
		}

		void Reload()
		{
			_smart_ptr<CVarEnumList<CString> > pEnum = new CVarEnumList<CString>();
			pEnum->AddItem(NO_VALUE_STRING, NO_VALUE_STRING);

			std::vector<CString> listItems;
			for (CAnimationGraph2::state_iterator iter = m_pGraph->StateBegin(); iter != m_pGraph->StateEnd(); ++iter)
			{
				CAGState2Ptr state = ( *iter );
				if ( ValidParentForSelectedNode( state ) )
				{
					listItems.push_back( state->GetName() );
				}
			}
			std::sort( listItems.begin(), listItems.end() );

			for (std::vector<CString>::const_iterator iter = listItems.begin(); iter != listItems.end(); ++iter)
				pEnum->AddItem( *iter, *iter );

			SetEnumList(pEnum);
		}

		void OnStateEvent( EAGStateEvent2 evt, CAGState2Ptr pState )
		{
			if (evt == eAGSE_ChangeName2)
				Reload();
		}

	private:
		void Change()
		{
			CString temp;
			Get(temp);
			if (temp != NO_VALUE_STRING)
				m_ptr = m_pGraph->FindState( temp );
			else
				m_ptr = CAGState2Ptr();
		}

		bool ValidParentForSelectedNode( CAGState2Ptr state )
		{
			if ( ! state )
			{
				return true;
			}

			bool foundLoop = ( state == m_selectedState );
			if ( foundLoop )
			{
				return false;
			}

			return ValidParentForSelectedNode( state->GetParent() );
		}

		CAGState2Ptr& m_ptr;
		CAnimationGraph2 * m_pGraph;
		CAGState2Ptr m_selectedState;
	};

	class CStateGroupVariable : public CVariableEnum<CString>, public IAnimationGraphListener2
	{
	public:
		CStateGroupVariable(CAnimationGraph2 * pGraph, CString& ptr) : m_pGraph(pGraph), m_ptr(ptr)
		{
			m_pGraph->AddListener(this);
		}
		~CStateGroupVariable()
		{
			m_pGraph->RemoveListener(this);
		}

		void Init()
		{
			Reload();
			AddOnSetCallback( CNotifyCallback<CStateGroupVariable>(this, &CStateGroupVariable::Change) );
		}

		void Reload()
		{
			_smart_ptr<CVarEnumList<CString> > pEnum = new CVarEnumList<CString>();
			pEnum->AddItem(NO_VALUE_STRING, NO_VALUE_STRING);

			std::vector<CString> listItems;
			for (CAnimationGraph2::state_iterator iter = m_pGraph->StateBegin(); iter != m_pGraph->StateEnd(); ++iter)
				listItems.push_back((*iter)->GetName());
			std::sort( listItems.begin(), listItems.end() );

			for (std::vector<CString>::const_iterator iter = listItems.begin(); iter != listItems.end(); ++iter)
				pEnum->AddItem( *iter, *iter );

			SetEnumList(pEnum);
		}

		void OnStateEvent( EAGStateEvent2 evt, CAGState2Ptr pState )
		{
			if (evt == eAGSE_ChangeName2)
				Reload();
		}

	private:
		void Change()
		{
			CString temp;
			Get(temp);
			if (temp != NO_VALUE_STRING)
				m_ptr = temp;
			else
				m_ptr = CString();
		}

		CString& m_ptr;
		CAnimationGraph2 * m_pGraph;
	};

	class CCriteriaModeVariable : public CVariableEnum<int>
	{
	public:
		CCriteriaModeVariable( CVarBlock * pVB, CAGInput2Ptr pInput, CAGState2Ptr pState, CAnimationGraphStateEditor2 * pSE ) : m_pVB(pVB), m_pInput(pInput), m_pState(pState), m_pSE(pSE)
		{
			ECriteriaType2 value = pState->GetCriteriaType(pInput);
			if ( value == eCT_DefinedInTemplate2 )
			{
				AddEnumItem( "Defined in template", eCT_DefinedInTemplate2 );
				Set( int(eCT_DefinedInTemplate2) );
			}
			else
			{
				if ( value != eCT_UseParent2 && m_pState->GetActiveParameterization() != NULL )
				{
					AddEnumItem( "Use Default", eCT_UseParent2 );
					if ( value == eCT_UseDefault2 )
						value = eCT_UseParent2;
				}
				else
				{
					AddEnumItem( "Use Parent State", eCT_UseParent2 );
				}
				AddEnumItem( "Any Value", eCT_AnyValue2 );
				AddEnumItem( "Specified Value", eCT_ExactValue2 );
				if (pInput->CanUseMinMaxCriteria())
					AddEnumItem( "Value Range (min/max)", eCT_MinMax2 );
				if (value == eCT_DifferentValues2)
					AddEnumItem( DIFFERENT_VALUES_STRING, eCT_DifferentValues2 );
				Set( int(value) );
				AddOnSetCallback( CNotifyCallback<CCriteriaModeVariable>(this, &CCriteriaModeVariable::Change) );
			}
		}

		void Init()
		{
			int value;
			Get(value);
			if ( value != eCT_DefinedInTemplate2 )
			{
				CVarBlockPtr pVB = m_pState->CreateCriteriaVariables( m_pInput, true, m_pState->GetActiveParameterization() );
				for (int i=0; i<pVB->GetVarsCount(); i++)
					m_pVB->AddVariable( pVB->GetVariable(i) );
			}
		}

	private:
		void Change()
		{
			int temp;
			Get(temp);
			ECriteriaType2 critType = (ECriteriaType2) temp;

			// now set the criteria
			m_pState->SetCriteriaType( m_pInput, critType );

			// and rebuild the panel
			if (m_pSE)
				m_pSE->StateEvent( eSEE_RebuildCriteria2 );
		}

		CVarBlock * m_pVB;
		CAGInput2Ptr m_pInput;
		CAGState2Ptr m_pState;
		CAnimationGraphStateEditor2 * m_pSE;
	};

	class CCriteriaKeyValueVariable : public CVariableEnum<CString>
	{
	public:
		CCriteriaKeyValueVariable( CVarBlock * pVB, CAGInput2Ptr pInput, CString& value ) : m_pVB(pVB), m_pInput(pInput), m_valueRef(value)
		{
			AddEnumItem( ADD_KEY_STRING, ADD_KEY_STRING );

			for ( CAGInput2::key_iterator it = pInput->KeyBegin(); it != pInput->KeyEnd(); ++it )
				AddEnumItem( *it, *it );

			CString temp;
			temp.Format( "[%s]", pInput->GetName() );
			temp.MakeLower();
			AddEnumItem( temp, temp );

			Set( m_valueRef );
			AddOnSetCallback( CNotifyCallback< CCriteriaKeyValueVariable >(this, &CCriteriaKeyValueVariable::Change) );
		}

	private:
		void Change()
		{
			CString temp;
			Get( temp );

			if ( temp != ADD_KEY_STRING )
			{
				m_valueRef = temp;
				return;
			}

			CString name;
			for ( int i=0; ; i++ )
			{
				name.Format( "input%d", i );
				if ( !m_pInput->HasKey(name) )
					break;
			}
			CStringDlg dlg( "Add Key" );
			dlg.m_strString = name;
			if ( dlg.DoModal() == IDOK && !dlg.m_strString.IsEmpty() )
			{
				if ( m_pInput->HasKey(dlg.m_strString) )
					AfxMessageBox("WARNING: Key already exists!");
				else
					m_pInput->AddKey( dlg.m_strString );
				m_valueRef = dlg.m_strString;
				AddEnumItem( m_valueRef, m_valueRef );
				Set( m_valueRef );
			}
			else
				Set( m_valueRef );
		}

		CVarBlock * m_pVB;
		CAGInput2Ptr m_pInput;
		CString& m_valueRef;
	};

	class COverridableModeVariable : public CVariableEnum<CString>
	{
	public:
		COverridableModeVariable( CVarBlock * pVB, CAGCategory2Ptr pCategory, CAGState2Ptr pState, CAnimationGraphStateEditor2 * pSE ) : m_pCategory(pCategory), m_pState(pState), m_pSE(pSE)
		{
			AddEnumItem( "Use parent implementation", "_parent" );
			AddEnumItem( "Use no implementation", "_none" );

			CAnimationGraph2 * pGraph = pState->GetGraph();
			for (CAnimationGraph2::state_factory_iterator iter = pGraph->StateFactoryBegin(); iter != pGraph->StateFactoryEnd(); ++iter)
			{
				if ((*iter)->GetCategory() == pCategory)
					AddEnumItem((*iter)->GetHumanName(), (*iter)->GetName());
			}

			Set( pState->GetStateFactoryMode(pCategory) );

			AddOnSetCallback( CNotifyCallback<COverridableModeVariable>( this, &COverridableModeVariable::OnChange ) );
		}

		void Init( CVarBlockPtr pVBDisplay )
		{
			CAGState2Ptr pState = m_pState;
			while (pState)
			{
				CString mode = pState->GetStateFactoryMode( m_pCategory );
				if (mode == "_parent")
				{
					pState = pState->GetParent();
					continue;
				}
				else if (mode == "_none")
				{
				}
				else
				{
					CVarBlockPtr pVBEdit = pState->GetStateFactoryVarBlock( m_pCategory );
					int nVars = pVBEdit->GetVarsCount();
					int flags = 0;
					if (pState != m_pState)
						flags |= IVariable::UI_DISABLED;
					for (int i=0; i<nVars; i++)
					{
						IVariablePtr pVar = pVBEdit->GetVariable(i);
						pVar->SetFlags( flags );
						pVBDisplay->AddVariable( pVar );
					}
				}

				break;
			}
		}

	private:
		CAGCategory2Ptr m_pCategory;
		CAGState2Ptr m_pState;
		CAnimationGraphStateEditor2 * m_pSE;

		void OnChange()
		{
			CString newMode;
			Get(newMode);
			m_pState->SetStateFactoryMode( m_pCategory, newMode );
		}
	};

	class CTemplateVariable : public CVariableEnum<CString>
	{
	public:
		CTemplateVariable( CVarBlock * pVB, CAGStateTemplate2Ptr pStateTemplate, CAGState2Ptr pState, CAnimationGraphStateEditor2 * pSE ) : m_pStateTemplate(pStateTemplate), m_pState(pState), m_pSE(pSE)
		{
			CAnimationGraph2 * pGraph = pState->GetGraph();

			std::vector<CString> templates;
			for (CAnimationGraph2::state_template_iterator iter = pGraph->StateTemplateBegin(); iter != pGraph->StateTemplateEnd(); ++iter)
				templates.push_back((*iter)->GetName());
			std::sort(templates.begin(), templates.end());
			for (std::vector<CString>::const_iterator iter = templates.begin(); iter != templates.end(); ++iter)
				AddEnumItem( *iter, *iter );

			Set( pStateTemplate->GetName() );

			if ( pState->GetActiveParameterization() )
				SetFlags( GetFlags() | IVariable::UI_DISABLED );

			AddOnSetCallback( CNotifyCallback<CTemplateVariable>( this, &CTemplateVariable::OnChange ) );
		}

		void Init( CVarBlockPtr pVBDisplay )
		{
			const std::map<CString, CString>& tplParams = m_pStateTemplate->GetParams();
			std::map<CString, CString>& stParams = m_pState->GetTemplateParameters();
			for (std::map<CString, CString>::const_iterator iter = tplParams.begin(); iter != tplParams.end(); ++iter)
				VariableHelper( pVBDisplay, iter->first, stParams[iter->first] );
		}

	private:
		CAGStateTemplate2Ptr m_pStateTemplate;
		CAGState2Ptr m_pState;
		CAnimationGraphStateEditor2 * m_pSE;

		void OnChange()
		{
			CString newMode;
			Get(newMode);
			m_pState->SetTemplateType( newMode );
		}
	};

	class CSetInclusionVariable : public CVariable<bool>
	{
	public:
		CSetInclusionVariable( CString key, std::set<CString>& values ) : m_key(key), m_values(values), m_initialized(false)
		{
		}

		void Init()
		{
			bool included = m_values.find(m_key) != m_values.end();
			SetValue(included);			
			m_initialized = true;
		}

		void OnSetValue(bool)
		{
			if (!m_initialized)
				return;
			bool included = false;
			GetValue(included);
			if (included)
				m_values.insert(m_key);
			else
				m_values.erase(m_key);
		}

	private:
		CString m_key;
		std::set<CString>& m_values;
		bool m_initialized;
	};

	class CMovementControlMethodVariable : public CVariableEnum<int>
	{
	public:

		CMovementControlMethodVariable(EMovementControlMethod* pMCM)
		{
			m_pMCM = pMCM;

			int temp = (int)*m_pMCM;
			Set(temp);

			AddEnumItem("Entity", eMCM_Entity);
			AddEnumItem("Animation", eMCM_Animation);
			AddEnumItem("DecoupledCatchUp", eMCM_DecoupledCatchUp);

			AddOnSetCallback(CNotifyCallback<CMovementControlMethodVariable>(this, &CMovementControlMethodVariable::Change));
		}

	private:

		void Change()
		{
			int temp;
			Get(temp);
			*m_pMCM = (EMovementControlMethod)temp;
		}

		EMovementControlMethod* m_pMCM;

	};

	class CLinkParamsVariable : public CVariableArray
	{
		class CCallback : public Functor1<IVariable*>
		{
		public:
			typedef CLinkParamsVariable H;

			CCallback( H* pHost, void (H::*notification)(IVariable*) ) : Functor1<IVariable*>(thunk, pHost, 0, &notification, sizeof(notification)) {}

			static void thunk( const FunctorBase& ftor, IVariable* pVar )
			{
				typedef void (H::*Notification)(IVariable*);
				Notification notification = *(Notification*)(void*)ftor.getMemFunc();
				H * callee = (H*)ftor.getCallee();
				(callee->*notification)(pVar);
			}
		};

	public:
		CLinkParamsVariable( CAGLink2Ptr pLink, const CAGState2Ptr pOwnerState, const CAGState2Ptr pRelatedState, bool bIsSource )
			: m_pLink( pLink )
			, m_pOwnerState( pOwnerState )
			, m_pRelatedState( pRelatedState )
			, m_bIsSource( bIsSource )
		{
			SetHumanName( m_pOwnerState->GetName() );
		}

		void Init()
		{
			DeleteAllChilds();
			const CParamsDeclaration2* pParams = m_pOwnerState->GetParamsDeclaration();
			if ( !pParams || pParams->empty() )
			{
				_smart_ptr< CVariableEnum<CString> > pVar = new CVariableEnum< CString >;
				pVar->SetFlags( IVariable::UI_DISABLED );
				pVar->AddEnumItem( "", "" );
				AddChildVar( pVar );
				return;
			}

			CParamsDeclaration2::const_iterator it, itEnd = pParams->end();
			for ( it = pParams->begin(); it != itEnd; ++it )
			{
				CString name;
				name = '[';
				name += it->first;
				name += ']';

				_smart_ptr< CVariableEnum<CString> > pVar = new CVariableEnum< CString >;
				pVar->SetHumanName( it->first );
				pVar->AddEnumItem( "", "" );

				const CParamsDeclaration2* pRelatedParams = m_pRelatedState->GetParamsDeclaration();
				CParamsDeclaration2::const_iterator itRelated, itRelatedEnd = pRelatedParams->end();
				for ( itRelated = pRelatedParams->begin(); itRelated != itRelatedEnd; ++itRelated )
				{
					CString temp;
					temp = '[';
					temp += itRelated->first;
					temp += ']';
					CString lower( temp );
					lower.MakeLower();
					pVar->AddEnumItem( temp, lower );
				}

				TSetOfCString::const_iterator itValues, itValuesEnd = it->second.end();
				for ( itValues = it->second.begin(); itValues != itValuesEnd; ++itValues )
					pVar->AddEnumItem( *itValues, *itValues );

				name.MakeLower();
				CAGLink2::TListMappedParams::iterator itMapped, itMappedEnd = m_pLink->m_listMappedParams.end();
				for ( itMapped = m_pLink->m_listMappedParams.begin(); itMapped != itMappedEnd; ++itMapped )
					if ( name == (m_bIsSource ? itMapped->first : itMapped->second) )
						pVar->Set( m_bIsSource ? itMapped->second : itMapped->first );

				pVar->AddOnSetCallback(CCallback( this, &CLinkParamsVariable::OnChange ));
				AddChildVar( pVar );
			}
		}

		CLinkParamsVariable* m_pCrossLinked;

	private:
		void OnChange( IVariable* pVar )
		{
			CString name;
			name = '[';
			name += pVar->GetHumanName();
			name += ']';
			name.MakeLower();
			CString value;
			pVar->Get( value );

			CString paramValue;
			if ( !value.IsEmpty() && value.GetAt(0) == '[' && value.GetAt(value.GetLength()-1) == ']' )
				paramValue = value;
			paramValue.MakeLower();

			bool bFound = false;
			CAGLink2::TListMappedParams::iterator it = m_pLink->m_listMappedParams.begin();
			CAGLink2::TListMappedParams::iterator itEnd = m_pLink->m_listMappedParams.end();
			if ( m_bIsSource )
			{
				while ( it != itEnd )
				{
					if ( it->first == name )
					{
						bFound = true;
						if ( value.IsEmpty() )
						{
							m_pLink->m_listMappedParams.erase( it++ );
						}
						else
						{
							it->second = value;
							++it;
						}
					}
					else if ( it->second == paramValue )
						m_pLink->m_listMappedParams.erase( it++ );
					else
						++it;
				}
				if ( !bFound && !value.IsEmpty() )
					m_pLink->m_listMappedParams.push_back( std::make_pair(name,value) );
			}
			else
			{
				while ( it != itEnd )
				{
					if ( it->second == name )
					{
						bFound = true;
						if ( value.IsEmpty() )
						{
							m_pLink->m_listMappedParams.erase( it++ );
						}
						else
						{
							it->first = value;
							++it;
						}
					}
					else if ( it->first == paramValue )
						m_pLink->m_listMappedParams.erase( it++ );
					else
						++it;
				}
				if ( !bFound && !value.IsEmpty() )
					m_pLink->m_listMappedParams.push_back( std::make_pair(value,name) );
			}
			m_pLink->MappingChanged();
		}

		CAGLink2Ptr m_pLink;
		const CAGState2Ptr m_pOwnerState;
		const CAGState2Ptr m_pRelatedState;
		bool m_bIsSource;
	};

	IVariablePtr VariableHelper_StateSelector( CVarBlockPtr varBlock, const CString& humanName, CAGState2Ptr& extendVar, CAnimationGraph2 * pGraph, CAGState2Ptr selectedState )
	{
		CAGState2Ptr origValue = extendVar;
		_smart_ptr<CStateVariable> var = new CStateVariable(pGraph, extendVar, selectedState);
		var->Init();
		if (origValue)
			var->Set(origValue->GetName());
		else
			var->Set(NO_VALUE_STRING);
		var->SetHumanName(humanName);
		varBlock->AddVariable(var);
		return &*var;
	}

	IVariablePtr VariableHelper_SetInclusion( CVarBlockPtr varBlock, const CString& humanName, const CString& key, std::set<CString>& values, CAnimationGraph2 * pGraph )
	{
		_smart_ptr<CSetInclusionVariable> var = new CSetInclusionVariable(key, values);
		var->Init();
		var->SetHumanName(humanName);
		varBlock->AddVariable(var);
		return &*var;
	}

	IVariablePtr VariableHelper_StateGroupSelector( CVarBlockPtr varBlock, const CString& humanName, CString& groupVar, CAnimationGraph2 * pGraph )
	{
		_smart_ptr<CStateGroupVariable> var = new CStateGroupVariable(pGraph, groupVar);
		var->Init();
		if (groupVar.IsEmpty())
			var->Set(NO_VALUE_STRING);
		else
			var->Set(groupVar);
		var->SetHumanName(humanName);
		varBlock->AddVariable(var);
		return &*var;
	}

	IVariablePtr VariableHelper_MCM(CVarBlockPtr varBlock, const CString& humanName, EMovementControlMethod* pMCM)
	{
		_smart_ptr<CMovementControlMethodVariable> var = new CMovementControlMethodVariable(pMCM);
		var->SetHumanName(humanName);
		varBlock->AddVariable(var);
		return &*var;
	}

	IVariablePtr VariableHelper_Criteria( CVarBlockPtr varBlock, const CString& humanName, CAGInput2Ptr pInput, CAGState2Ptr pState, CAnimationGraphStateEditor2 * pSE )
	{
		_smart_ptr<CCriteriaModeVariable> var = new CCriteriaModeVariable(varBlock, pInput, pState, pSE);
		var->SetHumanName(humanName);
		varBlock->AddVariable(var);
		var->Init();
		return &*var;
	}

	IVariablePtr VariableHelper_Overridable( CVarBlockPtr varBlock, const CString& humanName, CAGCategory2Ptr pCategory, CAGState2Ptr pState, CAnimationGraphStateEditor2 * pSE )
	{
		_smart_ptr<COverridableModeVariable> var = new COverridableModeVariable(varBlock, pCategory, pState, pSE);
		var->SetHumanName(humanName);
		if ( pState->GetActiveParameterization() )
			var->SetFlags( var->GetFlags() | IVariable::UI_DISABLED );
		varBlock->AddVariable(var);
		var->Init(varBlock);
		return &*var;
	}

	IVariablePtr VariableHelper_Template( CVarBlockPtr pVB, const CString& humanName, CAGStateTemplate2Ptr pStateTemplate, CAGState2Ptr pState, CAnimationGraphStateEditor2 * pSE )
	{
		_smart_ptr<CTemplateVariable> var = new CTemplateVariable(pVB, pStateTemplate, pState, pSE);
		var->SetHumanName(humanName);
		pVB->AddVariable(var);
		var->Init(pVB);
		return &*var;
	}

	_smart_ptr< CLinkParamsVariable > VariableHelper_ParamMapping( CVarBlockPtr pVB, CAGLink2Ptr pLink, CAGState2Ptr pStateFrom, CAGState2Ptr pStateTo, bool bIsSource )
	{
		_smart_ptr< CLinkParamsVariable > pVar = new CLinkParamsVariable( pLink, pStateFrom, pStateTo, bIsSource );
		pVB->AddVariable( pVar );
		pVar->Init();
		return pVar;
	}
}

/*
 * CAGCompositeOperation
 */

CString CAGCompositeOperation2::GetExplanation()
{
	CString out;
	for (std::vector<IAGCheckedOperation2Ptr>::const_iterator iter = m_ops.begin(); iter != m_ops.end(); ++iter)
		out += (*iter)->GetExplanation() + ", ";
	return out;
}

void CAGCompositeOperation2::Perform()
{
	for (std::vector<IAGCheckedOperation2Ptr>::const_iterator iter = m_ops.begin(); iter != m_ops.end(); ++iter)
		(*iter)->Perform();
}

/*
 * CAGCategory
 */

CAGCategory2::CAGCategory2( const IAnimationGraphCategoryIterator::SCategory& cat )
{
	m_name = cat.name;
	m_bOverridable = cat.overridable;
  m_bUsableWithTemplate = cat.usableWithTemplate;
}

/*
 * CAGStateFactoryParam
 */

CAGStateFactoryParam2Ptr CAGStateFactoryParam2::Create(const IAnimationGraphStateFactoryIterator::SStateFactoryParameter& p)
{
#define DEF_CREATE_TYPE(hname, cname) if (0 == strcmp(hname, p.type)) return new CAGStateFactoryParamImpl2<cname>(p);
	DEF_CREATE_TYPE("int", int);
	DEF_CREATE_TYPE("float", float);
	DEF_CREATE_TYPE("vec3", Vec3);
	DEF_CREATE_TYPE("string", CString);
	DEF_CREATE_TYPE("bool", bool);
#undef DEF_CREATE_TYPE
	CryFatalError("Unknown parameter type: %s", p.type);
	return CAGStateFactoryParam2Ptr();
}

CAGStateFactoryParam2::CAGStateFactoryParam2( const IAnimationGraphStateFactoryIterator::SStateFactoryParameter& p )
{
	m_bRequired = p.required;
	m_name = p.name;
	m_humanName = p.humanName;
	m_defaultValue = p.defaultValue;
	m_upgrade = p.upgradeFunction;
}

void CAGStateFactoryParam2::AddToVarBlock( CVarBlockPtr pVB )
{
	IVariablePtr pVar;

	if (!m_bRequired)
	{
		pVar = new CVariable<bool>();
		pVar->SetHumanName( "Use " + m_name );
		pVar->SetName( "_use_" + m_name );
		pVar->Set(false);
		pVB->AddVariable( pVar );
	}

	pVar = CreateVariable();
	pVar->SetHumanName( m_humanName );
	pVar->SetName( m_name );
	if (!m_defaultValue.IsEmpty())
		pVar->Set( m_defaultValue );
	pVB->AddVariable( pVar );
}

void CAGStateFactoryParam2::Load( XmlNodeRef node, CVarBlockPtr pVB )
{
	if (!m_bRequired)
	{
		if (node->haveAttr(m_name))
		{
			pVB->FindVariable("_use_" + m_name)->Set(true);
			pVB->FindVariable(m_name)->Set( node->getAttr(m_name) );
		}
		else
		{
			pVB->FindVariable("_use_" + m_name)->Set(false);
		}
	}
	else
	{
		if (!node->haveAttr(m_name) && m_upgrade)
			m_upgrade(node);
		pVB->FindVariable(m_name)->Set( node->getAttr(m_name) );
	}
}

void CAGStateFactoryParam2::Save( XmlNodeRef node, CVarBlockPtr pVB )
{
	bool have = true;
	if (!m_bRequired)
	{
		have = false;
		pVB->FindVariable("_use_" + m_name)->Get(have);
	}
	if (have)
	{
		CString value;
		pVB->FindVariable(m_name)->Get(value);
		node->setAttr(m_name, value);
	}
}

/*
 * CAGStateFactory
 */

CAGStateFactory2::CAGStateFactory2( CAnimationGraph2 * pGraph, const IAnimationGraphStateFactoryIterator::SStateFactory& p )
{
	m_pCategory = pGraph->FindCategory( p.category );
	assert(m_pCategory);
	m_name = p.name;
	m_humanName = p.humanName;

	typedef IAnimationGraphStateFactoryIterator::SStateFactoryParameter Param;
	for (const Param * pParam = p.pParams; pParam && pParam->type; ++pParam)
		m_params.push_back( CAGStateFactoryParam2::Create(*pParam) );
}

void CAGStateFactory2::FillVarBlock( CVarBlockPtr pVB )
{
	for (std::vector<CAGStateFactoryParam2Ptr>::const_iterator iter = m_params.begin(); iter != m_params.end(); ++iter)
	{
		(*iter)->AddToVarBlock( pVB );
	}
}

void CAGStateFactory2::Load( XmlNodeRef node, CVarBlockPtr pVB )
{
	for (std::vector<CAGStateFactoryParam2Ptr>::const_iterator iter = m_params.begin(); iter != m_params.end(); ++iter)
	{
		(*iter)->Load( node, pVB );
	}
}

XmlNodeRef CAGStateFactory2::ToXml( CVarBlockPtr pVB )
{
	XmlNodeRef node = CreateXmlNode( m_name );

	for (std::vector<CAGStateFactoryParam2Ptr>::const_iterator iter = m_params.begin(); iter != m_params.end(); ++iter)
	{
		(*iter)->Save( node, pVB );
	}

	return node;
}

/*
 * CAGInput
 */

CAGInput2::CAGInput2( CAnimationGraph2 * pGraph ) : m_pGraph(pGraph), m_type(eAGIT_String), m_floatMin(0), m_floatMax(0), m_intMin(0), m_intMax(0), m_signalled(false), m_priority(1), m_attachToBlendWeight("-1"), m_blendWeightMoveSpeed(0.0f), m_forceGuard(false), m_mixinFilter(false)
{
}

CAGInput2::CAGInput2( CAnimationGraph2 * pGraph, const CString& name ) : m_pGraph(pGraph), m_type(eAGIT_String), m_floatMin(0), m_floatMax(0), m_intMin(0), m_intMax(0), m_name(name), m_priority(1), m_signalled(false), m_attachToBlendWeight("-1"), m_blendWeightMoveSpeed(0.0f), m_forceGuard(false), m_mixinFilter(false)
{
}

bool CAGInput2::Load( XmlNodeRef node )
{
	const char * tag = node->getTag();
	m_name = node->getAttr("name");
	node->getAttr("signalled", m_signalled);
	node->getAttr("priority", m_priority);
	m_forceGuard = false;
	node->getAttr("forceGuard", m_forceGuard);
	m_mixinFilter = false;
	node->getAttr("mixinFilter", m_mixinFilter);

	m_defaultValue = node->getAttr("defaultValue");
	m_attachToBlendWeight = "-1";
	node->getAttr("attachToBlendWeight", m_attachToBlendWeight);
	m_blendWeightMoveSpeed = 0.0f;
	node->getAttr("blendWeightMoveSpeed", m_blendWeightMoveSpeed);

	if (0 == strcmp(tag, "IntegerState"))
	{
		m_type = eAGIT_Integer;
		if (!node->getAttr("min", m_intMin) || !node->getAttr("max", m_intMax))
			CryLogAlways("Integer state %s without min/max", (const char *)m_name);
	}
	else if (0 == strcmp(tag, "FloatState"))
	{
		m_type = eAGIT_Float;
		if (!node->getAttr("min", m_floatMin) || !node->getAttr("max", m_floatMax))
			CryLogAlways("Float state %s without min/max", (const char *)m_name);
	}
	else if (0 == strcmp(tag, "KeyState"))
	{
		m_type = eAGIT_String;
		const int nChildren = node->getChildCount();
		for (int i=0; i<nChildren; i++)
		{
			m_keyValues.insert( node->getChild(i)->getAttr("value") );
		}
	}
	else
	{
		CryLogAlways("Unknown state type %s for %s ", tag, (const char *)m_name);
		return false;
	}
	return true;
}

XmlNodeRef CAGInput2::ToXml()
{
	XmlNodeRef node = CreateXmlNode("");

	node->setAttr("name", m_name);
	node->setAttr("signalled", m_signalled);
	node->setAttr("priority", m_priority);
	node->setAttr("attachToBlendWeight", m_attachToBlendWeight);
	node->setAttr("blendWeightMoveSpeed", m_blendWeightMoveSpeed);
	node->setAttr("forceGuard", m_forceGuard);
	node->setAttr("mixinFilter", m_mixinFilter);
	if (!m_defaultValue.IsEmpty())
		node->setAttr("defaultValue", m_defaultValue);

	const char * nodeName = 0;
	switch (m_type)
	{
	case eAGIT_Float:
		node->setTag("FloatState");
		node->setAttr("min", m_floatMin);
		node->setAttr("max", m_floatMax);
		break;
	case eAGIT_Integer:
		node->setTag("IntegerState");
		node->setAttr("min", m_intMin);
		node->setAttr("max", m_intMax);
		break;
	case eAGIT_String:
		node->setTag("KeyState");
		for (std::set<CString>::const_iterator iter = m_keyValues.begin(); iter != m_keyValues.end(); ++iter)
		{
			XmlNodeRef value = node->createNode("Key");
			value->setAttr("value", *iter);
			node->addChild(value);
		}
		break;
	default:
		assert(false);
		return XmlNodeRef();
	}
	return node;
}

TAGPropMap CAGInput2::GetGeneralProperties()
{
	TAGPropMap pm;
	CVarBlockPtr pVB = new CVarBlock();
	pm[""] = pVB;
	VariableHelper( pVB, "Name", m_name )->AddOnSetCallback( CNotifyCallback<CAGInput2>(this, &CAGInput2::SetName) );
	VariableHelper( pVB, "Signalled", m_signalled );
	VariableHelper( pVB, "DefaultValue", m_defaultValue );
	VariableHelper( pVB, "Guarded", m_forceGuard );
	//VariableHelper( pVB, "Mixin Filter", m_mixinFilter );
	VariableHelper( pVB, "Priority", m_priority );
	//VariableHelper( pVB, "AttachToBlendWeight", m_attachToBlendWeight );
	//VariableHelper( pVB, "BlendWeightMoveSpeed", m_blendWeightMoveSpeed );

	_smart_ptr<CVariableEnum<int> > pType = new CVariableEnum<int>();
	pType->SetHumanName( "Type" );
	pType->AddEnumItem( "Key", eAGIT_String );
	pType->AddEnumItem( "Integer", eAGIT_Integer );
	pType->AddEnumItem( "Float", eAGIT_Float );
	pType->Set( int(m_type) );
	pType->AddOnSetCallback( functor(*this, &CAGInput2::SetType) );
	pVB->AddVariable(pType);

	return pm;
}

TAGPropMap CAGInput2::GetFloatProperties()
{
	TAGPropMap pm;
	CVarBlockPtr pVB = new CVarBlock();
	pm[""] = pVB;

	VariableHelper( pVB, "Minimum", m_floatMin );
	VariableHelper( pVB, "Maximum", m_floatMax );

	return pm;
}

TAGPropMap CAGInput2::GetIntegerProperties()
{
	TAGPropMap pm;
	CVarBlockPtr pVB = new CVarBlock();
	pm[""] = pVB;

	VariableHelper( pVB, "Minimum", m_intMin );
	VariableHelper( pVB, "Maximum", m_intMax );

	return pm;
}


#include <iterator> //Diesel new

std::vector<CString> CAGInput2::GetKeyProperties()
{
	std::vector<CString> output;

//Diesel cut
	//std::copy( m_keyValues.begin(), m_keyValues.end(), back_inserter(output) );
//Diesel new	
	std::copy(m_keyValues.begin(), m_keyValues.end(), std::back_inserter(output)  );

	
	return output;
}

void CAGInput2::SetName()
{
	m_name = m_pGraph->OnChangedInputName( this );
}

void CAGInput2::SetType( IVariable * pVar )
{
	int value = 0xc64;
	pVar->Get(value);
	switch (value)
	{
	case eAGIT_String:
	case eAGIT_Float:
	case eAGIT_Integer:
		m_type = static_cast<EAnimationGraphInputType>(value);
		m_pGraph->SendInputEvent( eAGIE_ChangeType2, this );
		break;
	default:
		assert(false);
		pVar->Set( int(m_type) );
	}
}

/*
 * CAGStateTemplate
 */

bool CAGStateTemplate2::Load( XmlNodeRef node )
{
	int n = node->getChildCount();
	m_name = node->getAttr("name");
	m_extend = node->getAttr("extend");
	for (int i=0; i<n; i++)
	{
		XmlNodeRef child = node->getChild(i);
		CString tag = child->getTag();
		if (tag == "Param")
		{
			m_params.insert( std::make_pair( child->getAttr("name"), child->getAttr("type") ) );
		}
		else if (tag == "SelectWhen")
		{
/*
			for (int j=0; j<child->getChildCount(); j++)
				m_selected.insert( child->getChild(j)->getTag() );
*/
		}
	}
	return true;
}

bool CAGStateTemplate2::ProcessParents( const CAnimationGraph2* pGraph )
{
	int pos = 0;
	CString single = m_extend.Tokenize( ",", pos );
	while ( !single.IsEmpty() )
	{
		CAGStateTemplate2* pParent = pGraph->GetTemplate( single );
		if ( pParent && pParent->ProcessParents(pGraph) )
		{
			// inherit params
			std::map<CString, CString>::iterator itParam, itParamEnd = pParent->m_params.end();
			for ( itParam = pParent->m_params.begin(); itParam != itParamEnd; ++itParam )
				m_params.insert( *itParam );

			// inherit select when
/*
			std::set<CString>::iterator itSelect, itSelectEnd = pParent->m_selected.end();
			for ( itSelect = pParent->m_selected.begin(); itSelect != itSelectEnd; ++itSelect )
				m_selected.insert( *itSelect );
*/
		}
		single = m_extend.Tokenize( ",", pos );
	}
	m_extend.Empty();
	return true;
}

/*
 * CAGState
 */

CAGState2::CAGState2( CAnimationGraph2 * pGraph ) : 
	m_pGraph(pGraph), 
	m_bNoCollider(false),
	m_cost(100),
	m_turnMul(3.0f),
	m_iconSnapshotTime(0.5f),
	m_pActiveParameterization(NULL),
	m_bMixedExcludeFromGraph(false)
{
	m_pTemplate = m_pGraph->GetDefaultTemplate();

	// set default values from graph
	m_generalNodeDetails = *(m_pGraph->GetAnimationNodeDefaultDetails());
	m_graphNodeDetails = *(m_pGraph->GetNodeDefaultDetails());
	m_movementNodeDetails = *(m_pGraph->GetDefaultMovementDetails());
	m_animParamsPresetName = "Default";
}

CAGState2::CAGState2( CAnimationGraph2 * pGraph, const CString& name ) : 
	m_pGraph(pGraph), 
	m_name(name), 
	m_bNoCollider(false),
	m_cost(100),
	m_turnMul(3.0f),
	m_iconSnapshotTime(0.5f),
	m_pActiveParameterization(NULL),
	m_bMixedExcludeFromGraph(false)
{
	m_pTemplate = m_pGraph->GetDefaultTemplate();

	// set default values from graph
	m_generalNodeDetails = *(m_pGraph->GetAnimationNodeDefaultDetails());
	m_graphNodeDetails = *(m_pGraph->GetNodeDefaultDetails());
	m_movementNodeDetails = *(m_pGraph->GetDefaultMovementDetails());
	m_animParamsPresetName = "Default";
}

bool CAGState2::IsNullState() const
{
	return (m_pTemplate == m_pGraph->GetDefaultTemplate()) && m_extraStateFactories.empty();
}

bool CAGState2::HasAnimationSlotInTemplate() const
{
	// figure out if this state possibly has an animation
	if (IsNullState())
		return false;

//	if (m_name[0] == '+')
//		return true;

	const char * animationParamNames[] = {
		"animation",
		"animation1",
		"animation2",
		"anim",
		"play"
	};
	for (int i=0; i<sizeof(animationParamNames)/sizeof(*animationParamNames); i++)
	{
		if (m_pTemplate->HasParam(animationParamNames[i]))
		{
			const std::map<CString, CString>& m = m_pTemplate->GetParams();
			std::map<CString, CString>::const_iterator iter = m.find(animationParamNames[i]);
			if (iter != m.end())
				if (iter->second[0])
					return true;
		}
	}

	return false;
}


CString CAGState2::GetAnimationNameFromAG1Template( bool& foundName, const TParameterizationId2* pParamId) const
{
	CString retVal = "";

	const char * animationParamNames[] = {
		"animation",
		"animation1",
		"animation2",
		"anim",
		"play"
	};

	foundName = false;
	int count = sizeof(animationParamNames)/sizeof(*animationParamNames);
	for (int i = 0; i < count; ++i)
	{
		if (m_pTemplate->HasParam(animationParamNames[i]))
		{
			const std::map<CString, CString>& m = m_pTemplate->GetParams();
			std::map<CString, CString>::const_iterator iter = m.find(animationParamNames[i]);
			if (iter != m.end())
				if (iter->second[0])
				{
					if (HasTemplateParameter(iter->first, pParamId))
					{
						foundName = true;
						return GetTemplateParameter(iter->first, pParamId);
					}
					//return iter->second;
				}
		}
	}


	return retVal;
}


IAGCheckedOperation2Ptr CAGState2::CreateDeleteOp()
{
	_smart_ptr<CAGCompositeOperation2> pOp = new CAGCompositeOperation2();
	for (CAnimationGraph2::state_iterator iter = m_pGraph->StateBegin(); iter != m_pGraph->StateEnd(); ++iter)
	{
		CAGState2Ptr pState = *iter;
		if (pState == this)
			continue;
		if (pState->m_pExtend == this)
			pOp->AddOperation( new CAGCheckedOperation2<CAGState2>(pState, &CAGState2::NullifyParent, "Parent state for " + pState->GetName()) );
	}
	for (CAnimationGraph2::view_iterator iter = m_pGraph->ViewBegin(); iter != m_pGraph->ViewEnd(); ++iter)
	{
		CAGView2Ptr pView = *iter;
		if (!pView->CanAddState(this))
			pOp->AddOperation( new CAGCheckedOperation1_2<CAGView2, CAGState2Ptr>(pView, &CAGView2::RemoveState, this, "Exists in view " + pView->GetName()) );
	}
	pOp->AddOperation( new CAGCheckedOperation1_2<CAnimationGraph2, CAGState2Ptr>(m_pGraph, &CAnimationGraph2::RemoveState, this, "") );
	return &*pOp;
}

void CAGState2::GetParameterizations( std::vector< TParameterizationId2 >& paramIds )
{
	paramIds.clear();

	CAGState2::CartesianProductHelper productHelper;
	CParamsDeclaration2 *pDecl = GetParamsDeclaration();
	CParamsDeclaration2::const_iterator itDecl;
	for ( itDecl = pDecl->begin(); itDecl != pDecl->end(); ++itDecl )
	{
		productHelper.sets.push_back( std::pair< CString, const TSetOfCString* >(itDecl->first, &itDecl->second) );
	}

	if ( !productHelper.sets.empty() )
	{
		productHelper.Make( paramIds, productHelper.sets.begin() );
	}
}

bool CAGState2::IsParameterizationExcluded( const TParameterizationId2* pParameterizationId ) 
{
	bool bRet = false;

	if ( pParameterizationId )
	{
		TParameterizationMap::iterator itParamsMap = m_paramsMap.find( *pParameterizationId );
		if ( itParamsMap != m_paramsMap.end() && itParamsMap->second.m_bExcludeFromGraph ) 
		{
			bRet = true;
		}
	}

	return bRet;
}

int CAGState2::GetExcludeFromGraph() const
{
	if ( m_bMixedExcludeFromGraph || !m_pActiveParameterization )
		return -1;
	return m_pActiveParameterization->m_bExcludeFromGraph ? 1 : 0;
}

void CAGState2::SetExcludeFromGraph( bool bExclude )
{
	assert( m_pActiveParameterization );
	m_bMixedExcludeFromGraph = false;
	m_pActiveParameterization->m_bExcludeFromGraph = bExclude;
}

void CAGState2::OptimizeParameterization()
{
	TParameterizationMap::iterator it = m_paramsMap.begin();
	while ( it != m_paramsMap.end() )
	{
		CParamStateOverride& paramOverride = it->second;

		TSelectWhenMap::iterator itSW = paramOverride.m_selectWhen.begin();
		while ( itSW != paramOverride.m_selectWhen.end() )
			if ( itSW->second.type == eCT_UseParent2 )
				paramOverride.m_selectWhen.erase( itSW++ );
			else
				++itSW;

		std::map< CString, CString >::iterator itTemplates = paramOverride.m_templateParams.begin();
		while ( itTemplates != paramOverride.m_templateParams.end() )
			if ( itTemplates->second == m_templateParams[ itTemplates->first ] )
				paramOverride.m_templateParams.erase( itTemplates++ );
			else
				++itTemplates;

		// AG 1.5
		// Remove values from the details that are the same as the default unparamed details
		if (paramOverride.m_nodeDetails.GetAnimationName() == m_unparameterizedDetails.GetAnimationName())
		{
			paramOverride.m_nodeDetails.SetDefault(true);
		}
		// end AG 1.5

		if ( !paramOverride.m_bExcludeFromGraph && paramOverride.m_selectWhen.empty() && paramOverride.m_templateParams.empty() && paramOverride.m_nodeDetails.IsDefault() )
			m_paramsMap.erase( it++ );
		else
			++it;
	}
}

void CAGState2::ActivateParameterization( const TParameterizationId2* pParameterizationId, const CString& viewName )
{
	if ( !m_vParameterizationIds.empty() )
	{
		// there is a parameterization with at least one wildcard activated currently/previously
		// make sure all changes are reflected in m_paramsMap

		bool bExcludeFromGraph = m_MultipleStateOverride.m_bExcludeFromGraph;

		for ( TVectorParamIds::iterator itIds = m_vParameterizationIds.begin(); itIds != m_vParameterizationIds.end(); ++itIds )
		{
			TParameterizationMap::iterator itParamsMap = m_paramsMap.find( *itIds );
			if ( itParamsMap != m_paramsMap.end() )
			{
				// that id is already in the map
				CParamStateOverride& paramOverride = itParamsMap->second;

				if ( !m_bMixedExcludeFromGraph )
					paramOverride.m_bExcludeFromGraph = bExcludeFromGraph;

				std::map< CString, CString >::iterator itTemplates = m_MultipleStateOverride.m_templateParams.begin();
				for ( ; itTemplates != m_MultipleStateOverride.m_templateParams.end(); ++itTemplates )
				{
					if ( itTemplates->second != DIFFERENT_VALUES_STRING )
					{
						paramOverride.m_templateParams[ itTemplates->first ] = itTemplates->second;
					}
				}

				// AG 1.5
				if ( m_MultipleStateOverride.m_nodeDetails.GetAnimationName() != DIFFERENT_VALUES_STRING && paramOverride.m_nodeDetails.IsDefault() )
					paramOverride.m_nodeDetails.SetAnimationName(m_MultipleStateOverride.m_nodeDetails.GetAnimationName(), true);
				// end AG 1.5

				for ( TSelectWhenMap::iterator itSelectWhen = paramOverride.m_selectWhen.begin();
					itSelectWhen != paramOverride.m_selectWhen.end(); ++itSelectWhen )
				{
					ECriteriaType2 currentType = itSelectWhen->second.type;
					ECriteriaType2 multipleType = eCT_UseParent2;
					TSelectWhenMap::iterator swFind = m_MultipleStateOverride.m_selectWhen.find( itSelectWhen->first );
					if ( swFind != m_MultipleStateOverride.m_selectWhen.end() )
						multipleType = swFind->second.type;
					if ( multipleType != eCT_DifferentValues2 )
					{
						if ( swFind != m_MultipleStateOverride.m_selectWhen.end() )
							itSelectWhen->second = swFind->second;
						else
							itSelectWhen->second.type = eCT_UseParent2;
					}
				}

				for ( TSelectWhenMap::iterator itSelectWhen = m_MultipleStateOverride.m_selectWhen.begin();
					itSelectWhen != m_MultipleStateOverride.m_selectWhen.end(); ++itSelectWhen )
				{
					if ( itSelectWhen->second.type != eCT_DifferentValues2 )
						paramOverride.m_selectWhen[ itSelectWhen->first ] = itSelectWhen->second;
				}
			}
			else
			{
				// that id isn't yet in the map so it will be added here
				CParamStateOverride& paramOverride = m_paramsMap[ *itIds ];

				paramOverride.m_bExcludeFromGraph = m_bMixedExcludeFromGraph ? false : bExcludeFromGraph;

				std::map< CString, CString >::iterator itTemplates = m_MultipleStateOverride.m_templateParams.begin();
				for ( ; itTemplates != m_MultipleStateOverride.m_templateParams.end(); ++itTemplates )
					if ( itTemplates->second != DIFFERENT_VALUES_STRING )
						paramOverride.m_templateParams[ itTemplates->first ] = itTemplates->second;

				// AG 1.5
				// do this for all things inside the struct (should there ever be more than just the animation name)
				if ( m_MultipleStateOverride.m_nodeDetails.GetAnimationName() != DIFFERENT_VALUES_STRING )
					paramOverride.m_nodeDetails.SetAnimationName(m_MultipleStateOverride.m_nodeDetails.GetAnimationName(), true);
				// end AG 1.5

				for ( TSelectWhenMap::iterator itSelectWhen = m_MultipleStateOverride.m_selectWhen.begin();
					itSelectWhen != m_MultipleStateOverride.m_selectWhen.end(); ++itSelectWhen )
				{
					if ( itSelectWhen->second.type != eCT_DifferentValues2 )
						paramOverride.m_selectWhen[ itSelectWhen->first ] = itSelectWhen->second;
				}
			}
		}
		m_vParameterizationIds.clear();
	}

	OptimizeParameterization();

	m_activeViewName = viewName;
	m_bMixedExcludeFromGraph = false;

	m_pActiveParameterization = NULL;
	if ( pParameterizationId )
	{
		m_ActiveParameterizationId = *pParameterizationId;

		CartesianProductHelper productHelper;
		productHelper.defaults = *pParameterizationId;

		typedef std::vector< CParamsDeclaration2::const_iterator > TVectorOfParamIters;
		TVectorOfParamIters vParamIters;
		typedef std::vector< TSetOfCString::const_iterator > TVectorOfValueIters;
		TVectorOfValueIters vValueIters;
		int iCurrentParam = 0;
		CParamsDeclaration2::const_iterator itCurrentParam = m_paramsDeclaration.begin();
		for( TParameterizationId2::const_iterator it = pParameterizationId->begin(); it != pParameterizationId->end(); ++it )
		{
			if ( it->second.IsEmpty() )
			{
				CParamsDeclaration2::const_iterator itCurrentParam = m_paramsDeclaration.find( it->first );
				if ( itCurrentParam != m_paramsDeclaration.end() )
					productHelper.sets.push_back( std::pair< CString, const TSetOfCString* >(it->first, &itCurrentParam->second) );
			}
		}

		if ( !productHelper.sets.empty() )
		{
			// wildcard parameterization is activated (not a single state, but at least one param is on [All]
			productHelper.Make( m_vParameterizationIds, productHelper.sets.begin() );

			m_MultipleStateOverride.m_bExcludeFromGraph = false;
			m_MultipleStateOverride.m_selectWhen.clear();
			m_MultipleStateOverride.m_templateParams = m_templateParams;

			for ( TVectorParamIds::const_iterator itIds = m_vParameterizationIds.begin(); itIds != m_vParameterizationIds.end(); ++itIds )
			{
				TParameterizationMap::iterator itParamMap = m_paramsMap.find( *itIds );
				if ( itParamMap != m_paramsMap.end() )
				{
					CParamStateOverride& paramOverride = itParamMap->second;
					if ( itIds == m_vParameterizationIds.begin() )
					{
						m_MultipleStateOverride.m_bExcludeFromGraph = paramOverride.m_bExcludeFromGraph;
						m_MultipleStateOverride.m_selectWhen = paramOverride.m_selectWhen;

						std::map< CString, CString >::iterator itTemplates = paramOverride.m_templateParams.begin();
						for ( ; itTemplates != paramOverride.m_templateParams.end(); ++itTemplates )
							m_MultipleStateOverride.m_templateParams[ itTemplates->first ] = itTemplates->second;

						// AG 1.5
						m_MultipleStateOverride.m_nodeDetails = paramOverride.m_nodeDetails;
						// end AG 1.5

						continue;
					}

					if ( !m_bMixedExcludeFromGraph )
						if ( m_MultipleStateOverride.m_bExcludeFromGraph != paramOverride.m_bExcludeFromGraph )
							m_bMixedExcludeFromGraph = true;

					std::map< CString, CString >::iterator itTemplates = paramOverride.m_templateParams.begin();
					for ( ; itTemplates != paramOverride.m_templateParams.end(); ++itTemplates )
					{
						CString& value = m_MultipleStateOverride.m_templateParams[ itTemplates->first ];
						if ( value != DIFFERENT_VALUES_STRING )
							if ( itTemplates->second != value )
								value = DIFFERENT_VALUES_STRING;
					}

					// AG 1.5
					if ( m_MultipleStateOverride.m_nodeDetails.GetAnimationName() != DIFFERENT_VALUES_STRING )
						if ( paramOverride.m_nodeDetails.GetAnimationName() !=  m_MultipleStateOverride.m_nodeDetails.GetAnimationName() )
						{
							m_MultipleStateOverride.m_nodeDetails.SetAnimationName(DIFFERENT_VALUES_STRING);
						}
					// end of AG 1.5

					itTemplates = m_MultipleStateOverride.m_templateParams.begin();
					for ( ; itTemplates != m_MultipleStateOverride.m_templateParams.end(); ++itTemplates )
						if ( paramOverride.m_templateParams.find( itTemplates->first ) == paramOverride.m_templateParams.end() &&
							m_templateParams[ itTemplates->first ] != itTemplates->second )
								itTemplates->second = DIFFERENT_VALUES_STRING;

					TSelectWhenMap::iterator itSelectWhen = paramOverride.m_selectWhen.begin();
					for ( ; itSelectWhen != paramOverride.m_selectWhen.end(); ++itSelectWhen )
					{
						ECriteriaType2 currentType = itSelectWhen->second.type;
						ECriteriaType2 multipleType = eCT_UseParent2;
						TSelectWhenMap::iterator swFind = m_MultipleStateOverride.m_selectWhen.find( itSelectWhen->first );
						if ( swFind != m_MultipleStateOverride.m_selectWhen.end() )
							multipleType = swFind->second.type;
						if ( currentType != multipleType )
							m_MultipleStateOverride.m_selectWhen[ itSelectWhen->first ].type = eCT_DifferentValues2;
						else
						{
							switch ( multipleType )
							{
							case eCT_UseParent2:
							case eCT_AnyValue2:
								break;
							case eCT_ExactValue2:
								switch ( itSelectWhen->first->GetType() )
								{
								case eAGIT_Float:
									if ( itSelectWhen->second.floatValue != swFind->second.floatValue )
										m_MultipleStateOverride.m_selectWhen[ itSelectWhen->first ].type = eCT_DifferentValues2;
									break;
								case eAGIT_Integer:
									if ( itSelectWhen->second.intValue != swFind->second.intValue )
										m_MultipleStateOverride.m_selectWhen[ itSelectWhen->first ].type = eCT_DifferentValues2;
									break;
								case eAGIT_String:
									if ( itSelectWhen->second.strValue != swFind->second.strValue )
										m_MultipleStateOverride.m_selectWhen[ itSelectWhen->first ].type = eCT_DifferentValues2;
									break;
								default:
									assert(false);
								}
								break;
							case eCT_MinMax2:
								switch ( itSelectWhen->first->GetType() )
								{
								case eAGIT_Float:
									if ( itSelectWhen->second.floatRange.minimum != swFind->second.floatRange.minimum ||
										itSelectWhen->second.floatRange.maximum != swFind->second.floatRange.maximum )
											m_MultipleStateOverride.m_selectWhen[ itSelectWhen->first ].type = eCT_DifferentValues2;
									break;
								case eAGIT_Integer:
									if ( itSelectWhen->second.intRange.minimum != swFind->second.intRange.minimum ||
										itSelectWhen->second.intRange.maximum != swFind->second.intRange.maximum )
											m_MultipleStateOverride.m_selectWhen[ itSelectWhen->first ].type = eCT_DifferentValues2;
									break;
								case eAGIT_String:
									break;
								default:
									assert(false);
								}
								break;
							default:
								assert(!"add an entry here!");
							}
						}
					}

					itSelectWhen = m_MultipleStateOverride.m_selectWhen.begin();
					for ( ; itSelectWhen != m_MultipleStateOverride.m_selectWhen.end(); ++itSelectWhen )
					{
						ECriteriaType2 multipleType = itSelectWhen->second.type;
						ECriteriaType2 currentType = eCT_UseParent2;
						TSelectWhenMap::iterator swFind = paramOverride.m_selectWhen.find( itSelectWhen->first );
						if ( swFind != paramOverride.m_selectWhen.end() )
							currentType = swFind->second.type;
						if ( currentType != multipleType )
							itSelectWhen->second.type = eCT_DifferentValues2;
						else
						{
							switch ( multipleType )
							{
							case eCT_UseParent2:
							case eCT_AnyValue2:
								break;
							case eCT_ExactValue2:
								switch ( itSelectWhen->first->GetType() )
								{
								case eAGIT_Float:
									if ( itSelectWhen->second.floatValue != swFind->second.floatValue )
										itSelectWhen->second.type = eCT_DifferentValues2;
									break;
								case eAGIT_Integer:
									if ( itSelectWhen->second.intValue != swFind->second.intValue )
										itSelectWhen->second.type = eCT_DifferentValues2;
									break;
								case eAGIT_String:
									if ( itSelectWhen->second.strValue != swFind->second.strValue )
										itSelectWhen->second.type = eCT_DifferentValues2;
									break;
								default:
									assert(false);
								}
								break;
							case eCT_MinMax2:
								switch ( itSelectWhen->first->GetType() )
								{
								case eAGIT_Float:
									if ( itSelectWhen->second.floatRange.minimum != swFind->second.floatRange.minimum ||
										itSelectWhen->second.floatRange.maximum != swFind->second.floatRange.maximum )
											itSelectWhen->second.type = eCT_DifferentValues2;
									break;
								case eAGIT_Integer:
									if ( itSelectWhen->second.intRange.minimum != swFind->second.intRange.minimum ||
										itSelectWhen->second.intRange.maximum != swFind->second.intRange.maximum )
											itSelectWhen->second.type = eCT_DifferentValues2;
									break;
								case eAGIT_String:
									break;
								default:
									assert(false);
								}
								break;
							default:
								assert(!"add an entry here!");
							}
						}
					}
				}
				else
				{
					if ( !m_bMixedExcludeFromGraph )
						if ( m_MultipleStateOverride.m_bExcludeFromGraph )
							m_bMixedExcludeFromGraph = true;

					std::map< CString, CString >::iterator itTemplates = m_templateParams.begin();
					for ( ; itTemplates != m_templateParams.end(); ++itTemplates )
					{
						CString& value = m_MultipleStateOverride.m_templateParams[ itTemplates->first ];
						if ( value != DIFFERENT_VALUES_STRING )
							if ( itTemplates->second != value )
								value = DIFFERENT_VALUES_STRING;
					}

					if ( m_MultipleStateOverride.m_nodeDetails.GetAnimationName() != DIFFERENT_VALUES_STRING )
					{
						m_MultipleStateOverride.m_nodeDetails.SetAnimationName(DIFFERENT_VALUES_STRING);
					}

					TSelectWhenMap::iterator itSelectWhen = m_MultipleStateOverride.m_selectWhen.begin();
					for ( ; itSelectWhen != m_MultipleStateOverride.m_selectWhen.end(); ++itSelectWhen )
					{
						ECriteriaType2 multipleType = itSelectWhen->second.type;
						if ( multipleType != eCT_UseParent2 )
							itSelectWhen->second.type = eCT_DifferentValues2;
					}
				}
			}

			m_pActiveParameterization = m_vParameterizationIds.empty() ? NULL : &m_MultipleStateOverride;
		}
		else
		{
			// one single state is activated (no wildcard parameters)
			m_pActiveParameterization = &m_paramsMap[ *pParameterizationId ];

			// copy non-overridden template params (later they will be removed in OptimizeParameterization if not changed)
			std::map< CString, CString >::iterator itTemplates = m_templateParams.begin();
			for ( ; itTemplates != m_templateParams.end(); ++itTemplates )
				if ( m_pActiveParameterization->m_templateParams.find( itTemplates->first ) == m_pActiveParameterization->m_templateParams.end() )
					m_pActiveParameterization->m_templateParams[ itTemplates->first ] = itTemplates->second;

			// Remove values from the details that are the same as the default unparam'ed details
			if (m_pActiveParameterization->m_nodeDetails.IsDefault())
			{
				m_pActiveParameterization->m_nodeDetails.SetAnimationName(m_unparameterizedDetails.GetAnimationName(), true);
			}

		}
	}
	else
		m_ActiveParameterizationId.clear();
}

bool CAGState2::Load_SelectWhen( XmlNodeRef node )
{
	return Load_SelectWhen(m_selectWhen, node);
}

bool CAGState2::Load_SelectWhen( TSelectWhenMap& selectWhenMap, XmlNodeRef node ) const
{
	const int nCriteria = node->getChildCount();
	for (int j=0; j<nCriteria; j++)
	{
		XmlNodeRef criteria = node->getChild(j);
		CAGInput2Ptr pInput;
		SCriteria2 c;
		if (!(pInput = m_pGraph->FindInput(criteria->getTag())))
		{
			CryLogAlways("Unable to find input %s", criteria->getTag());
			continue;
		}
		if (criteria->haveAttr("min") && criteria->haveAttr("max"))
		{
			c.type = eCT_MinMax2;
			switch (pInput->GetType())
			{
			case eAGIT_Float:
				{
					//if (!criteria->getAttr("min", c.floatRange.minimum) || !criteria->getAttr("max", c.floatRange.maximum))
						//return false;
					if (!GetParamFromTemplate(criteria, "min", c.floatRange.minimum) || !GetParamFromTemplate(criteria, "max", c.floatRange.maximum))
						continue;
				}
				break;
			case eAGIT_Integer:
				//if (!criteria->getAttr("min", c.intRange.minimum) || !criteria->getAttr("max", c.intRange.maximum))
					//return false;
				if (!GetParamFromTemplate(criteria, "min", c.intRange.minimum) || !GetParamFromTemplate(criteria, "max", c.intRange.maximum))
					continue;
				break;
			case eAGIT_String:
				return false;
			default:
				assert(false);
				return false;
			}
		}
		else if (criteria->haveAttr("value"))
		{
			c.type = eCT_ExactValue2;
			switch (pInput->GetType())
			{
			case eAGIT_Float:
				//if (!criteria->getAttr("value", c.floatValue))
					//return false;
				if (!GetParamFromTemplate(criteria, "value", c.floatValue))
					continue;
				break;
			case eAGIT_Integer:
				//if (!criteria->getAttr("value", c.intValue))
					//return false;
				if (!GetParamFromTemplate(criteria, "value", c.intValue))
					continue;
				break;
			case eAGIT_String:
				//c.strValue = criteria->getAttr("value");
					//return false;
				GetParamFromTemplate(criteria, "value", c.strValue);
				break;
			default:
				assert(false);
				return false;
			}
		}
		else
		{
			c.type = eCT_AnyValue2;
		}
		selectWhenMap.insert( std::make_pair(pInput, c) );
	}
	return true;
}

bool CAGState2::Load( XmlNodeRef node )
{
//	int iTemp;
//	bool bTemp;

	if (node->haveAttr("extend"))
	{
		m_pExtend = m_pGraph->FindState(node->getAttr("extend"));
		if (!m_pExtend)
			CryLogAlways( "Unable to find extend node: %s", node->getAttr("extend") );
	}
	m_graphNodeDetails.m_allowSelect = true;
		node->getAttr("allowSelect", m_graphNodeDetails.m_allowSelect);

	m_graphNodeDetails.m_includeInGame = false;
		node->getAttr("includeInGame", m_graphNodeDetails.m_includeInGame);

	m_graphNodeDetails.m_canMix = false;
		node->getAttr("canMix", m_graphNodeDetails.m_canMix);

	// AG 1.5: Create AnimcontrolledCamera Modifier here in case this attribute is true
	bool bAnimationControlledView = false;
	node->getAttr("animationControlledView", bAnimationControlledView);
	if (bAnimationControlledView)
	{
		CAG2ModifierBasePtr newModifier = GetGraph()->AddNewModifier("AnimControlledCamera");
		AddLinkedModifier(newModifier->GetId());
	}

	m_iconSnapshotTime = 0.5f;
	node->getAttr("snapshotTime", m_iconSnapshotTime);

	m_animParamsPresetName = "<Custom>";
	if (node->haveAttr("AnimNodePreset"))
		node->getAttr("AnimNodePreset", m_animParamsPresetName);

	m_graphNodeDetails.m_bHurryable = false;
	node->getAttr("hurryable", m_graphNodeDetails.m_bHurryable);

	m_bNoCollider = false;
	if (!node->getAttr("NoCollider", m_bNoCollider))
			node->getAttr("noCylinder", m_bNoCollider);

	m_turnMul = 3.0f;
	node->getAttr("additionalTurnMultiplier", m_turnMul);

	m_cost = 100;
	node->getAttr("cost", m_cost);
	m_name = node->getAttr("id");
	m_pTemplate = m_pGraph->GetDefaultTemplate();

	m_graphNodeDetails.m_bSkipFP = false;
	node->getAttr("skipFirstPerson", m_graphNodeDetails.m_bSkipFP);

	m_mixins.clear();

	m_pTemplate = m_pGraph->GetDefaultTemplate();
	if (XmlNodeRef templateNode = node->findChild("Template"))
	{
		m_pTemplate = m_pGraph->GetTemplate( templateNode->getAttr("name") );
		for (int i=0; i<templateNode->getNumAttributes(); i++)
		{
			const char * key, * value;
			templateNode->getAttributeByIndex(i, &key, &value);
			if ( strcmp("name",key) )
				m_templateParams[key] = value;
		}
	}

	// AG 1.5
	// load general node details
	XmlNodeRef detailsNode = node->findChild("NodeDetails");
	if (detailsNode)
		m_generalNodeDetails.Load( detailsNode );
	else
		m_generalNodeDetails.CreateFromOldGraphData( this );

	// load movement node details
	XmlNodeRef movementNode = node->findChild("MovementAndCollider");
	if (movementNode)
		m_movementNodeDetails.Load( movementNode );
	else
		m_movementNodeDetails.CreateFromOldGraphData( this );

	// load linked modifiers
	XmlNodeRef modifiersNode = node->findChild("LinkedModifiers");
	if (modifiersNode)
		LoadLinkedModifiers( modifiersNode );
	else
	{
		// Extract from templates and directly from file... 
		// Go through all childs of this state and check whether it is a modifier
		// (at least if this is an older version file)
		for (int i = 0; i < node->getChildCount(); ++i)
		{
			XmlNodeRef stateNode = node->getChild(i);
			TryLoadingModifier(stateNode, true);
		}

		// Add the modifiers from this state's parent
		if (m_pExtend)
		{
			int modCount = m_pExtend->GetLinkedModifierCount();
			for (int i = 0; i < modCount; ++i)
			{
				uint modId = m_pExtend->GetLinkedModifier(i);
				if (m_pExtend->IsLinkedModifierInheritable(modId))
				{
					AddLinkedModifier(modId, true);
				}
			}
		}

		// ExtractNodesFromTemplates function is used for Modifier extraction as well (for the ones hidden in templates)!
	}

	// load parameterized node details
	XmlNodeRef parameterizedDetailsNode = node->findChild("ParameterizedData");
	if (parameterizedDetailsNode)
		m_unparameterizedDetails.Load( parameterizedDetailsNode );
	else
		m_unparameterizedDetails.CreateFromOldGraphData( this, NULL );

	// Convert from old graph data to new data (in case this is old graph format)
	if (!ExtractNodesFromTemplates())
		return false; 

	// end AG 1.5

	const int nChildren = node->getChildCount();
	for (int i=0; i<nChildren; i++)
	{
		XmlNodeRef child = node->getChild(i);
		const char * tag = child->getTag();
		if (0 == strcmp(tag, "MixIn"))
		{
			m_mixins.insert( child->getAttr("id") );
			m_graphNodeDetails.m_canMix = true;
		}
		else if (0 == strcmp(tag, "SelectWhen"))
		{
			if ( !Load_SelectWhen(m_selectWhen, child) )
				return false;
		}
		else
		{
			/*
			// AG 1.5: State Extra Properties are obsolete - will be replaced by Modifiers
			//         Loading these in will cause problems with custom new Modifiers.
			CAGStateFactory2Ptr pStateFactory = m_pGraph->FindStateFactory( tag );
						if (!pStateFactory)
						{
							// skip
						}
						else if (!pStateFactory->GetCategory()->IsOverridable())
						{
							m_extraStateFactories.push_back(pStateFactory);
							m_extraStateFactories.back().Load(child);
						}
			*/
			
			/*
			else if (!m_bUseTemplate)
			{
				m_overridableStateFactories[pStateFactory->GetCategory()] = pStateFactory;
				m_overridableStateFactories[pStateFactory->GetCategory()].Load(child);
			}
			*/
		}
	}

	m_paramsDeclaration.clear();
	if (XmlNodeRef parameterizationNode = node->findChild( "Parameterization" ))
	{
		for ( int j = 0; j < parameterizationNode->getChildCount(); ++j )
		{
			XmlNodeRef child = parameterizationNode->getChild(j);
			const char * tag = child->getTag();
			if (0 == strcmp(tag, "Parameter"))
			{
				std::pair< CParamsDeclaration2::iterator, bool > result =
					m_paramsDeclaration.insert( CParamsDeclaration2::value_type(child->getAttr("id"),TSetOfCString()) );
				TSetOfCString& values = result.first->second;

				int valuesCount = child->getChildCount();
				while ( valuesCount-- )
				{
					XmlNodeRef valueNode = child->getChild(valuesCount);
					const char * tag = valueNode->getTag();
					if (0 == strcmp(tag, "Value"))
						values.insert( valueNode->getAttr("id") );
				}
			}
			else if (0 == strcmp(tag, "Override"))
			{
				TParameterizationId2 id;
				id.InitEmptyFromDeclaration( m_paramsDeclaration );
				if ( id.FromString( child->getAttr("id") ) == false )
				{
					CString msg;
					msg.Format( "Invalid id \"%s\" in Override node while loading state %s.\n\nIgnoring the node...", child->getAttr("id"), (const char*)m_name );
					AfxMessageBox( msg );
				}
				else
				{
					std::pair< TParameterizationMap::iterator, bool > result =
						m_paramsMap.insert( TParameterizationMap::value_type(id,CParamStateOverride()) );
					CParamStateOverride& paramOverride = result.first->second;

					// AG 1.5
					bool AG2_Params_Loaded = false;
					// end AG 1.5

					for ( int i = 0; i < child->getChildCount(); ++i )
					{
						XmlNodeRef childOfOverride = child->getChild(i);
						const char * tag = childOfOverride->getTag();
						if (0 == strcmp(tag, "ExcludeFromGraph"))
						{
							paramOverride.m_bExcludeFromGraph = true;
						}
						else if (0 == strcmp(tag, "SelectWhen"))
						{
							Load_SelectWhen( paramOverride.m_selectWhen, childOfOverride );
						}
						else if (0 == strcmp(tag, "Template"))
						{
							for (int i=0; i<childOfOverride->getNumAttributes(); i++)
							{
								const char * key, * value;
								childOfOverride->getAttributeByIndex(i, &key, &value);
								paramOverride.m_templateParams[key] = value;
							}
						}
						else if (0 == strcmp(tag, "ParameterizedData"))
						{
							AG2_Params_Loaded = true;
							paramOverride.m_nodeDetails.Load( childOfOverride );
						}
					}

					// AG 1.5
					// Does the ParameterizedData exist?
					if (!AG2_Params_Loaded)
					{
						// create it then
						paramOverride.m_nodeDetails.CreateFromOldGraphData( this, &id );
					}
					// end AG 1.5

				}
			}
		}
	}

	return true;
}

struct CompareGetFirstName
{
	template <class T>
	bool operator()( const T& a, const T& b ) const
	{
		return a.first->GetName() < b.first->GetName();
	}
};

bool CAGState2::SelectWhen_ToXml( TSelectWhenMap& selectWhenMap, XmlNodeRef parentNode ) const
{
	if (!selectWhenMap.empty())
	{
		XmlNodeRef selectWhen = parentNode->createNode("SelectWhen");

		std::vector< std::pair<CAGInput2Ptr, SCriteria2> > inputs;
		for (std::map<CAGInput2Ptr, SCriteria2>::const_iterator iter = selectWhenMap.begin(); iter != selectWhenMap.end(); ++iter)
			inputs.push_back(*iter);

		std::sort( inputs.begin(), inputs.end(), CompareGetFirstName() );

		for (std::vector< std::pair<CAGInput2Ptr, SCriteria2> >::const_iterator iter = inputs.begin(); iter != inputs.end(); ++iter)
		{
			XmlNodeRef crit = selectWhen->createNode( iter->first->GetName() );
			bool add = true;
			switch (iter->second.type)
			{
			case eCT_UseParent2:
				add = false;
				break;
			case eCT_MinMax2:
				switch (iter->first->GetType())
				{
				case eAGIT_Float:
					crit->setAttr("min", iter->second.floatRange.minimum);
					crit->setAttr("max", iter->second.floatRange.maximum);
					break;
				case eAGIT_Integer:
					crit->setAttr("min", iter->second.intRange.minimum);
					crit->setAttr("max", iter->second.intRange.maximum);
					break;
				case eAGIT_String:
					assert(false);
					return false;
					break;
				default:
					assert(false);
				}
				break;
			case eCT_ExactValue2:
				switch (iter->first->GetType())
				{
				case eAGIT_Float:
					crit->setAttr("value", iter->second.floatValue);
					break;
				case eAGIT_Integer:
					crit->setAttr("value", iter->second.intValue);
					break;
				case eAGIT_String:
					crit->setAttr("value", iter->second.strValue);
					break;
				default:
					assert(false);
					return false;
				}
				break;
			case eCT_AnyValue2:
				break;
			default:
				assert(!"Add criteria type here");
			}
			if (add)
				selectWhen->addChild(crit);
		}
		parentNode->addChild(selectWhen);
	}
	return true;
}

XmlNodeRef TPerParameterizationData::ToXml()
{
	XmlNodeRef node = CreateXmlNode("ParameterizedData");

	node->setAttr("animationName", m_animationName);

	return node;
}

bool TPerParameterizationData::Load( XmlNodeRef node )
{
	node->getAttr("animationName", m_animationName);
	m_isDefault = false;

	return true;
}

bool TPerParameterizationData::CreateFromOldGraphData( const CAGState2Ptr animNode, const TParameterizationId2* pParamId)
{
	// extract the animation name
	bool found = true;
	m_animationName = animNode->GetAnimationNameFromAG1Template(found, pParamId);
	if (found)
	{
		m_isDefault = false;
	}

	return true;
}

XmlNodeRef CAGState2::ToXml()
{
	XmlNodeRef node = CreateXmlNode("State");
	node->setAttr("id", m_name);
	node->setAttr("allowSelect", m_graphNodeDetails.m_allowSelect);
	node->setAttr("includeInGame", m_graphNodeDetails.m_includeInGame);
	node->setAttr("canMix", m_graphNodeDetails.m_canMix);
	node->setAttr("cost", m_cost);
	node->setAttr("MovementControlMethodH", (int)eMCM_Undefined);
	node->setAttr("MovementControlMethodV", (int)eMCM_Undefined);
	node->setAttr("hurryable", m_graphNodeDetails.m_bHurryable);
	node->setAttr("NoCollider", m_bNoCollider);
	node->setAttr("additionalTurnMultiplier", m_turnMul);
	node->setAttr("skipFirstPerson", m_graphNodeDetails.m_bSkipFP);
	node->setAttr("snapshotTime", m_iconSnapshotTime);
	node->setAttr("AnimNodePreset", m_animParamsPresetName);

	// AG 1.5: Check whether this state is linked to an AnimControlledFPCamera Modifier and save out
	SaveFPAnimControlledView(node);

	if (m_pExtend)
		node->setAttr("extend", m_pExtend->GetName());

	if ( !SelectWhen_ToXml(m_selectWhen, node) )
		return false;

	// AG 1.5
	// Make the general data into a struct and give it a save function
	XmlNodeRef detailsNode = node->createNode("NodeDetails");

	// Check for invalid layer index (which should be 1-based)
	CRY_ASSERT(m_generalNodeDetails.m_animLayer >= 1);
	if (m_generalNodeDetails.m_animLayer < 1)
		m_generalNodeDetails.m_animLayer = 1;

	m_generalNodeDetails.ToXML( detailsNode );
	node->addChild(detailsNode);
	node->addChild(m_unparameterizedDetails.ToXml());

	XmlNodeRef movementNode = node->createNode("MovementAndCollider");
	m_movementNodeDetails.ToXML( movementNode );
	node->addChild(movementNode);

	// Save and export all modifier ids that this state is linked to
	SaveLinkedModifiersToXML(node);
	ExportModifiers(node);

	// if the "all" option has an animation assigned to it, the data has
	// to be exported there as well.
	if (!m_unparameterizedDetails.GetAnimationName().IsEmpty())
	{
		node->addChild(m_generalNodeDetails.CreateTransitionLayerNode( this ));
		node->addChild(m_generalNodeDetails.CreateAnimationLayerNode(&m_unparameterizedDetails, &m_unparameterizedDetails));

		// only layer 1 can set the mcm and collider mode
		if (m_generalNodeDetails.m_animLayer == 1)
		{
			node->addChild(m_movementNodeDetails.CreateMovementCtrlNode( this ));
			node->addChild(m_movementNodeDetails.CreateColliderNode( this ));
		}
	}
	// end of AG 1.5

	for (std::vector<SStateFactoryInfo>::const_iterator iter = m_extraStateFactories.begin(); iter != m_extraStateFactories.end(); ++iter)
	{
		// Don't save out some of the raw nodes anymore in new AG - they are converted upon loading into proper modifiers
		// TODO: Ideally this whole entire block will go at some point, but as long as not all nodes are
		// converted into Modifiers, this is impossible.
		if (m_pGraph->IsObsoleteFactory(iter->pFactory->GetName()))
			continue;

		XmlNodeRef child = iter->pFactory->ToXml( iter->pVarBlock );
		if (!child)
			return XmlNodeRef();
		else
			node->addChild(child);
	}

	for (std::set<CString>::iterator iter = m_mixins.begin(); iter != m_mixins.end(); ++iter)
	{
		XmlNodeRef mixin = node->createNode("MixIn");
		mixin->setAttr("id", *iter);
		node->addChild(mixin);
	}

	if ( !m_paramsDeclaration.empty() )
	{
		OptimizeParameterization();

		XmlNodeRef parameterizationNode = node->createNode( "Parameterization" );
		
		for (CParamsDeclaration2::iterator it = m_paramsDeclaration.begin(); it != m_paramsDeclaration.end(); ++it )
		{
			XmlNodeRef parameterNode = parameterizationNode->createNode( "Parameter" );
			parameterNode->setAttr( "id", it->first );
			
			TSetOfCString& values = it->second;
			for (TSetOfCString::iterator itVals = values.begin(); itVals != values.end(); ++itVals )
			{
				XmlNodeRef valueNode = parameterNode->createNode( "Value" );
				valueNode->setAttr( "id", *itVals );
				parameterNode->addChild( valueNode );
			}
			parameterizationNode->addChild( parameterNode );
		}

		// Now come the parameterization overrides
		for ( TParameterizationMap::iterator it = m_paramsMap.begin(); it != m_paramsMap.end(); ++it )
		{
			CParamStateOverride& paramOverride = it->second;
			XmlNodeRef overrideNode = parameterizationNode->createNode( "Override" );
			overrideNode->setAttr( "id", it->first.AsString() );

			if ( paramOverride.m_bExcludeFromGraph )
			{
				XmlNodeRef excludeNode = overrideNode->createNode( "ExcludeFromGraph" );
				overrideNode->addChild( excludeNode );
			}

			SelectWhen_ToXml( paramOverride.m_selectWhen, overrideNode );

			// templates will no longer be used
			/*
			if ( !paramOverride.m_templateParams.empty() )
			{
				XmlNodeRef templateNode = overrideNode->createNode( "Template" );
				std::map< CString, CString >::const_iterator iter = paramOverride.m_templateParams.begin();
				for ( ; iter != paramOverride.m_templateParams.end(); ++iter )
					templateNode->setAttr( iter->first, iter->second );
				overrideNode->addChild( templateNode );
			}
			*/

			overrideNode->addChild(paramOverride.m_nodeDetails.ToXml());

			// write out transition params again because there might at one point be
			// parameterized data in them too...
			overrideNode->addChild(m_generalNodeDetails.CreateTransitionLayerNode( this ));

			overrideNode->addChild(m_generalNodeDetails.CreateAnimationLayerNode(&paramOverride.m_nodeDetails, &m_unparameterizedDetails));

			parameterizationNode->addChild( overrideNode );
		}

		node->addChild( parameterizationNode );
	}

	return node;
}

TAGPropMap CAGState2::GetGeneralProperties()
{
	CVarBlockPtr pVB = new CVarBlock();
	//SetGraph( VariableHelper( pVB, "State name", m_name, this, &CAGState2::OnChangeName ), m_pGraph );
	SetGraph( VariableHelper_StateSelector( pVB, "Parent state", m_pExtend, m_pGraph, this ), m_pGraph )->AddOnSetCallback( CNotifyCallback<CAGState2>( this, &CAGState2::OnChangeParent ) );
	SetGraph( VariableHelper( pVB, "Pathfind cost", m_cost ), m_pGraph );
//	SetGraph( VariableHelper( pVB, "Turn Speed Multiplier", m_turnMul ), m_pGraph );
//	SetGraph( VariableHelper( pVB, "Allow selection", m_graphNodeDetails.m_allowSelect ), m_pGraph )->AddOnSetCallback( CNotifyCallback<CAGState2>( this, &CAGState2::OnChangeParent ) );
//	SetGraph( VariableHelper( pVB, "Include in game", m_graphNodeDetails.m_includeInGame ), m_pGraph );
//	SetGraph( VariableHelper( pVB, "Use template", m_bUseTemplate ), m_pGraph )->AddOnSetCallback( CNotifyCallback<CAGState>( this, &CAGState::OnChangeUseTemplate ) );
//	SetGraph( VariableHelper( pVB, "Can mix", m_graphNodeDetails.m_canMix ), m_pGraph );
//	SetGraph( VariableHelper( pVB, "Can be hurried?", m_graphNodeDetails.m_bHurryable ), m_pGraph );
//	SetGraph( VariableHelper( pVB, "Skip for players?", m_graphNodeDetails.m_bSkipFP), m_pGraph );

	for (CAnimationGraph2::view_iterator iter = m_pGraph->ViewBegin(); iter != m_pGraph->ViewEnd(); ++iter)
	{
		CString name = (*iter)->GetName();
		if (name.IsEmpty() || name[0] != '+')
			continue;
		VariableHelper_SetInclusion( pVB, "Mix " + name, name, m_mixins, m_pGraph );
	}
	
	/*
	IVariablePtr pIconSnapshotTime = SetGraph( VariableHelper( pVB, "Icon Snapshot Time", m_iconSnapshotTime), m_pGraph);
	pIconSnapshotTime->SetLimits(0.0f, 1.0f);
	pIconSnapshotTime->AddOnSetCallback(CNotifyCallback<CAGState>(this, &CAGState::OnChangeIconSnapshotTime));
	*/

	TAGPropMap pm;
	pm[""] = pVB;
	return pm;
}

TAGPropMap CAGState2::GetCriteriaProperties( CAnimationGraphStateEditor2 * pSE )
{
	TAGPropMap pm;

	CVarBlockPtr pVBMain, pVBInherited, pVBUnused, pVBTemplate;

	for (CAnimationGraph2::input_iterator iterInputs = m_pGraph->InputBegin(); /*AllowSelect() &&*/ iterInputs != m_pGraph->InputEnd(); ++iterInputs)
	{
		CAGInput2Ptr pInput = *iterInputs;

		CVarBlockPtr pVB = new CVarBlock();
		SetGraph( VariableHelper_Criteria( pVB, pInput->GetName(), pInput, this, pSE ), m_pGraph );

		switch ( GetCriteriaType(pInput) )
		{
		case eCT_UseParent2:
			CAGState2* pParent;
			pParent = this;
			while ( pParent = pParent->GetParent() )
			{
				if ( pParent->GetCriteriaType( pInput ) != eCT_UseParent2 )
				{
					if ( !pVBInherited )
						pm["Inherited"] = pVBInherited = new CVarBlock();
					pm[ CString("Inherited/") + pInput->GetName() ] = pVB;
					break;
				}
			}
			if ( !pParent )
			{
				if ( !pVBUnused )
					pm["Unused"] = pVBUnused = new CVarBlock();
				pm[ CString("Unused/") + pInput->GetName() ] = pVB;
			}
			break;

		case eCT_DefinedInTemplate2:
			if ( !pVBTemplate )
				pm[" Defined in template"] = pVBTemplate = new CVarBlock();
			pm[ CString(" Defined in template/") + pInput->GetName() ] = pVB;
			break;

		default:
			if ( !pVBMain )
				pm[" "] = pVBMain = new CVarBlock();
			pm[ CString(" /") + pInput->GetName() ] = pVB;
		}
	}

	return pm;
}

TAGPropMap CAGState2::GetOverridableProperties( CAnimationGraphStateEditor2 * pSE )
{
	TAGPropMap pm;
	for (CAnimationGraph2::category_iterator iterCats = m_pGraph->CategoryBegin(); iterCats != m_pGraph->CategoryEnd(); ++iterCats)
	{
		CAGCategory2Ptr pCat = *iterCats;
		if (!pCat->IsOverridable())
			continue;

		CVarBlockPtr pVB = new CVarBlock();
		pm[pCat->GetName()] = pVB;

		SetGraph( VariableHelper_Overridable( pVB, "Implementation", pCat, this, pSE ), m_pGraph );
	}

	return pm;
}

TAGPropMap CAGState2::GetExtraProperties()
{
	CVarBlockPtr pVB = new CVarBlock();
	if (m_pExtend)
	{
		CVarBlockPtr pVBFrom = m_pExtend->GetExtraProperties()[""];
		const int n = pVBFrom->GetVarsCount();
		for (int i=0; i<n; i++)
		{
			IVariablePtr pVar = pVBFrom->GetVariable(i);
			if ((pVar->GetFlags() && IVariable::UI_DISABLED) == 0)
			{
				IVariablePtr pVar2 = new CVariable<CString>();
				pVar2->SetDataType( pVar->GetDataType() );
				pVar2->SetFlags( IVariable::UI_DISABLED );
				pVar2->SetHumanName( pVar->GetHumanName() );
				CString temp;
				pVar->Get( temp );
				pVar2->Set( temp );
				pVar = pVar2;
			}
			pVB->AddVariable( pVar );
		}
	}
	for (std::vector<SStateFactoryInfo>::const_iterator iter = m_extraStateFactories.begin(); iter != m_extraStateFactories.end(); ++iter)
	{
		ConstVariableHelper( pVB, "Type", iter->pFactory->GetName() );
		CVarBlockPtr pVBFrom = iter->pVarBlock;
		const int n = pVBFrom->GetVarsCount();
		for (int i=0; i<n; i++)
		{
			pVB->AddVariable( pVBFrom->GetVariable(i) );
		}
	}

	TAGPropMap pm;
	pm[""] = pVB;
	return pm;
}

TAGPropMap CAGState2::GetTemplateProperties( CAnimationGraphStateEditor2 * pSE )
{
	CVarBlockPtr pVB = new CVarBlock();

	SetGraph( VariableHelper_Template( pVB, "Template", m_pTemplate, this, pSE ), m_pGraph );

	TAGPropMap pm;
	pm[""] = pVB;
	return pm;
}

ECriteriaType2 CAGState2::GetCriteriaType( CAGInput2Ptr pInput ) const
{
	// Obsolete with AG 1.5
// 	if ( IsSelectionDefinedInTemplate(pInput) )
// 		return eCT_DefinedInTemplate2;

	const TSelectWhenMap& selectWhen = m_pActiveParameterization ? m_pActiveParameterization->m_selectWhen : m_selectWhen;
	std::map<CAGInput2Ptr, SCriteria2>::const_iterator iter = selectWhen.find(pInput);
	if ( iter == selectWhen.end() || iter->second.type == eCT_UseParent2 )
	{
		if ( m_pActiveParameterization == NULL )
			return eCT_UseParent2;
		iter = m_selectWhen.find(pInput);
		if ( iter == m_selectWhen.end() )
			return eCT_UseParent2;
		else if ( iter->second.type == eCT_UseParent2 )
			return eCT_UseParent2;
		else
			return eCT_UseDefault2;
	}
	else
		return iter->second.type;
}

void CAGState2::SetCriteriaType( CAGInput2Ptr pInput, ECriteriaType2 type )
{
	TSelectWhenMap& selectWhen = m_pActiveParameterization ? m_pActiveParameterization->m_selectWhen : m_selectWhen;
	selectWhen[pInput].type = type;
}

CVarBlockPtr CAGState2::CreateCriteriaVariables( CAGInput2Ptr pInput, bool enable, CAGState2::CParamStateOverride* pParameterization )
{
	TSelectWhenMap& selectWhen = pParameterization ? pParameterization->m_selectWhen : m_selectWhen;

	std::map<CAGInput2Ptr, SCriteria2>::iterator iter;
	iter = selectWhen.find(pInput);
	if (iter == selectWhen.end() || iter->second.type == eCT_UseParent2)
	{
		if (pParameterization)
			return CreateCriteriaVariables( pInput, false );
		else if (m_pExtend)
			return m_pExtend->CreateCriteriaVariables( pInput, false );
		else
			return new CVarBlock();
	}
	else
	{
		CVarBlockPtr ret = new CVarBlock();
		switch (iter->second.type)
		{
		case eCT_AnyValue2:
		case eCT_DifferentValues2:
			break;
		case eCT_ExactValue2:
			switch (pInput->GetType())
			{
			case eAGIT_Float:
				VariableHelper( ret, "Value", iter->second.floatValue );
				break;
			case eAGIT_Integer:
				VariableHelper( ret, "Value", iter->second.intValue );
				break;
			case eAGIT_String:
				{
					CCriteriaKeyValueVariable* var = new CCriteriaKeyValueVariable( ret, pInput, iter->second.strValue );
					var->SetHumanName("Value");
					var->SetFlags(0);
					ret->AddVariable(var);
				}
				break;
			default:
				assert(false);
				return CVarBlockPtr();
			}
			break;
		case eCT_MinMax2:
			switch (pInput->GetType())
			{
			case eAGIT_Float:
				VariableHelper( ret, "Minimum", iter->second.floatRange.minimum );
				VariableHelper( ret, "Maximum", iter->second.floatRange.maximum );
				break;
			case eAGIT_Integer:
				VariableHelper( ret, "Minimum", iter->second.intRange.minimum );
				VariableHelper( ret, "Maximum", iter->second.intRange.maximum );
				break;
			default:
				assert(false);
				return CVarBlockPtr();
			}
			break;
		default:
			assert(false);
			return CVarBlockPtr();
		}

		for (int i=0; i<ret->GetVarsCount(); i++)
		{
			if (!enable)
			{
				ret->GetVariable(i)->SetFlags( IVariable::UI_DISABLED );
				SetGraph( ret->GetVariable(i), m_pGraph );
			}
		}

		return ret;
	}
}

void CAGState2::SetTemplateType( const CString& type )
{
	CAGStateTemplate2Ptr newType = m_pGraph->GetTemplate(type);
	if (newType == m_pTemplate)
		return;

	m_pGraph->Modified();
	m_pTemplate = newType;
}

void CAGState2::SetStateFactoryMode( CAGCategory2Ptr pCategory, const CString& mode )
{
	if (mode == GetStateFactoryMode(pCategory))
		return;

	m_pGraph->Modified();

	m_overridableStateFactories.erase(pCategory);
	if (mode == "_parent")
		return;
	if (mode == "_none")
		m_overridableStateFactories[pCategory] = SStateFactoryInfo();
	else
		m_overridableStateFactories[pCategory] = m_pGraph->FindStateFactory( mode );
}

CString CAGState2::GetStateFactoryMode( CAGCategory2Ptr pCategory ) const
{
	std::map<CAGCategory2Ptr, SStateFactoryInfo>::const_iterator iter = m_overridableStateFactories.find(pCategory);
	if (iter == m_overridableStateFactories.end())
		return "_parent";
	else if (iter->second.pFactory)
		return iter->second.pFactory->GetName();
	else
		return "_none";
}

CVarBlockPtr CAGState2::GetStateFactoryVarBlock( CAGCategory2Ptr pCategory ) const
{
	std::map<CAGCategory2Ptr, SStateFactoryInfo>::const_iterator iter = m_overridableStateFactories.find(pCategory);
	assert( iter != m_overridableStateFactories.end() );
	assert( !!iter->second.pVarBlock );
	return iter->second.pVarBlock;
}

void CAGState2::OnChangeName()
{
	static bool ignore = false;
	if (ignore)
		return;
	ignore = true;

	CString temp = m_pGraph->OnChangedStateName(this);
	if ( m_name != temp )
	{
		AfxMessageBox( "There is already a state named '" + m_name + "'!\n\nTry again...", MB_OK|MB_ICONERROR );
		m_name = temp;
		m_pGraph->SendStateEvent( eAGSE_CanNotChangeName2, this );
	}

	ignore = false;
}

void CAGState2::OnChangeParent()
{
	m_pGraph->SendStateEvent( eAGSE_ChangeParent2, this );
}

void CAGState2::OnChangeUseTemplate()
{
	m_pGraph->SendStateEvent( eAGSE_ChangeUseTemplate2, this );
}

void CAGState2::OnChangeIconSnapshotTime()
{
	m_pGraph->SendStateEvent( eAGSE_ChangeIconSnapshotTime2, this );
}

void CAGState2::AddExtraState( CAGStateFactory2Ptr pSF )
{
	m_pGraph->Modified();
	m_extraStateFactories.push_back(pSF);
}

void CAGState2::RemoveExtraState( int i )
{
	m_pGraph->Modified();
	m_extraStateFactories.erase( m_extraStateFactories.begin()+i );
}

void CAGState2::GetExtraStateStrings( std::vector<CString>& items ) const
{
	for (std::vector<SStateFactoryInfo>::const_iterator iter = m_extraStateFactories.begin(); iter != m_extraStateFactories.end(); ++iter)
	{
		CString index;
		index.Format( "%d %s", (iter - m_extraStateFactories.begin()), (const char*)iter->pFactory->GetHumanName() );
		items.push_back( index );
	}
}

bool CAGState2::AddParamValue( const char* param, const char* value )
{
	CParamsDeclaration2::iterator declaration = m_paramsDeclaration.find( param );
	if ( declaration == m_paramsDeclaration.end() )
		return false;
	TSetOfCString& values = declaration->second;

	ActivateParameterization( NULL );
	if ( values.insert( value ).second == false )
		return false;

	if ( values.size() == 1 )
	{
		// for the first parameter value just keep all the existing entries in m_paramsMap (with modified id's)

		TParameterizationMap old;
		old.swap( m_paramsMap );
		for ( TParameterizationMap::iterator it = old.begin(), itEnd = old.end(); it != itEnd; ++it )
		{
			TParameterizationId2 paramId( it->first );

			TParameterizationId2::iterator itParam = paramId.find( param );
			assert( itParam != paramId.end() );
			assert( itParam->second.IsEmpty() );
			if ( itParam->second.IsEmpty() )
				itParam->second = value;
			m_paramsMap.insert( TParameterizationMap::value_type(paramId, it->second) );
		}
		return true;
	}

	if ( values.size() == 2 )
	{
		// for the second parameter value just duplicate all the existing entries in m_paramsMap (with modified id's)

		TParameterizationMap old;
		old.swap( m_paramsMap );
		for ( TParameterizationMap::iterator it = old.begin(), itEnd = old.end(); it != itEnd; ++it )
		{
			TParameterizationId2 paramId( it->first );
			m_paramsMap.insert( TParameterizationMap::value_type(paramId, it->second) );

			TParameterizationId2::iterator itParam = paramId.find( param );
			assert( itParam != paramId.end() );
			itParam->second = value;
			m_paramsMap.insert( TParameterizationMap::value_type(paramId, it->second) );
		}
		return true;
	}

	// there were two or more parameter values before adding the new one - do the complex merging here

	TSetOfCString::const_iterator itValues = values.begin(), itValuesEnd = values.end();

	TParameterizationMap old;
	old.swap( m_paramsMap );
	TParameterizationMap::iterator itParamMap;
	while ( (itParamMap = old.begin()) != old.end() )
	{
		TParameterizationId2 paramId( itParamMap->first );
		CParamStateOverride overrides( itParamMap->second );
		old.erase(itParamMap);

		// keep existing entries unmodified
		m_paramsMap.insert( TParameterizationMap::value_type(paramId, overrides) );

		TParameterizationId2::iterator itParam = paramId.find( param );
		CString firstValue( itParam->second );
		assert( itParam != paramId.end() );

		for ( itValues = values.begin(); itValues != itValuesEnd; ++itValues )
		{
			if ( *itValues != value && *itValues != firstValue )
			{
				CParamStateOverride currentOverrides;
				{
					// this block prevents accessing itOverride after erase()
					itParam->second = *itValues;
					TParameterizationMap::iterator itOverride = old.find( paramId );
					if ( itOverride == old.end() )
					{
						overrides.m_bExcludeFromGraph = false;
						overrides.m_selectWhen.clear();
						overrides.m_templateParams.clear();
						continue;
					}
					currentOverrides = itOverride->second;

					// keep existing entries unmodified
					m_paramsMap.insert( *itOverride );
					old.erase( itOverride );
				}

				// merge 'Exclude from graph' flag
				if ( overrides.m_bExcludeFromGraph != currentOverrides.m_bExcludeFromGraph )
					overrides.m_bExcludeFromGraph = false;

				// merge selection criteria
				TSelectWhenMap::iterator itSelectWhen = overrides.m_selectWhen.begin();
				while ( itSelectWhen != overrides.m_selectWhen.end() )
				{
					TSelectWhenMap::const_iterator findSW = currentOverrides.m_selectWhen.find( itSelectWhen->first );
					if ( findSW == currentOverrides.m_selectWhen.end() || itSelectWhen->second != findSW->second )
						overrides.m_selectWhen.erase( itSelectWhen++ );
					else
						++itSelectWhen;
				}

				// merge template parameters
				std::map< CString, CString >::iterator itTemplateParam = overrides.m_templateParams.begin();
				while ( itTemplateParam != overrides.m_templateParams.end() )
				{
					std::map< CString, CString >::const_iterator findTP = currentOverrides.m_templateParams.find( itTemplateParam->first );
					if ( findTP == currentOverrides.m_templateParams.end() || itTemplateParam->second != findTP->second )
						overrides.m_templateParams.erase( itTemplateParam++ );
					else
						++itTemplateParam;
				}
			}
		}

		// add the entry if needed
		if ( overrides.m_bExcludeFromGraph || !overrides.m_selectWhen.empty() || !overrides.m_templateParams.empty() )
		{
			itParam->second = value;
			m_paramsMap.insert( TParameterizationMap::value_type(paramId, overrides) );
		}
	}

	return true;
}

bool CAGState2::DeleteParamValue( const char* param, const char* value )
{
	CParamsDeclaration2::iterator declaration = m_paramsDeclaration.find( param );
	if ( declaration == m_paramsDeclaration.end() )
		return false;
	TSetOfCString& values = declaration->second;

	ActivateParameterization( NULL );
	TSetOfCString::iterator itValue = values.find( value );
	if ( itValue == values.end() )
		return false;
	values.erase( itValue );

	// Update link constraints

	CString paramLinkName('[');
	paramLinkName += param;
	paramLinkName += ']';
	paramLinkName.MakeLower();

	std::vector< CAGLink2Ptr > links;
	m_pGraph->StateLinks( links, this, true, true );
	for ( std::vector<CAGLink2Ptr>::iterator itLinks = links.begin(), itLinksEnd = links.end(); itLinks != itLinksEnd; ++itLinks )
	{
		CAGLink2* pLink = *itLinks;
		CAGLink2::TListMappedParams& mapping = pLink->m_listMappedParams;
		CAGLink2::TListMappedParams::iterator itMapping = mapping.begin();

		CAGState2Ptr from, to;
		pLink->GetLinkedStates( from, to );
		if ( this == from )
		{
			while ( itMapping != mapping.end() )
			{
				if ( itMapping->first == paramLinkName && itMapping->second == param )
				{
					// TODO: Ask for removing the link here, and remove the link instead of removing the link constrain
					mapping.erase( itMapping++ );
				}
				else
					++itMapping;
			}
		}
		else
		{
			while ( itMapping != mapping.end() )
			{
				if ( itMapping->second == paramLinkName && itMapping->first == param )
				{
					// TODO: Ask for removing the link here, and remove the link instead of removing the link constrain
					mapping.erase( itMapping++ );
				}
				else
					++itMapping;
			}
		}
	}

	if ( values.size() == 0 )
	{
		// for the last parameter value just keep all the existing entries in m_paramsMap (with modified id's)

		TParameterizationMap old;
		old.swap( m_paramsMap );
		for ( TParameterizationMap::iterator it = old.begin(), itEnd = old.end(); it != itEnd; ++it )
		{
			TParameterizationId2 paramId( it->first );
			TParameterizationId2::iterator itParam = paramId.find( param );
			assert( itParam != paramId.end() );
			itParam->second.Empty();
			m_paramsMap.insert( TParameterizationMap::value_type(paramId, it->second) );
		}
		return true;
	}

	// if this is not the last parameter value just remove the entries in m_paramsMap if id refers to the removed value

	TParameterizationMap::iterator it = m_paramsMap.begin();
	while ( it != m_paramsMap.end() )
	{
		TParameterizationId2 paramId( it->first );
		TParameterizationId2::iterator itParam = paramId.find( param );
		assert( itParam != paramId.end() );
		if ( itParam->second == value )
			m_paramsMap.erase( it++ );
		else
			++it;
	}
	return true;
}

bool CAGState2::RenameParamValue( const char* param, const char* oldValue, const char* newValue )
{
	if ( strcmp(oldValue, newValue) == 0 )
		return false;

	ActivateParameterization( NULL );
	CParamsDeclaration2::iterator itDeclaration = m_paramsDeclaration.find( param );
	if ( itDeclaration == m_paramsDeclaration.end() )
		return false;

	TSetOfCString& values = itDeclaration->second;
	{
		TSetOfCString::iterator itValue = values.find( oldValue );
		if ( itValue == values.end() )
			return false;
		if ( values.insert(newValue).second == false )
			return false;
		values.erase( itValue );
	}

	TParameterizationMap old;
	old.swap( m_paramsMap );
	for ( TParameterizationMap::iterator it = old.begin(), itEnd = old.end(); it != itEnd; ++it )
	{
		TParameterizationId2 paramId( it->first );
		TParameterizationId2::iterator itParam = paramId.find( param );
		assert( itParam != paramId.end() );
		if ( itParam->second == oldValue )
			itParam->second = newValue;
		m_paramsMap.insert( TParameterizationMap::value_type(paramId, it->second) );
	}

	// Update link constraints

	CString paramLinkName('[');
	paramLinkName += param;
	paramLinkName += ']';
	paramLinkName.MakeLower();

	std::vector< CAGLink2Ptr > links;
	m_pGraph->StateLinks( links, this, true, true );
	for ( std::vector<CAGLink2Ptr>::iterator itLinks = links.begin(), itLinksEnd = links.end(); itLinks != itLinksEnd; ++itLinks )
	{
		CAGLink2* pLink = *itLinks;
		CAGLink2::TListMappedParams& mapping = pLink->m_listMappedParams;

		CAGState2Ptr from, to;
		pLink->GetLinkedStates( from, to );
		if ( this == from )
		{
			for ( CAGLink2::TListMappedParams::iterator itMapping = mapping.begin(); itMapping != mapping.end(); ++itMapping )
				if ( itMapping->first == paramLinkName && itMapping->second == oldValue )
					itMapping->second = newValue;
		}
		else
		{
			for ( CAGLink2::TListMappedParams::iterator itMapping = mapping.begin(); itMapping != mapping.end(); ++itMapping )
				if ( itMapping->second == paramLinkName && itMapping->first == oldValue )
					itMapping->first = newValue;
		}
	}

	return true;
}

bool CAGState2::AddParameter( const CString& name )
{
	ActivateParameterization( NULL );
	if ( m_paramsDeclaration.insert( TMapParams::value_type(name, TSetOfCString()) ).second == false )
		return false;

	TParameterizationMap old;
	old.swap( m_paramsMap );
	for ( TParameterizationMap::iterator it = old.begin(), itEnd = old.end(); it != itEnd; ++it )
	{
		TParameterizationId2 paramId( it->first );
		paramId.insert( TParameterizationId2::value_type(name, CString()) );
		m_paramsMap.insert( TParameterizationMap::value_type(paramId, it->second) );
	}
	return true;
}

bool CAGState2::DeleteParameter( const CString& name )
{
	ActivateParameterization( NULL );
	CParamsDeclaration2::iterator itDeclaration = m_paramsDeclaration.find( name );
	if ( itDeclaration == m_paramsDeclaration.end() )
		return false;
	TSetOfCString& values = itDeclaration->second;

	if ( values.size() <= 1 )
	{
		// simpler case - only remove the parameter from all id's

		TParameterizationMap old;
		old.swap( m_paramsMap );
		for ( TParameterizationMap::iterator it = old.begin(), itEnd = old.end(); it != itEnd; ++it )
		{
			TParameterizationId2 paramId( it->first );
			TParameterizationId2::iterator itParam = paramId.find( name );
			assert( itParam != paramId.end() );
			paramId.erase( itParam );
			m_paramsMap.insert( TParameterizationMap::value_type(paramId, it->second) );
		}

		// finally delete the parameter declaration
		m_paramsDeclaration.erase( itDeclaration );
		return true;
	}

	// there are two or more parameter values - do the complex merging here

	TSetOfCString::const_iterator itValues = values.begin(), itValuesEnd = values.end();

	TParameterizationMap old;
	old.swap( m_paramsMap );
	TParameterizationMap::iterator itParamMap;
	while ( (itParamMap = old.begin()) != old.end() )
	{
		TParameterizationId2 paramId( itParamMap->first );
		CParamStateOverride overrides( itParamMap->second );
		old.erase(itParamMap);

		TParameterizationId2::iterator itParam = paramId.find( name );
		CString firstValue( itParam->second );
		assert( itParam != paramId.end() );

		for ( itValues = values.begin(); itValues != itValuesEnd; ++itValues )
		{
			if ( *itValues != firstValue )
			{
				CParamStateOverride currentOverrides;
				{
					// this block prevents accessing itOverride after erase()
					itParam->second = *itValues;
					TParameterizationMap::iterator itOverride = old.find( paramId );
					if ( itOverride == old.end() )
					{
						overrides.m_bExcludeFromGraph = false;
						overrides.m_selectWhen.clear();
						overrides.m_templateParams.clear();
						continue;
					}
					currentOverrides = itOverride->second;
					old.erase( itOverride );
				}

				// merge 'Exclude from graph' flag
				if ( overrides.m_bExcludeFromGraph != currentOverrides.m_bExcludeFromGraph )
					overrides.m_bExcludeFromGraph = false;

				// merge selection criteria
				TSelectWhenMap::iterator itSelectWhen = overrides.m_selectWhen.begin();
				while ( itSelectWhen != overrides.m_selectWhen.end() )
				{
					TSelectWhenMap::const_iterator findSW = currentOverrides.m_selectWhen.find( itSelectWhen->first );
					if ( findSW == currentOverrides.m_selectWhen.end() || itSelectWhen->second != findSW->second )
						overrides.m_selectWhen.erase( itSelectWhen++ );
					else
						++itSelectWhen;
				}

				// merge template parameters
				std::map< CString, CString >::iterator itTemplateParam = overrides.m_templateParams.begin();
				while ( itTemplateParam != overrides.m_templateParams.end() )
				{
					std::map< CString, CString >::const_iterator findTP = currentOverrides.m_templateParams.find( itTemplateParam->first );
					if ( findTP == currentOverrides.m_templateParams.end() || itTemplateParam->second != findTP->second )
						overrides.m_templateParams.erase( itTemplateParam++ );
					else
						++itTemplateParam;
				}
			}
		}

		// add the entry if needed
		if ( overrides.m_bExcludeFromGraph || !overrides.m_selectWhen.empty() || !overrides.m_templateParams.empty() )
		{
			paramId.erase( itParam );
			m_paramsMap.insert( TParameterizationMap::value_type(paramId, overrides) );
		}
	}

	// finally delete the parameter declaration
	m_paramsDeclaration.erase( itDeclaration );

	// Update link constraints

	CString paramLinkName('[');
	paramLinkName += name;
	paramLinkName += ']';
	paramLinkName.MakeLower();

	std::vector< CAGLink2Ptr > links;
	m_pGraph->StateLinks( links, this, true, true );
	for ( std::vector<CAGLink2Ptr>::iterator itLinks = links.begin(), itLinksEnd = links.end(); itLinks != itLinksEnd; ++itLinks )
	{
		CAGLink2* pLink = *itLinks;
		CAGLink2::TListMappedParams& mapping = pLink->m_listMappedParams;
		CAGLink2::TListMappedParams::iterator itMapping = mapping.begin(); 

		CAGState2Ptr from, to;
		pLink->GetLinkedStates( from, to );
		if ( this == from )
		{
			while ( itMapping != mapping.end() )
			{
				if ( itMapping->first == paramLinkName )
					mapping.erase( itMapping++ );
				else
					++itMapping;
			}
		}
		else
		{
			while ( itMapping != mapping.end() )
			{
				if ( itMapping->second == paramLinkName )
					mapping.erase( itMapping++ );
				else
					++itMapping;
			}
		}
	}

	return true;
}

bool CAGState2::RenameParameter( const CString& oldName, const CString& newName )
{
	if ( oldName.CompareNoCase(newName) == 0 )
		return false;

	ActivateParameterization( NULL );
	CParamsDeclaration2::iterator itDeclaration = m_paramsDeclaration.find( oldName );
	if ( itDeclaration == m_paramsDeclaration.end() )
		return false;
	if ( m_paramsDeclaration.insert( CParamsDeclaration2::value_type(newName, itDeclaration->second) ).second == false )
		return false;
	m_paramsDeclaration.erase( itDeclaration );

	TParameterizationMap old;
	old.swap( m_paramsMap );
	for ( TParameterizationMap::iterator it = old.begin(), itEnd = old.end(); it != itEnd; ++it )
	{
		TParameterizationId2 paramId( it->first );
		TParameterizationId2::iterator itParam = paramId.find( oldName );
		assert( itParam != paramId.end() );
		paramId.insert( TParameterizationId2::value_type(newName, itParam->second) );
		paramId.erase( itParam );
		m_paramsMap.insert( TParameterizationMap::value_type(paramId, it->second) );
	}

	// Update link constraints

	CString paramLinkOldName('[');
	paramLinkOldName += oldName;
	paramLinkOldName += ']';
	paramLinkOldName.MakeLower();

	CString paramLinkNewName('[');
	paramLinkNewName += newName;
	paramLinkNewName += ']';
	paramLinkNewName.MakeLower();

	std::vector< CAGLink2Ptr > links;
	m_pGraph->StateLinks( links, this, true, true );
	for ( std::vector<CAGLink2Ptr>::iterator itLinks = links.begin(), itLinksEnd = links.end(); itLinks != itLinksEnd; ++itLinks )
	{
		CAGLink2* pLink = *itLinks;
		CAGLink2::TListMappedParams& mapping = pLink->m_listMappedParams;

		CAGState2Ptr from, to;
		pLink->GetLinkedStates( from, to );
		if ( this == from )
		{
			for ( CAGLink2::TListMappedParams::iterator itMapping = mapping.begin(); itMapping != mapping.end(); ++itMapping )
				if ( itMapping->first == paramLinkOldName )
					itMapping->first = paramLinkNewName;
		}
		else
		{
			for ( CAGLink2::TListMappedParams::iterator itMapping = mapping.begin(); itMapping != mapping.end(); ++itMapping )
				if ( itMapping->second == paramLinkOldName )
					itMapping->second = paramLinkNewName;
		}
	}

	return true;
}

CString CAGState2::GetAnimationName() const
{
	if (m_pActiveParameterization)
		return m_pActiveParameterization->m_nodeDetails.GetAnimationName();

	return m_unparameterizedDetails.GetAnimationName();
}

void CAGState2::SetName( CString newName )
{
	m_name = newName; 
	OnChangeName();
}

bool CAGState2::ExtractNodesFromTemplates()
{
	string path = "Libs/AnimationGraphTemplates/";
	char filename[_MAX_PATH];
	strcpy( filename, path.c_str() );
	strcat( filename, GetTemplateType() );
	strcat( filename, ".xml" );

	XmlNodeRef rootFile = GetISystem()->LoadXmlFile( filename );

	// Process and convert the template's data into AG 1.5 nodes and data
	while (rootFile)
	{
		XmlNodeRef tempNode;

		// First step: Load the SelectWhen out of the template and merge it into the
		// already existing selectWhens
		tempNode = rootFile->findChild("SelectWhen");
		if (tempNode)
		{
			if (!Load_SelectWhen(m_selectWhen, tempNode))
				return false;
		}

		// Extract other nodes below here
		// ==================================

		// Modifier Nodes construction out of template nodes
		// Go through all the nodes in the template
		for (int i = 0; i < rootFile->getChildCount(); ++i)
		{
			XmlNodeRef node = rootFile->getChild(i);
			TryLoadingModifier(node, false);
		}

		// ==================================
		// end of template processing


		// Load parent if existing and run the same stuff again
		string extend = rootFile->getAttr("extend");
		strcpy( filename, path.c_str() );
		strcat( filename, extend );
		strcat( filename, ".xml" );
		rootFile = GetISystem()->LoadXmlFile( filename );
	}

	return true;
}

/*
 * CAGLink
 */

CAGLink2::CAGLink2( CAnimationGraph2 * pGraph ) : m_pGraph(pGraph), m_forceFollowChance(0), m_cost(100), m_overrideTransitionTime(0.0f)
{
}

bool CAGLink2::Load( XmlNodeRef node )
{
	if (!node->getAttr("forceFollowChance", m_forceFollowChance))
		m_forceFollowChance = 0;
	m_cost = 100;
	node->getAttr("cost", m_cost);
	m_overrideTransitionTime = 0.0f;
	node->getAttr("transitionTime", m_overrideTransitionTime);

	m_listMappedParams.clear();
	for ( int i = 0; i < node->getChildCount(); ++i )
	{
		XmlNodeRef child = node->getChild(i);
		if ( !strcmp("Mapping",child->getTag()) )
			m_listMappedParams.push_back(std::make_pair( CString(child->getAttr("from")), CString(child->getAttr("to")) ));
	}
	return true;
}

XmlNodeRef CAGLink2::ToXml()
{
	XmlNodeRef pNode = CreateXmlNode("Link");
	pNode->setAttr("forceFollowChance", m_forceFollowChance);
	pNode->setAttr("cost", m_cost);
	pNode->setAttr("transitionTime", m_overrideTransitionTime);

	TListMappedParams::iterator it, itEnd = m_listMappedParams.end();
	for ( it = m_listMappedParams.begin(); it != itEnd; ++it )
	{
		XmlNodeRef pChild = CreateXmlNode("Mapping");
		pChild->setAttr("from", it->first);
		pChild->setAttr("to", it->second);
		pNode->addChild(pChild);
	}
	return pNode;
}

TAGPropMap CAGLink2::GetGeneralProperties()
{
	TAGPropMap pm;
	CVarBlockPtr pVB = new CVarBlock();
	pm[""] = pVB;

	VariableHelper( pVB, "Force follow chance", m_forceFollowChance );
	VariableHelper( pVB, "Pathfind cost", m_cost );
	VariableHelper( pVB, "Override transition time", m_overrideTransitionTime );

	return pm;
}

TAGPropMap CAGLink2::GetMappingProperties()
{
	TAGPropMap pm;
	CVarBlockPtr pVB = new CVarBlock();
	pm[""] = pVB;

	CAGState2Ptr pFromState, pToState;
	GetLinkedStates( pFromState, pToState );
	CLinkParamsVariable* pFromVar = VariableHelper_ParamMapping( pVB, this, pFromState, pToState, true );
	CLinkParamsVariable* pToVar = VariableHelper_ParamMapping( pVB, this, pToState, pFromState, false );

	return pm;
}

void CAGLink2::GetLinkedStates( CAGState2Ptr& fromState, CAGState2Ptr& toState ) const
{
	m_pGraph->FindLinkedStates( this, fromState, toState );
}

void CAGLink2::MappingChanged()
{
	m_pGraph->SendLinkEvent( eAGLE_ChangeMapping2, this );
}

/*
 * CAGView
 */

CAGView2::CAGView2( CAnimationGraph2 * pGraph ) : m_pGraph(pGraph)
{
}

CAGView2::CAGView2( CAnimationGraph2 * pGraph, const CString& name ) : m_pGraph(pGraph), m_name(name)
{
}

CAGView2::~CAGView2()
{
	if (m_pLastCreatedGraph != 0)
		m_pLastCreatedGraph->RemoveListener(this);
}

void CAGView2::OnHyperGraphEvent( IHyperNode * pNode, EHyperGraphEvent event )
{
	CAGNodeBase2* pAGNode = (CAGNode2*)pNode;
	bool updatePos = false;
	bool makeConnections = false;

	if (!pAGNode || pAGNode->GetAGNodeType() == AG_NODE_MAIN2)
	{
		switch (event)
		{
		case EHG_NODE_ADD:
			if (m_states.find(pAGNode->GetState()) == m_states.end())
				m_pGraph->Modified();
			m_states[pAGNode->GetState()].hid = pAGNode->GetId();
			CreateNodesForState(pAGNode->GetState(), static_cast<CAGNode2*>(pAGNode), pAGNode->IsSelected());
			makeConnections = true;
			updatePos = true;
			break;
		case EHG_NODE_CHANGE:
			{
				updatePos = true;
				PositionNodesForState(pAGNode->GetState(), static_cast<CAGNode2*>(pAGNode));
			}
			break;
		case EHG_NODE_DELETE:
			m_pGraph->Modified();
			m_states.erase( pAGNode->GetState() );
			break;
		}
	}
	else if (pAGNode && pAGNode->GetAGNodeType() == AG_NODE_MODIFIER)
	{
		CAG2ModifierNode* pModifierNode = (CAG2ModifierNode*)pAGNode;
		makeConnections = false;
		updatePos = false;

		switch (event)
		{
		case EHG_NODE_ADD:
			{
				m_pGraph->Modified();
				CAG2ModifierBasePtr pModifier = pModifierNode->GetModifier();
				m_modifiers[pModifier].hid = pModifierNode->GetId();
				UpdateNodePosition(pModifierNode);
				MakeConnectionsInView(pModifierNode);
				break;
			}
		case EHG_NODE_CHANGE:
			{
				m_pGraph->Modified();
				UpdateNodePosition(pModifierNode);
				break;
			}
		case EHG_NODE_DELETE:
			{
				RemoveModifier(pModifierNode->GetModifier());
				m_pGraph->Modified();
				break;
			}
		}
	}

	if (updatePos)
	{
		Gdiplus::PointF& pos = m_states[pAGNode->GetState()].pos;
		if (!pos.Equals(pAGNode->GetPos()))
		{
			pos = pAGNode->GetPos();
			m_pGraph->Modified();
		}
	}

	if (makeConnections)
	{
		// Create links to the other states
		CAGState2Ptr pLookFor = pAGNode->GetState();
		CHyperGraph * pGraph = (CHyperGraph*) pAGNode->GetGraph();

		typedef std::vector<CAGState2Ptr> States;
		States states;
		for (StateMap::const_iterator iter = m_states.begin(); iter != m_states.end(); ++iter)
		{
			if (iter->second.hid == HyperNodeID(-1))
				continue;

			for (StateMap::const_iterator iter2 = m_states.begin(); iter2 != m_states.end(); ++iter2)
			{
				if (iter2->second.hid == HyperNodeID(-1))
					continue;

				if (iter->first != pLookFor && iter2->first != pLookFor)
					continue;

				if (m_pGraph->IsLinked(iter->first, iter2->first))
				{
					CHyperNode * from = (CHyperNode*) pGraph->FindNode( iter->second.hid );
					CHyperNode * to = (CHyperNode*) pGraph->FindNode( iter2->second.hid );
					pGraph->ConnectPorts( from, from->FindPort("out", false), to, to->FindPort("in", true), false );
				}
			}
		}

		// Finally also create links to the Modifiers
		MakeConnectionsFromStateToModifiersInView(pAGNode);
	}
}

void CAGView2::RemoveState( CAGState2Ptr pState )
{
	m_states.erase(pState);
}

void CAGView2::RemoveModifier( CAG2ModifierBasePtr pModifier )
{
	m_modifiers.erase(pModifier);
}

bool CAGView2::Load( XmlNodeRef node, float agVersion /*= 1.0f*/ )
{

	// testing
	//SStateInfo modifierInfo;

	m_name = node->getAttr("name");
	int nChildren = node->getChildCount();
	for (int i=0; i<nChildren; i++)
	{
		XmlNodeRef state = node->getChild(i);
		CString nodeTag = state->getTag();
		if (nodeTag.Compare("State") == 0)
		{
			CAGState2Ptr pState = m_pGraph->FindState( state->getAttr("id") );
			if (!pState)
				continue;
			SStateInfo stateInfo;
			if (!state->getAttr("x", stateInfo.pos.X) || !state->getAttr("y", stateInfo.pos.Y))
			{
				CryLogAlways("Unable to get coordinates for view state %s in %s - will skip this node.", pState->GetName(), m_name );
				continue;
			}
			m_states[pState] = stateInfo;

			// TODO: In case this is an old version graph, 
			// automatically place all linked modifiers of a state in the view
			if (agVersion < 1.5f)
			{
				int modCount = pState->GetLinkedModifierCount();
				Gdiplus::PointF modPos = stateInfo.pos + Gdiplus::PointF(-300, -100);
				for ( int i = 0; i < modCount; ++i)
				{
					// Go through all of this state's linked modifiers
					// if the modifier can be added to this view (because it isn't already in)
					// add it to a place a little offset to the state
					CAG2ModifierBasePtr pModifier = m_pGraph->GetModifier(pState->GetLinkedModifier(i));

					if (CanAddModifier(pModifier))
					{			
						// Add it into the list
						SStateInfo modifierInfo;
						modifierInfo.pos = modPos;
						m_modifiers[pModifier] = modifierInfo;
						modPos = modPos + Gdiplus::PointF(0, 75);
					}
				}
			}
		}
		else if (nodeTag.Compare("Modifier") == 0)
		{
			// Get the modifier that is referenced
			int modifierId = -1;
			state->getAttr("Id", modifierId);
			CAG2ModifierBasePtr pModifier = m_pGraph->GetModifier(modifierId);

			if (pModifier == NULL)
			{
				CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, "Trying to place modifier with id: %i in view, but no such modifier exists. Skipping.", modifierId);
				continue;
			}

			// load the position info
			SStateInfo modifierInfo;
			if (!state->getAttr("x", modifierInfo.pos.X) || !state->getAttr("y", modifierInfo.pos.Y))
			{
				CryLogAlways("Unable to get coordinates for view modifier %s with ID %i in %s - will skip this node.", pModifier->GetClassName(), pModifier->GetId(), m_name );
				continue;
			}

			// Add it into the list
			m_modifiers[pModifier] = modifierInfo;
		}
		else
		{
			CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, "Unknown view entry: %s - will be skipped.", nodeTag);
			continue;
		}


	}

	return true;
}

struct PairCompareFirst
{
	template <class T>
	bool operator()( const T& a, const T& b ) const
	{
		return a.first < b.first;
	}
};

XmlNodeRef CAGView2::ToXml()
{
	XmlNodeRef node = CreateXmlNode("View");
	node->setAttr("name", m_name);

	// Save out all states that are placed in this view with positions
	std::vector< std::pair<CString, SStateInfo> > stateInfo;
	for (StateMap::const_iterator iter = m_states.begin(); iter != m_states.end(); ++iter)
	{
		stateInfo.push_back( std::make_pair(iter->first->GetName(), iter->second) );
	}

	std::sort(stateInfo.begin(), stateInfo.end(), PairCompareFirst());

	for (std::vector< std::pair<CString, SStateInfo> >::const_iterator iter = stateInfo.begin(); iter != stateInfo.end(); ++iter)
	{
		XmlNodeRef child = CreateXmlNode("State");
		child->setAttr("id", iter->first);
		child->setAttr("x", iter->second.pos.X);
		child->setAttr("y", iter->second.pos.Y);
		node->addChild(child);
	}

	// Save out all modifiers that are placed in this view with positions
	ModifierMap::const_iterator modEndIt = m_modifiers.end();
	for (ModifierMap::const_iterator modIt = m_modifiers.begin(); modIt != modEndIt; ++modIt)
	{
		XmlNodeRef child = CreateXmlNode("Modifier");
		child->setAttr("id", modIt->first->GetId());
		child->setAttr("x", modIt->second.pos.X);
		child->setAttr("y", modIt->second.pos.Y);
		node->addChild(child);
	}

	return node;
}

_smart_ptr<CHyperGraph> CAGView2::GetHyperGraph()
{
	for (StateMap::iterator iter = m_states.begin(); iter != m_states.end(); ++iter)
	{
		iter->second.hid = HyperNodeID(-1);
	}

	if (m_pLastCreatedGraph)
		m_pLastCreatedGraph->RemoveListener(this);
	m_pLastCreatedGraph = m_pGraph->CreateGraph();
	m_pLastCreatedGraph->AddListener(this);
	CryLogAlways("CAGView::GetHyperGraph 0x%p new pGraph=0x%p m_pGraph=0x%p", this, (CHyperGraph*)m_pLastCreatedGraph, m_pGraph);

	// Create all the animation nodes in this view
	for (StateMap::iterator iter = m_states.begin(); iter != m_states.end(); ++iter)
	{
		Gdiplus::PointF pos = iter->second.pos;

		CAGState2Ptr pState = iter->first;
		CAGNode2 * pNode = (CAGNode2*) m_pLastCreatedGraph->CreateNode( pState->GetName(), pos );
		if (pNode)
			m_states[pState].hid = pNode->GetId();
		//pNode->SetPos(pos);

		//CreateNodesForState(iter->first, pNode);
	}

	// Create all the modifier nodes in this view
	for (ModifierMap::iterator iter = m_modifiers.begin(); iter != m_modifiers.end(); ++iter)
	{
		Gdiplus::PointF pos = iter->second.pos;

		CAG2ModifierBasePtr pModifier = iter->first;
		CString nodeName;
		nodeName.Format("$modifier %i", pModifier->GetId());
		CAGNodeBase2* pNode = (CAGNodeBase2*) m_pLastCreatedGraph->CreateNode( nodeName, pos );
		if (pNode)
			m_modifiers[pModifier].hid = pNode->GetId();
		//pNode->SetPos(pos);

		//CreateNodesForState(iter->first, pNode);
	}

	return m_pLastCreatedGraph;
}

void CAGView2::CreateNodesForState(CAGState2Ptr pState, CAGNode2 * pNode, bool bSelected)
{
	if (m_pGraph->IsShowingIcons())
	{
		CAnimationImage2Ptr image = m_pGraph->GetImageManager()->GetImage(m_pGraph, pState);
		if (image)
		{
			CAGImageNode2* pImageNode = (CAGImageNode2*) m_pLastCreatedGraph->CreateNode( CString("$image ") + pState->GetName() );
			pImageNode->SetImage(image);
			if (bSelected)
				pImageNode->SetSelected(true);

			pNode->AttachImageNode(pImageNode);
		}

		PositionNodesForState(pState, pNode);
	}
}

void CAGView2::PositionNodesForState(CAGState2Ptr pState, CAGNode2 * pNode)
{
	IHyperGraphEnumerator* pEnum = static_cast<CAGNode2*>(pNode)->GetChildrenEnumerator();
	for (IHyperNode* pChild = pEnum->GetFirst(); pChild; pChild = pEnum->GetNext())
	{
		CAnimationImage2Ptr image = m_pGraph->GetImageManager()->GetImage(m_pGraph, pState);
		float width = 0.0f;
		if (image)
			width = image->GetImage()->GetWidth();
		static_cast<CHyperNode*>(pChild)->SetPos(pNode->GetPos() - Gdiplus::PointF(width + 8.0f, 0.0f));
	}
	pEnum->Release();
}

bool CAGView2::CanAddState( CAGState2Ptr ptr )
{
	return !!ptr && m_states.find(ptr) == m_states.end();
}

bool CAGView2::HasState( CAGState2Ptr ptr )
{
	return m_states.find(ptr) != m_states.end();
}

bool CAGView2::HasModifier( CAG2ModifierBasePtr ptr )
{
	return m_modifiers.find(ptr) != m_modifiers.end();
}

bool CAGView2::CanAddModifier( CAG2ModifierBasePtr ptr )
{
	return (!!ptr) && (!HasModifier(ptr));
}

HyperNodeID CAGView2::GetNodeIdForModifier(CAG2ModifierBasePtr pState)
{
	HyperNodeID hid = HyperNodeID(-1);
	ModifierMap::iterator iMod = m_modifiers.find(pState);
	if (iMod != m_modifiers.end())
	{
		hid = iMod->second.hid;
	}

	return hid;
}

CAGNode2* CAGView2::GetNodeForState(CAGState2Ptr pState)
{
	CAGNode2* pNode = 0;
	StateMap::iterator itStateEntry = m_states.find(pState);
	HyperNodeID hid = HyperNodeID(-1);
	if (itStateEntry != m_states.end())
		hid = (*itStateEntry).second.hid;
	if (hid != -1)
		pNode = (CAGNode2*)m_pLastCreatedGraph->FindNode(hid);
	return pNode;
}

TAGPropMap CAGView2::GetGeneralProperties()
{
	TAGPropMap pm;
	CVarBlockPtr pVB = new CVarBlock();
	pm[""] = pVB;

	VariableHelper( pVB, "Name", m_name, this, &CAGView2::OnChangeName );

	return pm;
}

void CAGView2::OnChangeName()
{
	m_name = m_pGraph->OnChangedViewName( this );
}

void CAGView2::UpdateNodePosition( CAG2ModifierNode* pModifierNode )
{
	Gdiplus::PointF& pos = m_modifiers[pModifierNode->GetModifier()].pos;
	if (!pos.Equals(pModifierNode->GetPos()))
	{
		pos = pModifierNode->GetPos();
		m_pGraph->Modified();
	}
}

void CAGView2::MakeConnectionsInView( CAG2ModifierNode* pModifierNode )
{
	uint modifierId = pModifierNode->GetModifier()->GetId();
	CHyperGraph * pGraph = (CHyperGraph*) pModifierNode->GetGraph();

	for (StateMap::const_iterator iter = m_states.begin(); iter != m_states.end(); ++iter)
	{
		if (iter->second.hid == HyperNodeID(-1))
			continue;

		if (iter->first->IsModifierLinked(modifierId))
		{
			CHyperNode * from = (CHyperNode*) pGraph->FindNode( pModifierNode->GetId() );
			CHyperNode * to = (CHyperNode*) pGraph->FindNode( iter->second.hid );
			pGraph->ConnectPorts( from, from->FindPort("out", false), to, to->FindPort("in", true), false );
		}
	}
}

void CAGView2::MakeConnectionsFromStateToModifiersInView( CAGNodeBase2* pAGNode )
{
	//uint modifierId = pModifierNode->GetModifier()->GetId();
	CHyperGraph * pGraph = (CHyperGraph*) pAGNode->GetGraph();
	CAGState2Ptr pAGState = pAGNode->GetState();

	// go through all modifier that this state is linked to
	// check if they are present in this view
	// if so connect the ports
	int modCount = pAGState->GetLinkedModifierCount();
	for (int i = 0; i < modCount; ++i)
	{											
		CAG2ModifierBasePtr pMod = m_pGraph->GetModifier(pAGState->GetLinkedModifier(i));
		if (!HasModifier(pMod))
			continue;

		CHyperNode * from = (CHyperNode*) pGraph->FindNode( GetNodeIdForModifier(pMod) );
		CHyperNode * to = GetNodeForState(pAGState); //(CHyperNode*) pGraph->FindNode( pAGState );
		if (from && to)
			pGraph->ConnectPorts( from, from->FindPort("out", false), to, to->FindPort("in", true), false );
	}
}

/*
 * CAGNode
 */

static CHyperNodePainter_MultiIOBox2 animGraphPainter;
static CHyperNodePainter_Image2 animGraphImagePainter;

CAGNodeBase2::CAGNodeBase2(CAGState2Ptr pState)
:	m_pState(pState)
{
}

CAGNode2::CAGNode2( CHyperGraph * pGraph, HyperNodeID id, CAGState2Ptr pState ) : CAGNodeBase2(pState)
{
	assert (pState != 0);

	m_pGraph = pGraph;
	m_id = id;


	IVariablePtr pVar = new CVariableVoid();
	pVar->SetHumanName("in");
	pVar->SetName("in");
	CHyperNodePort portIn( pVar, true );
	portIn.bAllowMulti = true;
	AddPort(portIn);

	pVar = new CVariableVoid();
	pVar->SetHumanName("out");
	pVar->SetName("out");
	CHyperNodePort portOut( pVar, false );
	portOut.bAllowMulti = true;
	AddPort(portOut);

	//SetName( pState->GetName() );
	SetClass( pState->GetName() );

	SetPainter(&animGraphPainter);
}

void CAGNode2::AttachImageNode(CAGImageNode2* pImageNode)
{
	pImageNode->SetParent(this);
	this->m_imageNodes.push_back(pImageNode);
}

/*
 * CAGImageNode
 */
CAGImageNode2::CAGImageNode2( CHyperGraph * pGraph, HyperNodeID id, CAGState2Ptr pState ) : CAGNodeBase2(pState)
{
	m_pParent = 0;

	assert (pState != 0);

	m_pGraph = pGraph;
	m_id = id;

	//SetName( pState->GetName() );
	SetClass( pState->GetName() );

	SetPainter(&animGraphImagePainter);
}

/*
 * CAGEdge
 */

void CAGEdge2::DrawSpecial( Gdiplus::Graphics * pGr, Gdiplus::PointF where )
{
	if(!pGr) return;
	if (!pLink)
		return;
	int forceFollow = pLink->GetForceFollowChance();
	int	cost = pLink->GetCost();

	if (forceFollow || cost != 100)
	{
		Gdiplus::Font font(L"Tahoma", 9.0f);

		Gdiplus::PointF	pos( where );

		wchar_t buf[32];
		if( forceFollow )
		{
			swprintf( buf, L"%d", forceFollow );
			Gdiplus::SolidBrush brush( Gdiplus::Color(255,255,255) );
			pGr->DrawString( buf, -1, &font, pos, &brush );
		}

// DEJAN:
// The following block crashes without any obvious reason!
// However, currently it isn't really needed and might even be confusing,
// so I commented it out until we decide what to do with it.
/*
		pos.Y -= font.GetHeight( pGr );

		if (cost != 100)
		{
			swprintf( buf, L"%d", cost );
			Gdiplus::SolidBrush brush( Gdiplus::Color(230,230,230) );
			pGr->DrawString( buf, -1, &font, pos, &brush );
		}
*/
	}
}

CAGEdge2::CAGEdge2()
{
	m_isModifierLink = false;
}

bool CAGEdge2::IsEditable()
{
	if (!m_isModifierLink) 
		return true; 

	return false;
}

void CAGEdge2::SetToModifierLink( bool isModifier /*= true*/ )
{
	m_isModifierLink = isModifier;
}

int CAGEdge2::GetCustomSelectionMode()
{
	if (m_isModifierLink)
		return 5;

	return 6;
}

/*
 * CAnimationGraph
 */

CAnimationGraph2::CAnimationGraph2(CAnimationImageManager2Ptr pImageManager, CAnimationGraphDialog2 * pParent)
:	m_pImageManager(pImageManager)
{
	m_bShowIcons = false;
	m_modified = false;
	m_useExpensiveLocoStateSelection = false;

	IAnimationGraphCategoryIteratorPtr pCatIter = GetISystem()->GetIAnimationGraphSystem()->CreateCategoryIterator();
	IAnimationGraphCategoryIterator::SCategory cat;
	while (pCatIter->Next(cat))
	{
		CAGCategory2Ptr pCat = new CAGCategory2(cat);
		m_categories[pCat->GetName()] = pCat;
		m_categoryVec.push_back(pCat);
	}

	IAnimationGraphStateFactoryIteratorPtr pSFIter = GetISystem()->GetIAnimationGraphSystem()->CreateStateFactoryIterator();
	IAnimationGraphStateFactoryIterator::SStateFactory sf;
	while (pSFIter->Next(sf))
	{
		CAGStateFactory2Ptr pSF = new CAGStateFactory2(this, sf);
		if (IsObsoleteFactory(pSF->GetName()))
			continue;
		m_stateFactories[pSF->GetName()] = pSF;
		m_stateFactoryVec.push_back(pSF);
	}

	LoadTemplates();

	// new since AG 1.5
	// Reset the Animation parameter presets vector and fill with the 
	// default presets (like Loop, Transition, MovementCycle etc...)
	ResetAnimParamsPreset();

	m_modifierManager = new CModifierManager();
	m_modifierManager->Init(pParent);
}

CAnimationGraph2::~CAnimationGraph2()
{
	if (m_modified && IDOK == AfxMessageBox( "Save changes to " + m_filename + "?", MB_YESNO|MB_ICONQUESTION ))
		Save();

	SAFE_DELETE(m_modifierManager);

	// TODO: Delete Modifier Pointers from internal list
}

void CAnimationGraph2::CreateMinimumGraph()
{
	XmlParser parser;

	// Add an Input
	CAGInput2Ptr pInput = new CAGInput2(this);
//	XmlNodeRef pNode = parser.parseBuffer("<FloatState name=\"ActualMoveSpeed\" signalled=\"0\" priority=\"5\" attachToBlendWeight=\"-1\" blendWeightMoveSpeed=\"0\" forceGuard=\"0\" mixinFilter=\"0\" min=\"-50\" max=\"50\"/>");
	XmlNodeRef pNode = parser.parseBuffer("<KeyState name='Example_Input' signalled='0' priority='5' attachToBlendWeight='-1' blendWeightMoveSpeed='0' \
																				forceGuard='0' mixinFilter='0' defaultValue='none'><Key value='none'/></KeyState>");
	if (!pInput->Load(pNode))
		CryLogAlways("Failed creating default input");
	if (!AddInput(pInput))
		CryLogAlways("Failed adding default input.");

	// Add a State
	CAGState2Ptr pState = new CAGState2(this, "Idle");
	SAnimNodeParams nodeParams;
	nodeParams.ResetToLoopingSelectable();
	pState->SelectPreset(&nodeParams);
	if (!AddState(pState))
		CryLogAlways("Failed adding default state %s", (const char *)pState->GetName());

	// Add Input to the Selection Criteria of the state
	XmlNodeRef selectNode = parser.parseBuffer("<SelectWhen><Example_Input /></SelectWhen>");
	pState->Load_SelectWhen(selectNode);
}

bool CAnimationGraph2::Load( const CString& filename )
{
	m_filename = Path::GamePathToFullPath(filename);
	m_filename = Path::GetRelativePath(m_filename);

	// Is this the old format?
	//CString extension = Path::GetExt(m_filename);
	//if (extension.CompareNoCase("xml") == 0)
	//m_filename = Path::ReplaceExtension( m_filename, "ag2xml" );

	XmlParser parser;
	XmlNodeRef root = parser.parse(filename);
	if (!root)
		return false;

	// Load Version Information
	float agVersionNumber = 1.0f;
	if (root->haveAttr("version"))
		root->getAttr("version", agVersionNumber);

	if (agVersionNumber > AG_VERSION)
	{
		CString errorMessage;
		errorMessage.Format("XML File was saved with Animation Graph %.1f. This Editor is version %.1f.\nI cannot load an Animation Graph that was saved with a later version than this one.\nLoading Aborted.", agVersionNumber, AG_VERSION);
		AfxMessageBox( errorMessage, MB_OK );
		return false;
	}

	if (agVersionNumber < AG_VERSION)
	{
		CString errorMessage;
		errorMessage.Format("Old format (AnimationGraph %.1f) detected. Changes will be saved in new AG %.1f format and the file will be replaced.\nConsider saving under a different filename if you wish to keep the old graph.", agVersionNumber, AG_VERSION);
		AfxMessageBox( errorMessage, MB_OK );
	}

	// Graphs being converted from AG 1.0 will automatically be using the old and expensive SelectLocomotionState function
	// (this can be turned off again after conversion in the Settings if desired)
	m_useExpensiveLocoStateSelection = false;
	if (root->haveAttr("expensiveLocomotionSelection"))
		root->getAttr("expensiveLocomotionSelection", m_useExpensiveLocoStateSelection);
	else if (agVersionNumber == 1.0f)
		m_useExpensiveLocoStateSelection = true;

	m_sCharacterName.Empty();

	// Reset the Animation parameter presets vector and fill with the 
	// default presets
	ResetAnimParamsPreset();

	// load character file name
	{
		XmlNodeRef character = root->findChild("Character");
		if ( character )
			character->getAttr( "filename", m_sCharacterName );
	}

	// AG 1.5
	// load node defaults
	XmlNodeRef detailsNode = root->findChild("DefaultNodeDetails");
	if (detailsNode)
	{
		// if the node exist, load, if not, take defaults from constructor
		m_graphDefaultNodeDetails.Load(detailsNode);
	}

	XmlNodeRef detailsMovementNode = root->findChild("DefaultMovementNodeDetails");
	if (detailsNode)
	{
		// if the node exist, load, if not, take defaults from constructor
		m_graphDefaultMovementDetails.Load(detailsMovementNode);
	}

	XmlNodeRef detailsAnimNode = root->findChild("DefaultAnimNodeDetails");
	if (detailsAnimNode)
	{
		m_graphDefaultAnimationNodeDetails.Load(detailsAnimNode);
	}
	else
	{
		// use the defaults that are already created in the default constructor
		// but figure out the correct layer for this graph

		// go through all the states, check out the template name
		// count the number of templates and the number of times
		// the string "UB" is in there, to figure out, whether this is
		// an upper body graph
		XmlNodeRef states = root->findChild("States");
		if (!states)
			return false;

		int templateCount = 0;
		int UBcount = 0;
		int stateCount = states->getChildCount();
		for (int i = 0; i < stateCount; ++i)
		{
			XmlNodeRef stateNode = states->getChild(i);
			XmlNodeRef templateNode = stateNode->findChild("Template");
			if (templateNode)
			{
				++templateCount;
				CString templateName = "";
				templateNode->getAttr("name", templateName);
				if ((templateName.Find("UB") >= 0) || (templateName.Find("UpperBody") >= 0))
				{
					++UBcount;
				}
			}
		}

		// it is an UB graph if more than half of the templates are UB
		bool UBGraph = (templateCount > 0)? (UBcount / (float)templateCount) >= 0.5f : false;
		// if so, set the layer to 2 and partialBody to true
		if (UBGraph)
		{
			m_graphDefaultAnimationNodeDetails.m_animLayer = 2;
			m_graphDefaultAnimationNodeDetails.PartialBodyUpdate = true;
		}
	}

	// Load all modifier nodes
	// In this test case, just fake add one
	// TODO: properly dispose of the old ones in destructor
	LoadActiveModifiers(root);
	// end of AG 1.5

	// load inputs
	{
		XmlNodeRef inputs = root->findChild("Inputs");
		if (!inputs)
		{
			CryLogAlways("No inputs");
			return false;
		}
		const int nChildren = inputs->getChildCount();
		for (int i=0; i<nChildren; i++)
		{
			CAGInput2Ptr pInput = new CAGInput2(this);
			if (!pInput->Load(inputs->getChild(i)))
			{
				CryLogAlways("Failed loading input %d", i);
				return false;
			}
			if (!AddInput(pInput))
			{
				CryLogAlways("Failed adding input %d", i);
				return false;
			}
		}
	}
	// load states
	{
		XmlNodeRef states = root->findChild("States");
		if (!states)
			return false;
		std::queue<XmlNodeRef> statesToLoad;
		const int nChildren = states->getChildCount();
		for (int i=0; i<nChildren; i++)
			statesToLoad.push( states->getChild(i) );
		int skipCount = 0;
		bool allLoadedOk = true;
		while (!statesToLoad.empty() && skipCount < statesToLoad.size())
		{
			XmlNodeRef state = statesToLoad.front();
			statesToLoad.pop();
			if (state->haveAttr("extend") && !FindState(state->getAttr("extend")))
			{
				statesToLoad.push(state);
				skipCount ++;
			}
			else
			{
				skipCount = 0;
				CAGState2Ptr pState = new CAGState2(this);
				if (!pState->Load(state))
				{
					CryLogAlways("Failed loading state %s", state->getAttr("id"));
					allLoadedOk = false;
				}
				if (!AddState(pState))
				{
					CryLogAlways("Failed adding state %s", (const char *)pState->GetName());
					allLoadedOk = false;
				}
			}
		}
		if (skipCount)
		{
			CryLogAlways("Couldn't load all states");

			std::vector<CString> statesThatCouldntBeLoaded;
			while (!statesToLoad.empty())
			{
				statesThatCouldntBeLoaded.push_back( statesToLoad.front()->getAttr("id") );
				statesToLoad.pop();
			}
			std::sort( statesThatCouldntBeLoaded.begin(), statesThatCouldntBeLoaded.end() );
			for (std::vector<CString>::const_iterator iter = statesThatCouldntBeLoaded.begin(); iter != statesThatCouldntBeLoaded.end(); ++iter)
			{
				CryLogAlways("  %d: %s", (iter - statesThatCouldntBeLoaded.begin()), (const char *)*iter);
			}

			return false;
		}
		if (!allLoadedOk)
			return false;
	}
	// load links
	{
		XmlNodeRef links = root->findChild("Transitions");
		if (!links)
			return false;
		const int nChildren = links->getChildCount();
		for (int i=0; i<nChildren; i++)
		{
			XmlNodeRef child = links->getChild(i);
			CAGLink2Ptr pLink = new CAGLink2(this);
			if (!pLink->Load(child))
				return false;
			CString fromStr = child->getAttr("from");
			CString toStr = child->getAttr("to");
			CAGState2Ptr from = FindState( fromStr );
			CAGState2Ptr to = FindState( toStr );
			if (!from || !to)
			{
				CryLogAlways("Failed loading link %s->%s", (const char *)fromStr, (const char *)toStr);
				if (!from)
					CryLogAlways("%s does not exist", (const char *)fromStr);
				if (!to)
					CryLogAlways("%s does not exist", (const char *)toStr);
				continue;
			}
			bool ok = AddLink( from, to, pLink );
			if (ok && 0 == strcmp("BiLink", child->getTag()))
				ok &= AddLink( to, from, pLink );
			if (!ok)
			{
				CryLogAlways("Failed adding link %s->%s", (const char *)fromStr, (const char *)toStr);
				continue;
			}
		}
	}
	// load views
	{
		XmlNodeRef views = root->findChild("Views");
		if (!!views)
		{
			const int nChildren = views->getChildCount();
			for (int i=0; i<nChildren; i++)
			{
				XmlNodeRef child = views->getChild(i);
				CAGView2Ptr pView = new CAGView2(this);
				if (!pView->Load(child, agVersionNumber))
					return false;
				if (!AddView(pView))
					return false;
			}
		}
	}

	m_modified = false;

	return true;
}

struct CompareGetName
{
	template <class T>
	bool operator()( const T& a, const T& b ) const
	{
		return a->GetName() < b->GetName();
	}
};

struct CompareGetNamePair
{
	template <class T>
	bool operator()( const std::pair<T,T>& a, const std::pair<T,T>& b )
	{
		return a.first->GetName() < b.first->GetName() || (a.first->GetName() == b.first->GetName() && a.second->GetName() < b.second->GetName());
	}
};

template <class T>
static bool BlockToXml( XmlNodeRef parent, const CString& name, const std::set<T>& values )
{
	XmlNodeRef block = parent->createNode(name);

	std::vector<T> saveValues;
	for (std::set<T>::const_iterator iter = values.begin(); iter != values.end(); ++iter)
		saveValues.push_back(*iter);
	std::sort( saveValues.begin(), saveValues.end(), CompareGetName() );

	for (std::vector<T>::const_iterator iter = saveValues.begin(); iter != saveValues.end(); ++iter)
	{
		XmlNodeRef node = (*iter)->ToXml();
		if (!node)
			return false;
		block->addChild(node);
	}

	parent->addChild(block);
	return true;
}

XmlNodeRef CAnimationGraph2::ToXml()
{
	XmlNodeRef root = CreateXmlNode("AnimationSetup");
	root->setAttr("version", "1.5");
	root->setAttr("expensiveLocomotionSelection", m_useExpensiveLocoStateSelection);

	// save character file name
	if ( !m_sCharacterName.IsEmpty() )
	{
		XmlNodeRef character = root->createNode( "Character" );
		character->setAttr( "filename", m_sCharacterName );
		root->addChild( character );
	}

	// AG 1.5
	// save node defaults
	XmlNodeRef detailsNode = root->createNode("DefaultNodeDetails");
	m_graphDefaultNodeDetails.ToXML( detailsNode );
	root->addChild(detailsNode);
	XmlNodeRef detailsAnimNode = root->createNode("DefaultAnimNodeDetails");
	m_graphDefaultAnimationNodeDetails.ToXML( detailsAnimNode );
	root->addChild(detailsAnimNode);
	XmlNodeRef detailsMovementNode = root->createNode("DefaultMovementNodeDetails");
	m_graphDefaultMovementDetails.ToXML( detailsMovementNode );
	root->addChild(detailsMovementNode);
	// end of AG 1.5

	// save inputs
	if (!BlockToXml( root, "Inputs", m_inputs ))
		return XmlNodeRef();

	// AG 1.5
	// save states
	SaveActiveModifiersToXML(root);
	// end of AG 1.5

	// save states
	if (!BlockToXml( root, "States", m_states ))
		return XmlNodeRef();

	// save links
	XmlNodeRef links = root->createNode("Transitions");
	std::vector<TLink> saveTrans;
	for (std::map<TLink, CAGLink2Ptr>::const_iterator iter = m_links.begin(); iter != m_links.end(); ++iter)
		saveTrans.push_back(iter->first);
	std::sort( saveTrans.begin(), saveTrans.end(), CompareGetNamePair() );

	for (std::vector<TLink>::const_iterator iter = saveTrans.begin(); iter != saveTrans.end(); ++iter)
	{
		// ok, we can save this
		XmlNodeRef node = m_links[*iter]->ToXml();
		if (!node)
			return XmlNodeRef();
		node->setAttr("from", iter->first->GetName());
		node->setAttr("to", iter->second->GetName());
		links->addChild(node);
	}
	root->addChild(links);

	// save views
	if (!BlockToXml( root, "Views", m_views))
		return XmlNodeRef();

	return root;
}

void CAnimationGraph2::Save()
{
	if (m_filename.IsEmpty())
		return;

	ICharacterInstance * pInstance = NULL;

	CErrorsRecorder errRecorder;

	if (pInstance)
	{
		GenerateBadCALReport2( pInstance->GetIAnimationSet(), this );
		pInstance->Release();
	}
	GenerateOrphanNodesReport2(this);
	GenerateNullNodesWithNoForceLeaveReport2(this);

	XmlNodeRef xml = ToXml();
	if (!xml)
	{
		AfxMessageBox("Saving Failed");
		return;
	}

	// Create backups of .xml and .ag files
	CFileUtil::BackupFileDated( m_filename, true );
	CString sAGFilename = Path::ReplaceExtension( m_filename, "ag" );
	CFileUtil::BackupFileDated( sAGFilename, true );

	SaveXmlNode( xml,m_filename );

	GetISystem()->GetIAnimationGraphSystem()->ExportXMLToBinary( m_filename, xml );

	m_modified = false;
}

bool CAnimationGraph2::AddInput( CAGInput2Ptr pInput )
{
	const CString& name = pInput->GetName();
	if (m_inputsByName.find(name) != m_inputsByName.end())
		return false;
	if (m_inputs.find(pInput) != m_inputs.end())
		return false;
	m_inputs.insert( pInput );
	m_inputsByName.insert( std::make_pair(name, pInput) );
	Modified();
	return true;
}

bool CAnimationGraph2::RemoveInput( CAGInput2Ptr pInput )
{
	const CString& name = pInput->GetName();
	if (m_inputsByName.find(name) == m_inputsByName.end())
		return false;
	if (m_inputs.find(pInput) == m_inputs.end())
		return false;

	m_inputs.erase( pInput );
	m_inputsByName.erase( name );
	SendInputEvent( eAGIE_Removed, pInput );
	Modified();
	return true;
}

CAGInput2Ptr CAnimationGraph2::FindInput( const CString& name ) const
{
	std::map<CString, CAGInput2Ptr>::const_iterator iter = m_inputsByName.find(name);
	if (iter == m_inputsByName.end())
		return CAGInput2Ptr();
	else
		return iter->second;
}

bool CAnimationGraph2::AddModifier( CAG2ModifierBasePtr newModifier )
{
	uint modId = newModifier->GetId();
	if (m_modifiers.find(modId) != m_modifiers.end())
		return false;
	m_modifiers.insert( std::make_pair(modId, newModifier) );
	Modified();
	return true;
}

CAG2ModifierBasePtr CAnimationGraph2::GetModifier( const uint id )
{
	if (m_modifiers.empty())
		return NULL;

	if (m_modifiers.find(id) == m_modifiers.end())
		return NULL;

	return m_modifiers[id];
}

CAG2ModifierBasePtr CAnimationGraph2::AddNewModifier( const CString& className )
{
	// Check whether a modifier with that class already exists
	// and check whether it is a singleton
	CAG2ModifierBasePtr newModifier = GetFirstModifierOfClass(className);
	if (!newModifier || !newModifier->IsSingleton())
	{
		// Since it is not a singleton or at least doesn't exist yet
		// create a new instance of a modifier of this class and return.
		newModifier = GetModifierManager()->CreateModifier(className);
	}

	CRY_ASSERT_MESSAGE(newModifier, "Could not create modifier of class!");
	AddModifier(newModifier);
	return newModifier;
}

void CAnimationGraph2::RemoveModifier( CAG2ModifierBasePtr ptr )
{
	uint modId = ptr->GetId();
	if (m_modifiers.find(modId) == m_modifiers.end())
		return;

	m_modifiers.erase( modId );

	// Also remove from all states
	{
		std::set<CAGState2Ptr>::iterator it = m_states.begin();
		std::set<CAGState2Ptr>::iterator endIt = m_states.end();
		for ( ; it != endIt; ++it)
		{
			CAGState2Ptr pState = *it;
			pState->RemoveLinkedModifier( modId );
		}
	}

	// Also remove from all views
	{
		std::set<CAGView2Ptr>::iterator it = m_views.begin();
		std::set<CAGView2Ptr>::iterator endIt = m_views.end();
		for ( ; it != endIt; ++it)
		{
			CAGView2Ptr pView = *it;
			pView->RemoveModifier(ptr);
		}
	}

	Modified();
	SendModifierEvent(eAG2_Modifier_Delete, ptr);

	SAFE_DELETE(ptr);
	return;
}


int CAnimationGraph2::TestLoadModifierFromOldGraph( XmlNodeRef stateNode, CAGState2Ptr pAnimNode )
{
	// Check with the modifier manager whether it detect a modifier from this node
	// which will return a className in case of success
	const CString& className = m_modifierManager->GetClassNameFromOldVersionStateNode(stateNode);
	// if so, add it to the list of loaded modifiers
	if (!className.IsEmpty())
	{
		// use AddModifier to account for singletons
		CAG2ModifierBasePtr newModifier = AddNewModifier(className);

		// load the details data into the modifier directly
		newModifier->ConvertFromOldStateNode( stateNode, pAnimNode );

		// Note: The views will upon loading automatically place the linked 
		//       modifiers of each state in the views IF converting from 
		//       an old graph.

		return newModifier->GetId();
	}

	return -1;
}

bool CAnimationGraph2::AddState( CAGState2Ptr pState )
{
	const CString& name = pState->GetName();
	if (m_statesByName.find(name) != m_statesByName.end())
		return false;
	if (m_states.find(pState) != m_states.end())
		return false;
	m_states.insert( pState );
	m_statesByName.insert( std::make_pair(name, pState) );
	Modified();
	return true;
}

bool CAnimationGraph2::CloneState( CAGState2Ptr pState )
{
	// Create unique name
	CString sName;
	int i = 0;
	do 
	{
		++i;
		sName.Format( "%s_%d", pState->GetName(), i);
	} while( m_statesByName.find( sName ) != m_statesByName.end() );

	// Create new node and load params into it
	CAGState2Ptr pNewState = new CAGState2(this);
	XmlNodeRef pRef = pState->ToXml();	
	pRef->setAttr( "id", sName );
	pNewState->Load( pRef );

	m_states.insert( pNewState );
	m_statesByName.insert( std::make_pair(sName, pNewState) );
	Modified();
	SendStateEvent(eAGSE_Cloned2, pNewState); 
	return true;
}

void CAnimationGraph2::RemoveState( CAGState2Ptr pState )
{
	m_states.erase( pState );
	m_statesByName.erase( pState->GetName() );
	Modified();
	SendStateEvent(eAGSE_Removed2, pState);
}

CAGState2Ptr CAnimationGraph2::FindState( const CString& name ) const
{
	std::map<CString, CAGState2Ptr>::const_iterator iter = m_statesByName.find(name);
	if (iter == m_statesByName.end())
		return CAGState2Ptr();
	else
		return iter->second;
}

bool CAnimationGraph2::AddView( CAGView2Ptr pView )
{
	const CString& name = pView->GetName();
	if (m_viewsByName.find(name) != m_viewsByName.end())
		return false;
	if (m_views.find(pView) != m_views.end())
		return false;
	m_views.insert( pView );
	m_viewsByName.insert( std::make_pair(name, pView) );
	Modified();
	return true;
}

CAGView2Ptr CAnimationGraph2::FindView( const CString& name ) const
{
	std::map<CString, CAGView2Ptr>::const_iterator iter = m_viewsByName.find(name);
	if (iter == m_viewsByName.end())
		return CAGView2Ptr();
	else
		return iter->second;
}

bool CAnimationGraph2::RemoveView( CAGView2Ptr pView )
{
	std::set<CAGView2Ptr>::iterator it = m_views.find( pView );
	std::map<CString, CAGView2Ptr>::iterator itByName = m_viewsByName.find( pView->GetName() );
	if ( it == m_views.end() )
		return false;
	m_views.erase( it );
	m_viewsByName.erase( itByName );
	SendViewEvent( eAGVE_Removed2, pView );
	Modified();
	return true;
}

CAGCategory2Ptr CAnimationGraph2::FindCategory( const CString& name ) const
{
	std::map<CString, CAGCategory2Ptr>::const_iterator iter = m_categories.find(name);
	if (iter == m_categories.end())
		return CAGCategory2Ptr();
	else
		return iter->second;
}

CAGStateFactory2Ptr CAnimationGraph2::FindStateFactory( const CString& name ) const
{
	std::map<CString, CAGStateFactory2Ptr>::const_iterator iter = m_stateFactories.find(name);
	if (iter == m_stateFactories.end())
		return CAGStateFactory2Ptr();
	else
		return iter->second;
}

bool CAnimationGraph2::IsObsoleteFactory( const CString& factoryName ) const
{
	if (strcmp(factoryName, "Event") == 0)
		return true;
	if (strcmp(factoryName, "FallAndPlay") == 0)
		return true;
	if (strcmp(factoryName, "LookIk") == 0)
		return true;
	if (strcmp(factoryName, "SetInput") == 0)
		return true;
	if (strcmp(factoryName, "FacialEffector") == 0)
		return true;
	if (strcmp(factoryName, "AttachmentEffect") == 0)
		return true;
	if (strcmp(factoryName, "Output") == 0)
		return true;
	if (strcmp(factoryName, "Sound") == 0)
		return true;
	if (strcmp(factoryName, "Additive") == 0)
		return true;

	return false;
}

CAGLink2Ptr CAnimationGraph2::FindLink(CAGState2Ptr from, CAGState2Ptr to) const
{
	TLink link(from, to);
	std::map<TLink, CAGLink2Ptr>::const_iterator iter = m_links.find(link);
	if (iter == m_links.end())
		return CAGLink2Ptr();
	else
		return iter->second;
}

void CAnimationGraph2::FindLinkedStates( const CAGLink2* link, CAGState2Ptr& from, CAGState2Ptr& to ) const
{
	std::map<TLink, CAGLink2Ptr>::const_iterator it, itEnd = m_links.end();
	for ( it = m_links.begin(); it != itEnd; ++it )
		if ( it->second == link )
		{
			from = it->first.first;
			to = it->first.second;
			return;
		}
	from = to = NULL;
}

bool CAnimationGraph2::AddLink( CAGState2Ptr from, CAGState2Ptr to, CAGLink2Ptr pLink )
{
	TLink link(from, to);
	if (m_links.find(link) != m_links.end())
		return false;
	m_links.insert(std::make_pair(link, pLink));
	Modified();
	return true;
}

bool CAnimationGraph2::RemoveLink( CAGState2Ptr from, CAGState2Ptr to )
{
	TLink link(from, to);
	Modified();
	return m_links.erase( link );
}

bool CAnimationGraph2::IsLinked( CAGState2Ptr from, CAGState2Ptr to ) const
{
	return m_links.find( TLink(from, to) ) != m_links.end();
}

void CAnimationGraph2::AddListener( IAnimationGraphListener2 * pListener )
{
	stl::push_back_unique( m_listeners, pListener );
}

void CAnimationGraph2::RemoveListener( IAnimationGraphListener2 * pListener )
{
	stl::find_and_erase( m_listeners, pListener );
}

void CAnimationGraph2::SendStateEvent( EAGStateEvent2 evt, CAGState2Ptr pState )
{
	for (std::vector<IAnimationGraphListener2*>::const_iterator iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
	{
		(*iter)->OnStateEvent( evt, pState );
	}
}

void CAnimationGraph2::SendModifierEvent( EAG2ModifierEvent evt, CAG2ModifierBasePtr pModifier )
{
	for (std::vector<IAnimationGraphListener2*>::const_iterator iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
	{
		(*iter)->OnModifierEvent( evt, pModifier );
	}
}

void CAnimationGraph2::SendViewEvent( EAGViewEvent2 evt, CAGView2Ptr pView )
{
	for (std::vector<IAnimationGraphListener2*>::const_iterator iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
	{
		(*iter)->OnViewEvent( evt, pView );
	}
}

void CAnimationGraph2::SendLinkEvent( EAGLinkEvent2 evt, CAGLink2Ptr pLink )
{
	for (std::vector<IAnimationGraphListener2*>::const_iterator iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
	{
		(*iter)->OnLinkEvent( evt, pLink );
	}
	Modified();
}

void CAnimationGraph2::SendInputEvent( EAGInputEvent2 evt, CAGInput2Ptr pInput )
{
	for (std::vector<IAnimationGraphListener2*>::const_iterator iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
	{
		(*iter)->OnInputEvent( evt, pInput );
	}
}

template <class T>
CString CAnimationGraph2::OnChangedName( T ptr, std::map<CString, T>& byName )
{
	bool isAllowed = (byName.find(ptr->GetName()) == byName.end());

	for (std::map<CString, T>::iterator iter = byName.begin(); iter != byName.end(); ++iter)
	{
		if (iter->second == ptr)
		{
			if (isAllowed)
			{
				byName.erase( iter );
				byName.insert( std::make_pair(ptr->GetName(), ptr) );
				Modified();
			}
			else
				return iter->first;
			break;
		}
	}
	return ptr->GetName();
}

CString CAnimationGraph2::OnChangedStateName( CAGState2Ptr pState )
{
	CString temp = OnChangedName( pState, m_statesByName );
	if (temp == pState->GetName())
		SendStateEvent( eAGSE_ChangeName2, pState );
	return temp;
}

CString CAnimationGraph2::OnChangedViewName( CAGView2Ptr ptr )
{
	CString temp = OnChangedName( ptr, m_viewsByName );
	if (temp == ptr->GetName())
		SendViewEvent( eAGVE_ChangeName2, ptr );
	return temp;
}

CString CAnimationGraph2::OnChangedInputName( CAGInput2Ptr ptr )
{
	CString temp = OnChangedName( ptr, m_inputsByName );
	if (temp == ptr->GetName())
		SendInputEvent( eAGIE_ChangeName2, ptr );
	return temp;
}

void CAnimationGraph2::ReloadClasses()
{
}

CHyperGraph * CAnimationGraph2::CreateGraph()
{
	class CAGHyperGraph : public CHyperGraph
	{
	public:
		CAGHyperGraph( CAnimationGraph2 * pGraph ) : CHyperGraph(pGraph), m_pGraph(pGraph)
		{
		}

		void RemoveEdge( CHyperEdge *pEdge )
		{

			CAGNodeBase2 * pBaseOut = (CAGNodeBase2*) FindNode(pEdge->nodeOut);
			CAGNodeBase2 * pBaseIn = (CAGNodeBase2*) FindNode(pEdge->nodeIn);

			if ( pBaseOut->GetAGNodeType() == AG_NODE_MODIFIER 
				|| pBaseIn->GetAGNodeType() == AG_NODE_MODIFIER )
			{
				// Remove this modifier from the linked state's list of modifiers
				uint modifierId = ((CAG2ModifierNode*) pBaseOut)->GetModifier()->GetId();

				CAGState2Ptr pAGState = ((CAGNode2*) pBaseIn)->GetState();
				pAGState->RemoveLinkedModifier(modifierId);

			}
			else
			{
				CAGNode2 * pAGOut = (CAGNode2*) FindNode(pEdge->nodeOut);
				CAGNode2 * pAGIn = (CAGNode2*) FindNode(pEdge->nodeIn);
				m_pGraph->RemoveLink( pAGOut->GetState(), pAGIn->GetState() );
			}

			CHyperGraph::RemoveEdge(pEdge);
		}

	private:
		CHyperEdge * CreateEdge()
		{
			return new CAGEdge2();
		}

		void RegisterRegularEdge(CHyperEdge *pEdge, bool fromSpecialDrag)
		{
			CAGNode2 * pAGOut = (CAGNode2*) FindNode(pEdge->nodeOut);
			CAGNode2 * pAGIn = (CAGNode2*) FindNode(pEdge->nodeIn);
			CAGLink2Ptr pLink = m_pGraph->FindLink(pAGOut->GetState(), pAGIn->GetState());
			if (!pLink)
			{
				pLink =	new CAGLink2(m_pGraph);
				if (fromSpecialDrag)
					pLink->SetForceFollowChance(1);

				// find state parameters named same and automap them
				CAGState2Ptr from = pAGOut->GetState();
				CAGState2Ptr to = pAGIn->GetState();
				if ( from && to )
				{
					CParamsDeclaration2* pFromParams = from->GetParamsDeclaration();
					CParamsDeclaration2::iterator it1, it1End = pFromParams->end();
					CParamsDeclaration2* pToParams = to->GetParamsDeclaration();
					CParamsDeclaration2::iterator it2End = pToParams->end();
					for ( it1 = pFromParams->begin(); it1 != it1End; ++ it1 )
					{
						if ( pToParams->find( it1->first ) != it2End )
						{
							CString paramName;
							paramName = '[';
							paramName += it1->first;
							paramName += ']';
							paramName.MakeLower();
							pLink->m_listMappedParams.push_back( std::make_pair(paramName,paramName) );
						}
					}
				}

				m_pGraph->AddLink( pAGOut->GetState(), pAGIn->GetState(), pLink );
			}
			((CAGEdge2*)pEdge)->pLink = pLink;
		}

		void RegisterModifierEdge(CHyperEdge *pEdge, bool fromSpecialDrag)
		{
			((CAGEdge2*)pEdge)->SetToModifierLink(true);

			// Register with the animation state that it is now linked to this modifier
			CAG2ModifierNode* pModifierNode = (CAG2ModifierNode*) FindNode(pEdge->nodeOut);
			uint modifierId = pModifierNode->GetModifier()->GetId();

			CAGNode2* pAGConnectedNode = (CAGNode2*) FindNode(pEdge->nodeIn);
			CAGState2Ptr pAGState = pAGConnectedNode->GetState();
			pAGState->AddLinkedModifier(modifierId);
		}

		void RegisterEdge(CHyperEdge *pEdge, bool fromSpecialDrag)
		{
			CAGNodeBase2 * pBaseOut = (CAGNodeBase2*) FindNode(pEdge->nodeOut);
			CAGNodeBase2 * pBaseIn = (CAGNodeBase2*) FindNode(pEdge->nodeIn);
			
			if ( pBaseOut->GetAGNodeType() == AG_NODE_MODIFIER 
				|| pBaseIn->GetAGNodeType() == AG_NODE_MODIFIER )
			{
				RegisterModifierEdge(pEdge, fromSpecialDrag);
			}
			else
			{
				RegisterRegularEdge(pEdge, fromSpecialDrag);
			}

			CHyperGraph::RegisterEdge(pEdge, fromSpecialDrag);
		}
    
		CAnimationGraph2 * m_pGraph;
	};

	return new CAGHyperGraph( this );
}

CHyperNode * CAnimationGraph2::CreateNode( CHyperGraph* pGraph,const char *sNodeClass,HyperNodeID nodeId, const Gdiplus::PointF& pos, CBaseObject* pObj )
{
	// Normally in HyperGraphManager implementations sNodeClass would indicate the class of node
	// to create. However in this case it is being used to indicate the name of the state being represented.
	
	// In order to specify that the node should be an image node, sNodeClass should be of the form
	// "$image <state>".
	// In order to specify that the node should be an modifier node, sNodeClass should be of the form
	// "$modifier <id>".

	// Check whether the node is to be an image node.
	static const char imageClassPrefix[] = "$image ";
	int nameLength = strlen(sNodeClass);
	int prefixLength = sizeof(imageClassPrefix) - 1;
	bool isImage = false;
	if (nameLength > prefixLength && memcmp(sNodeClass, imageClassPrefix, prefixLength) == 0)
	{
		isImage = true;
		sNodeClass += prefixLength;
	}

	// Check whether the node is to be a modifier node
	bool isModifier = false;
	if (!isImage)
	{
		static const char modifierClassPrefix[] = "$modifier ";
		nameLength = strlen(sNodeClass);
		prefixLength = sizeof(modifierClassPrefix) - 1;
		if (nameLength > prefixLength && memcmp(sNodeClass, modifierClassPrefix, prefixLength) == 0)
		{
			isModifier = true;
			sNodeClass += prefixLength;
		}
	}


	CHyperNode* pNode = 0;
	CAGState2Ptr pState = FindState(sNodeClass);
	if (pState != 0)
	{
		if (isImage)
				pNode = new CAGImageNode2( pGraph, nodeId, pState);
		else
				pNode = new CAGNode2( pGraph, nodeId, pState);
		pNode->SetPos(pos);
	}
	else if (isModifier)
	{
		// create a new modifier node
		// And attach an actual modifier to it :)
		int modifierId = atoi(sNodeClass);
		CAG2ModifierBasePtr pModifier = GetModifier(modifierId);
		pNode = new CAG2ModifierNode( pGraph, nodeId, pModifier );
	}

	return pNode;
}

void CAnimationGraph2::ChildStates( std::vector<CAGState2Ptr>& states, CAGState2Ptr parent )
{
	states.resize(0);
	for (CAnimationGraph2::state_iterator iter = m_states.begin(); iter != m_states.end(); ++iter)
	{
		CAGState2* pParent = (*iter)->GetParent();
		while ( pParent != NULL && pParent != parent )
			pParent = pParent->GetParent();

		if ( pParent == parent )
			states.push_back(*iter);
	}
}

void CAnimationGraph2::StatesLinkedTo( std::vector<CAGState2Ptr>& states, CAGState2Ptr to )
{
	states.resize(0);
	for (std::map<TLink, CAGLink2Ptr>::const_iterator iter = m_links.begin(); iter != m_links.end(); ++iter)
	{
		if (iter->first.second == to)
			states.push_back(iter->first.first);
	}
}

void CAnimationGraph2::StatesLinkedFrom( std::vector<CAGState2Ptr>& states, CAGState2Ptr from )
{
	states.resize(0);
	for (std::map<TLink, CAGLink2Ptr>::const_iterator iter = m_links.lower_bound( TLink(from, NULL) ); iter != m_links.end() && iter->first.first == from; ++iter)
	{
		states.push_back(iter->first.second);
	}
}

void CAnimationGraph2::StateLinks( std::vector<CAGLink2Ptr>& links, CAGState2Ptr state, bool includeLinksTo, bool includeLinksFrom )
{
	links.resize(0);
	for (std::map<TLink, CAGLink2Ptr>::const_iterator iter = m_links.begin(); iter != m_links.end(); ++iter)
	{
		if (includeLinksTo && iter->first.second == state)
			links.push_back(iter->second);
		if (includeLinksFrom && iter->first.first == state)
			links.push_back(iter->second);
	}
	std::sort( links.begin(), links.end() );
	links.resize( std::unique( links.begin(), links.end() ) - links.begin() );
}

void CAnimationGraph2::Modified()
{
	if (!m_modified)
	{
		m_modified = true;
		for (std::vector<IAnimationGraphListener2*>::const_iterator iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
		{
			(*iter)->OnGraphModified();
		}
	}
}

void CAnimationGraph2::GetLinks( TLinkVec& links )
{
	for (std::map<TLink, CAGLink2Ptr>::const_iterator iter = m_links.begin(); iter != m_links.end(); ++iter)
		links.push_back(iter->first);
}

void CAnimationGraph2::ShowIcons(bool showIcons)
{
	m_bShowIcons = showIcons;
}

void CAnimationGraph2::LoadTemplates()
{
	CAGStateTemplate2Ptr pTemplate = new CAGStateTemplate2;
	m_templates.insert(pTemplate);
	m_templatesByName.insert( std::make_pair(pTemplate->GetName(), pTemplate) );

	ICryPak * pCryPak = gEnv->pCryPak;
	_finddata_t fd;
	char filename[_MAX_PATH];

	CString path = "Libs/AnimationGraphTemplates";
	CString search = path + "/*.xml";
	intptr_t handle = pCryPak->FindFirst( search, &fd );
	if (handle != -1)
	{
		int res = 0;

		do 
		{
			strcpy( filename, path );
			strcat( filename, "/" );
			strcat( filename, fd.name );

			if (filename[strlen(filename)-1] != '~')
			{
				XmlNodeRef root = GetISystem()->LoadXmlFile( filename );
				if (root)
				{
					pTemplate = new CAGStateTemplate2;
					if (!pTemplate->Load( root ))
					{
						CryLogAlways("Failed loading animation graph template %s", filename);
					}
					else
					{
						m_templates.insert(pTemplate);
						m_templatesByName.insert( std::make_pair(pTemplate->GetName(), pTemplate) );
					}
				}
				else
				{
					CryLogAlways("Failed parsing animation graph template %s", filename);
				}
			}

			res = pCryPak->FindNext( handle, &fd );
		}
		while (res >= 0);

		pCryPak->FindClose( handle );
	}

	// process inheritance
	std::set<CAGStateTemplate2Ptr>::iterator itTemplates, itTemplatesEnd = m_templates.end();
	for ( itTemplates = m_templates.begin(); itTemplates != itTemplatesEnd; ++itTemplates )
	{
		CAGStateTemplate2* pTemplate = *itTemplates;
		pTemplate->ProcessParents( this );
	}
}

CAGStateTemplate2Ptr CAnimationGraph2::GetDefaultTemplate() const
{
	std::map<CString, CAGStateTemplate2Ptr>::const_iterator it = m_templatesByName.find("Default");
	return it != m_templatesByName.end() ? it->second : NULL;
}

CAGStateTemplate2Ptr CAnimationGraph2::GetTemplate( const CString& name ) const
{
	std::map<CString, CAGStateTemplate2Ptr>::const_iterator iter = m_templatesByName.find(name);
	if (iter == m_templatesByName.end())
		return GetDefaultTemplate();
	return iter->second;
}

void CAnimationGraph2::SetCharacterName( const char* szCharacterName )
{
	if ( m_sCharacterName != szCharacterName )
	{
		m_sCharacterName = szCharacterName;
		Modified();
	}
}

void CAnimationGraph2::ResetAnimParamsPreset()
{
	m_animNodeParamsPresets.clear();
	SAnimNodeParams newPreset;

	// Create Preset Loop Selectable
	newPreset.ResetToLoopingSelectable();
	m_animNodeParamsPresets.push_back(newPreset);

	// Create Preset Movement Cycle
	newPreset.ResetToMovementCycleLoop();
	m_animNodeParamsPresets.push_back(newPreset);

	// Create Preset OneShot Selectable
	newPreset.ResetToOneShotSelectable();
	m_animNodeParamsPresets.push_back(newPreset);

	// Create Preset Transition
	newPreset.ResetToTransitionDefault();
	m_animNodeParamsPresets.push_back(newPreset);
	
	// [*DavidR | 18/Mar/2010] Create HitReaction Transition
	newPreset.ResetToHitReactionsDefault();
	m_animNodeParamsPresets.push_back(newPreset);
}

int CAnimationGraph2::GetAnimNodePresetCount() const
{
	return m_animNodeParamsPresets.size();
}

const SAnimNodeParams* CAnimationGraph2::GetAnimNodePreset( int idx ) const
{
	if ( idx >= 0 && idx < m_animNodeParamsPresets.size() )
		return &(m_animNodeParamsPresets.at(idx));
	else
		return NULL;
}

void CAnimationGraph2::SaveActiveModifiersToXML( XmlNodeRef parent )
{
	XmlNodeRef block = parent->createNode("Modifiers");

	for (ModifierMap::const_iterator iter = m_modifiers.begin(); iter != m_modifiers.end(); ++iter)
	{
		XmlNodeRef node = (iter->second)->ToXml();
		block->addChild(node);
	}

	parent->addChild(block);
}

void CAnimationGraph2::LoadActiveModifiers( XmlNodeRef parent )
{
	XmlNodeRef block = parent->findChild("Modifiers");
	if (!block)
	{
		// couldn't find any modifiers
		// this must be an old format graph
		return;
	}

	int highestIndex = -1;
	int numChilds = block->getChildCount();
	for (int i = 0; i < numChilds; ++i)
	{
		XmlNodeRef modifierNode = block->getChild(i);
		if (0 != strcmp("Modifier", modifierNode->getTag()))
		{
			CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, "AnimationGraph:: Unknown modifier entry: %s. Skipping.", modifierNode->getTag());
			continue;
		}

		// create, load and add to list of active instances
		CAG2ModifierBase* newModifier = m_modifierManager->LoadModifier(modifierNode);
		if (newModifier)
		{
			AddModifier(newModifier);
		}
		else
		{
			CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, "AnimationGraph:: Unknown modifier of type %s. Modifier will not work and be lost upon saving.", modifierNode->getAttr("Type"));
			continue;
		}
	}
}

CModifierManager* CAnimationGraph2::GetModifierManager() const
{
	return m_modifierManager;
}

CAG2ModifierBasePtr CAnimationGraph2::GetFirstModifierOfClass( const CString& className ) const
{
	ModifierMap::const_iterator it = m_modifiers.begin();
	ModifierMap::const_iterator endIt = m_modifiers.end();
	for ( ; it != endIt; ++it)
	{
		if (it->second->GetClassName().Compare(className) == 0)
			return it->second;
	}

	return NULL;
}

void SAnimNodeDetails::ToXML( XmlNodeRef node )
{
	node->setAttr("animLayer", m_animLayer);

	// AnimationLayer
	node->setAttr("fullRootPriority", fullRootPriority);
	node->setAttr("ensureInStack", ensureInStack);
	node->setAttr("stickyOutTime", stickyOutTime);
	node->setAttr("forceLeaveWhenFinished", forceLeaveWhenFinished);
	node->setAttr("speedMultiplier", speedMultiplier);
	node->setAttr("MPSpeedMultiplier", MPSpeedMultiplier);
	node->setAttr("stopCurrentAnimation", stopCurrentAnimation);
	node->setAttr("interruptCurrAnim", interruptCurrAnim);
	node->setAttr("stayInStateUntil", stayInStateUntil);
	node->setAttr("dontInterrupt", dontInterrupt);
//	node->setAttr("forceInStateUntil", forceInStateUntil);

	// ParamsLayer
	node->setAttr("TransitionTime", transitionTime);
	node->setAttr("KeyTime", keyTime);
	node->setAttr("Structure", structure);
	node->setAttr("ManualUpdate", ManualUpdate);
	node->setAttr("LoopAnimation", LoopAnimation);
	node->setAttr("RepeatLastKey", RepeatLastKey);
	node->setAttr("AllowAnimRestart", AllowAnimRestart);
	node->setAttr("VTimeWarping", VTimeWarping);
	node->setAttr("Idle2Move", Idle2Move);
	node->setAttr("Move2Idle", Move2Idle);
	node->setAttr("PartialBodyUpdate", PartialBodyUpdate);

	// TransitionLayer
	//node->setAttr("WaitForAnimation", WaitForAnimation);
}

void SAnimNodeDetails::Load( XmlNodeRef node )
{
	node->getAttr("animLayer", m_animLayer);

	// AnimationLayer
	node->getAttr("fullRootPriority", fullRootPriority);
	node->getAttr("ensureInStack", ensureInStack);
	node->getAttr("stickyOutTime", stickyOutTime);
	node->getAttr("forceLeaveWhenFinished", forceLeaveWhenFinished);
	node->getAttr("speedMultiplier", speedMultiplier);
	node->getAttr("MPSpeedMultiplier", MPSpeedMultiplier);
	node->getAttr("stopCurrentAnimation", stopCurrentAnimation);
	node->getAttr("interruptCurrAnim", interruptCurrAnim);
	node->getAttr("stayInStateUntil", stayInStateUntil);
	node->getAttr("dontInterrupt", dontInterrupt);
//	node->getAttr("forceInStateUntil", forceInStateUntil);

	// ParamsLayer
	node->getAttr("TransitionTime", transitionTime);
	node->getAttr("KeyTime", keyTime);
	//node->getAttr("Structure", structure);
	structure.Empty();  // this is saved in a Modifier and needs no loading
	node->getAttr("ManualUpdate", ManualUpdate);
	node->getAttr("LoopAnimation", LoopAnimation);
	node->getAttr("RepeatLastKey", RepeatLastKey);
	node->getAttr("AllowAnimRestart", AllowAnimRestart);
	node->getAttr("VTimeWarping", VTimeWarping);
	node->getAttr("Idle2Move", Idle2Move);
	node->getAttr("Move2Idle", Move2Idle);
	node->getAttr("PartialBodyUpdate", PartialBodyUpdate);

	// TransitionLayer
	//node->getAttr("WaitForAnimation", WaitForAnimation);
}

void SAnimNodeDetails::CreateFromOldGraphData( const CAGState2Ptr animNode )
{
	// take the graph defaults to start out with
	const SAnimNodeDetails* defaultValues = animNode->GetGraphDefaultAnimNodeDetails();
	*this = *defaultValues;

	// TODO:
	// throw a compatibility error if there's more than one AnimationLayer in the template file
	string path = "Libs/AnimationGraphTemplates/";
	char filename[_MAX_PATH];
	strcpy( filename, path.c_str() );
	strcat( filename, animNode->GetTemplateType() );
	strcat( filename, ".xml" );

	XmlNodeRef rootFile = GetISystem()->LoadXmlFile( filename );
	// this node will be useful for extracting other information later, so save it
	XmlNodeRef animLayerNode; 
	
	bool animLayerNodefound = false;
	CString paramName = "";
	while (rootFile && !animLayerNodefound)
	{
		// load file, search for AnimationLayer node
		for (int layer = 1; layer < 10; ++layer)
		{
			paramName.Format("AnimationLayer%i", layer);
			XmlNodeRef tempNode = rootFile->findChild(paramName);
			if (!tempNode)
				continue;

			if (!animLayerNodefound)
			{
				animLayerNode = tempNode;
				animLayerNodefound = true;
				m_animLayer = layer;
				//break;  // Do not break, rather let it run through all layers to be able to give a warning about unconverted data (see below)
			}
			else
			{
				// AG 1.5 backwards compatibility warning
				// TODO: it could be a viable option to convert the higher layer to an Additive Modifier instead
				CString msg;
				msg.Format( "Conversion Problem:\nTemplate '%s' has more than one AnimationLayer node.\nIt is used in State '%s'.\n\nThis is not supported in AnimationGraph 1.5 and up.\nAll higher layers will be ignored.", filename, animNode->GetName() );
				AfxMessageBox( msg );
				break;
			}
		}

		if (animLayerNodefound)
			break;

		// if not found, repeat check for inheritance (only single inheritance is supported here)
		string extend = rootFile->getAttr("extend");
		strcpy( filename, path.c_str() );
		strcat( filename, extend );
		strcat( filename, ".xml" );
		rootFile = GetISystem()->LoadXmlFile( filename );
	}

	if (animLayerNodefound)
	{
		// take the parameters from the AnimationLayer node
		ReadAnimationLayerParamsFromOldFormatXml(animNode, animLayerNode);

		// find a ParamsLayer node in the same file
		paramName.Format("ParamsLayer%i", m_animLayer);
		XmlNodeRef paramsLayerNode = animLayerNode = rootFile->findChild(paramName);

		// if it does exist, take the values from there too
		if (paramsLayerNode)
		{
			ReadParamLayerFromOldFormatXml(animNode, paramsLayerNode);
		}

		// This MUST come AFTER the ParamsLayer Node has been read!!
		// find a TransitionParamsLayer in the same file
		paramName.Format("TransitionParamsLayer%i", m_animLayer);
		XmlNodeRef transitionParamsLayerNode = animLayerNode = rootFile->findChild(paramName);

		if (transitionParamsLayerNode)
		{
			ReadTransitionParamLayerFromOldFormatXml(animNode, transitionParamsLayerNode);
			// read regular transition params layer in too, because they can include and replace the
			// ParamsLayer node completely (not the other way around!)
			ReadParamLayerFromOldFormatXml(animNode, transitionParamsLayerNode);
		}
	}
}

void SAnimNodeDetails::Reset()
{
	m_animLayer = 1;

	// AnimationLayer
	fullRootPriority			= false;
	ensureInStack					= false;
	stickyOutTime					= -1.0f;
	forceLeaveWhenFinished = false;
	speedMultiplier				= 1.0f;
	MPSpeedMultiplier			= 1.0f;
	stopCurrentAnimation	= false;
	interruptCurrAnim			= false;
	stayInStateUntil			= 0.0f;
	dontInterrupt					= false;
	forceInStateSpecified = false;
	//forceInStateUntil			= 0.0f;

	// ParamsLayer
	transitionTime		= 0.2f;
	keyTime						= -1.0f;
	structure					= "";
	ManualUpdate			= false;
	LoopAnimation			= false;
	RepeatLastKey			= true;
	AllowAnimRestart	= false;
	VTimeWarping			= false;
	Idle2Move					= false;
	Move2Idle					= false;
	PartialBodyUpdate = false;

	// TransitionLayer
	//WaitForAnimation	= false;
}

void SAnimNodeDetails::ReadAnimationLayerParamsFromOldFormatXml( const CAGState2Ptr animNode, XmlNodeRef animLayerNode )
{
	animNode->GetParamFromTemplate(animLayerNode, "fullRootPriority", fullRootPriority);
	animNode->GetParamFromTemplate(animLayerNode, "ensureInStack", ensureInStack);
	animNode->GetParamFromTemplate(animLayerNode, "stickyOutTime", stickyOutTime);
	animNode->GetParamFromTemplate(animLayerNode, "forceLeaveWhenFinished", forceLeaveWhenFinished);
	animNode->GetParamFromTemplate(animLayerNode, "speedMultiplier", speedMultiplier);
	animNode->GetParamFromTemplate(animLayerNode, "MPSpeedMultiplier", MPSpeedMultiplier);
	animNode->GetParamFromTemplate(animLayerNode, "stopCurrentAnimation", stopCurrentAnimation);
	animNode->GetParamFromTemplate(animLayerNode, "interruptCurrAnim", interruptCurrAnim);
	animNode->GetParamFromTemplate(animLayerNode, "stayInStateUntil", stayInStateUntil);
	// forceInStateUntil needs conversion
	float forceInStateUntil = 0.0f;
	forceInStateSpecified = animNode->GetParamFromTemplate(animLayerNode, "forceInStateUntil", forceInStateUntil);
	// if only stayInStateUntil exists, the forceStay will on loading be set to the same value
	// therefore the "dontInterrupt" is implicitly set by not specifying forceInState
	if (forceInStateUntil > 0.0f || (!forceInStateSpecified && stayInStateUntil > 0.0f))
	{
		dontInterrupt = true;
		stayInStateUntil = max(stayInStateUntil, forceInStateUntil);
	}
}

void SAnimNodeDetails::ReadParamLayerFromOldFormatXml( const CAGState2Ptr animNode, XmlNodeRef paramsLayerNode )
{

	// Transition time is a special case
	if (animNode->HasTemplateParam("transition_time"))
	{
		CString tTime = animNode->GetTemplateParameter("transition_time");
		transitionTime = atof(tTime);
	}

	animNode->GetParamFromTemplate(paramsLayerNode, "KeyTime", keyTime);

	// TODO (optional Improvement): 
	//   Check if one with the same Group Name already exists, though
	//   and then use that one instead. (no difference in functionality, but neater looks)
	animNode->GetParamFromTemplate(paramsLayerNode, "structure", structure);
	if (!structure.IsEmpty())
	{
		CAG2ModifierBasePtr newModifier = animNode->GetGraph()->AddNewModifier("TimeAlignGroup");
		XmlNodeRef tempNode = CreateXmlNode( "temporary" );
		tempNode->setAttr("groupName", structure);
		newModifier->Load(tempNode);
		animNode->AddLinkedModifier(newModifier->GetId());
	}
	structure.Empty();

	animNode->GetParamFromTemplate(paramsLayerNode, "ManualUpdate", ManualUpdate);
	animNode->GetParamFromTemplate(paramsLayerNode, "LoopAnimation", LoopAnimation);
	animNode->GetParamFromTemplate(paramsLayerNode, "RepeatLastKey", RepeatLastKey);
	animNode->GetParamFromTemplate(paramsLayerNode, "AllowAnimRestart", AllowAnimRestart);
	animNode->GetParamFromTemplate(paramsLayerNode, "VTimeWarping", VTimeWarping);
	animNode->GetParamFromTemplate(paramsLayerNode, "Idle2Move", Idle2Move);
	animNode->GetParamFromTemplate(paramsLayerNode, "Move2Idle", Move2Idle);
	animNode->GetParamFromTemplate(paramsLayerNode, "PartialBodyUpdate", PartialBodyUpdate);
}

void SAnimNodeDetails::ReadTransitionParamLayerFromOldFormatXml( const CAGState2Ptr animNode, XmlNodeRef transitionParamsLayerNode )
{
	bool waitForAnimation = false;
	animNode->GetParamFromTemplate(transitionParamsLayerNode, "WaitForAnimation", waitForAnimation);

	// Convert to forceInState if needed, as the functionality is the same
	if (waitForAnimation && !forceInStateSpecified)
	{
		dontInterrupt = true;
		if (stayInStateUntil <= 0.0f)
		{
			stayInStateUntil = 0.98f;
		}
	}

	// WaitForKeyTime does not make sense with the AnimationGraph - it is a flag that
	// comes in handy when starting two animations manually at the same time.
	//animNode->GetParamFromTemplate(transitionParamsLayerNode, "WaitForKeyTime", WaitForKeyTime);
}


void SAnimNodeParams::ResetToLoopingSelectable()
{
	// Do a full reset here, in case someone added
	// parameters but forgot to include them here
	animDetails.Reset();
	nodeDetails.Reset();
	name = "Loop";

	// AnimationLayer
	animDetails.fullRootPriority			= false;
	animDetails.ensureInStack					= false;
	animDetails.stickyOutTime					= -1.0f;
	animDetails.forceLeaveWhenFinished = false;
	animDetails.speedMultiplier				= 1.0f;
	animDetails.MPSpeedMultiplier			= 1.0f;
	animDetails.stopCurrentAnimation	= false;
	animDetails.interruptCurrAnim			= false;
	animDetails.stayInStateUntil			= 0.0f;
	animDetails.dontInterrupt					= false;

	// ParamsLayer
	animDetails.transitionTime		= 0.2f;
	animDetails.keyTime						= -1.0f;
	animDetails.structure					= "";
	animDetails.ManualUpdate			= false;
	animDetails.LoopAnimation			= true;
	animDetails.RepeatLastKey			= false;
	animDetails.AllowAnimRestart	= false;
	animDetails.VTimeWarping			= false;
	animDetails.Idle2Move					= false;
	animDetails.Move2Idle					= false;
	animDetails.PartialBodyUpdate = false;

	// Node Details
	nodeDetails.m_allowSelect		= true;
	nodeDetails.m_bHurryable		= false;
	nodeDetails.m_bSkipFP				= false;
	nodeDetails.m_canMix				= true;
	nodeDetails.m_includeInGame	= true;
}

void SAnimNodeParams::ResetToOneShotSelectable()
{
	// Do a full reset here, in case someone added
	// parameters but forgot to include them here
	animDetails.Reset();
	nodeDetails.Reset();
	name = "OneShot";

	// AnimationLayer
	animDetails.fullRootPriority			= false;
	animDetails.ensureInStack					= false;
	animDetails.stickyOutTime					= -1.0f;
	animDetails.forceLeaveWhenFinished = false;
	animDetails.speedMultiplier				= 1.0f;
	animDetails.MPSpeedMultiplier			= 1.0f;
	animDetails.stopCurrentAnimation	= false;
	animDetails.interruptCurrAnim			= false;
	animDetails.stayInStateUntil			= 0.98f;
	animDetails.dontInterrupt					= true;

	// ParamsLayer
	animDetails.transitionTime		= 0.2f;
	animDetails.keyTime						= -1.0f;
	animDetails.structure					= "";
	animDetails.ManualUpdate			= false;
	animDetails.LoopAnimation			= false;
	animDetails.RepeatLastKey			= true;
	animDetails.AllowAnimRestart	= false;
	animDetails.VTimeWarping			= false;
	animDetails.Idle2Move					= false;
	animDetails.Move2Idle					= false;
	animDetails.PartialBodyUpdate = false;

	// Node Details
	nodeDetails.m_allowSelect		= true;
	nodeDetails.m_bHurryable		= false;
	nodeDetails.m_bSkipFP				= false;
	nodeDetails.m_canMix				= true;
	nodeDetails.m_includeInGame	= true;
}

void SAnimNodeParams::ResetToMovementCycleLoop()
{
	// Do a full reset here, in case someone added
	// parameters but forgot to include them here
	animDetails.Reset();
	nodeDetails.Reset();
	name = "Movement Cycle";

	// AnimationLayer
	animDetails.fullRootPriority			= false;
	animDetails.ensureInStack					= false;
	animDetails.stickyOutTime					= -1.0f;
	animDetails.forceLeaveWhenFinished = false;
	animDetails.speedMultiplier				= 1.0f;
	animDetails.MPSpeedMultiplier			= 1.0f;
	animDetails.stopCurrentAnimation	= false;
	animDetails.interruptCurrAnim			= false;
	animDetails.stayInStateUntil			= 0.0f;
	animDetails.dontInterrupt					= false;

	// ParamsLayer
	animDetails.transitionTime		= 0.3f;
	animDetails.keyTime						= -1.0f;
	animDetails.structure					= "Moving";
	animDetails.ManualUpdate			= false;
	animDetails.LoopAnimation			= true;
	animDetails.RepeatLastKey			= false;
	animDetails.AllowAnimRestart	= false;
	animDetails.VTimeWarping			= false;
	animDetails.Idle2Move					= true;
	animDetails.Move2Idle					= false;
	animDetails.PartialBodyUpdate = false;

	// Node Details
	nodeDetails.m_allowSelect		= true;
	nodeDetails.m_bHurryable		= false;
	nodeDetails.m_bSkipFP				= false;
	nodeDetails.m_canMix				= true;
	nodeDetails.m_includeInGame	= true;
}

void SAnimNodeParams::ResetToTransitionDefault()
{
	// Do a full reset here, in case someone added
	// parameters but forgot to include them here
	animDetails.Reset();
	nodeDetails.Reset();
	name = "Transition";

	// AnimationLayer
	animDetails.fullRootPriority			= false;
	animDetails.ensureInStack					= false;
	animDetails.stickyOutTime					= -1.0f;
	animDetails.forceLeaveWhenFinished = false;
	animDetails.speedMultiplier				= 1.0f;
	animDetails.MPSpeedMultiplier			= 1.0f;
	animDetails.stopCurrentAnimation	= false;
	animDetails.interruptCurrAnim			= false;
	animDetails.stayInStateUntil			= 0.98f;
	animDetails.dontInterrupt					= false;

	// ParamsLayer
	animDetails.transitionTime		= 0.2f;
	animDetails.keyTime						= -1.0f;
	animDetails.structure					= "";
	animDetails.ManualUpdate			= false;
	animDetails.LoopAnimation			= false;
	animDetails.RepeatLastKey			= true;
	animDetails.AllowAnimRestart	= false;
	animDetails.VTimeWarping			= false;
	animDetails.Idle2Move					= false;
	animDetails.Move2Idle					= false;
	animDetails.PartialBodyUpdate = false;

	// Node Details
	nodeDetails.m_allowSelect		= false;
	nodeDetails.m_bHurryable		= false;
	nodeDetails.m_bSkipFP				= false;
	nodeDetails.m_canMix				= true;
	nodeDetails.m_includeInGame	= true;

}

void SAnimNodeParams::ResetToHitReactionsDefault()
{
	// Do a full reset here, in case someone added
	// parameters but forgot to include them here
	animDetails.Reset();
	nodeDetails.Reset();
	name = "HitReaction";

	// AnimationLayer
	animDetails.fullRootPriority			= false;
	animDetails.ensureInStack					= false;
	animDetails.stickyOutTime					= 0.1f;
	animDetails.forceLeaveWhenFinished = false;
	animDetails.speedMultiplier				= 1.0f;
	animDetails.MPSpeedMultiplier			= 1.0f;
	animDetails.stopCurrentAnimation	= false;
	animDetails.interruptCurrAnim			= true;
	animDetails.stayInStateUntil			= 1.0f;
	animDetails.dontInterrupt					= true;

	// ParamsLayer
	animDetails.transitionTime		= 0.1f;
	animDetails.keyTime						= -1.0f;
	animDetails.structure					= "";
	animDetails.ManualUpdate			= false;
	animDetails.LoopAnimation			= false;
	animDetails.RepeatLastKey			= true;
	animDetails.AllowAnimRestart	= false;
	animDetails.VTimeWarping			= false;
	animDetails.Idle2Move					= false;
	animDetails.Move2Idle					= false;
	animDetails.PartialBodyUpdate = false;

	// Node Details
	nodeDetails.m_bHurryable		= false;
	nodeDetails.m_bSkipFP				= false;
	nodeDetails.m_allowSelect		= true;
	nodeDetails.m_canMix				= true;
	nodeDetails.m_includeInGame	= true;

}


bool CAGState2::GetParamFromTemplate( XmlNodeRef refNode, CString paramName, CString &paramValue ) const
{
	//! Retrieves a parameter, either from the xml file, or from the exposed parameters, if it contains a $
	if (!refNode->haveAttr(paramName))
		return false;

	CString keyValue = refNode->getAttr(paramName);
	CString expression = "";
	CString retVal = "";

	// Is it just a reference to an exposed parameter?
	if (keyValue.Find('$') == -1)
	{
		// It's not, so just return the value
		paramValue = keyValue;
		return true;
	}
	else
	{
		// Parameter exists, but is exposed as a parameter in graph
		int varNameStartPos = keyValue.Find('$');
		if (varNameStartPos == 0)
		{
			// the param is "just" a plan variable
			keyValue.Remove('$');
		}
		else
		{
			// The entry is more complex, most likely an "iff" statement
			expression = keyValue;
			// get the variable name
			keyValue = expression.Mid(varNameStartPos + 1, expression.GetLength() - varNameStartPos - 1);
			int varNameEndPos = keyValue.FindOneOf(" ="); // whitespace or equal sign
			keyValue = keyValue.Mid(0, varNameEndPos);
			
			// get the default value, in case this is an iff
			if (expression.Mid(0, 3).CompareNoCase("iff") != 0)
			{
				// Time for a warning, this is where the backward compatibility ends
				// Rather create a new template and convert the complex expression into a simple one
				// ONLY supported statements:
				// param = "$varName"
				// param = "iff( $varName == 0, defaultValue, $varName )"
				CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_ERROR, "Cannot convert template expression, Parameter %s will not be converted, value might be lost upon saving", paramName);
				return false;
			}
			varNameStartPos = expression.Find(",");
			expression = expression.Mid(varNameStartPos + 1, expression.GetLength() - varNameStartPos - 1);
			varNameEndPos = expression.Find(",");
			expression = expression.Mid(0, varNameEndPos);
			expression.Trim();
		}
}

	// find the overwrite value, if there is one
	if (HasTemplateParam(keyValue))
	{
		retVal = GetTemplateParameter(keyValue);
	}

	if (!retVal.IsEmpty())
	{
		paramValue = retVal;
	}
	else
	{
		paramValue = expression;
	}

	return true;
}

bool CAGState2::GetParamFromTemplate( XmlNodeRef refNode, CString paramName, int &paramValue ) const
{
	CString retVal = "";
	bool ok = GetParamFromTemplate(refNode, paramName, retVal);

	if (!retVal.IsEmpty())
	{
		paramValue = atoi(retVal);
	}

	return ok;
}

bool CAGState2::GetParamFromTemplate( XmlNodeRef refNode, CString paramName, float &paramValue ) const
{
	CString retVal = "";
	bool ok = GetParamFromTemplate(refNode, paramName, retVal);

	if (!retVal.IsEmpty())
	{
		paramValue = atof(retVal);
	}

	return ok;
}

bool CAGState2::GetParamFromTemplate( XmlNodeRef refNode, CString paramName, bool &paramValue ) const
{
	CString retVal = "";
	bool ok = GetParamFromTemplate(refNode, paramName, retVal);

	if (!retVal.IsEmpty())
	{
		paramValue = (bool)(atoi(retVal));
	}

	return ok;
}

const SAnimNodeDetails* CAGState2::GetGraphDefaultAnimNodeDetails() const
{
	return m_pGraph->GetAnimationNodeDefaultDetails();
}

const SMovementParams* CAGState2::GetGraphDefaultMovementDetails() const
{
	return m_pGraph->GetDefaultMovementDetails();
}

CString CAGState2::GetNodePresetName() const
{
	return m_animParamsPresetName;
}

void CAGState2::SetPresetName( const char* name )
{
	m_animParamsPresetName = name;
}

void CAGState2::AddLinkedModifier( uint modifierId, bool inheritable /*= false*/ )
{
	// in case this modifier is already in the list... do nothing.
	if (std::find(m_linkedModifiers.begin(), m_linkedModifiers.end(), modifierId) != m_linkedModifiers.end())
		return;

	m_linkedModifiers.push_back(modifierId);

	if (inheritable)
		m_inheritableModifiers.push_back(modifierId);
}

void CAGState2::RemoveLinkedModifier( uint modifierId )
{
	std::vector<uint>::iterator modifierIter = std::find(m_linkedModifiers.begin(), m_linkedModifiers.end(), modifierId);

	// in case this modifier was never linked to this state in the first place... do nothing.
	if (modifierIter == m_linkedModifiers.end())
		return;

	m_linkedModifiers.erase(modifierIter);
}

void CAGState2::SaveLinkedModifiersToXML( XmlNodeRef node )
{
	// go through all linked modifiers and write out their id
	XmlNodeRef linkedModifiersNode = node->createNode("LinkedModifiers");
	std::vector<uint>::iterator iterEnd = m_linkedModifiers.end();
	for (std::vector<uint>::iterator modifierIter = m_linkedModifiers.begin(); modifierIter != iterEnd; ++modifierIter)
	{
		XmlNodeRef newModifier = linkedModifiersNode->createNode("Modifier");
		newModifier->setAttr("Id", *modifierIter);
		linkedModifiersNode->addChild(newModifier);
	}

	node->addChild(linkedModifiersNode);
}

void CAGState2::LoadLinkedModifiers( XmlNodeRef node )
{
	int numChilds = node->getChildCount();
	for (int i = 0; i < numChilds; ++i)
	{
		XmlNodeRef child = node->getChild(i);
		CString tag = child->getTag();
		if (strcmp(tag, "Modifier") != 0)
		{
			CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, "Unknown Node %s found inside LinkedModifiers for State %s. Skipping.", child->getTag(), GetName());
			continue;
		}

		if (!child->haveAttr("Id"))
		{
			CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, "The %i. linked modifier for state %s has no id. Skipping.", i, GetName());
			continue;
		}

		int modifierId = -1;
		child->getAttr("Id", modifierId);
		AddLinkedModifier(modifierId);
	}
}

bool CAGState2::IsModifierLinked( uint modifierId )
{
	if (std::find(m_linkedModifiers.begin(), m_linkedModifiers.end(), modifierId) != m_linkedModifiers.end())
		return true;

	return false;
}

bool CAGState2::IsLinkedModifierInheritable( uint modifierId )
{
	if (!IsModifierLinked(modifierId))
		return false;

	if (std::find(m_inheritableModifiers.begin(), m_inheritableModifiers.end(), modifierId) != m_inheritableModifiers.end())
		return true;

	return false;
}

int CAGState2::GetLinkedModifierCount()
{
	return m_linkedModifiers.size();
}

uint CAGState2::GetLinkedModifier( int index )
{
	CRY_ASSERT_MESSAGE(index < GetLinkedModifierCount(), "Tried to access modifier of a state with and out of bounds index");
	return m_linkedModifiers.at(index);
}

void CAGState2::ExportModifiers( XmlNodeRef node )
{
	std::vector<uint>::iterator it = m_linkedModifiers.begin();
	std::vector<uint>::iterator endIt = m_linkedModifiers.end();
	for ( ; it != endIt; ++it)
	{
		(m_pGraph->GetModifier(*it))->Export(node);
	}
}

void CAGState2::TryLoadingModifier( XmlNodeRef stateNode, bool inherited /* = false */ )
{
	int modId = m_pGraph->TestLoadModifierFromOldGraph(stateNode, this);
	if (modId != -1)
	{
		// This is valid so add it to the linked modifiers
		// States inheriting from this one will also inherit the
		// modifiers loaded in here (and only from here)
		AddLinkedModifier(modId, inherited);
	}
}

void CAGState2::SaveFPAnimControlledView( XmlNodeRef node )
{
	bool bAnimationControlledView = false;
	int modCount = GetLinkedModifierCount();
	for (int i = 0; i < modCount; ++i)
	{
		CAG2ModifierBasePtr pMod = GetGraph()->GetModifier(GetLinkedModifier(i));
		if (pMod->GetClassName() == "AnimControlledCamera")
		{
			bAnimationControlledView = true;
		}
	}
	node->setAttr("animationControlledView", bAnimationControlledView);
}

void CAGState2::SelectPreset( const SAnimNodeParams* pPreset )
{
	m_generalNodeDetails = pPreset->animDetails;
	m_graphNodeDetails = pPreset->nodeDetails;
	m_movementNodeDetails = pPreset->movementDetails;
	
	SetPresetName(pPreset->name);
}

XmlNodeRef SAnimNodeDetails::CreateAnimationLayerNode( TPerParameterizationData* pParamedData, TPerParameterizationData* pUnParamedData )
{
	CString nodeName;
	nodeName.Format("AnimationLayer%i", m_animLayer);
	XmlNodeRef node = CreateXmlNode(nodeName);

	CString animName = pParamedData->IsDefault() ? pUnParamedData->GetAnimationName() : pParamedData->GetAnimationName();

	node->setAttr("animation1", animName);
	node->setAttr("fullRootPriority", fullRootPriority);
	node->setAttr("ensureInStack", ensureInStack);
	node->setAttr("stickyOutTime", stickyOutTime);
	node->setAttr("forceLeaveWhenFinished", forceLeaveWhenFinished);
	node->setAttr("speedMultiplier", speedMultiplier);
	node->setAttr("MPSpeedMultiplier", MPSpeedMultiplier);
	node->setAttr("stopCurrentAnimation", stopCurrentAnimation);
	node->setAttr("interruptCurrAnim", interruptCurrAnim);

	if (dontInterrupt)
	{
		node->setAttr("stayInStateUntil", stayInStateUntil);
		node->setAttr("forceInStateUntil", stayInStateUntil);
	}
	else
	{
		node->setAttr("stayInStateUntil", stayInStateUntil);
		node->setAttr("forceInStateUntil", 0.0f);
	}

	return node;
}

XmlNodeRef SAnimNodeDetails::CreateTransitionLayerNode( const CAGState2Ptr animNode )
{
	CString nodeName;
	nodeName.Format("TransitionParamsLayer%i", m_animLayer);
	XmlNodeRef node = CreateXmlNode(nodeName);

	node->setAttr("TransitionTime", transitionTime);
	node->setAttr("KeyTime", keyTime);
	
	// Gather all linked modifiers of class type TimeAlignGroup
	// Get their group names and append
	CString timeAlignGroups = "";
	int modCount = animNode->GetLinkedModifierCount();
	for (int i = 0; i < modCount; ++i)
	{
		CAG2ModifierBasePtr pMod = animNode->GetGraph()->GetModifier(animNode->GetLinkedModifier(i));
		if (pMod->GetClassName() == "TimeAlignGroup")
		{
			if (!pMod->GetCustomText().IsEmpty())
			{
				if (!timeAlignGroups.IsEmpty())
					timeAlignGroups.Append("|");
				timeAlignGroups.Append(pMod->GetCustomText());
			}
		}
	}
	node->setAttr("Structure", timeAlignGroups);

	node->setAttr("ManualUpdate", ManualUpdate);
	node->setAttr("LoopAnimation", LoopAnimation);
	node->setAttr("RepeatLastKey", RepeatLastKey);
	node->setAttr("AllowAnimRestart", AllowAnimRestart);
	node->setAttr("VTimeWarping", VTimeWarping);
	node->setAttr("Idle2Move", Idle2Move);
	node->setAttr("Move2Idle", Move2Idle);
	node->setAttr("PartialBodyUpdate", PartialBodyUpdate);

	//node->setAttr("WaitForAnimation", WaitForAnimation);

	return node;
}

void SGraphNodeDetails::Load( XmlNodeRef node )
{
	node->getAttr("allowSelect", m_allowSelect);
	node->getAttr("includeInGame", m_includeInGame);
	node->getAttr("canMix", m_canMix);
	node->getAttr("Hurryable", m_bHurryable);
	node->getAttr("SkipFP", m_bSkipFP);
}

void SGraphNodeDetails::ToXML( XmlNodeRef node )
{
	node->setAttr("allowSelect", m_allowSelect);
	node->setAttr("includeInGame", m_includeInGame);
	node->setAttr("canMix", m_canMix);
	node->setAttr("Hurryable", m_bHurryable);
	node->setAttr("SkipFP", m_bSkipFP);
}

void SMovementParams::Reset()
{
	// MCM Reset
	mcmHoriz = eMCM_Entity;
	mcmVert = eMCM_SmoothedEntity;
	angle = -1.0f;
	distance = -1.0f;
	useHorizGraphDefault= true;
	useVertGraphDefault = true;

	// Collider Reset
	useGraphDefaultCollider = true;
	colliderMode = eColliderMode_Pushable;
}

void SMovementParams::ToXML( XmlNodeRef node )
{
	node->setAttr("useHorizGraphDefault", useHorizGraphDefault);
	node->setAttr("useVertGraphDefault", useVertGraphDefault);
	node->setAttr("mcmHoriz", mcmHoriz);
	node->setAttr("mcmVert", mcmVert);
	node->setAttr("angle", angle);
	node->setAttr("distance", distance);
	node->setAttr("useGraphDefaultCollider", useGraphDefaultCollider);
	node->setAttr("colliderMode", colliderMode);
}

void SMovementParams::Load( XmlNodeRef node )
{
	int tempInt;

	node->getAttr("useHorizGraphDefault", useHorizGraphDefault);
	node->getAttr("useVertGraphDefault", useVertGraphDefault);
	node->getAttr("mcmHoriz", tempInt);
	mcmHoriz = (EMovementControlMethod)tempInt;
	node->getAttr("mcmVert", tempInt);
	mcmVert = (EMovementControlMethod)tempInt;
	node->getAttr("angle", angle);
	node->getAttr("distance", distance);
	node->getAttr("useGraphDefaultCollider", useGraphDefaultCollider);
	node->getAttr("colliderMode", tempInt);
	colliderMode = (EColliderMode)tempInt;
}

void SMovementParams::CreateFromOldGraphData( const CAGState2Ptr animNode )
{
	// Get the data from the template

	// take the graph defaults to start out with
	const SMovementParams* defaultValues = animNode->GetGraphDefaultMovementDetails();
	*this = *defaultValues;

	string path = "Libs/AnimationGraphTemplates/";
	char filename[_MAX_PATH];
	strcpy( filename, path.c_str() );
	strcat( filename, animNode->GetTemplateType() );
	strcat( filename, ".xml" );

	XmlNodeRef rootFile = GetISystem()->LoadXmlFile( filename );

	// these nodes will be useful for extracting other information later, so save it
	XmlNodeRef mcmCtrlNode = NULL; 
	XmlNodeRef colliderNode = NULL; 

	while (rootFile && (!mcmCtrlNode || !colliderNode))
	{
		// load file, search for AnimationLayer node
		if (!mcmCtrlNode)
			mcmCtrlNode = rootFile->findChild("MovementControlMethod");
		if (!colliderNode)
			colliderNode = rootFile->findChild("ColliderMode");

		if (!mcmCtrlNode || !colliderNode)
		{
			// if one of them is not found, repeat check for inheritance
			string extend = rootFile->getAttr("extend");
			strcpy( filename, path.c_str() );
			strcat( filename, extend );
			strcat( filename, ".xml" );
			rootFile = GetISystem()->LoadXmlFile( filename );
		}
		else
		{
			break;
		}
	}

	// Now copy the values (if the nodes have been found)

	// MovementCtrlMethod
	if (mcmCtrlNode)
	{
		CString methodHorizontal = mcmCtrlNode->getAttr("horizontal");
		CString methodVertical = mcmCtrlNode->getAttr("vertical");

		for (int mode = eMCM_Undefined; mode < eMCM_COUNT; ++mode)
		{
			if (methodHorizontal == g_szMCMString[(EMovementControlMethod)mode])
			{
				mcmHoriz = (EMovementControlMethod)mode;
				useHorizGraphDefault = false;
			}
			if (methodVertical == g_szMCMString[(EMovementControlMethod)mode])
			{
				mcmVert = (EMovementControlMethod)mode;
				useVertGraphDefault = false;
			}
		}	

		// allowed deviation for angle and distance
		if (mcmCtrlNode->haveAttr("distance"))
			mcmCtrlNode->getAttr("distance", distance);
		else
			distance = -1.0f;
		
		if (mcmCtrlNode->haveAttr("angle"))
			mcmCtrlNode->getAttr("angle", angle);
		else
			angle = -1.0f;
	}

	// ColliderMode
	if (colliderNode)
	{
		CString colMode = colliderNode->getAttr("mode");
		for (int mode = eColliderMode_Undefined; mode < eColliderMode_COUNT; ++mode)
		{
			if (colMode == g_szColliderModeString[(EColliderMode)mode])
			{
				colliderMode = (EColliderMode)mode;
				useGraphDefaultCollider = false;
			}
		}
	}
}

XmlNodeRef SMovementParams::CreateMovementCtrlNode( const CAGState2Ptr animNode )
{
	XmlNodeRef node = CreateXmlNode("MovementControlMethod");

	const SMovementParams* defaultParams = animNode->GetGraphDefaultMovementDetails();
	
	// horizontal mcm
	if (useHorizGraphDefault)
		node->setAttr("horizontal", g_szMCMString[defaultParams->mcmHoriz]);
	else
		node->setAttr("horizontal", g_szMCMString[mcmHoriz]);

	// vertical mcm
	if (useVertGraphDefault)
		node->setAttr("vertical", g_szMCMString[defaultParams->mcmVert]);
	else
		node->setAttr("vertical", g_szMCMString[mcmVert]);

	// allowed distance
	if (distance >= 0.0f)
		node->setAttr("distance", distance);

	// allowed angle deviation
	if (angle >= 0.0f)
		node->setAttr("angle", angle);

	return node;
}

XmlNodeRef SMovementParams::CreateColliderNode( const CAGState2Ptr animNode )
{
	XmlNodeRef node = CreateXmlNode("ColliderMode");

	const SMovementParams* defaultParams = animNode->GetGraphDefaultMovementDetails();

	if (useGraphDefaultCollider)
	{
		node->setAttr("mode", g_szColliderModeString[defaultParams->colliderMode]);
	}
	else
	{
		node->setAttr("mode", g_szColliderModeString[colliderMode]);
	}

	return node;
}

