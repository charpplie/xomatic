////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Crytek Character Animation source code
//	
//	History:
//	10/9/2004 - Created by Ivo Herzeg <ivo@crytek.de>
//
//  Contains:
//  default-modelskeleton of the model   
/////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _CRY_MODELSKELETON
#define _CRY_MODELSKELETON

#include "Skeleton.h"

#include "ModelJoint.h"      //embedded


struct IdxAndName
{
	int32 m_idxJoint; 
	const char* m_strJoint; 
	IdxAndName()
	{
		m_idxJoint =-1;;
		m_strJoint = 0;
	}

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject( m_strJoint );
	}
};
struct IKLimbType
{
	uint64 m_nHandle;  //64-bit string 
	uint32 m_nSolver;  //32-bit string 
	uint32 m_nInterations;  //only need for iterative solvers 
	f32    m_fThreshold;    //only need for iterative solvers 
	f32    m_fStepSize;     //only need for iterative solvers 
	DynArray<IdxAndName> m_arrJointChain;
	DynArray<int16>	m_arrLimbChildren;
	DynArray<int16>	m_arrRootToEndEffector;
	IKLimbType()
	{
		m_nHandle		= 0;
		m_nSolver		= 0;
	}

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject( m_arrJointChain );
		pSizer->AddObject( m_arrLimbChildren );
		pSizer->AddObject( m_arrRootToEndEffector );
	}
};

/*
struct SJointsAimIK_Rot
{
	const char* m_strJointName;
	int16 m_nJointIdx;
	int16 m_nPosIndex;
	uint8 m_nPreEvaluate;
	uint8 m_nAdditive;
	SJointsAimIK_Rot() 
	{
		m_nPosIndex			=	-1;
		m_strJointName  = 0;
		m_nJointIdx     = -1;
	};
	//SJointsAimIK_Rot(class CModelSkeleton* pModelSkeleton, const char* pJointname, int8 nPreEvaluate, int8 nAdditive );
};

struct SJointsAimIK_Pos
{
	const char* m_strJointName;
	int16 m_nJointIdx;
	SJointsAimIK_Pos() 
	{
		m_strJointName = 0;
		m_nJointIdx    = -1;
	};
	//SJointsAimIK_Pos(class CModelSkeleton* pModelSkeleton, const char* pJointname );
};


struct DirectionalBlends
{
	string m_AnimToken;
	uint32 m_AnimTokenCRC32;
	const char* m_strParaJointName;
	int32 m_nParaJointIdx;
	const char* m_strStartJointName;
	int32 m_nStartJointIdx;
	DirectionalBlends() 
	{
		m_AnimTokenCRC32		=	0;
		m_strParaJointName	= 0;
		m_nParaJointIdx			= -1;
		m_strStartJointName	=	0;
		m_nStartJointIdx		= -1;
	};
};
*/

struct ADIKTarget
{
	uint64 m_nHandle; 
	int32 m_idxTarget; 
	const char* m_strTarget; 
	int32 m_idxWeight;
	const char* m_strWeight;
	ADIKTarget()
	{
		m_nHandle		= 0;
		m_idxTarget	= -1; 
		m_strTarget	= 0; 
		m_idxWeight	= -1;
		m_strWeight	= 0;
	}
	void GetMemoryUsage(ICrySizer *pSizer ) const {}
};

class CModelSkeleton :
	public ICharacterModelSkeleton,
	public _reference_target_t
{
	static const int ELastValue = eIM_Last;

public:

	CModelSkeleton()
	{
		memset(m_IdxArray, -1, sizeof(m_IdxArray));
	}

	void SetIKJointID();

	// Obsolete.
	uint32 GetNumJoints() const { return m_arrModelJoints.size(); };

	uint32 GetJointCount() const { return m_arrModelJoints.size(); };
	int32 GetJointIDByName(const char* strJointName) const;
	const char* GetJointNameByID(int32 nJointID) const;
	
	int32 GetJointParentIDByID(int32 nChildID) const
	{
		int32 numJoints = m_arrModelJoints.size();
		if(nChildID>=0 && nChildID<numJoints)
			return m_arrModelJoints[nChildID].m_idxParent;
		assert(0);
		return -1;
	}

	int32 GetJointIDByType(EAnimationJointType eType) const { return m_IdxArray[eType]; }

	int32 GetLimbDefinitionIdx(uint64 nHandle) const
	{
		uint32 numLimbTypes = m_IKLimbTypes.size();
		for (uint32 lt=0; lt<numLimbTypes; lt++)
		{
			if (nHandle==m_IKLimbTypes[lt].m_nHandle)
				return lt;
		}
		return -1;
	}
	const IKLimbType* GetLimbDefinition(int32 index) const { return &m_IKLimbTypes[index]; }

	CModelJoint* GetParent (uint32 i) 
	{
		int32 pidx = m_arrModelJoints[i].m_idxParent;
		if (pidx>=0)
			return &m_arrModelJoints[pidx];
		else
			return 0;
	}

	// return bone index from the index map
	inline uint16 GetIdx(EAnimationJointType eType)
	{
		return m_IdxArray[eType];
	}

	uint32 SizeOfSkeleton()
	{
		uint32 nSize = sizeof(CModelSkeleton);

		nSize += m_poseData.m_jointsRelative.get_alloc_size();
		nSize += m_poseData.m_jointsAbsolute.get_alloc_size();
		nSize += m_poseData.m_jointsStatus.get_alloc_size();
		nSize += m_arrModelJoints.get_alloc_size();

		nSize += m_AimIK_Rot.get_alloc_size();				//rotational joints used for Aim-IK
		nSize += m_AimIK_Pos.get_alloc_size();				//positional joints used for Aim-IK
		nSize += m_ADIKTargets.get_alloc_size();			//array with Animation Driven IK-Targets
		
		nSize += m_IKLimbTypes.get_alloc_size();      //array with limbs we can apply a two-bone solver
		uint32 numLimb = m_IKLimbTypes.size();
		for (uint32 i=0; i<numLimb; i++)
		{
			nSize+=m_IKLimbTypes[i].m_arrJointChain.get_alloc_size();
			nSize+=m_IKLimbTypes[i].m_arrLimbChildren.get_alloc_size();
		}

		nSize += m_arrMirrorJoints.get_alloc_size();
		return nSize;
	}

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject( m_arrModelJoints );		
		pSizer->AddObject( m_AimIK_Rot );
		pSizer->AddObject( m_AimIK_Pos );
		pSizer->AddObject( m_ADIKTargets );
		pSizer->AddObject( m_IKLimbTypes );
		pSizer->AddObject( m_arrMirrorJoints );
		pSizer->AddObject(m_IKLimbTypes);		
	}

	// helper function. return has legs model or not.
	bool IsHuman()
	{
		bool HasLegs = true;

		if (m_IdxArray[eIM_RHeelIdx] == -1) HasLegs = false;
		else
			if (m_IdxArray[eIM_LHeelIdx] == -1) HasLegs = false;
			else
				if (m_IdxArray[eIM_RToe0Idx]	== -1) HasLegs = false;
				else
					if (m_IdxArray[eIM_LToe0Idx]	== -1) HasLegs = false;

		return HasLegs;
	}

	// This is the bone hierarchy. All the bones of the hierarchy are present in this array
	DynArray<CModelJoint> m_arrModelJoints;
	Skeleton::CPoseData m_poseData;

	int16	m_IdxArray[ELastValue]; //Index list with important joints. Needed for IK

	DynArray<SJointsAimIK_Rot>	m_AimIK_Rot;						//rotational joints used for Aim-IK
	DynArray<SJointsAimIK_Pos>	m_AimIK_Pos;						//positional joints used for Aim-IK
	DynArray<DirectionalBlends> m_DirectionalBlends;	//positional joints used for Aim-IK
	DynArray<ADIKTarget>				m_ADIKTargets;					//array with Animation Driven IK-Targets
	DynArray<IKLimbType>				m_IKLimbTypes;					//array with limbs we can apply a two-bone solver
	DynArray<int16>							m_arrMirrorJoints;

}_ALIGN(128);



#endif
