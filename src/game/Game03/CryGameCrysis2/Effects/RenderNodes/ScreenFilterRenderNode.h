#ifndef _SCREEN_FILTER_RENDERNODE_
#define _SCREEN_FILTER_RENDERNODE_

#pragma once

// Includes
#include "../../CryEngine/Cry3DEngine/Cry3DEngineBase.h"
#include "Effects/GameEffectsSystem.h"

// Forward declares
class CREScreenFilter;
struct SScreenFilterRenderNodeParams;

//==================================================================================================
// Name: CScreenFilterRenderNode
// Desc: Screen filter render node
// Author: James Chilvers
//==================================================================================================
class CScreenFilterRenderNode : public IRenderNode, public Cry3DEngineBase
{
public:
	CScreenFilterRenderNode();

	// CScreenFilterRenderNode interface
	void							SetParams						(const SScreenFilterRenderNodeParams& params);
	void							UpdatePosFromCamera	();

	// IRenderNode interface
	const char*				GetName							() const;
	EERType						GetRenderNodeType		();
	const char*				GetEntityClassName	() const;

//Diesel cut
	//void							GetMemoryUsage			(ICrySizer* sizer) const; //Diesel cut

//Diesel new
	void GetMemoryStatistics(ICrySizer * s)
	{ 
		SIZER_SUBCOMPONENT_NAME(s, "ScreenFilterRenderNode");
		s->Add(*this);
	}
//Diesel new

	void							SetMaterial					(IMaterial* material);
	IMaterial*				GetMaterial					(Vec3* hitPos = NULL);
	void							SetMatrix						(const Matrix34& matrix);
	Vec3							GetPos							(bool worldOnly = true) const;
	const AABB				GetBBox							() const;
	void							SetBBox							(const AABB& worldSpaceBoundingBox);
	float							GetMaxViewDist			();
	IPhysicalEntity*	GetPhysics					() const;
	void							SetPhysics					(IPhysicalEntity* physics);
	void							Render							(const SRendParams &renderParams);
	IMaterial*				GetMaterialOverride	();

private:
	~CScreenFilterRenderNode();

	AABB										m_WSBBox;					// World space bounding box
	Matrix34								m_matrix;
	float										m_maxViewDist;
	CREGameEffect*					m_gameEffectRenderElement;
	CREScreenFilter*				m_screenFilterRenderElement;
	IMaterial*							m_material;
};//------------------------------------------------------------------------------------------------

//==================================================================================================
// Name: SScreenFilterRenderNodeParams
// Desc: Screen filter render node params
// Author: James Chilvers
//==================================================================================================
struct SScreenFilterRenderNodeParams
{
	SScreenFilterRenderNodeParams()
	{
		material = NULL;
	}

	IMaterial*	material;
	
};//------------------------------------------------------------------------------------------------

#endif // _SCREEN_FILTER_RENDERNODE_
