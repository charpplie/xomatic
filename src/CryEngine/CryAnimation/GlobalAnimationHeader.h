//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File: AnimationManager.h
//  Implementation of Animation Manager.h
//
//	History:
//	January 12, 2005: Created by Ivo Herzeg <ivo@crytek.de>
//
//////////////////////////////////////////////////////////////////////

#ifndef _CRYTEK__GAH_FLAGS
#define _CRYTEK__GAH_FLAGS


struct AnimEvents
{
	f32				m_time;							//percentage value
	string	  m_strEventName;
	string		m_strCustomParameter;
	string	  m_strBoneName;
	Vec3			m_vOffset;
	Vec3			m_vDir;
	string		m_strModelName;

	void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(m_strEventName);
		pSizer->AddObject(m_strCustomParameter);
		pSizer->AddObject(m_strBoneName);
		pSizer->AddObject(m_strModelName);
	}
};


struct GlobalAnimationHeader
{
	GlobalAnimationHeader() { m_nFlags=0;	};
	virtual ~GlobalAnimationHeader()	{	};

	ILINE uint32 IsAssetLoaded() const {return m_nFlags&CA_ASSET_LOADED;}
	ILINE void OnAssetLoaded() {m_nFlags |= CA_ASSET_LOADED;}

	ILINE uint32 IsAimpose() const {return m_nFlags&CA_AIMPOSE;}
	ILINE void OnAimpose() {m_nFlags |= CA_AIMPOSE;}

	ILINE uint32 IsAimposeUnloaded() const {return m_nFlags&CA_AIMPOSE_UNLOADED;}
	ILINE void OnAimposeUnloaded() {m_nFlags |= CA_AIMPOSE_UNLOADED;}

	ILINE void ClearAssetLoaded() {m_nFlags &= ~CA_ASSET_LOADED;}

	ILINE uint32 IsAssetCreated() const {return m_nFlags&CA_ASSET_CREATED;}
	ILINE void OnAssetCreated() {m_nFlags |= CA_ASSET_CREATED;}

	ILINE uint32 IsAssetAdditive() const { return m_nFlags&CA_ASSET_ADDITIVE; }
	ILINE void OnAssetAdditive() { m_nFlags |= CA_ASSET_ADDITIVE; }

	ILINE uint32 IsAssetCycle() const {return m_nFlags&CA_ASSET_CYCLE;}
	ILINE void OnAssetCycle() {m_nFlags |= CA_ASSET_CYCLE;}

	ILINE uint32 IsAssetLMG() const {return m_nFlags&CA_ASSET_LMG;}
	ILINE void OnAssetLMG() {m_nFlags |= CA_ASSET_LMG;}
	ILINE uint32 IsAssetLMGValid()const { return m_nFlags&CA_ASSET_LMG_VALID; }
	ILINE void OnAssetLMGValid() { m_nFlags |= CA_ASSET_LMG_VALID; }
	ILINE void InvalidateAssetLMG() { m_nFlags &= (CA_ASSET_LMG_VALID^-1); }

	ILINE uint32 IsAssetPMG() const {return m_nFlags&CA_ASSET_PMG;}
	ILINE void OnAssetPMG() {m_nFlags |= CA_ASSET_PMG;}
	ILINE uint32 IsAssetPMGValid()const { return m_nFlags&CA_ASSET_PMG_VALID; }
	ILINE void OnAssetPMGValid() { m_nFlags |= CA_ASSET_PMG_VALID; }
	ILINE void InvalidateAssetPMG() { m_nFlags &= (CA_ASSET_PMG_VALID^-1); }

	ILINE uint32 IsAssetProcessed()const { return m_nFlags&CA_ASSET_PROCESSED; }
	ILINE void OnAssetProcessed() { m_nFlags |= CA_ASSET_PROCESSED; }
	ILINE void ClearAssetProcessed() {m_nFlags &= ~CA_ASSET_PROCESSED;}

	ILINE uint32 IsAssetRequested()const { return m_nFlags&CA_ASSET_REQUESTED; }
	ILINE void OnAssetRequested() { m_nFlags |= CA_ASSET_REQUESTED; }
	ILINE void ClearAssetRequested() {m_nFlags &= ~CA_ASSET_REQUESTED;}

	ILINE uint32 IsAssetOnDemand()const { return m_nFlags&CA_ASSET_ONDEMAND; }
	ILINE void OnAssetOnDemand() { m_nFlags |= CA_ASSET_ONDEMAND; }

	ILINE uint32 IsAssetNotFound()const { return m_nFlags&CA_ASSET_NOT_FOUND; }
	ILINE void OnAssetNotFound() { m_nFlags |= CA_ASSET_NOT_FOUND; }
	ILINE void ClearAssetNotFound() {m_nFlags &= ~CA_ASSET_NOT_FOUND;}

	uint32 m_nFlags;
};



#endif
