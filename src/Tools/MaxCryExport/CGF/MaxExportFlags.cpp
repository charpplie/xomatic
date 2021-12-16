//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "MaxExportFlags.h"

MaxExportFlags::MaxExportFlags(bool bShouldMergeObjects, bool bShouldWriteWeights, bool bShouldWriteVertexColours, bool bShouldAllowMultipleUVs)
:	bShouldMergeObjects(bShouldMergeObjects),
	bShouldWriteWeights(bShouldWriteWeights),
	bShouldWriteVertexColours(bShouldWriteVertexColours),
	bShouldAllowMultipleUVs(bShouldAllowMultipleUVs)
{
}

bool MaxExportFlags::ShouldMergeObjects()
{
	return this->bShouldMergeObjects;
}

bool MaxExportFlags::ShouldWriteWeights()
{
	return this->bShouldWriteWeights;
}

bool MaxExportFlags::ShouldWriteVertexColours()
{
	return this->bShouldWriteVertexColours;
}

bool MaxExportFlags::ShouldAllowMultipleUVs()
{
	return this->bShouldAllowMultipleUVs;
}
