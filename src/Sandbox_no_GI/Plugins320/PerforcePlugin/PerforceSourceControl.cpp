////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   PerforceSourceControl.cpp
//  Version:     v1.00
//  Created:     22 Sen 2004 by Sergiy Shaykin.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CryFile.h"


#include "Include/ISourceControl.h"
#include "Include/IEditorClassFactory.h"


#include "PerforceSourceControl.h"
#include "PasswordDlg.h"

#define BBox BBox1
#include "platform_impl.h"



namespace
{
	CryCriticalSection g_cPerforceValues;
}


struct CPerforceThread : CrySimpleThread<>
{
	const char* m_filename;
	CPerforceSourceControl* m_pSourceControl;

	CPerforceThread(CPerforceSourceControl* pSourceControl, const char *filename)
	{
		m_pSourceControl = pSourceControl;
		m_filename = filename;
	}

	virtual ~CPerforceThread()
	{
		Stop();
	}

protected:
	virtual void Run()
	{
		CryThreadSetName(-1, "PerforcePlugin");
		m_pSourceControl->GetFileAttributesThread(m_filename);
	}
};




void CMyClientUser::HandleError(Error *e)
{
	/*
	StrBuf  m;
	e->Fmt( &m );
	if ( strstr( m.Text(), "file(s) not in client view." ) )
	e->Clear();
	else if ( strstr( m.Text(), "no such file(s)" ) )
	e->Clear();
	else if ( strstr( m.Text(), "access denied" ) )
	e->Clear();
	/**/
	m_e = *e;
}


void CMyClientUser::Init()
{
	m_bIsSetup = false;
	m_bIsPreCreateFile = false;
}


void CMyClientUser::PreCreateFileName(const char * file)
{
	m_bIsPreCreateFile = true;
	strcpy(m_file, file);
	m_findedFile[0]=0;
}


void CMyClientUser::OutputStat( StrDict *varList )
{
	if(m_bIsSetup && !m_bIsPreCreateFile)
		return;

	StrRef var, val;
	*m_action=0;
	*m_headAction=0;
	*m_otherAction=0;
	*m_depotFile=0;

	for( int i = 0; varList->GetVar( i, var, val ); i++ )
	{
		if(m_bIsPreCreateFile)
		{
			if(var=="clientFile")
			{
				char tmpval[MAX_PATH];
				strcpy(tmpval, val.Text());
				char * ch = tmpval;
				while(ch = strchr(ch, '/'))
					*ch = '\\';

				if(!stricmp(tmpval, m_file))
				{
					strcpy(m_findedFile, val.Text());
					m_bIsPreCreateFile = false;
				}
			}
		}
		else
		{
			if(var=="action")
				strcpy(m_action, val.Text());
			else if(var=="headAction")
				strcpy(m_headAction, val.Text());
			else if (var == "headRev")
				strcpy(m_clientHasLatestRev, val.Text());
			else if (!strncmp(var.Text(), "otherAction", 11) && !strcmp(val.Text(), "edit"))
				strcpy(m_otherAction, val.Text());
			else if (var == "haveRev")
			{
				if (val != m_clientHasLatestRev)
					m_clientHasLatestRev[0] = 0;
			}
			else if (var == "depotFile")
				strncpy(m_depotFile, val.Text(), (MAX_PATH)-1);
		}
	}
	m_bIsSetup = true;
}


void CMyClientUser::OutputInfo( char level, const char *data )
{
	if(!m_bIsPreCreateFile)
		return;

	const char * ch = strrchr(data, '/');
	if(ch)
	{
		if(!stricmp(ch+1, m_file))
		{
			strcpy(m_findedFile, ch+1);
			m_bIsPreCreateFile = false;
		}
	}
}

void CMyClientUser::Edit( FileSys *f1, Error *e )
{
	char msrc[4000];
	char mdst[4000];

	char * src=&msrc[0];
	char * dst=&mdst[0];

	f1->Open(FOM_READ, e);
	int size = f1->Read(msrc, 10240, e);
	msrc[size]=0;
	f1->Close(e);

	while(*dst=*src)
	{
		if(!strnicmp(src, "\nDescription", 11))
		{
			src++;
			while(*src!='\n' && *src!='\0')
				src++;
			src++;
			while(*src!='\n' && *src!='\0')
				src++;
			src--;
			strcpy(dst, "\nDescription:\n\t!Sandbox: ");
			dst += 25;
			strcpy(dst, m_desc);
			dst += strlen(m_desc)-1;
		}
		src++;
		dst++;
	}

	f1->Open(FOM_WRITE, e);
	f1->Write(mdst, strlen(mdst), e);
	f1->Close(e);

	strcpy(m_desc, m_initDesc);
}



////////////////////////////////////////////////////////////

extern ISystem* g_pSystem;

CPerforceSourceControl::CPerforceSourceControl() : m_ref(0)
{
	m_thread = 0;
	m_bIsWorkOffline = false;
	m_bIsFailConnectionLogged = false;
	m_dwLastAccessTime = 0;
	Connect();
	InitCommonControls();
}


CPerforceSourceControl::~CPerforceSourceControl()
{
	if(m_thread)
	{
		m_thread->WaitForThread();
		delete m_thread;
	}
}


bool CPerforceSourceControl::Connect()
{
	m_client.Init( &m_e );
	if ( m_e.Test() )
	{
		//m_bIsWorkOffline = true;
		if(!m_bIsFailConnectionLogged)
		{
			g_pSystem->GetILog()->Log("\nPerforce plugin: Failed to connect.");
			m_bIsFailConnectionLogged = true;
		}
		return false;
	}
	else
	{
		//m_bIsWorkOffline = false;
		g_pSystem->GetILog()->Log("\nPerforce plugin: Connected.");
		m_bIsFailConnectionLogged = false;
	}
	return true;
}

bool CPerforceSourceControl::Reconnect()
{
	if ( m_client.Dropped() )
	{
		m_bIsWorkOffline = true;
		if(!m_bIsFailConnectionLogged)
		{
			g_pSystem->GetILog()->Log("\nPerforce connection dropped: attempting reconnect");
		}
		FreeData();
		if(!Connect())
			return false;
	}
	return true;
}

void CPerforceSourceControl::FreeData()
{
	m_client.Final( &m_e );
}


void CPerforceSourceControl::ConvertFileNameCS(char *sDst, const char *sSrcFilename)
{
	*sDst = 0;
	bool bFinded = true;

	char szAdjustedFile[ICryPak::g_nMaxPath];
	strcpy(szAdjustedFile, sSrcFilename);

	//_finddata_t fd;
	WIN32_FIND_DATA fd;

	char csPath[ICryPak::g_nMaxPath]={0};

	char * ch, *ch1;

	ch = strrchr(szAdjustedFile, '\\');
	ch1 = strrchr(szAdjustedFile, '/');
	if(ch < ch1) ch = ch1;
	bool bIsFirstTime = true;

	bool bIsEndSlash = false;
	if(ch && ch-szAdjustedFile+1 == strlen(szAdjustedFile))
		bIsEndSlash = true;

	while(ch)
	{
		//intptr_t handle;
		//handle = gEnv->pCryPak->FindFirst( szAdjustedFile, &fd );
		HANDLE handle = FindFirstFile( szAdjustedFile, &fd );
		//if (handle != -1)
		if(handle != INVALID_HANDLE_VALUE)
		{
			char tmp[ICryPak::g_nMaxPath];
			strcpy(tmp, csPath);
			//strcpy(csPath, fd.name);
			strcpy(csPath, fd.cFileName);
			if(!bIsFirstTime)
				strcat(csPath, "\\");
			bIsFirstTime = false;
			strcat(csPath, tmp);

			//gEnv->pCryPak->FindClose( handle );
			FindClose(handle);
		}

		*ch = 0;
		ch = strrchr(szAdjustedFile, '\\');
		ch1 = strrchr(szAdjustedFile, '/');
		if(ch < ch1) ch = ch1;
	}

	if(!*csPath)
		return;

	strcat(szAdjustedFile,"\\");
	strcat(szAdjustedFile, csPath);
	if(bIsEndSlash || strlen(szAdjustedFile) < strlen(sSrcFilename))
		strcat(szAdjustedFile, "\\");

	// if we have only folder on disk find in perforce
	if(strlen(szAdjustedFile) < strlen(sSrcFilename))
	{
		if(IsFileManageable(szAdjustedFile))
		{
			char file[MAX_PATH];
			char clienFile[MAX_PATH]={0};

			bool bCont = true;
			while (bCont)
			{
				strcpy(file, &sSrcFilename[strlen(szAdjustedFile)]);
				char * ch = strchr(file, '/');
				char * ch1 = strchr(file, '\\');
				if(ch < ch1) ch = ch1;

				if(ch)
				{
					*ch=0;
					bFinded = bCont = FindDir(clienFile, szAdjustedFile, file);
				}
				else
				{
					bFinded = FindFile(clienFile, szAdjustedFile, file);
					bCont = false;
				}
				strcpy(szAdjustedFile, clienFile);
				if(bCont && strlen(clienFile)>=strlen(sSrcFilename))
					bCont = false;
			}
		}
	}

	if(bFinded)
		strcpy(sDst, szAdjustedFile);
}


void CPerforceSourceControl::MakePathCS(char *sDst, const char *sSrcFilename)
{
	char szAdjustedFile[ICryPak::g_nMaxPath];
	char szCheckedPath[ICryPak::g_nMaxPath];
	strcpy(szAdjustedFile, sSrcFilename);

	char * ch = &szAdjustedFile[0];
	char * ch1;

	while(ch)
	{
		ch1 = strchr(ch, '/');
		ch = strchr(ch, '\\');

		if(ch1 && ch > ch1) ch = ch1;

		if(ch)
		{
			strncpy(szCheckedPath, szAdjustedFile, ch-szAdjustedFile+1);
			szCheckedPath[ch-szAdjustedFile+1]=0;
			if(IsFileManageable(szCheckedPath, false))
			{
				strcpy(szAdjustedFile, szCheckedPath);
				break;
			}
			ch++;
		}
	}

	if(strlen(szAdjustedFile) < strlen(sSrcFilename))
	{
		if(IsFileManageable(szAdjustedFile))
		{
			char file[MAX_PATH];
			char clienFile[MAX_PATH]={0};

			bool bCont = true;
			while (bCont)
			{
				strcpy(file, &sSrcFilename[strlen(szAdjustedFile)]);
				char * ch = strchr(file, '/');
				char * ch1 = strchr(file, '\\');
				if(ch < ch1) ch = ch1;

				bool bFinded = false;

				if(ch)
				{
					*ch=0;
					bFinded = bCont = FindDir(clienFile, szAdjustedFile, file);
				}

				if(!bFinded)
				{
					strcpy(&szAdjustedFile[strlen(szAdjustedFile)], &sSrcFilename[strlen(szAdjustedFile)]);
					break;
				}

				strcpy(szAdjustedFile, clienFile);
				if(bCont && strlen(clienFile)>=strlen(sSrcFilename))
					bCont = false;
			}
		}
	}

	if(strlen(szAdjustedFile) ==  strlen(sSrcFilename))
		strcpy(sDst, szAdjustedFile);
	else
		strcpy(sDst, sSrcFilename);
}


void CPerforceSourceControl::RenameFolders(const char * path, const char * pathOld)
{
	const char * ch = strchr(pathOld, '\\');
	if(ch)
		ch = strchr(ch+1, '\\');
	while(ch)
	{
		ch++;
		const char * ch1 = strchr(ch, '\\');
		if(ch1 && strncmp(ch, &path[ch-pathOld], ch1-ch))
		{
			char newpath[ICryPak::g_nMaxPath];
			char newpathOld[ICryPak::g_nMaxPath];
			strncpy(newpath, path, ch1-pathOld);
			newpath[ch1-pathOld]=0;
			strncpy(newpathOld, pathOld, ch1-pathOld);
			newpathOld[ch1-pathOld]=0;
			MoveFile( newpathOld, newpath );
		}
		ch = ch1;
	}
}


bool CPerforceSourceControl::FindDir(char * clientFile, const char * folder, const char * dir)
{
	Reconnect();
	char fl[MAX_PATH];
	strcpy(fl, folder );
	strcat(fl, "*" );
	char * argv[] = { fl};
	m_ui.PreCreateFileName(dir);

	m_client.SetArgv( 1, argv );
	m_client.Run( "dirs", &m_ui );
	m_client.WaitTag();

	if(m_ui.m_e.Test())
		return false;

	strcpy(clientFile, folder);
	strcat(clientFile, m_ui.m_findedFile);
	strcat(clientFile, "\\");
	if(*m_ui.m_findedFile)
		return true;

	return false;
}


bool CPerforceSourceControl::FindFile(char * clientFile, const char * folder, const char * file)
{
	Reconnect();
	char fullPath[MAX_PATH];

	strcpy(fullPath, folder);
	strcat(fullPath, file);

	char fl[MAX_PATH];
	strcpy(fl, folder );
	strcat(fl, "*" );
	char * argv[] = { fl};
	m_ui.PreCreateFileName(fullPath);

	m_client.SetArgv( 1, argv );
	m_client.Run( "fstat", &m_ui );
	m_client.WaitTag();

	if(m_ui.m_e.Test())
		return false;

	strcpy(clientFile, m_ui.m_findedFile);
	if(*clientFile)
		return true;

	return false;
}


bool CPerforceSourceControl::IsFileManageable(const char *sFilename, bool bCheckFatal)
{
	if(!Reconnect())
		return false;

	bool bRet=false;
	bool fatal = false;

	if(m_bIsWorkOffline)
		return false;

	char fl[MAX_PATH];
	strcpy(fl, sFilename);
	//ConvertFileNameCS(fl, sFilename);

	char * argv[] = { fl};
	Run("fstat", 1, argv, true);

	m_ui.Init();
	m_client.SetArgv( 1, argv );
	m_client.Run( "fstat", &m_ui ); 
	m_client.WaitTag();

	if(bCheckFatal && m_ui.m_e.IsFatal())
	{
		fatal = true;
	}

	if(!fatal && !m_ui.m_e.IsError())
	{
		bRet=true;
	}

	m_ui.m_e.Clear();
	return bRet;
}

bool CPerforceSourceControl::IsFileExistsInDatabase(const char *sFilename)
{
	Reconnect();
	bool bRet=false;

	char fl[MAX_PATH];
	strcpy(fl, sFilename);
	char * argv[] = { fl};
	m_ui.Init();
	m_client.SetArgv( 1, argv );
	m_client.Run( "fstat", &m_ui ); 
	m_client.WaitTag();

	if(!m_ui.m_e.Test())
	{
		if(strcmp(m_ui.m_headAction, "delete"))
			bRet=true;
	}
	else
	{
		//  StrBuf errorMsg;
		//  m_ui.m_e.Fmt(&errorMsg);
	}
	m_ui.m_e.Clear();
	return bRet;
}

bool CPerforceSourceControl::IsFileCheckedOutByUser(const char *sFilename, bool * pIsByAnotherUser)
{
	Reconnect();
	bool bRet=false;

	char fl[MAX_PATH];
	strcpy(fl, sFilename);
	char * argv[] = {fl};
	m_ui.Init();
	m_client.SetArgv( 1, argv );
	m_client.Run( "fstat", &m_ui ); 
	m_client.WaitTag();

	if((!strcmp(m_ui.m_action,"edit") || !strcmp(m_ui.m_action,"add")) && !m_ui.m_e.Test())
		bRet=true;

	if(pIsByAnotherUser)
	{
		*pIsByAnotherUser=false;
		if(!strcmp(m_ui.m_otherAction,"edit") && !m_ui.m_e.Test())
			*pIsByAnotherUser=true;
	}

	m_ui.m_e.Clear();
	return bRet;
}

bool CPerforceSourceControl::IsFileLatestVersion(const char *sFilename)
{
	Reconnect();
	bool bRet=false;

	char fl[MAX_PATH];
	strcpy(fl, sFilename);
	char * argv[] = { fl};
	m_ui.Init();
	m_client.SetArgv( 1, argv );
	m_client.Run( "fstat", &m_ui ); 
	m_client.WaitTag();

	if (m_ui.m_clientHasLatestRev[0] != 0)
		bRet=true;
	m_ui.m_e.Clear();
	return bRet;
}


uint32 CPerforceSourceControl::GetFileAttributesAndFileName( const char *filename, char * FullFileName )
{
//	g_pSystem->GetILog()->Log("\n checking connection");
	Reconnect();

	if(FullFileName)
		FullFileName[0]=0;

	bool bCryFile = true;

	CCryFile file;
	if (!file.Open(filename,"rb"))
	{
		bCryFile = false;
	}

	uint32 attributes = 0;

	char sFullFilenameLC[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, sFullFilenameLC);
	strcat(sFullFilenameLC, "\\");
	if(bCryFile)
	{
		if(*sFullFilenameLC && !strnicmp(sFullFilenameLC, filename, strlen(sFullFilenameLC)))
			strcpy(sFullFilenameLC, file.GetAdjustedFilename());
		else
			strcat(sFullFilenameLC, file.GetAdjustedFilename());
	}
	else
		strcat(sFullFilenameLC, filename);

	char sFullFilename[ICryPak::g_nMaxPath];
	ConvertFileNameCS(sFullFilename, sFullFilenameLC);

	if(FullFileName)
		strcpy(FullFileName, sFullFilename);

	if (bCryFile && file.IsInPak())
	{
//		g_pSystem->GetILog()->Log("\n file is in pak");
		attributes = SCC_FILE_ATTRIBUTE_READONLY|SCC_FILE_ATTRIBUTE_INPAK;

		if(IsFileManageable(sFullFilename) && IsFileExistsInDatabase (sFullFilename))
		{
//			g_pSystem->GetILog()->Log("\n file is managable and also exists in the database");
			attributes |= SCC_FILE_ATTRIBUTE_MANAGED;
			bool isByAnotherUser;
			if(IsFileCheckedOutByUser(sFullFilename, &isByAnotherUser))
				attributes |= SCC_FILE_ATTRIBUTE_CHECKEDOUT;
			if(isByAnotherUser)
				attributes |= SCC_FILE_ATTRIBUTE_BYANOTHER;
		}
		return attributes;
	}

//	g_pSystem->GetILog()->Log("\n file is not in pak");
	DWORD dwAttrib = ::GetFileAttributes(sFullFilename);
	if (dwAttrib != INVALID_FILE_ATTRIBUTES)
	{
//		g_pSystem->GetILog()->Log("\n we have valid file attributes");
		attributes = SCC_FILE_ATTRIBUTE_NORMAL;
		if (dwAttrib & FILE_ATTRIBUTE_READONLY)
			attributes |= SCC_FILE_ATTRIBUTE_READONLY;

		if(IsFileManageable (sFullFilename))
		{
//			g_pSystem->GetILog()->Log("\n file is manageable");
			if(IsFileExistsInDatabase (sFullFilename))
			{
//				g_pSystem->GetILog()->Log("\n file exists in database");
				attributes |= SCC_FILE_ATTRIBUTE_MANAGED;
				bool isByAnotherUser;
				if(IsFileCheckedOutByUser(sFullFilename, &isByAnotherUser))
				{
//					g_pSystem->GetILog()->Log("\n file is checked out");
					attributes |= SCC_FILE_ATTRIBUTE_CHECKEDOUT;
				}
				if(isByAnotherUser)
				{
//					g_pSystem->GetILog()->Log("\n by another user");
					attributes |= SCC_FILE_ATTRIBUTE_BYANOTHER;
				}
			}
		}
		return attributes;
	}

//	g_pSystem->GetILog()->Log("\n file has invalid file attributes");
	if(!bCryFile)
	{
		if(IsFileManageable (sFullFilename))
		{
			if(IsFileExistsInDatabase (sFullFilename))
			{
//				g_pSystem->GetILog()->Log("\n file is managable and exists in the database");
				attributes = SCC_FILE_ATTRIBUTE_NORMAL | SCC_FILE_ATTRIBUTE_READONLY;
				attributes |= SCC_FILE_ATTRIBUTE_MANAGED;
				bool isByAnotherUser;
				if(IsFileCheckedOutByUser(sFullFilename, &isByAnotherUser))
					attributes |= SCC_FILE_ATTRIBUTE_CHECKEDOUT;
				if(isByAnotherUser)
					attributes |= SCC_FILE_ATTRIBUTE_BYANOTHER;
				return attributes;
			}
		}
	}

//	g_pSystem->GetILog()->Log("\n getting file attributes for sc failed.");
	return SCC_FILE_ATTRIBUTE_INVALID;
}



void CPerforceSourceControl::GetFileAttributesThread( const char *filename)
{
	uint32 unRetValue = GetFileAttributesAndFileName(filename, 0);

	AUTO_LOCK(g_cPerforceValues);
	m_unRetValue = unRetValue;
	m_isSetuped = true;
}



uint32 CPerforceSourceControl::GetFileAttributes( const char* filename )
{
	//return GetFileAttributesAndFileName(filename, 0);

	DWORD dwTime = GetTickCount();

	if(m_bIsWorkOffline || dwTime - m_dwLastAccessTime < 1000)
	{
		m_dwLastAccessTime = dwTime;
		return GetFileAttributesAndFileName(filename, 0);
	}

	m_isSkipThread = false;
	m_isSetuped = false;
	uint32 unRetValue = SCC_FILE_ATTRIBUTE_INVALID;

	if(m_thread)
	{
		m_thread->WaitForThread();
		delete m_thread;
	}
	m_thread = new CPerforceThread (this, filename);
	m_thread->Start();

	DWORD dwWaitTime = 5000; // 5 sec
	DWORD dwCurTime = dwTime;

	while(1)
	{
		if(!m_isSkipThread && GetTickCount() - dwCurTime > dwWaitTime)
		{
			if (MessageBox( NULL, _T("Connection to Perforce is not responded. Do you want to switch to the offline mode?"), _T("Perforce Pluging Error"), MB_TASKMODAL | MB_ICONWARNING | MB_YESNO ) == IDYES)
			{
				m_bIsWorkOffline = true;
				break;
			}
			dwCurTime = GetTickCount();
			dwWaitTime = 10000; // 10 sec
		}

		Sleep(50);

		if(m_isSetuped)
		{
			AUTO_LOCK(g_cPerforceValues);
			unRetValue = m_unRetValue;
			break;
		}
	}

	m_dwLastAccessTime = dwTime;
	return unRetValue;
}


bool CPerforceSourceControl::IsFolder(const char * filename, char * FullFileName)
{
	bool bFolder = false;

	char sFullFilename[ICryPak::g_nMaxPath];
	ConvertFileNameCS(sFullFilename, filename);

	uint32 attr = ::GetFileAttributes(sFullFilename);
	if(attr==INVALID_FILE_ATTRIBUTES)
	{
		if(*sFullFilename && sFullFilename[strlen(sFullFilename)-1]=='\\')
			bFolder = true;
		else
			return false;
	}
	else
		if((attr & FILE_ATTRIBUTE_DIRECTORY))
			bFolder = true;

	if(bFolder )
		strcpy(FullFileName, sFullFilename);

	return bFolder;
}


bool CPerforceSourceControl::Add( const char *filename, const char * desc, int nFlags)
{
	Reconnect();
	bool bRet = false;

	char FullFileName[MAX_PATH];

	CString	str = filename;
	int curPos = 0;


	CString resToken = str.Tokenize(";",curPos);
	while (!resToken.IsEmpty())
	{
		resToken.Trim();
		//bool bFolder = false;
		uint32 attrib = GetFileAttributesAndFileName(resToken, FullFileName);
		char sFullFilename[ICryPak::g_nMaxPath];
		MakePathCS(sFullFilename, FullFileName);
		if(strcmp(sFullFilename, FullFileName))
			RenameFolders(sFullFilename, FullFileName);

		//if(attrib == SCC_FILE_ATTRIBUTE_INVALID)
		//bFolder = IsFolder(resToken, FullFileName);

		/*if(bFolder)
		{
		CNxNString sNamespacePath;
		if (m_pIntegrator->MapManagedPath(FullFileName, sNamespacePath))
		{
		CNxNItem itm;
		if(pInt->GetItem(sNamespacePath, itm))
		if(itm.Import(FullFileName, ""))
		bRet = true;
		}
		}
		else*/ if((attrib!=SCC_FILE_ATTRIBUTE_INVALID) && !(attrib & SCC_FILE_ATTRIBUTE_MANAGED) && IsFileManageable(sFullFilename))
		{
			char * argv[] = { sFullFilename};
			m_client.SetArgv( 1, argv );
			m_client.Run( "add", &m_ui );
			if(desc)
			{
				strcpy(m_ui.m_desc, desc);
				m_client.SetArgv( 1, argv );
				m_client.Run( "submit", &m_ui );
			}
			if ( !m_ui.m_e.Test() )
				bRet = true;
		}
		resToken = str.Tokenize(";",curPos);
	}


	return bRet;
}

bool CPerforceSourceControl::CheckIn( const char *filename, const char * desc, int nFlags)
{
	Reconnect();
	bool bRet = false;

	char FullFileName[MAX_PATH];

	CString	str = filename;
	int curPos = 0;

	CString resToken = str.Tokenize(";",curPos);
	while (!resToken.IsEmpty())
	{
		resToken.Trim();

		bool bFolder=false;
		uint32 attrib = GetFileAttributesAndFileName(resToken, FullFileName);
		if(attrib == SCC_FILE_ATTRIBUTE_INVALID)
			bFolder = IsFolder(resToken, FullFileName);

		if(bFolder)
			strcat(FullFileName, "...");

		if(bFolder || ((attrib != SCC_FILE_ATTRIBUTE_INVALID) && (attrib & SCC_FILE_ATTRIBUTE_MANAGED) && (attrib & SCC_FILE_ATTRIBUTE_CHECKEDOUT)))
		{
			{
				char * argv[] = { "-c", "default", FullFileName};
				m_client.SetArgv( 3, argv );
				m_client.Run( "reopen", &m_ui );
			}

			if(desc)
				strcpy(m_ui.m_desc, desc);
			{
				char * argv[] = { FullFileName};
				m_client.SetArgv( 1, argv );
				m_client.Run( "submit", &m_ui );
			}

			if ( !m_ui.m_e.Test() )
				bRet=true;
		}
		resToken = str.Tokenize(";",curPos);
	}

	return bRet;
}

bool CPerforceSourceControl::CheckOut( const char *filename, int nFlags)
{
	Reconnect();
	bool bRet = false;

	char FullFileName[MAX_PATH];

	bool bFolder=false;
	uint32 attrib = GetFileAttributesAndFileName(filename, FullFileName);
	if(attrib == SCC_FILE_ATTRIBUTE_INVALID)
		bFolder = IsFolder(filename, FullFileName);

	if(bFolder)
		strcat(FullFileName, "...");

	if(bFolder || ((attrib & SCC_FILE_ATTRIBUTE_MANAGED) && !(attrib & SCC_FILE_ATTRIBUTE_CHECKEDOUT) ))
	{
		char * argv[] = { FullFileName};
		m_client.SetArgv( 1, argv );
		m_client.Run( "edit", &m_ui );
		if ( !m_ui.m_e.Test() )
			bRet=true;
	}
	return bRet;
}

bool CPerforceSourceControl::UndoCheckOut( const char *filename, int nFlags)
{
	Reconnect();
	bool bRet = false;

	char FullFileName[MAX_PATH];

	bool bFolder=false;
	uint32 attrib = GetFileAttributesAndFileName(filename, FullFileName);
	if(attrib == SCC_FILE_ATTRIBUTE_INVALID)
		bFolder = IsFolder(filename, FullFileName);

	if(bFolder)
		strcat(FullFileName, "...");

	if(bFolder  || ((attrib & SCC_FILE_ATTRIBUTE_MANAGED) && (attrib & SCC_FILE_ATTRIBUTE_CHECKEDOUT)))
	{
		char * argv[] = { FullFileName};
		m_client.SetArgv( 1, argv );
		m_client.Run( "revert", &m_ui );
		if ( !m_ui.m_e.Test() )
			bRet=true;
	}

	return bRet;
}

bool CPerforceSourceControl::Rename( const char *filename, const char *newname, const char * desc, int nFlags)
{
	Reconnect();
	bool bRet = false;
	char FullFileName[MAX_PATH];
	uint32 attrib = GetFileAttributesAndFileName(filename, FullFileName);

	if(!(attrib & SCC_FILE_ATTRIBUTE_MANAGED))
		return true;

	//if(m_pIntegrator->FileCheckedOutByAnotherUser(FullFileName))
	//return false;

	char FullNewFileName[MAX_PATH];
	strcpy(FullNewFileName, newname);

	{
		char * argv[] = { FullFileName, FullNewFileName };
		m_client.SetArgv( 2, argv );
		m_client.Run( "integrate", &m_ui );
	}

	{
		char * argv[] = { FullFileName };
		m_client.SetArgv( 1, argv );
		m_client.Run( "delete", &m_ui );
	}
	if(desc)
		strcpy(m_ui.m_desc, desc);
	{
		char * argv[] = { FullFileName };
		m_client.SetArgv( 1, argv );
		m_client.Run( "submit", &m_ui );
	}

	if(desc)
		strcpy(m_ui.m_desc, desc);
	{
		char * argv[] = { FullNewFileName };
		m_client.SetArgv( 1, argv );
		m_client.Run( "submit", &m_ui );
	}

	if ( !m_ui.m_e.Test() )
		bRet=true;

	//p4 integrate source_file target_file
	//p4 delete source_file
	//p4 submit 


	return bRet;
}

bool CPerforceSourceControl::Delete( const char *filename, const char * desc, int nFlags)
{
	Reconnect();
	bool bRet = false;
	char FullFileName[MAX_PATH];
	uint32 attrib = GetFileAttributesAndFileName(filename, FullFileName);

	if(!(attrib & SCC_FILE_ATTRIBUTE_MANAGED))
		return true;

	//if(m_pIntegrator->FileCheckedOutByAnotherUser(FullFileName))
	//return false;

	if((attrib & SCC_FILE_ATTRIBUTE_MANAGED) && (attrib & SCC_FILE_ATTRIBUTE_CHECKEDOUT))
	{
		char * argv[] = { FullFileName};
		m_client.SetArgv( 1, argv );
		m_client.Run( "revert", &m_ui );
	}

	char * argv[] = { FullFileName};
	m_client.SetArgv( 1, argv );
	m_client.Run( "delete", &m_ui );

	if(desc)
		strcpy(m_ui.m_desc, desc);
	m_client.SetArgv( 1, argv );
	m_client.Run( "submit", &m_ui );
	if ( !m_ui.m_e.Test() )
		bRet = true;

	return bRet;
}


bool CPerforceSourceControl::SetPasswd()
{
	m_isSkipThread = true;
	bool bRet = true;

	CString strPassword(m_client.GetPassword().Value());
	CString strClient(m_client.GetClient().Value());
	CString strUser(m_client.GetUser().Value());
	CString strPort(m_client.GetPort().Value());

	if(PerforceConnection::OpenPasswordDlg(strPort, strUser, strClient, strPassword, m_bIsWorkOffline))
	{
		Error e;
		m_client.DefinePassword(strPassword, &e);
		m_client.DefineClient(strClient, &e);
		m_client.DefinePort(strPort, &e);
		m_client.DefineUser(strUser, &e);
	}

	return bRet;
}


bool CPerforceSourceControl::Run(const char * func, int nArgs, char * argv[], bool bOnlyFatal, bool bTestPasswd)
{
	Reconnect();
	bool bRet=false;

	m_ui.Init();
	m_client.SetArgv( nArgs, argv );
	m_client.Run( func, &m_ui );
	m_client.WaitTag();















	if(bOnlyFatal)
	{
		if(!m_ui.m_e.IsFatal())
			bRet=true;
	}
	else
	{
		if ( !m_ui.m_e.Test() )
			bRet=true;
	}

	if(m_ui.m_e.GetSeverity()==E_FAILED && bTestPasswd && (m_ui.m_e.GetGeneric()==0x0004 || m_ui.m_e.GetGeneric()==0x0024)) // password expired
	{
		g_pSystem->GetILog()->Log("\nPerforce connection: Generic Error: %s", m_ui.m_e.GetGeneric()==0x0004 ? "trying to do something you can't" : "client configuration inadequate");
		SetPasswd();
	}

	m_ui.m_e.Clear();
	return bRet;
}


bool CPerforceSourceControl::GetLatestVersion( const char *filename, int nFlags)
{
	bool bRet = false;
	char FullFileName[MAX_PATH];

	CString	str = filename;
	int curPos = 0;

	CString resToken = str.Tokenize(";",curPos);
	for (; !resToken.IsEmpty(); resToken = str.Tokenize(";",curPos))
	{
		resToken.Trim();
		if(resToken.IsEmpty())
			continue;

		//uint32 attrib = GetFileAttributesAndFileName(filename, FullFileName);
		uint32 attrib = GetFileAttributesAndFileName(resToken, FullFileName);

		bool bFolder=false;
		if(attrib == SCC_FILE_ATTRIBUTE_INVALID)
		{
			bFolder = IsFolder(filename, FullFileName);
			if(!bFolder)
				//return true;
				continue;
		}
		else
			if(!(attrib & SCC_FILE_ATTRIBUTE_MANAGED))
				continue;

		if(bFolder)
			strcat(FullFileName, "...");

		if(attrib & SCC_FILE_ATTRIBUTE_CHECKEDOUT && (nFlags & GETLATEST_NO_REVERT) == 0)
		{
			char * argv[] = { FullFileName};
			m_client.SetArgv( 1, argv );
			m_client.Run( "revert", &m_ui ); 
		}
		if (nFlags & GETLATEST_ONLY_CHECK)
		{
			if (bFolder)
				bRet = true;
			else
				bRet = IsFileLatestVersion(FullFileName);
		}
		else
		{
  		char * argv[] = { "-f", FullFileName};
  		m_client.SetArgv( 2, argv );
  		m_client.Run( "sync", &m_ui );
			bRet = true;
		}
	}

	return bRet;
}

bool CPerforceSourceControl::GetInternalPath( const char *filename, char* outPath, int nOutPathSize)
{
	if(!filename || !outPath)
		return false;

	uint32 attrib = GetFileAttributesAndFileName(filename, 0);

	if(attrib & SCC_FILE_ATTRIBUTE_MANAGED && *m_ui.m_depotFile)
	{
		strncpy(outPath, m_ui.m_depotFile, nOutPathSize-1);
		return true;
	}
	return false;
}
