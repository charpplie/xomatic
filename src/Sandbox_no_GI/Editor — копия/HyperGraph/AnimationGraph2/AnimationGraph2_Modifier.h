/************************************************************************/
/*     Represents a modifier node (like ragdoll, events etc...)         */
/************************************************************************/


#ifndef __ANIMATIONGRAPH2_MODIFIER_NODE_H__
#define __ANIMATIONGRAPH2_MODIFIER_NODE_H__

#pragma once

#include "Resource.h"
//#include "Controls\TemplDef.h" // message map extensions for templates
//#include "ToolbarDialog.h"
//#include "Controls\PropertyCtrl.h"

//#include "CharacterEditor\CharacterEditor.h"
#include "AnimationGraph_2.h"
#include "..\HyperGraphView.h"


//////////////////////////////////////////////////////////////////////////
//Modifier Base class - DERIVE from this to implement your own modifiers//
//////////////////////////////////////////////////////////////////////////

class CAG2ModifierBase : public CDialog
{
	friend class CModifierManager;

protected:
	// Only the ModifierManager can create these
	CAG2ModifierBase();

public:
	~CAG2ModifierBase();

	//////////////////////////////////////////////////////////////////////////
	// Interface functions that derived classes must implement

	//! The unique type of the modifier in human readable formatting
	virtual const CString GetHumanReadableName() = 0;

	//! Unique class type name - no whitespaces please
	virtual const CString GetClassName() = 0;

	//! Save this instances data to an xml file so the modifier can
	//! be recreated later (just save your data, no export/node creation stuff) 
	virtual const void Save(XmlNodeRef modifierNode) const = 0;

	//! Load the instance data from an xml node
	//! This function is supposed to read in what ::Save wrote out before
	virtual const void Load(XmlNodeRef modifierNode) = 0;

	//! Export the node that is to be imported by the graph binary later (no metadata)
	virtual const void Export( XmlNodeRef node ) const = 0;

	//! Create a new instance of yourself 
	virtual CAG2ModifierBase* Duplicate() const = 0;


	// Optional if this Modifier has customizable Parameters
	// ==============================

	//! Returns the MFC IDD of the Dialog to display
	//! If this is IDD_AG2_MODIFIER_DEFAULT, this Modifier has no customizable properties
	virtual int GetDialogIDD() { return IDD_AG2_MODIFIER_DEFAULT; }

	//! Gets called after the modifier node is selected and the panel with the
	//! additional Parameters is shown. Use to initialize values etc
	virtual void InitParameterPanel() {}


	// Other Optional functions to override
	// ==============================

	//! Init function gets called after the ID has been set, but before Loading
	//! Not needed by most modifiers.
	virtual void Init() {};

	//! Return a custom text for this instance to be displayed in the graph node
	virtual const CString GetCustomText() const;

	//! If the modifier says it is a singleton, it will only be created once per graph
	//! and all nodes will reference the same one
	//! If you have no member data, return true here, otherwise leave as is.
	//! See for examples: WaitForStandUp vs. Comment Modifier
	virtual const bool IsSingleton() const { return false; }


	// AG 1.0 Conversion functions - optional
	// (Most likely you do not need to implement these for your custom modifiers)
	// ==============================

	//! Upon conversion of older graph version this function is called with various
	//! XMLNodes for questioning if this node could understand and possibly convert them.
	//! This function does not do the actual conversion, just returns a bool on
	//! whether it could.
	virtual bool CanConvertFromOldGraphVersion( const XmlNodeRef node ) const { return false; }

	//! Converts the data from an AG1.0 version graph into this modifier's data
	//! This function is called after CanConvertFromOldGraphVersion returns true.
	//! The parameter pAnimNode is passed along to give access to template parameters
	virtual void ConvertFromOldStateNode( const XmlNodeRef node, CAGState2Ptr pAnimNode ) {}



	//////////////////////////////////////////////////////////////////////////
	// Don't do anything to these functions below

	//! Constructs the string visible inside the modifier box in the view
	const CString GetNodeDisplayString();
	const CString GetListDisplayString();

	uint GetId() const;
	XmlNodeRef ToXml();

private:

	void SetId(uint id);

	uint m_id;
};




//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
//                           Modifier Manager                           //
//////////////////////////////////////////////////////////////////////////

//! This class takes care of loading/saving and instantiating the
//! modifiers. New modifiers need to add themselves in here to
//! be available in the editor. 
//! Just add your custom modifier in the Init() function. (and ONLY there please)
class CModifierManager : public CDialog
{
public:

	CModifierManager();
	~CModifierManager();

	// Methods
	//===================

	//! Register all modifier types in here
	void Init( CAnimationGraphDialog2* pParent );

	//! How many modifiers are known and registered
	int						 GetRegisteredModifierCount() const;

	//! Retrieves the class name of the registered modifier
	//! with the index in the list of known modifiers (This is NOT the modifier ID)
	const CString& GetModifierClassName(int listIndex) const;

	//! Retrieves the human readable name of the registered modifier (might contain whitespaces!)
	//! with the index in the list of known modifiers (This is NOT the modifier ID)
	CString GetModifierHumanReadableName(int listIndex) const;

	//! Creates a new Modifier instance of the correct type
	//! and loads its custom data from the node - and returns the new node
	//! Called upon when loading an animation graph and finding a modifier node.
	CAG2ModifierBase* LoadModifier(XmlNodeRef parent);

	//! Creates a new modifier of the given class
	//! It will not check whether this class is a singleton and already exists in any particular graph
	CAG2ModifierBase* CreateModifier(const CString& className);

	//! When a node gets selected in the View or in the Modifier List, this
	//! function will be called so that the details pane can be displayed - if there
	//! is one
	void	SetSelectedModifier(CAG2ModifierBasePtr pModifier);

	//! Will check whether this node is something that could be converted into
	//! a modifier node.
	//! If successful this will return the className of the Modifier that can load
	//! from this node.
	//! This is used upon loading an old version graph and conversion to new system.
	const CString& GetClassNameFromOldVersionStateNode( const XmlNodeRef stateNode ) const;

private:

	// Methods
	//===================

	//! Adds this modifier to the list of available modifier types
	bool AddModifierToClassMap(CAG2ModifierBasePtr newModifier);

	// Members
	//===================

	typedef std::map<CString, CAG2ModifierBasePtr> ModifierClassMap;
	CAG2ModifierBasePtr m_pSelectedModifier;
	ModifierClassMap m_modifierClasses;
	
	CAnimationGraphDialog2* m_pParent;
	uint m_highestLoadedId;
	uint m_nextID;
};


//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
//  HyperGraph Node for the Modifiers (you don't need to touch this)    //
//////////////////////////////////////////////////////////////////////////

class CAG2ModifierNode : public CAGNodeBase2
{
public:
	CAG2ModifierNode(CHyperGraph* pGraph, HyperNodeID id, CAG2ModifierBasePtr pModifier);
	~CAG2ModifierNode() {}

	//! returns the linked modifier
	CAG2ModifierBasePtr	GetModifier() const { return m_pModifier; }

	// HyperNode/NodeBase Functions - usually no need to modify these
	virtual void Init() {}
	virtual AGNodeType2 GetAGNodeType();
	virtual CHyperNode * Clone();
	virtual int	GetCustomSelectionMode() { return 5; }
	CAGState2Ptr GetState();

private:
	CAG2ModifierBasePtr m_pModifier;
};

//////////////////////////////////////////////////////////////////////////
//         Modifier Node Painter (you don't need to touch this)         //
//////////////////////////////////////////////////////////////////////////

#include "../IHyperNodePainter.h"

class CHyperNodePainter_Modifier : public IHyperNodePainter
{
public:
	virtual void Paint( CHyperNode * pNode, CDisplayList * pList );
	void PaintModifier( CHyperNode * pNode, CDisplayList * pList );
};


#endif  // __ANIMATIONGRAPH2_MODIFIER_NODE_H__

