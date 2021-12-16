//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __IEXPORTSOURCE_H__
#define __IEXPORTSOURCE_H__

class IExportSourceInfo;
class ISkeleton;
class ISourceObjectArray;
class IBreakablePhysicsInfo;
class ISceneProperties;
class IExportFlags;

class IExportSource
{
public:
	virtual ~IExportSource() {}
	virtual IExportSourceInfo* GetInfo() = 0;
	virtual ISkeleton* GetSkeleton() = 0;
	virtual ISourceObjectArray* GetObjects() = 0;
	virtual IBreakablePhysicsInfo* GetBreakablePhysicsInfo() = 0;
	virtual ISceneProperties* GetSceneProperties() = 0;
	virtual IExportFlags* GetExportFlags() = 0;
};

#endif //__IEXPORTSOURCE_H__
