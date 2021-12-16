//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __IEXPORTFLAGS_H__
#define __IEXPORTFLAGS_H__

class IExportFlags
{
public:
	virtual bool ShouldMergeObjects() = 0;
	virtual bool ShouldWriteWeights() = 0;
	virtual bool ShouldWriteVertexColours() = 0;
	virtual bool ShouldAllowMultipleUVs() = 0;
};

#endif //__IEXPORTFLAGS_H__
