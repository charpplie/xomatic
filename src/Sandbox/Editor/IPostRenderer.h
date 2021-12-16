////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name: IPostRenderer.h
//  Version:   v1.00
//  Created:   08-04-2010 by Dongjoon Kim
//  Description:
// -------------------------------------------------------------------------  
//  History: Callback functor interface for editor side customized post rendering
//
////////////////////////////////////////////////////////////////////////////


#ifndef __IPOSTRENDERER_H__
#define __IPOSTRENDERER_H__

#pragma once

class IPostRenderer
{
public:
	IPostRenderer():m_refCount(0){}

	virtual void OnPostRender() const = 0;

	void AddRef() { m_refCount++; }
	void Release() {  if (--m_refCount <= 0) delete this; }

protected:
	virtual ~IPostRenderer(){}

	int m_refCount;
};

#endif//__IPOSTRENDERER_H__