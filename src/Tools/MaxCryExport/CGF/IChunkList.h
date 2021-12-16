//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __ICHUNKLIST_H__
#define __ICHUNKLIST_H__

#include "CryHeaders.h"

class IChunkList
{
public:
	virtual int Append(void *ptr, CHUNK_HEADER *ch, bool assign_ID,int flags = 0) = 0;
};

#endif //__ICHUNKLIST_H__
