////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   IRenderListener.h
//  Created:     5/08/2009 by Paulo Zaffari.
//  Description: Interface for rendering custom 3D elements in the main 
//  render viewport. Particularly usefull for debug geometries.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef IRenderListener_h__
#define IRenderListener_h__

#pragma once

struct DisplayContext;

struct __declspec( uuid("{8D52F857-1027-4346-AC7B-F620DA7CCE42}") ) IRenderListener: public IUnknown
{
		virtual void Render(DisplayContext& rDisplayContext)=0;
};

#endif // IRenderListener_h__
