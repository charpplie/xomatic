#pragma once
////////////////////////////////////////////////////////////////////////////
//  Crytek Engine Source File.
//  (c) 2001 - 2012 Crytek GmbH
// -------------------------------------------------------------------------
//  File name:   BrushArgumentEx.h
//  Created:     14/July/2012 by Jaesik.
////////////////////////////////////////////////////////////////////////////

#include "BrushArgument.h"

class CBrushArgumentEx : public CBrushArgument
{
public:

	CBrushArgumentEx(
		CBrushRegion* pRegion,
		const BrushFloat& fScale,
		CBaseObject* pObject,
		std::vector<CBrushRegion::RegionPtr>* perpendicularRegions,
		CBrushDesignerDB* pDB );
	virtual ~CBrushArgumentEx(){}

	void Update(EBrushArgumentUpdate updateOp);

private:

	void AddCapRegions();

private:

	BrushFloat m_fScale;

};