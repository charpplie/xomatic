///////////////////////////////////////////////
// Procedural Buildings Base Rule
#ifndef __proceduralproductionrulevariationone_h__
#define __proceduralproductionrulevariationone_h__
#pragma once
#include "ProceduralSystem/ProceduralProductionRuleBase.h"

typedef std::map<CString,lstLink> TLinksMap;

class CProceduralProductionRuleVariationOne : public CProceduralProductionRuleBase
{
public:
	void GenerateFloor(TModulesList &modulesList);
	void AddModuleLinks(TModulesList &modulesList);
protected:
	CProceduralModule *GenerateModule(int nX,int nY);
	void CalcSize();
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
