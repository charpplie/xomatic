#include "StdAfx.h"
#include "LensFlareManager.h"
#include "LensFlareEditor.h"
#include "LensFlareItem.h"
#include "LensFlareLibrary.h"
#include "LensFlareUtil.h"

//////////////////////////////////////////////////////////////////////////
// CLensFlareManager implementation.
//////////////////////////////////////////////////////////////////////////
CLensFlareManager::CLensFlareManager() :
CBaseLibraryManager()
{
	m_bUniqNameMap = true;
	m_pLevelLibrary = (CBaseLibrary*)AddLibrary( "Level" );
	m_pLevelLibrary->SetLevelLibrary( true );
}

//////////////////////////////////////////////////////////////////////////
CLensFlareManager::~CLensFlareManager()
{
}

//////////////////////////////////////////////////////////////////////////
void CLensFlareManager::ClearAll()
{
	CBaseLibraryManager::ClearAll();

	m_pLevelLibrary = (CBaseLibrary*)AddLibrary( "Level" );
	m_pLevelLibrary->SetLevelLibrary( true );
}

//////////////////////////////////////////////////////////////////////////
CBaseLibraryItem* CLensFlareManager::MakeNewItem()
{
	return new CLensFlareItem;
}

//////////////////////////////////////////////////////////////////////////
CBaseLibrary* CLensFlareManager::MakeNewLibrary()
{
	return new CLensFlareLibrary(this);
}

//////////////////////////////////////////////////////////////////////////
CString CLensFlareManager::GetRootNodeName()
{
	return "FlareLibs";
}

//////////////////////////////////////////////////////////////////////////
CString CLensFlareManager::GetLibsPath()
{
	if (m_libsPath.IsEmpty())
		m_libsPath += FLARE_LIBS_PATH;

	return m_libsPath;
}

//////////////////////////////////////////////////////////////////////////
bool CLensFlareManager::LoadFlareItemByName( const CString &fullItemName, IOpticsElementBasePtr pDestOptics )
{	
	if( pDestOptics == NULL )
		return false;

	CLensFlareItem* pLensFlareItem = (CLensFlareItem*)LoadItemByName(fullItemName);
	if( pLensFlareItem == NULL )
		return false;

	LensFlareUtil::CopyOptics( pLensFlareItem->GetOptics(), pDestOptics, true );
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CLensFlareManager::Modified()
{
	CLensFlareEditor* pEditor = CLensFlareEditor::GetLensFlareEditor();
	if( pEditor == NULL )
		return;
	if( pEditor->GetCurrentLibrary() )
		pEditor->GetCurrentLibrary()->SetModified(true);
}

//////////////////////////////////////////////////////////////////////////
IDataBaseLibrary* CLensFlareManager::LoadLibrary( const CString &filename, bool bReload )
{	
	CLensFlareEditor* pEditor = CLensFlareEditor::GetLensFlareEditor();

	CString fileNameWithGameFolder(filename);

	fileNameWithGameFolder.Replace('\\','/');

	int nGamePathLength = Path::GetGameFolder().GetLength();
	CString gamePathName;
	if( nGamePathLength < filename.GetLength() )
		gamePathName = filename.Left(nGamePathLength);
	if( gamePathName != Path::GetGameFolder() )
	{
		fileNameWithGameFolder.Insert(0,"/");
		fileNameWithGameFolder.Insert(0,Path::GetGameFolder());		
	}

	int nLibraryIndex(-1);
	bool bSameAsCurrentLibrary(false);	

	for (int i = 0; i < m_libs.size(); i++)
	{
		if( stricmp(fileNameWithGameFolder,m_libs[i]->GetFilename()) == 0 )
		{
			IDataBaseLibrary* pExistingLib = m_libs[i];
			for (int j = 0; j < pExistingLib->GetItemCount(); j++)
				UnregisterItem( (CBaseLibraryItem*)pExistingLib->GetItem(j) );
			pExistingLib->RemoveAllItems();
			nLibraryIndex = i;
			if( pEditor )
				bSameAsCurrentLibrary = pEditor->GetCurrentLibrary() == pExistingLib;
			break;
		}
	}	

	TSmartPtr<CBaseLibrary> pLib = MakeNewLibrary();
	if (!pLib->Load( filename ))
	{
		Error( _T("Failed to Load Item Library: %s"),filename );
		return NULL;
	}

	if( nLibraryIndex != -1 )
	{
		m_libs[nLibraryIndex] = pLib;		
		if( bSameAsCurrentLibrary && pEditor )
		{
			pEditor->ResetElementTreeControl();
			pEditor->SelectLibrary(pLib ,true);
		}		
	}
	else
	{
		m_libs.push_back( pLib );
	}

	pLib->SetFilename(fileNameWithGameFolder);

	return pLib;
}
