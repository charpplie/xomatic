/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id: IrradianceVolumeRenderNode.h,v 1.0 2008/05/19 12:14:13 AntonKaplanyan Exp wwwrun $
$DateTime$
Description:  Routine for rendering and managing of irradiance volumetric data
-------------------------------------------------------------------------
History:
- 12:6:2008   12:14 : Created by Anton Kaplanyan
*************************************************************************/

#ifndef _IRRADIANCEVOLUME_RENDERNODE_
#define _IRRADIANCEVOLUME_RENDERNODE_

#pragma once


class CREIrradianceVolume;

class CIrradianceVolumeRenderNode : public IIrradianceVolumeRenderNode, public Cry3DEngineBase
{
	friend class CLightPropagationVolume;
protected:
	struct SIVSettings
	{
		Matrix34	m_mat;
		float			m_fDensity;						// volume density
	};
public:
	CIrradianceVolumeRenderNode();

	// implements IRenderNode
	virtual void SetMatrix(const Matrix34& mat);
	VIRTUAL void GetMatrix(Matrix34& mxGrid) const;

	virtual EERType GetRenderNodeType();
	virtual const char* GetEntityClassName() const;
	virtual const char* GetName() const;
	virtual Vec3 GetPos(bool bWorldOnly = true) const;
	virtual void Render(const SRendParams &rParam);
	virtual IPhysicalEntity* GetPhysics() const;
	virtual void SetPhysics(IPhysicalEntity*);
	virtual void SetMaterial(IMaterial* pMat);
	virtual IMaterial* GetMaterial(Vec3* pHitPos = 0);
	virtual IMaterial* GetMaterialOverride() { return m_pMaterial; }
	virtual float GetMaxViewDist();
	virtual void Precache();
	virtual void GetMemoryUsage(ICrySizer* pSizer);
	virtual const AABB GetBBox() const { return m_WSBBox; }
	virtual void SetBBox( const AABB& WSBBox ) {  }

	// implements IIrradianceVolumeRenderNode
	VIRTUAL bool TryInsertLight( const CDLight &light );
	VIRTUAL void SetDensity(const float fDensity);
	VIRTUAL float GetDensity() const { return m_settings.m_fDensity; }
	VIRTUAL void UpdateMetrics(const Matrix34& mx, const bool recursive = false);	// update grid metrics(e.g. after expansion)
	VIRTUAL bool AutoFit(const TArray<CDLight>& lightsToFit);	// tries to auto-fit grid's parameters
	VIRTUAL void EnableSpecular(const bool bEnabled);
	VIRTUAL bool IsSpecularEnabled() const;

	// tries to add a light source into the existing or new volumetric grid
	static bool TryInsertLightIntoVolumes( const CDLight &light );

private:
	typedef std::set<CIrradianceVolumeRenderNode*> IrradianceVolumeSet;

private:
	static void RegisterIrradianceVolume(CIrradianceVolumeRenderNode* p);
	static void UnregisterIrradianceVolume(CIrradianceVolumeRenderNode* p);	

	bool ValidateConditions(const SIVSettings& settings);

private:
	~CIrradianceVolumeRenderNode();

private:
	static IrradianceVolumeSet ms_volumes;
public:
	enum { nMaxGridSize = 32u };

	SIVSettings	m_settings;

	AABB	m_WSBBox;

	Vec3 m_pos;
	Vec3 m_size;

	Matrix34 m_matInv;

	_smart_ptr< IMaterial > m_pMaterial;

	int				m_nGridWidth;					// grid dimensions in texels
	int				m_nGridHeight;				// grid dimensions in texels
	int				m_nGridDepth;					// grid dimensions in texels
	int				m_nNumIterations;			// number of iterations to propagate
	Vec4			m_gridDimensions;			// grid size
	Vec3			m_gridCellSize;				// the distance between two neighbor cells
	float			m_maxGridCellSize;		// the max of m_gridCellSize components
	float			m_bIsValid;						// the validation flag

	CREIrradianceVolume* m_pRE;
	CRenderObject* m_pRO;
};


#endif // #ifndef _IRRADIANCEVOLUME_RENDERNODE_