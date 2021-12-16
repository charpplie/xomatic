//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __ISOURCEOBJECTARRAY_H__
#define __ISOURCEOBJECTARRAY_H__

class ISourceObject;

class ISourceObjectArray
{
public:
	virtual ~ISourceObjectArray() {}

	virtual int Count() = 0;
	virtual ISourceObject* Get(int i) = 0;
};

#endif //__ISOURCEOBJECTARRAY_H__
