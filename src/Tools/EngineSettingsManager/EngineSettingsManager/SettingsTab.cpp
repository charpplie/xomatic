#include "stdafx.h"
#include "EngineSettingsManager.h"
#include "SettingsTab.h"



#include "Utils.h"


IMPLEMENT_DYNAMIC(CSettingsTab, CTabCtrl)

BEGIN_MESSAGE_MAP(CSettingsTab, CTabCtrl)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////
CSettingsTab::CSettingsTab()
	: m_settingsDialog(0)
	, m_rcDialog(0)
	, m_editorDialog(0)
	, m_shaderCompilerDialog(0)
{
}


//////////////////////////////////////////////////////////////////////////
CSettingsTab::~CSettingsTab()
{
}


//////////////////////////////////////////////////////////////////////////
void CSettingsTab::Create(CEngineSettingsManager* settingsManager)
{
	m_settingsManager = settingsManager;

	InsertItem(0, "Engine", 0);
	InsertItem(1, "Editor", 0);
	InsertItem(2, "ResourceCompiler", 0);
	InsertItem(3, "ShaderCompiler", 0);

	m_settingsDialog = new CSettingsDialog();
	m_rcDialog = new CRCDialog();
	m_editorDialog = new CEditorDialog();
	m_shaderCompilerDialog = new CSCDialog();

	m_settingsDialog->Create(IDD_SETTINGSDIALOG, this, settingsManager);
	m_rcDialog->Create(IDD_RC_DIALOG, this, settingsManager);
	m_editorDialog->Create(IDD_EDITOR_DIALOG, this, settingsManager);
	m_shaderCompilerDialog->Create(IDD_SC_DIALOG, this, settingsManager);

	m_tabDialogs[0] = m_settingsDialog;
	m_tabDialogs[1] = m_editorDialog;
	m_tabDialogs[2] = m_rcDialog;
	m_tabDialogs[3] = m_shaderCompilerDialog;








	ActivateTabDialog(0);
}

//////////////////////////////////////////////////////////////////////////
void CSettingsTab::OnDestroy()
{
	if (m_shaderCompilerDialog)
	{
		m_shaderCompilerDialog->DestroyWindow();
		delete m_shaderCompilerDialog;
		m_shaderCompilerDialog = NULL;
	}

	if (m_editorDialog)
	{
		m_editorDialog->DestroyWindow();
		delete m_editorDialog;
		m_editorDialog = NULL;
	}

	if (m_rcDialog)
	{
		m_rcDialog->DestroyWindow();
		delete m_rcDialog;
		m_rcDialog = NULL;
	}

	if (m_settingsDialog)
	{
		m_settingsDialog->DestroyWindow();
		delete m_settingsDialog;
		m_settingsDialog = NULL;
	}









}


//////////////////////////////////////////////////////////////////////////
void CSettingsTab::ActivateTabDialog(int nSel)
{
	//when opening shader compiler tab, reload it
	if (nSel == 3)
		m_shaderCompilerDialog->ReloadDialog(m_settingsDialog->GetRootPath().GetBuffer());

	CRect l_rectClient;
	CRect l_rectWnd;

	GetClientRect(l_rectClient);
	AdjustRect(FALSE,l_rectClient);
	GetWindowRect(l_rectWnd);
	ScreenToClient(l_rectWnd);
	l_rectClient.OffsetRect(l_rectWnd.left,l_rectWnd.top);

	m_tabDialogs[nSel]->SetWindowPos(&wndTop, l_rectClient.left-1, l_rectClient.top, l_rectClient.Width(), l_rectClient.Height(), SWP_SHOWWINDOW);
	m_tabDialogs[nSel]->ShowWindow(SW_SHOW);
}


//////////////////////////////////////////////////////////////////////////
void CSettingsTab::DeactivateTabDialog(int nSel)
{
	m_tabDialogs[nSel]->ShowWindow(SW_HIDE);
}


//////////////////////////////////////////////////////////////////////////
bool CSettingsTab::PollSettings(CEngineSettingsManager* settingsManager) const
{
	if (m_settingsDialog->IsRootPathGood())
	{
		const tstring sRootPath = m_settingsDialog->GetRootPath().GetBuffer();
		settingsManager->SetRootPath(sRootPath);

		settingsManager->SetKey("RC_ShowWindow", m_rcDialog->IsShowWindow());
		settingsManager->SetKey("RC_HideCustom", m_rcDialog->IsHideCustom());
		settingsManager->SetKey("RC_Parameters", m_rcDialog->GetParams().GetBuffer());

		settingsManager->SetKey("EDT_Prefer32Bit", m_editorDialog->IsPrefer32Bit());

		return true;
	}

	return false;
}


//////////////////////////////////////////////////////////////////////////
bool CSettingsTab::SaveSettingsToSystemCfg() const
{
	if (m_settingsDialog->IsRootPathGood())
	{
		const tstring sRootPath = m_settingsDialog->GetRootPath().GetBuffer();
		m_shaderCompilerDialog->SaveSettingsToSystemCfg(sRootPath);









		return true;
	}

	return false;
}
