/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: TextureBrowserPanel.h,v 1.0 2008/08/13 11:52:18 PauloZaffari Exp wwwrun $
$DateTime$
Description:  MFC class implementing the texture browser panel.
-------------------------------------------------------------------------
History:
- 13:08:2008   11:52 : Created by Paulo Zaffari

*************************************************************************/

#ifndef TextureBrowserPanel_h__
#define TextureBrowserPanel_h__

#include "..\Controls/SliderCtrlEx.h"

class CTextureViewer;

class CTextureBrowserPanel: public CXTResizeDialog
{
	private:
		// Created this helper class to dodge a problem...
		// The problem here is that even with SetOwner being used to make
		// WM_COMMAND messages go to CTextureBrowserPanel, it those messages
		// were actually going to CXTPTaskPanel.
		// So, class CXTPTaskPanelSpecific has overriden method OnCommand
		// calling the proper methods from CTextureBrowserPanel as if
		// the WM_COMMAND method was being forwarded properly.
		// The problem also happens to many of the HSCROLL messages...this is
		// becoming quite annoying.
		class CXTPTaskPanelSpecific: public CXTPTaskPanel
		{
			public:
				CXTPTaskPanelSpecific():CXTPTaskPanel(){};
				~CXTPTaskPanelSpecific(){};

				BOOL OnCommand(WPARAM wParam,LPARAM lParam);

				CTextureBrowserPanel* GetOwner() const { return m_poMyOwner; }
				void SetOwner(CTextureBrowserPanel* val) { m_poMyOwner = val; }

				// As defining a message map for this class would break it...
				// since this is not a fully black-box implementation...
				// We had to use overridables in order to handle some "broken"
				// messages who simply aren't forwarded to the owners properly
				// (such as most scrollbar/trackbar(slider) messages.
				LRESULT WindowProc(UINT message,WPARAM wParam,LPARAM lParam);
			protected:
				CTextureBrowserPanel* m_poMyOwner;
		};

	public:
		// In case you wish to use this class somewhere else then with the CTextureViewer,
		// you should extract an interface for both methods, derive CTextureViewer from this
		// interface and as well as your new class.
		// While this is not necessary, I am just keeping things simple (no interfaces).
		CTextureBrowserPanel(CTextureViewer* poSettingsOwner, CWnd* pParent = NULL);
		~CTextureBrowserPanel();

		void SetFilenameDisplay(const char* szFilename);
		void SetSurfaceTypeDisplay(const char* szSurfaceType);
		void SetNumberOfMipsDisplay(const char* szNumberOfMips);
		void SetFileSizeDisplay(const char*  szFileSizeDisplay);
		void SetTextureResolutionDisplay(const char*  szResolution);

		void ClearFilters();

	// Dialog Data
	//{{AFX_DATA(CTerrainModifyPanel)
	enum { IDD = IDD_PANEL_TEXTURE_BROWSER};

	enum EControlIds
	{
		eTaskPanel=2048,

		eFilenameEdit,
		eTextureSizeHorizontalMinimum,
		eTextureSizeHorizontalMaximum,
		eTextureSizeVerticalMinimum,
		eTextureSizeVerticalMaximum,

		eTextureSemanticsDiffuse,
		eTextureSemanticsSpecular,
		eTextureSemanticsBump,
		eTextureSemanticsCubemap,

		eTextureFileDDS,
		eTextureFileTIF,
		eTextureFileUsedInLevel,
		eTextureHasAlphaChannel,
		eTextureAlphaChannel,

		eOpenFolderLink,
		ePreviewLink,

		eTextureCellSizeSlider,

		eAllControls,
	};

	enum EArrayOrders
	{
		EMinimumWidth=0,
		EMaximumWidth=1,

		EMinimumHeight=2,
		EMaximumHeight=3,

		EShowOnlyDDSFiles=0,
		EShowOnlyTIFFiles=1,
		EShowOnlyUsedInLevelFiles=2,
		EShowOnlyIfHasAlphaChannel=3,
		EShowOnlyAlphaChannel=4,

		EShowOnlyDiffuseSemantics=0,
		EShowOnlySpecularSemantics=1,
		EShowOnlyBumpSemantics=2,
		EShowOnlyCubemapSemantics=3,

		EGroupItemFilename=0,
		EGroupItemSurfaceType=1,
		EGroupItemNumberOfMips=2,
		EGroupItemFileSize=3,
		EGroupItemTextureResolution=4,
		ETotalGroupItems=5,
	};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	
	// Generated message map functions
	//{{AFX_MSG(CTerrainModifyPanel)
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();

	afx_msg void OnHScroll( UINT nSBCode,UINT nPos,CScrollBar* pScrollBar );

	afx_msg void OnFilenameChangeEdit();

	afx_msg void OnBnClickedTextureBrowserOpenFolderButton();

	afx_msg void OnCbnSelchangeTextureBrowserTextureSizeXMinimumCombo();
	afx_msg void OnCbnEditchangeTextureBrowserTextureSizeXMinimumCombo();

	afx_msg void OnCbnSelchangeTextureBrowserTextureSizeYMinimumCombo();
	afx_msg void OnCbnEditchangeTextureBrowserTextureSizeYMinimumCombo();

	afx_msg void OnCbnSelchangeTextureBrowserTextureSizeXMaximumCombo();
	afx_msg void OnCbnEditchangeTextureBrowserTextureSizeXMaximumCombo();

	afx_msg void OnCbnSelchangeTextureBrowserTextureSizeYMaximumCombo();
	afx_msg void OnCbnEditchangeTextureBrowserTextureSizeYMaximumCombo();

	afx_msg void OnBnClickedTextureBrowserSemanticsDiffuseCheck();
	afx_msg void OnBnClickedTextureBrowserSemanticsSpecularCheck();
	afx_msg void OnBnClickedTextureBrowserSemanticsBumpCheck();
	afx_msg void OnBnClickedTextureBrowserSemanticsCubemapCheck();

	afx_msg void OnBnClickedTextureBrowserFiletypeDDSCheck();
	afx_msg void OnBnClickedTextureBrowserFiletypeTIFCheck();
	afx_msg void OnBnClickedTextureBrowserUsedInLevelCheck();
	afx_msg void OnBnClickedTextureBrowserHasAlphaChanelCheck();
	afx_msg void OnBnClickedTextureBrowserShowAlphaChanelCheck();

	afx_msg void OnBnClickedTextureBrowserPreviewButton();

	afx_msg LRESULT OnTaskPanelNotify( WPARAM wParam, LPARAM lParam );

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CXTPTaskPanelSpecific		m_wndTaskPanel;
	CEdit										m_oFilenameEdit;
	CComboBox								m_aoTextureSizeComboBoxes[4];
	CButton									m_aoTextureSemanticsCheckBoxes[4];
	CButton									m_aoTextureFileCheckBoxes[5];
	CSliderCtrlEx						m_oTextureCellSizeSlider;

	CTextureViewer*					m_poSettingsOwner;

	CXTPTaskPanelGroupItem	*m_apoDisplayItems[ETotalGroupItems];
};


#endif // TextureBrowserPanel_h__