#include "stdafx.h"
#include "EngineSettingsManager.h"
#include "SCDialog.h"
#include "Utils.h"

IMPLEMENT_DYNAMIC(CSCDialog, CDialog)


//////////////////////////////////////////////////////////////////////////
CSCDialog::CSCDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CSCDialog::IDD, pParent)
{
}


//////////////////////////////////////////////////////////////////////////
void CSCDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_REMOTE_COMPILER_CB, m_buttonShadersRemoteCompile);
	DDX_Control(pDX, IDC_SERVER_EDIT, m_editShaderCompileServer);
	DDX_Control(pDX, IDC_SERVER_STATIC, m_labelServer);
	DDX_Control(pDX, IDC_INFO_STATIC, m_labelInfo);
}


//////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CSCDialog, CDialog)
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////
void CSCDialog::Create(UINT temp, CWnd* parent, CEngineSettingsManager* settingsManager)
{
	m_settingsManager = settingsManager;
	CDialog::Create(temp, parent);
}


//////////////////////////////////////////////////////////////////////////
BOOL CSCDialog::OnInitDialog()
{
	CDialog::OnInitDialog();	

	ReloadDialog((string)m_settingsManager->GetRootPath());

	return TRUE;
}

void CSCDialog::ReloadDialog(const string& sRootPath)
{
	//read settings from system.cfg
	if (LoadSettingsFromSystemCfg(sRootPath))
	{
		//if succeeded enable controls
		m_labelInfo.SetWindowText("");
		m_labelServer.EnableWindow(true);
		m_buttonShadersRemoteCompile.EnableWindow(true);
		m_editShaderCompileServer.EnableWindow(true);
	}
	else
	{
		//else disable controls and set to defaults
		m_labelServer.EnableWindow(false);
		m_buttonShadersRemoteCompile.EnableWindow(false);
		m_editShaderCompileServer.EnableWindow(false);
		m_buttonShadersRemoteCompile.SetCheck(BST_UNCHECKED);
		m_editShaderCompileServer.SetWindowText("");
		m_labelInfo.SetWindowText("system.cfg not found!\n\nRoot Directory not configured properly?");
	}
}

//////////////////////////////////////////////////////////////////////////
bool CSCDialog::LoadSettingsFromSystemCfg(const string& sRootPath)
{
	//open system.cfg for reading
	string sFileName = sRootPath + "\\system.cfg";

	FILE* pFile = fopen(sFileName.c_str(), "rb");

	//if file not found, cancel
	if (pFile==NULL)
		return false;

	//read file to memory
	fseek(pFile,0,SEEK_END);
	long size = ftell(pFile);
	fseek(pFile,0,SEEK_SET);
	char* pData = new char[size+1];
	pData[size] = 0;
	size = fread(pData,sizeof(char),size,pFile);
	fclose(pFile);

	//parse file for data

	int start = 0, end = 0;
	while (end<size)
	{
		while (pData[end]!='\n' && end<size)
			end++;

		memcpy(pData,&pData[start],end-start);
		pData[end-start] = 0;
		start = ++end;

		string sLine(pData);	

		int iSearchPos = int(sLine.find('='));
		if (iSearchPos>=0)
		{
			string sKey(CUtils::Trim(sLine.substr(0, iSearchPos)));
			string sValue(CUtils::Trim(sLine.substr(iSearchPos+1)));

			//ignore comments
			iSearchPos = int(sKey.find(';'));
			if (iSearchPos==0)
				continue;
			iSearchPos = int(sKey.find("--"));
			if (iSearchPos==0)
				continue;

			//get values if keys found
			if (sKey=="r_ShaderCompilerServer")
				m_editShaderCompileServer.SetWindowText(sValue.c_str());
			else if (sKey=="r_ShadersRemoteCompiler")
				m_buttonShadersRemoteCompile.SetCheck((sValue=="1") ? BST_CHECKED : BST_UNCHECKED);
		}
	}

	delete [] pData;

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CSCDialog::SaveSettingsToSystemCfg(const string& sRootPath) const
{
	//open system.cfg for reading
	string sFileName = sRootPath + "\\system.cfg";

	FILE* pFile = fopen(sFileName.c_str(), "rb");

	//if file not found, cancel
	if (pFile==NULL)
		return;

	//read file to memory
	fseek(pFile,0,SEEK_END);
	long size = ftell(pFile);
	fseek(pFile,0,SEEK_SET);
	char* pData = new char[size+1];
	pData[size] = 0;
	size = fread(pData,sizeof(char),size,pFile);
	fclose(pFile);

	//open/create system.cfg for writing back modified content
	pFile = fopen(sFileName.c_str(), "wb");

	//if file not opened/created, cancel
	if (pFile==NULL)
		return;

	//parse read data, modify if appropriate and write back
	bool bFoundShaderServer = false, bFoundRemoteCompiler = false;
	int start = 0, end = 0;
	while (end<size)
	{
		while (pData[end]!='\n' && end<size)
			end++;

		memcpy(pData,&pData[start],end-start);
		pData[end-start] = 0;
		start = ++end;

		string sLine(pData);
		sLine.append("\n");

		int iSearchPos = int(sLine.find('='));
		if (iSearchPos>=0)
		{
			string sKey(CUtils::Trim(sLine.substr(0, iSearchPos)));
			string sValue(CUtils::Trim(sLine.substr(iSearchPos+1)));

			//set values if keys found
			if (sKey=="r_ShaderCompilerServer")
			{
				CString sRet;
				m_editShaderCompileServer.GetWindowText(sRet);
				sValue = sRet;
				bFoundShaderServer = true;
			}
			else if (sKey=="r_ShadersRemoteCompiler")
			{
				sValue = (m_buttonShadersRemoteCompile.GetCheck()!=0) ? "1" : "0";
				bFoundRemoteCompiler = true;
			}

			//build new string line
			sLine = sKey + "=" + sValue + "\r\n";
		}

		fputs(sLine.c_str(),pFile);
	}
	
	//if cvars have not yet been existing but were modified, add them
	CString sRet;
	m_editShaderCompileServer.GetWindowText(sRet);
	string sShaderCompilerServer = CUtils::Trim((string)sRet);
	if (!bFoundShaderServer && sShaderCompilerServer!="")
	{
		string sLine("r_ShaderCompilerServer=" + sShaderCompilerServer + "\r\n");
		fputs(sLine.c_str(), pFile);
	}
	if (!bFoundRemoteCompiler && m_buttonShadersRemoteCompile.GetCheck()!=0)
	{
		fputs("r_ShadersRemoteCompiler=1\r\n", pFile);
	}

	fclose(pFile);

	delete [] pData;
}
