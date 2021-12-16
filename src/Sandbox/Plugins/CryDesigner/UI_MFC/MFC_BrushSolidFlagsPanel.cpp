#include "StdAfx.h"
#include "MFC_BrushSolidFlagsPanel.h"
#include "Objects/DesignerBrushObject.h"

namespace
{
	MFC_SolidFlagsPanelUI* g_pSolidFlagPanel;
	int g_nSolidFlagPanelID;
}

IBrushSolidFlagsPanel* CreateSolidFlagsPanel()
{
	if( !g_pSolidFlagPanel )
	{
		g_pSolidFlagPanel = new MFC_SolidFlagsPanelUI();
		g_nSolidFlagPanelID = GetIEditor()->AddRollUpPage( ROLLUP_OBJECTS,_T("Geometry Flags"),g_pSolidFlagPanel,false,-1,false );
	}
	return g_pSolidFlagPanel;
}

void MFC_SolidFlagsPanelUI::DestroyPanel()
{
	if( g_pSolidFlagPanel )
	{
		GetIEditor()->RemoveRollUpPage(ROLLUP_OBJECTS,g_nSolidFlagPanelID);
		g_pSolidFlagPanel = NULL;
		g_nSolidFlagPanelID = 0;
	}
}

MFC_SolidFlagsPanelUI::MFC_SolidFlagsPanelUI() : m_pVarBlock(new CVarBlock)
{
	mv_outdoor = false;
	mv_castShadows = false;
	mv_supportSecVisArea = false;
	mv_bakeShadows = true;
	mv_hideable = false;
	mv_rainOccluder = true;
	mv_ratioViewDist = BUtil::kDefaultViewDist;
	mv_excludeFromTriangulation = false;
	mv_noDynWater = false;
	mv_lightmapQuality = 1;
	mv_lightmapQuality->SetLimits( 0,100 );
	mv_ratioViewDist->SetLimits( 0,255 );
	mv_excludeMaterialPicking = false;
	mv_excludeCollision = false;
	mv_Occluder	= false;

	m_pVarBlock->AddVariable( mv_castShadows,"Cast Shadows" );
	m_pVarBlock->AddVariable( mv_supportSecVisArea,"SupportSecondVisarea" );
	m_pVarBlock->AddVariable( mv_outdoor,"Outdoors Only" );
	m_pVarBlock->AddVariable( mv_rainOccluder,"Rain Occluder" );
	m_pVarBlock->AddVariable( mv_ratioViewDist,"View Distance Ratio" );
	m_pVarBlock->AddVariable( mv_excludeFromTriangulation,"AI Exclude From Triangulation" );
	m_pVarBlock->AddVariable( mv_hideable,"AI Hideable" );
	m_pVarBlock->AddVariable( mv_noDynWater,"No Dynamic Water" );
	m_pVarBlock->AddVariable( mv_noStaticDecals,"No Static Decals" );
	m_pVarBlock->AddVariable( mv_excludeCollision,"Exclude Collision" );
	m_pVarBlock->AddVariable( mv_excludeMaterialPicking,"Exclude Picking Material" );
	m_pVarBlock->AddVariable( mv_Occluder,"Occluder" );
	m_pVarBlock->AddVariable( mv_bakeShadows, "Bake Shadows" );
}

void MFC_SolidFlagsPanelUI::AddVariables()
{
	SetVarBlock( m_pVarBlock.get(),functor(*this,&MFC_SolidFlagsPanelUI::OnVarChange) );
}

void MFC_SolidFlagsPanelUI::SetObject( CDesignerBrushObject* pObject )
{
	m_pObject = pObject;
	if (pObject)
	{
		DeleteVars();

		mv_ratioViewDist = pObject->GetViewDistRatio();

		int flags = pObject->GetRenderFlags();
		int statobjFlags = pObject->GetStatObjFlags();
		mv_outdoor = (flags&ERF_OUTDOORONLY) != 0;
		mv_rainOccluder = (flags&ERF_RAIN_OCCLUDER) != 0;
		mv_castShadows = 	(flags&ERF_CASTSHADOWMAPS) != 0;
		mv_supportSecVisArea = (flags&ERF_REGISTER_BY_BBOX) != 0;
		mv_Occluder = (flags&ERF_GOOD_OCCLUDER) != 0;
		mv_bakeShadows = (flags&ERF_BAKEDSHADOW) != 0;
		mv_hideable = (flags&ERF_HIDABLE) != 0;
		mv_noDynWater = (flags&ERF_NODYNWATER) != 0;
		mv_noStaticDecals = (flags&ERF_NO_DECALNODE_DECALS) != 0;
		mv_excludeFromTriangulation = (flags&ERF_EXCLUDE_FROM_TRIANGULATION) != 0;
		mv_excludeCollision = (statobjFlags&STATIC_OBJECT_NO_PLAYER_COLLIDE) != 0;
		mv_excludeMaterialPicking = pObject->IsExcludedPickingMaterial();

		AddVariables();
	}
}

void MFC_SolidFlagsPanelUI::ModifyFlag( int &nFlags,int flag,CSmartVariable<bool> &var,IVariable *pVar )
{
	ModifyFlag(nFlags,flag,flag,var,pVar);
}

void MFC_SolidFlagsPanelUI::ModifyFlag( int &nFlags,int flag,int clearFlag,CSmartVariable<bool> &var,IVariable *pVar )
{
	if (var.GetVar() == pVar)
		nFlags = (var) ? (nFlags | flag) : (nFlags & (~clearFlag));
}

void MFC_SolidFlagsPanelUI::OnVarChange( IVariable *pVar )
{
	CSelectionGroup *selection = GetIEditor()->GetSelection();
	for (int i = 0; i < selection->GetCount(); ++i)
	{
		CBaseObject *pObj = selection->GetObject(i);
		if(pObj->GetType() == OBJTYPE_SOLID)
		{
			CDesignerBrushObject *pSolid = static_cast<CDesignerBrushObject*>(pObj);
			int nFlags = pSolid->GetRenderFlags();
			int statobjFlags = pSolid->GetStatObjFlags();
			ModifyFlag( nFlags,ERF_OUTDOORONLY,mv_outdoor,pVar );
			ModifyFlag( nFlags,ERF_RAIN_OCCLUDER,mv_rainOccluder,pVar );
			ModifyFlag( nFlags,ERF_CASTSHADOWMAPS|ERF_HAS_CASTSHADOWMAPS,ERF_CASTSHADOWMAPS,mv_castShadows,pVar );
			ModifyFlag( nFlags,ERF_REGISTER_BY_BBOX,mv_supportSecVisArea,pVar );
			ModifyFlag( nFlags,ERF_BAKEDSHADOW,mv_bakeShadows,pVar );
			ModifyFlag( nFlags,ERF_HIDABLE,mv_hideable,pVar );
			ModifyFlag( nFlags,ERF_EXCLUDE_FROM_TRIANGULATION,mv_excludeFromTriangulation,pVar );
			ModifyFlag( nFlags,ERF_NODYNWATER,mv_noDynWater,pVar );
			ModifyFlag( nFlags,ERF_NO_DECALNODE_DECALS,mv_noStaticDecals,pVar );
			ModifyFlag( nFlags,ERF_GOOD_OCCLUDER,mv_Occluder,pVar );
			ModifyFlag( statobjFlags,STATIC_OBJECT_NO_PLAYER_COLLIDE,mv_excludeCollision,pVar );

			if( pVar == mv_excludeMaterialPicking.GetVar() )
			{
				bool bExcludePickingMaterial = false;
				mv_excludeMaterialPicking->Get(bExcludePickingMaterial);
				pSolid->ExcludePickingMaterial(bExcludePickingMaterial);
			}

			if (mv_ratioViewDist.GetVar() == pVar)
				pSolid->SetViewDistRatio( mv_ratioViewDist );

			pSolid->SetRenderFlags(nFlags);
			pSolid->SetStatObjFlags(statobjFlags);
			pSolid->UpdateEngineNode();
			pSolid->UpdateGroup();
		}
	}
}