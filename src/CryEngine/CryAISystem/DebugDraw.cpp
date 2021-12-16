/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2001-2009.
-------------------------------------------------------------------------
File name:   DebugDraw.cpp
$Id$
Description: move all the debug drawing functionality from AISystem. Need to clean it up eventually and 
make nice debug output functions, instead of one huge.

-------------------------------------------------------------------------
History:
- 29:11:2004   19:02 : Created by Kirill Bulatsev
- 2 Mar 2009         : Evgeny Adamenkov: Replaced IRenderer with CDebugDrawContext

*********************************************************************/

#include "StdAfx.h"

#ifdef CRYAISYSTEM_DEBUG

// AI includes
#include "CAISystem.h"
#include "DebugDrawContext.h"
#include "AILog.h"

#include "Graph.h"
#include "Puppet.h"
#include "AIVehicle.h"
#include "GoalPipe.h"
#include "GoalOp.h"
#include "AIPlayer.h"
#include "PipeUser.h"
#include "Leader.h"
#include "VolumeNavRegion.h"
#include "TriangularNavRegion.h"
#include "SmartObjects.h"
#include "PathFollower.h"
#include "Shape.h"
#include "LayeredNavMesh/PolygonSetOps.h"
#include "CodeCoverageGUI.h"
#include "StatsManager.h"
#include "PerceptionManager.h"
#include "FireCommand.h"

// (MATT) TODO Get a lightweight DebugDraw interface rather than pulling in this header file {2008/12/04}
#include "TacticalPointSystem/TacticalPointSystem.h"
#include "TargetSelection/TargetTrackManager.h"
#include "CoordinationSystem/CoordinationManager.h"   
#include "Communication/CommunicationManager.h"
#include "Cover/CoverSystem.h"
#include "BehaviorTree/ProfileDictionary.h"
#include "BehaviorTree/PersonalBehaviorTree.h"
#include "SelectionTree/SelectionTree.h"
#include "GroupSystem/GroupSystem.h"
#include "Readability/CoopReadabilitiesSystem.h"
#include "CentralInterestManager.h"
#include "PersonalInterestManager.h"

#pragma warning(disable: 4244)

void CAISystem::DebugDrawRecorderRange() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	const float fRecorderDrawStart = m_recorderDebugContext.fStartPos;
	const float fRecorderDrawEnd = m_recorderDebugContext.fEndPos;
	const float fRecorderDrawCursor = m_recorderDebugContext.fCursorPos;

	const bool bTimelineValid = (fabsf(fRecorderDrawEnd - fRecorderDrawStart) > FLT_EPSILON);
	const bool bCursorValid = bTimelineValid ? (fRecorderDrawCursor >= fRecorderDrawStart && fRecorderDrawCursor <= fRecorderDrawEnd) : (fRecorderDrawCursor > FLT_EPSILON);

	const ColorB colGray(128,128,128,255);

	CDebugDrawContext dc;

	// Debug all of the objects
	TDebugObjectsMap::const_iterator itObject = m_recorderDebugContext.DebugObjects.begin();
	TDebugObjectsMap::const_iterator itObjectEnd = m_recorderDebugContext.DebugObjects.end();
	for (; itObject != itObjectEnd; ++itObject)
	{
		const SAIRecorderObjectDebugContext &objectContext = itObject->second;
		if (false == objectContext.bEnableDrawing)
			continue;

		CAIObject *pAIObject = GetAIObjectByName(objectContext.sName.c_str());
		if (!pAIObject)
			continue;

		IAIDebugRecord *pRecord = pAIObject->GetAIDebugRecord();
		if (!pRecord)
			continue;

		IAIDebugStream *pPosStream = pRecord->GetStream(IAIRecordable::E_AGENTPOS);
		IAIDebugStream *pDirStream = pRecord->GetStream(IAIRecordable::E_AGENTDIR);
		if (!pPosStream || !pDirStream)
			continue;

		const ColorB colDebug = objectContext.color;
		float fCurrPosTime = 0.0f;
		float fCurrDirTime = 0.0f;
		Vec3 *pPos = NULL;
		Vec3 *pDir = NULL;

		// Draw range
		if (bTimelineValid && fRecorderDrawStart < pPosStream->GetEndTime() && fRecorderDrawEnd > pPosStream->GetStartTime())
		{
			pPosStream->Seek(fRecorderDrawStart);
			pDirStream->Seek(fRecorderDrawStart);
			pPos = (Vec3*)(pPosStream->GetCurrent(fCurrPosTime));
			pDir = (Vec3*)(pDirStream->GetCurrent(fCurrDirTime));

			// Draw start cursor pos
			if (pPos)
			{
				const Vec3& vPos(*pPos);
				dc->DrawSphere(vPos, 0.25f, colGray);

				if (pDir)
				{
					const Vec3& vDir(*pDir);
					dc->DrawArrow(vPos, vDir, 0.25f, colGray);
				}

				dc->DrawCone(vPos + Vec3(0,0,5), Vec3(0,0,-1), 0.5f, 4.0f, colGray);
				dc->Draw3dLabel(vPos + Vec3(0,0,0.8f), 1.0f, "%s START\n%.1fs", pAIObject->GetName(), fCurrPosTime);
			}

			int	j = 0;
			while (fCurrPosTime <= fRecorderDrawEnd && pPosStream->GetCurrentIdx() < pPosStream->GetSize())
			{
				float fNextPosTime = 0.0f;
				float fNextDirTime = 0.0f;
				Vec3 *pNextPos = (Vec3*)(pPosStream->GetNext(fNextPosTime));
				pDirStream->Seek(fNextPosTime);
				Vec3 *pNextDir = (Vec3*)(pDirStream->GetNext(fNextDirTime));
				if (pPos && pNextPos)
				{
					const Vec3 &vPos(*pPos);
					const Vec3 &vNext(*pNextPos);
					const Vec3& vNextDir = (pNextDir ? *pNextDir : Vec3Constants<float>::fVec3_Zero);

					if((j & 1) == 0)
					{
						dc->DrawLine(vPos, colDebug, vNext, colDebug);
						if (!vNextDir.IsZero())
							dc->DrawArrow(vNext, vNextDir, 0.25f, colDebug);
					}
					else
					{
						dc->DrawLine(vPos, colGray, vNext, colGray);
						if (!vNextDir.IsZero())
							dc->DrawArrow(vNext, vNextDir, 0.25f, colGray);
					}
				}

				fCurrPosTime = fNextPosTime;
				pPos = pNextPos;
				++j;
			}

			// Draw start end pos
			pPosStream->Seek(fRecorderDrawEnd);
			pDirStream->Seek(fRecorderDrawEnd);
			pPos = (Vec3*)(pPosStream->GetCurrent(fCurrPosTime));
			pDir = (Vec3*)(pDirStream->GetCurrent(fCurrDirTime));
			if (pPos)
			{
				const Vec3& vPos(*pPos);
				dc->DrawSphere(vPos, 0.25f, colGray);

				if (pDir)
				{
					const Vec3& vDir(*pDir);
					dc->DrawArrow(vPos, vDir, 0.25f, colGray);
				}

				dc->DrawCone(vPos + Vec3(0,0,5), Vec3(0,0,-1), 0.5f, 4.0f, colGray);
				dc->Draw3dLabel(vPos + Vec3(0,0,0.8f), 1.0f, "%s END\n%.1fs", pAIObject->GetName(), fCurrPosTime);
			}
		}

		// Draw cursor current pos
		if (bCursorValid)
		{
			Vec3 vCurrPos;

			pPosStream->Seek(fRecorderDrawCursor);
			pDirStream->Seek(fRecorderDrawCursor);
			pPos = (Vec3*)(pPosStream->GetCurrent(fCurrPosTime));
			pDir = (Vec3*)(pDirStream->GetCurrent(fCurrDirTime));
			if (pPos)
			{
				const Vec3& vDir = (pDir ? *pDir : Vec3Constants<float>::fVec3_OneZ);

				// Create label text that depicts everything that happened at this moment from all streams
				string sCursorText;
				sCursorText.Format("%s CURRENT\n%.1fs", pAIObject->GetName(), fCurrPosTime);

				for (int i = IAIRecordable::E_NONE; i < IAIRecordable::E_COUNT; ++i)
				{
					IAIDebugStream *pEventStream = pRecord->GetStream((IAIRecordable::e_AIDbgEvent)i);
					if (!pEventStream)
						continue;

					string sShortLabel, sText;
					pEventStream->Seek(fRecorderDrawCursor);
					if (pEventStream->GetCurrentString(sShortLabel, fCurrPosTime) &&
						fabsf(fCurrPosTime - fRecorderDrawCursor) <= 0.1f)
					{
						sText.Format("\n%s: %s", pEventStream->GetName(), sShortLabel.c_str());
						sCursorText += sText;
					}
				}

				vCurrPos = *pPos;
				dc->DrawSphere(vCurrPos, 0.25f, colDebug);
				dc->DrawCone(vCurrPos + Vec3(0,0,5), Vec3(0,0,-1), 0.5f, 4.0f, colDebug);
				dc->Draw3dLabel(vCurrPos + Vec3(0,0,0.8f), 1.0f, sCursorText.c_str());
			}

			// Current attention target info
			IAIDebugStream *pAttTargetPosStream = pRecord->GetStream(IAIRecordable::E_ATTENTIONTARGETPOS);
			if (pAttTargetPosStream)
			{
				pAttTargetPosStream->Seek(fRecorderDrawCursor);
				Vec3 *pAttTargetPos = (Vec3*)(pAttTargetPosStream->GetCurrent(fCurrPosTime));
				if (pAttTargetPos)
				{
					string sName = "Att Target: ";
					
					IAIDebugStream *pAttTargetStream = pRecord->GetStream(IAIRecordable::E_ATTENTIONTARGET);
					if (pAttTargetStream)
					{
						pAttTargetStream->Seek(fRecorderDrawCursor);
						sName += (const char*)(pAttTargetStream->GetCurrent(fCurrPosTime));
					}
					else
					{
						sName += "Unknown";
					}

					Vec3 vPos(*pAttTargetPos);
					ColorB colAttTargetDebug = colDebug;
					colAttTargetDebug.a /= 2;
					dc->DrawSphere(vPos, 0.25f, colAttTargetDebug);
					dc->DrawCone(vPos + Vec3(0,0,5), Vec3(0,0,-1), 0.5f, 4.0f, colAttTargetDebug);
					dc->DrawLine(vCurrPos, colDebug, vPos, colAttTargetDebug);
					dc->Draw3dLabel(vPos + Vec3(0,0,0.8f), 1.0f, sName.c_str());
				}
			}
		}
	}
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawDamageControlGraph() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CDebugDrawContext dc;
	Vec3 camPos(dc->GetCameraPos());

	static std::vector<CPuppet*>	shooters;
	shooters.clear();

	AIObjectOwners::const_iterator aio = m_Objects.find(AIOBJECT_PUPPET);
	for(; aio != m_Objects.end(); ++aio)
	{
		if(aio->first != AIOBJECT_PUPPET)
			break;
		CPuppet* pPuppet = aio->second.GetAIObject()->CastToCPuppet();
		if(!pPuppet)
			continue;
		if(!pPuppet->IsEnabled())
			continue;
		if(!pPuppet->m_targetDamageHealthThrHistory)
			continue;
		if(Distance::Point_PointSq(camPos, pPuppet->GetPos()) > sqr(150.0f))
			continue;
		shooters.push_back(pPuppet);
	}

	if(shooters.empty())
		return;

	dc->Init2DMode();
	dc->SetAlphaBlended(true);
	dc->SetBackFaceCulling(false);

	const Vec3	u(1,0,0);
	const Vec3	v(0,-1,0);
	const Vec3	w(0,0,1);

	static std::vector<Vec3>	values;

	if(gAIEnv.CVars.DebugDrawDamageControl > 2)
	{
		// Combined graph
		static std::vector<CAIActor*>	targets;
		targets.clear();
		for (unsigned int i = 0; i < shooters.size(); ++i)
		{
			CPuppet* pPuppet = shooters[i];
			if(pPuppet->GetAttentionTarget())
			{
				CAIActor* pTargetActor = 0;
				CAIObject* pTarget = (CAIObject*)pPuppet->GetAttentionTarget();
				assert(pTarget);
				if(pTarget->IsAgent())
					pTargetActor = pTarget->CastToCAIActor();
				else
				{
					CAIActor *pActor = CastToCAIActorSafe ( pTarget->GetAssociation().GetAIObject() );
					if( pActor && pActor->IsAgent() )
						pTargetActor = pActor;
				}
				if(pTargetActor)
				{
					for (unsigned int j = 0; j < targets.size(); ++j)
					{
						if(pTargetActor == targets[j])
						{
							pTargetActor = 0;
							break;
						}
					}
					if(pTargetActor)
						targets.push_back(pTargetActor);
				}
			}
		}

		const Vec3 u2(1,  0, 0);
		const Vec3 v2(0, -1, 0);
		const Vec3 w2(0,  0, 1);

		const float	sizex = 0.7f;
		const float	sizey = 0.2f;

		Vec3	orig = Vec3(0.15f, 1-0.05f, 0);

		// dim BG
		ColorB bg1(  0,   0,   0, 128);
		ColorB bg2(128, 128, 128,  64);
		dc->DrawTriangle(orig, bg1, orig+u2*sizex, bg1, orig+u2*sizex+v2*sizey, bg2);
		dc->DrawTriangle(orig, bg1, orig+u2*sizex+v2*sizey, bg2, orig+v2*sizey, bg2);

		// Draw time lines
/*		float	timeLen = (PROTO_HEALTH_HISTORY_LEN - 1) * PROTO_HEALTH_HISTORY_INTERVAL;
		const float tickInterval = 1.0f;
		unsigned int tickCount = (unsigned int)floor(timeLen / tickInterval);
		for (unsigned int i = 0; i < tickCount; ++i)
		{
			float	t = ((i * tickInterval) / timeLen) * sizex;
			dc->DrawLine(orig + t*u, ColorB(255,255,255,64), orig + t*u + v*sizey, ColorB(255,255,255,64));
		}*/

		CValueHistory<float>* history = 0;

		float	timeLen = 1.0f;
		float maxVal = 1.0f;
		for (unsigned int i = 0; i < targets.size(); ++i)
		{
			if(!targets[i]->m_healthHistory) continue;

			float maxHealth = (float)targets[i]->GetProxy()->GetActorMaxHealth();
			float maxHealthArmor = (float)(targets[i]->GetProxy()->GetActorMaxHealth() + targets[i]->GetProxy()->GetActorMaxArmor());
			maxVal = max(maxVal, maxHealthArmor / maxHealth);

			timeLen = max(timeLen, (float)(targets[i]->m_healthHistory->GetMaxSampleCount() *
				targets[i]->m_healthHistory->GetSampleInterval()));
		}

		// Draw value lines
		dc->DrawLine(orig, ColorB(255,255,255,128), orig + u2*sizex, ColorB(255,255,255,128));
		dc->DrawLine(orig + v2*sizey*(0.5f/maxVal), ColorB(255,255,255,64), orig + u2*sizex + v2*sizey*(0.5f/maxVal), ColorB(255,255,255,64));
		dc->DrawLine(orig + v2*sizey*(1.0f/maxVal), ColorB(255,255,255,128), orig + u2*sizex + v2*sizey*(1.0f/maxVal), ColorB(255,255,255,128));

		// Draw time lines
		const float tickInterval = 1.0f; //seconds
		unsigned int tickCount = (unsigned int)floor(timeLen / tickInterval);
		for (unsigned int j = 0; j < tickCount; ++j)
		{
			float	t = ((j * tickInterval) / timeLen) * sizex;
			dc->DrawLine(orig + t*u2, ColorB(255,255,255,64), orig + t*u2 + v2*sizey, ColorB(255,255,255,64));
		}

		const float s = 1.0f / maxVal;

		// Draw targets
		for (unsigned int i = 0; i < targets.size(); ++i)
		{
			history = targets[i]->m_healthHistory;
			if(history)
			{
				unsigned int n = history->GetSampleCount();
				values.resize(n);
				float dt = history->GetSampleInterval();

				for (unsigned int j = 0; j < n; ++j)
				{
					float	val = min(history->GetSample(j) * s, 1.0f) * sizey;
					float	t = ((timeLen - j * dt) / timeLen) * sizex;
					values[j] = orig + t*u2 + v2*val;
				}
				if(n > 0)
					dc->DrawPolyline(&values[0], n, false, ColorB(255,255,255,160),1.5f);
			}
		}


		// Draw shooters
		for (unsigned int i = 0; i < shooters.size(); ++i)
		{
			history = shooters[i]->m_targetDamageHealthThrHistory;
			if(history)
			{
				unsigned int n = history->GetSampleCount();
				values.resize(n);
				float dt = history->GetSampleInterval();
				for (unsigned int j = 0; j < n; ++j)
				{
					float	val = min(history->GetSample(j) * s, 1.0f) * sizey;
					float	t = ((timeLen - j * dt) / timeLen) * sizex;
					values[j] = orig + t*u2 + v2*val;
				}
				if(n > 0)
					dc->DrawPolyline(&values[0], n, false, ColorB(240,32,16,240));
			}
		}


/*		Vec3	values[PROTO_HEALTH_HISTORY_LEN];

		// Target health
		{
			unsigned int n = target.GetHealthHistorySampleCount();
			for (unsigned int i = 0; i < n; ++i)
			{
				float	val = (target.GetHealthHistorySample(i) / target.GetMaxHealth()) * sizey;
				float	t = ((timeLen - i * PROTO_HEALTH_HISTORY_INTERVAL) / timeLen) * sizex;
				values[i] = orig + t*u + v*val;
			}
			if(n > 0)
				dc->DrawPolyline(values, n, false, ColorB(255,255,255,160),1.5f);
		}

		// Unit Thresholds
		for (unsigned int j = 0; j < UNIT_COUNT; ++j)
		{
			unsigned int n = unit[j].GetTargetHealthThrHistorySampleCount();
			for (unsigned int i = 0; i < n; ++i)
			{
				float	val = (unit[j].GetTargetHealthThrHistorySample(i) / target.GetMaxHealth()) * sizey;
				float	t = ((timeLen - i * PROTO_HEALTH_HISTORY_INTERVAL) / timeLen) * sizex;
				values[i] = orig + t*u + v*val;
			}
			if(n > 0)
			{
				ColorB	color = trackColors[j % TRACK_COLOR_COUNT];
				color.a = 240;
				dc->DrawPolyline(values, n, false, color);
			}
		}*/
	}
	else
	{
		// Separate graphs
		const float	spacingy = min(0.9f/(float)(shooters.size()+1), 0.2f);
		const float	sizex = 0.25f;
		const float	sizey = spacingy * 0.95f;

		int sw = dc->GetWidth();
		int sh = dc->GetHeight();

		ColorB white(0, 192, 255);
		ColorB whiteTrans(255, 255, 255, 179);
		ColorB redTrans(255, 0, 0, 179);

		for (unsigned int i = 0; i < shooters.size(); ++i)
		{
			CPuppet* pPuppet = shooters[i];
			Vec3	orig = Vec3(0.05f, 0.05f + (i+1)*spacingy, 0);

			// dim BG
			ColorB	bg1(0,0,0,128); ColorB	bg2(128,128,128,64);
			dc->DrawTriangle(orig, bg1, orig+u*sizex, bg1, orig+u*sizex+v*sizey, bg2);
			dc->DrawTriangle(orig, bg1, orig+u*sizex+v*sizey, bg2, orig+v*sizey, bg2);

			// Draw time lines
			float	timeLen = pPuppet->m_targetDamageHealthThrHistory->GetMaxSampleCount() *
				pPuppet->m_targetDamageHealthThrHistory->GetSampleInterval();
			const float tickInterval = 1.0f; //seconds
			unsigned int tickCount = (unsigned int)floor(timeLen / tickInterval);
			for (unsigned int j = 0; j < tickCount; ++j)
			{
				float	t = ((j * tickInterval) / timeLen) * sizex;
				dc->DrawLine(orig + t*u, ColorB(255,255,255,64), orig + t*u + v*sizey, ColorB(255,255,255,64));
			}

			// Draw curve
			CValueHistory<float>* history = 0;

			float s = 1.0f;

			if(pPuppet->GetAttentionTarget())
			{
				CAIActor* pTargetActor = 0;
				CAIObject* pTarget = (CAIObject*)pPuppet->GetAttentionTarget();
				assert(pTarget);
				if(pTarget->IsAgent())
					pTargetActor = pTarget->CastToCAIActor();
				else
				{
					CAIActor *pActor = CastToCAIActorSafe ( pTarget->GetAssociation().GetAIObject() );
					if( pActor && pActor->IsAgent() )
						pTargetActor = pActor;
				}

				if(pTargetActor)
				{
					history = pTargetActor->m_healthHistory;
					if(history)
					{
						float maxHealth = (float)pTargetActor->GetProxy()->GetActorMaxHealth();
						float maxHealthArmor = (float)(pTargetActor->GetProxy()->GetActorMaxHealth() + pTargetActor->GetProxy()->GetActorMaxArmor());
						float maxVal = maxHealthArmor / maxHealth;

						s = 1.0f / maxVal;

						// Draw value lines
						dc->DrawLine(orig, ColorB(255,255,255,128), orig + u*sizex, ColorB(255,255,255,128));
						dc->DrawLine(orig + v*sizey*0.5f*s, ColorB(255,255,255,64), orig + u*sizex + v*sizey*0.5f*s, ColorB(255,255,255,64));
						dc->DrawLine(orig + v*sizey*s, ColorB(255,255,255,128), orig + u*sizex + v*sizey*s, ColorB(255,255,255,128));


						unsigned int n = history->GetSampleCount();
						values.resize(n);
						float dt = history->GetSampleInterval();
						for (unsigned int j = 0; j < n; ++j)
						{
							float	val = min(history->GetSample(j) * s, 1.0f) * sizey;
							float	t = ((timeLen - j * dt) / timeLen) * sizex;
							values[j] = orig + t*u + v*val;
						}
						if(n > 0)
							dc->DrawPolyline(&values[0], n, false, ColorB(255,255,255,160),1.5f);
					}
				}
			}
			else
			{
				float maxVal = max(1.0f, pPuppet->m_targetDamageHealthThrHistory->GetMaxSampleValue());
				s = 1.0f / maxVal;
			}

			history = pPuppet->m_targetDamageHealthThrHistory;
			if(history)
			{
				unsigned int n = history->GetSampleCount();
				values.resize(n);
				float dt = history->GetSampleInterval();
				for (unsigned int j = 0; j < n; ++j)
				{
					float	val = min(history->GetSample(j) * s, 1.0f) * sizey;
					float	t = ((timeLen - j * dt) / timeLen) * sizex;
					values[j] = orig + t*u + v*val;
				}
				if(n > 0)
					dc->DrawPolyline(&values[0], n, false, ColorB(240,32,16,240));
			}

			char* szZone = "";
			switch(pPuppet->m_targetZone)
			{
			case AIZONE_OUT: szZone = "Out"; break;
			case AIZONE_WARN: szZone = "Warn"; break;
			case AIZONE_COMBAT_NEAR: szZone = "Combat-Near"; break;
			case AIZONE_COMBAT_FAR: szZone = "Combat-Far"; break;
			case AIZONE_KILL: szZone = "Kill"; break;
			case AIZONE_IGNORE: szZone = "Ignored"; break;
			}

			// Shooter name
			dc->Draw2dLabel(orig.x*sw+3, (orig.y-spacingy+sizey)*sh-25, 1.2f, white, false, "%s", pPuppet->GetName());

			float aliveTime = pPuppet->GetTargetAliveTime();
			const float accuracy = pPuppet->GetParameters().m_fAccuracy;
			char szAlive[32] = "inf";
			if (accuracy > 0.001f)
				_snprintf(szAlive, 32, "%.1fs", aliveTime / accuracy);

			IAIObject *pAttTarget = pPuppet->GetAttentionTarget();
			const float fFireReactionTime = (pAttTarget ? pPuppet->GetFiringReactionTime(pAttTarget->GetPos()) : 0.0f);
			const float fCurrentReactionTime = pPuppet->GetCurrentFiringReactionTime();
			const bool bFireReactionPassed = pPuppet->HasFiringReactionTimePassed();

			dc->Draw2dLabel(orig.x*sw+3, (orig.y-spacingy+sizey)*sh-10, 1.1f, (bFireReactionPassed ? whiteTrans : redTrans), false, "Alive: %s  React: %.1f / %.1f  Acc: %.3f  Zone: %s  %s",
				szAlive, max(fFireReactionTime - fCurrentReactionTime, 0.0f), fFireReactionTime,
				accuracy, szZone, pPuppet->IsAllowedToHitTarget() ? "FIRE": "");
		}
	}

/*
	std::vector<Vec3>	values;

	const float maxHealth = target.GetMaxHealth()

	// Player health
	{
		unsigned int n = pPlayer->m_healthHistory->GetSampleCount();
		values.resize(n);
		float dt = pPlayer->m_healthHistory->GetSampleInterval();
		for (unsigned int i = 0; i < n; ++i)
		{
			float	val = (pPlayer->m_healthHistory.GetSample(i) / target.GetMaxHealth()) * sizey;
			float	t = ((timeLen - i * dt) / timeLen) * sizex;
			values[i] = orig + t*u + v*val;
			}
		if(n > 0)
			dc->DrawPolyline(values, n, false, ColorB(255,255,255,160),1.5f);
		}



		unsigned int n = pPuppet->m_targetDamageHealthThrHistory->GetSampleCount();
		values.resize(n);
		float dt = pPlayer->m_healthHistory.GetSampleInterval();
		for (unsigned int i = 0; i < n; ++i)
		{
			float	val = (unit[j].GetTargetHealthThrHistorySample(i) / target.GetMaxHealth()) * sizey;
			float	t = ((timeLen - i * PROTO_HEALTH_HISTORY_INTERVAL) / timeLen) * sizex;
			values[i] = orig + t*u + v*val;
		}
		if(n > 0)
		{
			ColorB	color = trackColors[j % TRACK_COLOR_COUNT];
			color.a = 240;
			dc->DrawPolyline(values, n, false, color);
		}
	}
*/
}

void CAISystem::DrawDebugShape (const SDebugSphere & sphere)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CDebugDrawContext dc;
	dc->DrawSphere(sphere.pos, sphere.radius, sphere.color);
}

void CAISystem::DrawDebugShape (const SDebugBox & box)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CDebugDrawContext dc;
	dc->DrawOBB(box.obb, box.pos, true, box.color, eBBD_Faceted);
}

void CAISystem::DrawDebugShape (const SDebugLine & line)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CDebugDrawContext dc;
	dc->DrawLine(line.start, line.color, line.end, line.color);
}

template <typename ShapeContainer>
void CAISystem::DrawDebugShapes (ShapeContainer & shapes, float dt)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	for (unsigned int i = 0; i < shapes.size();)
	{
		typename ShapeContainer::value_type & shape = shapes[i];
		// NOTE Mai 29, 2007: <pvl> draw it at least once
		DrawDebugShape (shape);

		shape.time -= dt;
		if (shape.time < 0)
		{
			shapes[i] = shapes.back();
			shapes.pop_back();
		}
		else
		{
			++i;
		}
	}
}

void CAISystem::DebugDrawFakeTracers() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CDebugDrawContext dc;
	dc->SetAlphaBlended(true);

	for(size_t i = 0; i < m_DEBUG_fakeTracers.size(); ++i)
	{
		Vec3 p0 = m_DEBUG_fakeTracers[i].p0;
		Vec3 p1 = m_DEBUG_fakeTracers[i].p1;
		Vec3 dir = p1 - p0;
		float u = 1 - m_DEBUG_fakeTracers[i].t / m_DEBUG_fakeTracers[i].tmax;
		p0 += dir*u*0.5f;
		p1 = p0 + dir*0.5f;

		float	a = (m_DEBUG_fakeTracers[i].a * m_DEBUG_fakeTracers[i].t / m_DEBUG_fakeTracers[i].tmax)*0.75f + 0.25f;
		Vec3	mid((p0+p1)/2);
		dc->DrawLine(p0, ColorB(128, 128, 128, 0), mid, ColorB(255, 255, 255, (uint8)(255*a)), 6.0f);
		dc->DrawLine(p1, ColorB(128, 128, 128, 0), mid, ColorB(255, 255, 255, (uint8)(255*a)), 6.0f);
	}
}

void CAISystem::DebugDrawFakeHitEffects() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CDebugDrawContext dc;
	dc->SetAlphaBlended(true);

	for(size_t i = 0; i < m_DEBUG_fakeHitEffect.size(); ++i)
	{
		float	a = m_DEBUG_fakeHitEffect[i].t / m_DEBUG_fakeHitEffect[i].tmax;
		Vec3	pos = m_DEBUG_fakeHitEffect[i].p + m_DEBUG_fakeHitEffect[i].n * (1 - sqr(a));
		float	r = m_DEBUG_fakeHitEffect[i].r * (0.5f + (1-a)*0.5f);
		dc->DrawSphere(pos, r, ColorB(m_DEBUG_fakeHitEffect[i].c.r,m_DEBUG_fakeHitEffect[i].c.g,m_DEBUG_fakeHitEffect[i].c.b,(uint8)(m_DEBUG_fakeHitEffect[i].c.a*a)));
	}
}

void CAISystem::DebugDrawFakeDamageInd() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	int mode = gAIEnv.CVars.DrawFakeDamageInd;

	CAIObject* pPlayer = GetPlayer();
	if(pPlayer)
	{
		// full screen quad
		CDebugDrawContext dc;
		dc->Init2DMode();
		dc->SetAlphaBlended(true);
		dc->SetBackFaceCulling(false);

		// Fullscreen flash for damage indication.
		if(m_DEBUG_screenFlash > 0.0f)
		{
			float	f = m_DEBUG_screenFlash * 2.0f;
			float	a = clamp(f, 0.0f, 1.0f);
			ColorB	color(239, 50, 25, (uint8)(a*255));
			dc->DrawTriangle(Vec3(0,0,0), color, Vec3(1,0,0), color, Vec3(1,1,0), color);
			dc->DrawTriangle(Vec3(0,0,0), color, Vec3(1,1,0), color, Vec3(0,1,0), color);
		}

		// Damage indicator triangles
		Matrix33	basis;
		basis.SetRotationVDir(pPlayer->GetViewDir());
		Vec3 u = basis.GetColumn0();
		Vec3 v = basis.GetColumn1();
		Vec3 w = basis.GetColumn2();

		float	rw = (float)dc->GetWidth();
		float	rh = (float)dc->GetHeight();
		float	as = rh/rw;

		const float FOV = dc->GetCameraFOV() * 0.95f;

		for (unsigned int i = 0; i < m_DEBUG_fakeDamageInd.size(); ++i)
		{
			Vec3	dir = m_DEBUG_fakeDamageInd[i].p - pPlayer->GetPos();
			dir.NormalizeSafe();
			float	x = u.Dot(dir);
			float	y = v.Dot(dir);
			float	d = sqrtf(sqr(x) + sqr(y));
			if(d < 0.00001f)
				continue;
			x /= d;
			y /= d;
			float	nx = y;
			float	ny = -x;

			const float	r0 = 0.15f;
			const float	r1 = 0.25f;
			const float	wi = 0.04f;

			float	a = 1 - sqr(1 - m_DEBUG_fakeDamageInd[i].t / m_DEBUG_fakeDamageInd[i].tmax);

			bool targetVis = false;

			if(mode == 2)
			{
				Vec3 dirLocal(u.Dot(dir), v.Dot(dir), w.Dot(dir));
				if(dirLocal.y > 0.1f)
				{
					dirLocal.x /= dirLocal.y;
					dirLocal.z /= dirLocal.y;

					float tz = tanf(FOV/2);
					float tx = rw/rh * tz;

					if(fabsf(dirLocal.x) < tx && fabsf(dirLocal.z) < tz)
						targetVis = true;
				}
			}

			ColorB color(239, 50, 25, (uint8)(a*255));

			if(!targetVis)
			{
				dc->DrawTriangle(Vec3(0.5f + (x*r0)*as, 0.5f - (y*r0),0), color,
					Vec3(0.5f + (x*r1+nx*wi)*as, 0.5f - (y*r1+ny*wi),0), color,
					Vec3(0.5f + (x*r1-nx*wi)*as, 0.5f - (y*r1-ny*wi),0), color);
			}
			else
			{
				// Draw silhouette when on FOV.
				static std::vector<Vec3> pos2d;
				static std::vector<Vec3> pos2dSil;
				static std::vector<ColorB> colorSil;
				pos2d.resize(m_DEBUG_fakeDamageInd[i].verts.size());

				for (unsigned int j = 0; j < m_DEBUG_fakeDamageInd[i].verts.size(); ++j)
				{
					const Vec3& v2 = m_DEBUG_fakeDamageInd[i].verts[j];
					Vec3& o = pos2d[j];
					dc->ProjectToScreen(v2.x, v2.y, v2.z, &o.x, &o.y, &o.z);
					o.x /= 100.0f;
					o.y /= 100.0f;
				}

				pos2dSil.clear();
				ConvexHull2D(pos2dSil, pos2d);

				if(pos2dSil.size() > 2)
				{
					colorSil.resize(pos2dSil.size());
					float miny = FLT_MAX, maxy = -FLT_MAX;
					for (unsigned int j = 0; j < pos2dSil.size(); ++j)
					{
						miny = min(miny, pos2dSil[j].y);
						maxy = max(maxy, pos2dSil[j].y);
					}
					float range = maxy - miny;
					if(range > 0.0001f)
						range = 1.0f / range;
					for (unsigned int j = 0; j < pos2dSil.size(); ++j)
					{
						float aa = clamp( (1 - (pos2dSil[j].y - miny) * range) * 2.0f, 0.0f, 1.0f);
						colorSil[j] = color;
						colorSil[j].a *= (uint8)(aa * a);
					}
					dc->DrawPolyline(&pos2dSil[0], pos2dSil.size(), true, &colorSil[0], 2.0f);
				}
			}
		}

		// Draw ambient fire indicators
		if(gAIEnv.CVars.DebugDrawDamageControl > 1)
		{
			const Vec3& playerPos = pPlayer->GetPos();
			AIObjectOwners::const_iterator aio = m_Objects.find(AIOBJECT_PUPPET);
			for(; aio != m_Objects.end(); ++aio)
			{
				if(aio->first != AIOBJECT_PUPPET) break;
				CPuppet* pPuppet = aio->second.GetAIObject()->CastToCPuppet();
				if(!pPuppet) continue;
				if(!pPuppet->IsEnabled()) continue;
				if(Distance::Point_PointSq(playerPos, pPuppet->GetPos()) > sqr(150.0f)) continue;

				Vec3	dir = pPuppet->GetPos() - playerPos;
				float	x = u.Dot(dir);
				float	y = v.Dot(dir);
				float	d = sqrtf(sqr(x) + sqr(y));
				if(d < 0.00001f)
					continue;
				x /= d;
				y /= d;
				float	nx = y;
				float	ny = -x;

				const float	r0 = 0.25f;
				const float	r1 = 0.28f;
				const float	w2 = 0.01f;

				ColorB color(255, 255, 255, pPuppet->IsAllowedToHitTarget() ? 255 : 64);
				dc->DrawTriangle(Vec3(0.5f + (x*r0)*as, 0.5f - (y*r0),0), color,
					Vec3(0.5f + (x*r1+nx*w2)*as, 0.5f - (y*r1+ny*w2),0), color,
					Vec3(0.5f + (x*r1-nx*w2)*as, 0.5f - (y*r1-ny*w2),0), color);
			}
		}
	}
}

// Draw rings around player to assist in gauging target distance
void CAISystem::DebugDrawPlayerRanges() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CDebugDrawContext dc;
	dc->SetAlphaBlended(true);

	CAIObject* player = GetPlayer();
	if(player && !player->GetPos().IsZero())
		dc->DrawCircles(player->GetPos(), 5.f, 20.f, 4, Vec3(1, 0, 0), Vec3(1, 1, 0));
}

// Draw Perception Indicators
void CAISystem::DebugDrawPerceptionIndicators()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	static CTimeValue	lastTime(-1.0f);
	if(lastTime.GetSeconds() < 0.0f)
		lastTime = GetFrameStartTime();
	CTimeValue	time = GetFrameStartTime();
	float	dt = (time - lastTime).GetSeconds();
	lastTime = time;

	CDebugDrawContext dc;
	dc->SetAlphaBlended(true);

	for( std::list<SPerceptionDebugLine>::iterator lineIt = m_lstDebugPerceptionLines.begin(); lineIt != m_lstDebugPerceptionLines.end(); )
	{
		SPerceptionDebugLine&	line = (*lineIt);
		line.time -= dt;
		if( line.time < 0 )
			lineIt = m_lstDebugPerceptionLines.erase(lineIt);
		else
		{
			dc->DrawLine(line.start, line.color, line.end, line.color, line.thickness);
			lineIt++;
		}
	}
}

// Draw Perception Modifiers
void CAISystem::DebugDrawPerceptionModifiers()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CDebugDrawContext dc;
	dc->SetAlphaBlended(true);

	ColorB color(20,255,255);
	PerceptionModifierShapeMap::iterator pmsi = m_mapPerceptionModifiers.begin(),pmsiEnd = m_mapPerceptionModifiers.end();
	for (;pmsi!=pmsiEnd;++pmsi)
	{
		SPerceptionModifierShape &shape = pmsi->second;
		if (shape.shape.empty()) continue;

		ListPositions::iterator first=shape.shape.begin();
		ListPositions::iterator second=++shape.shape.begin();
		for (; first!=shape.shape.end(); ++first, ++second)
		{
			Vec3 firstTop(first->x, first->y, shape.aabb.max.z);
			Vec3 firstBottom(first->x, first->y, shape.aabb.min.z);

			if (second == shape.shape.end())
			{
				// Handle the last side of shape based on whether shape is open or closed
				if (!shape.closed)
				{
					dc->DrawLine(firstBottom, color, firstTop, color, 1.f);
					continue;
				}
				else
					second = shape.shape.begin();
			}

			Vec3 secondTop(second->x, second->y, shape.aabb.max.z);
			Vec3 secondBottom(second->x, second->y, shape.aabb.min.z);

			dc->DrawLine(firstBottom, color, secondBottom, color, 1.f);
			dc->DrawLine(firstBottom, color, firstTop, color, 1.f);
			dc->DrawLine(firstTop, color, secondTop, color, 1.f);
		}
	}
}

void CAISystem::DebugDrawTargetTracks() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	gAIEnv.pTargetTrackManager->DebugDraw();
}

void CAISystem::DebugDrawCodeCoverage() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	gAIEnv.pCodeCoverageGUI->DebugDraw(gAIEnv.CVars.CodeCoverage);
}

void CAISystem::DebugDrawPerceptionManager()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CPerceptionManager::GetInstance()->DebugDraw(gAIEnv.CVars.DebugPerceptionManager);
	CPerceptionManager::GetInstance()->DebugDrawPerformance(gAIEnv.CVars.DebugPerceptionManager);
}

void CAISystem::DebugDrawNavigation() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	m_pNavigation->DebugDraw();
}

void CAISystem::DebugDrawGraph(int debugDrawValue) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CDebugDrawContext dc;

	if (debugDrawValue == 72)
		DebugDrawGraphErrors(m_pGraph);
	else if (debugDrawValue == 74)
		DebugDrawGraph(m_pGraph);
	else if (debugDrawValue == 79 || debugDrawValue == 179 || debugDrawValue == 279)
	{
		std::vector<Vec3> pos;
		pos.push_back(dc->GetCameraPos());
		DebugDrawGraph(m_pGraph, &pos, 15);
	}
}

void CAISystem::DebugDrawDeadBodies() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CDebugDrawContext dc;
	
	for (unsigned int i = 0, ni = m_deadBodies.size(); i < ni; ++i)
	{
		const Vec3& pos = m_deadBodies[i].pos;
		dc->DrawSphere(pos, 0.5f, ColorB(0, 0, 0));
		dc->Draw3dLabel(pos + Vec3(0,0,0.8f),1,"R.I.P. %d (%.1fs)", i, m_deadBodies[i].t);
	}
}

void CAISystem::DebugDrawNavModifiers() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	m_pNavigation->DebugDrawNavModifiers();
}

void CAISystem::DebugDrawLightManager()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	m_lightManager.DebugDraw();
}

void CAISystem::DebugDrawP0AndP1() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CDebugDrawContext dc;

	IEntity* ent0 = gEnv->pEntitySystem->FindEntityByName("p0");
	IEntity* ent1 = gEnv->pEntitySystem->FindEntityByName("p1");
	if (ent0 && ent1)
	{
		Vec3 p0 = ent0->GetWorldPos();
		Vec3 p1 = ent1->GetWorldPos();

		dc->DrawSphere(p0, 0.7f, ColorB(255,255,255,128));
		dc->DrawLine(p0, ColorB(255,255,255), p1, ColorB(255,255,255));

		Vec3 hit;
		float hitDist = 0;
		if (IntersectSweptSphere(&hit, hitDist, Lineseg(p0, p1), 0.65f, AICE_STATIC))
		{
			Vec3 dir = p1 - p0;
			dir.Normalize();
			dc->DrawSphere(p0 + dir*hitDist, 0.7f, ColorB(255,0,0));
		}
	}
}

// FIXME Mar 23, 2009: <pvl> just because of Convert() prototype!
#include "LayeredNavMesh/LayeredNavMeshRegion.h"
void CAISystem::DebugDrawPolygonSetOps() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	SShape * poly0 = GetAISystem()->GetGenericShapeOfName ("poly0");
	SShape * poly1 = GetAISystem()->GetGenericShapeOfName ("poly1");
	if (poly0 && poly1)
	{
		using PSO::Allocator;
		using PSO::PoolAllocator;
		using PSO::Polygon;
		using PSO::PolygonSetOp;

		const unsigned memPoolSize = 10 * 1024;
		_smart_ptr<Allocator> alloc ( new PoolAllocator (memPoolSize) );
		Polygon * p0 = Convert (0, *poly0, alloc);
		Polygon * p1 = Convert (1, *poly1, alloc);

		PolygonSetOp op (p0, p1, alloc);

		DebugDrawSourcePolygon (p0);
		DebugDrawSourcePolygon (p1);

		if (gEnv->pEntitySystem->FindEntityByName("AND"))
			DebugDrawResultPolygon (op.Conjunction ());
		else if (gEnv->pEntitySystem->FindEntityByName("OR"))
			DebugDrawResultPolygon (op.Disjunction ());
		else if (gEnv->pEntitySystem->FindEntityByName("DIFF"))
			DebugDrawResultPolygon (op.Difference ());
		else if (gEnv->pEntitySystem->FindEntityByName("XOR"))
			DebugDrawResultPolygon (op.SymmetricalDifference ());
	}
}

void CAISystem::DebugDrawPuppetPaths()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CAIObject *pAI = GetAIObjectByName("DebugRequestPathInDirection");
	if (pAI && pAI->GetType() == AIOBJECT_PUPPET)
	{
		CPuppet* puppet = (CPuppet *) pAI;
		Vec3 dir = puppet->GetMoveDir();

		static CTimeValue lastTime = GetAISystem()->GetFrameStartTime();
		CTimeValue thisTime = GetAISystem()->GetFrameStartTime();
		const float regenTime = 1.0f;

		if ((thisTime - lastTime).GetSeconds() > regenTime)
		{
			lastTime = thisTime;
			puppet->m_Path.Clear("DebugRequestPathInDirection");
			Vec3 startPos = puppet->GetPhysicsPos();
			const float maxDist = 15.0f;

			m_pPathfinder->RequestPathInDirection(startPos, dir, maxDist, puppet, 0.0f);
		}

		DebugDrawPathSingle(puppet);
	}
}

void CAISystem::DebugDrawBestPositions() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	IEntity* ent1 = gEnv->pEntitySystem->FindEntityByName("GetBestPositionStart");
	IEntity* ent2 = gEnv->pEntitySystem->FindEntityByName("GetBestPositionEnd");
	if (ent1 && ent2)
	{
		const float radius = 0.3f;
		const float maxCost = 15.0f;

		const Vec3 & startPos = ent1->GetPos();
		const Vec3 & endPos = ent2->GetPos();

		CStandardHeuristic heuristic;
		IAISystem::tNavCapMask navmask = IAISystem::NAV_TRIANGULAR | IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_VOLUME | IAISystem::NAV_WAYPOINT_3DSURFACE | IAISystem::NAV_FLIGHT ;
		static const AgentPathfindingProperties navProperties(
			navmask, 
			0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 
			5.0f, 10000.0f, -10000.0f, 0.0f, 20.0f, 0.0);
		heuristic.SetProperties(PathfindingHeuristicProperties(radius, navProperties));
		Vec3 pos = m_pPathfinder->GetBestPosition(
			heuristic, maxCost, startPos, endPos, 0, navmask  );

		CDebugDrawContext dc;
		dc->DrawSphere(pos + Vec3(0, 0, 1), radius, ColorB(255, 255, 255));
		dc->DrawCircles(startPos, maxCost, maxCost, 1, Vec3(1, 0, 0), Vec3(1, 1, 0));
	}
}

void CAISystem::DebugDrawCheckCapsules() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	IEntity* ent = gEnv->pEntitySystem->FindEntityByName("CheckCapsule");
	if (ent)
	{
		// match the params in CheckWalkability
		const float radius = walkabilityRadius;
		const Vec3 dir(0, 0, 0.9f);

		const Vec3 & pos = ent->GetPos();

		bool result = OverlapCapsule(Lineseg(pos, pos + dir), radius, AICE_ALL);
		ColorB color;
		if (result)
			color.Set(255, 0, 0);
		else
			color.Set(0, 255, 0);
		CDebugDrawContext dc;
		dc->DrawSphere(pos, radius, color);
		dc->DrawSphere(pos + dir, radius, color);
	}
}

void CAISystem::DebugDrawCheckRay() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	IEntity* entFrom = gEnv->pEntitySystem->FindEntityByName("CheckRayFrom");
	IEntity* entTo = gEnv->pEntitySystem->FindEntityByName("CheckRayTo");
	if (entFrom && entTo)
	{
		const Vec3 & posFrom = entFrom->GetPos();
		const Vec3 & posTo = entTo->GetPos();

		bool result = OverlapSegment(Lineseg(posFrom, posTo), AICE_ALL);
		ColorB color;
		if (result)
			color.Set(255, 0, 0);
		else
			color.Set(0, 255, 0);
		
		CDebugDrawContext dc;
		dc->DrawLine(posFrom, color, posTo, color);
	}
}

void CAISystem::DebugDrawCheckGetEnclosing() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	IEntity* ent = gEnv->pEntitySystem->FindEntityByName("CheckGetEnclosing");
	if (ent)
	{
		const float radius = 0.3f;
		const Vec3 & pos = ent->GetPos();
		Vec3 closestValid;
		unsigned int nodeIndex = m_pGraph->GetEnclosing(pos, IAISystem::NAVMASK_ALL, radius, 0, 5.0f, &closestValid, false);
		GraphNode* pNode = m_pGraph->GetNodeManager().GetNode(nodeIndex);
		ColorB color;
		if (pNode)
			color.Set(0, 255, 0);
		else
			color.Set(255, 0, 0);
		CDebugDrawContext dc;
		dc->DrawSphere(pos, radius, color);
		dc->DrawSphere(closestValid, radius, color);
		if (pNode)
			dc->DrawSphere(pNode->GetPos() + Vec3(0, 0, 0.5), 0.1f, ColorB(255, 255, 255));
	}
}

void CAISystem::DebugDrawCheckWalkability()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	IEntity* entFrom = gEnv->pEntitySystem->FindEntityByName("CheckWalkabilityFrom");
	IEntity* entTo = gEnv->pEntitySystem->FindEntityByName("CheckWalkabilityTo");
	if (entFrom && entTo)
	{
		CDebugDrawContext dc;
		
		{
			static CTimeValue lastTime = gEnv->pTimer->GetAsyncTime();
			CTimeValue thisTime = gEnv->pTimer->GetAsyncTime();
			float deltaT = (thisTime - lastTime).GetSeconds();
			static float testsPerSec = 0.0f;
			if (deltaT > 2.0f)
			{
				lastTime = thisTime;
				testsPerSec = g_CheckWalkabilityCalls / deltaT;
				g_CheckWalkabilityCalls = 0;
			}
			const int column = 5;
			const int row = 50;
			char buff[256];
			sprintf(buff,"%5.2f walkability calls per sec", testsPerSec);
			dc->Draw2dLabel(column, row, buff, ColorB(255, 0, 255));
		}

		const Vec3 & posFrom = entFrom->GetPos();
		const Vec3 & posTo = entTo->GetPos();
		int nBuildingID;
		IVisArea* pVisArea;
		m_pNavigation->CheckNavigationType(posFrom, nBuildingID, pVisArea, IAISystem::NAV_WAYPOINT_HUMAN);
		const SpecialArea* sa = m_pNavigation->GetSpecialArea(nBuildingID);
		const float radius = 0.3f;
		const bool all = true;
		bool result = CheckWalkability(posFrom, posTo, 0.0f, true, sa ? sa->GetPolygon() : ListPositions(), all ? AICE_ALL : AICE_STATIC);
		ColorB color;
		if (result)
			color.Set( 0, 255, 0);
		else
			color.Set(255,   0, 0);
		dc->DrawSphere(posFrom, radius, color);
		dc->DrawSphere(posTo, radius, color);
	}
}

void CAISystem::DebugDrawCheckWalkabilityTime() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	IEntity* entFrom = gEnv->pEntitySystem->FindEntityByName("CheckWalkabilityTimeFrom");
	IEntity* entTo = gEnv->pEntitySystem->FindEntityByName("CheckWalkabilityTimeTo");
	if (entFrom && entTo)
	{
		const Vec3 & posFrom = entFrom->GetPos();
		const Vec3 & posTo = entTo->GetPos();
		const float radius = 0.3f;
		const bool all = true;
		bool result;
		const int num = 100;
		CTimeValue startTime = gEnv->pTimer->GetAsyncTime();
		for (int i = 0 ; i < num ; ++i)
			result = CheckWalkability(posFrom, posTo, 0.0f, true, ListPositions(), all ? AICE_ALL : AICE_STATIC);
		CTimeValue endTime = gEnv->pTimer->GetAsyncTime();
		ColorB color;
		if (result)
			color.Set(0, 255, 0);
		else
			color.Set(255, 0, 0);
		CDebugDrawContext dc;
		dc->DrawSphere(posFrom, radius, color);
		dc->DrawSphere(posTo, radius, color);
		float time = (endTime - startTime).GetSeconds();

		const int column = 5;
		const int row = 50;
		char buff[256];
		sprintf(buff,"%d walkability calls in %5.2f sec", num, time);
		dc->Draw2dLabel(column, row, buff, ColorB(255, 0, 255));
	}
}

void CAISystem::DebugDrawCheckFloorPos() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	IEntity* ent = gEnv->pEntitySystem->FindEntityByName("CheckFloorPos");
	if (ent)
	{
		const Vec3 & pos = ent->GetPos();
		const float up = 0.5f;
		const float down = 5.0f;
		const float radius = 0.1f;
		Vec3 floorPos;
		bool result = GetFloorPos(floorPos, pos, up, down, radius, AICE_ALL);
		ColorB color;
		if (result)
			color.Set(0, 255, 0);
		else
			color.Set(255, 0, 0);
		
		CDebugDrawContext dc;
		dc->DrawSphere(floorPos, radius, color);
	}
}

void CAISystem::DebugDrawCheckGravity() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	IEntity* ent = gEnv->pEntitySystem->FindEntityByName("CheckGravity");
	if (ent)
	{
		Vec3 pos = ent->GetPos();
		Vec3 gravity;
		pe_params_buoyancy junk;
		gEnv->pPhysicalWorld->CheckAreas(pos, gravity, &junk);
		ColorB color(255, 255, 255);
		const float lenScale = 0.3f;
		CDebugDrawContext dc;
		dc->DrawLine(pos, color, pos + lenScale * gravity, color);
	}
}

void CAISystem::DebugDrawGetTeleportPos() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	IEntity* ent = gEnv->pEntitySystem->FindEntityByName("GetTeleportPos");
	if (ent)
	{
		const Vec3 &pos = ent->GetPos();
		Vec3 teleportPos = pos;

		IAISystem::tNavCapMask navCapMask = IAISystem::NAV_TRIANGULAR | IAISystem::NAV_WAYPOINT_HUMAN;

		int nBuildingID;
		IVisArea *pArea;
		IAISystem::ENavigationType currentNavType = m_pNavigation->CheckNavigationType(pos, nBuildingID, pArea, navCapMask);

		bool canTeleport = gAIEnv.pNavigation->GetNavRegion(currentNavType, gAIEnv.pGraph)->GetTeleportPosition(pos, teleportPos, "GetTeleportPos");

		CDebugDrawContext dc;
		if (canTeleport)
			dc->DrawSphere(teleportPos, 0.5f, ColorB(0, 255, 0));
		else
			dc->DrawSphere(pos, 0.5f, ColorB(255, 0, 0));
	}
}

void CAISystem::DebugDrawTestNode() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	// Simpler triangular obstacle query.
	IEntity* pEntTest = gEnv->pEntitySystem->FindEntityByName("test");
	if (!pEntTest)
		return;

	const Vec3& pos = pEntTest->GetWorldPos();
	const float rad = 0.6f;

	unsigned int nodeIndex = m_pGraph->GetEnclosing(pos, IAISystem::NAV_VOLUME, 0.6f, 0, 5.0f, 0, true, "Test");
	GraphNode* pNode = m_pGraph->GetNodeManager().GetNode(nodeIndex);
	CDebugDrawContext dc;
	if (pNode)
	{
		dc->DrawLine(pos, ColorB(255,255,255), pNode->GetPos(), ColorB(255,255,255), 3.0f);
		dc->DrawSphere(pNode->GetPos(), 0.1f, ColorB(255,255,255));

		for (unsigned int link = pNode->firstLinkIndex; link; link = m_pGraph->GetLinkManager().GetNextLink(link))
		{
			unsigned int connectedIndex = m_pGraph->GetLinkManager().GetNextNode(link);
			GraphNode *pConnectedNode = m_pGraph->GetNodeManager().GetNode(connectedIndex);
			if (!pConnectedNode)
				continue;

			bool blocked = false;
			if (m_pGraph->GetLinkManager().GetRadius(link) < rad)
				blocked = true;
			float extraLinkFactor = gAIEnv.pNavigation->GetExtraLinkCost(pNode->GetPos(), pConnectedNode->GetPos());
			if (extraLinkFactor < 0.0f)
				blocked = true;

			ColorB color(0,196,255);
			if (blocked)
				color = ColorB(255,0,0);

			dc->DrawLine(pNode->GetPos(), color, pConnectedNode->GetPos(), color, 3.0f);
		}
	}
	else
	{
		dc->Draw3dLabel(pos, 1.5f, "No Node");
	}
}

// Draw debug shapes
// These shapes come from outside the AI system (such as some debug code in script bind in CryAction) but related to AI.
void CAISystem::DebugDrawDebugShapes()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	const float dt = GetFrameDeltaTime();
	DrawDebugShapes (m_vecDebugLines, dt);
	DrawDebugShapes (m_vecDebugBoxes, dt);
	DrawDebugShapes (m_vecDebugSpheres, dt);
}

void CAISystem::DebugDrawGroupTactic()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	for(AIGroupMap::iterator it = m_mapAIGroups.begin(); it != m_mapAIGroups.end(); ++it)
		it->second->DebugDraw();
}

void CAISystem::DebugDrawDamageParts() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CAIPlayer* pPlayer = CastToCAIPlayerSafe(GetPlayer());
	CDebugDrawContext dc;
	if(pPlayer && pPlayer->GetDamageParts())
	{
		DamagePartVector*	parts = pPlayer->GetDamageParts();
		for(DamagePartVector::iterator it = parts->begin(); it != parts->end(); ++it)
		{
			SAIDamagePart&	part = *it;
			dc->Draw3dLabel(part.pos, 1,"^ DMG:%.2f\n  VOL:%.1f", part.damageMult, part.volume);
		}
	}
}

// Draw the approximate stance size for the player.
void CAISystem::DebugDrawStanceSize() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CAIObject*	player = GetPlayer();
	if(player && player->GetProxy())
	{
		SAIBodyInfo	bodyInfo;
		player->GetProxy()->QueryBodyInfo(bodyInfo);
		Vec3	pos = player->GetPhysicsPos();
		AABB	aabb(bodyInfo.stanceSize);
		aabb.Move(pos);
		CDebugDrawContext dc;
		dc->DrawAABB(aabb, true, ColorB(255, 255, 255, 128), eBBD_Faceted);
	}
}

void CAISystem::DebugDrawForceAGSignal() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	ColorB colorRed(255, 0, 0);
	const char* szInput = gAIEnv.CVars.ForceAGSignal;

	CDebugDrawContext dc;
	dc->Draw2dLabel(10, dc->GetHeight() - 90, 2.0f, colorRed, false, "Forced AG Signal Input: %s", szInput);
}

void CAISystem::DebugDrawForceAGAction() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	ColorB colorRed(255, 0, 0);
	const char* szInput = gAIEnv.CVars.ForceAGAction;

	CDebugDrawContext dc;
	dc->Draw2dLabel(10, dc->GetHeight() - 60, 2.0f, colorRed, false, "Forced AG Action Input: %s", szInput);
}

void CAISystem::DebugDrawForceStance() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	ColorB colorRed(255, 0, 0);
	const char* szStance = GetStanceName(gAIEnv.CVars.ForceStance);

	CDebugDrawContext dc;
	dc->Draw2dLabel(10, dc->GetHeight() - 30, 2.0f, colorRed, false, "Forced Stance: %s", szStance);
}

void CAISystem::DebugDrawForcePosture() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	ColorB colorRed(255, 0, 0);
	const char* szPosture = gAIEnv.CVars.ForcePosture;

	CDebugDrawContext dc;
	dc->Draw2dLabel(10, dc->GetHeight() - 30, 2.0f, colorRed, false, "Forced Posture: %s", szPosture);
}

// Player actions
void CAISystem::DebugDrawPlayerActions() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CAIPlayer* pPlayer = CastToCAIPlayerSafe(GetPlayer());
	if (pPlayer)
		pPlayer->DebugDraw();
}

void CAISystem::DebugDrawCrowdControl()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CDebugDrawContext dc;
	
	Vec3 camPos = dc->GetCameraPos();

	VectorSet<const CAIActor*>	avoidedActors;

	AIObjectOwners::const_iterator ai = m_Objects.find(AIOBJECT_PUPPET);
	for (; ai != m_Objects.end(); ++ai)
	{
		if (ai->first != AIOBJECT_PUPPET)
			break;
		CAIObject* obj = ai->second.GetAIObject();
		CPuppet* pPuppet = obj->CastToCPuppet();
		if (!pPuppet)
			continue;
		if (!pPuppet->IsEnabled())
			continue;
		if (!pPuppet->m_steeringEnabled)
			continue;
		if (Distance::Point_PointSq(camPos, pPuppet->GetPos()) > sqr(150.0f))
			continue;

		Vec3 center = pPuppet->GetPhysicsPos() + Vec3(0,0,0.75f);
		const float rad = pPuppet->GetMovementAbility().pathRadius;

		/* Draw circle sector */
		pPuppet->m_steeringOccupancy.DebugDraw(center, ColorB(0, 196, 255, 240));

		for (unsigned int i = 0, ni = pPuppet->m_steeringObjects.size(); i < ni; ++i)
		{
			const CAIActor* pActor = pPuppet->m_steeringObjects[i]->CastToCAIActor();
			avoidedActors.insert(pActor);
		}
	}

	for (unsigned int i = 0, ni = avoidedActors.size(); i < ni; ++i)
	{
		const CAIActor* pActor = avoidedActors[i];
		if (pActor->GetType() == AIOBJECT_VEHICLE)
		{
			IEntity* pActorEnt = pActor->GetEntity();
			AABB localBounds;
			pActorEnt->GetLocalBounds(localBounds);
			const Matrix34& tm = pActorEnt->GetWorldTM();

			SAIRect3 r;
			GetFloorRectangleFromOrientedBox(tm, localBounds, r);
			// Extend the box based on velocity.
			Vec3 vel = pActor->GetVelocity();
			float speedu = r.axisu.Dot(vel) * 0.25f;
			float speedv = r.axisv.Dot(vel) * 0.25f;
			if (speedu > 0)
				r.max.x += speedu;
			else
				r.min.x += speedu;
			if (speedv > 0)
				r.max.y += speedv;
			else
				r.min.y += speedv;

			Vec3 rect[4];
			rect[0] = r.center + r.axisu * r.min.x + r.axisv * r.min.y;
			rect[1] = r.center + r.axisu * r.max.x + r.axisv * r.min.y;
			rect[2] = r.center + r.axisu * r.max.x + r.axisv * r.max.y;
			rect[3] = r.center + r.axisu * r.min.x + r.axisv * r.max.y;

			/* ??? */
			dc->DrawPolyline(rect, 4, true, ColorB(0,196,255,128));
		}
		else
		{
			Vec3 pos = avoidedActors[i]->GetPhysicsPos() + Vec3(0,0,0.5f);
			const float rad = avoidedActors[i]->GetMovementAbility().pathRadius;
			/* Draw additional cyan circle around puppets */
			dc->DrawCircleOutline(pos, rad, ColorB(0,196,255,128));
		}
	}
}

static const char* GetMovemengtUrgencyLabel(int idx)
{
	switch(idx > 0 ? idx : -idx)
	{
	case 0: return "Zero"; break;
	case 1: return "Slow"; break;
	case 2: return "Walk"; break;
	case 3: return "Run"; break;
	default: break;
	}
	return "Sprint"; 
}

void CAISystem::DebugDrawAdaptiveUrgency() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CDebugDrawContext dc;
	
	Vec3 camPos = dc->GetCameraPos();

	int mode = gAIEnv.CVars.DebugDrawAdaptiveUrgency;

	AIObjectOwners::const_iterator ai = m_Objects.find(AIOBJECT_PUPPET);
	for (; ai != m_Objects.end(); ++ai)
	{
		if (ai->first != AIOBJECT_PUPPET)
			break;
		CAIObject* obj = ai->second.GetAIObject();
		CPuppet* pPuppet = obj->CastToCPuppet();
		if (!pPuppet)
			continue;
		if (!pPuppet->IsEnabled())
			continue;
		if (Distance::Point_PointSq(camPos, pPuppet->GetPos()) > sqr(150.0f))
			continue;

		if (pPuppet->m_adaptiveUrgencyScaleDownPathLen <= 0.0001f)
			continue;

		int minIdx = MovementUrgencyToIndex(pPuppet->m_adaptiveUrgencyMin);
		int maxIdx = MovementUrgencyToIndex(pPuppet->m_adaptiveUrgencyMax);
		int curIdx = MovementUrgencyToIndex(pPuppet->GetState()->fMovementUrgency);

		SAIBodyInfo bi;
		AIAssert(pPuppet->GetProxy());
		pPuppet->GetProxy()->QueryBodyInfo(bi);
		bi.stanceSize.Move(pPuppet->GetPhysicsPos());
		Vec3 top = bi.stanceSize.GetCenter() + Vec3(0,0,bi.stanceSize.GetSize().z/2 + 0.3f);

		if (mode > 1)
			dc->DrawAABB(bi.stanceSize, false, ColorB(240,220,0,128), eBBD_Faceted);

		if (curIdx < maxIdx)
		{
			dc->DrawAABB(bi.stanceSize, true, ColorB(240,220,0,128), eBBD_Faceted);
			dc->Draw3dLabel(top, 1.1f, "%s -> %s\n%d", GetMovemengtUrgencyLabel(maxIdx), GetMovemengtUrgencyLabel(curIdx), maxIdx - curIdx);
		}
	}
}

static void DrawRadarCircle(const Vec3& pos, float radius, const ColorB& color, const Matrix34& world, const Matrix34& screen)
{
	Vec3	last;

	float r = world.TransformVector(Vec3(radius,0,0)).GetLength();
	unsigned int n = (unsigned int)((r * gf_PI2) / 5.0f);
	if(n < 5) n = 5;
	if(n > 50) n = 50;

	CDebugDrawContext dc;
	for (unsigned int i = 0; i < n; i++)
	{
		float	a = (float)i / (float)(n - 1) * gf_PI2;
		Vec3	p = world.TransformPoint(pos + Vec3(cosf(a) * radius, sinf(a) * radius, 0));
		if(i > 0)
			dc->DrawLine(screen.TransformPoint(last), color, screen.TransformPoint(p), color);
		last = p;
	}
}

void CAISystem::DrawRadarPath(CPuppet* puppet, const Matrix34& world, const Matrix34& screen)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	const char *pName=gAIEnv.CVars.DrawPath;
	if(!pName)
		return;
	CAIObject *pTargetObject = GetAIObjectByName(pName);
	if (pTargetObject)
	{
		CPuppet *pTargetPuppet = pTargetObject->CastToCPuppet();
		if (!pTargetPuppet)
			return;
		DebugDrawPathSingle(pTargetPuppet);
		return;
	}
	if(strcmp(pName, "all"))
		return;

	// draw the first part of the path in a different colour
	if (!puppet->m_OrigPath.GetPath().empty())	
	{
		TPathPoints::const_iterator li,linext;
		li = puppet->m_OrigPath.GetPath().begin();
		linext = li;
		++linext;
		// (MATT) BUG! Surely. m_Path or m_OrigPath, which is it? Appears elsewhere too. {2008/05/29}
		Vec3 endPt = puppet->m_Path.GetNextPathPos(ZERO);
		CDebugDrawContext dc;
		while (linext != puppet->m_OrigPath.GetPath().end())
		{
			Vec3 p0 = world.TransformPoint(li->vPos);
			Vec3 p1 = world.TransformPoint(linext->vPos);
			dc->DrawLine(screen.TransformPoint(p0), ColorB(255, 0, 255), screen.TransformPoint(p1), ColorB(255, 0, 255));
			li=linext++;
		}
	}
}

void CAISystem::DebugDrawRadar()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	int	size = gAIEnv.CVars.DrawRadar;
	if(size == 0)
		return;

	CDebugDrawContext dc;
	dc->Init2DMode();
	dc->SetAlphaBlended(true);
	dc->SetBackFaceCulling(false);

	int	w = dc->GetWidth();
	int	h = dc->GetHeight();

	int	centerx = w/2;
	int	centery = h/2;

	int	radarDist = gAIEnv.CVars.DrawRadarDist;
	float	worldSize = radarDist * 2.0f;

/*	ColorB	color(255,0,0,128);
	ColorB	color2(255,255,0,128);
	pAux->DrawLine(Vec3(0,0,0), color, Vec3(0.5f,0.5f,0), color);
	pAux->DrawLine(Vec3(1,1,0), color2, Vec3(0.5f,0.5f,0), color2);
	pAux->DrawLine(Vec3(0,0,0), color, Vec3(100,200,0), color);*/

	Matrix34	worldToScreen;
	worldToScreen.SetIdentity();
	CCamera& cam = GetISystem()->GetViewCamera();
	Vec3	camPos = cam.GetPosition();
	Vec3	camForward = cam.GetMatrix().TransformVector(FORWARD_DIRECTION);
	float	rot(atan2f(camForward.x, camForward.y));
	// inv camera
//	worldToScreen.SetRotationZ(-rot);
	worldToScreen.AddTranslation(-camPos);
	worldToScreen = Matrix34::CreateRotationZ(rot) * worldToScreen;
	// world to screenspace conversion
	float	s = (float)size/worldSize;
	worldToScreen = Matrix34::CreateScale(Vec3(s,s,0)) * worldToScreen;
	// offset the position to upper right corner
	worldToScreen.AddTranslation(Vec3(centerx, centery,0));
	// Inverse Y
	worldToScreen = Matrix34::CreateScale(Vec3(1,-1,0)) * worldToScreen;
	worldToScreen.AddTranslation(Vec3(0, h,0));

	Matrix34	screenToNorm;
	screenToNorm.SetIdentity();
	// normalize to 0..1
	screenToNorm = Matrix34::CreateScale(Vec3(1.0f/(float)w, 1.0f/(float)h, 1)) * screenToNorm;

	// Draw 15m circle.
	DrawRadarCircle(camPos, 0.5f, ColorB(255, 255, 255, 128), worldToScreen, screenToNorm);
	for (int i = 0; i < (radarDist / 5); i++)
	{
		ColorB	color(120, 120, 120, 64);
		if(i & 1) color.a = 128;
		DrawRadarCircle(camPos, (1 + i) * 5.0f, color, worldToScreen, screenToNorm);
	}

	ColorB	red(239, 50, 25);
	ColorB	yellow(255, 235, 15);
	ColorB	orange(255, 162, 0);
	ColorB	green(137, 226, 9);
	ColorB	blue(9, 121, 226);
	ColorB	white(255, 255, 255);

	// output all puppets
	AIObjectOwners::const_iterator ai;
	if ((ai = m_Objects.find(AIOBJECT_PUPPET)) != m_Objects.end())
	{
		for (;ai!=m_Objects.end(); ++ai)
		{
			if (ai->first != AIOBJECT_PUPPET)
				break;
			CAIObject* obj = ai->second.GetAIObject();
			CPuppet* puppet = obj->CastToCPuppet();
			if(!puppet)
				continue;

			if(Distance::Point_PointSq(obj->GetPos(), camPos) > sqr(radarDist * 1.25f))
				continue;

			float	rad(puppet->GetParameters().m_fPassRadius);

			Vec3	pos, forw, right;
			pos = worldToScreen.TransformPoint(obj->GetPos());
			forw = worldToScreen.TransformVector(obj->GetViewDir() * rad);
//			forw.NormalizeSafe();
			right.Set(-forw.y, forw.x, 0);

			ColorB	color(255,0,0,128);

			int alertness = puppet->GetProxy()->GetAlertnessState();
			if(alertness == 0)
				color = green;
			else if(alertness == 1)
				color = orange;
			else if(alertness == 2)
				color = red;
			color.a = 255;
			if(!puppet->IsEnabled())
				color.a = 64;

			const float	arrowSize = 1.5f;

			dc->DrawTriangle(screenToNorm.TransformVector(pos - forw*0.71f*arrowSize + right*0.71f*arrowSize), color,
				screenToNorm.TransformVector(pos - forw*0.71f*arrowSize - right*0.71f*arrowSize), color,
				screenToNorm.TransformVector(pos + forw*arrowSize), color);
			DrawRadarCircle(puppet->GetPos(), rad, ColorB(255,255,255,64), worldToScreen, screenToNorm);

			DrawRadarPath(puppet, worldToScreen, screenToNorm);

			if(puppet->GetState()->fire)
			{
				ColorB fireColor(255, 26, 0);
				if(!puppet->IsAllowedToHitTarget())
					fireColor.Set(255, 255, 255, 128);
				Vec3	tgt = worldToScreen.TransformPoint(puppet->GetState()->vShootTargetPos);
				dc->DrawLine(screenToNorm.TransformVector(pos), fireColor, screenToNorm.TransformVector(tgt), fireColor);
			}

			ColorB	white(255, 255, 255);
			ColorB	black(  0,   0,   0, 179);

			char	szMsg[256] = "\0";

			float	accuracy = 0.0f;
			if(puppet->GetFireTargetObject())
				accuracy = puppet->GetAccuracy(puppet->GetFireTargetObject());

			if(puppet->GetProxy()->GetCurrentReadibilityName())
				_snprintf(szMsg, 256, "%s\nAcc:%.3f\nSay:%s", puppet->GetName(), accuracy, puppet->GetProxy()->GetCurrentReadibilityName());
			else if(!puppet->IsAllowedToHitTarget())
				_snprintf(szMsg, 256, "%s\nAcc:%.3f\nAMBIENT", puppet->GetName(), accuracy);
			else
				_snprintf(szMsg, 256, "%s\nAcc:%.3f\n", puppet->GetName(), accuracy);

			dc->Draw2dLabel(pos.x+1, pos.y-1, 1.2f, black, true, "%s", szMsg);
			dc->Draw2dLabel(pos.x,   pos.y,   1.2f, white, true, "%s", szMsg);
		}
	}

/*	// output all vehicles
	if ((ai = m_Objects.find(AIOBJECT_VEHICLE)) != m_Objects.end())
	{
		for (;ai!=m_Objects.end(); ++ai)
		{
			if (ai->first != AIOBJECT_VEHICLE)
				break;
			float dist2 = (GetCameraPos() - (ai->second)->GetPos()).GetLengthSquared();
			if(dist2>drawDist2)
				continue;
			DebugDrawAgent(ai->second);
		}
	}*/
}

void CAISystem::DebugDrawDistanceLUT()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if(gAIEnv.CVars.DrawDistanceLUT < 1)
		return;
/*
	CDebugDrawContext dc;
	dc->Init2DMode();
	dc->SetAlphaBlended(true);
	dc->SetBackFaceCulling(false);

	int	w = dc->GetWidth();
	int	h = dc->GetHeight();

	Matrix34	screenToNorm;
	screenToNorm.SetIdentity();
	// normalize to 0..1
	screenToNorm = Matrix34::CreateScale(Vec3(1.0f/(float)w, 1.0f/(float)h, 1)) * screenToNorm;

	AILinearLUT&	lut(m_VisDistLookUp);
	if(lut.GetSize() < 2)
		return;

	const float	graphWidth = 400.0f;
	const float	graphHeight = 200.0f;

	float	sx = w/2 - graphWidth/2;
	float	sy = 50 + graphHeight;

	{
		Vec3	v0, v1;

		v0.Set(sx, sy, 0);
		v1.Set(sx+graphWidth, sy, 0);
		dc->DrawLine(screenToNorm.TransformVector(v0), ColorB(255,255,255,128),
			screenToNorm.TransformVector(v1), ColorB(255,255,255,128));

		v0.Set(sx, sy-graphHeight/2, 0);
		v1.Set(sx+graphWidth, sy-graphHeight/2, 0);
		dc->DrawLine(screenToNorm.TransformVector(v0), ColorB(255,255,255,64),
			screenToNorm.TransformVector(v1), ColorB(255,255,255,64));

		v0.Set(sx, sy-graphHeight, 0);
		v1.Set(sx+graphWidth, sy-graphHeight, 0);
		dc->DrawLine(screenToNorm.TransformVector(v0), ColorB(255,255,255,128),
			screenToNorm.TransformVector(v1), ColorB(255,255,255,128));
	}

	float	scx = graphWidth / (lut.GetSize() - 1);
	float	scy = graphHeight / 100.0f;
	for(int i = 0; i < lut.GetSize(); ++i)
	{
		Vec3	v0(sx + i*scx, sy - lut.GetSampleValue(i)*scy, 0);
		if(i+1 < lut.GetSize())
		{
			Vec3	v1(sx + (i+1)*scx, sy - lut.GetSampleValue(i+1)*scy, 0);
			Vec3	v2(sx + i*scx, sy, 0);
			dc->DrawLine(screenToNorm.TransformVector(v0), ColorB(255,255,255),
				screenToNorm.TransformVector(v1), ColorB(255,255,255));
			dc->DrawLine(screenToNorm.TransformVector(v0), ColorB(255,255,255,32),
				screenToNorm.TransformVector(v2), ColorB(255,255,255,32));
		}


		dc->Draw2dLabel(v0.x, v0.y - 10, 1.2f, ColorB(255, 255, 255), true, "%.1f", lut.GetSampleValue(i));
	}

	// Draw the current value
	const char*	statsTargetName = gAIEnv.CVars.StatsTarget;
	if(!statsTargetName)
		return;
	CAIObject *pTargetObject = GetAIObjectByName(statsTargetName);
	if (!pTargetObject)
		return;
	CPuppet *pTargetPuppet = pTargetObject->CastToCPuppet();
	if (!pTargetPuppet)
		return;

	float fMaxPriority = -1.f;
	EventData maxEvent;
	CAIObject *pNextTarget = 0;
	PotentialTargetMap::iterator ei = pTargetPuppet->m_perceptionEvents.begin(), eiend = pTargetPuppet->m_perceptionEvents.end();
	for (; ei != eiend; ++ei)
	{
		const SAIPotentialTarget &ed = ei->second;
		// target selection based on priority
		if (ed.fPriority>fMaxPriority)
		{
			fMaxPriority = ed.fPriority;
			maxEvent = ed;
			if (ed.pDummyRepresentation)
				pNextTarget = ed.pDummyRepresentation;
			else
				pNextTarget = ei->first;
		}
	}

	if(pNextTarget)
	{
		float	d = Distance::Point_Point(pNextTarget->GetPos(), pTargetPuppet->GetPos());
		d = clamp(d / pTargetPuppet->GetParameters().m_PerceptionParams.sightRange, 0.0f, 1.0f);

		{
			Vec3	v0(sx + d*graphWidth, sy - (lut.GetValue(d) + 5)*scy, 0);
			Vec3	v1(sx + d*graphWidth, sy, 0);

			dc->DrawLine(screenToNorm.TransformVector(v0), ColorB(255,0,0),
				screenToNorm.TransformVector(v1), ColorB(255,0,0));

			dc->Draw2dLabel(v0.x, v0.y - 15, 1.5f, ColorB(255, 0, 0), true, "%.1f", lut.GetValue(d));
		}
	}
*/
}


struct SSlopeTriangle
{
	SSlopeTriangle(const Triangle& tri, float slope) : triangle(tri), slope(slope) {}
	SSlopeTriangle() {}
	Triangle triangle;
	float slope;
};

//====================================================================
// DebugDrawSteepSlopes
// Caches triangles first time this is enabled - then on subsequent
// calls just draws. Gets reset when the debug draw value changes
//====================================================================
void CAISystem::DebugDrawSteepSlopes()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	static std::vector<SSlopeTriangle> triangles;
	/// Current centre for the region we draw
	static Vec3 currentPos(0.0f, 0.0f, 0.0f);

	// Size of the area to calculate/use
	const float drawBoxWidth = 200.0f;
	// update when current position is greater than this (horizontally) 
	// distance from lastPos
	const float drawUpdateDist = drawBoxWidth * 0.25f;
	const float zOffset = 0.05f;

	CDebugDrawContext dc;
	
	Vec3 playerPos = dc->GetCameraPos();
	playerPos.z = 0.0f;

	if ( (playerPos - currentPos).GetLength() > drawUpdateDist )
	{
		currentPos = playerPos;
		triangles.resize(0); // force a refresh
	}
	if (gAIEnv.CVars.DebugDraw != 85)
	{
		triangles.resize(0);
		return;
	}

	if (gEnv->bEditor)
	{
		triangles.resize(0);
	}

	float criticalSlopeUp = gAIEnv.CVars.SteepSlopeUpValue;
	float criticalSlopeAcross = gAIEnv.CVars.SteepSlopeAcrossValue;

	if (triangles.empty())
	{
		I3DEngine *pEngine = gEnv->p3DEngine;

		int terrainArraySize = pEngine->GetTerrainSize();
		float dx = pEngine->GetHeightMapUnitSize();

		float minX = currentPos.x - 0.5f * drawBoxWidth;
		float minY = currentPos.y - 0.5f * drawBoxWidth;
		float maxX = currentPos.x + 0.5f * drawBoxWidth;
		float maxY = currentPos.y + 0.5f * drawBoxWidth;
		int minIx = (int) (minX / dx);
		int minIy = (int) (minY / dx);
		int maxIx = (int) (maxX / dx);
		int maxIy = (int) (maxY / dx);
		Limit(minIx, 1, terrainArraySize-1);
		Limit(minIy, 1, terrainArraySize-1);
		Limit(maxIx, 1, terrainArraySize-1);
		Limit(maxIy, 1, terrainArraySize-1);

		// indices start at +1 so we can use the previous indices
		for (int ix = minIx ; ix < maxIx ; ++ix)
		{
			for (int iy = minIy ; iy < maxIy ; ++iy)
			{
				Vec3 v11(dx * ix, dx * iy, 0.0f);
				Vec3 v01(dx * (ix-1), dx * iy, 0.0f);
				Vec3 v10(dx * ix, dx * (iy-1), 0.0f);
				Vec3 v00(dx * (ix-1), dx * (iy-1), 0.0f);

				v11.z = zOffset + dc->GetDebugDrawZ(v11, true);
				v01.z = zOffset + dc->GetDebugDrawZ(v01, true);
				v10.z = zOffset + dc->GetDebugDrawZ(v10, true);
				v00.z = zOffset + dc->GetDebugDrawZ(v00, true);

				Vec3 n1 = ((v10 - v00) % (v01 - v00)).GetNormalized();
				float slope1 = fabs(sqrtf(1.0f - n1.z * n1.z) / n1.z);
				if (slope1 > criticalSlopeUp)
					triangles.push_back(SSlopeTriangle(Triangle(v00, v10, v01), slope1));
				else if (slope1 > criticalSlopeAcross)
					triangles.push_back(SSlopeTriangle(Triangle(v00, v10, v01), -slope1));

				Vec3 n2 = ((v11 - v10) % (v01 - v10)).GetNormalized();
				float slope2 = fabs(sqrtf(1.0f - n2.z * n2.z) / n2.z);
				if (slope2 > criticalSlopeUp)
					triangles.push_back(SSlopeTriangle(Triangle(v10, v11, v01), slope2));
				else if (slope2 > criticalSlopeAcross)
					triangles.push_back(SSlopeTriangle(Triangle(v10, v11, v01), -slope2));
			}
		}
	}

	unsigned int numTris = triangles.size();
	for (unsigned int i = 0 ; i < numTris ; ++i)
	{
		const SSlopeTriangle& tri = triangles[i];
		ColorF color;
		// convert slope into 0-1 (the z-value of a unit vector having that slope)
		float slope = tri.slope > 0.0f ? tri.slope : -tri.slope;
		float a = sqrtf(slope * slope / (1.0f + slope * slope));
		if (tri.slope > 0.0f)
			color.Set(1.0f - 0.5f * a, 0.0f, 0.0f);
		else
			color.Set(0.0f, 1.0f - 0.5f * a, 1.0f);
		dc->DrawTriangle(tri.triangle.v0, color, tri.triangle.v1, color, tri.triangle.v2, color);
	}
}

//===================================================================
// DebugDrawVegetationCollision
//===================================================================
void CAISystem::DebugDrawVegetationCollision()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

  float range = gAIEnv.CVars.DebugDrawVegetationCollisionDist;
  if (range < 0.1f)
    return;

	I3DEngine *pEngine = gEnv->p3DEngine;

	CDebugDrawContext dc;

  Vec3 playerPos = dc->GetCameraPos();
	playerPos.z = pEngine->GetTerrainElevation(playerPos.x, playerPos.y);

  AABB aabb(AABB::RESET);
  aabb.Add(playerPos + Vec3(range, range, range));
  aabb.Add(playerPos - Vec3(range, range, range));

  ColorB triCol(128, 255, 255);
  const float zOffset = 0.05f;

	IPhysicalEntity** pObstacles;
	int count = gAIEnv.pWorld->GetEntitiesInBox(aabb.min, aabb.max, pObstacles, ent_static|ent_ignore_noncolliding);
	for (int i = 0 ; i < count ; ++i)
	{
    IPhysicalEntity *pPhysics = pObstacles[i];

    pe_status_pos status;
    status.ipart = 0;
    pPhysics->GetStatus( &status );

		Vec3 obstPos = status.pos;
		Matrix33 obstMat = Matrix33(status.q);
		obstMat *= status.scale;

		IGeometry* geom = status.pGeom;
		int type = geom->GetType();
		if (type == GEOM_TRIMESH)
		{
			const primitives::primitive * prim = geom->GetData();
			const mesh_data * mesh = static_cast<const mesh_data *>(prim);

			int numVerts = mesh->nVertices;
			int numTris = mesh->nTris;

    	static std::vector<Vec3>	vertices;
			vertices.resize( numVerts );

			for( int j = 0; j < numVerts; j++ )
				vertices[j] = obstPos + obstMat * mesh->pVertices[j];

			for (int j = 0; j < numTris; ++j )
			{
				int vidx0 = mesh->pIndices[j*3 + 0];
				int vidx1 = mesh->pIndices[j*3 + 1];
				int vidx2 = mesh->pIndices[j*3 + 2];

        Vec3 pt0 = vertices[vidx0];
        Vec3 pt1 = vertices[vidx1];
        Vec3 pt2 = vertices[vidx2];

        float z0 = pEngine->GetTerrainElevation(pt0.x, pt0.y);
        float z1 = pEngine->GetTerrainElevation(pt1.x, pt1.y);
        float z2 = pEngine->GetTerrainElevation(pt2.x, pt2.y);

        const float criticalAlt = 1.8f;
        if (pt0.z < z0 && pt1.z < z1 && pt2.z < z2)
          continue;
        if (pt0.z > z0 + criticalAlt && pt1.z > z1 + criticalAlt && pt2.z > z2 + criticalAlt)
          continue;
        pt0.z = z0 + zOffset;
        pt1.z = z1 + zOffset;
        pt2.z = z2 + zOffset;

			  dc->DrawTriangle(pt0, triCol, pt1, triCol, pt2, triCol);
      }
    }
  }
}

struct SHidePos
{
  SHidePos(const Vec3 &pos = ZERO, const Vec3 &dir = ZERO) : pos(pos), dir(dir) {}
  Vec3 pos;
  Vec3 dir;
};

struct SVolumeFunctor
{
  SVolumeFunctor(std::vector<SHidePos> &hidePositions) : hidePositions(hidePositions) {}
  void operator()(SVolumeHideSpot& hs, float) {hidePositions.push_back(SHidePos(hs.pos, hs.dir));}
  std::vector<SHidePos> &hidePositions;
};

//====================================================================
// DebugDrawHideSpots
// need to evaluate all navigation/hide types
//====================================================================
void CAISystem::DebugDrawHideSpots()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

  float range = gAIEnv.CVars.DebugDrawHideSpotRange;
  if (range < 0.1f)
    return;
  
  CDebugDrawContext dc;
  
  Vec3 playerPos = dc->GetCameraPos();

  Vec3 groundPos = playerPos;
  I3DEngine *pEngine = gEnv->p3DEngine;
  groundPos.z = pEngine->GetTerrainElevation(groundPos.x, groundPos.y);
  dc->DrawSphere(groundPos, 0.01f * (playerPos.z - groundPos.z), ColorB(255, 0, 0));

  MultimapRangeHideSpots hidespots;
  MapConstNodesDistance traversedNodes;
  // can't get SO since we have no entity...
  GetHideSpotsInRange(hidespots, traversedNodes, playerPos, range, 
    IAISystem::NAV_TRIANGULAR | IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE | 
    IAISystem::NAV_VOLUME | IAISystem::NAV_SMARTOBJECT | IAISystem::NAV_LAYERED_NAV_MESH, 0.0f, false, 0);

  /// for profiling (without draw overhead)
  const bool skipDrawing = false;
  if (skipDrawing)
    return;

  for (MultimapRangeHideSpots::const_iterator it = hidespots.begin() ; it != hidespots.end() ; ++it)
  {
    float distance = it->first;
    const SHideSpot &hs = it->second;

    const Vec3 &pos = hs.info.pos;
    Vec3 dir = hs.info.dir;

    const float radius = 0.3f;
    const float height = 0.3f;
    const float triHeight = 5.0f;
    int alpha = 255;
    if (hs.pObstacle && !hs.pObstacle->IsCollidable())
    {
      alpha = 128;
    }
    else if (hs.pAnchorObject && hs.pAnchorObject->GetType() == AIANCHOR_COMBAT_HIDESPOT_SECONDARY)
    {
      alpha = 128;
      dir.zero();
    }
    dc->DrawSphere(pos, 0.2f * radius, ColorB(0, 255, 0, alpha));
    if (dir.IsZero())
      dc->DrawCone(pos + Vec3(0, 0, triHeight), Vec3(0, 0, -1), radius, triHeight, ColorB(0, 255, 0, alpha));
    else
      dc->DrawCone(pos + height * dir, -dir, radius, height, ColorB(0, 255, 0, alpha));

  }

	// INTEGRATION : (MATT) These yellow cones kindof get in the way in G04 but handy for Crysis I believe {2007/05/31:17:29:30}
  ColorB color(255, 255, 0);
  for (MapConstNodesDistance::const_iterator it = traversedNodes.begin() ; it != traversedNodes.end() ; ++it)
  {
    const GraphNode *pNode = it->first;
    float dist = it->second;
    Vec3 pos = pNode->GetPos();
    float radius = 0.3f;
    float coneHeight = 1.0f;
    dc->DrawCone(pos + Vec3(0, 0, coneHeight), Vec3(0, 0, -1), radius, coneHeight, ColorB(255, 255, 0, 100));
    dc->Draw3dLabelEx(pos + Vec3(0, 0, 0.3f), 1.0f, color, true, false, "%5.2f", dist);
  }
}

//====================================================================
// DebugDrawHashSpace
//====================================================================
void CAISystem::DebugDrawHashSpace()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	static bool validate = true;

	const char* szEntityName = gAIEnv.CVars.DebugDrawHashSpaceAround;
	if (!szEntityName)
	{
		validate = true;
		return;
	}
	int len = strlen(szEntityName);
	if (len == 0 || (szEntityName[0] == '0' && len == 1))
	{
		validate = true;
		return;
	}

	// Validate the hash space
	if (validate)
	{
		m_pGraph->ValidateHashSpace();
		validate = false;
	}

	CDebugDrawContext dc;

	const CAllNodesContainer& allNodes = m_pGraph->GetAllNodes();
	const CHashSpace<CAllNodesContainer::SGraphNodeRecord, CAllNodesContainer::GraphNodeHashSpaceTraits>& hashSpace = allNodes.GetHashSpace();

	// Collect some stats.
	unsigned int maxObjectsPerBucket = 0;
	unsigned int numFilledBuckets = 0;
	float avgObjectsPerBucket = 0;
	for (unsigned int i = 0, ni = hashSpace.GetBucketCount(); i < ni; ++i)
	{
		unsigned int objs = hashSpace.GetObjectCountInBucket(i);
		maxObjectsPerBucket = max(maxObjectsPerBucket, objs);
		if (objs)
			numFilledBuckets++;
		avgObjectsPerBucket += objs;
	}
	avgObjectsPerBucket /= (float)hashSpace.GetBucketCount();
	float maxObjScale = !maxObjectsPerBucket ? 1.0f : 1.0f/(float)maxObjectsPerBucket;

	// Draw the hash buckets around the player.
	Vec3 pos(0,0,0);
	IEntity* pEntity = gEnv->pEntitySystem->FindEntityByName(szEntityName);
	if (pEntity)
	{
		pos = pEntity->GetWorldPos();
		int i, j, k;
		hashSpace.GetIJKFromPosition(pos, i, j, k);
		AABB cellBounds;
		for (int z = -1; z <= 1; ++z)
		{
			for (int y = -2; y <= 2; ++y)
			{
				for (int x = -2; x <= 2; ++x)
				{
					unsigned int bucket = hashSpace.GetHashBucketIndex(i+x, j+y, k+z);
					hashSpace.GetAABBFromIJK(i+x, j+y, k+z, cellBounds);
					unsigned int objects = hashSpace.GetObjectCountInBucket(bucket);

					// 'Fullness'
					Vec3 center = cellBounds.GetCenter();
					Vec3 size = cellBounds.GetSize() * (objects*maxObjScale*0.5f);

					if (x == 0 && y == 0 && z == 0)
					{
						// Check how many points in the bucket belongs to his location
						unsigned int local = 0;
						float localPercent = 0;
						for (unsigned int m = 0; m < objects; ++m)
						{
							const CAllNodesContainer::SGraphNodeRecord& nodeRec = hashSpace.GetObjectInBucket(m, bucket);
							Vec3 nodePos = nodeRec.GetPos(gAIEnv.pGraph->GetNodeManager());
							int ii, jj, kk;
							hashSpace.GetIJKFromPosition(nodePos, ii, jj, kk);
							if (ii == (i+x) && jj == (j+y) && kk == (k+z))
							{
								dc->DrawSphere(nodePos, 0.05f, ColorB(255,64,0));
								local++;
							}
						}
						if (objects)
							localPercent = (float)local/(float)objects;

						// Grid
						dc->DrawAABB(cellBounds, false, ColorB(255,255,255,64), eBBD_Faceted);
						dc->Draw3dLabel(center, 1.5f, "Bucket: %d\nObjects: %d\nLocal: %d (%.3f)", bucket, objects, local, localPercent);
						dc->DrawAABB(AABB(center-size, center+size), true, ColorB(0,196,255), eBBD_Faceted);
					}
					else
					{
						dc->Draw3dLabel(center, 1.0f, "%d", objects);
						dc->DrawAABB(AABB(center-size, center+size), true, ColorB(0,196,255,128), eBBD_Faceted);
					}
				}
			}
		}
	}

	// Test GetAllNodes performance.
	const unsigned int testCount = 100;
	static Vec3 testPoints[testCount];
	const float testRange = 20.0f;	// common range for hidepoint and get enclosing query.
	static bool testPointsInitialized = false;

	if (!testPointsInitialized)
	{
		for (unsigned int i = 0; i < testCount; ++i)
		{
			float a = ai_frand() * gf_PI2;
			float d = ((1+(ai_rand() % 20)) / 20.0f) * 15.0f;
			testPoints[i].Set(cosf(a)*d, sinf(a)*d, 0);
		}
		testPointsInitialized = true;
	}

	// Test get all nodes
	CTimeValue startTime = gEnv->pTimer->GetAsyncTime();
	static std::vector< std::pair<float, unsigned int> > testNodes;
	for (unsigned int i = 0; i < testCount; ++i)
	{
		testNodes.clear();
		allNodes.GetAllNodesWithinRange(testNodes, pos+testPoints[i], testRange, IAISystem::NAVMASK_ALL);
	}
	float testElapsedMs = (gEnv->pTimer->GetAsyncTime() - startTime).GetMilliSeconds();


	// Check the node distribution.
	dc->Init2DMode();
	dc->SetAlphaBlended(true);
	dc->SetBackFaceCulling(false);

	const Vec3	u(1,0,0);
	const Vec3	v(0,-1,0);
	const Vec3	w(0,0,1);

	float	sizex = 0.9f;
	float	sizey = 0.2f;

	Vec3	orig = Vec3(0.05f, 1-0.05f, 0);

	// dim BG
/*	ColorB	bg1(0,0,0,160); ColorB	bg2(128,128,128,128);
	dc->DrawTriangle(orig, bg1, orig+u*sizex, bg1, orig+u*sizex+v*sizey, bg2);
	dc->DrawTriangle(orig, bg1, orig+u*sizex+v*sizey, bg2, orig+v*sizey, bg2);*/

	float scale = maxObjectsPerBucket ? 1.0f/(float)maxObjectsPerBucket : 1.0f;

	static std::vector<unsigned int> values;
	static std::vector<Vec3> points;
	unsigned int valueCount = hashSpace.GetBucketCount();
	unsigned int shift = 0;
	while ((valueCount >> shift) > 1024)
		shift++;
	values.resize(hashSpace.GetBucketCount() >> shift);
	points.resize(hashSpace.GetBucketCount() >> shift);
	for (unsigned int i = 0, ni = values.size(); i < ni; ++i)
		values[i] = 0;

	for (unsigned int i = 0, ni = hashSpace.GetBucketCount(); i < ni; ++i)
	{
		unsigned int j = i >> shift;
		values[j] = max(values[j], hashSpace.GetObjectCountInBucket(i));
	}

	for (unsigned int i = 0, ni = values.size(); i < ni; ++i)
	{
		float a = (float)i / (float)values.size();
		points[i] = orig + u*sizex*a + v*sizey*values[i]*scale;
	}

	// Draw value lines
	dc->DrawLine(orig, ColorB(0,0,0,128), orig + u*sizex, ColorB(0,0,0,128));
	dc->DrawLine(orig + v*sizey*1.0f, ColorB(0,0,0,128), orig + u*sizex + v*sizey*1.0f, ColorB(0,0,0,128));

	dc->DrawPolyline(&points[0], points.size(), false, ColorB(255,255,255,160));

	// Stats
	int sw = dc->GetWidth();
	int sh = dc->GetHeight();

	ColorB white(255, 255, 255);

	float y = (orig.y-sizey)*sh-90.0f;
	dc->Draw2dLabel(orig.x*sw+3, y, 1.4f, white, false, "Nodes: %d", hashSpace.GetNumObjects());
	y += 20.0f;
	dc->Draw2dLabel(orig.x*sw+3, y, 1.2f, white, false, "Buckets: %d/%d  Usage: %.2f  Mem: %d kB", numFilledBuckets, hashSpace.GetBucketCount(), (float)numFilledBuckets/(float)hashSpace.GetBucketCount()*100.0f, (hashSpace.MemStats()+1023)/1024);
	y += 20.0f;
	dc->Draw2dLabel(orig.x*sw+3, y, 1.2f, white, false, "Max/Bucket: %d", maxObjectsPerBucket);
	y += 20.0f;
	dc->Draw2dLabel(orig.x*sw+3, y, 1.2f, white, false, "Avg/Bucket: %.2f", avgObjectsPerBucket);
	y += 20.0f;

	// Print out the test result
	dc->Draw2dLabel(orig.x*sw+3, (orig.y-sizey)*sh-150.0f, 1.5f, white, false, "Test GetAllNodes(%d) @ (%.1f, %.1f, %.1f): %.2fms", testCount, pos.x, pos.y, pos.z, testElapsedMs);
}

//====================================================================
// DebugDrawDynamicHideObjects
//====================================================================
void CAISystem::DebugDrawDynamicHideObjects()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	float range = gAIEnv.CVars.DebugDrawDynamicHideObjectsRange;
	if (range < 0.1f)
		return;

	Vec3 cameraPos = GetISystem()->GetViewCamera().GetPosition();
	Vec3 cameraDir = GetISystem()->GetViewCamera().GetViewdir();

	Vec3 pos = cameraPos + cameraDir * (range/2);
	Vec3 size(range/2, range/2, range/2);

	SEntityProximityQuery query;
	query.box.min = pos - size;
	query.box.max = pos + size;
	query.nEntityFlags = (uint32)ENTITY_FLAG_AI_HIDEABLE; // Filter by entity flag.

	CDebugDrawContext dc;
	gEnv->pEntitySystem->QueryProximity(query);
	for(int i = 0; i < query.nCount; ++i)
	{
		IEntity* pEntity = query.pEntities[i];
		if (!pEntity) continue;

		AABB bbox;
		pEntity->GetLocalBounds(bbox);
		dc->DrawAABB(bbox, pEntity->GetWorldTM(), true, ColorB(255, 0, 0, 128), eBBD_Faceted);
	}

	m_dynHideObjectManager.DebugDraw();
}

//====================================================================
// DebugDrawGraphErrors
//====================================================================
void CAISystem::DebugDrawGraphErrors(CGraph* pGraph) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CDebugDrawContext dc;
	
	unsigned int numErrors = pGraph->mBadGraphData.size();

	std::vector<Vec3> positions;
	const float minRadius = 0.1f;
	const float maxRadius = 2.0f;
	const int numCircles = 2;
	for (unsigned int iError = 0 ; iError < numErrors ; ++iError)
	{
		const CGraph::SBadGraphData & error = pGraph->mBadGraphData[iError];
		Vec3 errorPos1 = error.mPos1;
		errorPos1.z = dc->GetDebugDrawZ(errorPos1, true);
		Vec3 errorPos2 = error.mPos2;
		errorPos2.z = dc->GetDebugDrawZ(errorPos2, true);

		positions.push_back(errorPos1);

		switch (error.mType)
		{
		case CGraph::SBadGraphData::BAD_PASSABLE:
			dc->DrawCircles(errorPos1, minRadius, maxRadius, numCircles, Vec3(1, 0, 0), Vec3(1, 1, 0));
			dc->DrawCircles(errorPos2, minRadius, maxRadius, numCircles, Vec3(1, 0, 0), Vec3(1, 1, 0));
			dc->DrawLine(errorPos1, ColorB(255, 255, 0), errorPos2, ColorB(255, 255, 0));
			break;
		case CGraph::SBadGraphData::BAD_IMPASSABLE:
			dc->DrawCircles(errorPos1, minRadius, maxRadius, numCircles, Vec3(0, 1, 0), Vec3(1, 1, 0));
			dc->DrawCircles(errorPos2, minRadius, maxRadius, numCircles, Vec3(0, 1, 0), Vec3(1, 1, 0));
			dc->DrawLine(errorPos1, ColorB(255, 255, 0), errorPos2, ColorB(255, 255, 0));
			break;
		}
	}
	const float errorRadius = 10.0f;
	// draw the basic graph just in this region
	DebugDrawGraph(m_pGraph, &positions, errorRadius);
}

//====================================================================
// CheckDistance
//====================================================================
static inline bool CheckDistance(const Vec3 pos1, const std::vector<Vec3> * focusPositions, float radius)
{
	if (!focusPositions)
		return true;
	const std::vector<Vec3> & positions = *focusPositions;

	for (unsigned int i = 0 ; i < positions.size() ; ++i)
	{
		Vec3 delta = pos1 - positions[i];
		delta.z = 0.0f;
		if (delta.GetLengthSquared() < radius*radius)
			return true;
	}
	return false;
}

// A bit ugly, but make this global - it caches the debug graph that needs to be drawn.
// If it's empty then DebugDrawGraph tries to fill it. Otherwise we just draw it
// It will get zeroed when the graph is regenerated.
std::vector<const GraphNode *> g_DebugGraphNodesToDraw;
static CGraph* lastDrawnGraph = 0; // detects swapping between hide and nav

//====================================================================
// DebugDrawGraph
//====================================================================
void CAISystem::DebugDrawGraph(CGraph* pGraph, const std::vector<Vec3> * focusPositions, float focusRadius) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

//	if (pGraph != lastDrawnGraph)
	{
		lastDrawnGraph = pGraph;
		g_DebugGraphNodesToDraw.clear();
	}

	if (g_DebugGraphNodesToDraw.empty())
	{
		// NOTE Oct 27, 2009: <pvl> disabling these checks while implementing
		// demand-loading LNM's.  'm_pSafeFirst' can become isolated after an
		// unload and it's not clear to me what the right fix should be.  Moreover,
		// as I don't see any substantial usage of 'm_pSafeFirst' anywhere it could
		// be that the right fix would be to throw it away?
//		if (!pGraph->m_pSafeFirst)
//			return;

//		if (!pGraph->m_pSafeFirst->firstLinkIndex)
//			return;

		CAllNodesContainer& allNodes = pGraph->GetAllNodes();
		CAllNodesContainer::Iterator it(allNodes, NAV_TRIANGULAR | NAV_WAYPOINT_HUMAN | NAV_WAYPOINT_3DSURFACE | NAV_VOLUME | NAV_ROAD | NAV_SMARTOBJECT | NAV_LAYERED_NAV_MESH | NAV_CUSTOM_NAVIGATION);
		while (unsigned currentNodeIndex = it.Increment())
		{
			GraphNode* pCurrent = pGraph->GetNodeManager().GetNode(currentNodeIndex);

			g_DebugGraphNodesToDraw.push_back(pCurrent);
		}
	}
	// now just render
	const bool renderPassRadii = false;
	const float ballRad = 0.04f;

	CDebugDrawContext dc;

	unsigned int nNodes = g_DebugGraphNodesToDraw.size();
	for (unsigned int iNode = 0 ; iNode < nNodes ; ++iNode)
	{
		const GraphNode * node = g_DebugGraphNodesToDraw[iNode];
		AIAssert(node);
		
		ColorF color(0.f, 0.f, 0.f);
		Vec3 pos = node->GetPos();
		pos.z = dc->GetDebugDrawZ(pos, node->navType == IAISystem::NAV_TRIANGULAR);

		switch (node->navType)
		{
		case IAISystem::NAV_TRIANGULAR:
			{
				// draw the graph node itself in white if outside, black if in
				if (node->GetTriangularNavData()->isForbidden || node->GetTriangularNavData()->isForbiddenDesigner)
				{
					color.r = node->GetTriangularNavData()->isForbidden ? 0.5f : 0;
					color.g = node->GetTriangularNavData()->isForbiddenDesigner ? 0.3f : 0;
				}
				else
					color.Set(1.f, 1.f, 1.f);

				if (CheckDistance(pos, focusPositions, focusRadius))
				{
					dc->DrawSphere(pos, ballRad, color);

					unsigned int numVertices = node->GetTriangularNavData()->vertices.size();
					// connect and draw the vertices in blue
					color.Set(0, 0, 255);
					for (unsigned int iV = 0 ; iV < numVertices ; ++iV)
					{
						unsigned int iVNext = (iV + 1) % numVertices;
						Vec3 v0 = ((CVertexList&)m_VertexList).GetVertex(node->GetTriangularNavData()->vertices[iV]).vPos;
						Vec3 v1 = ((CVertexList&)m_VertexList).GetVertex(node->GetTriangularNavData()->vertices[iVNext]).vPos;
						v0.z = dc->GetDebugDrawZ(v0, true);
						v1.z = dc->GetDebugDrawZ(v1, true);
						dc->DrawLine(v0, ColorB(0, 128, 0), v1, ColorB(0, 128, 0));
						dc->DrawSphere(v0, ballRad, color);
					}
				}
			}
			break;

		case IAISystem::NAV_ROAD:
			{
				color.Set(255, 255, 255);
				dc->DrawSphere(pos, ballRad*3, color);
			}
			break;

		case IAISystem::NAV_LAYERED_NAV_MESH:
			{
				color.Set(255,255,255);
				dc->DrawSphere(pos + Vec3 (0.0f,0.0f,0.5f), ballRad*3.0f, color);
				dc->Draw3dLabelEx(pos + Vec3(0.0f,0.0f,0.7f), 2.0f, ColorB(255, 0, 0), true, false,"%x", node->GetLayeredMeshNavData()->polygonIndex);
			}
			break;

		case IAISystem::NAV_CUSTOM_NAVIGATION:
			{
				color.Set(255,255,255);
				dc->DrawSphere(pos + Vec3 (0.0f,0.0f,0.5f), ballRad*3.0f, color);
			}
			break;
		
		// Cover all other cases to provide a real view of the graph
		default:
			{
				color.Set(255,0,255);
				dc->Draw3dLabelEx(pos + Vec3(0.0f,0.0f,0.7f), 2.0f, ColorB(255, 255, 0), true, false, "T:%x", node->navType);
				dc->DrawSphere(pos + Vec3 (0.0f,0.0f,0.5f), ballRad*4.0f, color);
			}
			break;
		}

		for (unsigned link = node->firstLinkIndex; link; link = pGraph->GetLinkManager().GetNextLink(link))
		{
			unsigned int nextIndex = pGraph->GetLinkManager().GetNextNode(link);
			const GraphNode * next = pGraph->GetNodeManager().GetNode(nextIndex);
			AIAssert(next);

			ColorB endColor;
			if (pGraph->GetLinkManager().GetRadius(link) > 0.0f)
				endColor = ColorB(255, 255, 255);
			else
				endColor = ColorB(0, 0, 0);

			if (CheckDistance(pos, focusPositions, focusRadius))
			{
				Vec3 v0 = node->GetPos();
				Vec3 v1 = next->GetPos();
				v0.z = dc->GetDebugDrawZ(v0, node->navType == IAISystem::NAV_TRIANGULAR);
				v1.z = dc->GetDebugDrawZ(v1, node->navType == IAISystem::NAV_TRIANGULAR);
				if (pGraph->GetLinkManager().GetStartIndex(link) != pGraph->GetLinkManager().GetEndIndex(link))
				{
					Vec3 mid = pGraph->GetLinkManager().GetEdgeCenter(link);
					mid.z = dc->GetDebugDrawZ(mid, node->navType == IAISystem::NAV_TRIANGULAR);
					dc->DrawLine(v0, endColor, mid, ColorB(0, 255, 255));
					if (renderPassRadii) 
						dc->Draw3dLabel(mid,1,"%.2f",pGraph->GetLinkManager().GetRadius(link));

					int debugDrawVal = gAIEnv.CVars.DebugDraw;
					if (debugDrawVal == 179)
						dc->Draw3dLabel(mid,2,"%x",pGraph->LinkId (link));
					else if (debugDrawVal == 279)
					{
						float waterDepth = pGraph->GetLinkManager().GetMaxWaterDepth(link);
						if (waterDepth > 0.6f)
						{
							dc->Draw3dLabelEx(mid, 1, ColorB(255, 0, 0), true, false, "%.2f", waterDepth);
						}
						else
						{
							dc->Draw3dLabelEx(mid, 1, ColorB(255, 255, 255), true, false, "%.2f", waterDepth);
						}
					}
				}
				else
				{
					dc->DrawLine(v0 + Vec3 (0,0,0.5), endColor, v1 + Vec3 (0,0,0.5), endColor);
					if (renderPassRadii)
					{
						dc->Draw3dLabel(0.5f * (v0 + v1),1, "%.2f", pGraph->GetLinkManager().GetRadius(link));
					}
				}
			} // range check
		}
	}

  m_pNavigation->DebugDrawForbidden();
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawPath()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	const char *pName=gAIEnv.CVars.DrawPath;
	if(!pName )
		return;
	CAIObject *pTargetObject = GetAIObjectByName(pName);
	if (pTargetObject)
	{
		CPuppet* pTargetPuppet = pTargetObject->CastToCPuppet();
		if (!pTargetPuppet)
			return;
		DebugDrawPathSingle(pTargetPuppet);
		return;
	}
	if(strcmp(pName, "all"))
		return;

  m_pNavigation->GetTriangularNavRegion()->DebugDrawPathLinks();

	// find if there are any puppets
	const Vec3 offset(0, 0, 0.1f);
	int cnt=0;
	AIObjectOwners::const_iterator ai;
	if ((ai = m_Objects.find(AIOBJECT_PUPPET)) != m_Objects.end())
	{
		for (;ai!=m_Objects.end(); ++ai)
		{
			if (ai->first != AIOBJECT_PUPPET)
				break;
			cnt++;
			CPuppet *pPuppet = (CPuppet*) ai->second.GetAIObject();
			DebugDrawPathSingle(pPuppet);
		}
	}

	if ((ai = m_Objects.find(AIOBJECT_VEHICLE)) != m_Objects.end())
	{
		for (;ai!=m_Objects.end(); ++ai)
		{
			if (ai->first != AIOBJECT_VEHICLE)
				break;
			cnt++;
			if(!ai->second)
				continue;
			CPuppet *pTargetPuppet = ai->second->CastToCPuppet();
			if (!pTargetPuppet)
				continue;
			DebugDrawPathSingle(pTargetPuppet);
		}
	}
}

//===================================================================
// DebugDrawPathAdjustments
//===================================================================
void CAISystem::DebugDrawPathAdjustments() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	const char *pName=gAIEnv.CVars.DrawPathAdjustment;
	if(!pName )
		return;
	CAIObject *pTargetObject = GetAIObjectByName(pName);
	if (pTargetObject)
	{
		if (CPuppet *pTargetPuppet = pTargetObject->CastToCPuppet())
      pTargetPuppet->GetPathAdjustmentObstacles(false).DebugDraw();
    if (CAIVehicle *pTargetVehicle = pTargetObject->CastToCAIVehicle())
      pTargetVehicle->GetPathAdjustmentObstacles(false).DebugDraw();
		return;
	}
	if(strcmp(pName, "all"))
		return;
	// find if there are any puppets
	int cnt=0;
	AIObjectOwners::const_iterator ai;
	if ((ai = m_Objects.find(AIOBJECT_PUPPET)) != m_Objects.end())
  {
		for (;ai!=m_Objects.end(); ++ai)
		{
			if (ai->first != AIOBJECT_PUPPET)
				break;
			cnt++;
			CPuppet *pPuppet = (CPuppet*) ai->second.GetAIObject();
			pPuppet->GetPathAdjustmentObstacles(false).DebugDraw();
		}
  }
  if ((ai = m_Objects.find(AIOBJECT_VEHICLE)) != m_Objects.end())
  {
    for (;ai!=m_Objects.end(); ++ai)
    {
      if (ai->first != AIOBJECT_VEHICLE)
        break;
      cnt++;
      CAIVehicle *pVehicle = (CAIVehicle*) ai->second.GetAIObject();
      pVehicle->GetPathAdjustmentObstacles(false).DebugDraw();
    }
  }
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawPathSingle(const CPuppet *pPuppet) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if (!pPuppet->IsEnabled())
		return;

	// debug path draws all the path
	// m_Path gets nodes popped off the front...
	pPuppet->m_Path.Draw();
  if (pPuppet->m_pPathFollower)
    pPuppet->m_pPathFollower->Draw();

	// draw the first part of the path in a different colour
	if (!pPuppet->m_OrigPath.GetPath().empty())	
	{
		TPathPoints::const_iterator li,linext;
		li = pPuppet->m_OrigPath.GetPath().begin();
		linext = li;
		++linext;
    Vec3 endPt = pPuppet->m_Path.GetNextPathPos(ZERO);
    CDebugDrawContext dc;
		while (linext != pPuppet->m_OrigPath.GetPath().end())
		{
			Vec3 p0 = li->vPos;
			Vec3 p1 = linext->vPos;
      p0.z = dc->GetDebugDrawZ(p0, li->navType == IAISystem::NAV_TRIANGULAR);
      p1.z = dc->GetDebugDrawZ(p1, li->navType == IAISystem::NAV_TRIANGULAR);
			dc->DrawLine(p0, ColorB(255, 0, 255), p1, ColorB(255, 0, 255));
			endPt.z = li->vPos.z;
			if (endPt.IsEquivalent(li->vPos, 0.1f))
				break;
			li=linext++;
		}
	}
}


//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawAgents() const 
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if (gAIEnv.CVars.AgentStatsDist <= 1.0f)
		return;

	CDebugDrawContext dc;
	float	drawDistSq = sqr(gAIEnv.CVars.AgentStatsDist);

	// output all puppets
	AIObjectOwners::const_iterator ai;
	if ((ai = m_Objects.find(AIOBJECT_PUPPET)) != m_Objects.end())
	{
		for ( ; ai!=m_Objects.end(); ++ai)
		{
			if (ai->first != AIOBJECT_PUPPET)
				break;

			float distSq = (dc->GetCameraPos() - (ai->second.GetAIObject())->GetPos()).GetLengthSquared();
			if(distSq > drawDistSq)
				continue;

			DebugDrawAgent(ai->second.GetAIObject());
		}

		// output all vehicles
		if ((ai = m_Objects.find(AIOBJECT_VEHICLE)) != m_Objects.end())
		{
			for ( ; ai!=m_Objects.end(); ++ai)
			{
				if (ai->first != AIOBJECT_VEHICLE)
					break;

				float distSq = (dc->GetCameraPos() - (ai->second.GetAIObject())->GetPos()).GetLengthSquared();
				if (distSq > drawDistSq)
					continue;

				DebugDrawAgent(ai->second.GetAIObject());
			}
		}
	}

	// TODO(marcio): Sort by Z, and maybe reduce opacity to far away puppets!
}

namespace EnabledAgentStats
{
	
}


//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawAgent(CAIObject* pAgentObj) const 
{
	
	FUNCTION_PROFILER ( GetISystem(), PROFILE_AI );

	CAIActor* pAgent = CastToCAIActorSafe(pAgentObj);
	if(!pAgentObj || !pAgentObj->IsEnabled())
		return;

	enum EEnabledStateFlags
	{
		Name									= 1 << 0,
		Distances							= 1 << 1,
		Cover									= 1 << 2,
		BehaviorSelectionNode	= 1 << 3,
		Behavior							= 1 << 4,
		Target								= 1 << 5,
		GoalPipe							= 1 << 6,
		GoalOp								= 1 << 7,
		Stance								= 1 << 8,
		Firemode							= 1 << 9,
		TerritoryWave					= 1 << 10,
	};

	struct 
	{
		char ch;
		uint32 flag;
	} flagMap[] =
	{
		{ 'N', Name },
		{ 'd', Distances },
		{ 'c', Cover },
		{ 'B', BehaviorSelectionNode },
		{ 'b', Behavior },
		{ 't', Target },
		{ 'G', GoalPipe },
		{ 'g', GoalOp },
		{ 'S', Stance },
		{ 'f', Firemode },
		{ 'w', TerritoryWave },
	};
	uint32 flagCount = sizeof(flagMap)/sizeof(flagMap[0]);

	const char* enabledFlags = gAIEnv.CVars.DrawAgentStats;
	uint32 enabledStats  = 0;
	for (uint32 i = 0; i < flagCount; ++i)
	{
		if (strchr(enabledFlags, flagMap[i].ch))
			enabledStats |= flagMap[i].flag;
	}

	// count enabled flags to estimate the offset
	uint32 enabledBits = enabledStats;
	uint32 enabledCount = 0;

	for ( ; enabledBits; ++enabledCount)
		enabledBits &= enabledBits - 1;

	CAIVehicle* pVehicle = pAgent->CastToCAIVehicle();
	if(pVehicle && !pVehicle->IsDriverInside())
		return;

	CPipeUser* pPipeUser = pAgent->CastToCPipeUser();

	Vec3 pos = pAgent->GetPos();
	Vec3 agentHead = pos;

	CDebugDrawContext dc;

	float x, y, z;
	dc->ProjectToScreen(agentHead.x, agentHead.y, agentHead.z, &x, &y, &z);
	if ((z < 0.0f) || (z > 1.0f))
		return;

	x *= dc->GetWidth() * 0.01f;
	y *= dc->GetHeight() * 0.01f;

	const ColorB white(255, 255, 255, 255);
	const ColorB grey(112, 112, 112, 255);
	const ColorB green(34, 139, 34, 255);
	const ColorB orange(218, 165, 96, 255);
	const ColorB red(139, 0, 0, 255);
	const ColorB yellow(245, 222, 32, 255);
	const ColorB blue(70, 120, 180, 255);

	const float	fontSize = 1.25f;
	const float fontHeight = 10.0f * fontSize;

	float offset = pVehicle ? 100.0f : 130.0f;
	offset *= (enabledCount / (float)flagCount);

	if (pPipeUser->GetProxy() && pPipeUser->GetProxy()->GetLinkedVehicleEntityId())
		y -= 85.0f;

	y -= 20.0f + offset;
	x -= 50.0f;

	if (enabledStats & Name)
	{
		dc->Draw2dLabel(x, y, fontSize * 1.25f, pVehicle ? green : white, false, "%s", pAgent->GetName());
		y += fontHeight * 1.25f;
	}

	if (enabledStats & Distances)
	{
		float distToPlayer = 0.0f;

		if (IAIObject* pPlayer = GetPlayer())
			distToPlayer = (agentHead - pPlayer->GetPos()).len();

		float distToTarget = 0.0f;
		if (IAIObject* pTarget = pPipeUser->GetAttentionTarget())
			distToTarget = (agentHead - pTarget->GetPos()).len();

		float distToPathEnd = 0.0f;
		if (pPipeUser->m_State.fDistanceToPathEnd > 0.0001f)
			distToPathEnd = pPipeUser->m_State.fDistanceToPathEnd;

		dc->Draw2dLabel(x, y, fontSize, white, false, "Plyr: %.2f Trgt: %.2f Path: %.2f",
			distToPlayer, distToTarget, distToPathEnd);
		y += fontHeight;
	}

	if (!pVehicle && (enabledStats & Cover))
	{
		dc->Draw2dLabel(x, y, fontSize * 0.85f, pPipeUser->IsMovingToCover() ? green : grey, false, "%s", "MC");
		dc->Draw2dLabel(x + 14.0f, y, fontSize * 0.85f, pPipeUser->IsInCover() ? green : grey, false, "%s", "IC");
		dc->Draw2dLabel(x + 28.0f, y, fontSize * 0.85f, pPipeUser->IsCoverCompromised() ? red : grey, false, "%s", "CC");
		y += 0.85f * fontHeight;
	}

	y += fontHeight * 0.5f; // extra spacing

	static string text;
	text.clear();

	// Behavior Selection
	if (SelectionTree* behaviorSelectionTree = pAgent->GetBehaviorSelectionTree())
	{
		if (enabledStats & BehaviorSelectionNode)
		{
			SelectionNodeID nodeID = behaviorSelectionTree->GetCurrentNodeID();
			if (nodeID)
			{
				const SelectionTreeNode& node = behaviorSelectionTree->GetNode(nodeID);
				const char* nodeName = node.GetName();

				if (node.GetParentID())
				{
					const SelectionTreeNode& parentNode = behaviorSelectionTree->GetNode(node.GetParentID());
					text = parentNode.GetName();
					text.append(" > ");
					text.append(nodeName);
				}
				else
					text = nodeName;
			}

			if (!text.empty())
			{
				dc->Draw2dLabel(x, y, fontSize * 1.15f, blue, false, "%s", text.c_str());
				y += fontHeight * 1.15;
			}
			else
			{
				dc->Draw2dLabel(x, y, fontSize * 1.15f, red, false, "%s", "No Selection");
				y += fontHeight * 1.15;
			}
		}
	}
	else
	{
		const TBSSProfileUserId userId = pAgent->GetBehaviorTreeUserId();
		SBTProfileUser* pUser = m_pBTProfileDictionary->GetProfileUser(userId);
		IPersonalBehaviorTree* pPersonalBT = (pUser ? pUser->m_pPersonalTree : 0);
		SBTProfile *pProfile = (pPersonalBT ? pPersonalBT->GetProfile() : 0);

		if (pProfile)
			pPersonalBT->GetCurrentNodePath(text);
		else if (IAIActorProxy	*pProxy = pAgent->GetProxy())
			text = pProxy->GetCurrentBehaviorName();

		if (enabledStats & BehaviorSelectionNode)
		{
			if (!text.empty())
				dc->Draw2dLabel(x, y, fontSize * 1.15f, blue, false, "%s", text.c_str());
			else
				dc->Draw2dLabel(x, y, fontSize * 1.15f, red, false, "%s", "No Selection");

			y += fontHeight * 1.15f;
		}
	}

	text.clear();

	// Behavior
	if (enabledStats & Behavior)
	{
		text = pPipeUser->GetProxy()->GetCurrentBehaviorName();

		if (!text.empty())
			dc->Draw2dLabel(x, y, fontSize * 0.85f, white, false, "%s", text.c_str());
		else
			dc->Draw2dLabel(x, y, fontSize * 0.85f, red, false, "%s", "No Behavior");

		y += fontHeight * 0.85f;
	}
	
	if(pPipeUser)
	{
		SAIBodyInfo bodyInfo;
		pPipeUser->GetProxy()->QueryBodyInfo(bodyInfo);

		// Target
		if (enabledStats & Target)
		{
			if (pPipeUser->GetAttentionTarget())
			{
				text = ">> ";
				text += pPipeUser->GetAttentionTarget()->GetName();

				switch (pPipeUser->GetAttentionTargetType())
				{
				case AITARGET_VISUAL:
					text += "  <VIS";
					break;
				case AITARGET_MEMORY:
					text += "  <MEM";
					break;
				case AITARGET_SOUND:
					text += "  <SND";
					break;
				default:
					text += "  <-  ";
					break;
				}

				switch (pPipeUser->GetAttentionTargetThreat())
				{
				case AITHREAT_AGGRESSIVE: 
					text += " AGG>";
					break;
				case AITHREAT_THREATENING: 
					text += " THR>";
					break;
				case AITHREAT_INTERESTING: 
					text += " INT>";
					break;
				default:
					text += " -  >";
					break;
				}

				// attentionTarget
				if (gAIEnv.CVars.DrawAttentionTargetsPosition)
					dc->DrawSphere(pPipeUser->GetAttentionTarget()->GetPos(), 0.1f, red);
			}	
			else
				text = ">> No Target";

			dc->Draw2dLabel(x, y, fontSize, white, false, "%s", text.c_str());
			y += fontHeight;
		}

		// GoalPipe
		if (CGoalPipe* pPipe = pPipeUser->GetCurrentGoalPipe())
		{
			if (enabledStats & GoalPipe)
			{
				int lineCount = 1;
				const string& sDebugName = pPipe->GetDebugName();
				const string& pipeName = sDebugName.empty() ? pPipe->GetNameAsString() : sDebugName;

				text = pipeName;

				CGoalPipe* pSubPipe = pPipe;

				while (pSubPipe = pSubPipe->GetSubpipe())
				{
					const string& sDebugName = pSubPipe->GetDebugName();
					const string& subpipeName = sDebugName.empty() ? pSubPipe->GetNameAsString() : sDebugName;

					text += ", ";

					if ((text.length() + subpipeName.length()) < 112)
						text += subpipeName;
					else
					{
						dc->Draw2dLabel(x, y, fontSize, white, false, "%s", text.c_str());
						y += fontSize;

						if (lineCount++ < 3)
						{
							text = "  ";
							text += subpipeName;
						}
						else
						{
							text = "  ...";
							break;
						}
					}
				}

				const ColorB color = pPipeUser->IsPaused() ? grey : white;

				dc->Draw2dLabel(x, y, fontSize, color, false, "%s", text.c_str());
				y += fontHeight;
			}

			// Goal Op
			if (enabledStats & GoalOp)
			{
				const ColorB color = pPipeUser->IsPaused() ? grey : white;

				if (pPipeUser->m_lastExecutedGoalop != eGO_LAST)
					dc->Draw2dLabel(x, y, fontSize, color, false, "%s", pPipe->GetGoalOpName(pPipeUser->m_lastExecutedGoalop));
				y += fontHeight;
			}
		}


		// Stance
		if (!pVehicle && (enabledStats & Stance))
		{
			text = GetStanceName(pPipeUser->m_State.bodystate);

			if(pPipeUser->m_State.lean < -0.01f)
				text += "  LeanLeft";
			else if(pPipeUser->m_State.lean > 0.01f)
				text += "  LeanRight";
			if(pPipeUser->m_State.peekOver > 0.01f)
				text += "  PeekOver";

			if(pPipeUser->m_State.allowStrafing)
				text += "  Strafe";

			float urgency = cry_fabsf(pPipeUser->m_State.fMovementUrgency);
			bool signal = pPipeUser->m_State.fMovementUrgency >= 0.0f;

			if (urgency > 0.001f)
			{
				if (urgency < AISPEED_WALK)
					text += signal ? " Slow" : " -Slow";
				else if (urgency < AISPEED_RUN)
					text += signal ? " Walk" : " -Walk";
				else if (urgency < AISPEED_SPRINT)
					text += signal ? " Run" : " -Run";
				else
					text += signal ? " Sprint" : " -Sprint";
			}

			dc->Draw2dLabel(x, y, fontSize, orange, false, "%s", text.c_str());
			y += fontHeight;
		}

		// FireMode
		if (enabledStats & Firemode)
		{
			EAimState	aimState = pPipeUser->GetAimState();
			EFireMode	fireMode = pPipeUser->GetFireMode();

			ColorB firemodeColor = green;

			if (fireMode != FIREMODE_OFF)
			{
				text = "";
				if (aimState == AI_AIM_WAITING)
				{
					text = "\\";
					firemodeColor = yellow;
				}
				else if (aimState == AI_AIM_READY)
				{
					text = "--";
				}
				else if (aimState == AI_AIM_FORCED)
				{
					text = "==";
				}
				else if (aimState == AI_AIM_OBSTRUCTED)
				{
					firemodeColor = red;
					text = "||";
				}

				if ((fireMode == FIREMODE_MELEE) || (fireMode == FIREMODE_MELEE_FORCED))
				{
					text = "Melee";
				}
				else if(fireMode == FIREMODE_SECONDARY)
				{
					if(pPipeUser->m_State.fireSecondary)
						text += " )) ** ";
					else
						text += " )) ";
				}
				else if (fireMode != FIREMODE_AIM)
				{
					if (pPipeUser->m_State.fire)
						text += " >> ** ";
					else
						text += " >> ";
				}
			}
			else
			{
				firemodeColor = grey;
				text = "off";
			}

			dc->Draw2dLabel(x, y, fontSize, firemodeColor, false, "%s", text.c_str());

			if (fireMode != FIREMODE_OFF)
			{
				CPuppet* pPuppet = pPipeUser->CastToCPuppet();
				bool isAmbient = pPuppet && !pPuppet->IsAllowedToHitTarget();

				if (isAmbient)
				{
					const float centerY = y + 0.5f * (fontHeight - (fontHeight * 0.85f));
					dc->Draw2dLabel(x + 55.0f, centerY, fontSize * 0.85f, yellow, false, "%s", "A");
				}

				text = "";

				switch(fireMode)
				{
				case FIREMODE_BURST:
					text = "(Burst)";
					break;
				case FIREMODE_CONTINUOUS:
					text = "(Continuous)";
					break;
				case FIREMODE_FORCED:
					text = "(Forced)";
					break;
				case FIREMODE_AIM:
					text = "(Aim)";
					break;
				case FIREMODE_SECONDARY:
					text = "(Secondary)";
					break;
				case FIREMODE_SECONDARY_SMOKE:
					text = "(Secondary Smoke)";
					break;
				case FIREMODE_KILL:
					text = "(Kill)";
					break;
				case FIREMODE_BURST_WHILE_MOVING:
					text = "(Burst While Moving)";
					break;
				case FIREMODE_PANIC_SPREAD:
					text = "(Panic Spread)";
					break;
				case FIREMODE_BURST_DRAWFIRE:
					text = "(Draw Fire)";
					break;
				case FIREMODE_MELEE_FORCED:
					text = "(Forced)";
					break;
				case FIREMODE_BURST_SNIPE:
					text = "(Burst Snipe)";
					break;
				case FIREMODE_AIM_SWEEP:
					text = "(Aim Sweep)";
					break;
				case FIREMODE_BURST_ONCE:
					text = "(Burst Once)";
					break;
				}

				if (!text.empty())
				{
					const float centerY = y + 0.5f * (fontHeight - (fontHeight * 0.85f));
					dc->Draw2dLabel(x + 68.0f, centerY, fontSize * 0.85f, isAmbient ? yellow : white, false, "%s", text.c_str());
				}
			}

			y += fontHeight;
		}

		// Territory and Wave
		if (enabledStats & TerritoryWave)
		{
			if (CPuppet* pPuppet = pAgent->CastToCPuppet())
			{
				text = "Territory: ";
				text += pPuppet->GetTerritoryShapeName();
				text = "\nWave: ";
				text += pPuppet->GetWaveName();

				dc->Draw2dLabel(x, y, fontSize, white, false, "%s", text.c_str());
				y += fontHeight;
			}
		}

		// Debug Draw goal ops.
		if (gAIEnv.CVars.DrawGoals)
			pPipeUser->DebugDrawGoals();

		if (gAIEnv.CVars.DebugDrawCover)
		{
			if (pPipeUser->m_CurrentHideObject.IsValid())
				pPipeUser->m_CurrentHideObject.DebugDraw();
		}

		// Desired view direction and movement direction are drawn in yellow, actual look and move directions are drawn in blue.
		// The lookat direction is marked with sphere, and the movement direction is parked with cone.

		const ColorB desiredColor(255, 204, 13);
		const ColorB actualColor(26, 51, 204, 128);

		const float	rad = pPipeUser->m_Parameters.m_fPassRadius;

		Vec3	physPos = pPipeUser->GetPhysicsPos() + Vec3(0,0,0.25f);

		// Pathfinding radius circle
		dc->DrawCircleOutline(physPos, rad, actualColor);

		// The desired movement direction.
		if (pPipeUser->m_State.vMoveDir.GetLengthSquared() > 0.0f)
		{
			const Vec3&	dir = pPipeUser->m_State.vMoveDir;
			const Vec3	target = physPos + dir * (rad + 0.7f);
			dc->DrawLine(physPos+dir*rad, desiredColor, target, desiredColor);
			dc->DrawCone(target, dir, 0.05f, 0.2f, desiredColor);

			// The desired speed.
			/*			const ColorB speedColor(51, 255, 77);
			const Vec3	speedPos(physPos + Vec3(0, 0, 0.2f));
			const Vec3	speedTarget = speedPos + dir * (rad + 1.0f) * pPipeUser->m_State.fDesiredSpeed;
			dc->DrawLine(speedPos, speedColor, speedTarget, speedColor);*/
		}
		// The desired lookat direction.
		if (!pPipeUser->m_State.vLookTargetPos.IsZero())
		{
			Vec3	dir = pPipeUser->m_State.vLookTargetPos - pPipeUser->GetPos();
			CRY_ASSERT(dir.IsValid());
			dir.NormalizeSafe(Vec3(ZERO));
			const Vec3 target = agentHead + dir * (rad + 0.7f);
			dc->DrawLine(pos, desiredColor, target, desiredColor);
			dc->DrawSphere(target, 0.07f, desiredColor);
		}
		// The actual movement direction
		if (!pPipeUser->GetMoveDir().IsZero())
		{
			const Vec3&	dir = pPipeUser->GetMoveDir();
			const Vec3	target = physPos + dir * (rad + 1.0f);
			dc->DrawLine(physPos+dir*rad, actualColor, target, actualColor);
			dc->DrawCone(target, dir, 0.05f, 0.2f, actualColor);
		}
		// The actual body direction
		if (!pPipeUser->GetBodyDir().IsZero())
		{
			const Vec3&	dir = pPipeUser->GetBodyDir();
			const Vec3	target = physPos + dir * rad;
			dc->DrawCone(target, dir, 0.07f, 0.3f, actualColor);
		}
		// The actual lookat direction
		if (!pPipeUser->GetViewDir().IsZero())
		{
			const Vec3&	dir = pPipeUser->GetViewDir();
			const Vec3	target = pos + dir * (rad + 1.0f);
			dc->DrawLine(pos, actualColor, target, actualColor);
			dc->DrawSphere(target, 0.07f, actualColor);
		}

		// The aim & fire direction
		{
			/*			bool	allowedToHit = false;
			CPuppet* pPuppet = pAgent->CastToCPuppet();
			if (pPuppet)
			allowedToHit = pPuppet->IsAllowedToHitTarget();*/
			ColorB fireColor(255, 26, 0, 179); 
			//			if (!allowedToHit)
			//				fireColor.Set(255, 255, 255, 51);

			Vec3 firePos = pPipeUser->GetFirePos();

			// Aim dir
			if (!pPipeUser->m_State.vAimTargetPos.IsZero())
			{
				Vec3	dir = pPipeUser->m_State.vAimTargetPos - firePos;
				dir.NormalizeSafe(Vec3(ZERO));
				const Vec3	target = firePos + dir * (rad + 0.5f);
				dc->DrawLine(firePos, desiredColor, target, desiredColor);
				dc->DrawCylinder(target, dir, 0.035f, 0.07f, desiredColor);
				//				dc->DrawSphere(target, 0.07f, desiredColor);
			}

			// Weapon dir
			{
				const Vec3&	dir = bodyInfo.vFireDir;
				const Vec3	target = firePos + dir * (rad + 0.8f);
				dc->DrawLine(firePos, fireColor, target, fireColor);
				dc->DrawCylinder(target, dir, 0.035f, 0.07f, fireColor);
			}

			/*
			if (pPipeUser->m_State.fire)
			{
			dc->DrawCone(pPipeUser->GetFirePos(), bodyInfo.vFireDir, 0.07f, 1.0f, fireColor);
			dc->DrawLine(pPipeUser->GetFirePos(), fireColor, pPipeUser->m_State.vShootTargetPos, fireColor);
			}
			else
			{
			dc->DrawCone(pPipeUser->GetFirePos(), bodyInfo.vFireDir, 0.07f, 0.75f, actualColor);
			}
			*/

			// Aim
			/*			EAimState	aim = pPipeUser->GetAimState();
			if (aim == AI_AIM_READY || aim == AI_AIM_FORCED)
			dc->DrawSphere(pPipeUser->GetFirePos() + bodyInfo.vFireDir, 0.07f, actualColor);

			if (!pPipeUser->m_State.vAimTargetPos.IsZero())
			{
			Vec3 dir = pPipeUser->m_State.vAimTargetPos - pPipeUser->GetFirePos();
			float len = dir.NormalizeSafe();
			const unsigned int maxDashes = 40;
			const float dashSize = 0.25f;
			const float maxLen = maxDashes * 2 * dashSize;
			if (len > maxLen)
			len 

			//				dc->DrawLine(pPipeUser->GetFirePos() + Vec3(0,0,0.01f), fireColor*0.75f, pPipeUser->m_State.vAimTargetPos + Vec3(0,0,0.01f), fireColor*0.75f);
			}
			*/
		}

		const float	drawFOV = gAIEnv.CVars.DrawAgentFOV;
		if( drawFOV > 0 )
		{
			// Draw the view cone
			dc->DrawWireFOVCone(pos, pPipeUser->GetViewDir(),
				pPipeUser->m_Parameters.m_PerceptionParams.sightRange * drawFOV * 0.99f,
				DEG2RAD(pPipeUser->m_Parameters.m_PerceptionParams.FOVPrimary) * 0.5f, ColorB(255,255,255));

			dc->DrawWireFOVCone(pos, pPipeUser->GetViewDir(),
				pPipeUser->m_Parameters.m_PerceptionParams.sightRange * drawFOV,
				DEG2RAD(pPipeUser->m_Parameters.m_PerceptionParams.FOVSecondary) * 0.5f, ColorB(128,128,128));

			Vec3 midPoint = pos + pPipeUser->GetViewDir()*pPipeUser->m_Parameters.m_PerceptionParams.sightRange * drawFOV/2;
			dc->DrawLine(pos, ColorB(255, 255, 255), midPoint, ColorB(255, 255, 255));
			dc->Draw3dLabel(midPoint,1.5f,"FOV %.1f/%.1f",
				pPipeUser->m_Parameters.m_PerceptionParams.FOVPrimary,
				pPipeUser->m_Parameters.m_PerceptionParams.FOVSecondary);
		}

		// my Ref point - yellow
		string rfname = gAIEnv.CVars.DrawRefPoints;
		if(rfname!="")
		{
			int group =atoi(rfname) ;
			if(rfname=="all" || rfname==pPipeUser->GetName() || ((rfname=="0" || group>0) && group==pPipeUser->GetGroupId()))
			{
				if (pPipeUser->GetRefPoint())
				{
					Vec3 rppos = pPipeUser->GetRefPoint()->GetPos();
					dc->DrawLine(pPipeUser->GetPos(), ColorB(255, 0, 0), rppos, ColorB(255, 255, 0));
					dc->DrawSphere(rppos, .5, ColorB(255, 255, 0));
					rppos.z += 1.5f;
					dc->Draw3dLabel(rppos, fontSize, pPipeUser->GetRefPoint()->GetName());
				}
				/* moved in DebugDrawOneGroup
				CAIObject* pBeacon = (CAIObject*) GetAISystem()->GetBeacon(pAgent->GetGroupId());
				if(pBeacon)
				{
				dc->DrawSphere(pBeacon->GetPos(),0.35f, ColorB(0, 255, 255));
				dc->Draw3dLabel(pBeacon->GetPos()+ Vec3(0,0,1),1,"%s", pBeacon->GetName());

				}
				*/
			}
		}

		CPuppet* pPuppet = pAgent->CastToCPuppet();

		if(gAIEnv.CVars.DebugTargetSilhouette > 0)
		{
			if(pPuppet->m_targetSilhouette.valid && !pPuppet->m_targetSilhouette.points.empty())
			{
				const Vec3& u = pPuppet->m_targetSilhouette.baseMtx.GetColumn0();
				const Vec3& v = pPuppet->m_targetSilhouette.baseMtx.GetColumn2();

				// Draw silhouette
				size_t n = pPuppet->m_targetSilhouette.points.size();
				for(size_t i = 0; i < n; ++i)
				{
					size_t	j = (i + 1) % n;
					Vec3	pi = pPuppet->m_targetSilhouette.center + u * pPuppet->m_targetSilhouette.points[i].x + v * pPuppet->m_targetSilhouette.points[i].y;
					Vec3	pj = pPuppet->m_targetSilhouette.center + u * pPuppet->m_targetSilhouette.points[j].x + v * pPuppet->m_targetSilhouette.points[j].y;
					dc->DrawLine(pi, ColorB(255, 255, 255), pj, ColorB(255, 255, 255));
				}

				if(!pPuppet->m_targetLastMissPoint.IsZero())
				{
					dc->Draw3dLabel(pPuppet->m_targetLastMissPoint, 1.0f, "Last Miss");
					dc->DrawSphere(pPuppet->m_targetLastMissPoint, 0.1f, ColorB(255, 128, 0));
				}

				// The attentiontarget on the silhouette plane.
				if(!pPuppet->m_targetPosOnSilhouettePlane.IsZero())
				{
					Vec3	dir = (pPuppet->m_targetPosOnSilhouettePlane - pPuppet->GetFirePos()).GetNormalizedSafe();
					dc->DrawCone(pPuppet->m_targetPosOnSilhouettePlane, dir, 0.15f, 0.2f, ColorB(255, 0, 0, pPuppet->m_targetDistanceToSilhouette < 3.0f ? 255 : 64));
					dc->DrawLine(pPuppet->GetFirePos(), ColorB(255, 0, 0), pPuppet->m_targetPosOnSilhouettePlane, ColorB(255, 0, 0));
				}

				{
					const Vec3& u2 = pPuppet->m_targetSilhouette.baseMtx.GetColumn0();
					const Vec3& v2 = pPuppet->m_targetSilhouette.baseMtx.GetColumn2();

					Vec3 targetBiasDirectionProj = pPuppet->m_targetSilhouette.ProjectVectorOnSilhouette(pPuppet->m_targetBiasDirection);
					targetBiasDirectionProj.NormalizeSafe(Vec3(0,0,0));
					Vec3 pos2 = pPuppet->m_targetSilhouette.center + u2*targetBiasDirectionProj.x + v2*targetBiasDirectionProj.y;

					dc->DrawLine(pPuppet->m_targetSilhouette.center, ColorB(255, 0, 0, 0), pos2, ColorB(255, 0, 0));

					dc->DrawLine(pPuppet->m_targetSilhouette.center, ColorB(0, 0, 255, 0), pPuppet->m_targetSilhouette.center + pPuppet->m_targetBiasDirection, ColorB(0, 0, 255));

					dc->DrawLine(pos2, ColorB(255, 255, 255, 128), pPuppet->m_targetSilhouette.center + pPuppet->m_targetBiasDirection, ColorB(255, 255, 255, 128));
				}

				// Test miss points
				/*				Vec3	missPt(0,0,0);
				if(pPuppet->GetMissPointAroundTarget(0.5f, missPt))
				{
				dc->DrawLine(pPuppet->GetFirePos(), ColorB(255, 255, 255), missPt, ColorB(255, 255, 255));
				}*/

				char* szZone = "";
				switch(pPuppet->m_targetZone)
				{
				case AIZONE_OUT: szZone = "Out"; break;
				case AIZONE_WARN: szZone = "Warn"; break;
				case AIZONE_COMBAT_NEAR: szZone = "Combat-Near"; break;
				case AIZONE_COMBAT_FAR: szZone = "Combat-Far"; break;
				case AIZONE_KILL: szZone = "Kill"; break;
				case AIZONE_IGNORE: szZone = "Ignored"; break;
				}

				dc->Draw3dLabel(pPuppet->GetPos() - Vec3(0,0,1.5f), 1.2f, "Focus:%d\nZone:%s", (int)(pPuppet->m_targetFocus*100.0f), szZone);

			}

			/*			Vec3	p = m_silhuetteCenter + u * m_projTargetVel.x + v * m_projTargetVel.y;
			dc->DrawLine(m_silhuetteCenter, ColorB(255, 255, 255), p, ColorB(255, 255, 255));

			if(m_lastMissAngle >= 0.0f)
			{
			float	x = cosf(m_lastMissAngle);
			float	y = sinf(m_lastMissAngle);
			Vec3	p = m_silhuetteCenter + u*x + v*y;
			dc->DrawLine(m_silhuetteCenter, ColorB(255, 0, 0), p, ColorB(255, 0, 0));
			}*/
		}

		// Display the readibilities.
		if( gAIEnv.CVars.DrawReadibilities )
			pAgent->GetProxy()->DebugDraw(2);
	}

	if (gAIEnv.CVars.DrawProbableTarget > 0)
	{
		CPuppet*	pPuppet = pAgent->CastToCPuppet();
		if(pPuppet)
		{
			if (pPuppet->GetTimeSinceLastLiveTarget() >= 0.0f)
			{
				const Vec3&	pos2 = pPuppet->GetPos();
				Vec3	livePos = pPuppet->GetLastLiveTargetPosition();
				Vec3	livePosConstrained = livePos;

				ColorB	color1			(171, 60, 184);
				ColorB	color1Trans	(171, 60, 184, 128);
				ColorB	color2			(110, 60, 184);
				ColorB	color2Trans	(110, 60, 184, 128);

				if (pPuppet->GetTerritoryShape() && pPuppet->GetTerritoryShape()->ConstrainPointInsideShape(livePosConstrained, true))
				{
					Vec3	mid = livePosConstrained * 0.7f + pos2 * 0.3f;
					dc->DrawLine(mid, color1, livePosConstrained, color1Trans);
					dc->DrawLine(livePosConstrained, color2Trans, livePos, color2Trans);
					dc->DrawSphere(livePosConstrained, 0.25f, color1Trans);
					dc->DrawSphere(livePos, 0.5f, color2Trans);
				}
				else
				{
					Vec3	mid = livePos * 0.7f + pos2 * 0.3f;
					dc->DrawLine(mid, color1, livePos, color1Trans);
					dc->DrawSphere(livePos, 0.5f, color1Trans);
				}
			}
		}
	}

	// Display damage parts.
	if(gAIEnv.CVars.DebugDrawDamageParts > 0)
	{
		if(pAgent->GetDamageParts())
		{
			DamagePartVector*	parts = pAgent->GetDamageParts();
			for(DamagePartVector::iterator it = parts->begin(); it != parts->end(); ++it)
			{
				SAIDamagePart&	part = *it;
				dc->Draw3dLabel(part.pos, 1,"^ DMG:%.2f\n  VOL:%.1f", part.damageMult, part.volume);
			}
		}
	}

	// Draw the approximate stance size.
	if(gAIEnv.CVars.DebugDrawStanceSize > 0)
	{
		if(pAgent->GetProxy())
		{
			SAIBodyInfo	bodyInfo;
			pAgent->GetProxy()->QueryBodyInfo(bodyInfo);
			Vec3	pos2 = pAgent->GetPhysicsPos();
			AABB	aabb(bodyInfo.stanceSize);
			aabb.Move(pos2);
			dc->DrawAABB(aabb, true, ColorB(255, 255, 255, 128), eBBD_Faceted);
		}
	}

	// Draw active exact positioning request
	{
		CPuppet*	pPuppet = pAgent->CastToCPuppet();
		if (pPuppet)
		{
			// Draw pending actor target request. These should not be hanging around, so draw one of it is requested.
			if( const SAIActorTargetRequest* pReq = pPuppet->GetActiveActorTargetRequest())
			{
				Vec3	pos2 = pReq->approachLocation;
				Vec3	dir = pReq->approachDirection;
				dc->DrawArrow(pos2 - dir*0.5f, dir, 0.2f, ColorB(0,255,0,196));
				dc->DrawLine(pos2, ColorB(0, 255, 0), pos2 - Vec3(0, 0, 0.5f), ColorB(0, 255, 0));
				dc->DrawLine(pReq->approachLocation, ColorB(255, 255, 255, 128), pReq->animLocation, ColorB(255, 255, 255, 128));
				if(!pReq->animation.empty())
					dc->Draw3dLabel(pos2 + Vec3(0,0,0.5f), 1.5f,"%s", pReq->animation.c_str());
				else if(!pReq->vehicleName.empty())
					dc->Draw3dLabel(pos2 + Vec3(0,0,0.5f), 1.5f,"%s Seat:%d", pReq->vehicleName.c_str(), pReq->vehicleSeat);
			}

			const Vec3	size(0.1f, 0.1f, 0.1f);
			if (!pPuppet->m_DEBUGCanTargetPointBeReached.empty())
			{
				for(ListPositions::iterator it = pPuppet->m_DEBUGCanTargetPointBeReached.begin(); it != pPuppet->m_DEBUGCanTargetPointBeReached.end(); ++it)
				{
					ColorB	color(255,0,0,128);
					Vec3	pos2 = *it;
					dc->DrawAABB(AABB(pos2-size, pos2+size), true, color, eBBD_Faceted);
					dc->DrawLine(pos2, color, pos2 - Vec3(0,0,1.0f), color);
				}
			}
			if (!pPuppet->m_DEBUGUseTargetPointRequest.IsZero())
			{
				ColorB	color(0,0,255);
				Vec3	pos2 = pPuppet->m_DEBUGUseTargetPointRequest + Vec3(0,0,0.4f);
				dc->DrawAABB(AABB(pos2-size, pos2+size), true, color, eBBD_Faceted);
				dc->DrawLine(pos2, color, pos2 - Vec3(0,0,1.0f), color);
			}

			// Actor target phase.
			if( pPuppet->m_State.actorTargetReq.id != 0)
			{

				Vec3	pos2 = pPuppet->m_State.actorTargetReq.approachLocation;
				//				Vec3	dir = pPuppet->m_State.actorTargetReq.approachDirection;

				dc->DrawRangeCircle(pos2 + Vec3(0,0,0.3f), pPuppet->m_State.actorTargetReq.startArcAngle, pPuppet->m_State.actorTargetReq.startArcAngle, ColorB(255,255,255,128), ColorB(255,255,255), true);

				const char*	szPhase = "";
				switch(pPuppet->m_State.curActorTargetPhase)
				{
				case eATP_None: szPhase = "None"; break;
				case eATP_Waiting: szPhase = "Waiting"; break;
				case eATP_Starting: szPhase = "Starting"; break;
				case eATP_Started: szPhase = "Started"; break;
				case eATP_Playing: szPhase = "Playing"; break;
				case eATP_StartedAndFinished: szPhase = "StartedAndFinished"; break;
				case eATP_Finished: szPhase = "Finished"; break;
				case eATP_Error: szPhase = "Error"; break;
				}

				const char*	szType = "<INVALID!>";

				PathPointDescriptor::SmartObjectNavDataPtr pSmartObjectNavData = pPuppet->m_Path.GetLastPathPointAnimNavSOData();
				if(pSmartObjectNavData)
					szType = "NAV_SO";
				else if(pPuppet->GetActiveActorTargetRequest())
					szType = "ACTOR_TGT";

				dc->Draw3dLabel(pos2 + Vec3(0,0,1.0f), 1,"%s\nPhase:%s ID:%d", szType, szPhase, pPuppet->m_State.actorTargetReq.id);
			}
		}
	}
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawPendingEvents(CPuppet* pTargetPuppet, int xPos, int yPos) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	typedef std::map<float, string> t_PendingMap;
	t_PendingMap eventsMap;

	PotentialTargetMap targetMap;
	pTargetPuppet->GetPotentialTargets(targetMap);

	PotentialTargetMap::const_iterator ei = targetMap.begin();
	for (;ei != targetMap.end(); ++ei)
	{
		char	buffString[256];
		const SAIPotentialTarget &ed = ei->second;
		CAIObject *pNextTarget = 0;
		if ((ed.type == AITARGET_VISUAL && ed.threat == AITHREAT_AGGRESSIVE) || ed.refDummyRepresentation.IsNil())
			pNextTarget = ei->first.GetAIObject();
		else
			pNextTarget = ed.refDummyRepresentation.GetAIObject();

		string curName("NULL");
		if (pNextTarget)
			curName = pNextTarget->GetName();

		float timeout = max(0.0f, ed.GetTimeout(pTargetPuppet->GetParameters().m_PerceptionParams));

		const char* szTargetType = "";
		const char* szTargetThreat = "";
	
		switch (ed.type)
		{
		case AITARGET_VISUAL: szTargetType = "VIS"; break;
		case AITARGET_MEMORY: szTargetType = "MEM"; break;
		case AITARGET_SOUND: szTargetType = "SND"; break;
		default: szTargetType = "-  "; break;
		}

		switch (ed.threat)
		{
		case AITHREAT_AGGRESSIVE: szTargetThreat = "AGG"; break;
		case AITHREAT_THREATENING: szTargetThreat = "THR"; break;
		case AITHREAT_INTERESTING: szTargetThreat = "INT"; break;
		default: szTargetThreat = "-  "; break;
		}

		sprintf(buffString, "%.1fs  <%s %s>  %s", timeout, szTargetType, szTargetThreat, curName.c_str());

		//		eventsMap[ed.fPriority] = buffString;
		eventsMap.insert( std::make_pair(ed.priority, buffString));
	}
	int column(40);
	int row(5);
	char buff[256];
	CDebugDrawContext dc;
	for(t_PendingMap::reverse_iterator itr=eventsMap.rbegin(); itr!=eventsMap.rend(); ++itr)
	{
		sprintf(buff,"%.3f %s", itr->first, itr->second.c_str());

		ColorB color;
		if (itr == eventsMap.rbegin())
		{
			// Highlight the current target
			color[0] = 255;
			color[1] = 255;
			color[2] = 255;
			color[3] = 255;
		}
		else
		{
			color[0] = 0;
			color[1] = 192;
			color[2] = 255;
			color[3] = 255;
		}

		dc->Draw2dLabel(column, row, buff, color);
		++row;
		//		dc->TextToScreen((float)xPos, (float)yPos, "%.1f %s", itr->first, itr->second.c_str());
		//		yPos+=12;
	}
}


//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawStatsTarget(const char *pName)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if(!pName )
		return;
	CAIObject *pTargetObject = GetAIObjectByName(pName);
	if (!pTargetObject)
		return;

	CDebugDrawContext dc;
	
	if(!pTargetObject->IsEnabled())
	{
		dc->TextToScreen(0, 60,"%s  #####>>>-----    IS DISABLED -----<<<#####", pTargetObject->GetName());
		return;
	}

	CPuppet* pTargetPuppet = pTargetObject->CastToCPuppet();
	if (!pTargetPuppet)
		return;

	DebugDrawAgent(pTargetObject);

	if(pTargetPuppet->m_State.fire)
		dc->TextToScreen(0,60,">>>FIRING<<<");
	else if(pTargetPuppet->AllowedToFire())
		dc->TextToScreen(0,60,"---FIRING---");
	else
		dc->TextToScreen(0,60,"no fire");

	// Output stance
	string	stanceStr;
	SAIBodyInfo bodyInfo;
	pTargetPuppet->GetProxy()->QueryBodyInfo(bodyInfo);
	stanceStr = GetStanceName(bodyInfo.stance);

  dc->TextToScreen(0, 48, "%s", pTargetPuppet->GetName());
  if(!pTargetPuppet->m_bCanReceiveSignals)
    dc->TextToScreen(0, 50, ">> Cannot Receive Signals! <<");
	dc->TextToScreen(0, 52, "Stance: %s", stanceStr.c_str());

	// This logic is borrowed from AIproxy in order to display the same urgency as sent further down the pipeline from AIproxy.
	float urgency = pTargetPuppet->m_State.fMovementUrgency;
	bool wantsToMove = !pTargetPuppet->m_State.vMoveDir.IsZero();
	if (pTargetPuppet && pTargetPuppet->GetProxy() && pTargetPuppet->GetProxy()->IsAnimationBlockingMovement())
		wantsToMove = false;
	if (!wantsToMove)
		urgency = (pTargetPuppet->m_State.curActorTargetPhase == eATP_Waiting) ? fabs(pTargetPuppet->m_State.fMovementUrgency) : 0.0f;

  dc->TextToScreen(0, 56, "DesiredSpd (urgency): %5.3f (%5.3f) Dir: (%5.3f, %5.3f, %5.3f)",
    pTargetPuppet->m_State.fDesiredSpeed, urgency,
    pTargetPuppet->m_State.vMoveDir.x, pTargetPuppet->m_State.vMoveDir.y, pTargetPuppet->m_State.vMoveDir.z);

	dc->TextToScreen(0, 58, "Turn speed: %-5.3f rad/s  Slope:%-5.3f", pTargetPuppet->m_bodyTurningSpeed, bodyInfo.slopeAngle);

/*	if (pTargetPuppet->GetPathFollower())
	{
		CPathFollower* pathFollower = pTargetPuppet->GetPathFollower();

		float normalSpeed = pTargetPuppet->GetNormalMovementSpeed(pTargetPuppet->m_State.fMovementUrgency, true);
		float slowSpeed = pTargetPuppet->GetManeuverMovementSpeed();

		float minSpeed = 0, maxSpeed = 0;
	
		IPuppetProxy* pPuppetProxy =  0;
		if (pTargetPuppet->GetProxy())
			pTargetPuppet->GetProxy()->QueryProxy(AIPROXY_PUPPET, (void**)&pPuppetProxy);

		bool useAnimSpeed = true;
		if (!pPuppetProxy || !pPuppetProxy->QueryCurrentAnimationSpeedRange(minSpeed, maxSpeed))
		{
			minSpeed = normalSpeed * 0.75f;
			maxSpeed = normalSpeed * 1.1f;
			useAnimSpeed = false;
		}

		normalSpeed = Clamp(normalSpeed, minSpeed, maxSpeed);

		dc->TextToScreen(11, 58, "Speeds: %5.3f (min:%5.3f  max:%5.3f) %s", normalSpeed, minSpeed, maxSpeed, useAnimSpeed ? "ANIM" : "PROC");
	}*/

/*
{
	char *pWhatIsMyTargetStr("NONE");
//	char	strBuffer[64];
	int	targetFlags(pTargetPuppet->GetTargetFlags());
	
	if(targetFlags & AITGT_VISUAL )
		pWhatIsMyTargetStr = targetFlags & AITGT_THREAT ? "VISUAL_THREAT" : "VISUAL_INTERESTING";
	else if(targetFlags & AITGT_AUDIO )
		pWhatIsMyTargetStr = targetFlags & AITGT_THREAT ? "AUDIO_THREAT" : "AUDIO_INTERESTING";
	else if(targetFlags & AITGT_MEMORY )
		pWhatIsMyTargetStr = targetFlags & AITGT_THREAT ? "MEMORY_THREAT" : "MEMORY_INTERESTING";
	else if(targetFlags & AITGT_FORCED )
		pWhatIsMyTargetStr = targetFlags & AITGT_THREAT ? "FORCED_THREAT" : "FORCED_INTERESTING";
	else if(targetFlags & AITGT_DUMMY )
		pWhatIsMyTargetStr = "_DUMMY_";
	else
		pWhatIsMyTargetStr = "_NONE_";
	dc->TextToScreen(0, 49, pWhatIsMyTargetStr);
}
*/


	CGoalPipe *pPipe = pTargetPuppet->GetCurrentGoalPipe();
	if (pPipe)
	{
		dc->TextToScreen(0, 62, "Goalpipe: %s", pTargetPuppet->GetCurrentGoalPipe()->GetName());

		const char* szGoalopName = "--";
		if (pTargetPuppet->m_lastExecutedGoalop != eGO_LAST)
			szGoalopName = pPipe->GetGoalOpName(pTargetPuppet->m_lastExecutedGoalop);
		dc->TextToScreen(0, 64, "Current goal: %s", szGoalopName);

		int i=0;
		CGoalPipe *pSubPipe = pPipe;
		while (pSubPipe->IsInSubpipe())
		{
			pSubPipe = pSubPipe->GetSubpipe();
			char str[1024];
			memset(str,32,sizeof(char)*1024);
			str[0] = '+';
			strcpy(&str[1], pSubPipe->GetName());
			dc->TextToScreen(0.f, 66.f + 2.f * i, str);
			i++;
		}
	}

	DebugDrawPendingEvents(pTargetPuppet, 50, 20);

	if (!pTargetPuppet->m_State.vLookTargetPos.IsZero()) 
		dc->TextToScreen(0, 70, "LOOK");
	if (!pTargetPuppet->m_State.vAimTargetPos.IsZero()) 
		dc->TextToScreen(10, 70, "AIMLOOK");
	
	PotentialTargetMap targetMap;
	pTargetPuppet->GetPotentialTargets(targetMap);

	float maxExposure = 0.0f;
	float maxThreat = 0.0f;
	PotentialTargetMap::const_iterator ei = targetMap.begin();
	for (;ei != targetMap.end(); ++ei)
	{
		const SAIPotentialTarget& ed = ei->second;
		maxExposure = max(maxExposure, ed.exposure);
		maxThreat = max(maxThreat, ed.threatTime);
	}


	IAIObject* pAttentionTarget = pTargetPuppet->GetAttentionTarget();
	dc->TextToScreen(0, 74, "Attention target: %s", pAttentionTarget ? pAttentionTarget->GetName() : "<no target>");
	dc->TextToScreen(0, 76, "(maxThreat:%.3f  maxExposure:%.3f) alert: %d  %s",
		maxThreat,
		maxExposure,
		pTargetPuppet->GetProxy()->GetAlertnessState(),
		pTargetPuppet->IsAlarmed() ? "Alarmed" : "");

	if (!pTargetPuppet->m_State.vSignals.empty())
	{
		int i=0;

		dc->TextToScreen(0, 78, "Pending signals:");
		DynArray<AISIGNAL>::iterator sig,iend=pTargetPuppet->m_State.vSignals.end();
		for (sig=pTargetPuppet->m_State.vSignals.begin();sig!=iend;++sig,i++)
			dc->TextToScreen(0.f, 80.f + 2.f * i, (*sig).strText);
	}

	dc->TextToScreen(50, 62, "GR.MEMBERS:%d GROUPID:%d", m_mapGroups.count(pTargetPuppet->GetGroupId()),pTargetPuppet->GetGroupId());

	if (pTargetPuppet->GetProxy())
		pTargetPuppet->GetProxy()->DebugDraw(1);

/*
	if(pTargetObject->m_State.fire)
		dc->DrawLine(pTargetObject->GetPos(), ColorB(255, 51, 26), pTargetObject->GetPos()+pTargetObject->GetViewDir()*125.0f, ColorB(51, 51, 255));
	else
		dc->DrawLine(pTargetObject->GetPos(), ColorB(51, 51, 255), pTargetObject->GetPos()+pTargetObject->GetViewDir()*125.0f, ColorB(51, 51, 255));
	dc->DrawLine(pTargetObject->GetPos(), ColorB(26, 255, 26), pTargetObject->GetPos()+pTargetObject->GetMoveDir()*125.0f, ColorB(51, 51, 255));

//	dc->DrawLine(pTargetObject->GetPos(), ColorB(26, 255, 26), pTargetObject->GetPos()+pTargetObject->m_State.vLookDir*125.0f, ColorB(51, 51, 255));
*/

	const ColorB desiredColor(255, 204, 13, 77);
	const ColorB actualColor(26, 51, 204, 128);
	const ColorB fireColor(255, 26, 0);

	const Vec3&	pos = pTargetObject->GetPos();
/*
	const float	rad = pTargetPuppet->m_Parameters.m_fPassRadius;
	const float	sightRange = pTargetPuppet->m_Parameters.m_PerceptionParams.sightRange;

	// Extend the direction vectors.

	// The desired movement direction.
	if( pTargetObject->m_State.vMoveDir.GetLengthSquared() > 0.0f )
	{
		const Vec3&	dir = pTargetObject->m_State.vMoveDir;
		const Vec3	start = pos + dir * (rad + 1.0f);
		const Vec3	target = pos + dir * sightRange;
		dc->DrawLine(start, desiredColor * 0.5f, target, desiredColor * 0.5f);
	}
	// The desired lookat direction.
	{
		const Vec3&	dir = pTargetObject->m_State.vLookDir;
		const Vec3	start = pos + dir * (rad + 2.0f);
		const Vec3	target = pos + dir * (rad + 2.0f);
		dc->DrawLine(start, desiredColor * 0.5f, target, desiredColor * 0.5f);
	}
*/
	// The actual movement direction
/*	{
		const Vec3&	dir = pTargetObject->GetMoveDir();
		const Vec3	start = pos + dir * (rad + 1.3f);
		const Vec3	target = pos + dir * sightRange;
		dc->DrawLine(start, actualColor * 0.5f, target, actualColor * 0.5f);
	}*/

	// The actual lookat direction
	{
/*		const Vec3	start = pos;
		const Vec3	target = pTargetObject->m_State.vShootTargetPos;

		// Indicate firing.
		if( pTargetObject->m_State.fire )
		{
//			const Vec3	target = pos + dir * (rad + 1.5f);
//			dc->DrawCone(target, dir, 0.2f, 1.0f, fireColor);
//			dc->DrawCone(target + dir * 0.9f, dir, 0.2f, 1.0f, fireColor);
			dc->DrawLine(start, fireColor, target, fireColor);
		}
		else
		{
			if( pTargetPuppet->AllowedToFire() )
			{
//				const Vec3	target = pos + dir * (rad + 1.5f);
//				dc->DrawCone(target, dir, 0.2f, 1.0f, fireCol * 0.5f);
//				dc->DrawCone(target, dir, 0.07f, 0.2f, fireCol);
			}
//			dc->DrawLine(start, fireColor * 0.5f, target, fireColor * 0.5f);
			dc->DrawLine(start, actualColor * 0.5f, target, actualColor * 0.5f);
		}*/


//	const ColorB queueFireColor(102, 255, 102);
//for(int dbgIdx(0);dbgIdx<curLimit; ++dbgIdx)
//	dc->DrawLine(s_shots[dbgIdx].src, fireColor, s_shots[dbgIdx].dst, fireColor);

//		dc->DrawSphere(pTargetPuppet->m_LastMissPoint, .25, ColorB(255, 0, 255));
	}

	// Draw fire command handler stuff.
	if(pTargetPuppet->m_pFireCmdHandler)
		pTargetPuppet->m_pFireCmdHandler->DebugDraw();
	if (pTargetPuppet->m_pFireCmdGrenade)
		pTargetPuppet->m_pFireCmdGrenade->DebugDraw();

	// draw hide point
//	if(pTargetPuppet
//	if(pPipeUser->m_bAllowedToFire)
//		pRC->DrawCone(posSelf+Vec3(0.f,0.f,.7f), Vec3(0,0,-1), .15f, .5f,ColorB(255,20,20));
//	if(pAIObj->m_State.fire)
	if(pTargetPuppet->m_CurrentHideObject.IsValid())
	{
		dc->DrawCone(pTargetPuppet->m_CurrentHideObject.GetObjectPos()+Vec3(0,0,5), Vec3(0,0,-1), .2f, 5.f,
			pTargetPuppet->AllowedToFire() ? ColorB(255, 26, 26) : ColorB(255, 255, 26));
	}

  // draw predicted info
  for (int iPred = 0 ; iPred < pTargetPuppet->m_State.predictedCharacterStates.nStates ; ++iPred)
  {
    const SAIPredictedCharacterState &predState = pTargetPuppet->m_State.predictedCharacterStates.states[iPred];
    ColorB posColor(255, 255, 255);
    ColorB velColor(  0, 255, 179);

    dc->DrawSphere(predState.position, 0.1f, posColor);

    Vec3 velOffset(0.0f, 0.0f, 0.1f);
    dc->DrawSphere(predState.position + velOffset, 0.1f, velColor);
    dc->DrawLine(predState.position + velOffset, posColor,
      predState.position + velOffset + predState.velocity * 0.2f, posColor);
    static string speedTxt;
    speedTxt.Format("%5.2f", predState.velocity.GetLength());
    dc->Draw3dLabel(predState.position + 4 * velOffset, 1.0f, speedTxt.c_str());
  }

	// Track and draw the trajectory of the agent.
  Vec3 physicsPos = pTargetObject->GetPhysicsPos();
	if( gAIEnv.CVars.DrawTrajectory )
	{
		int type = gAIEnv.CVars.DrawTrajectory;
		static int lastReason = -1;
		bool	updated = false;

		int	reason = 0;
		if (type == 1)
			reason = pTargetPuppet->m_DEBUGmovementReason;
		else if (type ==2)
			reason = bodyInfo.stance;

		if( !m_lastStatsTargetTrajectoryPoint.IsZero() )
		{
			Vec3	delta = m_lastStatsTargetTrajectoryPoint - physicsPos;
			if( delta.len2() > sqr( 0.1f ) )
			{
				Vec3	prevDelta( delta );
				if( !m_lstStatsTargetTrajectory.empty() )
					prevDelta = m_lstStatsTargetTrajectory.back().end - m_lstStatsTargetTrajectory.back().start;

				float	c = delta.GetNormalizedSafe().Dot( prevDelta.GetNormalizedSafe() );

				if( lastReason != reason || c < cosf(DEG2RAD(15.0f)) || Distance::Point_Point( m_lastStatsTargetTrajectoryPoint, physicsPos ) > 2.0f )
				{
					ColorB	color;
					if (type == 1)
					{
						// Color the path based on movement reason.
						switch( pTargetPuppet->m_DEBUGmovementReason )
						{
						case CPipeUser::AIMORE_UNKNOWN:			color.Set(171, 168, 166); break;
						case CPipeUser::AIMORE_TRACE:				color.Set( 95, 182, 223); break;
						case CPipeUser::AIMORE_MOVE:				color.Set( 49, 110, 138); break;
						case CPipeUser::AIMORE_MANEUVER:		color.Set( 85, 191,  48); break;
						case CPipeUser::AIMORE_SMARTOBJECT:	color.Set(240, 169,  16); break;
						}
					}
					else if (type == 2)
					{
						// Color the path based on stance.
						switch( bodyInfo.stance )
						{
						case STANCE_NULL:					color.Set(171, 168, 166);	break;
						case STANCE_PRONE:				color.Set( 85, 191,  48);	break;
						case STANCE_STAND:				color.Set( 95, 182, 223);	break;
						case STANCE_STEALTH:			color.Set( 49, 110, 138);	break;
						case STANCE_LOW_COVER:		color.Set( 32, 98, 108);	break;
						case STANCE_HIGH_COVER:		color.Set( 32, 98, 108);	break;
						}
					}
          else if (type == 3)
          {
            float sp = pTargetPuppet->m_State.fDesiredSpeed;
            float ur = pTargetPuppet->m_State.fMovementUrgency;
            if (sp <= 0.0f)
              color.Set(0, 0, 255);
            else if (ur <= 1.0f)
              color.Set(0, sp * 255, 0);
            else
              color.Set((sp - 1.0f) * 255, 0, 0);
          }
					m_lstStatsTargetTrajectory.push_back(SDebugLine(m_lastStatsTargetTrajectoryPoint, physicsPos, color, 0));
					m_lastStatsTargetTrajectoryPoint = physicsPos;

					lastReason = reason;
				}
			}

			if( !updated && !m_lstStatsTargetTrajectory.empty() )
				m_lstStatsTargetTrajectory.back().end = physicsPos;
		}
		else
		{
			m_lastStatsTargetTrajectoryPoint = physicsPos;
		}

		for( std::list<SDebugLine>::iterator lineIt = m_lstStatsTargetTrajectory.begin(); lineIt != m_lstStatsTargetTrajectory.end(); ++lineIt )
		{
			SDebugLine&	line = (*lineIt);
			dc->DrawLine(line.start, line.color, line.end, line.color);
		}

		if( m_lstStatsTargetTrajectory.size() > 600 )
			m_lstStatsTargetTrajectory.pop_front();
	}

/*
	// Draw ranges relative to the player
	{
		const ColorB targetNoneColor				( 0,   0, 0, 128);
		const ColorB targetInterestingColor	(128, 255, 0, 196);
		const ColorB targetThreateningColor	(255, 210, 0, 196);
		const ColorB targetAggressiveColor	(255,  64, 0, 196);

		CDebugDrawContext dc1;
		dc1->SetAlphaBlending(true);
		dc1->SetDepthWrite(false);

		CAIActor* pEventOwner = GetPlayer()->CastToCAIActor();
		if (!pEventOwner)
			return;

		float distToTarget = Distance::Point_Point(pTargetPuppet->GetPos(), pEventOwner->GetPos());
		float sightRange = pTargetPuppet->GetSightRange(pEventOwner);
		float sightRangeThr = sightRange * (0.2f + pTargetPuppet->GetPerceptionAlarmLevel() * 0.3f);

		float sightRangeScale = 1.0f;
		float sightScale = 1.0f;

		// Target under water
		float waterOcclucion = GetAISystem()->GetWaterOcclusionValue(pEventOwner->GetPos());
		sightRangeScale *= 0.1f + (1 - waterOcclucion) * 0.9f;

		// Target stance
		float stanceSize = 1.0f;
		SAIBodyInfo bi;
		if (pEventOwner->GetProxy())
			pEventOwner->GetProxy()->QueryBodyInfo(bi);
		float targetHeight = bi.m_stanceSize.GetSize().z;
		if (pEventOwner->GetType() == AIOBJECT_VEHICLE)
			targetHeight = 0.0f;
		if (targetHeight > 0.0f)
			stanceSize = targetHeight / pTargetPuppet->m_Parameters.m_PerceptionParams.stanceScale;
		sightRangeScale *= 0.25f + stanceSize * 0.75f;

		//use target scale factor
		if (pTargetPuppet->m_Parameters.m_PerceptionParams.bThermalVision)
			sightRangeScale *= pEventOwner->m_Parameters.m_PerceptionParams.heatScale;
		else
			sightRangeScale *= pEventOwner->m_Parameters.m_PerceptionParams.camoScale;

		// Secondly calculate factors that will affect the actual sighting value.

		// Scale the sighting based on the agent FOV.
		if (pTargetPuppet->m_FOVPrimaryCos > 0.0f)
		{
			Vec3 dirToTarget = pEventOwner->GetPos() - pTargetPuppet->GetPos();
			Vec3 viewDir = pTargetPuppet->GetViewDir();
			if (!pTargetPuppet->IsUsing3DNavigation())
			{
				dirToTarget.z = 0;
				viewDir.z = 0;
			}
			dirToTarget.Normalize();
			viewDir.Normalize();

			float dot = viewDir.Dot(dirToTarget);
			float fovFade = 1.0f;
			if (dot < pTargetPuppet->m_FOVPrimaryCos)
				fovFade = LinStep(pTargetPuppet->m_FOVSecondaryCos, pTargetPuppet->m_FOVPrimaryCos, dot);

			// When the target is really close, reduce the effect of the FOV.
			if (distToTarget < pTargetPuppet->m_fRadius * 2)
				fovFade = max(fovFade, LinStep(pTargetPuppet->m_fRadius * 2, pTargetPuppet->m_fRadius, distToTarget));

			sightScale *= fovFade;
		}

		// Target cloak
		if (!pEventOwner->IsInvisibleFrom(pTargetPuppet->GetPos()))
		{
			if (pEventOwner->m_Parameters.m_fCloakScale > 0.5f && !pEventOwner->GetGrabbedEntity())
			{
				// Cloak on and not carrying anything
				// Fade out the perception increment based on the cloak distance parameters.
				const float cloakMinDist = gAIEnv.CVars.CloakMinDist;
				const float cloakMaxDist = gAIEnv.CVars.CloakMaxDist;
				sightScale *= LinStep(cloakMaxDist, cloakMinDist, distToTarget);
			}
		}

		sightRange *= sightRangeScale;
		sightRangeThr *= sightRangeScale;

		float visualThreatLevel = 0.0f;
		if (sightRange > 0.0f)
			visualThreatLevel = LinStep(sightRange, sightRangeThr, distToTarget) * sightScale;

		Vec3 dir = pEventOwner->GetPos() - pTargetPuppet->GetPos();
		dir.z = 0;
		dir.Normalize();
		const Vec3 up(0,0,1);
		const Vec3 pos = pTargetPuppet->GetPos();
		const float h = 2.0f;

		dc1->DrawLine(pos, ColorB(255, 255, 255), pos + dir*sightRange, ColorB(255, 255, 255));
		dc1->DrawLine(pos, ColorB(255, 255, 255), pos + up*h, ColorB(255, 255, 255));
		dc1->DrawLine(pos+up*h, ColorB(255, 255, 255), pos + up*h + dir*sightRangeThr, ColorB(255, 255, 255));
		dc1->DrawLine(pos + dir*sightRange, ColorB(255, 255, 255), pos + up*h + dir*sightRangeThr, ColorB(255, 255, 255));

		float d = 0;
		// Interested
		d = sightRange + (sightRangeThr - sightRange) * 0.3f;
		dc1->DrawLine(pos + dir*d, targetInterestingColor, pos + up*h + dir*d, targetInterestingColor);

		// Threatened
		d = sightRange + (sightRangeThr - sightRange) * 0.6f;
		dc->DrawLine(pos + dir*d, targetThreateningColor, pos + up*h + dir*d, targetThreateningColor);

		// Aggressive
		d = sightRange + (sightRangeThr - sightRange) * 0.9f;
		dc1->DrawLine(pos + dir*d, targetAggressiveColor, pos + up*h + dir*d, targetAggressiveColor);
	}
*/


	const ColorB targetNoneColor				( 0,   0, 0, 128);
	const ColorB targetInterestingColor	(128, 255, 0, 196);
	const ColorB targetThreateningColor	(255, 210, 0, 196);
	const ColorB targetAggressiveColor	(255,  64, 0, 196);

	CDebugDrawContext dc2;
	dc2->SetAlphaBlended(true);
	dc2->SetDepthWrite(false);

	CCamera& cam = GetISystem()->GetViewCamera();
	Vec3 axisx = cam.GetMatrix().TransformVector(Vec3(1,0,0));
	Vec3 axisy = cam.GetMatrix().TransformVector(Vec3(0,0,1));

	// Draw perception events
	for (PotentialTargetMap::iterator ei2 = targetMap.begin(), end = targetMap.end(); ei2 != end; ++ei2)
	{
		SAIPotentialTarget& ed = ei2->second;
		CAIObject* pOwner = ei2->first.GetAIObject();

		ColorB color;
		switch(ed.threat)
		{
		case AITHREAT_INTERESTING:	color = targetInterestingColor;	break;
		case AITHREAT_THREATENING:	color = targetThreateningColor;	break;
		case AITHREAT_AGGRESSIVE:		color = targetAggressiveColor;	break;
		default: color = targetNoneColor;
		}

		Vec3 pos2(0,0,0);
		const char* szType = "NONE";
		CAIObject * const pDummyRep = ed.refDummyRepresentation.GetAIObject();
		switch (ed.type)
		{
		case AITARGET_VISUAL:
			szType = "VISUAL";
			{
				ColorB vcolor(255, 255, 255);
				if (ed.threat == AITHREAT_AGGRESSIVE)
				{
					pos2 = pOwner->GetPos();
				}
				else
				{
					pos2 = pDummyRep->GetPos();
					vcolor.a = 64;
				}
				dc2->DrawLine(pTargetPuppet->GetPos(), vcolor, pos2, vcolor);
			}
			break;
		case AITARGET_BEACON:
			szType = "BEACON";
			{
				ColorB vcol(255,255,255);
				pos2 = pOwner->GetPos();
				dc2->DrawLine(pTargetPuppet->GetPos(), vcol, pos2, vcol);
			}
			break;
		case AITARGET_MEMORY:
			szType = "MEMORY";
			pos2 = pDummyRep->GetPos();
			{
				ColorB vcolor(0,0,0,128);
				dc2->DrawLine(pTargetPuppet->GetPos(), vcolor, pos2, vcolor);
			}
			break;
		case AITARGET_SOUND:
			szType = "SOUND";
			pos2 =  pDummyRep->GetPos();
			break;
		default:
			if (ed.visualThreatLevel > ed.soundThreatLevel)
				pos2 = ed.visualPos;
			else
				pos2 = ed.soundPos;
		};

		dc2->DrawSphere(pos2, 0.25f, color);

		const char* szVisType = "";
		switch (ed.visualType)
		{
		case SAIPotentialTarget::VIS_VISIBLE: szVisType = "vis"; break;
		case SAIPotentialTarget::VIS_MEMORY: szVisType = "mem"; break;
		};

		const char* szOwner = pOwner->GetName();

		dc2->Draw3dLabel(pos2, 1.0f, "@%s\n%s\nt=%.1fs/%.1fs\nexp=%.3f\nsound=%.3f\nsight=%.3f\n%s", 
			szOwner, szType, ed.threatTimeout, ed.threatTime, ed.exposure, ed.soundThreatLevel, ed.visualThreatLevel, szVisType);
	}
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawBehaviorStatsTarget(const char* pName)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if(!pName )
		return;

	CAIObject *pTargetObject = GetAIObjectByName(pName);
	if (!pTargetObject)
		return;

	CPuppet* pTargetPuppet = pTargetObject->CastToCPuppet();
	if (!pTargetPuppet)
		return;

	CAIActor* pActor = pTargetPuppet->CastToCAIActor();
	if (pActor)
	{
		const TBSSProfileUserId userId = pActor->GetBehaviorTreeUserId();
		SBTProfileUser *pUser = m_pBTProfileDictionary->GetProfileUser(userId);
		IPersonalBehaviorTree *pPersonalBT = (pUser ? pUser->m_pPersonalTree : NULL);
		if (pPersonalBT)
		{
			pPersonalBT->DebugDraw();
		}
	}
}


//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawBehaviorSelection(const char* agentName)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if(!agentName || !*agentName)
		return;

	CAIObject *targetObject = GetAIObjectByName(agentName);
	if (!targetObject)
		return;

	CAIActor* targetActor = targetObject->CastToCAIActor();
	if (targetActor)
		targetActor->DebugDrawBehaviorSelectionTree();
}


//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawGroupSystem(int nMode)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if( nMode == 1 )
		gAIEnv.pGroupSystem->UpdateDraw();
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawCooperativeReadabilities(const char* pName) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if (strcmp(pName, "none"))
	{
		IEntity* pEntity = gEnv->pEntitySystem->FindEntityByName( pName );
		if (pEntity)
			GetCooperativeReadabilitiesSystem()->UpdateDraw(pEntity->GetId());
	}
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawFormations() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	for(FormationMap::const_iterator itForm=m_mapActiveFormations.begin(); itForm!= m_mapActiveFormations.end();++itForm)
		if(itForm->second)
			(itForm->second)->Draw();
}

//====================================================================
// DebugDrawTaggedNodes
//====================================================================
void CAISystem::DebugDrawTaggedNodes() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CDebugDrawContext dc;
	ColorB color(255, 255, 255);

	const AStarSearchNodeVector& taggedNodes = m_pPathfinder->GetTaggedNodesVector();
	for (AStarSearchNodeVector::const_iterator it = taggedNodes.begin() ; it != taggedNodes.end() ; ++it)
	{
		const AStarSearchNode* node = *it;
		if (node->graphNode->navType == IAISystem::NAV_TRIANGULAR && node->graphNode->GetTriangularNavData()->vertices.size() >= 3)
		{
			Vec3 one,two,three;
			one = m_VertexList.GetVertex(node->graphNode->GetTriangularNavData()->vertices[0]).vPos;
			two = m_VertexList.GetVertex(node->graphNode->GetTriangularNavData()->vertices[1]).vPos;
			three = m_VertexList.GetVertex(node->graphNode->GetTriangularNavData()->vertices[2]).vPos;
      one.z = dc->GetDebugDrawZ(one, true);
      two.z = dc->GetDebugDrawZ(two, true);
      three.z = dc->GetDebugDrawZ(three, true);

			dc->DrawLine(one, color, two, color);
			dc->DrawLine(two, color, three, color);
			dc->DrawLine(three, color, one, color);
		}
		Vec3 nodePos = node->graphNode->GetPos();
		nodePos.z = dc->GetDebugDrawZ(nodePos, node->graphNode->navType == IAISystem::NAV_TRIANGULAR);
		dc->Draw3dLabel(nodePos,1,"%.2f", node->fCostFromStart);
	}
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawNode() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	const char *pName=gAIEnv.CVars.DrawNode;
	if(!pName )
		return;
		
	if(!strcmp(pName, "none"))
		return;
		
	CAIObject *pTargetObject = GetAIObjectByName(pName);
	if (pTargetObject)
	{
		DebugDrawNodeSingle(pTargetObject);
		return;
	}

	if(!strcmp(pName, "player"))
	{
		DebugDrawNodeSingle(GetPlayer());
		return;
	}

	if(strcmp(pName, "all"))
		return;

	DebugDrawNodeSingle(GetPlayer());
	// find if there are any puppets
	AIObjectOwners::const_iterator ai;
	if ((ai = m_Objects.find(AIOBJECT_PUPPET)) != m_Objects.end())
  {
		for (;ai!=m_Objects.end(); ++ai)
		{
			if (ai->first != AIOBJECT_PUPPET)
				break;
			DebugDrawNodeSingle(ai->second.GetAIObject());
		}

    m_pNavigation->DebugDrawForbidden();
  }
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawNodeSingle(CAIObject* pTargetObject) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if (!pTargetObject)
		return;

	CGraph *pGraph = m_pGraph;

	ColorB color(255, 255, 255);

  Vec3 ppos = pTargetObject->GetPhysicsPos();
	unsigned int playerNodeIndex = pGraph->GetEnclosing(ppos, IAISystem::NAVMASK_SURFACE);
	GraphNode *pPlayerNode = pGraph->GetNodeManager().GetNode(playerNodeIndex);
//	AIAssert(pPlayerNode);
	if (!pPlayerNode)
		return;

	CDebugDrawContext dc;

	int debugTextType = gAIEnv.CVars.DrawNodeLinkType;

	bool	bHasPassibleConnections = false;
	for (unsigned int link = pPlayerNode->firstLinkIndex; link; link = pGraph->GetLinkManager().GetNextLink(link))
	{
		unsigned int connectedIndex = pGraph->GetLinkManager().GetNextNode(link);
		GraphNode *pConnectedNode=pGraph->GetNodeManager().GetNode(connectedIndex);
		if (!pConnectedNode)
			continue;
		if (pPlayerNode->navType == IAISystem::NAV_TRIANGULAR)
		{
			float value;
			switch (debugTextType)
			{
			case 0: value = pGraph->GetLinkManager().GetRadius(link); break;
			case 1: value = pGraph->GetLinkManager().GetExposure(link); break;
			case 2: value = pGraph->GetLinkManager().GetMaxWaterDepth(link); break;
			case 3: value = pGraph->GetLinkManager().GetMinWaterDepth(link); break;
			default: value = -123456.0f; break;
			}
			float cutoff = gAIEnv.CVars.DrawNodeLinkCutoff;
//			float fColor[4] = {value <= cutoff, value > cutoff, 0 , 1};
			dc->Draw3dLabelEx(pGraph->GetLinkManager().GetEdgeCenter(link), 1.0f, color, true, false, "%.2f", value);
		}
		if (pGraph->GetLinkManager().GetRadius(link)<0)
		{
			color.Set(0, 255, 0);
		}
		else
		{
			color.Set(255, 255, 255);
			bHasPassibleConnections = true;
		}
    Vec3 nodePos = pPlayerNode->GetPos();
    Vec3 otherNodePos = pConnectedNode->GetPos();
    Vec3 edgePos = pGraph->GetLinkManager().GetEdgeCenter(link);
    edgePos.z = dc->GetDebugDrawZ(edgePos, pPlayerNode->navType == IAISystem::NAV_TRIANGULAR);
    nodePos.z = dc->GetDebugDrawZ(nodePos, pPlayerNode->navType == IAISystem::NAV_TRIANGULAR);
    otherNodePos.z = dc->GetDebugDrawZ(otherNodePos, pPlayerNode->navType == IAISystem::NAV_TRIANGULAR);
		dc->DrawLine(nodePos, color, edgePos, color);
		dc->DrawLine(edgePos, color, otherNodePos, color);
	} //vi
	color.Set(255, 255, 0);
	if(!bHasPassibleConnections)
		color.Set(255, 0, 0);
  const float centerRadius = 0.05f;
  Vec3 nodePos = pPlayerNode->GetPos();
  nodePos.z = dc->GetDebugDrawZ(nodePos, pPlayerNode->navType == IAISystem::NAV_TRIANGULAR);
	dc->DrawSphere(nodePos, centerRadius, color);

	if (pPlayerNode->navType == IAISystem::NAV_TRIANGULAR)
	{
		GraphNode *pCurrentNode =pPlayerNode;
		// draw the triangle
		for (unsigned int link = pCurrentNode->firstLinkIndex; link; link = pGraph->GetLinkManager().GetNextLink(link))
		{
			Vec3 start = m_VertexList.GetVertex(pCurrentNode->GetTriangularNavData()->vertices[pGraph->GetLinkManager().GetStartIndex(link)]).vPos;
			Vec3 end = m_VertexList.GetVertex(pCurrentNode->GetTriangularNavData()->vertices[pGraph->GetLinkManager().GetEndIndex(link)]).vPos;
			Vec3 horDelta = end - start;
			horDelta.z = 0.0f;
			float horDist = horDelta.GetLength();
			if (pGraph == m_pGraph && pGraph->GetLinkManager().GetRadius(link) > 0.0f && horDist > 0.0001f)
			{
				Vec3 mid = pGraph->GetLinkManager().GetEdgeCenter(link);
				mid.z = start.z;
				Vec3 horDir = horDelta / horDist;
				Vec3 startSafe = mid - pGraph->GetLinkManager().GetRadius(link) * horDir;
				Vec3 endSafe = mid + pGraph->GetLinkManager().GetRadius(link) * horDir;

				// now set the z values
				float distToStartSafe = (startSafe - start).GetLength();
				float distToEndSafe = (endSafe - start).GetLength();
				startSafe.z = start.z + (end.z - start.z) * distToStartSafe / horDist;
				endSafe.z = start.z + (end.z - start.z) * distToEndSafe / horDist;

				const float criticalRadius = 0.4f;
				color.Set(255, 0, 0);
        start.z = dc->GetDebugDrawZ(start, true);
        startSafe.z = dc->GetDebugDrawZ(startSafe, true);
        endSafe.z = dc->GetDebugDrawZ(endSafe, true);
        end.z = dc->GetDebugDrawZ(end, true);
				dc->DrawLine(start, color, startSafe, color);
				dc->DrawLine(endSafe, color, end, color);
				if (pGraph->GetLinkManager().GetRadius(link) > criticalRadius)
					color.Set(255, 255, 0);
				dc->DrawLine(startSafe, color, endSafe, color);
			}
			else
			{
				color.Set(255, 0, 0);
        start.z = dc->GetDebugDrawZ(start, true);
        end.z = dc->GetDebugDrawZ(end, true);
				dc->DrawLine(start, color, end, color);
			}
		}
		ObstacleIndexVector::iterator oi;
		for (oi=pCurrentNode->GetTriangularNavData()->vertices.begin();oi!=pCurrentNode->GetTriangularNavData()->vertices.end();oi++)
		{
      const ObstacleData &ob = m_VertexList.GetVertex(*oi);
      if (ob.IsCollidable())
        color.Set(255, 128, 128);
      else
        color.Set(128, 255, 128);
			dc->DrawSphere(ob.vPos, 0.05f, color);
      if (ob.fApproxRadius > 0.0f)
        dc->DrawCircles(ob.vPos, ob.fApproxRadius, ob.fApproxRadius, 1, Vec3(1, 1, 1), Vec3(1, 1, 1));
		}
	}
}


//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawType() const 
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	int type=gAIEnv.CVars.DrawType;

	AIObjectOwners::const_iterator ai;
	if ((ai = m_Objects.find(type)) != m_Objects.end())
		for (;ai!=m_Objects.end(); ++ ai)
		{
			if (ai->first != type)
				break;
			DebugDrawTypeSingle(ai->second.GetAIObject());
		}
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawTypeSingle(CAIObject* pAIObj) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	Vec3 pos = pAIObj->GetPhysicsPos();
	ColorB color(230, 230, 26);
	CDebugDrawContext dc;
	dc->DrawLine(pos, color, pos + pAIObj->GetMoveDir()*2.0f, color);
	if(pAIObj->IsEnabled())
		color.Set(0, 0, 179);
	else
		color.Set(255, 77, 51);
	dc->DrawSphere(pos, .3f, color);
	dc->Draw3dLabel(pos, 1, "%s", pAIObj->GetName());
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawTargetsList() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	float drawDist2 = static_cast<float>(gAIEnv.CVars.DrawTargets);
	drawDist2*=drawDist2;
	int column(1), row(1);
	string eventDescr;
	string atTarget;
	AIObjectOwners::const_iterator ai;
	CDebugDrawContext dc;
	if ((ai = m_Objects.find(AIOBJECT_PUPPET)) != m_Objects.end())
		for (;ai!=m_Objects.end(); ++ai)
		{
			if (ai->first != AIOBJECT_PUPPET)
				break;
			if (!ai->second.GetAIObject()->IsEnabled())
				continue;
			eventDescr = "--";
			atTarget = "--";
			float dist2 = (dc->GetCameraPos() - (ai->second.GetAIObject())->GetPos()).GetLengthSquared();
			if(dist2>drawDist2)
				continue;
			CPuppet *pTargetPuppet = ai->second.GetAIObject()->CastToCPuppet();
			if (!pTargetPuppet)
				continue;
			DebugDrawTargetUnit(pTargetPuppet);
			float fMaxPriority = -1.f;
			const SAIPotentialTarget* maxEvent = 0;
			CAIObject *pNextTarget = 0;

			PotentialTargetMap targetMap;
			pTargetPuppet->GetPotentialTargets(targetMap);

			PotentialTargetMap::iterator ei = targetMap.begin(), eiend = targetMap.end();
			for (; ei != eiend; ++ei)
			{
				const SAIPotentialTarget &ed = ei->second;
				// target selection based on priority
				if (ed.priority>fMaxPriority)
				{
					fMaxPriority = ed.priority;
					maxEvent = &ed;

					if ((ed.type == AITARGET_VISUAL && ed.threat == AITHREAT_AGGRESSIVE) || ed.refDummyRepresentation.IsNil())
						pNextTarget = ei->first.GetAIObject();
					else
						pNextTarget = ed.refDummyRepresentation.GetAIObject();
				}
			}

			if (fMaxPriority >= 0.0f && maxEvent)
			{
				const char* szName = "NULL";
				if (pNextTarget)
					szName = pNextTarget->GetName();

				const char* szTargetType = "";
				const char* szTargetThreat = "";

				switch (maxEvent->type)
				{
				case AITARGET_VISUAL: szTargetType = "VIS"; break;
				case AITARGET_MEMORY: szTargetType = "MEM"; break;
				case AITARGET_SOUND: szTargetType = "SND"; break;
				default: szTargetType = "-  "; break;
				}

				switch (maxEvent->threat)
				{
				case AITHREAT_AGGRESSIVE: szTargetThreat = "AGG"; break;
				case AITHREAT_THREATENING: szTargetThreat = "THR"; break;
				case AITHREAT_INTERESTING: szTargetThreat = "INT"; break;
				default: szTargetThreat = "-  "; break;
				}

				float timeout = maxEvent->GetTimeout(pTargetPuppet->GetParameters().m_PerceptionParams);

				char bfr[256];
				sprintf( bfr, "%.3f %.1f  <%s %s>  %s", fMaxPriority, timeout, szTargetType, szTargetThreat, szName);
				eventDescr = bfr;
			}
			CPipeUser* pPipeUser = ai->second.GetAIObject()->CastToCPipeUser();
			if (pPipeUser && pPipeUser->GetAttentionTarget())
			{
				atTarget = pPipeUser->GetAttentionTarget()->GetName();
			}
			ColorB color(0, 255, 255);
			ColorB colorGray(128, 102, 102);
			
			dc->Draw2dLabel(column, row, ai->second.GetAIObject()->GetName(), colorGray);
			dc->Draw2dLabel(column+12, row, eventDescr.c_str(), color);
			dc->Draw2dLabel(column+36, row, atTarget.c_str(), color);
			++row;
		}
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawStatsList() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	float drawDist2 = static_cast<float>(gAIEnv.CVars.DrawStats);
	drawDist2*=drawDist2;
	int column(1), row(1);
  // avoid memory allocations
	static string sGoalPipeName;
	static string atTargetName;
	AIObjectOwners::const_iterator ai;
	CDebugDrawContext dc;
	if ((ai = m_Objects.find(AIOBJECT_PUPPET)) != m_Objects.end())
		for (;ai!=m_Objects.end(); ++ai)
		{
			if (ai->first != AIOBJECT_PUPPET)
				break;
			if (!ai->second.GetAIObject()->IsEnabled())
				continue;
			float dist2 = (dc->GetCameraPos() - (ai->second.GetAIObject())->GetPos()).GetLengthSquared();
			if(dist2>drawDist2)
				continue;
			CPuppet* pTargetPuppet = ai->second.GetAIObject()->CastToCPuppet();
			if (!pTargetPuppet)
				continue;

			CPipeUser* pPipeUser = ai->second.GetAIObject()->CastToCPipeUser();
			if (pPipeUser && pPipeUser->GetAttentionTarget())
				atTargetName = pPipeUser->GetAttentionTarget()->GetName();
			else
				atTargetName = "--";

			CGoalPipe* pPipe = pPipeUser ? pPipeUser->GetCurrentGoalPipe() : 0;

			sGoalPipeName = pPipe ? pPipe->GetNameAsString() : "--";

			if(pPipeUser)
			{
				ColorB color(0, 255, 255);
				dc->Draw2dLabel(column, row, ai->second.GetAIObject()->GetName(), color);
				dc->Draw2dLabel(column+12, row, atTargetName.c_str(), color);
				dc->Draw2dLabel(column+36, row, sGoalPipeName.c_str(), color);

				const char* szGoalopName = "--";
				if (pPipeUser->m_lastExecutedGoalop != eGO_LAST && pPipe)
					szGoalopName = pPipe->GetGoalOpName(pPipeUser->m_lastExecutedGoalop);
				dc->Draw2dLabel(column+52, row, szGoalopName, color);
				++row;
			}
		}
}

//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawEnabledPuppets()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	const float xPos = 28.0f;
	const float yPos = 25.0f;
	const float yStep = 2.0f;

	float yOffset = 0.0f;

	CDebugDrawContext dc;
	dc->TextToScreen(xPos, yPos, "---- Enabled AI Puppets ----");
	yOffset += yStep * 2;

	int nEnabled = m_enabledPuppetsSet.size();

	PuppetSet::const_iterator it = m_enabledPuppetsSet.begin();
	PuppetSet::const_iterator itEnd = m_enabledPuppetsSet.end();

	for ( ; it!=itEnd ; ++it, yOffset += yStep)
	{
			CPuppet* pPuppet = it->GetAIObject();
			if (!pPuppet)
				continue;
			const char *sName = pPuppet->GetName();
			const char *sTerritory = pPuppet->GetTerritoryShapeName();
			const char *sWave = pPuppet->GetWaveName();
			const int iAlertLevel = pPuppet->GetAlertness();
			dc->TextToScreen(xPos, yPos + yOffset, "%-20s (%d) - %s - %s", sName, iAlertLevel, sTerritory, sWave);
	}
}

//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawEnabledPlayers() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	static float xPos = 28.0f;
	static float yPos = 25.0f;
	static float yStep = 2.0;

	float yOffset = 0.0f;

	CDebugDrawContext dc;

	dc->TextToScreen(xPos, yPos, "---- Enabled AI Players ----");
	yOffset += yStep * 2;

	//AIObjects::const_iterator it=m_Objects.find(AIOBJECT_PLAYER);
	AIObjectOwners::const_iterator it = m_Objects.find(AIOBJECT_PLAYER);
	for ( ; it != m_Objects.end() && it->first==AIOBJECT_PLAYER; ++it)
	{
		CAIPlayer* pPlayer = CastToCAIPlayerSafe(it->second.GetAIObject());
		if (pPlayer)
		{
			IAIActorProxy *pProxy = pPlayer->GetProxy();

			const char *sName = pPlayer->GetName();
			const int iHealth = pProxy ? pProxy->GetActorHealth() : 0;
			const int iArmor = pProxy ? pProxy->GetActorArmor() : 0;
			dc->TextToScreen(xPos, yPos + yOffset, "%-20s - %dH %dA", sName, iHealth, iArmor);
		}
	}
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawUpdate() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	EDrawUpdateMode mode = DRAWUPDATE_NONE;
	switch (gAIEnv.CVars.DebugDrawUpdate)
	{
	case 1: mode = DRAWUPDATE_NORMAL; break;
	case 2: mode = DRAWUPDATE_WARNINGS_ONLY; break;
	};
	if (mode == DRAWUPDATE_NONE)
		return;

	int totalCount = 0;
	int	activeVehicleCount = 0;
	int	activePuppetCount = 0;

	int row(1);

	AIObjectOwners::const_iterator ai;
	for (ai = m_Objects.find(AIOBJECT_PUPPET); ai!=m_Objects.end(); ++ai)
	{
		if (ai->first != AIOBJECT_PUPPET)
			break;
		CPuppet* pTargetPuppet = ai->second.GetAIObject()->CastToCPuppet();
		if (pTargetPuppet->IsActive())
		{
			++activePuppetCount;
			if (DebugDrawUpdateUnit(pTargetPuppet, row, mode))
				++row;
		}
		++totalCount;
	}
	for (ai = m_Objects.find(AIOBJECT_VEHICLE); ai!=m_Objects.end(); ++ai)
	{
		if (ai->first != AIOBJECT_VEHICLE)
			break;
		CPuppet* pTargetPuppet = ai->second.GetAIObject()->CastToCPuppet();
		if (pTargetPuppet->IsActive())
		{
			++activeVehicleCount;
			if (DebugDrawUpdateUnit(pTargetPuppet, row, mode))
				++row;
		}
		++totalCount;
	}

	char	buffString[128];
	sprintf(buffString, "AI UPDATES - Puppets: %3d  Vehicles: %3d  Total: %3d/%d  EnabledPuppetSet: %d",
		activePuppetCount, activeVehicleCount, (activePuppetCount+activeVehicleCount), totalCount, m_enabledPuppetsSet.size());
	CDebugDrawContext dc;
	dc->Draw2dLabel(1, 0, buffString, ColorB(0, 192, 255));
}


//
//-----------------------------------------------------------------------------------------------------------
bool CAISystem::DebugDrawUpdateUnit(CPuppet* pTargetPuppet, int row, EDrawUpdateMode mode) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	bool bShouldUpdate = pTargetPuppet->GetProxy()->IfShouldUpdate();

	if (mode == DRAWUPDATE_WARNINGS_ONLY && bShouldUpdate)
		return false;

	ColorB colorOk(255, 255, 255);
	ColorB colorWarning(255, 192, 0);
	ColorB& color = bShouldUpdate ? colorOk : colorWarning;

	CDebugDrawContext dc;

	dc->Draw2dLabel(1, row, pTargetPuppet->GetName(), color);

	char	buffString[32];
	float dist = Distance::Point_Point(pTargetPuppet->GetPos(), dc->GetCameraPos());
	sprintf(buffString, "%6.1fm", dist);
	dc->Draw2dLabel(20, row, buffString, color);

	if (pTargetPuppet->GetProxy()->IsUpdateAlways())
		dc->Draw2dLabel(27, row, "Always", color);

	return true;
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawLocate() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	const char *pString=gAIEnv.CVars.DrawLocate;

	if( pString[0] ==0 || !strcmp(pString,"none"))
		return;
		
	if (!strcmp(pString, "squad"))
	{
		AIObjectOwners::const_iterator ai;
		if ((ai = m_Objects.find(AIOBJECT_PUPPET)) != m_Objects.end())
			for (;ai!=m_Objects.end(); ++ai)
			{
				if (ai->first != AIOBJECT_PUPPET)
					break;
				CPuppet* pPuppet = (CPuppet*) ai->second.GetAIObject();
				if(pPuppet->GetGroupId() == 0)
					DebugDrawLocateUnit(pPuppet);
			}
	}
	else 	if(!strcmp(pString,"enemy"))
	{
		AIObjectOwners::const_iterator ai;
		if ((ai = m_Objects.find(AIOBJECT_PUPPET)) != m_Objects.end())
			for (;ai!=m_Objects.end(); ++ai)
			{
				if (ai->first != AIOBJECT_PUPPET)
					break;
				CPuppet *pPuppet = (CPuppet*) ai->second.GetAIObject();
				if(pPuppet->GetGroupId() != 0)
					DebugDrawLocateUnit(pPuppet);
			}
	}
	else 	if(!strcmp(pString,"all"))
	{
		AIObjectOwners::const_iterator ai;
		if ((ai = m_Objects.find(AIOBJECT_PUPPET)) != m_Objects.end())
			for (;ai!=m_Objects.end(); ++ai)
			{
				if (ai->first != AIOBJECT_PUPPET)
					break;
				CPuppet *pPuppet = (CPuppet*) ai->second.GetAIObject();
				DebugDrawLocateUnit(pPuppet);
			}
	}
	else if(strlen(pString)>1)
	{
		CAIObject *pTargetObject = GetAIObjectByName(pString);
		if (pTargetObject)
		{
			DebugDrawLocateUnit(pTargetObject);
		}
		else
		{
			int groipId=-1;
			if(sscanf(pString, "%d", &groipId) == 1)
			{
				AIObjectOwners::const_iterator ai;
				if ((ai = m_Objects.find(AIOBJECT_PUPPET)) != m_Objects.end())
					for (;ai!=m_Objects.end(); ++ai)
					{
						if (ai->first != AIOBJECT_PUPPET)
							break;
						CPuppet *pPuppet = (CPuppet*) ai->second.GetAIObject();
						if(pPuppet->GetGroupId() == groipId)
							DebugDrawLocateUnit(pPuppet);
					}
			}
		}
	}
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawTargetUnit(CAIObject* pAIObj) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CPipeUser *pPipeUser = pAIObj->CastToCPipeUser();
	if (!pPipeUser)
		return;
	CAIObject* pTarget = static_cast<CAIObject*>(pPipeUser->GetAttentionTarget());
	if(!pTarget)
		return;
	Vec3	posSelf = pAIObj->GetPos();
	Vec3	posTarget = pTarget->GetPos();
	Vec3	verOffset(0,0,2);
	//Vec3	middlePoint( (posSelf+posTarget)*.5f + Vec3(0,0,3) );
	Vec3	middlePoint( posSelf+(posTarget-posSelf)*.73f + Vec3(0,0,3) );

	ColorB colorSelf(250,250,25);
	ColorB colorTarget(250,50,50);
	ColorB colorMiddle(250,100,50);

	CDebugDrawContext dc;
	dc->DrawLine(posSelf, colorSelf, posSelf+verOffset, colorSelf);
	dc->DrawLine(posSelf+verOffset, colorSelf, middlePoint, colorMiddle);
	dc->DrawLine(middlePoint, colorMiddle, posTarget, colorTarget);
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawSelectedHideSpots() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	AIObjectOwners::const_iterator ai = m_Objects.find(AIOBJECT_PUPPET);
	if (ai != m_Objects.end())
	{
		float drawDist2 = sqr(static_cast<float>(gAIEnv.CVars.DrawHideSpots));
		CDebugDrawContext dc;
	
		for (;ai!=m_Objects.end(); ++ai)
		{
			if (ai->first != AIOBJECT_PUPPET)
				break;

			CAIObject* pAIObject = ai->second.GetAIObject();
			if (pAIObject && pAIObject->IsEnabled() &&
				((dc->GetCameraPos() - pAIObject->GetPhysicsPos()).GetLengthSquared() <= drawDist2))
			{
				DebugDrawMyHideSpot(pAIObject);
			}
		}
		}
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawMyHideSpot(CAIObject* pAIObj) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CPipeUser *pPipeUser = pAIObj->CastToCPipeUser();
	if (!pPipeUser)
		return;

	if(!pPipeUser->m_CurrentHideObject.IsValid())
		return;

	CDebugDrawContext dc;
	dc->DrawCone(pPipeUser->m_CurrentHideObject.GetLastHidePos()+Vec3(0,0,5), Vec3(0,0,-1), .2f, 5.f, ColorB(255, 26, 26));

	Vec3	posSelf = pAIObj->GetPhysicsPos();
	Vec3	posTarget = pPipeUser->m_CurrentHideObject.GetLastHidePos()+Vec3(0,0,5); // ObjectPos()+Vec3(0,0,5);
	Vec3	verOffset(0,0,2);
	Vec3	middlePoint( (posSelf+posTarget)*.5f + Vec3(0,0,3) );

	ColorB colorSelf(250,250,25);
	ColorB colorTarget(250,50,50);
	ColorB colorMiddle(250,100,50);

	pPipeUser->m_CurrentHideObject.DebugDraw();

	dc->DrawLine(posSelf, colorSelf, posSelf+verOffset, colorSelf);
	dc->DrawLine(posSelf+verOffset, colorSelf, middlePoint, colorMiddle);
	dc->DrawLine(middlePoint, colorMiddle, posTarget+verOffset, colorMiddle);
	dc->DrawLine(posTarget+verOffset, colorMiddle, posTarget, colorTarget);
}


//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawLocateUnit(CAIObject* pAIObj) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if (!pAIObj->IsEnabled())
		return;

	CPipeUser* pPipeUser = pAIObj->CastToCPipeUser();
	if (!pPipeUser)
		return;

	Vec3	posSelf = pAIObj->GetPos();
	Vec3	posMarker = posSelf + Vec3(0,0,3);
	ColorB colorSelf(20,20,250);
	ColorB colorMarker;

	CDebugDrawContext dc;

	if (pPipeUser->GetAttentionTarget())
	{
		colorMarker.Set(250, 27, 27);
		dc->Draw3dLabel(posMarker-Vec3(0.f,0.f,.21f), .8f, "%s\n\r%s", pAIObj->GetName(),pPipeUser->GetAttentionTarget()->GetName());
	}
	else
	{
		colorMarker.Set(250, 250, 30);
		dc->Draw3dLabel(posMarker-Vec3(0.f,0.f,.21f), .8f, "%s", pAIObj->GetName());
	}

	dc->Init3DMode();
	dc->SetDepthTest(false);
	dc->DrawSphere(posMarker, .2f, colorMarker);
	dc->DrawLine(posMarker, colorMarker, posSelf, colorSelf);

	if(pPipeUser->AllowedToFire())
		dc->DrawCone(posSelf + Vec3(0.f, 0.f, .7f), Vec3(0, 0, -1), .15f, .5f, ColorB(255, 20, 20));
	if(pPipeUser->m_State.fire)
		dc->DrawCone(posSelf + Vec3(0.f, 0.f, .7f), pAIObj->GetMoveDir(), .25f, .25f, ColorB(255, 20, 250));
}

//
//-----------------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawBadAnchors()
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CVolumeNavRegion * volumeNavRegion = m_pNavigation->GetVolumeNavRegion();
	if( !volumeNavRegion )
		return;

	int type=gAIEnv.CVars.DrawBadAnchors;

	CDebugDrawContext dc;
	
	AIObjectOwners::const_iterator ai;
	if ((ai = m_Objects.find(type)) != m_Objects.end())
	{
		for (;ai!=m_Objects.end(); ++ ai)
		{
			if (ai->first != type)
				break;
			unsigned int nodeIndex = volumeNavRegion->GetEnclosing((ai->second.GetAIObject())->GetPos());
			GraphNode*	pNode = m_pGraph->GetNodeManager().GetNode(nodeIndex);
			if( !pNode )
			{
				// Bad anchor.
				dc->DrawSphere((ai->second.GetAIObject())->GetPos(), .4f, ColorB(255, 179, 179));
			}
			}
		}
}

//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawGroups(short int groupId)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if (groupId >= 0)
		return DebugDrawOneGroup(groupId);
	else
	{
		short prevId(-1);
		for(AIObjects::const_iterator itr(m_mapGroups.begin()); itr!=m_mapGroups.end(); ++itr)
		{
			if(prevId!=itr->first)
				DebugDrawOneGroup(itr->first);
			prevId = itr->first;
		}
	}
}

//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawOneGroup(short grpID)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);
	AIObjects::const_iterator itr(m_mapGroups.find(grpID));
	if(itr == m_mapGroups.end())
		return;

	Vec3	groupDrawPos(0,0,0);
	int		groupCount(0);
	bool hasLeader(false);
	for(;itr!=m_mapGroups.end() && itr->first==grpID; ++itr)
	{
		CAIObject *pCurObj(itr->second.GetAIObject());
		if(GetLeader(itr->first) == pCurObj)
			continue;

		if(GetLeaderAIObject(pCurObj) == pCurObj)
		{
			groupDrawPos = pCurObj->GetPos();
			hasLeader = true;
			break;
		}
		groupDrawPos += pCurObj->GetPos();
		++groupCount;
	}
	if(groupCount<2)
		return;
	if(!hasLeader)
		groupDrawPos /= groupCount;
	groupDrawPos += Vec3(0,0,5);

	ColorB colorSelf(250,250,25);
	ColorB colorCenter(250,50,250);
	
	CDebugDrawContext dc;
	
	for(itr=m_mapGroups.find(grpID); itr!=m_mapGroups.end() && itr->first==grpID; ++itr)
	{
		CAIObject *pCurObj(itr->second.GetAIObject());
		if(GetLeader(itr->first) == pCurObj)
			continue;
		dc->DrawLine(pCurObj->GetPos(), colorSelf, groupDrawPos, colorCenter);
	}
	if(hasLeader)
	{
		dc->DrawSphere(groupDrawPos, 1.3f, colorCenter);
	}

	// draw beacon
	CAIObject* pBeacon = (CAIObject*) GetAISystem()->GetBeacon(grpID);
	if(pBeacon)
	{
		Vec3 pos(pBeacon->GetPos());
		ColorB color(0, 255, 255);
		dc->DrawSphere(pos, 0.35f, color);
		dc->DrawLine(pos,color,pos+Vec3(0,0,2), color);
		pos.z+=2;
		dc->DrawCone(pos, Vec3Constants<float>::fVec3_OneZ, 0.20f, 0.5f, color);
		pos.z+=1;
		dc->Draw3dLabel(pos, 1, "%s", pBeacon->GetName());
	}
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawShooting() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	const char *pName(gAIEnv.CVars.DrawShooting);
	if(!pName )
		return;

	CAIObject *pTargetObject = GetAIObjectByName(pName);
	if (!pTargetObject)
		return;

	CPuppet* pTargetPuppet = pTargetObject->CastToCPuppet();
	if (!pTargetPuppet)
		return;

	int column(1), row(1);
	char	buffString[128];

	ColorB colorOn ( 26, 255, 26);
	ColorB colorOff(255,  26, 26);

	SAIWeaponInfo	weaponInfo;
	pTargetObject->GetProxy()->QueryWeaponInfo(weaponInfo);
	CDebugDrawContext dc;

	if(weaponInfo.outOfAmmo)
	{
		dc->Draw2dLabel(column, row, ">>> OUT OF AMMO <<<", colorOn);
		++row;
	}

	switch(pTargetPuppet->GetAimState())
	{
	case 	AI_AIM_NONE:			// No aiming requested
		sprintf(buffString, "aimState >>> none");
		break;
	case 	AI_AIM_WAITING:			// Aiming requested, but not yet ready.
		sprintf(buffString, "aimState >>> waiting");
		break;
	case 	AI_AIM_OBSTRUCTED:	// Aiming obstructed.
		sprintf(buffString, "aimState >>> OBSTRUCTED");
		break;
	case 	AI_AIM_READY:
		sprintf(buffString, "aimState >>> ready");
		break;
	case 	AI_AIM_FORCED:
		sprintf(buffString, "aimState >>> forced");
		break;
	default:
		sprintf(buffString, "aimState >>> undefined");
	}

	dc->Draw2dLabel(column, row, buffString, colorOn);

	++row;

	sprintf(buffString, "current accuracy: %.3f", pTargetPuppet->GetAccuracy((CAIObject*)pTargetPuppet->GetAttentionTarget()));
	dc->Draw2dLabel(column, row, buffString, colorOn);

	++row;

	if( pTargetPuppet->AllowedToFire())
		dc->Draw2dLabel(column, row, "fireCommand ON", colorOn);
	else
	{
		dc->Draw2dLabel(column, row, "fireCommand OFF", colorOff);
		return;
	}

	++row;

	if( pTargetPuppet->m_State.fire)
		dc->Draw2dLabel(column, row, "trigger DOWN", colorOn);
	else
		dc->Draw2dLabel(column, row, "trigger OFF", colorOff);

	++row;

	if(pTargetPuppet->m_pFireCmdHandler && stricmp("instant_deprecated", pTargetPuppet->m_pFireCmdHandler->GetName())==0 )
	{
		CFireCommandInstantDeprecated *pFireCmd(static_cast<CFireCommandInstantDeprecated*>(pTargetPuppet->m_pFireCmdHandler));
		if(pFireCmd->IsStartingNow())
		{
			sprintf(buffString, "Starting %d time left %.2f", pFireCmd->m_nDrawFireCounter, pFireCmd->m_fFadingToNormalTime);
			dc->Draw2dLabel(column, row, buffString, colorOff);
		}
		else
			dc->Draw2dLabel(column, row, "NORMAL fire ", colorOn);

		++row;

		if(pFireCmd->dbg_FrinedsInLine)
			dc->Draw2dLabel(column, row, "Friend in way", colorOff);
		else
			dc->Draw2dLabel(column, row, "no friend", colorOn);

		++row;

		if(!pFireCmd->dbg_IsAiming)
			dc->Draw2dLabel(column, row, "NO AIMING", colorOff);
		else
			dc->Draw2dLabel(column, row, "aiming ok", colorOn);

		++row;

		if(pFireCmd->dbg_AngleTooDifferent)
		{
			sprintf(buffString, "Angle too different %.2f", pFireCmd->dbg_AngleDiffValue);
			dc->Draw2dLabel(column, row, buffString, colorOff);
		}
		else
		{
			sprintf(buffString, "Angle good %.2f", pFireCmd->dbg_AngleDiffValue);
			dc->Draw2dLabel(column, row, buffString, colorOn);
		}

		++row;

		sprintf(buffString, "shots>>>  %d %d %.2f", pFireCmd->dbg_ShotCounter, pFireCmd->dbg_ShotHitCounter,   
			static_cast<float>(pFireCmd->dbg_ShotHitCounter)/static_cast<float>(pFireCmd->dbg_ShotCounter));
		dc->Draw2dLabel(column, row, buffString, colorOn);

		++row;

		pTargetObject->GetProxy()->DebugDraw();
	}
	else
	{
		if(pTargetPuppet->m_pFireCmdHandler)
		{
			sprintf(buffString, "CAN'T HANDLE CURRENT WEAPON -- %s", pTargetPuppet->m_pFireCmdHandler->GetName());
			dc->Draw2dLabel(column, row, buffString, colorOff);
		}
		else
			dc->Draw2dLabel(column, row, "ZERO WEAPON !!!", colorOff);
	}
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawAreas() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	const int ALERT_STANDBY_IN_RANGE = 340;
	const int ALERT_STANDBY_SPOT = 341;
	const int COMBAT_TERRITORY = 342;

	AIObjectOwners::const_iterator end = m_Objects.end();

	CDebugDrawContext dc;

	Vec3 camPos = dc->GetCameraPos();
	float	statsDist = gAIEnv.CVars.AgentStatsDist;


	// Draw standby areas
	for(ShapeMap::const_iterator it = m_mapGenericShapes.begin(); it != m_mapGenericShapes.end(); ++it)
	{
		const SShape&	shape = it->second;

		if(Distance::Point_AABBSq(camPos, shape.aabb) > sqr(100.0f))
			continue;

		int	a = 255;
		if(!shape.enabled)
			a = 120;

		std::vector<Vec3> tempVector(shape.shape.begin(), shape.shape.end());
		if (!tempVector.empty())
		{
		switch(shape.type)
		{
		case ALERT_STANDBY_IN_RANGE:
				dc->DrawRangePolygon(&tempVector[0], tempVector.size(), 0.75f, ColorB(255,128,0,a/2), ColorB(255,128,0,a), true);
			break;
		case COMBAT_TERRITORY:
				dc->DrawRangePolygon(&tempVector[0], tempVector.size(), 0.75f, ColorB(40,85,180,a/2), ColorB(40,85,180,a), true);
			break;
		}
	}
	}

	// Draw attack directions.
	for (AIObjectOwners::const_iterator ai = m_Objects.find(ALERT_STANDBY_SPOT); ai != end; ++ai)
	{
		if (ai->first != ALERT_STANDBY_SPOT) break;
		CAIObject* obj = ai->second.GetAIObject();

		const float	rad = obj->GetRadius();

		dc->DrawRangeCircle(obj->GetPos() + Vec3(0,0,0.5f), rad, 0.25f, ColorB(255,255,255,120), ColorB(255, 255, 255), true);

		Vec3	dir(obj->GetMoveDir());
		dir.z = 0;
		dir.NormalizeSafe();
		dc->DrawRangeArc(obj->GetPos() + Vec3(0,0,0.5f), dir, DEG2RAD(120.0f), rad - 0.7f, rad/2, ColorB(255,0,0,64), ColorB(255, 255, 255), false);
	}

	AIObjectOwners::const_iterator ai = m_Objects.find(AIOBJECT_PUPPET);
	for (;ai!=m_Objects.end(); ++ai)
	{
		if (ai->first != AIOBJECT_PUPPET)
			break;

		CAIObject* obj = ai->second.GetAIObject();
		if(!obj->IsEnabled())
			continue;

		if(Distance::Point_PointSq(obj->GetPos(), camPos) > sqr(statsDist))
			continue;

		CPuppet* puppet = obj->CastToCPuppet();
		if (!puppet)
			continue;

		CDebugDrawContext dc;

		SShape* territoryShape = puppet->GetTerritoryShape();
		if(territoryShape)
		{
			float dist = 0;
			Vec3 nearestPt;
			ListPositions::const_iterator	it = territoryShape->NearestPointOnPath(puppet->GetPos(), dist, nearestPt);
			Vec3	p0 = puppet->GetPos() - Vec3(0,0,0.5f);
			dc->Draw3dLabel(p0 * 0.7f + nearestPt * 0.3f,1,"Terr");
			dc->DrawLine(p0, ColorB(30,208,224), nearestPt, ColorB(30,208,224));		
		}

		SShape* refShape = puppet->GetRefShape();
		if(refShape)
		{
			float dist = 0;
			Vec3 nearestPt;
			ListPositions::const_iterator	it = refShape->NearestPointOnPath(puppet->GetPos(), dist, nearestPt);
			Vec3	p0 = puppet->GetPos() - Vec3(0,0,1.0f);
			dc->Draw3dLabel(p0 * 0.7f + nearestPt * 0.3f,1,"Ref");
			dc->DrawLine(p0, ColorB(135,224,30), nearestPt, ColorB(135,224,30));		
		}
	}
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawExpensiveAccessoryQuota() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CDebugDrawContext dc;
	
	Vec3 cameraPos = dc->GetCameraPos();

	const float analyzeDist = 350;

	for (AIObjectOwners::const_iterator ai = m_Objects.find(AIOBJECT_PUPPET) ;ai!=m_Objects.end(); ++ai)
	{
		if (ai->first != AIOBJECT_PUPPET)
			break;
		CAIObject* obj = ai->second.GetAIObject();
		if (!obj->IsEnabled())
			continue;
		CPuppet* pPuppet = obj->CastToCPuppet();
		if (!pPuppet)
			continue;
		if (Distance::Point_PointSq(obj->GetPos(), cameraPos) > sqr(analyzeDist))
			continue;

		if (pPuppet->IsAllowedToUseExpensiveAccessory())
		{
			SAIBodyInfo	bodyInfo;
			pPuppet->GetProxy()->QueryBodyInfo(bodyInfo);
			Vec3	pos = pPuppet->GetPhysicsPos();
			AABB	aabb(bodyInfo.stanceSize);
			aabb.Move(pos);
			dc->DrawAABB(aabb, true, ColorB(0, 196, 255, 64), eBBD_Faceted);
		}
	}
}

//
//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawAmbientFire() const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CAIPlayer* pPlayer = CastToCAIPlayerSafe(GetPlayer());
	if(!pPlayer)
		return;

	const float analyzeDist = 350;

	CDebugDrawContext dc;

	for (AIObjectOwners::const_iterator ai = m_Objects.find(AIOBJECT_PUPPET) ;ai!=m_Objects.end(); ++ai)
	{
		if (ai->first != AIOBJECT_PUPPET)
			break;
		CAIObject* obj = ai->second.GetAIObject();
		if (!obj->IsEnabled())
			continue;
		CPuppet* pPuppet = obj->CastToCPuppet();
		if (!pPuppet)
			continue;
		if (Distance::Point_PointSq(obj->GetPos(), pPlayer->GetPos()) > sqr(analyzeDist))
			continue;

		if (pPuppet->IsAllowedToHitTarget())
		{
//			dc->DrawCone(obj->GetPhysicsPos(), Vec3(0,0,1), 0.3f, 1.0f, ColorB(255,255,255));
			SAIBodyInfo	bodyInfo;
			pPuppet->GetProxy()->QueryBodyInfo(bodyInfo);
			Vec3	pos = pPuppet->GetPhysicsPos();
			AABB	aabb(bodyInfo.stanceSize);
			aabb.Move(pos);
			dc->DrawAABB(aabb, true, ColorB(255,0,0,64), eBBD_Faceted);
		}
	}

}

//-----------------------------------------------------------------------------------------------------------
void CAISystem::AddDebugBox(const Vec3& pos, const OBB& obb, uint8 r, uint8 g, uint8 b, float time)
{
	if(gAIEnv.CVars.DebugDraw > 0)
		m_vecDebugBoxes.push_back(SDebugBox(pos, obb, ColorB(r, g, b), time));
}

//-----------------------------------------------------------------------------------------------------------
void CAISystem::AddPerceptionDebugLine( const char *tag, const Vec3& start, const Vec3& end, uint8 r, uint8 g, uint8 b, float time, float thickness )
{
	if(gAIEnv.CVars.DebugDraw > 0)
	{
		if (tag)
		{
			std::list<SPerceptionDebugLine>::iterator it = m_lstDebugPerceptionLines.begin();
			for (; it != m_lstDebugPerceptionLines.end(); ++it)
			{
				if (strlen(it->name) > 0 && _stricmp(it->name, tag) == 0)
				{
					*it = SPerceptionDebugLine( tag, start, end, ColorB( r, g, b ), time, thickness );
					return;
				}
			}
		}
		
		m_lstDebugPerceptionLines.push_back( SPerceptionDebugLine( tag?tag:"", start, end, ColorB( r, g, b ), time, thickness ) );
	}
}

//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawInterestSystem(int iLevel) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	const CCentralInterestManager*  pInterestManager = CCentralInterestManager::GetInstance();

	CDebugDrawContext dc;
	ColorB cGreen = ColorB(0, 255, 0);
	ColorB cYellow = ColorB(255, 255, 0);
	ColorB cRed = ColorB(255, 0, 0);

	if(iLevel > 0)
	{
		CCentralInterestManager::TVecPIMs::const_iterator it = pInterestManager->GetPIMs()->begin();
		CCentralInterestManager::TVecPIMs::const_iterator itEnd = pInterestManager->GetPIMs()->end();
		for (; it != itEnd; ++it)
		{
			const CPersonalInterestManager* pPersonal = &(*it);
			if(!pPersonal->IsReset() && pPersonal->IsInterested())
			{
				dc->DrawLine(pPersonal->GetAssigned()->GetPos(), cYellow, pPersonal->GetInterestingPos(), cYellow);
			}
		}
	}

	if(iLevel > 1)
	{
		CCentralInterestManager::TVecInteresting::const_iterator it = pInterestManager->GetInterestingEntities()->begin();
		CCentralInterestManager::TVecInteresting::const_iterator itEnd = pInterestManager->GetInterestingEntities()->end();
		for (; it != itEnd; ++it)
		{
			const SEntityInterest* pInteresting = &(*it);
			if(pInteresting->IsValid())
			{
				dc->DrawWireSphere(pInteresting->GetEntity()->GetPos(), pInteresting->fRadius, (pInteresting->actionName.empty()) ? cGreen : cYellow);
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------------
void CAISystem::DEBUG_AddFakeDamageIndicator(CPuppet* pShooter, float t)
{
	m_DEBUG_fakeDamageInd.push_back(SDebugFakeDamageInd(pShooter->GetPos(), t));

	IPhysicalEntity*	phys = 0;

	// If the AI is tied to a vehicle, use the vehicles physics to draw the silhouette.
	SAIBodyInfo bi;
	if(pShooter->GetProxy())
	{
		pShooter->GetProxy()->QueryBodyInfo(bi);
		if(bi.linkedVehicleEntity && bi.linkedVehicleEntity->GetAI() && bi.linkedVehicleEntity->GetAI()->GetProxy())
			phys = bi.linkedVehicleEntity->GetAI()->GetProxy()->GetPhysics(true);
	}

	if(!phys)
		phys = pShooter->GetProxy()->GetPhysics(true);

	if(!phys)
		return;

	SDebugFakeDamageInd& ind = m_DEBUG_fakeDamageInd.back();

	pe_status_nparts statusNParts;
	int nParts = phys->GetStatus(&statusNParts);

	pe_status_pos statusPos;
	phys->GetStatus(&statusPos);

	pe_params_part paramsPart;
	for (statusPos.ipart = 0, paramsPart.ipart = 0 ; statusPos.ipart < nParts ; ++statusPos.ipart, ++paramsPart.ipart)
	{
		phys->GetParams(&paramsPart);
		phys->GetStatus(&statusPos);

		primitives::box box;
		statusPos.pGeomProxy->GetBBox(&box);

		box.center *= statusPos.scale;
		box.size *= statusPos.scale;

		box.size.x += 0.05f;
		box.size.y += 0.05f;
		box.size.z += 0.05f;

		ind.verts.push_back(statusPos.pos + statusPos.q * (box.center + box.Basis.GetColumn0() * box.size.x));
		ind.verts.push_back(statusPos.pos + statusPos.q * (box.center + box.Basis.GetColumn0() * -box.size.x));
		ind.verts.push_back(statusPos.pos + statusPos.q * (box.center + box.Basis.GetColumn1() * box.size.y));
		ind.verts.push_back(statusPos.pos + statusPos.q * (box.center + box.Basis.GetColumn1() * -box.size.y));
		ind.verts.push_back(statusPos.pos + statusPos.q * (box.center + box.Basis.GetColumn2() * box.size.z));
		ind.verts.push_back(statusPos.pos + statusPos.q * (box.center + box.Basis.GetColumn2() * -box.size.z));
	}
}

#endif //CRYAISYSTEM_DEBUG


//-----------------------------------------------------------------------------------------------------------
// NOTE: The following are virtual! They cannot be stripped out just yet.
//-----------------------------------------------------------------------------------------------------------


//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDraw()
{
#ifdef CRYAISYSTEM_DEBUG
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	if (!m_bInitialized) return;

	if (gEnv->bMultiplayer && !gEnv->bServer) return;	// early out for MP/coop clients

	gAIEnv.pStatsManager->Render();
	gAIEnv.pStatsManager->Reset(eStatReset_Frame);

	int	debugDrawValue = gAIEnv.CVars.DebugDraw;

	// As a special case, we want to draw the InterestSystem alone sometimes
	if (gAIEnv.CVars.DebugInterest > 0)
		DebugDrawInterestSystem(gAIEnv.CVars.DebugInterest);

	if (debugDrawValue < 0) return;   // if -ve ONLY display warnings

	// Set render state to standard - return it to this if you change it
	CDebugDrawContext dc;
	dc->Init3DMode();
	dc->SetAlphaBlended(true);

	if (gAIEnv.CVars.DebugDrawCoordination > 0)
		gAIEnv.pCoordinationManager->DebugDraw();

	if (gAIEnv.CVars.DebugDrawCoordination > 1)
		gAIEnv.pCoordinationManager->DebugDrawStatus();

	if (gAIEnv.CVars.DebugDrawCommunication > 0)
		gAIEnv.pCommunicationManager->DebugDraw();

	if (gAIEnv.CVars.DebugDrawCover > 0)
		gAIEnv.pCoverSystem->DebugDraw();

	if (!IsEnabled())
	{
		dc->Draw2dLabel(100, 200, 1.5f, ColorB(255, 255, 255), false, "AI System is disabled; ai_DebugDraw is on");
		return;
	}

	if (gAIEnv.CVars.DebugDrawEnabledPuppets == 1)
		DebugDrawEnabledPuppets();		// Called only in this line => Maybe we should remove it from the interface?
	else if (gAIEnv.CVars.DebugDrawEnabledPlayers == 1)
		DebugDrawEnabledPlayers();	

	DebugDrawUpdate();				// Called only in this line => Maybe we should remove it from the interface?

	if (gAIEnv.CVars.DrawFakeTracers > 0)
		DebugDrawFakeTracers();

	if (gAIEnv.CVars.DrawFakeHitEffects > 0)
		DebugDrawFakeHitEffects();

	if (gAIEnv.CVars.DrawFakeDamageInd > 0)
		DebugDrawFakeDamageInd();

	if (gAIEnv.CVars.DrawPlayerRanges > 0)
		DebugDrawPlayerRanges();

	if (gAIEnv.CVars.DrawPerceptionIndicators > 0 || gAIEnv.CVars.DrawPerceptionDebugging > 0)
		DebugDrawPerceptionIndicators();

	if (gAIEnv.CVars.DrawPerceptionModifiers > 0)
		DebugDrawPerceptionModifiers();

	DebugDrawCodeCoverage();
	DebugDrawTargetTracks();

	if (debugDrawValue == 0) return;

	AILogDisplaySavedMsgs();

	DebugDrawRecorderRange();
	
	if (gAIEnv.CVars.DebugPerceptionManager > 0)
		DebugDrawPerceptionManager();

	//------------------------------------------------------------------------------------
	// Check for graph and return on fail - note that graph is assumed by most of system
	//------------------------------------------------------------------------------------
	if (!m_pGraph)
		return;

	DebugDrawNavigation();
	DebugDrawGraph(debugDrawValue);

	if (gAIEnv.CVars.DebugDrawDamageControl > 1)
		DebugDrawDamageControlGraph();

	if (gAIEnv.CVars.DrawAreas > 0)
		DebugDrawAreas();

	if (gAIEnv.CVars.DebugDrawDeadBodies > 0)
		DebugDrawDeadBodies();
	
	if (debugDrawValue == 80)
		DebugDrawTaggedNodes();

	DebugDrawSteepSlopes();
	DebugDrawVegetationCollision();

	if ( m_bUpdateSmartObjects && gAIEnv.CVars.DrawSmartObjects)
		m_pSmartObjectManager->DebugDraw();

	DebugDrawPath();
	DebugDrawPathAdjustments();
	DebugDrawNavModifiers();
	DebugDrawStatsTarget(gAIEnv.CVars.StatsTarget);
	DebugDrawBehaviorStatsTarget(gAIEnv.CVars.BehaviorStatsTarget);
	DebugDrawBehaviorSelection(gAIEnv.CVars.DebugBehaviorSelection);
	DebugDrawGroupSystem(gAIEnv.CVars.DebugDrawGroups);	
	DebugDrawCooperativeReadabilities(gAIEnv.CVars.DebugDrawCoopReadabilities);

	if (gAIEnv.CVars.DrawFormations)
		DebugDrawFormations();	

	DebugDrawNode();
	DebugDrawType();
	DebugDrawLocate();
	DebugDrawBadAnchors();

	if (gAIEnv.CVars.DrawTargets)
		DebugDrawTargetsList();

	if (gAIEnv.CVars.DrawStats)
		DebugDrawStatsList();

	short int groupId = gAIEnv.CVars.DrawGroups;
	if (groupId >= -1)
		DebugDrawGroups(groupId);
	
	DebugDrawHideSpots();
	
	if (gAIEnv.CVars.DrawHideSpots)
		DebugDrawSelectedHideSpots();

	DebugDrawDynamicHideObjects();
	DebugDrawHashSpace();
	DebugDrawAgents();
	DebugDrawShooting();

	gAIEnv.pTacticalPointSystem->DebugDraw();

	DebugDrawLightManager();
	//DebugDrawP0AndP1();
	DebugDrawPolygonSetOps();
	DebugDrawPuppetPaths();
	DebugDrawBestPositions();
	DebugDrawCheckCapsules();
	DebugDrawCheckRay();
	m_pSmartObjectManager->DebugDrawValidateSmartObjectArea();
	DebugDrawCheckGetEnclosing();
	DebugDrawCheckWalkability();
	DebugDrawCheckWalkabilityTime();
	DebugDrawCheckFloorPos();
	DebugDrawCheckGravity();
	//DebugDrawPaths();

	if (debugDrawValue == 1017)
		DebugDrawTestNode();

	DebugDrawDebugShapes();

	if (gAIEnv.CVars.DrawGroupTactic > 0)
		DebugDrawGroupTactic();

	DebugDrawRadar();

	if (gAIEnv.CVars.DebugDrawAmbientFire > 0)
		DebugDrawAmbientFire();

	if (gAIEnv.CVars.DebugDrawExpensiveAccessoryQuota > 0)
		DebugDrawExpensiveAccessoryQuota();

	if (gAIEnv.CVars.DebugDrawDamageParts > 0)
		DebugDrawDamageParts();

	if (gAIEnv.CVars.DebugDrawStanceSize > 0)
		DebugDrawStanceSize();

	if (strcmp(gAIEnv.CVars.ForcePosture, "0"))
	{
		DebugDrawForcePosture();
	}
	else
	{
		if (strcmp(gAIEnv.CVars.ForceAGAction, "0"))
			DebugDrawForceAGAction();

		if (strcmp(gAIEnv.CVars.ForceAGSignal, "0"))
			DebugDrawForceAGSignal();

		if (gAIEnv.CVars.ForceStance != -1)
			DebugDrawForceStance();
	}

	if (gAIEnv.CVars.DebugDrawAdaptiveUrgency > 0)
		DebugDrawAdaptiveUrgency();

	if (gAIEnv.CVars.DebugDrawPlayerActions > 0)
		DebugDrawPlayerActions();

	if (gAIEnv.CVars.DebugDrawCrowdControl > 0)
		DebugDrawCrowdControl();

	if (gAIEnv.CVars.DebugDrawBannedNavsos > 0)
		m_pSmartObjectManager->DebugDrawBannedNavsos();


	// Aux render flags restored by helper
#endif //CRYAISYSTEM_DEBUG
}

//-----------------------------------------------------------------------------------------------------------
void CAISystem::DebugDrawFakeTracer(const Vec3& pos, const Vec3& dir)
{
#ifdef CRYAISYSTEM_DEBUG
	CAIObject* pPlayer = GetPlayer();
	if(!pPlayer) return;

	Vec3	dirNorm = dir.GetNormalizedSafe();
	const Vec3& playerPos = pPlayer->GetPos();
	Vec3	dirShooterToPlayer = playerPos - pos;

	float projDist = dirNorm.Dot(dirShooterToPlayer);

	float distShooterToPlayer = dirShooterToPlayer.NormalizeSafe();

	if(projDist < 0.0f)
		return;

	Vec3	nearestPt = pos + dirNorm * distShooterToPlayer;

	float tracerDist = Distance::Point_Point(nearestPt, playerPos);

	const float maxTracerDist = 20.0f;
	if(tracerDist > maxTracerDist)
		return;

	float a = 1 - tracerDist/maxTracerDist;

	ray_hit hit;
	float maxd = distShooterToPlayer * 2.0f;
	if(gEnv->pPhysicalWorld->RayWorldIntersection(pos, dirNorm * maxd, COVER_OBJECT_TYPES, HIT_COVER, &hit, 1))
	{
		maxd = hit.dist;
		// fake hit fx
		Vec3	p = hit.pt + hit.n * 0.1f;
		ColorF col(0.4f+ai_frand()*0.3f, 0.4f+ai_frand()*0.3f, 0.4f+ai_frand()*0.3f,0.9f);
		m_DEBUG_fakeHitEffect.push_back(SDebugFakeHitEffect(p, hit.n, (0.5f + ai_frand() * 0.5f) * 0.9f, 0.5f + ai_frand() * 0.3f, col));
	}

	const float maxTracerLen = 50.0f;
	float d0 = distShooterToPlayer - (0.75f + ai_frand()*0.25f) * maxTracerLen/2;
	float d1 = d0 + (0.5f + ai_frand()*0.5f) * maxTracerLen;

	Limit(d0, 0.0f, maxd);
	Limit(d1, 0.0f, maxd);

	m_DEBUG_fakeTracers.push_back(SDebugFakeTracer(pos + dirNorm*d0, pos + dirNorm*d1, a, 0.15f + ai_frand()*0.15f));
#endif //CRYAISYSTEM_DEBUG
}

//-----------------------------------------------------------------------------------------------------------
void CAISystem::AddDebugLine(const Vec3& start, const Vec3& end, uint8 r, uint8 g, uint8 b, float time)
{
#ifdef CRYAISYSTEM_DEBUG
	if(gAIEnv.CVars.DebugDraw > 0)
		m_vecDebugLines.push_back(SDebugLine( start, end, ColorB(r, g, b), time));
#endif //CRYAISYSTEM_DEBUG
}

//-----------------------------------------------------------------------------------------------------------
void CAISystem::AddDebugSphere(const Vec3& pos, float radius, uint8 r, uint8 g, uint8 b, float time)
{
#ifdef CRYAISYSTEM_DEBUG
	if(gAIEnv.CVars.DebugDraw > 0)
		m_vecDebugSpheres.push_back(SDebugSphere(pos, radius, ColorB(r, g, b), time));
#endif //CRYAISYSTEM_DEBUG
}
