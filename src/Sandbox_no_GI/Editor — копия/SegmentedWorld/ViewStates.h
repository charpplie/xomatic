#ifndef __MiniMap_h__
#define __MiniMap_h__

#if _MSC_VER > 1000
#pragma once
#endif

SW_NAMESPACE_BEGIN();

struct TViewStates
{
	Vec2 m_ptWorldMapOfs;
	int  m_nWorldMapZoom;
	Vec3 m_ptCamPos;
	Ang3 m_vCamAngles;

	TViewStates()
	{
		m_ptWorldMapOfs = Vec2(0,0);
		m_nWorldMapZoom = 0;
		m_ptCamPos = Vec3(1e35f);
		m_vCamAngles = Ang3(0, 0, 0);
	}
};


SW_NAMESPACE_END();

#endif // __MiniMap_h__