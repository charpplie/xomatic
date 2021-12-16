////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetSoundDatabase.cpp
//  Version:	v1.00
//  Created:	12/07/2010 by Nicusor Nedelcu
//  Description:	Implements AssetSoundDatabase.h
// -------------------------------------------------------------------------  
//  History:
//
//////////////////////////////////////////////////////////////////////////// 

#include "stdafx.h"
#include "AssetSoundDatabase.h"
#include "AssetSoundItem.h"
#include "ISound.h"
#include "IRenderer.h"
#include "IMusicSystem.h"
#include "ImageExtensionHelper.h"
#include "Include/IAssetViewer.h"
#include "StringUtils.h"
#include "Util/IndexedFiles.h"

//TODO: sounds DB not yet available
REGISTER_CLASS_DESC(CAssetSoundDatabase);

CAssetSoundDatabase::CAssetSoundDatabase() : CAssetDisplayDatabase()
{
	m_soundListenerID = LISTENERID_INVALID;

	// add fields
	static const int kFilenameColWidth = 150;
	static const int kDccFilenameColWidth = 50;
	static const int kFileSizeColWidth = 50;
	static const int kRelativePathColWidth = 50;
	static const int kLengthColWidth = 50;
	static const int kUsedInLevelColWidth = 40;
	static const int kLoadedInLevelColWidth = 40;
	static const int kTagsColWidth = 60;

	m_assetFields.push_back( SAssetField( "filename", "Filename", SAssetField::eAssetFieldType_String, kFilenameColWidth ) );
	m_assetFields.push_back( SAssetField( "filesize", "Filesize (KB)", SAssetField::eAssetFieldType_Int32, kFileSizeColWidth ) );
	m_assetFields.push_back( SAssetField( "relativepath", "Path", SAssetField::eAssetFieldType_String, kRelativePathColWidth ) );
	m_assetFields.push_back( SAssetField( "length", "Length (Msec)", SAssetField::eAssetFieldType_Int32, kLengthColWidth ) );
	m_assetFields.push_back( SAssetField( "usedinlevel", "Used in level", SAssetField::eAssetFieldType_Bool, kUsedInLevelColWidth ) );
	m_assetFields.push_back( SAssetField( "loadedinlevel", "Loaded in level", SAssetField::eAssetFieldType_Bool, kLoadedInLevelColWidth ) );
	m_assetFields.push_back( SAssetField( "dccfilename", "DCC Filename", SAssetField::eAssetFieldType_String, kDccFilenameColWidth ) );
	m_assetFields.push_back( SAssetField( "tags", "Tags", SAssetField::eAssetFieldType_String, kTagsColWidth ) );
}

CAssetSoundDatabase::~CAssetSoundDatabase()
{
	// empty, call FreeData first
}

//////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE CAssetSoundDatabase::QueryInterface( const IID &riid, void **ppvObj ) 
{ 
	if(riid == __uuidof(IAssetDisplayDatabase)/* && m_pIntegrator*/)
	{
		*ppvObj = this;
		return S_OK;
	}
	return E_NOINTERFACE ; 
}

void CAssetSoundDatabase::FreeData()
{
	if( m_soundListenerID != LISTENERID_INVALID )
	{
		gEnv->pSoundSystem->RemoveListener( m_soundListenerID );
		m_soundListenerID = LISTENERID_INVALID;

		// UnPause all other sounds
		gEnv->pSoundSystem->Pause(false);
		// UnPause Music
		gEnv->pMusicSystem->Pause(false);
	}

	for( TFilenameAssetMap::iterator iter = m_assets.begin(), iterEnd = m_assets.end(); iter != iterEnd; ++iter )
	{
		iter->second->Release();
	}

	m_assets.clear();
}

const char* CAssetSoundDatabase::GetDatabaseTypeExt() const
{
	return "fdp,wav,ogg,mp3"; 
};

const char* CAssetSoundDatabase::GetTransactionFilename() const
{
	return "soundAssetTransactions.xml";
}

const char* CAssetSoundDatabase::GetDatabaseName() const
{
	return "Sounds";
}

void CAssetSoundDatabase::CollectCachedEventgroup( XmlNodeRef& gr, const CString& Block, const CString& Path, int level )
{
	const char* pNameGr = "";
	XmlNodeRef name = gr->findChild( "name" );
	
	if( name )
	{
		pNameGr = name->getContent();
	}

	CString NewPath = Path + pNameGr;
	
	for( size_t j = 0, jCount = gr->getChildCount(); j < jCount; ++j )
	{
		XmlNodeRef ev = gr->getChild( j );
	
		if( !strcmp( ev->getTag(), "event" ) )
		{
			const char* pNameEv = "";
			XmlNodeRef name = ev->findChild( "name" );

			if( name )
			{
				pNameEv = name->getContent();
			}

			CAssetSoundItem*			poSoundDatabaseItem = NULL;
			poSoundDatabaseItem = new CAssetSoundItem();

			if( !poSoundDatabaseItem )
				return;

			CString fpath = Block;
			fpath += ":";
			fpath += NewPath;
			fpath += ":";

			poSoundDatabaseItem->SetFileSize( 0 );
			poSoundDatabaseItem->SetFilename( pNameEv );
			poSoundDatabaseItem->SetRelativePath( fpath );
			poSoundDatabaseItem->SetOwnerDisplayDatabase( this );
			poSoundDatabaseItem->SetFileExtension( "fsb" );
			poSoundDatabaseItem->SetFlag( IAssetDisplay::eAssetFlags_Visible, true );
			fpath += pNameEv;
			m_assets[fpath] = poSoundDatabaseItem;
		}
		else
		if( !strcmp( ev->getTag(), "eventgroup" ) )
		{
			CollectCachedEventgroup( ev, Block, NewPath + "/", level + 1 );
		}
	}
}

void CAssetSoundDatabase::Refresh()
{
	if( !m_poAssociatedViewer )
	{
		return;
	}

	FreeData();

	ListenerID idClosest = gEnv->pSoundSystem->GetClosestActiveListener( Vec3() );
	m_soundListenerID = gEnv->pSoundSystem->CreateListener();
	IListener *pListener = gEnv->pSoundSystem->GetListener( m_soundListenerID );

	if( !pListener )
	{
		Log( "Cannot create sound listener object" );
	}
	else
	{
		pListener->SetRecordLevel( 1.0f );
		pListener->SetActive( true );
		gEnv->pSoundSystem->Update( eSoundUpdateMode_All );
	}

	const char						*szSoundName = NULL;
	CString								strExtension;
	CString								strFilename;
	CFileUtil::FileArray	cFiles;
	int										nTotalFiles = 0;
	int										nCurrentFile = 0;
	CString								strIntermediateFilename;
	CString								strOutputSoundName, strPathOnly, strFileNameOnly;
	CCryFile							file;
	CAssetSoundItem*			poSoundDatabaseItem = NULL;

	// search for sound files
	std::vector<CString> tags; tags.push_back("fdp");
	CIndexedFiles::GetDB().GetFilesWithTags(cFiles, tags);

	nTotalFiles = cFiles.size();

	for( nCurrentFile = 0; nCurrentFile < nTotalFiles; ++nCurrentFile )
	{
		CFileUtil::FileDesc& rstFileDescriptor = cFiles[nCurrentFile];

		// if not a real .fdp file
		if( Path::GetExt( rstFileDescriptor.filename ) != "fdp" )
		{
			continue;
		}

		strIntermediateFilename = rstFileDescriptor.filename.GetBuffer();
		strIntermediateFilename.MakeLower();
		strOutputSoundName = strIntermediateFilename;
		Path::ConvertBackSlashToSlash( strOutputSoundName );

		XmlNodeRef root = XmlHelpers::LoadXmlFromFile( strIntermediateFilename );

		char path[_MAX_PATH];
		strcpy(path, strIntermediateFilename );
		char * ch;

		while(ch = strchr(path,'\\'))
			*ch='/';

		if(ch = strrchr(path,'/'))
			*ch = 0;

		if( root )
		{
			const char * pName="";

			XmlNodeRef name = root->findChild( "name" );
			
			if( name )
				pName = name->getContent();

			for( size_t i = 0, iCount = root->getChildCount(); i < iCount; ++i )
			{
				XmlNodeRef gr= root->getChild( i );

				if( !strcmp( gr->getTag(), "eventgroup" ) )
				{
					CollectCachedEventgroup( gr, path, "", 1 );
				}
			}
		}
	}
}

void CAssetSoundDatabase::CacheFieldsInfoForAlreadyLoadedAssets()
{
	ISystem *pSystem = GetISystem();
	ISoundSystem *pSoundSystem = pSystem->GetISoundSystem();
	//TODO: check loaded sounds
}
