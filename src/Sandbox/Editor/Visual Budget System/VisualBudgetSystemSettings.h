////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   VisualBudgetSystemSettings.h
//  Created:     3/08/2009 by Paulo Zaffari.
//  Description: This dialog is used to configure the options for the 
//  visual budget system.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef VisualBudgetSystemSettings_h__
#define VisualBudgetSystemSettings_h__

#pragma once

class CVisualBudgetSystem;

class CVisualBudgetSystemSettings : public CDialog
{
	public:
		enum {IDD = IDD_VISUAL_BUDGET_SYSTEM};
	protected:
	private:

	public:
		CVisualBudgetSystemSettings(CWnd* pParent = NULL);   // standard constructor
		virtual ~CVisualBudgetSystemSettings();
	protected:
		void	DoDataExchange(CDataExchange* pDX);
		BOOL	OnInitDialog();

		void	UpdateEnabledStatus();

		DECLARE_MESSAGE_MAP()

		afx_msg void OnBnClickedEnableVisualBudgetSystem();
		afx_msg void OnBnClickedOk();
		afx_msg void RealTimeUpdate();

		afx_msg void OnBnClickedAccountTerrain();
		afx_msg void OnBnClickedAccountMiscelaneous();
		afx_msg void OnBnClickedAccountBrush();
		afx_msg void OnBnClickedAccountVegetation();
		afx_msg void OnBnClickedAccountRoads();
		afx_msg void OnBnClickedAccountClouds();
		afx_msg void OnBnClickedAccountEntities();
		afx_msg void OnBnClickedAccountDecals();

		virtual void OnOK() {};
		virtual void OnCancel() {};
	private:

	public:
	protected:
		BOOL									m_bEnabledStatus;
		BOOL									m_bAnalyzeSingleSector;
		int										m_displayType;
		int										m_resourceType;
		int										m_cellSizeIndex;

		CButton								m_oEnabledVisualBudgetSystemCheckButton;
		CButton								m_oAnalyzeSingleSectorButton;
		CButton								m_oAccountTerrainCheckButton;
		CButton								m_oAccountMiscelaneousCheckButton;
		CButton								m_oAccountBrushesCheckButton;
		CButton								m_oAccountVegetationCheckButton;
		CButton								m_oAccountRoadsCheckButton;
		CButton								m_oAccountCloudsCheckButton;
		CButton								m_oAccountEntitiesCheckButton;
		CButton								m_oAccountDecalsCheckButton;

		CButton								m_oOkButton;

		BOOL									m_bAccountTerrain;
		BOOL									m_bAccountMiscelaneous;
		BOOL									m_bAccountBrushes;
		BOOL									m_bAccountVegetation;
		BOOL									m_bAccountRoads;
		BOOL									m_bAccountClouds;
		BOOL									m_bAccountEntities;
		BOOL									m_bAccountDecals;

		CVisualBudgetSystem&	m_roVisualBudgetSystem;
	private:
};

#endif // VisualBudgetSystemSettings_h__
