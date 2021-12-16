//////////////////////////////////////////////////////////////////////////
//  Crytek Source File.
//  Copyright (C), Crytek Studios, 2009
//////////////////////////////////////////////////////////////////////////
//  ModelNodes.cpp
//  Description: model related flownodes
//	* change player model
//
//////////////////////////////////////////////////////////////////////////
//  History:
//  * 01/10/2009   15:37 : Created by Christian Helmich
//
//////////////////////////////////////////////////////////////////////////


#include "StdAfx.h"
#include <ISystem.h>
#include <ICryAnimation.h>
#include <IActorSystem.h>
#include <IAnimationGraph.h>
#include <IAnimatedCharacter.h>
#include <GameObjects/GameObject.h>
#include "IAnimatedCharacter.h"

#include "FlowBaseNode.h"
#include "CryAction.h"
#include "IMovementController.h"

//////////////////////////////////////////////////////////////////////////
//ModelChange node

class CFlowNode_ModelChange : public CFlowBaseNode
{
public:
	enum EInputs {
		IN_MC_MODEL_FILE = 0,
		IN_MC_ENTITY_SLOT,
		IN_MC_FN_ACTIVATE
	};

	CFlowNode_ModelChange( SActivationInfo * pActInfo ); 
	virtual ~CFlowNode_ModelChange() {}

	virtual void GetMemoryStatistics(ICrySizer * s);
	virtual void GetConfiguration(SFlowNodeConfig& config);
	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo );

protected:
	bool	ChangeModel(SActivationInfo *pActInfo);
};

//----------------------
CFlowNode_ModelChange::CFlowNode_ModelChange( SActivationInfo * pActInfo ):
	CFlowBaseNode()
{
}

//----------------------
void	CFlowNode_ModelChange::GetMemoryStatistics(ICrySizer * s)
{
	s->Add(*this);
}

//----------------------
void	CFlowNode_ModelChange::GetConfiguration(SFlowNodeConfig& config)
{
	static const SInputPortConfig inputs[] = {
		InputPortConfig<string>("Model name", "", _HELP("Model filename"), _HELP("Path to model file")),
		InputPortConfig<int>("Entity slot", 0, _HELP("Entity slot to load model into"), _HELP("Entity slot")),
		InputPortConfig<bool>("Activate", false, _HELP("Switches to new model"), _HELP("Activate")),
		{0}
	};
	config.pInputPorts = inputs;
	config.nFlags |= EFLN_TARGET_ENTITY;
	config.sDescription = _HELP("Change the model of the attached entity");
	config.SetCategory(EFLN_ADVANCED);
}

//----------------------
void	CFlowNode_ModelChange::ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
{
	switch (event)
	{
	case eFE_Activate:
		if (!IsPortActive(pActInfo, IN_MC_FN_ACTIVATE))
			break;		

		bool bRet = ChangeModel(pActInfo);
		assert(bRet);
		break;		
	}
}

//----------------------
bool	CFlowNode_ModelChange::ChangeModel(SActivationInfo *pActInfo)
{
	if (!pActInfo->pEntity)
	{	
		CryLogAlways("Could not retrieve current entity for model change");
		return false;	
	}

	const string& modelFilename = GetPortString(pActInfo, IN_MC_MODEL_FILE);
	const int			entitySlot		= GetPortInt(pActInfo, IN_MC_ENTITY_SLOT);

	int iReturnedSlot = pActInfo->pEntity->LoadCharacter(entitySlot, modelFilename.c_str());
	if (iReturnedSlot == -1)
	{		
		CryLogAlways("Model change failed");		
		return false;
	}
#ifdef _DEBUG
	CryLogAlways("Model change succeeded: model %s attached to slot %i (%i)",
		modelFilename.c_str(),
		iReturnedSlot,
		entitySlot);
#endif //_DEBUG

	return true;
}

//////////////////////////////////////////////////////////////////////////
// flownode registering

REGISTER_FLOW_NODE_SINGLETON( "Entity:ChangeModel", CFlowNode_ModelChange );

//////////////////////////////////////////////////////////////////////////
//<eof>