#ifndef __CoverSurfaceManager_h__
#define __CoverSurfaceManager_h__

#pragma once

#include <ICoverSystem.h>


class CAICoverSurface;
class CCoverSurfaceManager
{
public:
	CCoverSurfaceManager();
	virtual ~CCoverSurfaceManager();

	bool WriteToFile(const char* fileName);
	bool ReadFromFile(const char* fileName);

	void ClearGameSurfaces();
	void AddSurfaceObject(CAICoverSurface* surface);
	void RemoveSurfaceObject(CAICoverSurface* surface);

	typedef std::set<CAICoverSurface*> SurfaceObjects;
	const SurfaceObjects& GetSurfaceObjects() const;

private:
	SurfaceObjects m_surfaceObjects;
};

#endif