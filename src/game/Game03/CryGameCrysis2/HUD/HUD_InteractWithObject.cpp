/************************************************************************/
/* Hud callback used to interact with object via "scanning"             */
/* created by Jan Mueller, 02/2010																			*/
/************************************************************************/

#include "StdAfx.h"
#include "HUD_InteractWithObject.h"
#include "HUD.h"

IEntityClass* CHUD_InteractWithObject::m_pInteractiveEntityClass = NULL;

//////////////////////////////////////////////////////////////////////////
CHUD_InteractWithObject::CHUD_InteractWithObject()
{
	m_pInteractiveEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("InteractiveEntity");
	assert(m_pInteractiveEntityClass);
}

//////////////////////////////////////////////////////////////////////////
CHUD_InteractWithObject::~CHUD_InteractWithObject()
{

}

//////////////////////////////////////////////////////////////////////////
void CHUD_InteractWithObject::OnHUDEvent( const SHUDEvent& event )
{
	if(event.eventType != eHUDEvent_OnEntityScanned)
		return;

	EntityId id = static_cast<EntityId>(event.GetData(0).m_int);
	IEntity *pEntity = gEnv->pEntitySystem->GetEntity(id);
	if(!pEntity)
		return;

	bool bUsable = IsUsableEntity(pEntity);
	if(!bUsable)
		return;

	UseInteractiveEntity(pEntity);
}

//////////////////////////////////////////////////////////////////////////
bool CHUD_InteractWithObject::IsUsableEntity( const IEntity *pEntity )
{
	if(!pEntity)
		return false;

	if(!m_pInteractiveEntityClass)
		m_pInteractiveEntityClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass("InteractiveEntity");

	//check class
	IEntityClass *pClass = pEntity->GetClass();
	if(pClass != m_pInteractiveEntityClass)
		return false;

	//read properties
	IScriptTable *pScriptTable = pEntity->GetScriptTable();
	SmartScriptTable properties;
	if (!(pScriptTable && pScriptTable->GetValue("Properties", properties)))
		return false;

	//can be used by looking at it ?
	bool bUsableByScanning = false;
	properties->GetValue("bUsableByScanning", bUsableByScanning);
	if(!bUsableByScanning)
		return false;

	//can be used by scanning
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CHUD_InteractWithObject::UseInteractiveEntity( IEntity *pEntity ) const
{
	assert(pEntity->GetClass() == m_pInteractiveEntityClass);

	IScriptTable *pScriptTable = pEntity->GetScriptTable();
	assert(pScriptTable);
	
	Script::CallMethod(pScriptTable, "OnUsed", pScriptTable, 0);
}