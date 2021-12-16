////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 2010.
// -------------------------------------------------------------------------
//  File name:   ResourceManager.h
//  Version:     v1.00
//  Created:     8/02/2010 by Timur.
//  Description: Interface to the Resource Manager
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ResourceManager.h"

#define GAME_DATA_PAK_FILENAME "GameData.pak"
#define LEVEL_CACHE_BIND_ROOT "LevelCache"
#define LEVEL_RESOURCE_LIST   "resourcelist.txt"

//////////////////////////////////////////////////////////////////////////
// IResourceList implementation class.
//////////////////////////////////////////////////////////////////////////
class CLevelResourceList : public IResourceList
{
public:
	CLevelResourceList()
	{
		m_pFileBuffer = 0;
		m_nBufferSize = 0;
		m_nCurrentLine = 0;
	};
	~CLevelResourceList()
	{
		Clear();
	};

	void UnifyFilename( const char *filename,char *buffer,int bufferSize )
	{
		strcpy_s( buffer,bufferSize,filename );
		char *p = buffer;
		int len = strlen(buffer);
		for (int i = 0; i < len; i++)
		{
			if (*p == '\\')
				*p = '/';
			*p = tolower(*p);
		}
	}

	virtual void Add( const char *sResourceFile )
	{
		assert(0); // Not implemented.
	}
	virtual void Clear()
	{
		delete m_pFileBuffer;
		m_pFileBuffer = 0;
		m_nBufferSize = 0;
		m_lines.clear();
		m_nCurrentLine = 0;
	}

	struct ComparePredicate { bool operator()( const char *s1,const char *s2) { return strcmp(s1,s2) < 0; }	};

	virtual bool IsExist( const char *sResourceFile )
	{
		char filename[260];
		UnifyFilename(sResourceFile,filename,sizeof(filename));
		Lines::const_iterator it = std::lower_bound(m_lines.begin(),m_lines.end(),filename,ComparePredicate());
		if (it != m_lines.end())
		{
			if (strcmp(*it,filename) == 0)
				return true;
		}
		return false;
	}
	virtual void Load( const char *sResourceListFilename )
	{
		Clear();
		m_lines.reserve(1000);
		CCryFile file;
		if (file.Open( sResourceListFilename,"rb" ))
		{
			m_nBufferSize = file.GetLength();
			m_pFileBuffer = new char[m_nBufferSize];
			file.ReadRaw( m_pFileBuffer,file.GetLength() );

			// Parse file, every line in a file represents a resource filename.
			char seps[] = "\r\n";
			char *token = strtok( m_pFileBuffer, seps );
			while (token != NULL)
			{
				m_lines.push_back(token);
				token = strtok( NULL, seps );
			}
		}
	}
	virtual const char* GetFirst()
	{
		m_nCurrentLine = 0;
		if (!m_lines.empty())
			return m_lines[0];
		return NULL;
	}
	virtual const char* GetNext()
	{
		m_nCurrentLine++;
		if (m_nCurrentLine < m_lines.size())
		{
			return m_lines[m_nCurrentLine];
		}
		return NULL;
	}

	void GetMemoryStatistics(ICrySizer *pSizer)
	{
		pSizer->Add( this,sizeof(*this) );
		pSizer->Add( m_pFileBuffer,m_nBufferSize );
		pSizer->AddContainer( m_lines );
	}

private:
	char *m_pFileBuffer;
	int m_nBufferSize;
	typedef std::vector<const char*> Lines;
	Lines m_lines;
	int m_nCurrentLine;
};


//////////////////////////////////////////////////////////////////////////
void CResourceManager::PrepareLevel( const char *sLevelFolder,const char *sLevelName )
{
	UnloadAllLevelCachePaks();

	m_currentLevelCacheFolder = CryPathString("_LevelCache/") + sLevelName;

	IResourceList *pResList = gEnv->pCryPak->GetRecorderdResourceList(ICryPak::RFOM_Level);

  // Load resourcelist.txt, TODO: make sure there are no duplicates
	string filename = PathUtil::Make(sLevelFolder,LEVEL_RESOURCE_LIST);
  pResList->Load( filename.c_str() );

	// Need to be checked again.

/*
	// Open GameData pak loading it completely into the memory.
	CryPathString pakPath = PathUtil::GetGameFolder().c_str();
	pakPath += "/";
	pakPath += GAME_DATA_PAK_FILENAME;
	gEnv->pCryPak->ClosePack( pakPath.c_str(),0 );
	if (gEnv->pCryPak->OpenPack( PathUtil::GetGameFolder(),pakPath.c_str(),ICryPak::FLAGS_PAK_IN_MEMORY ))
	{
		m_openedPaks.push_back( pakPath.c_str() );
	}
	*/
}

//////////////////////////////////////////////////////////////////////////
IResourceList* CResourceManager::GetLevelResourceList()
{
	IResourceList *pResList = gEnv->pCryPak->GetRecorderdResourceList(ICryPak::RFOM_Level);
	return pResList;
}

//////////////////////////////////////////////////////////////////////////
bool CResourceManager::LoadLevelCachePak( const char *sPakName )
{
	CryPathString pakPath = GetCurrentLevelCacheFolder() + "/" + sPakName;
	if (gEnv->pCryPak->OpenPack( PathUtil::GetGameFolder()+ "/" + LEVEL_CACHE_BIND_ROOT,pakPath.c_str(),ICryPak::FLAGS_PAK_IN_MEMORY|ICryPak::FLAGS_FILENAMES_AS_CRC32 ))
	{
		m_openedPaks.push_back( pakPath.c_str() );
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CResourceManager::UnloadLevelCachePak( const char *sPakName )
{
	CryPathString pakPath = GetCurrentLevelCacheFolder() + "/" + sPakName;
	gEnv->pCryPak->ClosePack( pakPath.c_str() );
	stl::find_and_erase( m_openedPaks,pakPath.c_str() );
}

//////////////////////////////////////////////////////////////////////////
void CResourceManager::UnloadAllLevelCachePaks()
{
	for (int i = 0; i < (int)m_openedPaks.size(); i++)
	{
		gEnv->pCryPak->ClosePack( m_openedPaks[i].c_str() );
	}
}

//////////////////////////////////////////////////////////////////////////
void CResourceManager::Update()
{
	
}

//////////////////////////////////////////////////////////////////////////
void CResourceManager::Init()
{
	GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CResourceManager::Shutdown()
{
	UnloadAllLevelCachePaks();
	GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CResourceManager::OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam )
{
	switch (event)
	{
	case ESYSTEM_EVENT_LEVEL_UNLOAD:
	case ESYSTEM_EVENT_LEVEL_PRECACHED:
	case ESYSTEM_EVENT_LEVEL_PRECACHE_START: //Diesel new 3203
		UnloadAllLevelCachePaks();
		break;
	}
}

