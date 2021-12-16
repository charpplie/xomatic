/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2013.
*************************************************************************/

#ifndef __LODGENERATORSHARED_H__
#define __LODGENERATORSHARED_H__

//////////////////////////////////////////////////////////////////////////
//  file panel
//////////////////////////////////////////////////////////////////////////

class CLodGeneratorFilePanel : public CDialog
{
public:
	CLodGeneratorFilePanel(CWnd* pParent = NULL);
	virtual ~CLodGeneratorFilePanel();

	enum { IDD = IDD_PANEL_GEOM_LOD_GEN_FILES };

	const static char * kPanelCaption;

protected:
	DECLARE_MESSAGE_MAP()

	BOOL OnInitDialog();
	BOOL PreTranslateMessage(MSG* pMsg);

public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBnClickedOpen();
	afx_msg void OnBnClickedSelected();	
	afx_msg void OnBnClickedMatEd();

protected:
	void SelectObject(const CString& objectPath, const CString& materialPath);

public:
	void OnOpenWithPathParameter(const CString& filepath);
	const void RefreshMaterialFile();
	const CString LoadedFile();
	const CString MaterialFile();

private:
	CToolTipCtrl* m_pToolTip;
};

#endif