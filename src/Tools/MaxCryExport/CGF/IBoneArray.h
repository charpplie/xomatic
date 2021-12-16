//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __IBONEARRAY_H__
#define __IBONEARRAY_H__

class IBone;

class IBoneArray
{
public:
	virtual ~IBoneArray() {}

	virtual int Count() = 0;
	virtual IBone* Get(int nIndex) = 0;
};

#endif //__IBONEARRAY_H__
