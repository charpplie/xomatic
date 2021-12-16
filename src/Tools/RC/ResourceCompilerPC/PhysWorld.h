//////////////////////////////////////////////////////////////////////
//
//  CryEngine Source code
//	
//	File:PhysWorld.h
//  Declaration and implementation of the class CPhysWorldLoader
//
//	History:
//
//////////////////////////////////////////////////////////////////////
#ifndef _PHYSWORLD_HDR_
#define _PHYSWORLD_HDR_

#include <primitives.h>
#include <physinterface.h>

class CPhysWorldLoader {
public:
	CPhysWorldLoader() 
	{
		m_hPhysics = ::LoadLibrary("CryPhysics");
		if (!m_hPhysics)
		{
			MessageBox( NULL,"CryPhysics.dll failed to load","Error",MB_OK|MB_ICONERROR );
		}
		m_pPhysWorld = 0;
		if(!m_hPhysics)
			return;
		IPhysicalWorld *(*pfnCreatePhysicalWorld)(void *pSystem) = (IPhysicalWorld*(*)(void*)) ::GetProcAddress(m_hPhysics,"CreatePhysicalWorld");
		if(!(m_pPhysWorld = pfnCreatePhysicalWorld(0)))
			return;
		m_pPhysWorld->Init();
	}
	~CPhysWorldLoader()
	{
		if (m_pPhysWorld)
			m_pPhysWorld->Release();
		::FreeLibrary(m_hPhysics);
	}

	IPhysicalWorld *GetWorldPtr() { return m_pPhysWorld; }

private:
	IPhysicalWorld *m_pPhysWorld;
	HMODULE m_hPhysics;
};

#endif
