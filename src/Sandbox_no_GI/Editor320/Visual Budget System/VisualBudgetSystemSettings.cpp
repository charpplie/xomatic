////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   VisualBudgetSystemSettings.cpp
//  Created:     3/08/2009 by Paulo Zaffari.
//  Description: This dialog is used to configure the options for the 
//  visual budget system.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include <stdafx.h>
#include "VisualBudgetSystemSettings.h"
#include "VisualBudgetSystem.h"

#include "ViewManager.h"


BEGIN_MESSAGE_MAP(CVisualBudgetSystemSettings, CDialog)
	ON_BN_CLICKED(IDC_VISUAL_BUDGET_SYSTEM_DISPLAY_VISUAL_BUDGET_SYSTEM_CHECKBOX, &CVisualBudgetSystemSettings::OnBnClickedEnableVisualBudgetSystem)
	ON_BN_CLICKED(IDC_VISUAL_BUDGET_SYSTEM_OK_BUTTON, &CVisualBudgetSystemSettings::OnBnClickedOk)

	ON_BN_CLICKED(IDC_LINES, &CVisualBudgetSystemSettings::RealTimeUpdate)
	ON_BN_CLICKED(IDC_BOXES, &CVisualBudgetSystemSettings::RealTimeUpdate)

	ON_BN_CLICKED(IDC_GEOMETRY, &CVisualBudgetSystemSettings::RealTimeUpdate)
	ON_BN_CLICKED(IDC_TEXTURE, &CVisualBudgetSystemSettings::RealTimeUpdate)
	ON_BN_CLICKED(IDC_BOTH, &CVisualBudgetSystemSettings::RealTimeUpdate)

	ON_BN_CLICKED(IDC_CELLSIZE_128, &CVisualBudgetSystemSettings::OnBnClickedOk)
	ON_BN_CLICKED(IDC_CELLSIZE_64, &CVisualBudgetSystemSettings::OnBnClickedOk)
	ON_BN_CLICKED(IDC_CELLSIZE_32, &CVisualBudgetSystemSettings::OnBnClickedOk)

	//ON_BN_CLICKED(IDC_VISUAL_BUDGET_SYSTEM_CANCEL_BUTTON, &CVisualBudgetSystemSettings::OnBnClickedCancel)

	ON_BN_CLICKED(IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_TERRAIN_CHECKBOX, &CVisualBudgetSystemSettings::OnBnClickedAccountTerrain)
	ON_BN_CLICKED(IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_MISCELANEOUS_CHECKBOX, &CVisualBudgetSystemSettings::OnBnClickedAccountMiscelaneous)

	ON_BN_CLICKED(IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_BRUSHES_CHECKBOX, &CVisualBudgetSystemSettings::OnBnClickedAccountBrush)
	ON_BN_CLICKED(IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_VEGETATION_CHECKBOX, &CVisualBudgetSystemSettings::OnBnClickedAccountVegetation)

	ON_BN_CLICKED(IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_ROADS_CHECKBOX, &CVisualBudgetSystemSettings::OnBnClickedAccountRoads)
	ON_BN_CLICKED(IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_CLOUDS_CHECKBOX, &CVisualBudgetSystemSettings::OnBnClickedAccountClouds)

	ON_BN_CLICKED(IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_ENTITIES_CHECKBOX, &CVisualBudgetSystemSettings::OnBnClickedAccountEntities)
	ON_BN_CLICKED(IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_DECALS_CHECKBOX, &CVisualBudgetSystemSettings::OnBnClickedAccountDecals)

END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////
CVisualBudgetSystemSettings::CVisualBudgetSystemSettings(CWnd* pParent):
CDialog(IDD),
m_roVisualBudgetSystem(CVisualBudgetSystem::GetVisualBudgetSystem())
{
	m_displayType = 0;
	m_resourceType = 0;
	m_cellSizeIndex = 0;
}
//////////////////////////////////////////////////////////////////////////
CVisualBudgetSystemSettings::~CVisualBudgetSystemSettings()
{
	// Disable the visual budget system and the detailed analysis mode if enabled.
	m_roVisualBudgetSystem.SetDetailedAnalysisStatus(false);
	CViewport	*poViewport=GetIEditor()->GetViewManager()->GetViewport("Perspective");
	if (poViewport)
	{
		if (poViewport->IsRenderListenerRegistered(&m_roVisualBudgetSystem))
		{
			poViewport->UnregisterRenderListener(&m_roVisualBudgetSystem);
		}
	}
}
//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystemSettings::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX,IDC_VISUAL_BUDGET_SYSTEM_DISPLAY_VISUAL_BUDGET_SYSTEM_CHECKBOX,m_oEnabledVisualBudgetSystemCheckButton);
	DDX_Control(pDX,IDC_VISUAL_BUDGET_SYSTEM_DISPLAY_VISUAL_BUDGET_SYSTEM_ANALYZE_SINGLE_SECTOR_CHECKBOX,m_oAnalyzeSingleSectorButton);

	DDX_Control(pDX,IDC_VISUAL_BUDGET_SYSTEM_OK_BUTTON,m_oOkButton);
	DDX_Radio(pDX,IDC_LINES, m_displayType);
	DDX_Radio(pDX,IDC_GEOMETRY, m_resourceType);
	DDX_Radio(pDX,IDC_CELLSIZE_128, m_cellSizeIndex);

	DDX_Control(pDX,IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_TERRAIN_CHECKBOX,m_oAccountTerrainCheckButton);
	DDX_Control(pDX,IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_MISCELANEOUS_CHECKBOX,m_oAccountMiscelaneousCheckButton);
	DDX_Control(pDX,IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_BRUSHES_CHECKBOX,m_oAccountBrushesCheckButton);
	DDX_Control(pDX,IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_VEGETATION_CHECKBOX,m_oAccountVegetationCheckButton);
	DDX_Control(pDX,IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_ROADS_CHECKBOX,m_oAccountRoadsCheckButton);
	DDX_Control(pDX,IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_CLOUDS_CHECKBOX,m_oAccountCloudsCheckButton);
	DDX_Control(pDX,IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_ENTITIES_CHECKBOX,m_oAccountEntitiesCheckButton);
	DDX_Control(pDX,IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_DECALS_CHECKBOX,m_oAccountDecalsCheckButton);

	DDX_Check(pDX,IDC_VISUAL_BUDGET_SYSTEM_DISPLAY_VISUAL_BUDGET_SYSTEM_CHECKBOX,m_bEnabledStatus);
	DDX_Check(pDX,IDC_VISUAL_BUDGET_SYSTEM_DISPLAY_VISUAL_BUDGET_SYSTEM_ANALYZE_SINGLE_SECTOR_CHECKBOX,m_bAnalyzeSingleSector);

	DDX_Check(pDX,IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_TERRAIN_CHECKBOX,m_bAccountTerrain);
	DDX_Check(pDX,IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_MISCELANEOUS_CHECKBOX,m_bAccountMiscelaneous);
	DDX_Check(pDX,IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_BRUSHES_CHECKBOX,m_bAccountBrushes);
	DDX_Check(pDX,IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_VEGETATION_CHECKBOX,m_bAccountVegetation);
	DDX_Check(pDX,IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_ROADS_CHECKBOX,m_bAccountRoads);
	DDX_Check(pDX,IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_CLOUDS_CHECKBOX,m_bAccountClouds);
	DDX_Check(pDX,IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_ENTITIES_CHECKBOX,m_bAccountEntities);
	DDX_Check(pDX,IDC_VISUAL_BUDGET_SYSTEM_ACCOUNTED_ELEMENTS_DECALS_CHECKBOX,m_bAccountDecals);
}
//////////////////////////////////////////////////////////////////////////
BOOL CVisualBudgetSystemSettings::OnInitDialog()
{
	BOOL bRes = __super::OnInitDialog();

	m_displayType = m_roVisualBudgetSystem.GetDisplayType();
	m_resourceType = m_roVisualBudgetSystem.GetMeasuredResourceType();
	if (m_roVisualBudgetSystem.GetCellSize() == 64.0f)
		m_cellSizeIndex = 1;
	else if (m_roVisualBudgetSystem.GetCellSize() == 32.0f)
		m_cellSizeIndex = 2;
	else
		m_cellSizeIndex = 0;

	m_bAnalyzeSingleSector = m_roVisualBudgetSystem.GetDetailedAnalysisStatus()?TRUE:FALSE;

	m_bAccountTerrain = m_roVisualBudgetSystem.GetAccountTerrainStatus()?TRUE:FALSE;
	m_bAccountMiscelaneous = m_roVisualBudgetSystem.GetAccountMiscelaneousStatus()?TRUE:FALSE;
	m_bAccountVegetation = m_roVisualBudgetSystem.GetAccountVegetationStatus()?TRUE:FALSE;
	m_bAccountBrushes = m_roVisualBudgetSystem.GetAccountBrushStatus()?TRUE:FALSE;

	m_bAccountRoads = m_roVisualBudgetSystem.GetAccountRoadsStatus()?TRUE:FALSE;
	m_bAccountClouds = m_roVisualBudgetSystem.GetAccountCloudsStatus()?TRUE:FALSE;

	m_bAccountEntities = m_roVisualBudgetSystem.GetAccountEntitiesStatus()?TRUE:FALSE;
	m_bAccountDecals = m_roVisualBudgetSystem.GetAccountDecalsStatus()?TRUE:FALSE;

	UpdateEnabledStatus();
	UpdateData(FALSE);

	return TRUE;
}
//////////////////////////////////////////////////////////////////////////
void	CVisualBudgetSystemSettings::UpdateEnabledStatus()
{
	CViewport	*poViewport=GetIEditor()->GetViewManager()->GetViewport("Perspective");
	BOOL enable;
	if (poViewport)
	{
		if (poViewport->IsRenderListenerRegistered(&m_roVisualBudgetSystem))
		{
			enable = TRUE;
		}
		else
		{
			enable = FALSE;
		}

		m_bEnabledStatus = enable;
		m_oAnalyzeSingleSectorButton.EnableWindow(enable);
		GetDlgItem(IDC_LINES)->EnableWindow(enable);
		GetDlgItem(IDC_BOXES)->EnableWindow(enable);
		GetDlgItem(IDC_GEOMETRY)->EnableWindow(enable);
		GetDlgItem(IDC_TEXTURE)->EnableWindow(enable);
		GetDlgItem(IDC_BOTH)->EnableWindow(enable);
		GetDlgItem(IDC_CELLSIZE_128)->EnableWindow(enable);
		GetDlgItem(IDC_CELLSIZE_64)->EnableWindow(enable);
		GetDlgItem(IDC_CELLSIZE_32)->EnableWindow(enable);

		// Accounting options.
		m_oAccountTerrainCheckButton.EnableWindow(enable);
		m_oAccountMiscelaneousCheckButton.EnableWindow(enable);
		m_oAccountBrushesCheckButton.EnableWindow(enable);
		m_oAccountVegetationCheckButton.EnableWindow(enable);
		m_oAccountRoadsCheckButton.EnableWindow(enable);
		m_oAccountCloudsCheckButton.EnableWindow(enable);
		m_oAccountEntitiesCheckButton.EnableWindow(enable);
		m_oAccountDecalsCheckButton.EnableWindow(enable);
	}
}
//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystemSettings::OnBnClickedEnableVisualBudgetSystem()
{
	if (m_oEnabledVisualBudgetSystemCheckButton.GetCheck()==BST_CHECKED)
	{
		CViewport	*poViewport=GetIEditor()->GetViewManager()->GetViewport("Perspective");
		if (poViewport)
		{
			if (!poViewport->IsRenderListenerRegistered(&m_roVisualBudgetSystem))
			{
				poViewport->RegisterRenderListener(&m_roVisualBudgetSystem);
			}
		}
	}
	else if (m_oEnabledVisualBudgetSystemCheckButton.GetCheck()==BST_UNCHECKED)
	{
		CViewport	*poViewport=GetIEditor()->GetViewManager()->GetViewport("Perspective");
		if (poViewport)
		{
			if (poViewport->IsRenderListenerRegistered(&m_roVisualBudgetSystem))
			{
				poViewport->UnregisterRenderListener(&m_roVisualBudgetSystem);
			}
		}
	}
	else
	{
		assert(!"[EDITOR][CVisualBudgetSystemSettings::OnBnClickedEnableVisualBudgetSystem()]Unexpected checkbox value.");
		return;
	}	
	UpdateEnabledStatus();
}

//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystemSettings::OnBnClickedOk()
{
	UpdateData(TRUE);

	m_roVisualBudgetSystem.SetDetailedAnalysisStatus(m_bAnalyzeSingleSector!=FALSE);

	m_roVisualBudgetSystem.SetDisplayType(m_displayType);
	m_roVisualBudgetSystem.SetMeasuredResourceType(CVisualBudgetSystem::ResourceType(m_resourceType));
	if (m_cellSizeIndex == 1)
		m_roVisualBudgetSystem.SetCellSize(64.0f);
	else if (m_cellSizeIndex == 2)
		m_roVisualBudgetSystem.SetCellSize(32.0f);
	else
		m_roVisualBudgetSystem.SetCellSize(128.0f);
	m_roVisualBudgetSystem.SetAccountTerrainStatus(m_bAccountTerrain!=FALSE);
	m_roVisualBudgetSystem.SetAccountMiscelaneousStatus(m_bAccountMiscelaneous!=FALSE);
	m_roVisualBudgetSystem.SetAccountBrushStatus(m_bAccountBrushes!=FALSE);
	m_roVisualBudgetSystem.SetAccountVegetationStatus(m_bAccountVegetation!=FALSE);
	m_roVisualBudgetSystem.SetAccountRoadsStatus(m_bAccountRoads!=FALSE);
	m_roVisualBudgetSystem.SetAccountCloudsStatus(m_bAccountClouds!=FALSE);
	m_roVisualBudgetSystem.SetAccountEntitiesStatus(m_bAccountEntities!=FALSE);
	m_roVisualBudgetSystem.SetAccountDecalsStatus(m_bAccountDecals!=FALSE);

	m_roVisualBudgetSystem.Update();
}


//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystemSettings::RealTimeUpdate()
{
	UpdateData(TRUE);
	m_roVisualBudgetSystem.SetDisplayType(m_displayType);
	m_roVisualBudgetSystem.SetMeasuredResourceType(CVisualBudgetSystem::ResourceType(m_resourceType));
}

//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystemSettings::OnBnClickedAccountTerrain()
{
	// Not really relevant.
}
//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystemSettings::OnBnClickedAccountMiscelaneous()
{
	// Not really relevant.
}
//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystemSettings::OnBnClickedAccountBrush()
{
	// Not really relevant.
}
//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystemSettings::OnBnClickedAccountVegetation()
{
	// Not really relevant.
}
//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystemSettings::OnBnClickedAccountRoads()
{
	// Not really relevant.
}
//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystemSettings::OnBnClickedAccountClouds()
{
	// Not really relevant.
}
//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystemSettings::OnBnClickedAccountEntities()
{
	// Not really relevant.
}
//////////////////////////////////////////////////////////////////////////
void CVisualBudgetSystemSettings::OnBnClickedAccountDecals()
{
	// Not really relevant.
}
//////////////////////////////////////////////////////////////////////////
