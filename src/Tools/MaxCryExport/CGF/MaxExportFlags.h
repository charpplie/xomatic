//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __MAXEXPORTFLAGS_H__
#define __MAXEXPORTFLAGS_H__

#include "IExportFlags.h"

class MaxExportFlags : public IExportFlags
{
public:
	MaxExportFlags(bool bShouldMergeObjects, bool bShouldWriteWeights, bool bShouldWriteVertexColours, bool bShouldAllowMultipleUVs);

	// IExportFlags
	virtual bool ShouldMergeObjects();
	virtual bool ShouldWriteWeights();
	virtual bool ShouldWriteVertexColours();
	virtual bool ShouldAllowMultipleUVs();

private:
	bool bShouldMergeObjects;
	bool bShouldWriteWeights;
	bool bShouldWriteVertexColours;
	bool bShouldAllowMultipleUVs;
};

#endif //__MAXEXPORTFLAGS_H__
