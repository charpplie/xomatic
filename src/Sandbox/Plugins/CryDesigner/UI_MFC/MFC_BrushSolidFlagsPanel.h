#pragma once

#include "PropertiesPanel.h"
#include "Util/Variable.h"
#include "IBaseToolPanel.h"

class CDesignerBrushObject;
class CVarBlock;

class MFC_SolidFlagsPanelUI : public CPropertiesPanel, public IBrushSolidFlagsPanel
{
public:

	MFC_SolidFlagsPanelUI();
	void DestroyPanel() override;
	void AddVariables() override;
	void SetObject( CDesignerBrushObject* pObject ) override;
	void ModifyFlag( int &nFlags,int flag,CSmartVariable<bool> &var,IVariable *pVar ) override;
	void ModifyFlag( int &nFlags,int flag,int clearFlag,CSmartVariable<bool> &var,IVariable *pVar ) override;
	void OnVarChange( IVariable *pVar ) override;
	void SetMultiSelectFlag( bool bEnable ) override { CPropertiesPanel::SetMultiSelect(bEnable); }

private:
	std::unique_ptr<CVarBlock> m_pVarBlock;
	CDesignerBrushObject* m_pObject;

	CSmartVariable<bool> mv_outdoor;
	CSmartVariable<bool> mv_castShadows;
	CSmartVariable<bool> mv_supportSecVisArea;
	CSmartVariable<bool> mv_bakeShadows;
	CSmartVariable<bool> mv_rainOccluder;
	CSmartVariable<bool> mv_hideable;
	CSmartVariable<int> mv_ratioViewDist;
	CSmartVariable<bool> mv_excludeFromTriangulation;
	CSmartVariable<bool> mv_noDynWater;
	CSmartVariable<bool> mv_noStaticDecals;
	CSmartVariable<bool> mv_excludeMaterialPicking;
	CSmartVariable<float> mv_lightmapQuality;
	CSmartVariable<bool> mv_excludeCollision;
	CSmartVariable<bool> mv_Occluder;
};
