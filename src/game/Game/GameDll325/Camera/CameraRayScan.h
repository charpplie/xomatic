/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2008.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Manager for camera raycasts

-------------------------------------------------------------------------
History:
- 12:2008 : Created By Jan Müller

*************************************************************************/

#ifndef CAMERA_RAY_SCAN
#define CAMERA_RAY_SCAN

#include "IDeferredRaycastManager.h"

enum ECameraRays
{
	eRAY_CENTER = 0,
	eRAY_TOP_LEFT,
	eRAY_TOP_RIGHT,
	eRAY_BOTTOM_LEFT,
	eRAY_BOTTOM_RIGHT,
	eRAY_TOP_CENTER,
	eRAY_BOTTOM_CENTER,
	eNUM_RAYS
};

class CCameraRayScan : IDeferredRaycastReceiver
{
public:
	CCameraRayScan();
	~CCameraRayScan();

	//IDeferredRaycastReceiver
	virtual void OnDataReceived(const EventPhysRWIResult *pRWIResult);
	virtual void OnDataReceived(const EventPhysPWIResult *pPWIResult);
	virtual void OnDRWReset();
	//~IDeferredRaycastReceiver

	//update all rays
	void				ShootRays(const Vec3 &rayPos, const Vec3 &rayDir, IPhysicalEntity **pSkipEnts = NULL, int numSkipEnts = 0);
	//send one ray and save in pHit (deferred)
	void				ShootRay(const Vec3 &rayPos, const Vec3 &rayDir, ray_hit *pHit, int objTypes = g_objTypes, int geomFlags = g_geomFlags, IPhysicalEntity **pSkipEnts = NULL, int numSkipEnts = 0);
	//send one sphere and save in pDist (deferred)
	void				ShootSphere(const Vec3 &spherePos, const Vec3 &sphereDir, float fRadius, float *pDist, int objTypes = g_objTypes, int geomFlags = g_geomFlags, IPhysicalEntity **pSkipEnts = NULL, int numSkipEnts = 0);
	//get current hit
	ray_hit			*GetHit(ECameraRays nr = eNUM_RAYS);
	//get ray dir for hit
	const Vec3&	GetRayDir(ECameraRays nr) const;

private:

	static const int g_objTypes;
	static const int g_geomFlags;

	ray_hit m_rayData[eNUM_RAYS];
	Vec3		m_rayDirs[eNUM_RAYS];
	CDeferredRaycastHelper m_rayCastWrapper;
};

#endif