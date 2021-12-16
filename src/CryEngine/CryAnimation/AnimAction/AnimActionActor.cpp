#include "stdafx.h"
#include <CryExtension/CryCreateClassInstance.h>

#include "AnimActionActor.h"
#include "AnimActionManager.h"
#include "AALocomotion.h"
#include "AASequence.h"
#include "AARandom.h"

// global token
uint32 CAnimActionActor::g_nAnimActionToken = 0;

// TODO: Integrate this better into CryAction.
extern CAnimActionManager g_AnimActionManager; 

// TODO: From CryAnimation, make it proper.
#define AnimFileWarning(...)

CAnimActionActor::CAnimActionActor()
{
	m_vCamPos = Vec3(ZERO);
	m_nLastAnimationToken = 0;
	m_nClassIndex = -1;

	m_AnimLocation.SetIdentity();
}

CAnimActionActor::~CAnimActionActor()
{
}

void CAnimActionActor::Init(ICharacterInstance* pCharInstance)
{
	m_pICharInstance = pCharInstance;
	m_pISkeletonAnim = pCharInstance->GetISkeletonAnim();
	m_pIAnimationSet = pCharInstance->GetIAnimationSet();

	CAnimActionManager* pAnimActionManager = &g_AnimActionManager;
	uint32 numAnimActionClasses = pAnimActionManager->GetAAClassCount();
	m_Actions.resize(numAnimActionClasses);

	string name;
	name.reserve(32);
	for (uint32 i=0; i<numAnimActionClasses; i++)
	{
		IAnimActionClass* AAClass = pAnimActionManager->GetAAClass(i);
		const char* strClassName = AAClass->GetAAClassName();
		int32 index = pAnimActionManager->GetAAClassIndexByName(strClassName);

		name = "AnimAction_";
		name += strClassName;
		if (::CryCreateClassInstance(name.c_str(), m_Actions[index]))
			m_Actions[index]->Init(this);
	}
}

bool CAnimActionActor::StartAnimAction( const char *sAnimAction,SAnimActionStartParams &params )
{
	// Find animation action in a character model
	m_nClassIndex = -1;
	IAnimationSet* pIAnimationSet = m_pICharInstance->GetIAnimationSet();
	CAnimActionManager* pAnimActionManager = &g_AnimActionManager;

	int32 nGlobalAnimId = pIAnimationSet->GetGlobalIDByName( sAnimAction );
	if (nGlobalAnimId < 0)
	{
		AnimFileWarning( m_pICharInstance->GetFilePath(), "AnimAction '%s' not valid", sAnimAction );
		return false;
	}

	// check asset type
	int32 numGlobalAAC = int32(pAnimActionManager->m_arrGlobalAAC.size());
	if ( nGlobalAnimId >= numGlobalAAC)
		return false; //most likely not an AAC

	GlobalHeaderAAC& rGlobalAAHeader = pAnimActionManager->m_arrGlobalAAC[ nGlobalAnimId ];
	if (rGlobalAAHeader.IsAssetAAC() == false)
		return false;
	if (rGlobalAAHeader.IsAssetAACValid() == false)
	{
		AnimFileWarning( m_pICharInstance->GetFilePath(), "AnimAction '%s' not valid", sAnimAction );
		return false;
	}

	IAnimActionClass* pPrototype = rGlobalAAHeader.m_pAnimActionPrototype;
	const char* strClassName = pPrototype->GetAAClassName();
	int32 index = pAnimActionManager->GetAAClassIndexByName(strClassName);

	m_Actions[index]->AnimActionInit();
	m_Actions[index]->m_pAnimActionActor	= this;
	m_Actions[index]->m_pPrototype				= pPrototype;
	m_Actions[index]->params = params;
	m_Actions[index]->status = IAnimAction::eActionStatusBlendIn;
	m_nClassIndex = index;
	return true;
}

bool CAnimActionActor::StopAnimAction(uint32 nLayer)
{
	m_nClassIndex = -1;
	return true;
}

void CAnimActionActor::Update()
{
	if (m_nClassIndex<0)
		return;

	// TODO: Implement back use of Console::GetInst().ca_AnimActionDebug cvar.
/*
	//////////////////////////////////////////////////////////////////////////
	// Debug Draw.
	//////////////////////////////////////////////////////////////////////////
	if (Console::GetInst().ca_AnimActionDebug)
	{
		IAnimAction::SDebugDrawInfo info;
		info.x = 10.0f;
		info.y = 10.0f;
		info.ystep = 16.0f;

	//	IAnimAction action = m_Actions[m_nClassIndex];

		const char* strActionName = m_Actions[m_nClassIndex]->GetPrototype()->GetActionName();
		const char* strClassName  = m_Actions[m_nClassIndex]->GetPrototype()->GetAAClassName();
		DrawAnimActionDebugText( info.x,info.y,1.3f,ColorF(1.0f,1.0f,1.0f,1.0f),"AnimAction: %s (%s)",strActionName, strClassName );
		info.y += info.ystep;
		m_Actions[m_nClassIndex]->DebugDraw( info );
	}
*/

	//-------------------------------------------------------------------------
	ICharacterInstance* pCharInstance = GetCharacterInstance();
// TODO: Implement back.
//	m_vCamPos =pCharInstance->m_pCamera->GetPosition();

	IAnimAction::SPlaybackContext context;
	ZeroStruct( context );
	context.params = m_Actions[m_nClassIndex]->params;

	int32 status = m_Actions[m_nClassIndex]->Update( context );
}



int32 CAnimActionActor::GetActiveAnimAction() const
{
	return m_nClassIndex;
}
IAnimAction* CAnimActionActor::GetActiveIAnimAction() const
{
	if (m_nClassIndex<0)
		return 0;
	return m_Actions[m_nClassIndex].get();
}

CAnimation* CAnimActionActor::FindInFIFOByAAToken( int32 nLayerID, uint32 nAnimActionToken, int8* index )
{
	uint8 count = m_pISkeletonAnim->GetNumAnimsInFIFO( nLayerID );
	while ( count-- )
	{
		CAnimation& animation = m_pISkeletonAnim->GetAnimFromFIFO( nLayerID, count );
		if ( animation.m_AnimParams.m_nAnimActionToken == nAnimActionToken )
		{
			if ( index != NULL )
				*index = count;
			return &animation;
		}
	}
	if ( index != NULL )
		*index = -1;
	return NULL;
}


//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

f32 CAnimActionActor::GetTransWeightFromFIFO( const char* strAName, uint32 idx )
{
	f32 tweight=0.0f;
	uint8 count = m_pISkeletonAnim->GetNumAnimsInFIFO( 0 );
	for (uint32 i=0; i<count; i++)	
	{
		CAnimation& animation = m_pISkeletonAnim->GetAnimFromFIFO(0,i);

		const char* pName=0;
		if (animation.m_Parametric.m_nParametricID>=0)
			pName	=	m_pIAnimationSet->GetNameByAnimID(animation.m_Parametric.m_nParametricID);
		else 
			pName	=	m_pIAnimationSet->GetNameByAnimID(animation.m_Parametric.m_nAnimID[0]);

		if ( stricmp(pName,strAName)==0 )
		{
			if (animation.m_AnimParams.m_nIndexAAC==idx)
				tweight+=animation.m_fTransitionWeight;
		}
	}
	return tweight;
}

//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

f32 CAnimActionActor::GetAnimTimeFromFIFO( const char* strAName, uint32 idx )
{
	uint8 count = m_pISkeletonAnim->GetNumAnimsInFIFO( 0 );
	for (uint32 i=0; i<count; i++)	
	{
		CAnimation& animation = m_pISkeletonAnim->GetAnimFromFIFO(0,i);

		const char* pName=0;
		if (animation.m_Parametric.m_nParametricID>=0)
			pName	=	m_pIAnimationSet->GetNameByAnimID(animation.m_Parametric.m_nParametricID);
		else 
			pName	=	m_pIAnimationSet->GetNameByAnimID(animation.m_Parametric.m_nAnimID[0]);

		if ( stricmp(pName,strAName)==0 )
		{
			if (animation.m_AnimParams.m_nIndexAAC==idx)
				return animation.m_fAnimTime*100.0f;
		}

	}

	return 0.0f;
}

size_t CAnimActionActor::SizeOfThis (ICrySizer * pSizer)
{
	size_t TotalSize = 0;
	TotalSize += sizeof(IAnimActionPtr)  * m_Actions.capacity();
	return TotalSize;
}

void CAnimActionActor::GetMemoryUsage(ICrySizer * pSizer) const
{	
	pSizer->AddObject(m_Actions);
}
