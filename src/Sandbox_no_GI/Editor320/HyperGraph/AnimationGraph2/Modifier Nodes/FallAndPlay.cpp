#include "StdAfx.h"
#include "FallAndPlay.h"


CAG2Modifier_FallAndPlay::CAG2Modifier_FallAndPlay() : CAG2ModifierBase()
{

}

CAG2Modifier_FallAndPlay::~CAG2Modifier_FallAndPlay()
{

}

const void CAG2Modifier_FallAndPlay::Save( XmlNodeRef modifierNode ) const
{
	// This node actually has no custom data
}

const void CAG2Modifier_FallAndPlay::Load( XmlNodeRef modifierNode )
{
	// This node actually has no custom data
}

CAG2ModifierBase* CAG2Modifier_FallAndPlay::Duplicate() const
{
	return new CAG2Modifier_FallAndPlay();
}

const void CAG2Modifier_FallAndPlay::Export( XmlNodeRef node ) const
{
	// <Event onEnter="EnableFallAndPlay"/>
	// and
	// <FallAndPlay/>

	XmlNodeRef eventNode = node->createNode("Event");
	eventNode->setAttr("onEnter", "EnableFallAndPlay");
	eventNode->setAttr("matchAnimation", "0");
	node->addChild(eventNode);

	XmlNodeRef fallnplayNode = node->createNode("FallAndPlay");
	node->addChild(fallnplayNode);
}

bool CAG2Modifier_FallAndPlay::CanConvertFromOldGraphVersion( const XmlNodeRef node ) const
{
	// it is enough that the Event exists, the FallAndPlay node itself is mandatory for this modifier anyway
	if (stricmp(node->getTag(), "Event"))
		return false;

	CString testVal;
	node->getAttr("onEnter", testVal);
	if (stricmp(node->getTag(), "EnableFallAndPlay") == 0)
		return true;

	return false;
}
