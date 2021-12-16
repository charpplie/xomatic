#if !defined(__PARTICLE_EFFECT_PANEL_H___)
#define __PARTICLE_EFFECT_PANEL_H___

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EntityLinksPanel.h : header file
//

class CParticleEffectObject;

/////////////////////////////////////////////////////////////////////////////
// CParticleEffectPanel dialog

class CParticleEffectPanel : public CXTResizeDialog
{
	// Construction
public:
	CParticleEffectPanel(CWnd* pParent = NULL);   // standard constructor

	// Dialog Data
	enum { IDD = IDD_PANEL_PARTICLE_EFFECT };

	void SetParticleEffectEntity( class CParticleEffectObject *entity );

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CParticleEffectPanel)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK() {};
	virtual void OnCancel() {};	

	DECLARE_MESSAGE_MAP()

	CParticleEffectObject* m_pEntity;

public:
	afx_msg void OnBnClickedGotodatabase();
};

#endif // !__PARTICLE_EFFECT_PANEL_H___
