/**********************************************************************
 *<
	FILE: SkinEngine.cpp

	DESCRIPTION:  Implementation of the SkinEngine

	CREATED BY: Nikolai Sander
				
	HISTORY: created 7/10/99

 *>	Copyright (c) 1999, All Rights Reserved.
 **********************************************************************/

#include "stdafx.h"
#include "MAXComponents.h"
#include "SkinEngine.h"
#include "defines.h"


// [MichaelS 14/5/2008] Needed so we can create an instance without including the header file,
// since that file clashes with the modifier code - for instance there are two classes called
// BoneDataClass...
ISkinEngine* CreateSkinEngineInstance()
{
	ISkinEngine* pSkinEngine;
	if (S_OK == CSkinEngine::CreateInstance(&pSkinEngine))
		return pSkinEngine;
	return 0;
}

// [MichaelS] Ripped from MaxCryExport.
void Matrix3_to_CryMatrix(Matrix34 &cm, Matrix3 &m3)
{
	MRow *arow=m3.GetAddr();

	cm.m00 = arow[0][0];
	cm.m10 = arow[0][1];
	cm.m20 = arow[0][2];
	cm.m01 = arow[1][0];
	cm.m11 = arow[1][1];
	cm.m21 = arow[1][2];
	cm.m02 = arow[2][0];
	cm.m12 = arow[2][1];
	cm.m22 = arow[2][2];
	cm.m03 = arow[3][0];
	cm.m13 = arow[3][1];
	cm.m23 = arow[3][2];
}

void CryMatrix_to_Matrix3(Matrix3 &m3, Matrix34 &cm)
{
	MRow *arow=m3.GetAddr();
	arow[0][0]=cm.m00; 
	arow[0][1]=cm.m10; 
	arow[0][2]=cm.m20; 
	arow[1][0]=cm.m01; 
	arow[1][1]=cm.m11; 
	arow[1][2]=cm.m21; 
	arow[2][0]=cm.m02; 
	arow[2][1]=cm.m12; 
	arow[2][2]=cm.m22; 
	arow[3][0]=cm.m03; 
	arow[3][1]=cm.m13;
	arow[3][2]=cm.m23;
}
// [MichaelS] End.

/////////////////////////////////////////////////////////////////////////////
// CSkinEngine

CSkinEngine::CSkinEngine()
{
	BoneData.SetCount(0);
	PointData.SetCount(0);
}

CSkinEngine::~CSkinEngine()
{
	BoneData.SetCount(0);
	PointData.SetCount(0);
}


STDMETHODIMP CSkinEngine::SetNumPoints(int numPoints)
{

	PointData.SetCount(numPoints);

	return S_OK;
}

STDMETHODIMP CSkinEngine::SetNumBones(int numBones)
{

	if(BoneData.Count() != numBones)
	{
		BoneData.SetCount(numBones);		
	}
	
	return S_OK;
}

STDMETHODIMP CSkinEngine::SetPointData(int pointIdx, int numData, 
										DWORD b_stride, int *BoneIndexArray, 
										DWORD w_stride, float *WeightArray, 
										DWORD sci_stride, int *SubCurveIdxArray, 
										DWORD ssi_stride, int *SubSegIdxArray, 
										DWORD ssd_stride, float *SubSegDistArray, 
										DWORD t_stride, float *TangentsArray, 
										DWORD op_stride, float *OPointsArray)
{
	// Bone Data
	PointData[pointIdx].m_BoneIndices = StrideArray<int>(BoneIndexArray, b_stride);
	PointData[pointIdx].m_BoneWeights = StrideArray<float>(WeightArray, w_stride);
	PointData[pointIdx].m_numBones = numData;
	
	// Spline Data
	PointData[pointIdx].m_SubCurveIndices = StrideArray<int>(SubCurveIdxArray, sci_stride);
	PointData[pointIdx].m_SubSegIndices = StrideArray<int>(SubSegIdxArray, ssi_stride);
	PointData[pointIdx].m_SubSegDistance = StrideArray<float>( SubSegDistArray, ssd_stride);
	PointData[pointIdx].m_Tangents = StrideArray<Point3>( (Point3 *) TangentsArray, t_stride);
	PointData[pointIdx].m_OPoints = StrideArray<Point3>( (Point3 *) OPointsArray, op_stride);


	return S_OK;
}

STDMETHODIMP CSkinEngine::SetInitBoneTM(int boneIdx, float *InitTM)
{
	BoneData[boneIdx].SetInitTM(InitTM);
	return S_OK;
}

STDMETHODIMP CSkinEngine::SetBoneTM(int boneIdx, float *currentTM)
{
	BoneData[boneIdx].SetCurrentTM(Matrix3(&currentTM[0],&currentTM[3],&currentTM[6],&currentTM[9]));
	
	return S_OK;
}

#define USE_DUAL_QUAT_SKINNING
STDMETHODIMP CSkinEngine::MapPoint(int idx, float *pin, float *pout)
{
	Point3 p(pin);
//	Point3 initP(10.f,10.f,-100.f);
//	initP.x = p[0];
//	initP.y = p[1];
//	initP.z = p[2];

	if(PointData[idx].m_numBones > 0)
	{

		Point3 tp(0.0f,0.0f,0.0f);
		float influence = 0.0f;
		if (PointData[idx].m_numBones ==1)
		{
			Point3 vec;
			
			float influ = PointData[idx].m_BoneWeights[0];
			
			int bid;
			bid = PointData[idx].m_BoneIndices[0];
			
			vec = (p*BoneData[bid].GetXFormTM(m_MeshTM));
			vec = vec - p;
			vec = vec * influ;
			p += vec;
			
			// Spline animation :
			
			if ((BoneData[bid].GetFlags() & BONE_SPLINE_FLAG) && (influ != 0.0f))
			{
				p = SplineAnimation(idx,0,p);
			}
			
			pout[0] = p[0] ; pout[1] = p[1] ; pout[2] = p[2];
			return S_OK;
		}

		// [MichaelS] Change to dual-quat skinning.
#if defined(USE_DUAL_QUAT_SKINNING)
		QuatD blendDQ(ZERO);
		QuatD FirstDQ(ZERO);
		QuatD CurrentDQ(ZERO);
		FirstDQ = BoneData[ PointData[idx].m_BoneIndices[0] ].GetXFormTMDQ(m_MeshTM) ;
		for (int j=0;j<PointData[idx].m_numBones;j++)
		{
			float influ = PointData[idx].m_BoneWeights[j];
			int bid;
			bid = PointData[idx].m_BoneIndices[j];

			if (influ != 0.0f)
			{
				CurrentDQ = BoneData[bid].GetXFormTMDQ(m_MeshTM);
				float scale = influ;
				//if (useSign)
				{
					scale = (0 < (FirstDQ.nq | CurrentDQ.nq)) ? influ : -influ;
				}

				blendDQ = blendDQ + CurrentDQ * scale;
				influence += influ;
			}
		}
		float l = blendDQ.nq.GetLength();
		if (l > 0.1)
		{
			blendDQ.nq *= 1.0f / l;
			blendDQ.dq *= 1.0f / l;
		}
		Matrix3 blendMatrix;
		CryMatrix_to_Matrix3(blendMatrix, Matrix34(blendDQ));
		tp = p * blendMatrix;
#else //defined(USE_DUAL_QUAT_SKINNING)
		for (int j=0;j<PointData[idx].m_numBones;j++)
		{
			float influ = PointData[idx].m_BoneWeights[j];
			int bid;
			bid = PointData[idx].m_BoneIndices[j];
			
			if (influ != 0.0f)
			{
				tp  += (p*BoneData[bid].GetXFormTM(m_MeshTM))*influ;
				influence += influ;
			}
		}
#endif //defined(USE_DUAL_QUAT_SKINNING)
		
		// Spline animation :
		
		for (int j=0;j<PointData[idx].m_numBones;j++)
		{
			int bid;
			bid = PointData[idx].m_BoneIndices[j];
			
			if (BoneData[bid].GetFlags() & BONE_SPLINE_FLAG) 
			{
				float influ = PointData[idx].m_BoneWeights[j];
				
				if (influ != 0.0f)
				{
					tp = SplineAnimation(idx,j,tp);
				}
			}
		}
		
		if (influence > 0.00001)
		{
			pout[0] = tp[0] ; pout[1] = tp[1] ; pout[2] = tp[2];
			return S_OK;
		}
	}
	pout[0] = p[0];
	pout[1] = p[1];
	pout[2] = p[2];
	return S_OK;
}

STDMETHODIMP CSkinEngine::SetInitTM(float *InitTM)
{
	m_MeshTM = Matrix3(&InitTM[0],&InitTM[3],&InitTM[6],&InitTM[9]);

	return S_OK;
}

STDMETHODIMP CSkinEngine::SetBoneFlags(int boneIdx, DWORD flags)
{
	BoneData[boneIdx].SetFlags(flags);
	return S_OK;
}

Point3 CSkinEngine::SplineAnimation(int vertex, int bone, Point3 p)
{
	Point3 ps(0.0f,0.0f,0.0f),pr(0.0f,0.0f,0.0f),pdef(0.0f,0.0f,0.0f),pt(0.0f,0.0f,0.0f);
	Point3 MovedU,MovedTan;
	
	int bid = PointData[vertex].m_BoneIndices[bone];
	int cid = PointData[vertex].m_SubCurveIndices[bone];
	int sid = PointData[vertex].m_SubSegIndices[bone];
	float u = PointData[vertex].m_SubSegDistance[bone];
	
	Matrix3 tm    = Inverse(m_MeshTM * Inverse(BoneData[bid].GetCurrentTM()));
	
	Fire_GetInterpCurvePiece3D(bid, cid, sid ,u, MovedU);
	MovedU = MovedU * tm;

	Fire_GetTangentPiece3D(bid, cid, sid, u, MovedTan);
	MovedTan = VectorTransform(tm,MovedTan);
	
	Point3 OPoint;
	OPoint = PointData[vertex].m_OPoints[bone] * tm;
	Point3 OTan = VectorTransform(tm,PointData[vertex].m_Tangents[bone]);
	
	float s = 1.0f;  //scale 
	float angle = 0.0f;
	float influ = PointData[vertex].m_BoneWeights[bone];
	
	OTan = Normalize(OTan);
	MovedTan = Normalize(MovedTan);
	
	float dot = 0.0f;
	
	if ( OTan != MovedTan)
	{
		dot = DotProd(OTan,MovedTan);
		if (fabs(1.0-fabs(dot)) >= 0.001f)
			angle = (float) acos(DotProd(OTan,MovedTan)) * influ;
	}
	
	Point3 perp = CrossProd(OTan,MovedTan);
	Matrix3 RotateMe(1);
	
	if (dot != 1.0f)
		RotateMe = RotAngleAxisMatrix(Normalize(perp), angle);
	
	ps = p-OPoint;
	pr = ps * RotateMe + OPoint;
	pt = (MovedU - OPoint) * influ;
	pdef = pr + pt;
	return pdef;
	
}

void BoneDataClass::SetInitTM(float *ptm)
{
	bCacheValid = false;
	bCacheValidDQ = false;
	m_InitTM = Matrix3( &ptm[0], &ptm[3], &ptm[6], &ptm[9]);
}

void BoneDataClass::SetCurrentTM(Matrix3 &tm)
{
	bCacheValid = false;
	bCacheValidDQ = false;
	m_CurrentTM = tm;
}
void BoneDataClass::SetFlags(DWORD flags)
{
	m_flags = flags;
}

Matrix3 &BoneDataClass::GetXFormTM(Matrix3 &MeshTM)
{
	if(!bCacheValid)		
	{
		m_XFormTM = MeshTM * Inverse(m_InitTM) * m_CurrentTM * Inverse(MeshTM);		
		bCacheValid = true;
	}
	
	return m_XFormTM;
}

QuatD &BoneDataClass::GetXFormTMDQ(Matrix3 &MeshTM)
{
	if(!bCacheValid)		
	{
		m_XFormTM = MeshTM * Inverse(m_InitTM) * m_CurrentTM * Inverse(MeshTM);		
		bCacheValid = true;
	}
	if(!bCacheValidDQ)		
	{
		// Convert matrix to crytek format to use crytek maths lib.
		Matrix34 m;
		Matrix3_to_CryMatrix(m, m_XFormTM);
		m_dualQuat = QuatD(m);

		bCacheValid = true;
	}
	
	return m_dualQuat;
}
