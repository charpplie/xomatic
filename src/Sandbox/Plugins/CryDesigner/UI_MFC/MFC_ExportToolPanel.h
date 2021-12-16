#pragma once

#include "IBaseToolPanel.h"

class CBrushDesignerExportTool;

class MFC_ExportToolPanel : public CXTResizeDialog, public IBaseToolPanel
{	
	DECLARE_DYNAMIC(MFC_ExportToolPanel)
	DECLARE_MESSAGE_MAP()
public:

	MFC_ExportToolPanel(CBrushDesignerExportTool* pExportTool);

	virtual ~MFC_ExportToolPanel(){}

	enum { IDD = IDD_PANEL_DESIGNER_EXPORT };

	void OnOK() {};
	void OnCancel() {};
	
	void DoDataExchange(CDataExchange* pDX);

	void DestroyPanel() override;

	BOOL OnInitDialog() override;	
	void PostNcDestroy(){ delete this; }

	void OnBnClickedDesignerExportCgf();
	void OnBnClickedDesignerExportGrp();
	void OnBnClickedDesignerExportObj();

private:

	CBrushDesignerExportTool* m_pExportTool;

};