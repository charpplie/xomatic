////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2006.
// -------------------------------------------------------------------------
//  File name:   CompressionController.h
//  Version:     v1.00
//  Created:     6/22/2004 by Alexey Medvedev.
//  Compilers:   Visual Studio.NET 2005
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef _COMPRESSION_CONTROLLER
#define _COMPRESSION_CONTROLLER

#include "QuatQuantization.h"

class IController;
struct GlobalAnimationHeaderCAF;


struct BaseCompressedQuat
{
	virtual void FromQuat(Quat& q) = 0;
	virtual Quat ToQuat() = 0;
};

struct BaseCompressedVec3
{
	virtual void FromVec3(Vec3& v) = 0;
	virtual Vec3 ToVec3() = 0;
};

template<class _Realization>
struct CompressedQuatImpl : public BaseCompressedQuat
{

	virtual void FromQuat(Quat& q)
	{	
		val.ToInternalType(q);
	}

	virtual Quat ToQuat()
	{
		return val.ToExternalType();
	}

private:
	_Realization val;
};

typedef CompressedQuatImpl<NoCompressQuat> TNoCompressedQuat;
typedef CompressedQuatImpl<ShotInt3Quat> TShortInt3CompressedQuat;
typedef CompressedQuatImpl<SmallTreeDWORDQuat> TSmallTreeDWORDCompressedQuat;
typedef CompressedQuatImpl<SmallTree48BitQuat> TSmallTree48BitCompressedQuat;
typedef CompressedQuatImpl<SmallTree64BitQuat> TSmallTree64BitCompressedQuat;
typedef CompressedQuatImpl<SmallTree64BitExtQuat> TSmallTree64BitExtCompressedQuat;
typedef CompressedQuatImpl<PolarQuat> TPolarQuatQuat;

template<class _Realization>
struct CompressedVec3Impl : public BaseCompressedVec3
{

	virtual void FromVec3(Vec3& q)
	{	
		val.ToInternalType(q);
	}

	virtual Vec3 ToVec3()
	{
		return val.ToExternalType();
	}

private:
	_Realization val;
};

typedef CompressedVec3Impl<NoCompressVec3> TBaseCompressedVec3;

enum ECompressionState
{
	eNoCompression,
	eCompression,
	eSkipTrack,
	eUseOld
};

struct CompressionLevelInfo
{
	float m_PositionError;
	float m_RotationError;
	float m_ScaleError;
	uint32 m_PositionFormat;
	uint32 m_RotationFormat;
	uint32 m_ScaleFormat;
	ECompressionState m_bRotCompression; // O - no compression
	ECompressionState m_bPosCompression;
	ECompressionState m_bSclCompression;

	uint32 m_DeletePos;
	uint32 m_DeleteRot;


	uint32 boneLevel;

};

struct CompressionInfo
{
	static bool CompareInfo(const CompressionLevelInfo& a, uint32 b)
	{
		return a.boneLevel < b;		
	}

	CompressionLevelInfo* GetInfo(uint32 boneId)
	{
		if (boneId < m_Info.size())
			return &m_Info[boneId];

		return 0;
	}

	std::vector<CompressionLevelInfo> m_Info;
};

class CCompressonator
{
public:
	void CreateCompression(ConvertContext& cc, GlobalAnimationHeaderCAF& header, GlobalAnimationHeaderCAF& newheader, CompressionInfo& info, CSkeletonInfo * skeleton, SAnimationDesc& desc, int GAID, uint32 PrintDebugText, uint32 nCompressRoot  );
	void CreateCompression_OverrideAsset(ConvertContext& cc, GlobalAnimationHeaderCAF& header, GlobalAnimationHeaderCAF& newheader, CompressionInfo& info, CSkeletonInfo * skeleton, SAnimationDesc& desc, int GAID, uint32 PrintDebugText, uint32 nCompressRoot  );
	void CreateCompression_AdditiveAsset(ConvertContext& cc, GlobalAnimationHeaderCAF& header, GlobalAnimationHeaderCAF& newheader, CompressionInfo& info, CSkeletonInfo * skeleton, SAnimationDesc& desc, int GAID, uint32 PrintDebugText, uint32 nCompressRoot  );
	void FillTrackInfo(GlobalAnimationHeaderCAF &header, uint32 c, uint32 &curControllerId, std::vector<Quat>& Rotations, std::vector<Vec3>& Positions, std::vector<int>& RotTimes, std::vector<int>& PosTimes);
	void CreateWaveletCompression(GlobalAnimationHeaderCAF& header, GlobalAnimationHeaderCAF& newheader, CompressionInfo& info);
	void CompressRotations(CController * pController, CompressionLevelInfo * pInfo, bool bAddFirst);
	void CompressPositions(CController * pController, CompressionLevelInfo * pInfo, bool bAddFirst);
public:
	std::vector<Quat> m_Rotations;
	std::vector<Vec3> m_Positions;
	std::vector<Vec3> m_Scales;
	std::vector<int> m_PosTimes;
	std::vector<int> m_RotTimes;
	float m_fTimeScale; 
};




#endif