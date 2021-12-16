#ifndef _CRE_SCREEN_FILTER_
#define _CRE_SCREEN_FILTER_

#pragma once

//==================================================================================================
// Name: SScreenFilterRenderElementParams
// Desc: ScreenFilter render element params
// Author: James Chilvers
//==================================================================================================
struct SScreenFilterRenderElementParams
{
	SScreenFilterRenderElementParams()
	{
		centre.Set(0.0f,0.0f,0.0f);
		fovV = 0.0f;
		fovH = 0.0f;
		halfWidth = 0.0f;
		halfHeight = 0.0f;
		distanceFromCamera = 0.0f;
	}

	Vec3			centre;
	float			fovV;
	float			fovH;
	float			halfWidth;
	float			halfHeight;
	float			distanceFromCamera;
};//------------------------------------------------------------------------------------------------

//==================================================================================================
// Name: CREScreenFilter
// Desc: Screen Filter render element
// Author: James Chilvers
//==================================================================================================
class CREScreenFilter : public IREGameEffect
{
public:
	CREScreenFilter();
	~CREScreenFilter();

	// IREGameEffect interface
	void mfPrepare();
	bool mfDraw(CShader* ef, SShaderPass* sfm);
	float mfDistanceToCameraSquared(Matrix34& matInst);

	// CREScreenFilter interface
	void Initialise(CREGameEffect* reGameEffect);
	SScreenFilterRenderElementParams* GetParams() { return &m_params; }

private:

	SScreenFilterRenderElementParams		m_params;
	CREGameEffect*											m_reGameEffect;
};//------------------------------------------------------------------------------------------------

#endif // _CRE_SCREEN_FILTER_
