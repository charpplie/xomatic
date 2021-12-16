
//#include "ProceduralSystem/ProceduralProductionRuleBase.h"
#ifndef __proceduralproductionrulebasesimple_h__
#define __proceduralproductionrulebasesimple_h__
#pragma once

#include "..\ProceduralSystem/ProceduralProductionRuleBase.h"


class CProceduralProductionRuleBaseSimple: public CProceduralProductionRuleBase
{
public:
	void GenerateFloor(TModulesList &modulesList);	
protected:
	CProceduralModule *GenerateModule(int nX,int nY);
	void CalcSize();	
	bool IsModuleInModulesList(TModulesList &searchList, CProceduralModule &module);
};
//////////////////////////////////////////////////////////////////////////
#endif