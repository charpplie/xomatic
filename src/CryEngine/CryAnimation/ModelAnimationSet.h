////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Crytek Character Animation source code
//	
//	History:
//	10/9/2004 - Created by Ivo Herzeg <ivo@crytek.de>
//
//  Contains:
//  interface class to all motions  
/////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _CRY_MODEL_ANIMTATION_CONTAINER_HDR_
#define _CRY_MODEL_ANIMTATION_CONTAINER_HDR_

#include <VectorMap.h>
#include "ModelSkeleton.h"
#include <NameCRCHelper.h>
#include "VectorSet.h"
#include "StlUtils.h"

class CModelMesh;
class CCharacterModel;
class CryCGALoader;
class CSkeletonAnim;
struct CGlobalHeaderDBA;

struct StrafeWeights4;
struct StrafeWeights6;
struct IdleStepWeights6;

enum {
	CAF_File,
	AIM_File,
	LMG_File,
	PMG_File,
};


struct DebugJoint
{
	DebugJoint() {
		m_idxParent	=	-1;
		m_RelativeQuat.SetIdentity();	
		m_AbsoluteQuat.SetIdentity();
	}

	int16 m_idxParent;					//index of the parent-joint
	QuatT	m_RelativeQuat;		//current relative rotation and position of animation
	QuatT	m_AbsoluteQuat;
};

//! this structure contains info about loaded animations
struct ModelAnimationHeader : public CNameCRCHelper
{

	int16 m_nGlobalAnimId;
	int16 m_nAssetType;
	string m_strAimPose1[2];
	string m_strAimPose2[2];

	size_t SizeOfAnimationHeader() const 
	{	
		size_t size = sizeof(ModelAnimationHeader);
		size += m_Name.length(); 
		size += m_strAimPose1[0].capacity(); 
		size += m_strAimPose1[1].capacity(); 
		size += m_strAimPose2[0].capacity(); 
		size += m_strAimPose2[1].capacity(); 
		return size;
	}

	void GetMemoryUsage( ICrySizer *pSizer ) const 
	{			
		pSizer->AddObject(m_strAimPose1[0]);
		pSizer->AddObject(m_strAimPose1[1]);
		pSizer->AddObject(m_strAimPose2[0]);
		pSizer->AddObject(m_strAimPose2[1]);	
		CNameCRCHelper::GetMemoryUsage(pSizer);
	}

	ModelAnimationHeader() : m_nGlobalAnimId(-1),m_nAssetType(CAF_File){};
	virtual ~ModelAnimationHeader()	{}

	const char * GetAnimName() const { return GetName(); };

	void SetAnimName(const string& name) { SetNameChar(name.c_str()); };

};


//////////////////////////////////////////////////////////////////////////
// Implementation of ICryAnimationSet, holding the information about animations
// and bones for a single model. Animations also include the subclass of morph targets
//////////////////////////////////////////////////////////////////////////
class CAnimationSet : public IAnimationSet
{
public:
	CAnimationSet();
	~CAnimationSet();

	// gets called when the given animation (by global id) is unloaded.
	// the animation controls need to be unbound to free up the memory
	void Init();
	// prepares to load the specified number of CAFs by reserving the space for the controller pointers
	void prepareLoadCAFs (unsigned nReserveAnimations);
	// tries to load the animation info if isn't present in memory; returns NULL if can't load
	const ModelAnimationHeader* GetModelAnimationHeader(int32 i);

	size_t SizeOfAnimationSet() const;
	void GetMemoryUsage(ICrySizer *pSizer) const;

	// Reload animation from the file
	int StreamCAF(int nGlobalAnimID, bool asynch);
	int ReloadCAF(const char * szFileName, bool bFullPath = false);

	// Renew animations information
	int RenewCAF(const char * szFileName, bool bFullPath = false);
	int RenewCAF(int nGlobalAnimID);

	const char* GetFacialAnimationPathByName(const char* szName);
	int GetNumFacialAnimations();
	const char* GetFacialAnimationName(int index);
	const char* GetFacialAnimationPath(int index);

	// when the animinfo is given, it's used to set the values of the global animation as if the animation has already been loaded once -
	// so that the next time the anim info is available and there's no need to load the animation synchronously
	// Returns the global anim id of the file, or -1 if error
	int LoadCAF(const char * szFileName, const char * szAnimName);
	int LoadAIM(const char * szFileName, const char * szAnimName);
	int LoadANM(const char * szFileName, const char * szAnimName, std::vector<CControllerTCB>& m_LoadCurrAnimation, CryCGALoader* pCGA, uint32 unique_model_id );
	int LoadLMG(const char * szFileName, const char * szAnimName);
	int LoadLMGFromMemory(const char* resourceName, const char* szAnimName, const char* xmlData);
	int LoadPMG(const char * szFileName, const char * szAnimName);

	bool Reading_XML_for_PMG(  GlobalAnimationHeaderPMG& rGlobalAnim ); 

	int CreateOrSetAnimationLMG( const char* animationName, int globalAnimationLmgId );

	LmgAnimationStatus IsAnimationValidForLMG(const char* lmgBlendCode, const char* szAnimationName);

	IController* GAH_GetRootController(GlobalAnimationHeaderCAF& rGAH, uint32 globalID, const CCharacterModel* pModel, const char* pAnimName);

	//private:
	// This only loops over all local anims and calls the 'per-anim' function.
	// TODO: Maybe it could be removed and the 'per-anim' function called on global load or something instead.
	void ComputeSelectionProperties();
	bool ComputeSelectionProperties(int32 localAnimID);
	//public:

	void VerifyLMGs();
	void VerifyPMGs();
	int CreateDummyPMG();

	bool IsInPMGParamNameMap(const uint32 globalAnimID, const char* name) const;

	// pointer to the model
	CCharacterModel* m_pModel;	


	CNameCRCMap	m_AnimationHashMap;

	typedef std::multimap<uint32, int32> TCRCmap;
	TCRCmap m_FullPathAnimationsMap;


	struct FacialAnimationEntry
	{
		FacialAnimationEntry(const string& name, const string& path): name(name), path(path) {}
		string name;
		string path;
		operator const char*() const {return name.c_str();}

		void GetMemoryUsage( ICrySizer *pSizer ) const
		{
			pSizer->AddObject(name);
			pSizer->AddObject(path);
		}
	};

	typedef VectorSet<FacialAnimationEntry, stl::less_stricmp<FacialAnimationEntry> > FacialAnimationSet;
	FacialAnimationSet m_facialAnimations;


	//---------------------------------------------------------------------
	//---------------------------------------------------------------------
	//---------------------------------------------------------------------
	uint32 numMorphTargets() const; 
	const char* GetNameMorphTarget (int nMorphTargetId);

	// Returns the index of the animation (bone-animations and morphtargets), -1 if there's no such animation	
	int GetAnimIDByName (const char* szAnimationName);

	const char* GetNameByAnimID ( int nAnimationId);

	// for internal use only
	SPU_INDIRECT(CommandBufferExecute(M))
		uint32 GetAnimationCount() const { return m_arrAnimations.size(); }

	GlobalAnimationHeaderCAF* GetGAH_CAF(int nAnimationId);
	GlobalAnimationHeaderCAF* GetGAH_CAF(const char* AnimationName);
	GlobalAnimationHeaderAIM* GetGAH_AIM(int nAnimationId);
	GlobalAnimationHeaderAIM* GetGAH_AIM(const char* AnimationName);
	GlobalAnimationHeaderLMG* GetGAH_LMG(int nAnimationId);
	GlobalAnimationHeaderLMG* GetGAH_LMG(const char* AnimationName);
	GlobalAnimationHeaderPMG* GetGAH_PMG(int nAnimationId);
	GlobalAnimationHeaderPMG* GetGAH_PMG(const char* AnimationName);

	float GetStart (int nAnimationId);
	f32 GetSpeed(int nAnimationId);
	f32 GetSlope(int nAnimationId);
	Vec2 GetMinMaxSpeedAsset_msec(int32 animID );

	LMGCapabilities GetLMGPropertiesByName(const char* name, Vec2& vStrafeDirection, f32 fDesiredTurn, f32 fSlope  ); 

	f32 GetIWeightForSpeed(int nAnimationId, f32 Speed);
	f32 GetDuration_sec(int nAnimationId);
	uint32 GetAnimationFlags(int nAnimationId);
	uint32 GetBlendSpaceCode(int nAnimationId);
	CryAnimationPath GetAnimationPath(const char* szAnimationName); 
	const QuatT& GetAnimationStartLocation(const char* szAnimationName); 
	VIRTUAL const SAnimationSelectionProperties* GetAnimationSelectionProperties(const char* szAnimationName);

	const QuatT& GetJointStartLocation(const char* szAnimationName, int32 jointCRC32);

	void AnalyseAndModifyAnimations(const string& strPath, CCharacterModel* pModel); 
	void EvaluteGlobalAnimID();
	void ProcessAimPoses(CCharacterModel* pModel,const char* animname,GlobalAnimationHeaderAIM& rAIM,int32 nDirectionIdx,uint32 nAnimTokenCRC32); 

	void CreateSkeletonArray( std::vector< std::vector<DebugJoint> >& arrSkeletons, uint32 nAnimID );
	void SetFootBits(GlobalAnimationHeaderCAF& rGlobalAnimHeader, uint32 HowManyPoses, f32 LTS,f32 LTO,uint32 LTOE0);
	uint32 SetFootplantBitsManually( int32 nGlobalAnimID );
	void SetFootplantBitsAutomatically( std::vector< std::vector<DebugJoint> >& arrSkeletons, int32 nAnimID,int32 nGlobalAnimID, int32 lHidx,int32 rHidx,int32 lTidx,int32 rTidx, uint8 detect  );
	void SetFootplantVectors( std::vector< std::vector<DebugJoint> >& arrSkeletons, uint32 nAnimID, SFootPlant& rFootPlants, uint32 nGlobalAnimID );

	std::vector<std::vector<int> > m_arrStandupAnims;
	std::vector<string> m_arrStandupAnimTypes;

	const char* GetFilePathByName (const char* szAnimationName);
	const char* GetFilePathByID(int nAnimationId);

	int32 GetGlobalIDByName(const char* szAnimationName);
	int32 GetGlobalIDByAnimID(int nAnimationId);
	ILINE uint32 GetGlobalIDByAnimID_Fast(int nAnimationId) 
	{ 
		assert(nAnimationId>=0); 
		assert(nAnimationId<m_arrAnimations.size()); 
		return m_arrAnimations[nAnimationId].m_nGlobalAnimId; 
	};
	f32 GetClosestQuatInChannel(const char* szAnimationName,int32 JointID, const Quat& q);

	void CopyPoses(const char* strAnimName, uint32 tkey,uint32 skey, GlobalAnimationHeaderAIM& rAIM, CCharacterModel* pModel );
	void Blend_2_Poses( CModelSkeleton* pModelSkeleton, uint32 skey0, uint32 skey1, uint32 tkey, GlobalAnimationHeaderAIM& rGAH, f32 t );
	void Blend_4_Poses( CModelSkeleton* pModelSkeleton, uint32 skey0, uint32 skey1, uint32 skey2, uint32 skey3, uint32 tkey, GlobalAnimationHeaderAIM& rGAH, f32 t );
	Quat GetRotationByKey( IController* pIController, uint32 tkey, const CModelSkeleton* pModelSkeleton, uint32 j, GlobalAnimationHeaderAIM& rAIM );
	Vec3 GetPositionByKey( IController* pIController, uint32 tkey, const CModelSkeleton* pModelSkeleton, uint32 j, GlobalAnimationHeaderAIM& rAIM  );
	uint32 HasPositionChannel( IController* pIController );




	void SetDummyPMGAnimName(const uint32 globalAnimID, const char* animNameA, const char* animNameB);
	void CalculateLocatorParameters( const GlobalAnimationHeaderPMG& pmgHeader);

	FacialAnimationSet& GetFacialAnimations() {return m_facialAnimations;}

	const char *GetFnPAnimGroupName(int idx) { return (unsigned int)idx<m_arrStandupAnimTypes.size() ? m_arrStandupAnimTypes[idx]:0; }

	// Not safe method. Just direct access to m_arrAnimations
	const ModelAnimationHeader&  GetModelAnimationHeaderRef(int i)
	{
		return m_arrAnimations[i];
	}

	Vec3 PolarCoordinate(const Quat& q);
	Vec3r PolarCoordinate(const Quatr& q);
	uint32 AnnotateExamples(uint32 numPoses, QuadIndices* arrQuat);

	void ComputeAimPose( GlobalAnimationHeaderAIM& rGAH, CModelSkeleton* pModelSkeleton, QuatT* arrAbsPose, uint32 nAimPose );
	void NLerp2AimPose( CModelSkeleton* pModelSkeleton, QuatT*  arrRelPose0,QuatT*  arrRelPose1,f32 t, QuatT*  arrAbsPose );
	void Blend4AimPose( GlobalAnimationHeaderAIM& rGAH, CModelSkeleton* pModelSkeleton, int8 i0,int8 i1,int8 i2,int8 i3, const Vec4& w, QuatT* arrRelPose,QuatT* arrAbsPose );


	f32 CalculatePMGParameters(const uint32 animInd, const uint32 paramInd, const string paramName, const GlobalAnimationHeaderPMG& pmgHeader, GlobalAnimationHeaderCAF& rGAH, const int GAID, IController* pRootCtl);
	void GetPMGParamLimits(const uint32 globalAnimID, const uint32 paramInd, f32& minV, f32& maxV) const;
	uint32 GetPMGParamDim(const uint32 globalAnimID) const;
	string GetPMGParamName(const uint32 globalAnimID, const uint32 paramInd) const;
	void ComputerWeaponBonePosition(const uint32 paramInd, const GlobalAnimationHeaderPMG& pmgHeader, GlobalAnimationHeaderCAF& rGAH, int GAID, Vec3& pos);
	void ComputerAimPoseCoord(const uint32 paramInd, const GlobalAnimationHeaderPMG& pmgHeader, GlobalAnimationHeaderCAF& rGAH, int GAID, Vec3& polarCoord);

	uint32 m_CharEditMode;

	void VerifyLocomotionGroup(int animationID);
	LmgAnimationStatus IsAnimationValidForLMG(uint32 lmgBlendCode, const char* szAnimationName);

private:
	// No more direct access to vector. No more LocalIDs!!!! This is just vector of registered animations!!!
	// When animation started we need build remap controllers\bones
	DynArray<ModelAnimationHeader> m_arrAnimations;

};



#endif
