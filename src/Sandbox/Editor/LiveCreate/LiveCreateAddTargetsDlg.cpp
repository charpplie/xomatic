#include "StdAfx.h"
#include "ILiveCreateCommon.h"
#include "ILiveCreatePlatform.h"
#include "LiveCreateAddTargetsDlg.h"
#include "LiveCreateAddByIpDlg.h"
#include "LiveCreateEditConnectionDlg.h"
#include "LiveCreate/EditorLiveCreate.h"
#include "EditorLiveCreateTasks.h"

#ifndef NO_LIVECREATE

//-----------------------------------------------------------------------------

IMPLEMENT_SERIAL(CLiveCreateTargetRecord, CXTPReportRecord, VERSIONABLE_SCHEMA | _XTP_SCHEMA_CURRENT)

CLiveCreateTargetRecord::CLiveCreateTargetRecord()
	: m_pFactory(NULL)
	, m_pAddresTask(NULL)
	, m_pGameInfoTask(NULL)
{
}

CLiveCreateTargetRecord::CLiveCreateTargetRecord(const LiveCreate::IPlatformHandlerFactory::TargetInfo& target)
	: m_pFactory(target.pFactory)
	, m_pAddresTask(NULL)
	, m_pGameInfoTask(NULL)
	, m_targetName(target.targetName)
{
	// start the address resolve task right away
	m_pAddresTask = new LiveCreate::CBGTask_ResolveAddress(m_pFactory, m_targetName);
	m_pAddresTask->AddRef();
	GetIEditor()->GetBackgroundTaskManager()->AddTask(m_pAddresTask, eTaskPriority_BackgroundScan, eTaskThreadMask_Any);

	// name & address 
	AddItem(new CXTPReportRecordItemText(m_pFactory->GetPlatformName()));
	AddItem(new CXTPReportRecordItemText(m_targetName));
	AddItem(new CXTPReportRecordItemText("Resolving..."));
	AddItem(new CXTPReportRecordItemText("No info"));

	// initial icons - gray
	GetItem(2)->SetIconIndex(0); // no address
	GetItem(3)->SetIconIndex(0); // no game status
}

CLiveCreateTargetRecord::CLiveCreateTargetRecord(CLiveCreateTargetRecord* pRecord)
{
	CryFatalError("Not implemented - should not be called");
}

CLiveCreateTargetRecord& CLiveCreateTargetRecord::operator= (const CLiveCreateTargetRecord& rOther)
{
	CryFatalError("Not implemented - should not be called");
	return *this;
}

CLiveCreateTargetRecord::~CLiveCreateTargetRecord()
{
	if (NULL != m_pGameInfoTask)
	{
		m_pGameInfoTask->Cancel();
		m_pGameInfoTask->Release();
		m_pGameInfoTask = NULL;
	}

	if (NULL != m_pAddresTask)
	{
		m_pAddresTask->Cancel();
		m_pAddresTask->Release();
		m_pAddresTask = NULL;
	}
}

bool CLiveCreateTargetRecord::UpdateTasks()
{
	if (NULL != m_pAddresTask)
	{
		if (m_pAddresTask->HasFinished())
		{
			CXTPReportRecordItemText* pText = (CXTPReportRecordItemText*)GetItem(2);

			if (m_pAddresTask->HasFinishedWithoutError())
			{
				m_validAddress = m_pAddresTask->GetResolvedAddress();
				pText->SetValue(m_validAddress);
				pText->SetIconIndex(3); // green

				// start the game info query task
				m_pGameInfoTask = new LiveCreate::CBGTask_GetHostInfoPacket(m_validAddress);
				m_pGameInfoTask->AddRef();
				GetIEditor()->GetBackgroundTaskManager()->AddTask(m_pGameInfoTask, eTaskPriority_RealtimePreview, eTaskThreadMask_Any);
			}
			else
			{
				pText->SetValue("INVALID");
				pText->SetIconIndex(1); // red
			}

			SAFE_RELEASE(m_pAddresTask);
			return true;
		}
	}

	if (NULL != m_pGameInfoTask)
	{
		if (m_pGameInfoTask->HasFinished())
		{
			CXTPReportRecordItemText* pText = (CXTPReportRecordItemText*)GetItem(3);

			if (m_pGameInfoTask->HasFinishedWithoutError())
			{
				m_gameName = m_pGameInfoTask->GetInfoPacket().gameFolder;
				m_buildExecutable = m_pGameInfoTask->GetInfoPacket().buildExecutable;
				m_buildDirectory = m_pGameInfoTask->GetInfoPacket().buildDirectory;
				pText->SetTooltip(m_buildExecutable);
				pText->SetValue(m_gameName);
				pText->SetIconIndex(3); // green
			}
			else
			{
				pText->SetValue("Not running");
			}
		}

		SAFE_RELEASE(m_pGameInfoTask);
		return true;
	}

	return false;
}

void CLiveCreateTargetRecord::CreateItems()
{
	AddItem(new CXTPReportRecordItemText(_T("")));
	AddItem(new CXTPReportRecordItemText(_T("")));
	AddItem(new CXTPReportRecordItemText(_T("")));
	AddItem(new CXTPReportRecordItemText(_T("")));
}

void CLiveCreateTargetRecord::GetItemMetrics( XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics)
{
	CXTPReportRecord::GetItemMetrics(pDrawArgs, pItemMetrics);
}

//-----------------------------------------------------------------------------


IMPLEMENT_DYNAMIC(CLiveCreateAddTargetsDlg, CDialog)

#define ID_TIMER_UPDATE_TARGET_LIST 1001

void CLiveCreateAddTargetsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDOK, m_btnOK);
	DDX_Control(pDX, IDC_REFRESH, m_btnRefresh);
}

BEGIN_MESSAGE_MAP(CLiveCreateAddTargetsDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CLiveCreateAddTargetsDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CLiveCreateAddTargetsDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_REFRESH, &CLiveCreateAddTargetsDlg::OnBnClickedRefresh)
	ON_BN_CLICKED(IDC_BUTTON_ADD_PEER, &CLiveCreateAddTargetsDlg::OnBnClickedAddCustom)
	ON_BN_CLICKED(IDC_BUTTON_ADD_MATERIAL, &CLiveCreateAddTargetsDlg::OnBnClickedAddByIP)
	ON_WM_TIMER()
END_MESSAGE_MAP()

CLiveCreateAddTargetsDlg::CLiveCreateAddTargetsDlg(CWnd* pParent /*= NULL*/)
	: CDialog(CLiveCreateAddTargetsDlg::IDD, pParent)
	, m_pTargetSearchTask(NULL)
{
}

CLiveCreateAddTargetsDlg::~CLiveCreateAddTargetsDlg()
{
	for (size_t i=0; i<m_pTargetSearchTask.size(); ++i)
	{
		m_pTargetSearchTask[i]->Cancel();
		m_pTargetSearchTask[i]->Release();
	}

	m_pTargetSearchTask.clear();
}

BOOL CLiveCreateAddTargetsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	if( !m_lstTargets.SubclassDlgItem(IDC_LIST_PEERS, this))
	{
		return FALSE;
	}

	m_lstTargets.AddColumn(new CXTPReportColumn(0, "Platform", 40));
	m_lstTargets.AddColumn(new CXTPReportColumn(1, "Target", 70));
	m_lstTargets.AddColumn(new CXTPReportColumn(2, "Address", 70));
	m_lstTargets.AddColumn(new CXTPReportColumn(3, "Game", 70));
	m_lstTargets.AllowEdit(FALSE);
	m_lstTargets.GetReportHeader()->AllowColumnRemove(FALSE);
	m_lstTargets.GetReportHeader()->AllowColumnSort(FALSE);
	m_lstTargets.GetPaintManager()->SetGridStyle(TRUE, xtpReportGridSmallDots);

	m_imglstTargets.Create(16, 16, ILC_COLOR32, 0, 1);
	m_imglstTargets.Add(LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(IDI_BALL_DISABLED))); // gray
	m_imglstTargets.Add(LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(IDI_BALL_OFFLINE))); // red
	m_imglstTargets.Add(LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(IDI_BALL_PENDING))); // yellow
	m_imglstTargets.Add(LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(IDI_BALL_ONLINE))); // green
	m_lstTargets.SetImageList(&m_imglstTargets);

	m_lstTargets.GetPaintManager()->m_strNoItems = "No targets discovered.";

	OnBnClickedRefresh();

	return TRUE;
}

void CLiveCreateAddTargetsDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == ID_TIMER_UPDATE_TARGET_LIST)
	{
		// searching
		if (!m_pTargetSearchTask.empty())
		{
			bool bTargetsAdded = false;

			TTargetSearchTasks::const_iterator it = m_pTargetSearchTask.begin();
			while (it != m_pTargetSearchTask.end())
			{
				LiveCreate::CBGTask_SearchForHostsForPlatform* pTask = (*it);
				if (pTask->HasFinished())
				{
					// create host entries for every located target
					const uint numTargets = pTask->GetNumTargets();
					for (uint32 i=0; i<numTargets; ++i)
					{
						const LiveCreate::IPlatformHandlerFactory::TargetInfo& target = pTask->GetTarget(i);

						CLiveCreateTargetRecord* pRec = new CLiveCreateTargetRecord(target);
						m_lstTargets.AddRecord(pRec);
						bTargetsAdded = true;
					}

					// remove task from list
					pTask->Release();
					it = m_pTargetSearchTask.erase(it);
				}
				else
				{
					++it;
				}
			}

			// no more tasks left, reenable the "Refresh" button
			if (m_pTargetSearchTask.empty())
			{
				m_btnRefresh.EnableWindow(TRUE);
				m_btnRefresh.SetWindowText("Refresh");
				m_btnRefresh.UpdateWindow();
				m_btnRefresh.RedrawWindow();
			}

			// refresh list
			if (bTargetsAdded)
			{
				m_lstTargets.Populate();
				m_lstTargets.RedrawControl();
				m_lstTargets.Invalidate(TRUE);
			}
		}

		// internal update
		bool bNeedsRedraw = false;
		{
			const uint numItems = m_lstTargets.GetRecords()->GetCount();
			for (uint i=0; i<numItems; ++i)
			{
				CLiveCreateTargetRecord* pRec = (CLiveCreateTargetRecord*)m_lstTargets.GetRecords()->GetAt(i);
				if (NULL!=pRec)
				{
					bNeedsRedraw |= pRec->UpdateTasks();
				}
			}
		}

		if (bNeedsRedraw)
		{
			m_lstTargets.RedrawControl();
			m_lstTargets.Invalidate(FALSE);
		}
	}
}

void CLiveCreateAddTargetsDlg::OnBnClickedOk()
{
	const uint numItems = m_lstTargets.GetSelectedRows()->GetCount();
	if (numItems == 0)
	{
		CDialog::OnCancel();
	}
	else
	{
		for (uint i=0; i<numItems; ++i)
		{		
			CLiveCreateTargetRecord* pRec = (CLiveCreateTargetRecord*)m_lstTargets.GetSelectedRows()->GetAt(i)->GetRecord();
			if (NULL!=pRec )
			{
				TargetInformation info;
				info.m_pFactory = pRec->m_pFactory;
				info.m_platformName = pRec->m_pFactory->GetPlatformName();
				info.m_targetName = pRec->m_targetName;
				info.m_validAddress = pRec->m_validAddress;
				info.m_buildDirectory = pRec->m_buildDirectory;
				info.m_buildExecutable = pRec->m_buildExecutable;
				m_outputTargets.push_back(info);
			}
		}

		CDialog::OnOK();
	}
}

void CLiveCreateAddTargetsDlg::OnBnClickedCancel()
{
	CDialog::OnCancel();
}

void CLiveCreateAddTargetsDlg::OnBnClickedRefresh()
{
	// The button is disabled until the search completes
	m_btnRefresh.EnableWindow(FALSE);
	m_btnRefresh.SetWindowText("Scanning...");
	m_btnRefresh.UpdateWindow();
	m_btnRefresh.RedrawWindow();

	// Clear current list
	m_lstTargets.ResetContent(TRUE);

	// Start refreshing the UI from time to time
	SetTimer(ID_TIMER_UPDATE_TARGET_LIST, 100, NULL);

	// Create target search task for every platform
	const bool bUseTargetAPI = true;
	if (bUseTargetAPI)
	{
		const uint32 numPlatforms = gEnv->pLiveCreateManager->GetNumPlatforms();
		for (uint32 i=0; i<numPlatforms; ++i)
		{
			LiveCreate::IPlatformHandlerFactory* pFactory = gEnv->pLiveCreateManager->GetPlatformFactory(i);
			LiveCreate::CBGTask_SearchForHostsForPlatform* pTask = new LiveCreate::CBGTask_SearchForHostsForPlatform(pFactory);
			pTask->AddRef();
			GetIEditor()->GetBackgroundTaskManager()->AddTask(pTask, eTaskPriority_RealtimePreview, eTaskThreadMask_Any);
			m_pTargetSearchTask.push_back(pTask);
		}
	}
}

void CLiveCreateAddTargetsDlg::OnBnClickedAddCustom()
{
	CLiveCreateEditConnectionDlg dlg;

	CLiveCreateEditConnectionDlg::Parameters params;
	params.bAdding = true;
	params.bIsEnabled = false;
	dlg.SetParameters(params);

	if (dlg.DoModal() == IDCANCEL)
		return;

	// get the parameters back
	params = dlg.GetParameters();

	// create a host info object for the created parameters
	TargetInformation targetInfo;
	targetInfo.m_platformName = params.platformName;
	targetInfo.m_targetName = params.targetName.IsEmpty() ? params.address : params.targetName;
	targetInfo.m_validAddress = params.address;
	targetInfo.m_buildDirectory = params.buildDirectory;
	targetInfo.m_buildExecutable = params.buildExecutable;
	m_outputTargets.push_back(targetInfo);

	CDialog::OnOK();
}

void CLiveCreateAddTargetsDlg::OnBnClickedAddByIP()
{
	CLiveCreateAddByIpDlg dlg;

	if (dlg.DoModal() == IDCANCEL)
		return;

	// get the parameters back
	const CString ip = dlg.GetIP();

	// create a host info object for the created parameters
	TargetInformation targetInfo;
	targetInfo.m_platformName = "Any";
	targetInfo.m_targetName = ip;
	targetInfo.m_validAddress = ip;
	targetInfo.m_buildDirectory = "";
	targetInfo.m_buildExecutable = "";
	m_outputTargets.push_back(targetInfo);

	CDialog::OnOK();
}

#endif