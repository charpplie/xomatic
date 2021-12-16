#include "StdAfx.h"
#include "..\StringDlg.h"
#include "IViewPane.h"
#include "resource.h"
//#include "ICryAnimation.h"
//#include "PropertiesPanel.h"
#include "AnimationGraph_2.h"
#include "AnimationGraph2_Modifier.h"
#include "AnimationGraphDialog_2.h"

#include "Modifier Nodes/Additive.h"
#include "Modifier Nodes/AnimControlledCamera.h"
#include "Modifier Nodes/AttachmentEffect.h"
#include "Modifier Nodes/Comment.h"
#include "Modifier Nodes/DisableLookIK.h"
#include "Modifier Nodes/Event.h"
#include "Modifier Nodes/FallAndPlay.h"
#include "Modifier Nodes/Output.h"
#include "Modifier Nodes/Ragdoll.h"
#include "Modifier Nodes/SetInput.h"
#include "Modifier Nodes/TimeAlignmentGroup.h"
#include "Modifier Nodes/WaitforStandUp.h"
#include "Modifier Nodes/WaitForUBAllowed.h"
#include "Modifier Nodes/WaitForFalling.h"
#include "Modifier Nodes/PlayFacialSequence.h"
#include "Modifier Nodes/PlaySound.h"


#define TYPE_STRING "Type"
#define ID_STRING "Id"


//////////////////////////////////////////////////////////////////////////
//Modifier Base class - DERIVE from this to implement your own modifiers//
//////////////////////////////////////////////////////////////////////////

CAG2ModifierBase::CAG2ModifierBase( )
{
	m_id = -1;
}

CAG2ModifierBase::~CAG2ModifierBase()
{

}

uint CAG2ModifierBase::GetId() const
{
	return m_id;
}

void CAG2ModifierBase::SetId( uint id )
{
	m_id = id;
}

XmlNodeRef CAG2ModifierBase::ToXml()
{
	// For safety use a generic node name
	// instead of the class name (it could contain whitespaces after all!)
	XmlNodeRef newNode = CreateXmlNode( "Modifier" );

	// write name and id of this modifier
	newNode->setAttr(TYPE_STRING, GetClassName());
	newNode->setAttr(ID_STRING, GetId());

	// Call the derived class' save function so it can add it's data too
	Save(newNode);

	return newNode;
}

const CString CAG2ModifierBase::GetCustomText() const
{
	CString idString;
	idString.Format("%i", GetId());
	return idString;
}

const CString CAG2ModifierBase::GetNodeDisplayString()
{
	CString displayString;

	if (IsSingleton())
	{
		displayString.Format("%s\n ", GetHumanReadableName());
	}
	else
	{
		displayString.Format("%s\n%s", GetHumanReadableName(), GetCustomText());
	}

	return displayString;
}

const CString CAG2ModifierBase::GetListDisplayString()
{
	CString displayString;

	if (IsSingleton())
	{
		displayString.Format("%s", GetHumanReadableName());
	}
	else
	{
		displayString.Format("%s - (%s)", GetHumanReadableName(), GetCustomText());
	}

	return displayString;
}



//////////////////////////////////////////////////////////////////////////
//                           Modifier Manager                           //
//////////////////////////////////////////////////////////////////////////

void CModifierManager::Init( CAnimationGraphDialog2* pParent )
{
	// Create one default instance of each modifier
	m_pParent = pParent;
	m_highestLoadedId = 0;
	m_nextID = 0;
	
	// Add your new modifiers to this list here if you created your own
	AddModifierToClassMap(new CAG2Modifier_Additive());
	AddModifierToClassMap(new CAG2Modifier_AnimControlledCamera());
	AddModifierToClassMap(new CAG2Modifier_AttachmentEffect());
	AddModifierToClassMap(new CAG2Modifier_Comment());
	AddModifierToClassMap(new CAG2Modifier_DisableLookIK());
	AddModifierToClassMap(new CAG2Modifier_FacialSequence());
	AddModifierToClassMap(new CAG2Modifier_FallAndPlay());
	AddModifierToClassMap(new CAG2Modifier_Output());
	AddModifierToClassMap(new CAG2Modifier_PlaySound());
	AddModifierToClassMap(new CAG2Modifier_Ragdoll());
	AddModifierToClassMap(new CAG2Modifier_SetInput());
	AddModifierToClassMap(new CAG2Modifier_TimeAlignGroup());
	AddModifierToClassMap(new CAG2Modifier_WaitForFalling());
	AddModifierToClassMap(new CAG2Modifier_WaitForStandUp());
	AddModifierToClassMap(new CAG2Modifier_WaitForUBAllowed());
	
	// Please do not move this up above FallAndPlay and RagDoll, as it will look
	// prettier upon converting an old version graph to let these ones handle specific
	// events instead of the generalized event modifier
	AddModifierToClassMap(new CAG2Modifier_Event());  

	// Create
	Create(IDD_AG2_MODIFIER_DEFAULT, pParent->GetNodeDetailsParent() );
	SetOwner(pParent);
	ShowWindow(SW_HIDE);
}


CAG2ModifierBase* CModifierManager::LoadModifier( XmlNodeRef parent )
{
	if (!parent->haveAttr(TYPE_STRING))
		return NULL;

	if (!parent->haveAttr(ID_STRING))
		return NULL;

	// get basic information about this modifier
	int	id = -1;
	parent->getAttr(ID_STRING, id);
	CString typeName = parent->getAttr(TYPE_STRING);

	// parameter validity checking
	if (typeName.IsEmpty())
	{
		CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, "Modifier has no class type and cannot be loaded.");
		return NULL;
	}
	if (id < 0)
	{
		CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, "Modifier %s has an invalid id and will not be loaded.", typeName);
		return NULL;
	}

	// Double check that this is a registered and known modifier type 
	if (m_modifierClasses.find(typeName) == m_modifierClasses.end())
	{
		CryWarning(VALIDATOR_MODULE_ANIMATION, VALIDATOR_WARNING, "Modifier Type %s is unknown. Skipped loading.", typeName);
		return NULL;
	}

	// Now actually create a new modifier of the given type
	// and let it load its data from the node
	CAG2ModifierBase* newModifier = CreateModifier(typeName);
	newModifier->SetId(id);
	newModifier->Load(parent);

	// Adjust highest and next id (Next ID gets modified by CreateModifier and needs to be adjusted here)
	if (id > m_highestLoadedId)
		m_highestLoadedId = id;
	m_nextID = m_highestLoadedId + 1;

	return newModifier;
}

bool CModifierManager::AddModifierToClassMap( CAG2ModifierBasePtr newModifier )
{
	if (m_modifierClasses.find(newModifier->GetClassName()) != m_modifierClasses.end())
	{
		CRY_ASSERT_MESSAGE("Modifier of class type %s already exists, skipping creation.", newModifier->GetClassName());
		return false;
	}

	m_modifierClasses[newModifier->GetClassName()] = newModifier;

	return true;
}

CModifierManager::~CModifierManager()
{
	// TODO: Delete everything inside m_modifierClasses
}

CModifierManager::CModifierManager()
{
	m_pSelectedModifier = NULL;
	m_nextID = 0;
	m_highestLoadedId = 0;
}

int CModifierManager::GetRegisteredModifierCount() const
{
	return m_modifierClasses.size();
}

const CString& CModifierManager::GetModifierClassName( int listIndex ) const
{
	CRY_ASSERT_MESSAGE(listIndex < m_modifierClasses.size(), "Tried to access modifier array with an out-of-bounds index");
	ModifierClassMap::const_iterator it = m_modifierClasses.begin();
	for (int i = 0; i < listIndex; ++i)
	{
		++it;
	}
	return it->first;
}

CString CModifierManager::GetModifierHumanReadableName( int listIndex ) const
{
	CRY_ASSERT_MESSAGE(listIndex < m_modifierClasses.size(), "Tried to access modifier array with an out-of-bounds index");
	ModifierClassMap::const_iterator it = m_modifierClasses.begin();
	for (int i = 0; i < listIndex; ++i)
	{
		++it;
	}
	return it->second->GetHumanReadableName();
}

CAG2ModifierBase* CModifierManager::CreateModifier( const CString& className )
{
	CRY_ASSERT_MESSAGE((m_modifierClasses.find(className) != m_modifierClasses.end()), "Unknown Modifier class!");

	CAG2ModifierBase* newModifier = m_modifierClasses[className]->Duplicate();
	newModifier->SetId(m_nextID);
	newModifier->Init();
	m_nextID++;

	return newModifier;
}

void CModifierManager::SetSelectedModifier( CAG2ModifierBasePtr pModifier )
{
	if (m_pSelectedModifier && pModifier && m_pSelectedModifier->GetId() == pModifier->GetId())
		return;

	// Turn off old one (if there is one)
	// Destroy (somehow)
	if (m_pSelectedModifier && m_pSelectedModifier->GetDialogIDD() != IDD_AG2_MODIFIER_DEFAULT)
	{
		m_pSelectedModifier->ShowWindow(SW_HIDE);
		m_pSelectedModifier->DestroyWindow();
	}

	// If there is no selected modifier any more
	// hide the panel and return
	m_pSelectedModifier = pModifier;
	if (!pModifier)
	{
		ShowWindow(SW_HIDE);
		return;
	}

	// Create new one, if there is one
	if (pModifier->GetDialogIDD() == IDD_AG2_MODIFIER_DEFAULT)
	{
		// Show default "This Modifier has no parameters" pane
		ShowWindow(SW_SHOW);
	}
	else
	{
		// Show the Modifiers Details Pane
		// Create, Init
		pModifier->Create(pModifier->GetDialogIDD(), m_pParent->GetNodeDetailsParent());
		pModifier->InitParameterPanel();
		pModifier->ShowWindow(SW_SHOW);
	}
}

const CString& CModifierManager::GetClassNameFromOldVersionStateNode( const XmlNodeRef stateNode ) const
{
	// Go through all registered modifier class and ask whether they can understand this node
	ModifierClassMap::const_iterator it = m_modifierClasses.begin();
	ModifierClassMap::const_iterator endIt = m_modifierClasses.end();
	for ( ; it != endIt; ++it)
	{
		CAG2ModifierBasePtr pModifier = it->second;
		if (pModifier->CanConvertFromOldGraphVersion(stateNode))
			return it->first;
	}

	static CString empty("");
	return empty;
}

//////////////////////////////////////////////////////////////////////////
//  HyperGraph Node for the Modifiers (you don't need to touch this)    //
//////////////////////////////////////////////////////////////////////////

static CHyperNodePainter_Modifier animGraphModifierPainter;

CAG2ModifierNode::CAG2ModifierNode(CHyperGraph * pGraph, HyperNodeID id, CAG2ModifierBasePtr pModifier ) : CAGNodeBase2(NULL)
{
	m_id = id;
	m_pGraph = pGraph;
	m_pModifier = pModifier;

	IVariablePtr pVar;
/*
	pVar = new CVariableVoid();
	pVar->SetHumanName("in");
	pVar->SetName("in");
	CHyperNodePort portIn( pVar, true );
	portIn.bAllowMulti = true;
	AddPort(portIn);
*/

	pVar = new CVariableVoid();
	pVar->SetHumanName("out");
	pVar->SetName("out");
	CHyperNodePort portOut( pVar, false );
	portOut.bAllowMulti = true;
	AddPort(portOut);

	//SetName( pState->GetName() );
	SetClass( "$modifier ModifierName" );

	SetPainter(&animGraphModifierPainter);
}

AGNodeType2 CAG2ModifierNode::GetAGNodeType()
{
	return AG_NODE_MODIFIER;
}

CHyperNode * CAG2ModifierNode::Clone()
{
	return new CAGNode2(m_pGraph, m_id, NULL);
}

CAGState2Ptr CAG2ModifierNode::GetState()
{
	return NULL;
}



//////////////////////////////////////////////////////////////////////////
//         Modifier Node Painter (you don't need to touch this)         //
//////////////////////////////////////////////////////////////////////////


namespace
{

	struct SAssets
	{
		SAssets() :
		font(L"Tahoma", 10.0f),
		// font_associated(L"Tahoma", 14.0f), 12.0f for more detailed
		brushBackground( Gdiplus::Color(239,237,89) ),
		brushBackgroundSelected( Gdiplus::Color(198,228,249) ),
		brushBackgroundNotInGame( Gdiplus::Color(255,128,128) ),
		brushBackgroundAllowSelectionInGame( Gdiplus::Color(128,255,128) ),
		brushText( Gdiplus::Color(0,0,0) ),
		penBorder( Gdiplus::Color(5,5,80), 1.0f ),
		modifierBGNormal( Gdiplus::Color(150,150,255) ),
		modifierBGSelected( Gdiplus::Color(100,100,237) )
	{
		sf.SetAlignment( Gdiplus::StringAlignmentCenter );
	}
	~SAssets()
	{
	}

	// Animation Nodes
	Gdiplus::Font font;
	Gdiplus::SolidBrush brushBackground;
	Gdiplus::SolidBrush brushBackgroundSelected;
	Gdiplus::SolidBrush brushBackgroundNotInGame;
	Gdiplus::SolidBrush brushBackgroundAllowSelectionInGame;
	Gdiplus::SolidBrush brushText;
	Gdiplus::StringFormat sf;
	Gdiplus::Pen penBorder;

	// Modifier Nodes
	Gdiplus::SolidBrush modifierBGNormal;
	Gdiplus::SolidBrush modifierBGSelected;
	};

	static SAssets * pAssets = 0;
}

void CHyperNodePainter_Modifier::Paint( CHyperNode * pNode, CDisplayList * pList )
{
	if (!pAssets)
		pAssets = new SAssets();

	CAGNodeBase2* baseNode = (CAGNodeBase2*) pNode;
	if (baseNode->GetAGNodeType() == AG_NODE_MODIFIER)
	{
		PaintModifier(pNode, pList);
	}
}

void CHyperNodePainter_Modifier::PaintModifier( CHyperNode * pNode, CDisplayList * pList )
{
	CAG2ModifierNode * pAGNode = (CAG2ModifierNode*) pNode;
	bool isSelected = pNode->IsSelected();

	CDisplayRectangle * tt;
	if (isSelected)
	{
		tt = pList->Add<CDisplayRectangle>()
		->SetHitEvent(eSOID_ShiftFirstOutputPort)
	  ->SetFilled(&pAssets->brushBackgroundSelected);
	}

	CDisplayRectangle * pBackground = pList->Add<CDisplayRectangle>()
		->SetHitEvent(eSOID_ShiftFirstOutputPort)
		->SetStroked(&pAssets->penBorder);

	if (isSelected)
		pBackground->SetFilled(&pAssets->modifierBGSelected);
	/*	else if (!pState->IncludeInGame())
	pBackground->SetFilled( &pAssets->brushBackgroundNotInGame );
	else if (pState->AllowSelect())
	pBackground->SetFilled( &pAssets->brushBackgroundAllowSelectionInGame );*/
	else
		pBackground->SetFilled(&pAssets->modifierBGNormal);

	CDisplayString * pString = pList->Add<CDisplayString>()
		->SetHitEvent(eSOID_ShiftFirstOutputPort)
		->SetText( pAGNode->GetModifier()->GetNodeDisplayString() )
		->SetBrush( &pAssets->brushText )
		->SetFont( &pAssets->font )
		->SetStringFormat( &pAssets->sf );

	Gdiplus::RectF rect = pString->GetBounds(pList->GetGraphics());
	rect.Width += 32.0f;
	rect.Height += 6.0f;
	rect.X = -0.0f;
	rect.Y = -0.0f;
	pBackground->SetRect( rect );
	pString->SetLocation( Gdiplus::PointF( rect.Width * 0.5f, 3.0f ) );

	pList->SetAttachRect( 0, rect );
	pList->SetAttachRect( 1000, rect );

	if (isSelected)
	{
		rect.Width += 4.0f;
		rect.Height += 4.0f;
		rect.X -= 2.0f;
		rect.Y -= 2.0f;
		tt->SetRect( rect );
	}
}

#undef TYPE_STRING
#undef ID_STRING
