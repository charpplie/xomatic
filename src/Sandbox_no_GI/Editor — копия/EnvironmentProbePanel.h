#ifndef __EnvironmentProbePanel_h__
#define __EnvironmentProbePanel_h__

#if _MSC_VER > 1000
#pragma once
#endif

class CEnvironementProbeObject;

class CEnvironmentProbePanel : public CDialog
{
	DECLARE_DYNAMIC(CEnvironmentProbePanel)

	public:
		
		enum { IDD = IDD_ENVIRONMENT_PANEL_PROBE };
		void SetEntity(CEnvironementProbeObject *pEntity);
		virtual BOOL OnInitDialog();
		virtual void DoDataExchange(CDataExchange *pDX);
		afx_msg void OnGenerateCubemap();
		afx_msg void OnGenerateAllCubemaps();

		DECLARE_MESSAGE_MAP()

	private: 
		CEnvironementProbeObject * m_pEntity;
		CCustomButton m_GenerateCubemapsButton;
		CCustomButton m_GenerateAllCubemapsButton;
};

#endif // __EnvironmentProbePanel_h__
