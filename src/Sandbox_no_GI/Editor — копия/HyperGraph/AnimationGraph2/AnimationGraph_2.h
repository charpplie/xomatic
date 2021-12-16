#ifndef __ANIMATIONGRAPH2_H__
#define __ANIMATIONGRAPH2_H__

#pragma once

#include <set>
#include <map>
#include "IAnimationGraph.h"
#include "IAnimationGraphSystem.h"
#include "../HyperGraph.h"
#include "../HyperGraphManager.h"
#include "AnimationImageManager_2.h"

class CAnimationGraph2;
class CAnimationGraphStateEditor2;
class CPropertyCtrl;
class CModifierManager;

typedef std::set< CString > TSetOfCString;
typedef std::map< CString, TSetOfCString, stl::less_stricmp<CString> > TMapParams;


class TParameterizationId2;


class CParamsDeclaration2 : public TMapParams
{
public:
	bool GuessParamId( const TParameterizationId2& hint, TParameterizationId2& returnId ) const;
};


class TParameterizationId2 : public std::map< CString, CString >
{
public:
	bool operator < ( const TParameterizationId2& p ) const
	{
		assert( size() == p.size() );
		std::pair< const_iterator, const_iterator > it2 = std::mismatch( begin(), end(), p.begin() );
		if ( it2.first == end() )
			return false;
		return *it2.first < *it2.second;
	}
	bool operator == ( const TParameterizationId2& p ) const
	{
		assert( size() == p.size() );
		std::pair< const_iterator, const_iterator > it2 = std::mismatch( begin(), end(), p.begin() );
		return it2.first == end();
	}
	CString AsString() const
	{
		CString result;
		const_iterator it = begin();
		while ( it != end() )
		{
			result += it->second;
			if ( ++it == end() )
				break;
			result += ',';
		}
		return result;
	}
	bool FromString( const CString& value )
	{
		int i = 0;
		int len = value.GetLength();
		CString token;
		iterator it = begin();
		while ( i <= len )
		{
			int comma = value.Find( ',', i );
			if ( comma == -1 )
				comma = len;
			assert( it != end() );
			if ( it == end() )
				return false;
			it->second = value.Mid( i, comma-i );
			i = comma+1;
			++it;
		}
		assert( it == end() );
		return it == end();
	}
	void InitEmptyFromDeclaration( const CParamsDeclaration2& params )
	{
		CParamsDeclaration2::const_iterator it, itEnd = params.end();
		for ( it = params.begin(); it != itEnd; ++it )
			insert(value_type( it->first, CString() ));
	}
};

enum ECriteriaType2
{
	eCT_AnyValue2,
	eCT_MinMax2,
	eCT_ExactValue2,
	eCT_UseParent2,
	eCT_UseDefault2,
	eCT_DifferentValues2,
	eCT_DefinedInTemplate2,  // <-- TODO: remove!
};

struct SCriteria2
{
	SCriteria2()
	{
		type = eCT_AnyValue2;
		floatRange.minimum = 0;
		floatRange.maximum = 0;
		intRange.minimum = 0;
		intRange.maximum = 0;
		floatValue = 0;
		intValue = 0;
	}

	ECriteriaType2 type;
	struct 
	{
		float minimum;
		float maximum;
	} floatRange;
	struct 
	{
		int minimum;
		int maximum;
	} intRange;
	float floatValue;
	int intValue;
	CString strValue;

	bool operator == ( const SCriteria2& rhs ) const
	{
		if ( type != rhs.type )
			return false;
		switch ( type )
		{
		case eCT_AnyValue2:
			return true;
		case eCT_MinMax2:
			return floatRange.minimum == rhs.floatRange.minimum
				&& floatRange.maximum == rhs.floatRange.maximum
				&& intRange.minimum == rhs.intRange.minimum
				&& intRange.maximum == rhs.intRange.maximum;
		case eCT_ExactValue2:
			return floatValue == rhs.floatValue
				&& intValue == rhs.intValue
				&& strValue == rhs.strValue;
		//case eCT_UseParent:
		//case eCT_UseDefault:
		//case eCT_DifferentValues:
		//case eCT_DefinedInTemplate:
		default:
			assert( !"Non-handled switch case in SCriteria operator ==" );
		}
		return false;
	}
	bool operator != ( const SCriteria2& rhs ) const { return !(operator==(rhs));}
};

class TPerParameterizationData;


class CAGState2;
typedef _smart_ptr<CAGState2> CAGState2Ptr;


// Per-Parameterization Details for animation nodes
class TPerParameterizationData
{
public:
	TPerParameterizationData()
	{
		Reset();
	}

	void Reset()
	{
		m_isDefault = true;
		m_animationName.Empty();
	}

	//! returns true if one of the parameters is not empty/default value
	inline bool IsDefault() const
	{
		return m_isDefault;
	}

	void SetDefault(bool val)
	{
		m_isDefault = val;
	}

	void SetAnimationName(const CString& newName, bool isDefault = false)
	{
		m_animationName = newName;
		m_isDefault = isDefault;
	}

	inline const CString GetAnimationName() const
	{
		return m_animationName;
	}

	XmlNodeRef ToXml();
	bool Load( XmlNodeRef node );
	bool CreateFromOldGraphData( const CAGState2Ptr animNode, const TParameterizationId2* pParamId);

private:
	bool m_isDefault;
	CString m_animationName;
};

//! Generalized node details that are independent from the node type
struct SGraphNodeDetails
{
	// Load and Save in new format
	void ToXML( XmlNodeRef node );
	void Load( XmlNodeRef node );

	SGraphNodeDetails()
	{
		Reset();
	}

	void Reset()
	{
		m_allowSelect		= true;
		m_includeInGame = true;
		m_canMix				= true;
		m_bHurryable		= true;
		m_bSkipFP				= false;
	}

	bool m_allowSelect;
	bool m_includeInGame;
	bool m_canMix;
	bool m_bHurryable;
	bool m_bSkipFP;

};

// here comes all the animation node's details data (AnimGraph 2.0)
// =======================================
struct SAnimNodeDetails
{
	SAnimNodeDetails()
	{
		Reset();
	}

	void Reset();

	// Load and Save in new format
	void ToXML( XmlNodeRef node );
	void Load( XmlNodeRef node );

	// Save these out additionally so the ingame graph can read it
	// it will be ignored when loading in the AG Editor
	XmlNodeRef CreateTransitionLayerNode( const CAGState2Ptr animNode );
	XmlNodeRef CreateAnimationLayerNode( TPerParameterizationData* pParamedData, TPerParameterizationData* pUnParamedData );

	// the following is needed for backwards compatibility, to convert old graph data into new format
	// ======================================================================
	void CreateFromOldGraphData( const CAGState2Ptr animNode );
	void ReadAnimationLayerParamsFromOldFormatXml( const CAGState2Ptr animNode, XmlNodeRef animLayerNode );
	void ReadParamLayerFromOldFormatXml( const CAGState2Ptr animNode, XmlNodeRef paramsLayerNode );
	void ReadTransitionParamLayerFromOldFormatXml( const CAGState2Ptr animNode, XmlNodeRef transitionParamsLayerNode );
	// ======================================================================

	// here come the actual data
	// =======================

	int m_animLayer;  // 1-based layer index

	// heritage stuff, might be subject to change
	// AnimationLayer
	float		stickyOutTime;
	float		speedMultiplier;
	float		MPSpeedMultiplier;
	float		stayInStateUntil;
	//float		forceInStateUntil;
	bool		ensureInStack;
	bool		fullRootPriority;
	bool		stopCurrentAnimation;
	bool		interruptCurrAnim;
	bool		forceLeaveWhenFinished;
	bool		dontInterrupt;
	bool		forceInStateSpecified;

	// ParamsLayer
	float		transitionTime;
	float		keyTime;
	CString	structure;
	bool		ManualUpdate;
	bool		LoopAnimation;
	bool		RepeatLastKey;
	bool		AllowAnimRestart;
	bool		VTimeWarping;
	bool		Idle2Move;
	bool		Move2Idle;
	bool		PartialBodyUpdate;

	// TransitionLayer
//	bool		WaitForAnimation;
};

struct SMovementParams
{
	SMovementParams()
	{
		Reset();
	}

	//! Sets the default values
	void Reset();

	// Load and Save in new format
	void ToXML( XmlNodeRef node );
	void Load( XmlNodeRef node );

	//! Reads out the data from the old format templates
	void CreateFromOldGraphData( const CAGState2Ptr animNode );

	// Exporting
	XmlNodeRef CreateMovementCtrlNode( const CAGState2Ptr animNode );
	XmlNodeRef CreateColliderNode( const CAGState2Ptr animNode );

	// Movement Control Method Members
	bool		useHorizGraphDefault;
	bool		useVertGraphDefault;
	float		angle;
	float		distance;
	EMovementControlMethod mcmHoriz;
	EMovementControlMethod mcmVert;

	// Collider Mode Members
	bool					useGraphDefaultCollider;
	EColliderMode	colliderMode;
};

struct SAnimNodeParams
{
	void ResetToLoopingSelectable();
	void ResetToOneShotSelectable();
	void ResetToTransitionDefault();
	void ResetToMovementCycleLoop();
	void ResetToHitReactionsDefault();

	CString						name;
	SAnimNodeDetails  animDetails;
	SGraphNodeDetails	nodeDetails;
	SMovementParams		movementDetails;
};


typedef std::vector<SAnimNodeParams> AnimParamsPresetsVector;
typedef std::map<CString, CVarBlockPtr> TAGPropMap;














// used for telling the user about doing something, before actually doing it!
struct IAGCheckedOperation2 : public _reference_target_t
{
	virtual CString GetExplanation() = 0;
	virtual void Perform() = 0;
};

typedef _smart_ptr<IAGCheckedOperation2> IAGCheckedOperation2Ptr;

template <class T>
class CAGCheckedOperation2 : public IAGCheckedOperation2
{
public:
	typedef void (T::*OpFunc)();
	CAGCheckedOperation2( _smart_ptr<T> ptr, OpFunc func, CString explanation ) : m_ptr(ptr), m_func(func), m_explanation(explanation)
	{
	}

	virtual CString GetExplanation() { return m_explanation; }
	virtual void Perform() { ((&*m_ptr)->*m_func)(); }

private:
	_smart_ptr<T> m_ptr;
	OpFunc m_func;
	CString m_explanation;
};

template <class T, class Param>
class CAGCheckedOperation1_2 : public IAGCheckedOperation2
{
public:
	typedef void (T::*OpFunc)(Param);
	CAGCheckedOperation1_2( _smart_ptr<T> ptr, OpFunc func, Param param, CString explanation ) : m_ptr(ptr), m_func(func), m_explanation(explanation), m_param(param)
	{
	}

	virtual CString GetExplanation() { return m_explanation; }
	virtual void Perform() { ((&*m_ptr)->*m_func)(m_param); }

private:
	_smart_ptr<T> m_ptr;
	OpFunc m_func;
	CString m_explanation;
	Param m_param;
};

class CAGCompositeOperation2 : public IAGCheckedOperation2
{
public:
	void AddOperation( IAGCheckedOperation2Ptr op ) { m_ops.push_back(op); }
	virtual CString GetExplanation();
	virtual void Perform();

private:
	std::vector<IAGCheckedOperation2Ptr> m_ops;
};

class CAGCategory2 : public _reference_target_t
{
public:
	CAGCategory2( const IAnimationGraphCategoryIterator::SCategory& );

	const CString& GetName() const { return m_name; }
	bool IsOverridable() const { return m_bOverridable; }
  bool IsUsableWithTemplate() const { return m_bUsableWithTemplate; }

private:
	CString m_name;
	bool m_bOverridable;
  bool m_bUsableWithTemplate;
};
typedef _smart_ptr<CAGCategory2> CAGCategory2Ptr;

class CAGStateFactoryParam2;
typedef _smart_ptr<CAGStateFactoryParam2> CAGStateFactoryParam2Ptr;
class CAGStateFactoryParam2 : public _reference_target_t
{
public:
	static CAGStateFactoryParam2Ptr Create(const IAnimationGraphStateFactoryIterator::SStateFactoryParameter&);

	const CString& GetName() const { return m_name; }
	void AddToVarBlock( CVarBlockPtr pVB );
	void Load( XmlNodeRef node, CVarBlockPtr pVB );
	void Save( XmlNodeRef node, CVarBlockPtr pVB );

protected:
	CAGStateFactoryParam2( const IAnimationGraphStateFactoryIterator::SStateFactoryParameter& );

private:
	virtual IVariablePtr CreateVariable() = 0;

	bool m_bRequired;
	CString m_name;
	CString m_humanName;
	CString m_defaultValue;
	AnimationGraphUpgradeFunction m_upgrade;
};

template <class T>
class CAGStateFactoryParamImpl2 : public CAGStateFactoryParam2
{
public:
	CAGStateFactoryParamImpl2( const IAnimationGraphStateFactoryIterator::SStateFactoryParameter& p ) : CAGStateFactoryParam2(p) {}

private:
	virtual IVariablePtr CreateVariable()
	{
		return new CVariable<T>();
	}
};

class CAGStateFactory2;
typedef _smart_ptr<CAGStateFactory2> CAGStateFactory2Ptr;
class CAGStateFactory2 : public _reference_target_t
{
public:
	CAGStateFactory2( CAnimationGraph2 * pGraph, const IAnimationGraphStateFactoryIterator::SStateFactory& p );

	const CString& GetName() const { return m_name; }
	const CString& GetHumanName() const { return m_humanName; }
	CAGCategory2Ptr GetCategory() const { return m_pCategory; }

	void FillVarBlock( CVarBlockPtr pVB );
	void Load( XmlNodeRef node, CVarBlockPtr pVB );
	XmlNodeRef ToXml( CVarBlockPtr pVB );

private:
	CAGCategory2Ptr m_pCategory;
	CString m_name;
	CString m_humanName;
	std::vector<CAGStateFactoryParam2Ptr> m_params;
};

/*
enum EAGInputType
{
	eAGIT_Integereger,
	eAGIT_Float,
	eAGIT_String
};
*/

class CAGInput2 : public _reference_target_t
{
public:
	CAGInput2( CAnimationGraph2 * pGraph );
	CAGInput2( CAnimationGraph2 * pGraph, const CString& name );

	bool Load( XmlNodeRef node );
	XmlNodeRef ToXml();

	CAnimationGraph2 * GetGraph() { return m_pGraph; }

	const CString& GetName() const { return m_name; }
	bool CanUseMinMaxCriteria() const { return m_type != eAGIT_String; }
	EAnimationGraphInputType GetType() const { return m_type; }

	typedef std::set<CString>::const_iterator key_iterator;
	key_iterator KeyBegin() { return m_keyValues.begin(); }
	key_iterator KeyEnd() { return m_keyValues.end(); }

	bool HasKey(const CString& key) const { assert(m_type == eAGIT_String); return m_keyValues.find(key) != m_keyValues.end(); }
	void AddKey(const CString& key) { m_keyValues.insert(key); }
	void RemoveKey(const CString& key) { m_keyValues.erase(key); }

	TAGPropMap GetGeneralProperties();
	TAGPropMap GetFloatProperties();
	TAGPropMap GetIntegerProperties();
	std::vector<CString> GetKeyProperties();

private:
	CAnimationGraph2 * m_pGraph;
	EAnimationGraphInputType m_type;
	CString m_name;
	float m_floatMin;
	float m_floatMax;
	int m_intMin;
	int m_intMax;
	std::set<CString> m_keyValues;
	bool m_signalled;
	bool m_forceGuard;
	bool m_mixinFilter;
	CString m_defaultValue;
	int m_priority;
	CString m_attachToBlendWeight;
	float m_blendWeightMoveSpeed;

	void SetType( IVariable * pVar );
	void SetName();
};
typedef _smart_ptr<CAGInput2> CAGInput2Ptr;

class CAGLink2 : public _reference_target_t
{
public:
	CAGLink2( CAnimationGraph2 * pGraph );
	bool Load( XmlNodeRef node );
	XmlNodeRef ToXml();

	int GetForceFollowChance() const { return m_forceFollowChance; }
	void SetForceFollowChance( int chance ) { m_forceFollowChance = chance; }
	int	GetCost() const { return m_cost; }

	void GetLinkedStates( CAGState2Ptr& fromState, CAGState2Ptr& toState ) const;
	
	TAGPropMap GetGeneralProperties();
	TAGPropMap GetMappingProperties();
	void MappingChanged();

	CAnimationGraph2* GetGraph() const { return m_pGraph; }

private:
	CAnimationGraph2 * m_pGraph;
	int m_forceFollowChance;
	int m_cost;
	float m_overrideTransitionTime;

public:
	typedef std::list< std::pair<CString,CString> > TListMappedParams;
	TListMappedParams m_listMappedParams;
};
typedef _smart_ptr<CAGLink2> CAGLink2Ptr;

class CAGStateTemplate2;
typedef _smart_ptr<CAGStateTemplate2> CAGStateTemplate2Ptr;
class CAGStateTemplate2 : public _reference_target_t
{
public:
	CAGStateTemplate2() : m_name("Default") {}
	bool Load( XmlNodeRef node );
	bool HasParam( const CString& param ) const { return m_params.find(param) != m_params.end(); }
	bool IsSelected( const CString& criteria ) const { return m_selected.find(criteria) != m_selected.end(); }

	const std::map<CString, CString>& GetParams() const { return m_params; }

	CString GetName() const { return m_name; }

	// called only while loading
	bool ProcessParents( const CAnimationGraph2* pGraph );

private:
	CString m_name;
	std::map<CString, CString> m_params;
	std::set<CString> m_selected;
	CString m_extend;
};

class CAGState2;
typedef _smart_ptr<CAGState2> CAGState2Ptr;
class CAGState2 : public _reference_target_t
{
	struct CParamStateOverride;

public:
	struct CartesianProductHelper
	{
		typedef std::vector< std::pair< CString, const TSetOfCString* > > Sets;
		Sets sets;
		TParameterizationId2 defaults;

		void Make( std::vector< TParameterizationId2 >& result, Sets::const_iterator itSets )
		{
			if ( itSets == sets.end() )
				result.push_back( defaults );
			else
			{
				CString currentIndex = itSets->first;
				const TSetOfCString* currentValues = itSets->second;
				++itSets;

				for ( TSetOfCString::const_iterator it = currentValues->begin(); it != currentValues->end(); ++it )
				{
					defaults[ currentIndex ] = *it;
					Make( result, itSets );
				}
			}
		}
	};

	CAGState2( CAnimationGraph2 * pGraph );
	CAGState2( CAnimationGraph2 * pGraph, const CString& name );

	XmlNodeRef ToXml();

	//! Saves out the boolean attribute of whether this state has an animation controlled camera view.
	//! The function goes through all modifiers and if one of them is AnimControlledCamera
	//! it will save out true. Default is false.
	void SaveFPAnimControlledView( XmlNodeRef node );

	const CString& GetName() const { return m_name; }
	void SetName(CString newName);
	CAGState2Ptr GetParent() const { return m_pExtend; }
	CAnimationGraph2 * GetGraph() const { return m_pGraph; }
	bool AllowSelect() const { return m_graphNodeDetails.m_allowSelect; }
	bool IncludeInGame() const { return m_graphNodeDetails.m_includeInGame; }

	void SetIconSnapshotTime(float iconSnapshotTime) {m_iconSnapshotTime = iconSnapshotTime;}
	float GetIconSnapshotTime() const {return m_iconSnapshotTime;}

	bool IsNullState() const;

	bool Load( XmlNodeRef node );

	//! Needed to convert AG 1.0 graph into AG 1.5 and up
	//! Reads in the state templates (with single inheritance)
	//! and extracts the state's nodes
	bool ExtractNodesFromTemplates();

	//! Returns whether the template is asking for an animation name.
	//! Whether the template parameters actually specify an animation defined is not checked.
	bool HasAnimationSlotInTemplate() const;

	//! When loading an Animation Graph 1.0 graph, this method can extract
	//! the animation name (in most cases) from the template parameters so
	//! it can be stored in the proper AG 1.5+ place
	//! Animation Name can have parameters: [xyz]
	CString GetAnimationNameFromAG1Template( bool& foundName, const TParameterizationId2* pParamId = NULL ) const;

	//! This retrieves the animation name for the current state
	//! or parameterized state. Should the active parameterization have no override,
	//! it will retrieve the default animation.
	//! Animation Name can have parameters ([xyz])
	CString GetAnimationName() const;

	//! Retrieves the name of the currently active preset
	CString GetNodePresetName() const;

	//! This selects an actual Preset and copies all the values and sets the name.
	//! It calls ::SetPresetName in the process.
	void	SelectPreset(const SAnimNodeParams* pPreset);

	//! Setting the name of the active preset
	//! Just for easy access of a name. This is NOT selecting an active preset!
	//! Used inside the NodeDetails Pane implementation to set the name independent
	//! from the values.
	void	SetPresetName(const char* name);

	//! Will try to create a modifier out of the passed in xml node
	//! and add it to the list of linked ones. This respects Singletons
	//! and will check whether the node can actually be interpreted as a modifier.
	//! This is used in graph conversion from 1.0 --> 1.5 and newer
	void	TryLoadingModifier( XmlNodeRef stateNode, bool inherited = false );

	//! The state notes that it has this modifier linked to it
	//! and will call upon it for saving (exporting)
	//! If the modifier with this id is already linked, it will
	//! not be added again.
	//! the "inheritable" parameter is only relevant in graph conversion from
	//! Animation Graph 1.0, when Extra Properties could be inherited from parents
	void	AddLinkedModifier(uint modifierId, bool inheritable = false);

	//! Returns true if this Modifier is in the list of linked modifiers for this state
	bool	IsModifierLinked( uint modifierId );

	//! Backwards Compatibility: AG1.0 supported inheriting modifier-like nodes
	bool	IsLinkedModifierInheritable( uint modifierId );

	int		GetLinkedModifierCount();
	uint	GetLinkedModifier( int index );

	//! Removes this modifier's link to this state. The modifier
	//! will no longer be called upon for saving etc. by this state.
	void	RemoveLinkedModifier(uint modifierId);

	//! Save all modifier ids that this state is linked to
	void	SaveLinkedModifiersToXML( XmlNodeRef node );

	//! Load all modifier ids that this state is linked to
	void	LoadLinkedModifiers( XmlNodeRef node );

	//! Go through all linked modifiers and have them export their ag factory nodes
	void	ExportModifiers( XmlNodeRef node );

	//! Loads election Criteria from xml nodes and adds it to the existing ones.
	bool Load_SelectWhen( XmlNodeRef node );


	TAGPropMap GetGeneralProperties();
	TAGPropMap GetCriteriaProperties( CAnimationGraphStateEditor2 * pSE );
	TAGPropMap GetOverridableProperties( CAnimationGraphStateEditor2 * pSE );
	TAGPropMap GetExtraProperties();
	TAGPropMap GetTemplateProperties( CAnimationGraphStateEditor2 * pSE );

	ECriteriaType2 GetCriteriaType( CAGInput2Ptr pInput ) const;
	void SetCriteriaType( CAGInput2Ptr pInput, ECriteriaType2 type );
	CVarBlockPtr CreateCriteriaVariables( CAGInput2Ptr pInput, bool enabled = true, CParamStateOverride* pParameterization = NULL );

	CString GetStateFactoryMode( CAGCategory2Ptr pCategory ) const;
	void SetStateFactoryMode( CAGCategory2Ptr pCategory, const CString& mode );
	CVarBlockPtr GetStateFactoryVarBlock( CAGCategory2Ptr pCategory ) const;

	void AddExtraState( CAGStateFactory2Ptr );
	void RemoveExtraState( int i );
	void GetExtraStateStrings( std::vector<CString>& items ) const;

	IAGCheckedOperation2Ptr CreateDeleteOp();

	std::map<CString, CString>& GetTemplateParameters()
	{
		return m_pActiveParameterization ? m_pActiveParameterization->m_templateParams : m_templateParams;
	}
	void SetTemplateType( const CString& type );
	CString GetTemplateType() const { return m_pTemplate->GetName(); }
	bool IsSelectionDefinedInTemplate( const CAGInput2* pInput ) const { return m_pTemplate->IsSelected( pInput->GetName() ); }
	bool HasTemplateParam( const CString& name, bool failEmpty = true ) const
	{
		std::map<CString, CString>::const_iterator iter = m_templateParams.find(name);
		if (iter == m_templateParams.end())
			return false;
		if (failEmpty && iter->second.IsEmpty())
			return false;
		return true;
	}
	CString GetTemplateParameter( const CString& name, const TParameterizationId2* pParamId = NULL ) const
	{
		const std::map<CString, CString>* pTemplateParams = &m_templateParams;
		if ( pParamId )
		{
			TParameterizationMap::const_iterator it = m_paramsMap.find( *pParamId );
			if ( it != m_paramsMap.end() )
				pTemplateParams = &it->second.m_templateParams;
		}
		std::map<CString, CString>::const_iterator iter = pTemplateParams->find(name);
		if (iter == pTemplateParams->end())
			return "";
		return iter->second;
	}

	bool HasTemplateParameter( const CString& name, const TParameterizationId2* pParamId = NULL ) const
	{
		const std::map<CString, CString>* pTemplateParams = &m_templateParams;
		if ( pParamId )
		{
			TParameterizationMap::const_iterator it = m_paramsMap.find( *pParamId );
			if ( it != m_paramsMap.end() )
				pTemplateParams = &it->second.m_templateParams;
		}
		std::map<CString, CString>::const_iterator iter = pTemplateParams->find(name);
		if (iter == pTemplateParams->end())
			return false;
		return true;
	}

	void SetTemplateParameter( const CString& name, const CString& value )
	{
		m_templateParams[name] = value;
	}

	size_t GetFactoryCount()
	{
		return m_overridableStateFactories.size() + m_extraStateFactories.size();
	}

private:
	void OnChangeName();
	void OnChangeParent();
	void OnChangeUseTemplate();
	void OnChangeIconSnapshotTime();
	void NullifyParent() { m_pExtend = NULL; }

	CAnimationGraph2 * m_pGraph;
	CAGState2Ptr m_pExtend;
//	EMovementControlMethod m_movementControlMethodH;
//	EMovementControlMethod m_movementControlMethodV;
	bool m_bNoCollider;
	float m_iconSnapshotTime;

	int m_cost;
	float m_turnMul;

	std::set<CString> m_mixins;

	typedef std::map<CAGInput2Ptr, SCriteria2> TSelectWhenMap;
	TSelectWhenMap m_selectWhen;
	bool SelectWhen_ToXml( TSelectWhenMap& selectWhenMap, XmlNodeRef parentNode ) const;
	bool Load_SelectWhen( TSelectWhenMap& selectWhenMap, XmlNodeRef node ) const;

	struct SStateFactoryInfo
	{
		SStateFactoryInfo()
		{
		}
		SStateFactoryInfo( CAGStateFactory2Ptr pFactory )
		{
			this->pFactory = pFactory;
			this->pVarBlock = new CVarBlock();
			this->pFactory->FillVarBlock( this->pVarBlock );
		}
		void Load( XmlNodeRef node )
		{
			this->pFactory->Load( node, pVarBlock );
		}
		CAGStateFactory2Ptr pFactory;
		CVarBlockPtr pVarBlock;
	};

	std::map<CAGCategory2Ptr, SStateFactoryInfo> m_overridableStateFactories;
	std::vector<SStateFactoryInfo> m_extraStateFactories;

public:


	// the following is needed for backwards compatibility, to convert old graph data into new format
	// ======================================================================
	//! Retrieves a parameter, either from the xml file, or from the exposed parameters, if it contains a $
	bool GetParamFromTemplate( XmlNodeRef refNode, CString paramName, CString &paramValue ) const;
	bool GetParamFromTemplate( XmlNodeRef refNode, CString paramName, int &paramValue ) const;
	bool GetParamFromTemplate( XmlNodeRef refNode, CString paramName, float &paramValue ) const;
	bool GetParamFromTemplate( XmlNodeRef refNode, CString paramName, bool &paramValue ) const;
	// ======================================================================


private:
	// =======================================
	// General Node Details

	//! Name of this state as given by the user
	CString m_name;

	//! Name of the Preset used for setting these data
	//! as stored in the preset vector
	CString	m_animParamsPresetName;

	std::vector<uint>				 m_linkedModifiers;
	
	//! Backwards compatibility: relevant for conversion of AG1.0 parenting
	//! This will become invalid after loading is done
	std::vector<uint>				 m_inheritableModifiers;

	SGraphNodeDetails				 m_graphNodeDetails;
	SAnimNodeDetails				 m_generalNodeDetails;
	SMovementParams					 m_movementNodeDetails;
	TPerParameterizationData m_unparameterizedDetails;

	// =======================================

	// old templates' stuff
	CAGStateTemplate2Ptr m_pTemplate;
	std::map<CString, CString> m_templateParams;


	// parameterization
	CParamsDeclaration2 m_paramsDeclaration;

	struct CParamStateOverride
	{
		CParamStateOverride() 
			: m_bExcludeFromGraph(false) 
		{}

		bool m_bExcludeFromGraph;
		TPerParameterizationData m_nodeDetails;
		TSelectWhenMap m_selectWhen;

		// TODO: obsolete - will be removed
		std::map< CString, CString > m_templateParams;
	};

	typedef std::map< TParameterizationId2, CParamStateOverride > TParameterizationMap;
	TParameterizationMap m_paramsMap;
	CParamStateOverride m_MultipleStateOverride;
	CParamStateOverride* m_pActiveParameterization;
	TParameterizationId2 m_ActiveParameterizationId;
	typedef std::vector< TParameterizationId2 > TVectorParamIds;
	TVectorParamIds m_vParameterizationIds;
	CString m_activeViewName;

public:
	bool m_bMixedExcludeFromGraph;
	CParamsDeclaration2 * GetParamsDeclaration() { return &m_paramsDeclaration; }
	void ActivateParameterization( const TParameterizationId2* pParameterizationId, const CString& viewName = "" );
	CParamStateOverride* GetActiveParameterization() const { return m_pActiveParameterization; }

	//! Returns a pointer to the details data for the active parameterization
	//! If the state is not parameterized, or this parameterization doesn't have
	//! it's own details, NULL is returned.
	//! NOTE: Some values might be empty, because they are not overridden. 
	//!				In this case the the unparameterized data
	TPerParameterizationData* GetActivePerParamDetails() { return (m_pActiveParameterization)? &m_pActiveParameterization->m_nodeDetails : NULL; }

	//! Returns a pointer to the details data that can be overridden in parameterizations
	TPerParameterizationData* GetUnparamedPerParamDetails()	{ return &m_unparameterizedDetails; }

	//! Retrieves the default values from the animation graph and passes them through
	const SAnimNodeDetails* GetGraphDefaultAnimNodeDetails() const;

	//! Retrieves the default values from the animation graph
	const SMovementParams*	GetGraphDefaultMovementDetails() const;

	//! Returns the specific node details for this node, that might differ from the default ones (above)
	inline SAnimNodeDetails* GetAnimNodeDetails() { return &m_generalNodeDetails; }

	//! The graph node details are the same for all node types and not animation-node specific
	inline SGraphNodeDetails* GetGraphNodeDetails() { return &m_graphNodeDetails; }

	//! Returns the pointer to this node's mcm and collider mode details
	inline SMovementParams*	GetMovementNodeDetails() { return &m_movementNodeDetails; }

	void ResetParameterization() { ActivateParameterization( NULL ); m_paramsMap.clear(); }
	bool IsParameterized() const { return !m_paramsMap.empty(); }
	void OptimizeParameterization();
	int GetExcludeFromGraph() const;
	void SetExcludeFromGraph( bool bExclude );
	bool IsParameterizationExcluded( const TParameterizationId2* pParameterizationId );
	void GetParameterizations( std::vector< TParameterizationId2 >& paramIds );

	// state parameters manipulation
	bool AddParamValue( const char* param, const char* value );
	bool DeleteParamValue( const char* param, const char* value );
	bool RenameParamValue( const char* param, const char* oldValue, const char* newValue );
	bool AddParameter( const CString& name );
	bool DeleteParameter( const CString& name );
	bool RenameParameter( const CString& oldName, const CString& newName );

};

class CAGNode2;
class CAG2ModifierNode;
class CAG2ModifierBase;
class CAGNodeBase2;
typedef CAG2ModifierBase* CAG2ModifierBasePtr;

class CAGView2 : public _reference_target_t, public IHyperGraphListener
{
public:
	CAGView2( CAnimationGraph2 * pGraph );
	CAGView2( CAnimationGraph2 * pGraph, const CString& name );
	virtual ~CAGView2();

	bool Load( XmlNodeRef node, float agVersion = 1.0f );
	XmlNodeRef ToXml();

	const CString& GetName() const { return m_name; }
	CAnimationGraph2 * GetGraph() const { return m_pGraph; }
	_smart_ptr<CHyperGraph> GetHyperGraph();
	void OnHyperGraphEvent( IHyperNode * pNode, EHyperGraphEvent event );

	//! Cannot add the exact same state twice into the view
	bool CanAddState( CAGState2Ptr );
	void RemoveState( CAGState2Ptr );
	bool HasState( CAGState2Ptr );
	CAGNode2* GetNodeForState(CAGState2Ptr pState);

	//! Cannot add the exact same modifier twice into the view
	bool CanAddModifier( CAG2ModifierBasePtr );
	bool HasModifier( CAG2ModifierBasePtr );
	void RemoveModifier( CAG2ModifierBasePtr );
	HyperNodeID GetNodeIdForModifier(CAG2ModifierBasePtr pState);

	TAGPropMap GetGeneralProperties();

	void CreateNodesForState(CAGState2Ptr pState, CAGNode2 * pNode, bool bSelected = false);
	void PositionNodesForState(CAGState2Ptr pState, CAGNode2 * pNode);

private:
	void OnChangeName();

	struct SStateInfo
	{
		SStateInfo() : pos(0,0), hid(-1) {}
		SStateInfo( HyperNodeID id ) : pos(0,0), hid(id) {}
		Gdiplus::PointF pos;
		HyperNodeID hid;
	};

	//! Stores the new hypernode position in the node data (so it can be saved/loaded)
	void UpdateNodePosition( CAG2ModifierNode* pModifierNode );

	//! Goes through all the AG states in the view and checks if any are linked
	//! to this state - if so, create hyperlinks for them 
	void MakeConnectionsInView(CAG2ModifierNode* pModifierNode);

	//! After placing a state in a view it will create visible arrow edges to all the modifiers
	//! that are already in this view and are linked with the state
	void MakeConnectionsFromStateToModifiersInView( CAGNodeBase2* pAGNode );

	typedef std::map<CAGState2Ptr, SStateInfo> StateMap;
	typedef std::map<CAG2ModifierBasePtr, SStateInfo> ModifierMap;
	CAnimationGraph2 * m_pGraph;
	CString m_name;
	StateMap m_states;
	ModifierMap m_modifiers;
	_smart_ptr<CHyperGraph> m_pLastCreatedGraph;
};
typedef _smart_ptr<CAGView2> CAGView2Ptr;

enum AGNodeType2
{
	AG_NODE_MAIN2,
	AG_NODE_IMAGE2,
	AG_NODE_MODIFIER
};

class CAGNodeBase2 : public CHyperNode
{
public:
	CAGNodeBase2(CAGState2Ptr pState);

	virtual AGNodeType2 GetAGNodeType() = 0;

	CAGState2Ptr GetState() { return m_pState; }

protected:
	CAGState2Ptr m_pState;
};

class CAGImageNode2;
class CAGNode2 : public CAGNodeBase2
{
public:
	CAGNode2( CHyperGraph * pGraph, HyperNodeID id, CAGState2Ptr pState );
	// CHyperNode
	virtual void Init() {}
	virtual void Done() {}
	virtual CHyperNode * Clone()
	{
		return new CAGNode2(m_pGraph, m_id, m_pState);
	}

	void AttachImageNode(CAGImageNode2* pImageNode);

	virtual AGNodeType2 GetAGNodeType() {return AG_NODE_MAIN2;}

	virtual IHyperGraphEnumerator* GetChildrenEnumerator()
	{ 
		ImageNodeCollection::iterator begin = m_imageNodes.begin();
		ImageNodeCollection::iterator end = m_imageNodes.end();
		return new CHyperGraphIteratorEnumerator<ImageNodeCollection::iterator>(begin, end);
	}

private:
	typedef std::vector<CAGImageNode2*> ImageNodeCollection;
	std::vector<CAGImageNode2*> m_imageNodes;
};

class CAGImageNode2 : public CAGNodeBase2
{
public:
	CAGImageNode2( CHyperGraph * pGraph, HyperNodeID id, CAGState2Ptr pState );
	// CHyperNode
	virtual void Init() {}
	virtual void Done() {}
	virtual CHyperNode * Clone()
	{
		return new CAGNode2(m_pGraph, m_id, m_pState);
	}

	void SetParent(CAGNode2* pNode) {m_pParent = pNode;}
	void SetImage(CAnimationImage2Ptr pImage) {m_pImage = pImage;}
	CAnimationImage2Ptr GetImage() {return m_pImage;}

	virtual AGNodeType2 GetAGNodeType() {return AG_NODE_IMAGE2;}

	virtual IHyperNode* GetParent() { return m_pParent; }

private:
	CAGNode2* m_pParent;
	CAnimationImage2Ptr m_pImage;
};

class CAGEdge2 : public CHyperEdge
{

public:
	CAGEdge2();

	bool IsEditable();
	void SetToModifierLink(bool isModifier = true);

	virtual int	 GetCustomSelectionMode();

	void DrawSpecial(Gdiplus::Graphics * pGr, Gdiplus::PointF where);
	CAGLink2Ptr pLink;

private:
	bool m_isModifierLink;
};

enum EAGStateEvent2
{
	eAGSE_ChangeName2,
	eAGSE_ChangeParent2,
	eAGSE_ChangeUseTemplate2,
	eAGSE_Removed2,
	eAGSE_ChangeIconSnapshotTime2,
	eAGSE_CanNotChangeName2,
	eAGSE_Cloned2,
};

enum EAG2ModifierEvent
{
	eAG2_Modifier_Delete,
};

enum EAGViewEvent2
{
	eAGVE_ChangeName2,
	eAGVE_Removed2,
};

enum EAGInputEvent2
{
	eAGIE_ChangeName2,
	eAGIE_ChangeType2,
	eAGIE_Removed,
};

enum EAGLinkEvent2
{
	eAGLE_ChangeMapping2,
};

struct IAnimationGraphListener2
{
	virtual void OnStateEvent( EAGStateEvent2, CAGState2Ptr pState ) {}
	virtual void OnModifierEvent( EAG2ModifierEvent, CAG2ModifierBasePtr pModifier ) {}
	virtual void OnViewEvent( EAGViewEvent2, CAGView2Ptr pView ) {}
	virtual void OnInputEvent( EAGInputEvent2, CAGInput2Ptr pInput ) {}
	virtual void OnLinkEvent( EAGLinkEvent2, CAGLink2Ptr pLink ) {}
	virtual void OnGraphModified() {}
};

class CAnimationGraphDialog2;
class CAnimationGraph2 : public CHyperGraphManager, public _reference_target_t
{
public:
	CAnimationGraph2(CAnimationImageManager2Ptr pImageManager, CAnimationGraphDialog2 * pParent);
	~CAnimationGraph2();

	// CHyperGraphManager
	void ReloadClasses();
	CHyperGraph * CreateGraph();
	CHyperNode * CreateNode( CHyperGraph* pGraph,const char *sNodeClass,HyperNodeID nodeId, const Gdiplus::PointF& pos=Gdiplus::PointF(0.0f, 0.0f), CBaseObject* pObj = NULL );
	// ~CHyperGraphManager

	bool Load( const CString& filename );
	void SetFilename( const CString& filename ) { m_filename = filename; }
	CString GetFilename() const { return m_filename; }
	XmlNodeRef ToXml();
	void Save();

	//! Creates all things needed as a bare minimum for the graph to export and work ingame.
	//! AG 1.0-1.5: One state and one input, which is assigned to the state.
	//! This function is called when "New" is selected from the graph menu.
	void CreateMinimumGraph();

	void AddListener( IAnimationGraphListener2 * pListener );
	void RemoveListener( IAnimationGraphListener2 * pListener );

	bool AddInput( CAGInput2Ptr );
	bool RemoveInput( CAGInput2Ptr );
	CAGInput2Ptr FindInput( const CString& ) const;
	bool AddState( CAGState2Ptr );
	bool CloneState( CAGState2Ptr );
	void RemoveState( CAGState2Ptr );
	CAGState2Ptr FindState( const CString& ) const;
	bool AddLink( CAGState2Ptr from, CAGState2Ptr to, CAGLink2Ptr link );
	bool RemoveLink( CAGState2Ptr from, CAGState2Ptr to );
	CAGLink2Ptr FindLink( CAGState2Ptr from, CAGState2Ptr to ) const;
	void FindLinkedStates( const CAGLink2* link, CAGState2Ptr& from, CAGState2Ptr& to ) const;

	// Modifier functions
	// ====================================

	bool AddModifier( CAG2ModifierBasePtr newModifier );
	CAG2ModifierBasePtr GetModifier( const uint modifierId );

	//! Asks the ModifierManager to create a new Modifier of this class
	//! If the modifier is of the singleton type and already exists in the graph,
	//! the already existing one will be taken instead.
	CAG2ModifierBasePtr AddNewModifier( const CString& className );

	void RemoveModifier( CAG2ModifierBasePtr ptr );

	int TestLoadModifierFromOldGraph( XmlNodeRef stateNode, CAGState2Ptr pAnimNode );

	// ====================================
	// End of Modifier functions

	bool IsLinked( CAGState2Ptr from, CAGState2Ptr to ) const;
	CAGCategory2Ptr FindCategory( const CString& ) const;
	CAGStateFactory2Ptr FindStateFactory( const CString& ) const;
	bool IsObsoleteFactory( const CString& ) const;
	bool AddView( CAGView2Ptr );
	bool RemoveView( CAGView2Ptr );
	CAGView2Ptr FindView( const CString& ) const;

	SAnimNodeDetails*		GetAnimationNodeDefaultDetails() { return &m_graphDefaultAnimationNodeDetails; }
	SGraphNodeDetails*	GetNodeDefaultDetails() { return &m_graphDefaultNodeDetails; }
	SMovementParams*		GetDefaultMovementDetails() { return &m_graphDefaultMovementDetails; }

	typedef std::set<CAGState2Ptr>::const_iterator state_iterator;
	state_iterator StateBegin() const { return m_states.begin(); }
	state_iterator StateEnd() const { return m_states.end(); }

	typedef std::set<CAGInput2Ptr>::const_iterator input_iterator;
	input_iterator InputBegin() const { return m_inputs.begin(); }
	input_iterator InputEnd() const { return m_inputs.end(); }

	typedef std::vector<CAGCategory2Ptr>::const_iterator category_iterator;
	category_iterator CategoryBegin() const { return m_categoryVec.begin(); }
	category_iterator CategoryEnd() const { return m_categoryVec.end(); }

	typedef std::vector<CAGStateFactory2Ptr>::const_iterator state_factory_iterator;
	state_factory_iterator StateFactoryBegin() const { return m_stateFactoryVec.begin(); }
	state_factory_iterator StateFactoryEnd() const { return m_stateFactoryVec.end(); }

	typedef std::set<CAGView2Ptr>::const_iterator view_iterator;
	view_iterator ViewBegin() const { return m_views.begin(); }
	view_iterator ViewEnd() const { return m_views.end(); }

	typedef std::map<uint, CAG2ModifierBasePtr> ModifierMap;
	typedef ModifierMap::const_iterator modifier_iterator;
	modifier_iterator ModifiersBegin() const { return m_modifiers.begin(); }
	modifier_iterator ModifiersEnd() const { return m_modifiers.end(); }

	typedef std::set<CAGStateTemplate2Ptr>::const_iterator state_template_iterator;
	state_template_iterator StateTemplateBegin() const { return m_templates.begin(); }
	state_template_iterator StateTemplateEnd() const { return m_templates.end(); }
	CAGStateTemplate2Ptr GetDefaultTemplate() const; 
	CAGStateTemplate2Ptr GetTemplate( const CString& name ) const;

	void ChildStates( std::vector<CAGState2Ptr>& states, CAGState2Ptr parent );
	void StatesLinkedTo( std::vector<CAGState2Ptr>& states, CAGState2Ptr to );
	void StatesLinkedFrom( std::vector<CAGState2Ptr>& states, CAGState2Ptr from );
	void StateLinks( std::vector<CAGLink2Ptr>& links, CAGState2Ptr state, bool includeLinksTo, bool includeLinksFrom );

	CString OnChangedStateName( CAGState2Ptr ptr );
	CString OnChangedViewName( CAGView2Ptr ptr );
	CString OnChangedInputName( CAGInput2Ptr ptr );
	void SendStateEvent( EAGStateEvent2, CAGState2Ptr );
	void SendViewEvent( EAGViewEvent2, CAGView2Ptr );
	void SendInputEvent( EAGInputEvent2, CAGInput2Ptr );
	void SendLinkEvent( EAGLinkEvent2, CAGLink2Ptr );
	void SendModifierEvent( EAG2ModifierEvent, CAG2ModifierBasePtr );

	bool IsModified() const { return m_modified; }
	void Modified();

	typedef std::pair<CAGState2Ptr, CAGState2Ptr> TLink;
	typedef std::vector<TLink> TLinkVec;
	void GetLinks( TLinkVec& links );

	//! the name of the file to the cdf character used for animation loading
	//! and preview (passed on to the preview dialog)
	const CString& GetCharacterName() const { return m_sCharacterName; }
	void SetCharacterName( const char* szCharacterName );

	const bool IsUsingExpensiveExpensiveLocoStateSelection() const { return m_useExpensiveLocoStateSelection; }
	void SetUseExpensiveLocoStateSelection(bool use) { m_useExpensiveLocoStateSelection = use; }

	//! Preset settings for animation nodes
	//! These include general and animation specific settings
	const SAnimNodeParams* GetAnimNodePreset(int idx) const;
	int	GetAnimNodePresetCount() const;

	CAnimationImageManager2Ptr GetImageManager() {return m_pImageManager;}

	CModifierManager* GetModifierManager() const;

	void ShowIcons(bool showIcons);
	bool IsShowingIcons() {return m_bShowIcons;}

private:
	template <class T>
	CString OnChangedName( T ptr, std::map<CString, T>& byName );

	void LoadTemplates();

	//! Save the modifier instances for this graph to the xml file
	void SaveActiveModifiersToXML(XmlNodeRef parent);

	//! Loads the modifier instances active in this graph
	void LoadActiveModifiers(XmlNodeRef parent);

	//! Retrieves the first modifier of the given class in the graph,
	//! for example to check whether this is a singleton modifier that already exists
	CAG2ModifierBasePtr GetFirstModifierOfClass( const CString& className ) const;

	//! Reset the Animation parameter presets vector and fill with the 
	//! default presets
	void ResetAnimParamsPreset();

	CString m_filename;
	bool m_modified;

	//! This is for backward compatibility only. 
	//! It turns on the usage of SelectLocomotionState function (expensive and unreliable, but used in Crysis 1)
	bool m_useExpensiveLocoStateSelection;

	// default node settings (presented in menu->settings->default settings)
	SAnimNodeDetails					m_graphDefaultAnimationNodeDetails;
	SGraphNodeDetails					m_graphDefaultNodeDetails;
	SMovementParams						m_graphDefaultMovementDetails;
	AnimParamsPresetsVector		m_animNodeParamsPresets;
	ModifierMap								m_modifiers;
	CModifierManager*					m_modifierManager;

	std::set<CAGInput2Ptr> m_inputs;
	std::map<CString, CAGInput2Ptr> m_inputsByName;
	std::set<CAGState2Ptr> m_states;
	std::map<CString, CAGState2Ptr> m_statesByName;
	std::map<TLink, CAGLink2Ptr> m_links;
	std::set<CAGView2Ptr> m_views;
	std::map<CString, CAGView2Ptr> m_viewsByName;
	std::set<CAGStateTemplate2Ptr> m_templates;
	std::map<CString, CAGStateTemplate2Ptr> m_templatesByName;

	std::vector<IAnimationGraphListener2*> m_listeners;

	CString m_sCharacterName;
	CAnimationImageManager2Ptr m_pImageManager;
	bool m_bShowIcons;

	// metadata... kept here because I can't think of a better place to keep it
	std::vector<CAGCategory2Ptr> m_categoryVec;
	std::map<CString, CAGCategory2Ptr> m_categories;
	std::map<CString, CAGStateFactory2Ptr> m_stateFactories;
	std::vector<CAGStateFactory2Ptr> m_stateFactoryVec;

};
typedef _smart_ptr<CAnimationGraph2> CAnimationGraph2Ptr;

template <class T>
CString GenerateName( CAnimationGraph2* pGraph, T (CAnimationGraph2::*func)(const CString&) const, const CString& base )
{
	for (int i=0; ; i++)
	{
		CString temp;
		temp.Format("%s%d", (const char*)base, i);
		if (!(pGraph->*func)(temp))
			return temp;
	}
}

#endif
