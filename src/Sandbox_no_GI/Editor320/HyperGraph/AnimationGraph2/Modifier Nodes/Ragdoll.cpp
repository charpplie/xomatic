#include "StdAfx.h"
#include "Ragdoll.h"


CAG2Modifier_Ragdoll::CAG2Modifier_Ragdoll() : CAG2ModifierBase()
{

}

CAG2Modifier_Ragdoll::~CAG2Modifier_Ragdoll()
{

}

const void CAG2Modifier_Ragdoll::Save( XmlNodeRef modifierNode ) const
{
	// This node actually has no custom data
}

const void CAG2Modifier_Ragdoll::Load( XmlNodeRef modifierNode )
{
	// This node actually has no custom data
}

CAG2ModifierBase* CAG2Modifier_Ragdoll::Duplicate() const
{
	return new CAG2Modifier_Ragdoll();
}

const void CAG2Modifier_Ragdoll::Export( XmlNodeRef node ) const
{
	// <Event onEnter="Ragdoll"/>

	XmlNodeRef ragdollNode = node->createNode("Event");
	ragdollNode->setAttr("onEnter", "Ragdoll");
	node->addChild(ragdollNode);
}

bool CAG2Modifier_Ragdoll::CanConvertFromOldGraphVersion( const XmlNodeRef node ) const
{
	if (stricmp(node->getTag(), "Event"))
		return false;

	CString testVal;
	node->getAttr("onEnter", testVal);
	if (stricmp(node->getTag(), "Ragdoll") == 0)
		return true;

	return false;
}