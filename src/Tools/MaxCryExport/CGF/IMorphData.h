//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __IMORPHDATA_H__
#define __IMORPHDATA_H__

class IMorphChannelArray;

class IMorphData
{
public:
	virtual IMorphChannelArray* GetMorphChannels() = 0;
	virtual float GetMinOffset() = 0;
};

#endif //__IMORPHDATA_H__
