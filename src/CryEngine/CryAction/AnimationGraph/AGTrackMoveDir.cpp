#include "StdAfx.h"
#include "AGTrackMoveDir.h"
#include "AnimationGraphState.h"
#include "AnimatedCharacter.h"



CAGTrackMoveDir::CAGTrackMoveDir( CAGTrackMoveDirFactory * pFactory ) 
: IAnimationStateNode(eASNF_Update)
, m_pFactory(pFactory)
, m_interpolatedMoveDir(Vec3(ZERO))
{ 
}

void CAGTrackMoveDir::EnterState( SAnimationStateData& data, bool dueToRollback )
{  
	//   if (m_pFactory->DebugVal() != 0)
	//     CryLog("AGSound:EnterState <%s>", data.pEntity->GetName());

	//   if (m_pFactory->m_input.length() > 0)
	//     m_inputId = data.pState->GetInputId(m_pFactory->m_input.c_str());  

	m_lastFrameId = -1;

	// Get latest movement direction and initialize all old and new value with it
	m_interpolatedMoveDir = GetMoveDir(data);
}

bool CAGTrackMoveDir::CanLeaveState( SAnimationStateData& data )
{
	return true;
}

void CAGTrackMoveDir::LeaveState( SAnimationStateData& data )
{ 
}

void CAGTrackMoveDir::GetCompletionTimes( SAnimationStateData& data, CTimeValue start, CTimeValue& hard, CTimeValue& sticky )
{  
//	hard = sticky = 0.0f;
}

Vec3 CAGTrackMoveDir::GetMoveDir( SAnimationStateData& data )
{
	return data.pAnimatedCharacter->GetRequestedEntityMovement().t;
}

void CAGTrackMoveDir::Update( SAnimationStateData& data )
{	
	//         float drawColor[4] = {1,1,1,1};    
	//         gEnv->pRenderer->Draw2dLabel(10.f, 300.f, 1.2f, drawColor, false, "AGSound: delaying for %f", m_delay);          

	if (!data.pEntity || !data.pAnimatedCharacter)
	{
		// TODO: 
		// if the data isn't available this frame, a re-init is needed as soon as the data is available again
		// that sets the interpolated value to the non-interpolated one
		return;
	}

	// Make sure this instance is only updated once per frame
	int curFrameId = gEnv->pRenderer->GetFrameID();
	if (curFrameId == m_lastFrameId)
		return;
	m_lastFrameId = curFrameId;


	// Get latest movement dir
	float frameTime = gEnv->pTimer->GetFrameTime();
	Vec3 newMoveDir = GetMoveDir(data);
	float moveDirLength = newMoveDir.NormalizeSafe();

	// TODO: Check for a CVar here first
	DebugRender(data, newMoveDir);

	// Check whether the angle difference between old and new is big enough

	// Check whether old and new are in different sections

	// Possibly send a signal if one is associated
	// and then set interpolated direction the new one so it isn't triggering a signal
	// again immediately

	// make sure tVal is never smaller than 0.0, so that the sum of the multipliers is always 1.0f
	float tVal = (m_pFactory->m_trackingTime - frameTime) / m_pFactory->m_trackingTime;
	tVal = max(0.0f, tVal);	

	// Update interpolated movement dir
	m_interpolatedMoveDir = m_interpolatedMoveDir * tVal + newMoveDir * (1.0f - tVal);
	m_interpolatedMoveDir.Normalize();



	/*
	float	animBlend = pCharacter->GetISkeletonAnim()->GetUserData(m_animBlendID);

	IActorSystem*	pASystem = gEnv->pGame->GetIGameFramework()->GetIActorSystem();
	if(pASystem )
	{
	IActor*  pActor = pASystem->GetActor( data.pEntity->GetId() );
	*/
}

void CAGTrackMoveDir::DebugRender( SAnimationStateData &data, Vec3 newMoveDir )
{
	// get entity position
	Vec3 entityPos = data.pEntity->GetWorldPos() + Vec3(0.0f, 0.0f, 0.10f);

	// draw circle underneath for the sections
	gEnv->pRenderer->GetIRenderAuxGeom()->DrawCylinder(entityPos - Vec3(0,0,0.05f), Vec3(0,0,1.0f), 1.0f, 0.01f, ColorB(50, 125, 5, 5));

	// TODO: draw the various sections

	// draw arrows
	gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(entityPos, ColorB(40,255,255,255), 
		entityPos + newMoveDir, ColorB(40,255,255,255), 4.0f);
	gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(entityPos, ColorB(255,40,255,255), 
		entityPos + m_interpolatedMoveDir, ColorB(255,40,255,255), 4.0f);
}

const IAnimationStateNodeFactory::Params * CAGTrackMoveDirFactory::GetParameters()
{
	static const Params params[] = 
	{     
		{false,   "float",  "trackingTime", "TrackingTime", "0.4" },
		{0}
	};
	return params;
}

bool CAGTrackMoveDirFactory::Init( const XmlNodeRef& node, IAnimationGraphPtr )
{
	bool ok = true;
   
	m_trackingTime = 0.4f;


	node->getAttr("trackingTime", m_trackingTime);
	
	return ok;
}


void CAGTrackMoveDirFactory::Release()
{
	delete this;
}

IAnimationStateNode * CAGTrackMoveDirFactory::Create()
{
	return new CAGTrackMoveDir(this);
}

const char * CAGTrackMoveDirFactory::GetCategory()
{
	return "TrackMoveDir";
}

const char * CAGTrackMoveDirFactory::GetName()
{
	return "TrackMoveDir";
}

void CAGTrackMoveDirFactory::SerializeAsFile(bool reading, AG_FILE *file)
{
	FileSerializationHelper h(reading, file);

	h.Value(&m_trackingTime);
}




