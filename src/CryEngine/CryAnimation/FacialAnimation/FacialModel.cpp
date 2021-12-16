////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2005.
// -------------------------------------------------------------------------
//  File name:   FacialModel.cpp
//  Version:     v1.00
//  Created:     10/10/2005 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FacialModel.h"
#include "FaceAnimation.h"
#include "FaceEffectorLibrary.h"

#include <I3DEngine.h>
#include <CryHeaders.h>
#include "../CharacterInstance.h"
#include "../ModelMesh.h"
#include "../Model.h"

#undef PI
#define PI 3.14159265358979323f

//////////////////////////////////////////////////////////////////////////
CFacialModel::CFacialModel( CCharacterModel *pModel )
{
	m_pModel = pModel;
	InitMorphTargets();
	m_bAttachmentChanged = false;
}

//////////////////////////////////////////////////////////////////////////
CFaceState* CFacialModel::CreateState()
{
	CFaceState *pState = new CFaceState(this);
	if (m_pEffectorsLibrary)
		pState->SetNumWeights( m_pEffectorsLibrary->GetLastIndex() );
	return pState;
}

//////////////////////////////////////////////////////////////////////////
void CFacialModel::InitMorphTargets()
{
	CModelMesh *pModelMesh = m_pModel->GetModelMesh(0);
	int nMorphTargetsCount = (int)pModelMesh->m_morphTargets.size();
	m_effectors.reserve(nMorphTargetsCount);
	m_morphTargets.reserve( nMorphTargetsCount );
	for (int i = 0; i < nMorphTargetsCount; i++)
	{
		CMorphTarget *pMorphTarget = pModelMesh->m_morphTargets[i];
		if (!pMorphTarget)
			continue;

		CFacialMorphTarget *pMorphEffector = new CFacialMorphTarget(i);
		string morphname = pMorphTarget->m_name;
		if (!morphname.empty() && morphname[0] == '#') // skip # character at the begining of the morph target.
			morphname = morphname.substr(1);
		pMorphEffector->SetNameString(morphname);
		pMorphEffector->SetIndexInState(i);

		SEffectorInfo ef;
		ef.pEffector = pMorphEffector;
		ef.nMorphTargetId = i;
		m_effectors.push_back( ef );

		SMorphTargetInfo mrphTrg;
		morphname.MakeLower();
		mrphTrg.name = morphname;
		mrphTrg.nMorphTargetId = i;
		m_morphTargets.push_back( mrphTrg );
	}

	std::sort( m_morphTargets.begin(),m_morphTargets.end() );

	//m_libToFaceState
}

//////////////////////////////////////////////////////////////////////////
void CFacialModel::AssignLibrary( IFacialEffectorsLibrary *pLibrary )
{
	assert( pLibrary );
	if (pLibrary == m_pEffectorsLibrary)
		return;

	m_pEffectorsLibrary = (CFacialEffectorsLibrary*)pLibrary;

	// Adds missing morph targets to the Facial Library.
	uint32 numEffectors = m_effectors.size();
	for (uint32 i=0; i<numEffectors; i++)
	{
		CFacialEffector *pModelEffector = m_effectors[i].pEffector;

		CFacialEffector *pEffector = (CFacialEffector*)m_pEffectorsLibrary->Find( pModelEffector->GetName() );
		if (!pEffector)
		{
			// Library does not contain this effector, so add it into the library.
			if (m_effectors[i].nMorphTargetId >= 0)
			{
				IFacialEffector* pNewEffector = m_pEffectorsLibrary->CreateEffector( EFE_TYPE_MORPH_TARGET,pModelEffector->GetName() );
				IFacialEffector* pRoot = m_pEffectorsLibrary->GetRoot();
				if (pRoot && pNewEffector)
					pRoot->AddSubEffector(pNewEffector);
			}
		}
	}

	// Creates a mapping table between Face state index to the index in MorphTargetsInfo array.
	// When face state is applied to the model, this mapping table is used to find what morph target is associated with state index.
	int nIndices = m_pEffectorsLibrary->GetLastIndex();
	m_faceStateIndexToMorphTargetId.resize(nIndices);
	for (int i = 0; i < nIndices; i++)
	{
		// Initialize all face state mappings.
		m_faceStateIndexToMorphTargetId[i] = -1;
	}
	SMorphTargetInfo tempMT;
	for (CFacialEffectorsLibrary::NameToEffectorMap::iterator it = m_pEffectorsLibrary->m_nameToEffectorMap.begin(); it != m_pEffectorsLibrary->m_nameToEffectorMap.end(); ++it)
	{
		CFacialEffector *pEffector = it->second;
		int nStateIndex = pEffector->GetIndexInState();
		if (nStateIndex >= 0 && nStateIndex < nIndices)
		{
			// Find morph target for this state index.
			// Morph targets must be sorted by name for binary_find to work correctly.
			tempMT.name = pEffector->GetName();
			tempMT.name.MakeLower();
			MorphTargetsInfo::iterator it2 = stl::binary_find( m_morphTargets.begin(),m_morphTargets.end(),tempMT );
			if (it2 != m_morphTargets.end())
			{
				m_faceStateIndexToMorphTargetId[nStateIndex] = (int)std::distance(m_morphTargets.begin(),it2);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
IFacialEffectorsLibrary* CFacialModel::GetLibrary()
{
	return m_pEffectorsLibrary;
}

//////////////////////////////////////////////////////////////////////////
int CFacialModel::GetMorphTargetCount() const
{
	return m_morphTargets.size();
}

//////////////////////////////////////////////////////////////////////////
const char* CFacialModel::GetMorphTargetName(int morphTargetIndex) const
{
	return (morphTargetIndex >= 0 && morphTargetIndex < int(m_morphTargets.size()) ? m_morphTargets[morphTargetIndex].name.c_str() : 0);
}

//////////////////////////////////////////////////////////////////////////
void CFacialModel::ApplyFaceStateToCharacter( CFaceState* pFaceState,CSkinInstance *pCharacter, CCharInstance *pMasterCharacter, int numForcedRotations, CFacialAnimForcedRotationEntry* forcedRotations, bool blendBoneRotations )
{
	if (!m_pEffectorsLibrary)
		return;
	
	// Update Face State with the new number of indices.
	if (pFaceState->GetNumWeights() != m_pEffectorsLibrary->GetLastIndex())
	{
		pFaceState->SetNumWeights( m_pEffectorsLibrary->GetLastIndex() );
	}


	CSkeletonPose* pSkeletonPose = &pMasterCharacter->m_SkeletonPose;
	uint32 numJoints = pSkeletonPose->GetJointCount();
	assert(numJoints <= MAX_JOINT_AMOUNT);
	
	QuatT additionalRotPos[MAX_JOINT_AMOUNT];
	Vec3 rate[MAX_JOINT_AMOUNT];
	f32 LookIK_Fade[MAX_JOINT_AMOUNT];
	for (uint32 i=0; i<numJoints; i++)
	{
		additionalRotPos[i].SetIdentity();
		rate[i]=Vec3(ZERO);
		LookIK_Fade[i]=1.0f;
	}

	// Apply forced rotations if any. This is currently used only in the facial editor preview mode to
	// support the code interface to force the neck and eye orientations.
	{
		// Make sure we use the master skeleton, in case we are an attachment.
		for (int rotIndex = 0; pMasterCharacter && pCharacter && forcedRotations && rotIndex < numForcedRotations; ++rotIndex)
		{
			//CSkeletonPose* pSkeletonPose = &pMasterCharacter->m_SkeletonPose;
			int32 id=forcedRotations[rotIndex].jointId;
			if (id >= 0 && id < MAX_JOINT_AMOUNT)
				additionalRotPos[id].q = additionalRotPos[id].q * forcedRotations[rotIndex].rotation;
		}
	}



	int nNumIndices = pFaceState->GetNumWeights();
	const CModelJoint* pModelJoint = pSkeletonPose->GetModelJointPointer(0);
	for (int i = 0; i < nNumIndices; i++)
	{
		float fWeight = pFaceState->GetWeight(i);
		if (fabs(fWeight) < EFFECTOR_MIN_WEIGHT_EPSILON)
			continue;

		if (i < (int)m_faceStateIndexToMorphTargetId.size() && m_faceStateIndexToMorphTargetId[i] >= 0)
		{
			// This is a morph target.
			int nMorphIndex = m_faceStateIndexToMorphTargetId[i];
			if (nMorphIndex >= 0 && nMorphIndex < (int)m_morphTargets.size())
			{
				int nMorphTargetId = m_morphTargets[nMorphIndex].nMorphTargetId;

				// Add morph target to character.
				CryModEffMorph morph;
				morph.m_fTime = 0;
				morph.m_nFlags = CryCharMorphParams::FLAGS_FREEZE | CryCharMorphParams::FLAGS_NO_BLENDOUT | CryCharMorphParams::FLAGS_INSTANTANEOUS;
				morph.m_nMorphTargetId = nMorphTargetId;
				morph.m_Params.m_fAmplitude = fWeight;
				morph.m_Params.m_fBlendIn = 0;
				morph.m_Params.m_fBlendOut = 0;
				morph.m_Params.m_fBalance = pFaceState->GetBalance(i);
				pCharacter->m_Morphing.m_arrMorphEffectors.push_back(morph);
			}
		}
		else
		{
			CFacialEffector *pEffector = m_pEffectorsLibrary->GetEffectorFromIndex(i);
			if (pEffector)
			{
				// Not a morph target.
				switch (pEffector->GetType())
				{
				case EFE_TYPE_ATTACHMENT:
					ApplyAttachmentEffector( pMasterCharacter,pEffector,pFaceState->GetWeight(i) );
					break;
				case EFE_TYPE_BONE:
					ApplyBoneEffector( pMasterCharacter,pEffector,pFaceState->GetWeight(i),additionalRotPos );
					break;
				}
			}
		}
	}

	uint32 numFaceJoints = pSkeletonPose->m_FaceAnimPosSmooth.size();
	if (numFaceJoints!=numJoints)
	{
		pSkeletonPose->m_FaceAnimPosSmooth.resize(numJoints);
		pSkeletonPose->m_FaceAnimPosSmoothRate.resize(numJoints);
		for (uint32 i=0; i<numJoints; i++)
		{
			pSkeletonPose->m_FaceAnimPosSmooth[i]		 = Vec3(ZERO);
			pSkeletonPose->m_FaceAnimPosSmoothRate[i] = Vec3(ZERO);
		}
	}
	for (uint32 i=0; i<numJoints; i++)
	{
		SmoothCD(pSkeletonPose->m_FaceAnimPosSmooth[i], pSkeletonPose->m_FaceAnimPosSmoothRate[i], pMasterCharacter->m_fOriginalDeltaTime, additionalRotPos[i].t, 0.05f);
	}

//	float fColor[4] = {0,1,0,1};
//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fColor, false,"Facial Animation" );	
//	g_YLine+=16.0f;


	/*
	if (blendBoneRotations)
	{
		g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fColor, false,"blendBoneRotations" );	g_YLine+=16.0f;
		// Blend the additional rotations - this is necessary when we are changing sequences or Look IK settings to avoid snapping.
		const float interpolationRate = 5.0f;
		float interpolateFraction = min(1.0f, max(0.0f, pMasterCharacter->m_fOriginalDeltaTime * interpolationRate));
		for (uint32 boneIndex=0; boneIndex<numJoints; ++boneIndex)
		{
			additionalRotPos[boneIndex].q.SetNlerp(additionalRotPos[boneIndex].q, additionalRotPos[boneIndex].q, interpolateFraction);
			additionalRotPos[boneIndex].t.SetLerp(additionalRotPos[boneIndex].t, additionalRotPos[boneIndex].t, interpolateFraction);
		}
	}
	else
	{
		// Usually we don't want to blend, we want the exact settings as they are in the fsq to preserve the subtleties of the animation.
		for (uint32 boneIndex=0; boneIndex<numJoints; ++boneIndex)
			additionalRotPos[boneIndex] = additionalRotPos[boneIndex];
	}*/


	QuatT temp;
	int32 head=pSkeletonPose->m_pModelSkeleton->GetIdx(eIM_HeadIdx);
	int32 leye=pSkeletonPose->m_pModelSkeleton->GetIdx(eIM_LeftEyeIdx);
	int32 reye=pSkeletonPose->m_pModelSkeleton->GetIdx(eIM_RightEyeIdx);
	if (head>0 && head < MAX_JOINT_AMOUNT)
	{
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fColor, false,"Head: %f (%f %f %f)",additionalRotPos[head].q.w,additionalRotPos[head].q.v.x,additionalRotPos[head].q.v.y,additionalRotPos[head].q.v.z );	g_YLine+=16.0f;
		LookIK_Fade[head]=1.0f-pSkeletonPose->m_LookIK().m_LookIKBlend;
		pSkeletonPose->m_LookIK().m_addHead=Quat::CreateSlerp(additionalRotPos[head].q,IDENTITY,LookIK_Fade[head]);

		if (pSkeletonPose->m_bPhysicsRelinquished)	
			additionalRotPos[head].q.SetIdentity(); //if this is a rag-doll, then don't rotate the head.
	}

	if (leye>0 && leye < MAX_JOINT_AMOUNT)
	{
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fColor, false,"LEye: %f (%f %f %f)",additionalRotPos[leye].q.w,additionalRotPos[leye].q.v.x,additionalRotPos[leye].q.v.y,additionalRotPos[leye].q.v.z);	g_YLine+=16.0f;
		LookIK_Fade[leye]=1.0f-pSkeletonPose->m_LookIK().m_LookIKBlend;
		pSkeletonPose->m_LookIK().m_addLEye = Quat::CreateSlerp(additionalRotPos[leye].q,IDENTITY,LookIK_Fade[leye]);
	}

	if (reye>0 && reye < MAX_JOINT_AMOUNT)
	{
	//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fColor, false,"REye: %f (%f %f %f)",additionalRotPos[reye].q.w,additionalRotPos[reye].q.v.x,additionalRotPos[reye].q.v.y,additionalRotPos[reye].q.v.z  );	g_YLine+=16.0f;
	//	int32 p = pSkeletonPose->m_parrModelJoints[reye].m_idxParent;
		LookIK_Fade[reye]=1.0f-pSkeletonPose->m_LookIK().m_LookIKBlend;
		pSkeletonPose->m_LookIK().m_addREye = Quat::CreateSlerp(additionalRotPos[reye].q,IDENTITY,LookIK_Fade[reye]);
	}


//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fColor, false,"ApplyFaceStateToCharacter" );	
//	g_YLine+=16.0f;

	//IVO: make sure that all facial movements are relative to the parent (in this case the head)
	uint32 numFacePos = pSkeletonPose->m_FaceAnimPosSmooth.size();
	if (numFacePos==numJoints)
	{
		for (uint32 i=1; i<numJoints; i++)
		{

			//very ugly hack to adjust the translation value.
			//the translation value is is absolute-space, but to animate faces and together with a moving head 
			//we need the data in relative bone space.
			int32 p=pSkeletonPose->m_parrModelJoints[i].m_idxParent;
			temp.q = additionalRotPos[i].q;
			temp.t = !pSkeletonPose->GetPoseDataDefault().m_jointsAbsolute[p].q * pSkeletonPose->m_FaceAnimPosSmooth[i];

			temp.q.SetNlerp(IDENTITY,temp.q,LookIK_Fade[i]);

			//	g_pIRenderer->Draw2dLabel( 1,g_YLine, 1.4f, fColor, false,"rate: %f %f %f ",pSkeletonPose->m_FaceAnimPosSmooth[i].x,pSkeletonPose->m_FaceAnimPosSmooth[i].y,pSkeletonPose->m_FaceAnimPosSmooth[i].z );	
			//	g_YLine+=16.0f;

			pSkeletonPose->GetPoseDataWriteable()->m_jointsRelative[i]	= pSkeletonPose->GetPoseDataWriteable()->m_jointsRelative[i]*temp;			
			//pSkeletonPose->m_RelativePose[i]	= pSkeletonPose->m_RelativePose[i]*additionalRotPos[i].q;			
		}
	}

	// TEMPORARY HACK TO MAKE HEADS WORK
//	pSkeletonPose->m_bHackForceAbsoluteUpdate = true;
}

//////////////////////////////////////////////////////////////////////////
void CFacialModel::ApplyAttachmentEffector( CSkinInstance *pSkinInstance,CFacialEffector *pEffector,float fWeight )
{
	const char *sAttachmentName = pEffector->GetParamString( EFE_PARAM_BONE_NAME );
	
	IAttachment *pIAttachment = pSkinInstance->GetIAttachmentManager()->GetInterfaceByName( sAttachmentName );
	if (pIAttachment)
	{
		Ang3 vRot = Ang3(pEffector->GetParamVec3( EFE_PARAM_BONE_ROT_AXIS ));
		vRot = vRot * fWeight;

		CAttachment *pCAttachment = (CAttachment*)pIAttachment;
		Quat q;
		q.SetRotationXYZ( vRot );
		pCAttachment->m_additionalRotation = pCAttachment->m_additionalRotation * q;
		m_bAttachmentChanged = true;
	}
	else
	{
		// No such attachment, look into the parent.
		CAttachment* pMasterAttachment = pSkinInstance->m_pSkinAttachment;
		if (pMasterAttachment) 
		{
			uint32 type = pMasterAttachment->GetType();
			if (pMasterAttachment->GetType() == CA_SKIN || pMasterAttachment->GetType() == CA_PART)
			{
				// Apply on parent bones.
				CCharInstance* pCharacter = pMasterAttachment->m_pAttachmentManager->m_pSkelInstance;
				IAttachment *pAttachment = pCharacter->GetIAttachmentManager()->GetInterfaceByName( sAttachmentName );
				if (pAttachment)
				{
					Ang3 vRot = Ang3(pEffector->GetParamVec3( EFE_PARAM_BONE_ROT_AXIS ));
					vRot = vRot * fWeight;

					CAttachment *pCAttachment = (CAttachment*)pAttachment;
					Quat q;
					q.SetRotationXYZ( vRot );
					pCAttachment->m_additionalRotation = pCAttachment->m_additionalRotation * q;
					m_bAttachmentChanged = true;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CFacialModel::ApplyBoneEffector( CCharInstance* pCharacterInst,CFacialEffector *pEffector,float fWeight,QuatT* additionalRotPos )
{
	const char *sBoneName = pEffector->GetParamString( EFE_PARAM_BONE_NAME );

	CAttachment* pMasterAttachment = pCharacterInst->m_pSkinAttachment;
	if (pMasterAttachment) 
	{
		uint32 type = pMasterAttachment->GetType();
		if (pMasterAttachment->GetType() == CA_SKIN || pMasterAttachment->GetType() == CA_PART)
		{
			// Apply on parent bones.
			CCharInstance* pCharacter = pMasterAttachment->m_pAttachmentManager->m_pSkelInstance;
			int nBoneId = pCharacter->m_SkeletonPose.GetJointIDByName(sBoneName);
			if (nBoneId >= 0)
			{
				Vec3 vPos = pEffector->GetParamVec3( EFE_PARAM_BONE_POS_AXIS );
				Ang3 vRot = Ang3(pEffector->GetParamVec3( EFE_PARAM_BONE_ROT_AXIS ));
				vRot = vRot * fWeight;
				vPos = vPos * fWeight;


				Quat q;
				q.SetRotationXYZ( vRot );
				additionalRotPos[nBoneId].q = additionalRotPos[nBoneId].q * q;
				additionalRotPos[nBoneId].t += vPos;
			}
			return;
		}
	}

	int nBoneId = pCharacterInst->m_SkeletonPose.GetJointIDByName(sBoneName);
	if (nBoneId >= 0)
	{
		CSkeletonPose* pSkeletonPose = 	&pCharacterInst->m_SkeletonPose;
			Vec3 vPos = pEffector->GetParamVec3( EFE_PARAM_BONE_POS_AXIS );
			Ang3 vRot = Ang3(pEffector->GetParamVec3( EFE_PARAM_BONE_ROT_AXIS ));
			vRot = vRot * fWeight;
			vPos = vPos * fWeight;

			Quat q;
			q.SetRotationXYZ( vRot );
			if (pSkeletonPose->m_LookIK().m_AllowAdditionalTransforms)
			{
				additionalRotPos[nBoneId].q = additionalRotPos[nBoneId].q * q;
				additionalRotPos[nBoneId].t += vPos;
			}
	}
	else
	{
		// No such attachment.
	}
}

size_t CFacialModel::SizeOfThis()
{
	size_t nSize(sizeof(CFacialModel));
	nSize += sizeofVector(m_effectors);
	nSize += sizeofVector(m_faceStateIndexToMorphTargetId);
	nSize += sizeofVector(m_morphTargets);

	return nSize;
}

void CFacialModel::GetMemoryUsage(ICrySizer *pSizer) const
{
	pSizer->AddObject(this, sizeof(*this));
	pSizer->AddObject(m_effectors);
	pSizer->AddObject(m_faceStateIndexToMorphTargetId);
	pSizer->AddObject(m_morphTargets);
}

#include UNIQUE_VIRTUAL_WRAPPER(IFacialModel)