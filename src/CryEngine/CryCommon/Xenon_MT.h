////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   XenonSpecific.h
//  Version:     v1.00
//  Created:     24/9/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __Xenon_MT_h__
#define __Xenon_MT_h__
#pragma once

#include <Xmcore.h>

#ifdef _DEBUG
#	pragma comment(lib, "Xmcored.lib")
#else
#	pragma comment(lib, "Xmcore.lib")
#endif

//////////////////////////////////////////////////////////////////////////
// Xenon implementation.
//////////////////////////////////////////////////////////////////////////
namespace CryMT
{
	template <class T>
	class CLocklessPointerQueue
	{
	public:
		CLocklessPointerQueue() { m_lockFreeQueue.Initialize();	}
		~CLocklessPointerQueue() { m_lockFreeQueue.Destroy(); };
		bool	empty() const { return m_lockFreeQueue.IsEmpty() == TRUE; }
		bool	push( T* ptr ) { return m_lockFreeQueue.Add(ptr) == S_OK; }
		// pop can return NULL, always check for it before use.
		T*    pop() { return m_lockFreeQueue.Remove(); }

	private:
		XLockFreeQueue<T> m_lockFreeQueue;
	};
}; // namespace CryMT

//////////////////////////////////////////////////////////////////////////
#endif // __Xenon_MT_h__

