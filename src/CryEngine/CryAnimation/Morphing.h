//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File:Morphing.cpp
//  Implementation of the Morphing class
//
//	History:
//	June 16, 2007: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////

#ifndef _CRY_MORPH_HDR_
#define _CRY_MORPH_HDR_

#include "ModelMesh.h"
class CFacialInstance;

//////////////////////////////////////////////////////////////////////////
// the decal structure describing the realized decal with the geometry
// it consists of primary faces/vertices (with the full set of getter methods declared with the special macro)
// and helper faces/vertices. Primary faces index into the array of primary vertices. Primary vertices
// refer to the indices of the character vertices and contain the new UVs
// Helper vertices are explicitly set in the object CS, and helper faces index within those helper vertex array.
class CMorphing : public IMorphing
{
public:

	CMorphing() { m_bInstantMorphsApply=false; }
	void InitMorphing( CCharacterModel* m_pInstance );




	//-------------------------------------------------------------------------
	//-----------                morph-targets                   --------------
	//-------------------------------------------------------------------------
  //! returns morphs count
	VIRTUAL size_t GetNumMorphs() const;
  //! returns morph name by index
	VIRTUAL const char* GetMorphName(size_t index) const;
	//! Start the specified by parameters morph target
	void StartMorph (const char* szMorphTarget,const CryCharMorphParams& params);
	//! Start the morph target
	void StartMorph (int nMorphTargetId, const CryCharMorphParams& params);
	void RunMorph (int nMorphTargetId, const CryCharMorphParams&Params);
	int32 RunMorph (const char* szMorphTarget,const CryCharMorphParams&Params,bool bShowNotFoundWarning=true);

	// returns true if calling Morph () is really necessary (there are any pending morphs)
	bool NeedMorph () {	return !m_arrMorphEffectors.empty();	}

	//! Set morph speed scale
	//! Finds the morph target with the given id, sets its morphing speed and returns true;
	//! if there's no such morph target currently playing, returns false
	bool SetMorphSpeed (const char* szMorphTarget, f32 fSpeed);

	//! freezes all currently playing morphs at the point they're at
	void FreezeAllMorphs();

	//returns the morph target effector, or NULL if no such effector found
	CryModEffMorph* GetMorphTarget (int nMorphTargetId);

	//! Stops morph by target id
	bool StopMorph (const char* szMorphTarget);

	//! Finds the morph with the given id and sets its relative time.
	//! Returns false if the operation can't be performed (no morph)
	//! The default implementation for objects that don't implement morph targets is to always fail
	bool SetMorphTime (int nMorphTargetId, f32 fTime);
	bool SetMorphTime (const char* szMorphTarget, f32 fTime);
	void UpdateMorphEffectors (CFacialInstance* pFacialInstance, f32 fDeltaTimeSec, const QuatTS& rAnimLocationNext, float fZoomAdjustedDistanceFromCamera );
	void StopAllMorphs();
	void SetLinearMorphSequence(f32 i) { m_LinearMorphSequence=i; };
  void SetInstantMorphsApply(bool value) { m_bInstantMorphsApply=value; }

	size_t SizeOfThis() 
	{
		return m_arrMorphEffectors.capacity()*sizeof(CryModEffMorph) + m_morphTargetsState.capacity()*sizeof(SVF_P3F);
	}

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject( m_arrMorphEffectors );
		pSizer->AddObject( m_morphTargetsState );
	}

	DynArray<CryModEffMorph> m_arrMorphEffectors;
	DynArray<SVF_P3F> m_morphTargetsState;
	f32 m_fMorphVertexDrag;	//This is an array with number of vertices matching to num vertices in model.
	f32 m_LinearMorphSequence;
  bool  m_bInstantMorphsApply;
	CCharacterModel_AutoPtr m_pModel;
};










#endif
