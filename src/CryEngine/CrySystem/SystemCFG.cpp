
//////////////////////////////////////////////////////////////////////
//
//	Crytek CryENGINE Source code
// 
//	File: SystemCFG.cpp
//  Description: handles system cfg
// 
//	History:
//	-Jan 21,2004: created
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h" 
#include "System.h"
#include <time.h>
#include "XConsole.h"
#include "CryFile.h"

#include <IScriptSystem.h>
#include "SystemCFG.h" 
#if defined(LINUX)
#include "ILog.h"
#if (EXE_VERSION_INFO_0 > 1)
#define EXE_VERSION_INFO_1 1
#define EXE_VERSION_INFO_2 1
#define EXE_VERSION_INFO_3 1
#endif // EXE_VERSION_INFO_0 > 1
#endif

#ifndef EXE_VERSION_INFO_0
#define EXE_VERSION_INFO_0 1
#endif 

#ifndef EXE_VERSION_INFO_1
#define EXE_VERSION_INFO_1 0
#endif 

#ifndef EXE_VERSION_INFO_2
#define EXE_VERSION_INFO_2 0
#endif 

#ifndef EXE_VERSION_INFO_3
#define EXE_VERSION_INFO_3 1
#endif 

//////////////////////////////////////////////////////////////////////////
const SFileVersion& CSystem::GetFileVersion()
{
	return m_fileVersion;
}

//////////////////////////////////////////////////////////////////////////
const SFileVersion& CSystem::GetProductVersion()
{
	return m_productVersion;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::QueryVersionInfo()   
{
#ifndef WIN32
		//do we need some other values here?
		m_fileVersion.v[0] = m_productVersion.v[0] = EXE_VERSION_INFO_3;
		m_fileVersion.v[1] = m_productVersion.v[1] = EXE_VERSION_INFO_2;
		m_fileVersion.v[2] = m_productVersion.v[2] = EXE_VERSION_INFO_1;
		m_fileVersion.v[3] = m_productVersion.v[3] = EXE_VERSION_INFO_0;
#else  //WIN32
	char moduleName[_MAX_PATH];
	DWORD dwHandle;
	UINT len;

	char ver[1024*8];

//	GetModuleFileName( NULL, moduleName, _MAX_PATH );//retrieves the PATH for the current module
	strcpy(moduleName,"CrySystem.dll");	// we want to version from the system dll

	int verSize = GetFileVersionInfoSize( moduleName,&dwHandle );
	if (verSize > 0)
	{
		GetFileVersionInfo( moduleName,dwHandle,1024*8,ver );
		VS_FIXEDFILEINFO *vinfo;
		VerQueryValue( ver,"\\",(void**)&vinfo,&len );




		const uint32 verIndices[4] = {0,1,2,3};

		m_fileVersion.v[verIndices[0]] = m_productVersion.v[verIndices[0]] = vinfo->dwFileVersionLS & 0xFFFF;
		m_fileVersion.v[verIndices[1]] = m_productVersion.v[verIndices[1]] = vinfo->dwFileVersionLS >> 16;
		m_fileVersion.v[verIndices[2]] = m_productVersion.v[verIndices[2]] = vinfo->dwFileVersionMS & 0xFFFF;
		m_fileVersion.v[verIndices[3]] = m_productVersion.v[verIndices[3]] = vinfo->dwFileVersionMS >> 16;
	}
#endif //WIN32
}

//////////////////////////////////////////////////////////////////////////
void CSystem::LogVersion()
{
	// Get time.
	time_t ltime;
	time( &ltime );
	tm *today = localtime( &ltime );

	char s[1024];


	strftime( s,128,"%d %b %y (%H %M %S)",today);		

	const SFileVersion &ver = GetFileVersion();

	CryLogAlways("BackupNameAttachment=\" Build(%d) %s\"  -- used by backup system\n",ver.v[0],s);			// read by CreateBackupFile()

	// Use strftime to build a customized time string.
	strftime( s,128,"Log Started at %c", today );
	CryLogAlways( s );

#if defined(WIN64) || defined(LINUX64)
	CryLogAlways( "Running 64 bit version" );




#else
	CryLogAlways( "Running 32 bit version" );
#endif
#if !defined(PS3) && !defined(XENON) && !defined(LINUX)
	GetModuleFileName(NULL,s,sizeof(s));
	CryLogAlways( "Executable: %s",s );
#endif

	CryLogAlways( "FileVersion: %d.%d.%d.%d",m_fileVersion.v[3],m_fileVersion.v[2],m_fileVersion.v[1],m_fileVersion.v[0] );
	CryLogAlways( "ProductVersion: %d.%d.%d.%d",m_productVersion.v[3],m_productVersion.v[2],m_productVersion.v[1],m_productVersion.v[0] );

#ifdef STLPORT
	CryLogAlways( "Using STLport C++ Standard Library implementation\n" );
#else //STLPORT
	CryLogAlways( "Using Microsoft (tm) C++ Standard Library implementation\n" );
#endif //STLPORT
}

//////////////////////////////////////////////////////////////////////////
void CSystem::LogBuildInfo()
{
	ICVar *pGameName=m_env.pConsole->GetCVar("cvGameName");
	if(pGameName)
	{
		CryLogAlways( "GameName: %s", pGameName->GetString() );
	}
	else
	{
		CryLogAlways( "Couldn't find game name in cvar cvGameName!" );
	}
	CryLogAlways( "BuildTime: " __DATE__ " " __TIME__ );
}



//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
class CCVarSaveDump : public ICVarDumpSink
{
public:

	CCVarSaveDump(FILE *pFile)
	{
		m_pFile=pFile;
	}

	virtual void OnElementFound(ICVar *pCVar)
	{
		if (!pCVar)
			return;
		int nFlags = pCVar->GetFlags();
		if (((nFlags & VF_DUMPTODISK) && (nFlags & VF_MODIFIED)) || (nFlags & VF_WASINCONFIG))
		{
			string szValue = pCVar->GetString();
			int pos;

			pos = 1;
			for(;;)
			{
				pos = szValue.find_first_of("\\", pos);

				if (pos == string::npos)
				{
					break;
				}

				szValue.replace(pos, 1, "\\\\", 2);
				pos+=2;
			}

			// replace " with \" 
			pos = 1;
			for(;;)
			{
				pos = szValue.find_first_of("\"", pos);

				if (pos == string::npos)
				{
					break;
				}

				szValue.replace(pos, 1, "\\\"", 2);
				pos+=2;
			}

			string szLine = pCVar->GetName();

			if(pCVar->GetType()==CVAR_STRING)
				szLine += " = \"" + szValue + "\"\r\n";
			 else
				szLine += " = " + szValue + "\r\n";

			if(pCVar->GetFlags()&VF_WARNING_NOTUSED)
				fputs("-- REMARK: the following was not assigned to a console variable\r\n",m_pFile);

			fputs(szLine.c_str(), m_pFile);
		}
	}

private: // --------------------------------------------------------

	FILE *				m_pFile;					//
};

//////////////////////////////////////////////////////////////////////////
void CSystem::SaveConfiguration()
{
/*	// save config before we quit
	if (!m_env.pGame)
		return;

	// get player's profile
	ICVar *pProfile=m_env.pConsole->GetCVar("g_playerprofile");
	if (pProfile)
	{	
		const char *sProfileName=pProfile->GetString();
		//m_env.pGame->SaveConfiguration( "system.cfg","game.cfg",sProfileName);
	}
	// always save the current profile in the root, otherwise, nexttime, the game will have the default one
	// wich is annoying
	//m_env.pGame->SaveConfiguration( "system.cfg","game.cfg",NULL);

	m_rDriver->Set(sSave.c_str());
*/

	if(m_rDriver && m_sSavedRDriver!="")
		m_rDriver->Set(m_sSavedRDriver.c_str());

	// the following code is simple replacement for the old code
	// without profiles and without saving of the actionmap
	// todo: improve and remove this comment

	if(m_sys_SaveCVars->GetIVal())
	{
		FILE *pFile=fxopen("system.cfg","wb");
		if(pFile)
		{
			fputs("-- [System-Configuration]\r\n",pFile);
			fputs("-- Attention: This file is generated by the system! Editing is not recommended! \r\n\r\n",pFile);

			CCVarSaveDump SaveDump(pFile);
			
			GetIConsole()->DumpCVars(&SaveDump);
			
			fclose(pFile); 

			GetILog()->Log("SaveConfiguration() succeeded");
		}
		else GetILog()->LogError("SaveConfiguration() failed");
	}

}

//////////////////////////////////////////////////////////////////////////
// system cfg
//////////////////////////////////////////////////////////////////////////
CSystemConfiguration::CSystemConfiguration(const string& strSysConfigFilePath,CSystem *pSystem, ILoadConfigurationEntrySink *pSink )
: m_strSysConfigFilePath( strSysConfigFilePath ), m_bError(false), m_pSink(pSink)
{
	assert(pSink);

	m_pSystem = pSystem;
	m_bError = !ParseSystemConfig();
}

//////////////////////////////////////////////////////////////////////////
CSystemConfiguration::~CSystemConfiguration()
{
}

//////////////////////////////////////////////////////////////////////////
bool CSystemConfiguration::ParseSystemConfig()
{
	string filename = m_strSysConfigFilePath;
	if (strlen(PathUtil::GetExt(filename)) == 0)
	{
		filename = PathUtil::ReplaceExtension(filename,"cfg");
	}

	CCryFile file;

	{
		const char *szLog = "Loading cvar config (try root,game,config):";
		string filenameLog;

		if (file.Open( string("./")+filename, "rb",ICryPak::FOPEN_HINT_QUIET ))
			filenameLog = string("./")+filename;
		else if (file.Open( filename, "rb",ICryPak::FOPEN_HINT_QUIET ))
			filenameLog = string("Game/")+filename;
		else if (file.Open( string("config/")+filename, "rb",ICryPak::FOPEN_HINT_QUIET ))
			filenameLog = string("Game/config/")+filename;
		else
		{
			CryLog("%s \"%s\" not found!", szLog, filename.c_str());
			return false;
		}

		CryLog("%s \"%s\" found in %s", szLog, PathUtil::GetFile(filenameLog.c_str()), PathUtil::GetPath(filenameLog.c_str()).c_str() );
	}

	int nLen = file.GetLength();
	char *sAllText = new char [nLen + 16];
	file.ReadRaw( sAllText,nLen );
	sAllText[nLen] = '\0';
	sAllText[nLen+1] = '\0';
	
	string strGroup;			// current group e.g. "[General]"

	char *strLast = sAllText+nLen;
	char *str = sAllText;
	while (str < strLast)
	{
		char *s = str;
		while (str < strLast && *str != '\n' && *str != '\r')
			str++;
		*str = '\0';
		str++;
		while (str < strLast && (*str == '\n' || *str == '\r'))
			str++;


		string strLine = s;

		// detect groups e.g. "[General]"   should set strGroup="General"
		{
			string strTrimmedLine( RemoveWhiteSpaces(strLine) );
			size_t size = strTrimmedLine.size();

			if(size>=3)
			if(strTrimmedLine[0]=='[' && strTrimmedLine[size-1]==']')		// currently no comments are allowed to be behind groups
			{
				strGroup = &strTrimmedLine[1];strGroup.resize(size-2);		// remove [ and ]
				continue;																									// next line
			}
		}

		//trim all whitespace characters at the beginning and the end of the current line and store its size
		strLine.Trim();
		size_t strLineSize = strLine.size();

		//skip comments, comments start with ";" or "--" but may have preceding whitespace characters
		if (strLineSize > 0)
		{
			if (strLine[0] == ';')
				continue;
			else if (strLine.find("--") == 0)
				continue;
		}
		//skip empty lines
		else
			continue;
		

		//if line contains a '=' try to read and assign console variable
		string::size_type posEq( strLine.find( "=", 0 ) );
		if (string::npos!=posEq)
		{
			string stemp( strLine, 0, posEq );
			string strKey( RemoveWhiteSpaces(stemp) );

//				if (!strKey.empty())
			{
				// extract value
				string::size_type posValueStart( strLine.find( "\"", posEq + 1 ) + 1 );
				// string::size_type posValueEnd( strLine.find( "\"", posValueStart ) );
				string::size_type posValueEnd( strLine.rfind( '\"' ) );

				string strValue;

				if( string::npos != posValueStart && string::npos != posValueEnd )
					strValue=string( strLine, posValueStart, posValueEnd - posValueStart );
				else
				{
					string strTmp( strLine, posEq + 1, strLine.size()-(posEq + 1) );
					strValue = RemoveWhiteSpaces(strTmp);
				}

				{
					// replace '\\\\' with '\\' and '\\\"' with '\"'
					strValue.replace( "\\\\","\\" );
					strValue.replace( "\\\"","\"" );

//						m_pSystem->GetILog()->Log("Setting %s to %s",strKey.c_str(),strValue.c_str());
					m_pSink->OnLoadConfigurationEntry(strKey,strValue,strGroup);
				}
			}					
		}
		else
		{
			gEnv->pLog->LogWithType(ILog::eWarning,"%s -> invalid configuration line: %s", filename.c_str(), strLine.c_str());
		}

	}

	delete []sAllText;

	m_pSink->OnLoadConfigurationEntry_End();

	return true;
}


//////////////////////////////////////////////////////////////////////////
void CSystem::OnLoadConfigurationEntry( const char *szKey, const char *szValue, const char *szGroup )
{
	if (!gEnv->pConsole)
		return;

	if(*szKey!=0)
			gEnv->pConsole->LoadConfigVar( szKey,szValue );
}

//////////////////////////////////////////////////////////////////////////
void CSystem::LoadConfiguration( const char *sFilename, ILoadConfigurationEntrySink *pSink )
{
	if (sFilename && strlen(sFilename) > 0)
	{	
		if(!pSink)
			pSink = this;

		CSystemConfiguration tempConfig(sFilename,this,pSink);
	}
}

