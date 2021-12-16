//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "MaxPhysicsFrame.h"

MaxPhysicsFrame::MaxPhysicsFrame(INode* pFrameNode, INode* pFrameParentNode)
:	pFrameNode(pFrameNode),
	pFrameParentNode(pFrameParentNode)
{
}

MaxPhysicsFrame::~MaxPhysicsFrame()
{
}

void MaxPhysicsFrame::ReadFrameMatrix(float fMatrix[3][3])
{
	Matrix3 frametm = this->pFrameNode->GetNodeTM(0);
	Matrix3 parenttm = this->pFrameParentNode->GetNodeTM(0);
	for (int j = 0; j < 3; j++)
	{
		for (int k = 0; k < 3; k++)
		{
			fMatrix[j][k] = 0.0f;
			for (int l = 0; l < 3; l++)
				fMatrix[j][k] += parenttm.GetRow(j)[l] * frametm.GetRow(k)[l];
		}
	}
}
