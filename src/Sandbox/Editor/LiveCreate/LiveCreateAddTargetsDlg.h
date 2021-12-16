#pragma once
#include "EditorLiveCreateManager.h"

#ifndef NO_LIVECREATE

namespace LiveCreate
{
	class CBGTask_SearchForHostsForPlatform;
	class CBGTask_ResolveAddress;
	class CBGTask_GetHostInfoPacket;
}

class CLiveCreateTargetRecord : public CXTPReportRecord
{
	DECLARE_SERIAL(CLiveCreateTargetRecord)

public:
	LiveCreate::IPlatformHandlerFactory* m_pFactory;
	LiveCreate::CBGTask_ResolveAddress* m_pAddresTask;
	LiveCreate::CBGTask_GetHostInfoPacket* m_pGameInfoTask;
	CString m_targetName;
	CString m_validAddress;
	CString m_buildDirectory;
	CString m_buildExecutable;
	CString m_gameName;

public:
	CLiveCreateTargetRecord();
	CLiveCreateTargetRecord(const LiveCreate::IPlatformHandlerFactory::TargetInfo& target);
	virtual ~CLiveCreateTargetRecord();

	bool UpdateTasks();

protected:
	CLiveCreateTargetRecord(CLiveCreateTargetRecord* pRecord);
	CLiveCreateTargetRecord& operator= (const CLiveCreateTargetRecord& rOther);

	// CXTPReportRecord interface
	virtual void CreateItems();
	virtual void GetItemMetrics( XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics);
};

class CLiveCreateAddTargetsDlg : public CDialog
{
	DECLARE_DYNAMIC(CLiveCreateAddTargetsDlg)

public:
	struct TargetInformation
	{
		LiveCreate::IPlatformHandlerFactory* m_pFactory;
		CString m_platformName;
		CString m_targetName;
		CString m_validAddress;
		CString m_buildDirectory;
		CString m_buildExecutable;
	};

public:
	CLiveCreateAddTargetsDlg(CWnd* pParent = NULL);
	virtual ~CLiveCreateAddTargetsDlg();

	// Dialog Data
	enum { IDD = IDD_LIVECREATE_ADD_TARGETS };

	typedef std::vector<TargetInformation> TOutputTargetList;
	inline const TOutputTargetList& GetTargets() const { return m_outputTargets; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnInitDialog();
	virtual void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedRefresh();
	afx_msg void OnBnClickedAddCustom();
	afx_msg void OnBnClickedAddByIP();

private:
	CImageList m_imglstTargets;
	CryMutex m_lockKnownTargets;
	CXTPReportControl m_lstTargets;
	CButton m_btnOK;
	CButton m_btnRefresh;

	TOutputTargetList m_outputTargets;

	typedef std::vector<LiveCreate::CBGTask_SearchForHostsForPlatform*> TTargetSearchTasks;
	TTargetSearchTasks m_pTargetSearchTask;
};

#endif