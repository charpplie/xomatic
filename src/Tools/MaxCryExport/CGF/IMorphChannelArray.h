//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __IMORPHCHANNELARRAY_H__
#define __IMORPHCHANNELARRAY_H__

class IMorphChannel;

class IMorphChannelArray
{
public:
	virtual int Count() = 0;
	virtual IMorphChannel* Get(int i) = 0;
};

#endif //__IMORPHCHANNELARRAY_H__
