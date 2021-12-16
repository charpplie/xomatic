/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id: D3DIrradianceVolume.h,v 1.0 2008/05/19 12:14:13 AntonKaplanyan Exp wwwrun $
$DateTime$
Description:  Routine for rendering and managing of irradiance volumetric data
-------------------------------------------------------------------------
History:
- 19:5:2008   12:14 : Created by Anton Kaplanyan
*************************************************************************/

#ifndef __D3DIRRADIANCEVOLUME_H__
#define __D3DIRRADIANCEVOLUME_H__


#include <IShader.h>

// the main class that manages multiple 3D grids
class CIrradianceVolumesManager
{
	typedef std::set<CREIrradianceVolume*> IrradianceVolumeSet;
	friend class CREIrradianceVolume;
public:
	// construct/destruct object
	CIrradianceVolumesManager();
	~CIrradianceVolumesManager();

	// registration framework
	void RegisterIrradianceVolume(CREIrradianceVolume* p);
	void UnregisterIrradianceVolume(CREIrradianceVolume* p);

	// heavy method, don't call it too often
	bool IsRenderable();

	// apply GI grid to pAccRT. Note: the Render() method doesn't process it
	void RenderGI(CTexture* pDepthRT, CTexture* pNormalRT);

	// evaluate all the grids
	void Render(CTexture* pDepthRT, CTexture* pNormalRT);

	// singleton instance access point
	inline static CIrradianceVolumesManager& Instance() { return s_pInstance; }

	void UpdateReflectiveShadowmapSize(SReflectiveShadowMap& rRSM, int nWidth, int nHeight);

	CREIrradianceVolume* GetVolume(int index)
	{
		if(index < (int)m_grids.size())
		{
			IrradianceVolumeSet::iterator i = m_grids.begin();
			std::advance(i, index);
			return *i;
		}
		return NULL;
	}
	
	CREIrradianceVolume* GetGIVolumeByLight(ILightSource* pLighSource)
	{
		IrradianceVolumeSet::const_iterator itEnd = m_pGIIrradianceVolumes.end();
		for(IrradianceVolumeSet::iterator it = m_pGIIrradianceVolumes.begin();it != itEnd;++it)
			if((*it)->GetAttachedLightSource() == pLighSource)
				return *it;
		return NULL;
	}
	bool IsGIRenderable() const { return !m_pGIIrradianceVolumes.empty(); }

	// is irradiance volumes enabled
	inline const bool IsEnabled() const { return m_enabled; }

	SReflectiveShadowMap	m_RSM;
protected:

	// enable/disable VIS
	void Toggle(const bool enable);

	// destroy all resources
	void Cleanup();

	// PS3 specific signed blending mode
	void SetSignedAdditiveBlendingMode();

protected:
	// names for shaders
	CCryName 								m_semGridMatrixSemantic;
	CCryName 								m_semInvGridMatrixSemantic;
	CCryName 								m_semGridSizeSemantic;
	CCryName 								m_semInvGridSizeSemantic;
	CCryName 								m_semLightPositionSemantic;
	CCryName 								m_semLightColorSemantic;
	CCryName 								m_semCameraMatrix;
	CCryName 								m_semCameraFrustrumLB;
	CCryName 								m_semCameraFrustrumLT;
	CCryName 								m_semCameraFrustrumRB;
	CCryName 								m_semCameraFrustrumRT;
	CCryName 								m_techSolveTechName;
	CCryName 								m_techCollectTechName;
	CCryName 								m_techApplyTechName;
	CCryName 								m_techInjectLight;
	CCryName 								m_techInjectColorMap;
	CCryName 								m_techPostinjectLight;

	// is VIS enabled
	bool	m_enabled;





	D3DVertexDeclaration* 	m_R2VBVertexDeclaration;					// VD for injection of colored shadow-maps with R2VB
	
	// array of grids
	IrradianceVolumeSet			m_grids;

	IrradianceVolumeSet			m_pGIIrradianceVolumes;

	CVertexBuffer* 					m_lightInjectionPointsListVB;							// VB for light injection(8 points)/deferred application(unit cube)
	CVertexBuffer* 					m_simulateVB;															// VB for light propagation
	CVertexBuffer*					m_postinjectVB;														// VB for post-injection
	CVertexBuffer* 					m_applyVB;																// VB for deferred apply(unit cube)
	CVertexBuffer* 					m_RSMInjPointsVB;													// VB for injection of colored shadow-maps
	std::vector<SVF_P3F_T3F>	m_simulateVBPointer;	// pointer to allocated memory
	std::vector<SVF_P3F_T3F>	m_postinjectVBPointer;// pointer to allocated memory

	volatile int						m_nNextFreeId;	// id for irradiance volumes

	// singleton instance
	static CIrradianceVolumesManager s_pInstance;
};

#define IrrVolumes (CIrradianceVolumesManager::Instance())

#endif
