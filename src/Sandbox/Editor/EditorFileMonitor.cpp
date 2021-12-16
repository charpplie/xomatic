// (c) 2001-2012 Crytek GmbH
#include "StdAfx.h"
#include "EditorFileMonitor.h"
#include "LiveCreate/EditorLiveCreate.h"
#include "ResourceCompilerHelper.h"
#include "GameEngine.h"
#include "Include/IAnimationCompressionManager.h"
#include <StringUtils.h>

//////////////////////////////////////////////////////////////////////////
CEditorFileMonitor::CEditorFileMonitor()
{
	GetIEditor()->RegisterNotifyListener(this);	
}

//////////////////////////////////////////////////////////////////////////
CEditorFileMonitor::~CEditorFileMonitor()
{
	CFileChangeMonitor::DeleteInstance();
}

//////////////////////////////////////////////////////////////////////////
void CEditorFileMonitor::OnEditorNotifyEvent(EEditorNotifyEvent ev)
{
	if (ev == eNotify_OnInit)
	{
		// Setup file change monitoring
		gEnv->pSystem->SetIFileChangeMonitor( this );

		// We don't want the file monitor to be enabled while
		// in console mode...
		if (!GetIEditor()->IsInConsolewMode())
			MonitorDirectories();

		CFileChangeMonitor::Instance()->Subscribe(this);
	}
	else if (ev == eNotify_OnQuit)
	{
		gEnv->pSystem->SetIFileChangeMonitor(NULL);
		CFileChangeMonitor::Instance()->StopMonitor();
		GetIEditor()->UnregisterNotifyListener(this);
	}
}

//////////////////////////////////////////////////////////////////////////
bool CEditorFileMonitor::RegisterListener(IFileChangeListener *pListener, const char* sMonitorItem)
{
	return RegisterListener(pListener, sMonitorItem, "*");
}


//////////////////////////////////////////////////////////////////////////
static string CanonicalizePath(const char* path)
{
	std::vector<char> canonicalizedPath(strlen(path) + 1, '\0');
	if (PathCanonicalize(&canonicalizedPath[0], path))
		return string(&canonicalizedPath[0]);

	return string(path);
}

//////////////////////////////////////////////////////////////////////////
bool CEditorFileMonitor::RegisterListener(IFileChangeListener *pListener, const char* sFolderRelativeToGame, const char* sExtension)
{
	bool failed = false;
	
	string gameFolder = Path::GetGameFolder();
	const char* modDirectory = gameFolder.c_str();
	int modIndex = 0;
	do
	{
		string naivePath = CString(GetIEditor()->GetMasterCDFolder()).GetString();
		if (!naivePath.empty() && naivePath[naivePath.size() - 1] != '\\')
			naivePath += "\\";
		naivePath += modDirectory;
		if (!naivePath.empty() && naivePath[naivePath.size() - 1] != '\\')
			naivePath += "\\";
		naivePath += sFolderRelativeToGame;
		naivePath.replace('/', '\\');

		string canonicalizedPath = CanonicalizePath(naivePath.c_str());

		if (Path::IsFolder(canonicalizedPath.c_str()))
		{
			if ( CFileChangeMonitor::Instance()->MonitorItem( canonicalizedPath.c_str() ) )
			{
				m_vecFileChangeCallbacks.push_back( SFileChangeCallback( pListener, sFolderRelativeToGame, sExtension) );
			}		 
			else
			{
				CryLogAlways( "File Monitor: [%s] not found outside of PAK files. Monitoring disabled for this item", sFolderRelativeToGame );
				failed = true;
			}	
		}
		modDirectory = gEnv->pCryPak->GetMod(modIndex); 
		++modIndex;
	}
	while (modDirectory != 0);

	return !failed;
}

bool CEditorFileMonitor::UnregisterListener(IFileChangeListener *pListener)
{
	bool bRet = false;

	// Note that we remove the listener, but we don't currently remove the monitored item
	// from the file monitor. This is fine, but inefficient

	std::vector<SFileChangeCallback>::iterator iter = m_vecFileChangeCallbacks.begin();
	while (iter != m_vecFileChangeCallbacks.end())
	{
		if (iter->pListener == pListener)
		{
			iter = m_vecFileChangeCallbacks.erase(iter);
			bRet = true;
		}
		else
			iter++;
	}

	return bRet;
}

//////////////////////////////////////////////////////////////////////////
void CEditorFileMonitor::MonitorDirectories()
{
	CString masterCD = Path::AddBackslash( CString(GetIEditor()->GetMasterCDFolder()) );

	// NOTE: Instead of monitoring each sub-directory we monitor the whole root
	// folder. This is needed since if the sub-directory does not exist when
	// we register it it will never get monitored properly.
	CFileChangeMonitor::Instance()->MonitorItem( masterCD / Path::GetGameFolder() / "" );

	// Add mod paths too
	for (int index = 0; ; index++)
	{
		const char* sModPath = gEnv->pCryPak->GetMod(index);
		if (!sModPath)
			break;
		CFileChangeMonitor::Instance()->MonitorItem( masterCD / sModPath / "" );
	}

	// Add editor directory for scripts
	CFileChangeMonitor::Instance()->MonitorItem( masterCD / "Editor" / "" );
}

//////////////////////////////////////////////////////////////////////////
static bool IsFilenameEndsWithDotDaeDotZip(const char *fln)
{
	size_t len = strlen(fln);
	if ( len < 8 )
		return false;

	if ( stricmp(fln + len - 8, ".dae.zip") == 0 )
		return true;
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////
static bool RecompileColladaFile(const char *path)
{
	string pathWithGameFolder = PathUtil::ToUnixPath(PathUtil::AddSlash(PathUtil::GetGameFolder())) + string(path);
	if (CResourceCompilerHelper::CallResourceCompiler(
		pathWithGameFolder.c_str(), "/refresh", NULL, false, CResourceCompilerHelper::eRcExePath_currentFolder, true, true, L".")
		!= CResourceCompilerHelper::eRcCallResult_success )
		return true;
	else 
		return false;
}


//////////////////////////////////////////////////////////////////////////
const char* GetPathRelativeToModFolder(const char* pathRelativeToGameFolder)
{
	if (pathRelativeToGameFolder[0] == '\0')
		return pathRelativeToGameFolder;

	string gameFolder = PathUtil::GetGameFolder();
	string modLocation;
	const char* modFolder = gameFolder.c_str();
	int modIndex = 0;
	do
	{
		if (_strnicmp(modFolder, pathRelativeToGameFolder, strlen(modFolder)) == 0)
		{
			const char* result = pathRelativeToGameFolder + strlen(modFolder);
			if (*result == '\\' ||  *result == '/')
				++result;
			return result;
		}

		modFolder = gEnv->pCryPak->GetMod(modIndex);
		++modIndex;
	}
	while (modFolder != 0);

	return "";
}

///////////////////////////////////////////////////////////////////////////

// Called when file monitor message is received
void CEditorFileMonitor::OnFileMonitorChange(const SFileChangeInfo& rChange)
{
	CCryEditApp *app = ((CCryEditApp*)AfxGetApp());
	if (app == NULL || app->IsExiting())
		return;

	// skip folders!
	if (Path::IsFolder(rChange.filename))
	{
		return;
	}

	// Process updated file.
	// Make file relative to MasterCD folder.
	CString filename = rChange.filename;

	// Make sure there is no leading slash
	if (!filename.IsEmpty() && filename[0] == '\\' || filename[0] == '/')
		filename = filename.Mid(1);


	if (!filename.IsEmpty())
	{
		// Make it relative to the game folder
		CString filenameRelGame = GetPathRelativeToModFolder(filename.GetString());

		CString ext = filename.Right(filename.GetLength() - filename.ReverseFind('.') - 1);

		// Check for File Monitor callback
		std::vector<SFileChangeCallback>::iterator iter;
		for ( iter=m_vecFileChangeCallbacks.begin(); iter!=m_vecFileChangeCallbacks.end(); ++iter )
		{
			SFileChangeCallback& sCallback = *iter;

			// We compare against length of callback string, so we get directory matches as well as full filenames
			if ( sCallback.pListener)
			{
				if (sCallback.extension == "*" || stricmp( ext, sCallback.extension ) == 0 )
				{
					if (_strnicmp( filenameRelGame, sCallback.item, sCallback.item.GetLength() ) == 0)
					{
							sCallback.pListener->OnFileChange( filenameRelGame, IFileChangeListener::EChangeType(rChange.changeType) );
					}
					else if (_strnicmp( filename, sCallback.item, sCallback.item.GetLength() ) == 0)
					{
							sCallback.pListener->OnFileChange( filename, IFileChangeListener::EChangeType(rChange.changeType) );
					}
				}
			}
		}

		//TODO:  have all these file types encapsulated in some IFileChangeFileTypeHandler to deal with each of them in a more generic way
		bool isCAF = stricmp(ext, "caf") == 0;
		bool isLMG = (stricmp(ext, "lmg") == 0) || (stricmp(ext, "bspace") == 0) || (stricmp(ext, "comb") == 0);

		bool isExportLog = stricmp(ext, "exportlog") == 0;
		bool isRCDone = stricmp(ext, "rcdone") == 0;
		bool isCOLLADA = (stricmp(ext, "dae") == 0 || IsFilenameEndsWithDotDaeDotZip(filename.GetString()));

		if (!isCAF && !isLMG && !isCOLLADA && !isExportLog && !isRCDone)
		{
			GetIEditor()->GetGameEngine()->ReloadResourceFile(filenameRelGame);
		}
		else if (isCOLLADA)
		{
			// Make a corresponding .cgf path.
			CString cgfFileName;
			int nameLength = filename.GetLength();

			cgfFileName=filename;
			if ( stricmp(ext, "dae") == 0 )
			{
				cgfFileName.SetAt(nameLength-3,'c');
				cgfFileName.SetAt(nameLength-2,'g');
				cgfFileName.SetAt(nameLength-1,'f');
			}
			else
			{
				cgfFileName.SetAt(nameLength-7,'c');
				cgfFileName.SetAt(nameLength-6,'g');
				cgfFileName.SetAt(nameLength-5,'f');
				cgfFileName.SetAt(nameLength-4,0);
			}
			IStatObj * pStatObjectToReload = GetIEditor()->Get3DEngine()->FindStatObjectByFilename(cgfFileName.GetBuffer());
					
			// If the corresponding .cgf file exists, recompile the changed COLLADA file.
			if (pStatObjectToReload)
			{
				CryLog("Recompile DAE file: %s", (LPCTSTR)filename);
				RecompileColladaFile(filename.GetString());
			}
		}
		else
		{
			ICharacterManager* pICharacterManager = GetISystem()->GetIAnimationSystem();
			stack_string strPath = filenameRelGame;
			CryStringUtils::UnifyFilePath(strPath);

			if (isLMG)
			{
				CryLog("Reload blendspace file: %s", (LPCTSTR)strPath);
				pICharacterManager->ReloadLMG(strPath.c_str());
			}

		}
		// Set this flag to make sure that the viewport will update at least once,
		// so that the changes will be shown, even if the app does not have focus.
		((CCryEditApp*)AfxGetApp())->ForceNextIdleProcessing();

#ifndef NO_LIVECREATE
		GetIEditor()->GetLiveCreate()->SyncFile(filename);
#endif
	}
}
