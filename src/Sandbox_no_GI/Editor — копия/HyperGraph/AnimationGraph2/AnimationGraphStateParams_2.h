#ifndef __ANIMATIONGRAPHSTATEPARAMS2_H__
#define __ANIMATIONGRAPHSTATEPARAMS2_H__

#pragma once



//////////////////////////////////////////////////////////////////////////
// CAnimationGraphStateParamCtrl2
//////////////////////////////////////////////////////////////////////////

class CAnimationGraphStateParamCtrl2 : public CDialog, public _reference_target_t
{
	DECLARE_DYNAMIC( CAnimationGraphStateParamCtrl2 );
	enum { IDD = IDD_PANEL_AG_PARAM };

public:
	CAnimationGraphStateParamCtrl2( const CString& name ): m_name(name) {}

	void AdjustHeight();
	void SetParamNameText(const CString& text);
	const CString& GetName() { return m_name; }
	const CString& GetSelectedValue() { return m_SelectedValue; }
	CListBox* GetValueList(){ return &m_ValuesList; }

	afx_msg void OnListBoxSelChange();

protected:
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange( CDataExchange* pDX );
	afx_msg BOOL OnEraseBkgnd( CDC* pDC );
	afx_msg void OnListBoxDblClk();
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint pos );
	afx_msg void OnParamNameKillFocus();

	virtual void OnOK() { OnParamNameKillFocus(); }
	virtual void OnCancel() { m_ParamNameEdit.SetWindowText( m_name ); }

private:
	CString m_name;
	CString m_SelectedValue;
	CEdit m_ParamNameEdit;
	CListBox m_ValuesList;
	int m_ItemHeight;
};
typedef _smart_ptr< CAnimationGraphStateParamCtrl2 > CAnimationGraphStateParamCtrl2Ptr;




//////////////////////////////////////////////////////////////////////////
// CAnimationGraphStateParamsDlg2
//////////////////////////////////////////////////////////////////////////

class CAnimationGraphStateParamsDlg2 : public CDialog
{
	DECLARE_DYNCREATE( CAnimationGraphStateParamsDlg2 );
public:
	CAnimationGraphStateParamsDlg2() {}
	~CAnimationGraphStateParamsDlg2() {}

	enum { IDD = IDD_PANEL_AG_STATEPARAMS };

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg void OnAddParam();
	afx_msg void OnCheckBoxChanged();

protected:
	virtual void OnOK() {}
	virtual void OnCancel() {}
};




//////////////////////////////////////////////////////////////////////////
// CAnimationGraphStateParamsPanel2
//////////////////////////////////////////////////////////////////////////

class CAnimationGraphStateParamsPanel2 : public CDialog
{
	DECLARE_DYNCREATE( CAnimationGraphStateParamsPanel2 );

	friend class CAnimationGraphStateParamCtrl2;

public:
	CAnimationGraphStateParamsPanel2() : m_bIgnoreVScroll(false), m_pParamsDeclaration(NULL) {}
	~CAnimationGraphStateParamsPanel2() {}

	enum { IDD = IDD_PANEL_PROPERTIES };

	void Init();

	bool OnAddParam( const char* name );
	bool AddParamValue( const char* param, const char* value );
	bool DeleteParamValue( const char* param, const char* value );
	bool RenameParamValue( const char* param, const char* oldValue, const char* newValue );

	void OnSelChanged();

	void SetParamsDeclaration( CParamsDeclaration2* pParamsDcl );

protected:
	DECLARE_MESSAGE_MAP()

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

	virtual void OnOK() {}
	virtual void OnCancel() {}

private:
	int	m_MiniPanelWidth;
	int	m_StateParamsDlgHeight;
	bool m_bIgnoreVScroll;

	CParamsDeclaration2* m_pParamsDeclaration;

	void ArrangeMiniPanels();

	bool DeleteParam( CAnimationGraphStateParamCtrl2* pParamCtrl );
	bool RenameParam( CAnimationGraphStateParamCtrl2* pParamCtrl, const CString& newName );

public:
	CAnimationGraphStateParamsDlg2 m_StateParamsDlg;

	typedef std::list< CAnimationGraphStateParamCtrl2Ptr > TListParamCtrls;
	TListParamCtrls m_ParamCtrls;

	TParameterizationId2 m_CurrentSelection;
};


#endif // __ANIMATIONGRAPHSTATEPARAMS2_H__
