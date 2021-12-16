////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Crytek Character Animation source code
//	
//	
//  Contains:
//	This file contains just prototype-code for testing new features
//	As soon as this stuff is mature enough we will move it into production-tools (in this case the resource-compiler)
//	
//	
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CharacterManager.h"
#include "Model.h"
#include "ModelAnimationSet.h"
#include "ModelMesh.h"



const uint32 nAmount=0x200;
const char* g_MultipleFootPlantName[nAmount];
int32 g_MultipleFootPlantGlobalID[nAmount];
int32 g_MultipleFootPlantSegNo[nAmount];
int8 g_MultipleFootPlantDetectFP[nAmount];
SSegments g_MultipleFootPlantSegments[nAmount];

uint32 g_MultipleFootPlantCounter=0;


void CAnimationSet::AnalyseAndModifyAnimations(const string& strPath, CCharacterModel* pModel) 
{
	if (pModel==0)
		return;

	uint32 initialized=0;
	uint32 numGlobalAnims = g_AnimationManager.m_arrGlobalCAF.size();
	for(uint32 i=0; i<numGlobalAnims; i++)
	{
		if (g_AnimationManager.m_arrGlobalCAF[i].IsAssetProcessed())
			initialized++;
	}

	//---------------------------------------------------------------------------
	uint32 numAnims = m_arrAnimations.size();
	//this test is mainly for human-characters
	EvaluteGlobalAnimID();

	//--------------------------------------------------------------------------------
	//--------------------------------------------------------------------------------
	//--------------------------------------------------------------------------------
	int16 lheel_idx = -1;
	int16 rheel_idx = -1;
	int16 ltoe_idx = -1;
	int16 rtoe_idx = -1;

	uint32 numBones =  pModel->m_ModelSkeleton.m_arrModelJoints.size();
	for (uint32 i=0; i<numBones; i++)
	{
		const char * BoneName =  pModel->m_ModelSkeleton.m_arrModelJoints[i].GetJointName();
		if (0 == stricmp(BoneName,"Bip01 L Heel"))
			lheel_idx=i;
		if (0 == stricmp(BoneName,"Bip01 L Toe0"))
			ltoe_idx=i;

		if (0 == stricmp(BoneName,"Bip01 R Heel"))
			rheel_idx=i;
		if (0 == stricmp(BoneName,"Bip01 R Toe0"))
			rtoe_idx=i;
	}

	uint32 footskel=0;
	if (lheel_idx>=0 && rheel_idx>=0  && ltoe_idx>=0 && rtoe_idx>=0)
		footskel=1;
	if (footskel)
	{

		//-----------------------------------------------------------------------
		//-----------------------------------------------------------------------
		//---------             patch animations                     ------------
		//-----------------------------------------------------------------------
		//-----------------------------------------------------------------------
		for (uint32 i=0; i<numAnims; i++)
		{
			const char* animname= GetNameByAnimID( i );
			const ModelAnimationHeader* AnimHeader = GetModelAnimationHeader(i);
			if (AnimHeader==0)
				continue;

			uint32 GlobalID = AnimHeader->m_nGlobalAnimId;	
			GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[GlobalID];
			if (rGlobalAnimHeader.IsAssetLMG())
				continue;


			//	if (rGlobalAnimHeader.IsAssetProcessed())
			//		continue;

			int found=-1;
			for (uint32 h=0; h<g_MultipleFootPlantCounter; h++)
			{
				if (g_MultipleFootPlantGlobalID[h]==GlobalID)
				{
					found=h;
					break;
				}
			}
			if (found<0)
				continue;


			//@Ivo; Should work now
			//@Alexey: I had to revert this. It was breaking the idle-steps
			//const CModelJoint* pModelJoint = &pModel->m_ModelSkeleton.m_arrModelJoints[0];
			//IController* pController = g_AnimationManager.m_arrGlobalCAF[GlobalID].GetControllerByJointCRC32(pModelJoint[0].m_nJointCRC32);//pModelJoint[0].m_arrControllersMJoint[i];
			//if (pController)
			//	SetFootplantBitsAutomatically( g_arrSkeletons, i,GlobalID, lheel_idx,rheel_idx,ltoe_idx,rtoe_idx,ltoenub_idx,rtoenub_idx,g_MultipleFootPlantDetectFP[found]);
			rGlobalAnimHeader.m_FootPlantBits.clear();
			rGlobalAnimHeader.OnAssetProcessed();
		}
	} 
}







void CAnimationSet::EvaluteGlobalAnimID()
{
	for (uint32 a=0; a<nAmount; a++)
		g_MultipleFootPlantDetectFP[a] = 1; 

	//-------------------------------------------------------------------------------------------
	//---              for this animation we don't need footplants                            ---          
	//-------------------------------------------------------------------------------------------
	uint32 f=0;

	CharacterManager::TFootPlantTable &rTable=g_pCharacterManager->m_arrFootPlantTable;
	uint32 nFootPlants=rTable.size();
	for(f=0;f<nFootPlants;++f)
	{
		g_MultipleFootPlantName[f]=rTable[f].m_sAssetName.c_str();
		g_MultipleFootPlantSegNo[f] = rTable[f].m_segNo;
		g_MultipleFootPlantSegments[f].a	=	rTable[f].m_A;
		g_MultipleFootPlantSegments[f].b	=	rTable[f].m_B;
		g_MultipleFootPlantSegments[f].c	=	rTable[f].m_C;
		g_MultipleFootPlantSegments[f].d	=	rTable[f].m_D;
		g_MultipleFootPlantSegments[f].e	=	rTable[f].m_E;
		g_MultipleFootPlantSegments[f].f	=	rTable[f].m_F;
	}


	g_MultipleFootPlantCounter=f;

	//FILE *pOutFile=fopen("Footplants_new.xml","wt");
	//fprintf(pOutFile,"<FOOTPLANT_TABLE> <!-- These anim assets are added to the MultipleFootPlant lookup tables. -->\n");
	for (uint32 i=0; i<g_MultipleFootPlantCounter; i++)
	{
		//if(g_MultipleFootPlantName[i])
		//	fprintf(pOutFile,"\t<FOOTPLANT ASSET=\"%s\" SEG_NO=\"%d\" A=\"%0.3f\" B=\"%0.3f\" C=\"%0.3f\" D=\"%0.3f\" E=\"%0.3f\" />\n",
		//		g_MultipleFootPlantName[i],
		//		g_MultipleFootPlantSegNo[i],
		//		g_MultipleFootPlantSegments[i].a,
		//		g_MultipleFootPlantSegments[i].b,
		//		g_MultipleFootPlantSegments[i].c,
		//		g_MultipleFootPlantSegments[i].d,
		//		g_MultipleFootPlantSegments[i].e
		//		);

		g_MultipleFootPlantGlobalID[i] = -1; 
		int32 id = GetAnimIDByName(g_MultipleFootPlantName[i]);
		if (id>=0)
		{
			const ModelAnimationHeader* pAnim = GetModelAnimationHeader(id);
			if (pAnim)
			{
				uint32 GlobalAnimId = pAnim->m_nGlobalAnimId;
				g_MultipleFootPlantGlobalID[i]=GlobalAnimId;

				GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[GlobalAnimId];
				rGAH.m_Segments = g_MultipleFootPlantSegNo[i];
				rGAH.m_SegmentsTime[0] = g_MultipleFootPlantSegments[i].a;
				rGAH.m_SegmentsTime[1] = g_MultipleFootPlantSegments[i].b;
				rGAH.m_SegmentsTime[2] = g_MultipleFootPlantSegments[i].c;
				rGAH.m_SegmentsTime[3] = g_MultipleFootPlantSegments[i].d;
				rGAH.m_SegmentsTime[4] = g_MultipleFootPlantSegments[i].e;

				rGAH.m_FootPlantVectors.m_LHeelStart=-10000.0f; rGAH.m_FootPlantVectors.m_LHeelEnd=-10000.0f;
				rGAH.m_FootPlantVectors.m_LToe0Start=-10000.0f; rGAH.m_FootPlantVectors.m_LToe0End=-10000.0f;
				rGAH.m_FootPlantVectors.m_RHeelStart=-10000.0f; rGAH.m_FootPlantVectors.m_RHeelEnd=-10000.0f;
				rGAH.m_FootPlantVectors.m_RToe0Start=-10000.0f; rGAH.m_FootPlantVectors.m_RToe0End=-10000.0f;
			}
		}
	}
	//fprintf(pOutFile,"</FOOTPLANT_TABLE>\n");
	//fclose(pOutFile);
}



//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CAnimationSet::SetFootplantBitsAutomatically( std::vector< std::vector<DebugJoint> >& arrSkeletons, int32 nAnimID,int32 nGlobalAnimID,int32 lHidx,int32 rHidx,int32 lTidx,int32 rTidx, uint8 detect )
{

	CreateSkeletonArray( arrSkeletons, nAnimID);

#define FLOORDIST (0.01f)
#define SPEED_XY (0.01f)

	GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID];

	std::vector<f32> g_arrLHeelVelocity;
	std::vector<f32> g_arrLToe0Velocity;
	std::vector<f32> g_arrRHeelVelocity;
	std::vector<f32> g_arrRToe0Velocity;


	uint32 numPoses=arrSkeletons.size();

	if (lHidx>0 && rHidx>0 && lTidx>0 && rTidx>0)
	{
		//arrFootPlants.resize(numPoses);
		//nGlobalAnimID
		if (rGlobalAnimHeader.m_FootPlantBits.empty())
			rGlobalAnimHeader.m_FootPlantBits.resize(numPoses);

		assert (numPoses == rGlobalAnimHeader.m_FootPlantBits.size());
		for (uint32 i=0; i<numPoses; i++)	
			rGlobalAnimHeader.m_FootPlantBits[i]=0; //disable all of them

		if (detect==0)
			return;


		g_arrLHeelVelocity.resize(numPoses);
		g_arrRHeelVelocity.resize(numPoses);
		g_arrLToe0Velocity.resize(numPoses);
		g_arrRToe0Velocity.resize(numPoses);

		for (uint32 s=0; s<numPoses; s++) 
		{
			g_arrLHeelVelocity[s]=1000.0f;
			g_arrRHeelVelocity[s]=1000.0f;

			g_arrLToe0Velocity[s]=1000.0f;
			g_arrRToe0Velocity[s]=1000.0f;
		}


		Plane plane;
		f32 sloperad = rGlobalAnimHeader.m_fSlope;
		plane.n=Matrix33::CreateRotationX( sloperad ) * Vec3(0,0,1);
		plane.d=0;

		f32 dist=0;
		Vec3 sq; Vec3 zp;
		for (uint32 s=1; s<numPoses; s++) 
		{
			sq= arrSkeletons[s][lHidx].m_AbsoluteQuat.t - arrSkeletons[s-1][lHidx].m_AbsoluteQuat.t; sq.z=0;
			zp=(arrSkeletons[s][lHidx].m_AbsoluteQuat.t + arrSkeletons[s-1][lHidx].m_AbsoluteQuat.t )*0.5f;
			dist=(plane|zp);
			g_arrLHeelVelocity[s]=(dist<FLOORDIST) ? sq.GetLength() : 1000.0f;
			sq= arrSkeletons[s][rHidx].m_AbsoluteQuat.t - arrSkeletons[s-1][rHidx].m_AbsoluteQuat.t; sq.z=0;
			zp=(arrSkeletons[s][rHidx].m_AbsoluteQuat.t + arrSkeletons[s-1][rHidx].m_AbsoluteQuat.t )*0.5f;
			dist=(plane|zp);
			g_arrRHeelVelocity[s]=(dist<FLOORDIST) ? sq.GetLength() : 1000.0f;;

			sq= arrSkeletons[s][lTidx].m_AbsoluteQuat.t - arrSkeletons[s-1][lTidx].m_AbsoluteQuat.t; sq.z=0;
			zp=(arrSkeletons[s][lTidx].m_AbsoluteQuat.t + arrSkeletons[s-1][lTidx].m_AbsoluteQuat.t)*0.5f;
			dist=(plane|zp);
			g_arrLToe0Velocity[s]=(dist<FLOORDIST) ? sq.GetLength() : 1000.0f;
			sq= arrSkeletons[s][rTidx].m_AbsoluteQuat.t - arrSkeletons[s-1][rTidx].m_AbsoluteQuat.t; sq.z=0;
			zp=(arrSkeletons[s][rTidx].m_AbsoluteQuat.t + arrSkeletons[s-1][rTidx].m_AbsoluteQuat.t )*0.5f;
			dist=(plane|zp);
			g_arrRToe0Velocity[s]=(dist<FLOORDIST) ? sq.GetLength() : 1000.0f;
		}

		g_arrLHeelVelocity[0]=g_arrLHeelVelocity[numPoses-1];
		g_arrRHeelVelocity[0]=g_arrRHeelVelocity[numPoses-1];
		g_arrLToe0Velocity[0]=g_arrLToe0Velocity[numPoses-1];
		g_arrRToe0Velocity[0]=g_arrRToe0Velocity[numPoses-1];

		for (uint32 s=0; s<numPoses; s++) 
		{

			uint8 test = rGlobalAnimHeader.m_FootPlantBits[s];
			rGlobalAnimHeader.m_FootPlantBits[s]=0;

			if (g_arrLHeelVelocity[s]<SPEED_XY)
				rGlobalAnimHeader.m_FootPlantBits[s]|=LHEEL;
			if (g_arrRHeelVelocity[s]<SPEED_XY)
				rGlobalAnimHeader.m_FootPlantBits[s]|=RHEEL;

			if (g_arrLToe0Velocity[s]<SPEED_XY)
				rGlobalAnimHeader.m_FootPlantBits[s]|=LTOE0;
			if (g_arrRToe0Velocity[s]<SPEED_XY)
				rGlobalAnimHeader.m_FootPlantBits[s]|=RTOE0;

			//if (test != )
			//	assert(test == rGlobalAnimHeader.m_FootPlantBits[s]);
		}

	}
}








//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------
void CAnimationSet::CreateSkeletonArray( std::vector< std::vector<DebugJoint> >& arrSkeletons, uint32 nAnimID )
{
	Diag33 temp;
	const ModelAnimationHeader* pAnim = GetModelAnimationHeader(nAnimID);
	GlobalAnimationHeaderCAF& rGAH = g_AnimationManager.m_arrGlobalCAF[pAnim->m_nGlobalAnimId];

	f32 duration = rGAH.m_fEndSec - rGAH.m_fStartSec;

	f64 PosesPerKey = (1.0/60.0);
	uint32 HowManyPoses = uint32(duration/PosesPerKey);//= HowManyPoses; 
	if (HowManyPoses==0)
		HowManyPoses=1;

	//	rGAH.m_FootPlantBits.resize(HowManyPoses);

	arrSkeletons.resize(HowManyPoses);
	uint32 numJoints = m_pModel->m_ModelSkeleton.m_arrModelJoints.size();
	for(uint32 i=0; i<HowManyPoses; i++)
		arrSkeletons[i].resize(numJoints);

	const CModelJoint* pModelJoint = &m_pModel->m_ModelSkeleton.m_arrModelJoints[0];
	for (uint32 s=0; s<HowManyPoses; s++) 
	{
		//evaluate all controllers for this animation
		f32 newtime = (1.0f/HowManyPoses)*s; 
		for (uint32 j=0; j<numJoints; j++)
		{
			arrSkeletons[s][j].m_RelativeQuat=m_pModel->m_ModelSkeleton.m_poseData.m_jointsRelative[j];
			IController* pController = g_AnimationManager.m_arrGlobalCAF[pAnim->m_nGlobalAnimId].GetControllerByJointCRC32(pModelJoint[j].m_nJointCRC32); 
			if (pController)
				pController->GetOP(	rGAH.NTime2KTime(newtime), arrSkeletons[s][j].m_RelativeQuat.q, arrSkeletons[s][j].m_RelativeQuat.t);
		}

		//calculate absolute joints
		arrSkeletons[s][0].m_AbsoluteQuat = arrSkeletons[s][0].m_RelativeQuat;
		for (uint32 i=1; i<numJoints; i++)
		{
			int16 idx = arrSkeletons[s][i].m_idxParent =	m_pModel->m_ModelSkeleton.m_arrModelJoints[i].m_idxParent;
			arrSkeletons[s][i].m_AbsoluteQuat	= arrSkeletons[s][idx].m_AbsoluteQuat * arrSkeletons[s][i].m_RelativeQuat;
			arrSkeletons[s][i].m_AbsoluteQuat.q.Normalize();
		}

	}
}



//--------------------------------------------------------------------------
std::vector<ColorRGB> g_arrDistMap24;

void CAnimationSet::SetFootplantVectors( std::vector< std::vector<DebugJoint> >& arrSkeletons, uint32 nAnimID, SFootPlant& rFootPlants, uint32 nGlobalAnimID )
{

#define lm0 (10)
#define lm1 (11)
#define rm0 (13)
#define rm1 (14)

#define lx0 (16)
#define lx1 (17)
#define rx0 (18)
#define rx1 (19)


	GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID];
	int32 poses0 = rGlobalAnimHeader.m_FootPlantBits.size();

	g_arrDistMap24.resize(poses0*20);
	for (int32 i=0; i<(poses0*20); i++)
	{
		g_arrDistMap24[i].r=0;
		g_arrDistMap24[i].g=0;
		g_arrDistMap24[i].b=0;
	}

	int32 hposes=poses0/2;
	for (int32 i=0; i<poses0; i++)
	{

		uint8 FootStep = rGlobalAnimHeader.m_FootPlantBits[i];
		if (FootStep&LHEEL)
			g_arrDistMap24[i+poses0*0].b=0xff;
		if (FootStep&LTOE0)
			g_arrDistMap24[i+poses0*1].r=0xff;
		if (FootStep&LNUB0)
			g_arrDistMap24[i+poses0*2].g=0xff;

		if (FootStep&RHEEL)
			g_arrDistMap24[i+poses0*5].b=0xff;
		if (FootStep&RTOE0)
			g_arrDistMap24[i+poses0*6].r=0xff;
		if (FootStep&RNUB0)
			g_arrDistMap24[i+poses0*7].g=0xff;

		//---------------------------------------------------
		if (i<hposes)
		{
			assert((i+hposes)<poses0);
			if (FootStep&LHEEL)
				g_arrDistMap24[i+poses0*lm0+hposes].b=0xff;
			if (FootStep&LTOE0)
				g_arrDistMap24[i+poses0*lm1+hposes].r=0xff;
			if (FootStep&LNUB0)
				g_arrDistMap24[i+poses0*lm1+hposes].r=0xff;
		} 
		else 
		{
			if (FootStep&LHEEL)
				g_arrDistMap24[i+poses0*lm0-hposes].b=0xff;
			if (FootStep&LTOE0)
				g_arrDistMap24[i+poses0*lm1-hposes].r=0xff;
			if (FootStep&LNUB0)
				g_arrDistMap24[i+poses0*lm1-hposes].r=0xff;
		}

		if (FootStep&RHEEL)
			g_arrDistMap24[i+poses0*rm0].b=0xff;
		if (FootStep&RTOE0)
			g_arrDistMap24[i+poses0*rm1].r=0xff;
		if (FootStep&RNUB0)
			g_arrDistMap24[i+poses0*rm1].r=0xff;
	}		

	f32 r=0.05f;
	//delete area in footsteps
	for (f32 i=0.0f; i<r; i=i+0.001f)
	{
		//start of step
		uint32 idx0=(uint32)(i*(poses0));
		g_arrDistMap24[idx0+poses0*lm0].b=0;
		g_arrDistMap24[idx0+poses0*lm1].r=0;
		g_arrDistMap24[idx0+poses0*rm0].b=0;
		g_arrDistMap24[idx0+poses0*rm1].r=0;

		//end of step
		uint32 idx1=(uint32)( (1.0f-r+i) *(poses0));
		assert(idx1<(uint32)poses0);
		g_arrDistMap24[idx1+poses0*lm0].b=0;
		g_arrDistMap24[idx1+poses0*lm1].r=0;
		g_arrDistMap24[idx1+poses0*rm0].b=0;
		g_arrDistMap24[idx1+poses0*rm1].r=0;
	}

	int16 lhs=-1;
	int16 lhe=-1;
	int16 lts=-1;
	int16 lte=-1;
	int16 rhs=-1;
	int16 rhe=-1;
	int16 rts=-1;
	int16 rte=-1;
	for (int32 i=0,t=poses0-1; i<poses0; i++,t--)
	{
		if (lhs==-1)
			if (g_arrDistMap24[i+poses0*lm0].b==0xff) lhs=i;
		if (lts==-1)
			if (g_arrDistMap24[i+poses0*lm1].r==0xff) lts=i;
		if (lhe==-1)
			if (g_arrDistMap24[t+poses0*lm0].b==0xff) lhe=t;
		if (lte==-1)
			if (g_arrDistMap24[t+poses0*lm1].r==0xff) lte=t;

		if (rhs==-1)
			if (g_arrDistMap24[i+poses0*rm0].b==0xff) rhs=i;
		if (rts==-1)
			if (g_arrDistMap24[i+poses0*rm1].r==0xff) rts=i;
		if (rhe==-1)
			if (g_arrDistMap24[t+poses0*rm0].b==0xff) rhe=t;
		if (rte==-1)
			if (g_arrDistMap24[t+poses0*rm1].r==0xff) rte=t;
	}

	if (lhs!=-1 && lhe!=-1)
		for (int32 i=lhs; i<lhe; i++)	g_arrDistMap24[i+poses0*lm0].b=0xff;
	if (lts!=-1 && lte!=-1)
		for (int32 i=lts; i<lte; i++)	g_arrDistMap24[i+poses0*lm1].r=0xff;

	if (rhs!=-1 && rhe!=-1)
		for (int32 i=rhs; i<rhe; i++)	g_arrDistMap24[i+poses0*rm0].b=0xff;
	if (rts!=-1 && rte!=-1)
		for (int32 i=rts; i<rte; i++)	g_arrDistMap24[i+poses0*rm1].r=0xff;

	//------------------------------------------------------------------------------------

	lhs=-1;
	lhe=-1;
	lts=-1;
	lte=-1;
	rhs=-1;
	rhe=-1;
	rts=-1;
	rte=-1;
	for (int32 i=0,t=poses0-1; i<poses0; i++,t--)
	{
		if (lhs==-1)
			if (g_arrDistMap24[i+poses0*lm0].b==0xff) lhs=i;
		if (lts==-1)
			if (g_arrDistMap24[i+poses0*lm1].r==0xff) lts=i;
		if (lhe==-1)
			if (g_arrDistMap24[t+poses0*lm0].b==0xff) lhe=t;
		if (lte==-1)
			if (g_arrDistMap24[t+poses0*lm1].r==0xff) lte=t;

		if (rhs==-1)
			if (g_arrDistMap24[i+poses0*rm0].b==0xff) rhs=i;
		if (rts==-1)
			if (g_arrDistMap24[i+poses0*rm1].r==0xff) rts=i;
		if (rhe==-1)
			if (g_arrDistMap24[t+poses0*rm0].b==0xff) rhe=t;
		if (rte==-1)
			if (g_arrDistMap24[t+poses0*rm1].r==0xff) rte=t;
	}

	if (lhs!=-1 && lhe!=-1 && rhs!=-1 && rhe!=-1)
	{
		if ( (lhe-hposes)>rhs )
		{
			int32 slh=((lhe-hposes)+rhs)/2;
			g_arrDistMap24[slh+hposes+poses0*lm0].g=0xff;	//left heel start
			g_arrDistMap24[slh+poses0*rm0].g=0x3f;	//right heel end
			for (int32 i=(slh+hposes); i<poses0; i++)
				g_arrDistMap24[i+poses0*lm0].b=0;	//left heel start
			for (int32 i=0; i<slh; i++)
				g_arrDistMap24[i+poses0*rm0].b=0;	//right heel start
		}

		if ( (lhs+hposes)<rhe )
		{
			int32 elh=((lhs+hposes)+rhe)/2;
			g_arrDistMap24[elh-hposes+poses0*lm0].g=0x7f;	//left heel start
			g_arrDistMap24[elh+poses0*rm0].g=0x3f;	//right heel end
			for (int32 i=0; i<(elh-hposes); i++)
				g_arrDistMap24[i+poses0*lm0].b=0;	//left heel start
			for (int32 i=elh; i<poses0; i++)
				g_arrDistMap24[i+poses0*rm0].b=0;	//right heel end
		}
	}

	if (lts!=-1 && lte!=-1 && rts!=-1 && rte!=-1)
	{
		if ( (lte-hposes)>rts )
		{
			int32 slt=((lte-hposes)+rts)/2;
			//g_arrDistMap24[slt+hposes+poses0*lm1].g=0xff;	//right toe start
			//g_arrDistMap24[slt+poses0*rm1].g=0x3f;	//left toe start
			for (int32 i=(slt+hposes); i<poses0; i++)
				g_arrDistMap24[i+poses0*lm1].r=0;	//right toe start
			for (int32 i=0; i<slt; i++)
				g_arrDistMap24[i+poses0*rm1].r=0;	//left toe start
		}

		if ( (lts+hposes)<rte )
		{
			int32 elt=((lts+hposes)+rte)/2;
			//g_arrDistMap24[elt-hposes+poses0*lm1].g=0x7f;	//left toe start
			//g_arrDistMap24[elt+poses0*rm1].g=0x3f;	//right toe end
			for (int32 i=0; i<(elt-hposes); i++)
				g_arrDistMap24[i+poses0*lm1].r=0;	//left toe start
			for (int32 i=elt; i<poses0; i++)
				g_arrDistMap24[i+poses0*rm1].r=0;	//right toe end
		}
	}

	//-----------------------------------------------------------------------------

	for (int32 i=0; i<poses0; i++)
	{
		if (i<hposes)
		{
			g_arrDistMap24[i+poses0*lx0+hposes].b=g_arrDistMap24[i+poses0*lm0].b;	//left toe start
			g_arrDistMap24[i+poses0*lx1+hposes].r=g_arrDistMap24[i+poses0*lm1].r;	//left toe start
		}
		else
		{
			g_arrDistMap24[i+poses0*lx0-hposes].b=g_arrDistMap24[i+poses0*lm0].b;	//left toe start
			g_arrDistMap24[i+poses0*lx1-hposes].r=g_arrDistMap24[i+poses0*lm1].r;	//left toe start
		}

		g_arrDistMap24[i+poses0*rx0].b=g_arrDistMap24[i+poses0*rm0].b;	//left toe start
		g_arrDistMap24[i+poses0*rx1].r=g_arrDistMap24[i+poses0*rm1].r;	//left toe start
	}

	for (int32 i=0; i<poses0; i++)
	{
		uint32 footplant=0;
		if (g_arrDistMap24[i+poses0*lx0].b==0xff)
			footplant|=LHEEL;
		if (g_arrDistMap24[i+poses0*lx1].r==0xff)
			footplant|=LTOE0;
		if (g_arrDistMap24[i+poses0*rx0].b==0xff)
			footplant|=RHEEL;
		if (g_arrDistMap24[i+poses0*rx1].r==0xff)
			footplant|=RTOE0;

		//		if (rGlobalAnimHeader.m_FootPlantBits[i]!=footplant)
		rGlobalAnimHeader.m_FootPlantBits[i]=footplant;

	}

	//------------------------------------------------------------------

	lhs=-1;
	lhe=-1;
	lts=-1;
	lte=-1;
	rhs=-1;
	rhe=-1;
	rts=-1;
	rte=-1;
	for (int32 i=0,t=poses0-1; i<poses0; i++,t--)
	{
		if (lhs==-1)
			if (g_arrDistMap24[i+poses0*lm0].b==0xff) lhs=i;
		if (lts==-1)
			if (g_arrDistMap24[i+poses0*lm1].r==0xff) lts=i;
		if (lhe==-1)
			if (g_arrDistMap24[t+poses0*lm0].b==0xff) lhe=t;
		if (lte==-1)
			if (g_arrDistMap24[t+poses0*lm1].r==0xff) lte=t;

		if (rhs==-1)
			if (g_arrDistMap24[i+poses0*rm0].b==0xff) rhs=i;
		if (rts==-1)
			if (g_arrDistMap24[i+poses0*rm1].r==0xff) rts=i;
		if (rhe==-1)
			if (g_arrDistMap24[t+poses0*rm0].b==0xff) rhe=t;
		if (rte==-1)
			if (g_arrDistMap24[t+poses0*rm1].r==0xff) rte=t;
	}
/*

	f32 pose = f32(poses0);


	f32 oldv = rFootPlants.m_LHeelStart;
	if (lhs!=-1)
	{
		newv = rFootPlants.m_LHeelStart	=	lhs/pose;
		//assert(fabs(newv - oldv) < 0.000001f);
	}

	oldv = rFootPlants.m_LHeelEnd;
	if (lhe!=-1)
	{
		newv = rFootPlants.m_LHeelEnd		=	lhe/pose;
		//assert(fabs(newv - oldv) < 0.000001f);
	}

	oldv = rFootPlants.m_LToe0Start;
	if (lts!=-1)
	{
		newv = rFootPlants.m_LToe0Start	=	lts/pose;
		//assert(fabs(newv - oldv) < 0.000001f);
	}

	oldv = rFootPlants.m_LToe0End;
	if (lte!=-1)
	{
		newv = rFootPlants.m_LToe0End		=	lte/pose;
	}

	oldv = rFootPlants.m_RHeelStart;
	if (rhs!=-1)
	{
		newv = rFootPlants.m_RHeelStart	=	rhs/pose;;
		//		assert(fabs(newv - oldv) < 0.000001f);
	}

	oldv = rFootPlants.m_RHeelEnd;
	if (rhe!=-1)
	{
		newv = rFootPlants.m_RHeelEnd	=	rhe/pose;;
		//		assert(fabs(newv - oldv) < 0.000001f);
	}

	oldv = rFootPlants.m_RToe0Start;
	if (rts!=-1)
	{
		newv = rFootPlants.m_RToe0Start	=	rts/pose;;
		//		assert(fabs(newv - oldv) < 0.000001f);
	}

	oldv = rFootPlants.m_RToe0End;
	if (rte!=-1)
	{
		newv = rFootPlants.m_RToe0End		=	rte/pose;;
		//		assert(fabs(newv - oldv) < 0.000001f);
	}

*/


}














//--------------------------------------------------------------------------
#define TO_HEX(r,g,b,a) ( (uint32) (((uint8)(r)|((uint16)((uint8)(g))<<8))|(((uint32)(uint8)(b))<<16)) | (((uint32)(uint8)(a))<<24) )

uint32 CAnimationSet::SetFootplantBitsManually( int32 nGlobalAnimID )
{
	GlobalAnimationHeaderCAF& rGlobalAnimHeader = g_AnimationManager.m_arrGlobalCAF[nGlobalAnimID];
	f32 duration = rGlobalAnimHeader.m_fEndSec - rGlobalAnimHeader.m_fStartSec;
	f64 PosesPerKey = (1.0/60.0);
	uint32 HowManyPoses = uint32(duration/PosesPerKey);//= HowManyPoses; 
	if (HowManyPoses==0)
		HowManyPoses=1;


	f32 LTS=-1.0f; f32 LHS=-1.0f;	f32 LTO=-1.0f;	f32 LHO=-1.0f;
	f32 RTS=-1.0f; f32 RHS=-1.0f;	f32 RTO=-1.0f;	f32 RHO=-1.0f;
	uint32 footevents=0;

	uint32 numEvents = rGlobalAnimHeader.m_AnimEventsCAF.size();
	for (uint32 i=0; i<numEvents; i++ )
	{
		const char* AEvent = rGlobalAnimHeader.m_AnimEventsCAF[i].m_strEventName.c_str();
		switch( *(uint32*)AEvent )
		{

		case TO_HEX('#','L','T','S'):
			LTS=rGlobalAnimHeader.m_AnimEventsCAF[i].m_time;
			footevents|=0x01;
			break;
		case TO_HEX('#','L','H','S'):
			LHS=rGlobalAnimHeader.m_AnimEventsCAF[i].m_time;
			footevents|=0x02;
			break;
		case TO_HEX('#','L','T','O'):
			LTO=rGlobalAnimHeader.m_AnimEventsCAF[i].m_time;
			footevents|=0x04;
			break;
		case TO_HEX('#','L','H','O'):
			LHO=rGlobalAnimHeader.m_AnimEventsCAF[i].m_time;
			footevents|=0x08;
			break;

		case TO_HEX('#','R','T','S'):
			RTS=rGlobalAnimHeader.m_AnimEventsCAF[i].m_time;
			footevents|=0x10;
			break;
		case TO_HEX('#','R','H','S'):
			RHS=rGlobalAnimHeader.m_AnimEventsCAF[i].m_time;
			footevents|=0x20;
			break;
		case TO_HEX('#','R','T','O'):
			RTO=rGlobalAnimHeader.m_AnimEventsCAF[i].m_time;
			footevents|=0x40;
			break;
		case TO_HEX('#','R','H','O'):
			RHO=rGlobalAnimHeader.m_AnimEventsCAF[i].m_time;
			footevents|=0x80;
			break;

		}
	}

	//-------------------------------------------------------------					

	if (footevents != 0xff)
		return 0;

	rGlobalAnimHeader.m_FootPlantBits.resize(HowManyPoses);
	for (uint32 i=0; i<HowManyPoses; i++)	
		rGlobalAnimHeader.m_FootPlantBits[i]=0;

	SetFootBits(rGlobalAnimHeader,HowManyPoses,LTS,LTO,LTOE0);
	SetFootBits(rGlobalAnimHeader,HowManyPoses,LHS,LHO,LHEEL);

	SetFootBits(rGlobalAnimHeader,HowManyPoses,RTS,RTO,RTOE0);
	SetFootBits(rGlobalAnimHeader,HowManyPoses,RHS,RHO,RHEEL);


	/*	for (uint32 s=0; s<numPoses; s++) 
	{
	if (g_arrLHeelVelocity[s]<SPEED_XY)
	rGlobalAnimHeader.m_FootPlantBits[s]|=LHEEL;
	if (g_arrRHeelVelocity[s]<SPEED_XY)
	rGlobalAnimHeader.m_FootPlantBits[s]|=RHEEL;

	if (g_arrLToe0Velocity[s]<SPEED_XY)
	rGlobalAnimHeader.m_FootPlantBits[s]|=LTOE0;
	if (g_arrRToe0Velocity[s]<SPEED_XY)
	rGlobalAnimHeader.m_FootPlantBits[s]|=RTOE0;

	if (g_arrLToeNub0Velocity[s]<SPEED_XY)
	rGlobalAnimHeader.m_FootPlantBits[s]|=LNUB0;
	if (g_arrRToeNub0Velocity[s]<SPEED_XY)
	rGlobalAnimHeader.m_FootPlantBits[s]|=RNUB0;
	}*/


	return 1;
}


void CAnimationSet::SetFootBits(GlobalAnimationHeaderCAF& rGlobalAnimHeader, uint32 HowManyPoses,f32 LTS,f32 LTO, uint32 FootBit)
{
	f32 frequency = 0.5f/HowManyPoses;
	if (LTS<=LTO)
		for (f32 t=LTS; t<LTO; t+=frequency )
		{
			uint32 p = uint32(t*HowManyPoses);
			if (p>=HowManyPoses)
				p=HowManyPoses-1;
			rGlobalAnimHeader.m_FootPlantBits[p] |= FootBit;
		}
	else
	{
		for (f32 t=0; t<LTO; t+=frequency )
		{
			uint32 p = uint32(t*HowManyPoses);
			if (p>=HowManyPoses)
				p=HowManyPoses-1;
			rGlobalAnimHeader.m_FootPlantBits[p] |= FootBit;
		}
		for (f32 t=LTS; t<1.0f; t+=frequency )
		{
			uint32 p = uint32(t*HowManyPoses);
			if (p>=HowManyPoses)
				p=HowManyPoses-1;
			rGlobalAnimHeader.m_FootPlantBits[p] |= FootBit;
		}

	}
}

//--------------------------------------------------------------------------

