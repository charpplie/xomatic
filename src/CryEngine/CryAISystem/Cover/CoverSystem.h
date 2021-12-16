#ifndef __CoverSystem_h__
#define __CoverSystem_h__


#pragma once


#include "Cover.h"
#include "CoverSurface.h"


class CCoverSystem
	: public ICoverSystem
{
	static const uint32 PreallocatedDynamicCount = 32;
public:
	CCoverSystem();

	// ICoverSystem
	virtual ICoverSampler* CreateCoverSampler(const char* samplerName = "default");

	virtual void Reset();
	virtual void Clear();
	virtual bool ReadSurfacesFromFile(const char* fileName);

	virtual CoverSurfaceID AddSurface(const SurfaceInfo& surfaceInfo);
	virtual void RemoveSurface(const CoverSurfaceID& surfaceID);
	virtual void UpdateSurface(const CoverSurfaceID& surfaceID, const SurfaceInfo& surfaceInfo);

	virtual uint32 GetSurfaceCount() const;
	virtual bool GetSurfaceInfo(const CoverSurfaceID& surfaceID, SurfaceInfo* surfaceInfo) const;

	virtual void DrawSurface(const CoverSurfaceID& surfaceID);
	//~ICoverSystem

	void DebugDraw();

private:
	typedef std::vector<CoverSurface> Surfaces;
	Surfaces m_surfaces;

	uint32 m_staticCount;
	typedef std::vector<CoverSurfaceID> FreeIDs;
	FreeIDs m_freeIDs;
};


#endif //__CoverSystem_h__