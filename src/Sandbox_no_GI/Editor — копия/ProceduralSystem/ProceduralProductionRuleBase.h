///////////////////////////////////////////////
// Procedural Buildings Base Rule Module Generation Class
#ifndef __proceduralproductionrulebase_h__
#define __proceduralproductionrulebase_h__
#pragma once

#include "ProceduralSystem/ProceduralGeneration.h"

//////////////////////////////////////////////////////////////////////////
class CProceduralProductionRuleBase
{
public:
	virtual void GenerateFloor(TModulesList &modulesList);	

protected:
	virtual void CalcSize();
	virtual CProceduralModule *GenerateModule(int nX,int nY);
	TModulesList modules;
	int nSizeX,nSizeY;
};
//////////////////////////////////////////////////////////////////////////
#endif