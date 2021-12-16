/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: AssetTextureDatabase.h,v 1.0 2009/04/15 11:00:00 PauloZaffari Exp wwwrun $
$DateTime$
Description: Header file for the class implementing IAssetDisplay
interface. It declares the headers of the actual used 
functions.
-------------------------------------------------------------------------
History:
- 15/04/2009		11:00 - Created by Paulo Zaffari
- 12/03/2010		17:16 - Nicusor Nedelcu - refactored
*************************************************************************/

#include "stdafx.h"
#include "AssetTextureDatabase.h"
#include "AssetTextureItem.h"
#include "ITexture.h"
#include "IRenderer.h"
#include "ImageExtensionHelper.h"
#include "Include/IAssetViewer.h"

REGISTER_CLASS_DESC(CAssetTextureDatabase);

CAssetTextureDatabase::CAssetTextureDatabase()
{
	m_poAssociatedViewer = NULL;
	m_ref = 1;

	// add fields
	static const int kFilenameColWidth = 150;
	static const int kFileSizeColWidth = 50;
	static const int kRelativePathColWidth = 150;
	static const int kWidthColWidth = 50;
	static const int kHeightColWidth = 50;
	static const int kMipsColWidth = 50;
	static const int kFormatColWidth = 100;
	static const int kTypeColWidth = 50;
	static const int kUsedInLevelColWidth = 40;

	m_assetFields.push_back( SAssetField( "filename", "Filename", SAssetField::eAssetFieldType_String, kFilenameColWidth ) );
	m_assetFields.push_back( SAssetField( "filesize", "Filesize", SAssetField::eAssetFieldType_Int32, kFileSizeColWidth ) );
	m_assetFields.push_back( SAssetField( "relativepath", "Path", SAssetField::eAssetFieldType_String, kRelativePathColWidth ) );
	m_assetFields.push_back( SAssetField( "width", "Width", SAssetField::eAssetFieldType_Int32, kWidthColWidth ) );
	m_assetFields.push_back( SAssetField( "height", "Height", SAssetField::eAssetFieldType_Int32, kHeightColWidth ) );
	m_assetFields.push_back( SAssetField( "mips", "MipCount", SAssetField::eAssetFieldType_Int32, kMipsColWidth ) );
	m_assetFields.push_back( SAssetField( "format", "Format", SAssetField::eAssetFieldType_String, kFormatColWidth ) );
	m_assetFields.push_back( SAssetField( "type", "Type", SAssetField::eAssetFieldType_String, kTypeColWidth ) );
	m_assetFields.push_back( SAssetField( "usedinlevel", "Used in level", SAssetField::eAssetFieldType_String, kUsedInLevelColWidth ) );

	{
		SAssetField* pField = GetAssetFieldByName( "width" );
		
		pField->m_bUseEnumValues = true;
		pField->m_enumValues.push_back( std::make_pair( "4", "4" ) );
		pField->m_enumValues.push_back( std::make_pair( "8", "8" ) );
		pField->m_enumValues.push_back( std::make_pair( "16", "16" ) );
		pField->m_enumValues.push_back( std::make_pair( "32", "32" ) );
		pField->m_enumValues.push_back( std::make_pair( "64", "64" ) );
		pField->m_enumValues.push_back( std::make_pair( "128", "128" ) );
		pField->m_enumValues.push_back( std::make_pair( "256", "256" ) );
		pField->m_enumValues.push_back( std::make_pair( "512", "512" ) );
		pField->m_enumValues.push_back( std::make_pair( "1024", "1024" ) );
		pField->m_enumValues.push_back( std::make_pair( "2048", "2048" ) );
	}

	{
		SAssetField* pField = GetAssetFieldByName( "height" );

		pField->m_bUseEnumValues = true;
		pField->m_enumValues.push_back( std::make_pair( "4", "4" ) );
		pField->m_enumValues.push_back( std::make_pair( "8", "8" ) );
		pField->m_enumValues.push_back( std::make_pair( "16", "16" ) );
		pField->m_enumValues.push_back( std::make_pair( "32", "32" ) );
		pField->m_enumValues.push_back( std::make_pair( "64", "64" ) );
		pField->m_enumValues.push_back( std::make_pair( "128", "128" ) );
		pField->m_enumValues.push_back( std::make_pair( "256", "256" ) );
		pField->m_enumValues.push_back( std::make_pair( "512", "512" ) );
		pField->m_enumValues.push_back( std::make_pair( "1024", "1024" ) );
		pField->m_enumValues.push_back( std::make_pair( "2048", "2048" ) );
	}

	{
		SAssetField* pField = GetAssetFieldByName( "mips" );

		pField->m_bUseEnumValues = true;
		pField->m_enumValues.push_back( std::make_pair( "1", "1" ) );
		pField->m_enumValues.push_back( std::make_pair( "2", "2" ) );
		pField->m_enumValues.push_back( std::make_pair( "3", "3" ) );
		pField->m_enumValues.push_back( std::make_pair( "4", "4" ) );
		pField->m_enumValues.push_back( std::make_pair( "5", "5" ) );
		pField->m_enumValues.push_back( std::make_pair( "6", "6" ) );
		pField->m_enumValues.push_back( std::make_pair( "7", "7" ) );
		pField->m_enumValues.push_back( std::make_pair( "8", "8" ) );
		pField->m_enumValues.push_back( std::make_pair( "9", "9" ) );
		pField->m_enumValues.push_back( std::make_pair( "10", "10" ) );
	}
}

CAssetTextureDatabase::~CAssetTextureDatabase()
{
}

//////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE CAssetTextureDatabase::QueryInterface( const IID &riid, void **ppvObj ) 
{ 
	if(riid == __uuidof(IAssetDisplayDatabase)/* && m_pIntegrator*/)
	{
		*ppvObj = this;
		return S_OK;
	}
	return E_NOINTERFACE ; 
}

ULONG STDMETHODCALLTYPE CAssetTextureDatabase::AddRef()
{
	return ++m_ref;
};

ULONG STDMETHODCALLTYPE CAssetTextureDatabase::Release() 
{ 
	if((--m_ref) == 0)
	{
		FreeData();
		delete this;
		return 0; 
	}
	else
		return m_ref;
}

void CAssetTextureDatabase::FreeData()
{
	if( !m_poAssociatedViewer )
		return;

	if( !m_assets.size() )
		return;

	for( TFilenameAssetMap::iterator iter = m_assets.begin(), iterEnd = m_assets.end(); iter != iterEnd; ++iter )
	{
		iter->second->Release();
	}

	m_assets.clear();
}

bool CAssetTextureDatabase::SetAssociatedViewer(IAssetViewer* poAssociatedViewer)
{
	m_poAssociatedViewer = poAssociatedViewer;

	return true;
}

IAssetViewer* CAssetTextureDatabase::GetAssociatedViewer()
{
	return m_poAssociatedViewer;
}

const char* CAssetTextureDatabase::GetDatabaseTypeExt()
{
	return "dds"; 
};

IAssetDisplayDatabase::TAssetFields& CAssetTextureDatabase::GetAssetFields()
{
	return m_assetFields;
}

SAssetField* CAssetTextureDatabase::GetAssetFieldByName( const char* pFieldName )
{
	for( size_t i = 0, iCount = m_assetFields.size(); i < iCount; ++i )
	{
		if( m_assetFields[i].m_fieldName == pFieldName )
		{
			return &m_assetFields[i];
		}
	}

	return NULL;
}

const char* CAssetTextureDatabase::GetDatabaseName()
{
	return "Textures";
}

void CAssetTextureDatabase::Refresh()
{
	if( !m_poAssociatedViewer )
	{
		return;
	}

	FreeData();

	const char						*szTextureName = NULL;
	CString								strExtension;
	CString								strFilename;
	CFileUtil::FileArray	cFiles;
	int										nTotalFiles = 0;
	int										nCurrentFile = 0;
	string								strIntermediateFilename;
	CString								strOutputTextureName, strPathOnly, strFileNameOnly;
	CCryFile							file;
	CAssetTextureItem*		poTextureDatabaseItem = NULL;

	// search for DDS files
	CFileUtil::ScanDirectory( PathUtil::GetGameFolder().c_str(), "*.dds", cFiles, true );

	// for every file, check if is a texture...
	nTotalFiles = cFiles.size();

	for( nCurrentFile = 0; nCurrentFile < nTotalFiles; ++nCurrentFile )
	{
		CFileUtil::FileDesc& rstFileDescriptor = cFiles[nCurrentFile];
		strIntermediateFilename = rstFileDescriptor.filename.GetBuffer();
		strIntermediateFilename.MakeLower();
		strOutputTextureName = strIntermediateFilename;
		Path::ConvertSlashToBackSlash( strOutputTextureName );

		// no need for temp files.
		if( strstr( rstFileDescriptor.filename, "~~~TempFile~~~" ) != NULL )
		{
			continue;
		}

		poTextureDatabaseItem = new CAssetTextureItem();

		if( !poTextureDatabaseItem )
			return;

		strFileNameOnly = Path::GetFile( strOutputTextureName );
		strPathOnly = Path::GetPath( strOutputTextureName );

		poTextureDatabaseItem->SetFileSize( rstFileDescriptor.size );
		poTextureDatabaseItem->SetFilename( strFileNameOnly.GetBuffer() );
		poTextureDatabaseItem->SetRelativePath( strPathOnly.GetBuffer() );
		poTextureDatabaseItem->SetOwnerDisplayDatabase( this );
		poTextureDatabaseItem->SetFileExtension( strExtension.GetBuffer() );
		poTextureDatabaseItem->SetFlag( IAssetDisplay::eAssetFlags_Visible, true );
		m_assets[strOutputTextureName.GetBuffer()] = poTextureDatabaseItem;
	}

	// clear up the file array
	cFiles.clear();
	CFileUtil::ScanDirectory( PathUtil::GetGameFolder().c_str(),"*.tif", cFiles, true );

	// for every file, check if is a texture...
	nTotalFiles = cFiles.size();

	for( nCurrentFile = 0; nCurrentFile < nTotalFiles; ++nCurrentFile )
	{
		CFileUtil::FileDesc& rstFileDescriptor = cFiles[nCurrentFile];
		strIntermediateFilename = rstFileDescriptor.filename.GetBuffer();
		strIntermediateFilename.MakeLower();
		strOutputTextureName = strIntermediateFilename;
		Path::ConvertSlashToBackSlash( strOutputTextureName );

		// no need for temp files.
		if( strstr( strOutputTextureName.GetBuffer(), "~~~TempFile~~~" ) != NULL )
		{
			continue;
		}

		strFilename = strOutputTextureName.GetBuffer();
		strExtension = Path::GetExt( strOutputTextureName.GetBuffer() );
		strFilename = Path::ReplaceExtension( strFilename, "dds" );

		// textures without names are not important for us.
		szTextureName = (char*)rstFileDescriptor.filename.GetBuffer();

		if( !szTextureName )
		{
			continue;
		}

		// textures which name starts with '$' are dynamic textures or in any way textures that
		// don't interest us.
		if( szTextureName[0] == '$' )
		{
			continue;
		}

		strFileNameOnly = Path::GetFile( strOutputTextureName );
		strPathOnly = Path::GetPath( strOutputTextureName );

		poTextureDatabaseItem = new CAssetTextureItem();

		if( !poTextureDatabaseItem )
			return;

		poTextureDatabaseItem->SetFileSize( rstFileDescriptor.size );
		poTextureDatabaseItem->SetFilename( strFileNameOnly.GetBuffer() );
		poTextureDatabaseItem->SetRelativePath( strPathOnly.GetBuffer() );
		poTextureDatabaseItem->SetOwnerDisplayDatabase( this );
		poTextureDatabaseItem->SetFileExtension( strExtension.GetBuffer() );
		poTextureDatabaseItem->SetFlag( IAssetDisplay::eAssetFlags_Visible, true );
		m_assets[strIntermediateFilename] = poTextureDatabaseItem;
	}
}

IAssetDisplayDatabase::TFilenameAssetMap&	CAssetTextureDatabase::GetAssets()
{
	return m_assets;
}

void CAssetTextureDatabase::ApplyFilters( const TAssetFieldFiltersMap& rFieldFilters )
{
	bool bAssetIsVisible = false;
	std::map<string,char*> cFieldFiltersValueRawData, cFieldFiltersMinValueRawData, cFieldFiltersMaxValueRawData;

	// bake, prepare the raw data values for the filter value and min/max range, so we do not have to atoi/atof parse text values each time
	// but have the raw int/float/double in memory, already parsed as raw data

	//TODO: parse filter values

	for( TFilenameAssetMap::iterator iterAsset = m_assets.begin(), iterAssetsEnd = m_assets.end(); iterAsset != iterAssetsEnd; ++iterAsset )
	{
		IAssetDisplay* pAsset = iterAsset->second;

		bAssetIsVisible = true;

		// loop through all field filters and abort if one of them does not comply
		for( TAssetFieldFiltersMap::const_iterator iter = rFieldFilters.begin(), iterEnd = rFieldFilters.end(); iter != iterEnd; ++iter )
		{
			// if asset is not visible, doesnt match a filter, then bother not to check other field filters 
			if( !bAssetIsVisible )
				break;

			const SAssetField& field = iter->second;

			// if this filter is disabled, skip it
			if( !field.m_bEnableFilter )
				continue;

			// if this filter is empty, skip it
			if( field.m_filterValue == "" )
				continue;

			SAssetField::EAssetFilterCondition filterCondition = field.m_filterCondition;

			// if this filter has a range condition, check to see if max value is set, if not then fallback to EQUAL condition, because we have no range anyway
			if( filterCondition == SAssetField::eAssertFilterCondition_InsideRange && field.m_maxFilterValue == "" )
			{
				filterCondition = SAssetField::eAssertFilterCondition_Equal;
			}

			switch( field.m_fieldType )
			{
				case SAssetField::eAssetFieldType_None:
				{
					continue;
				}

				case SAssetField::eAssetFieldType_Int8:
				{
					char filterValue, assetFieldValue;

					filterValue = atoi( field.m_filterValue.c_str() );

					if( !pAsset->GetAssetFieldValue( field.m_fieldName.c_str(), &assetFieldValue ) )
					{
						bAssetIsVisible = false;
						continue;
					}

					switch( filterCondition )
					{
						case SAssetField::eAssertFilterCondition_Equal: bAssetIsVisible = ( assetFieldValue == filterValue ); break;
						case SAssetField::eAssertFilterCondition_Greater: bAssetIsVisible = ( assetFieldValue > filterValue ); break;
						case SAssetField::eAssertFilterCondition_Less: bAssetIsVisible = ( assetFieldValue < filterValue ); break;
						case SAssetField::eAssertFilterCondition_GreaterOrEqual: bAssetIsVisible = ( assetFieldValue >= filterValue ); break;
						case SAssetField::eAssertFilterCondition_LessOrEqual: bAssetIsVisible = ( assetFieldValue <= filterValue ); break;
						case SAssetField::eAssertFilterCondition_Not: bAssetIsVisible = ( assetFieldValue != filterValue ); break;
						case SAssetField::eAssertFilterCondition_InsideRange:
						{
							char maxFilterValue = atoi( field.m_maxFilterValue.c_str() );
							bAssetIsVisible = ( assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue ); 
							break;
						}
					}

					break;
				}

				case SAssetField::eAssetFieldType_Int16:
				{
					short int filterValue, assetFieldValue;

					filterValue = atoi( field.m_filterValue.c_str() );

					if( !pAsset->GetAssetFieldValue( field.m_fieldName.c_str(), &assetFieldValue ) )
					{
						bAssetIsVisible = false;
						continue;
					}

					switch( filterCondition )
					{
						case SAssetField::eAssertFilterCondition_Equal: bAssetIsVisible = ( assetFieldValue == filterValue ); break;
						case SAssetField::eAssertFilterCondition_Greater: bAssetIsVisible = ( assetFieldValue > filterValue ); break;
						case SAssetField::eAssertFilterCondition_Less: bAssetIsVisible = ( assetFieldValue < filterValue ); break;
						case SAssetField::eAssertFilterCondition_GreaterOrEqual: bAssetIsVisible = ( assetFieldValue >= filterValue ); break;
						case SAssetField::eAssertFilterCondition_LessOrEqual: bAssetIsVisible = ( assetFieldValue <= filterValue ); break;
						case SAssetField::eAssertFilterCondition_Not: bAssetIsVisible = ( assetFieldValue != filterValue ); break;
						case SAssetField::eAssertFilterCondition_InsideRange:
						{
							short int maxFilterValue = atoi( field.m_maxFilterValue.c_str() );
							bAssetIsVisible = ( assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue ); 
							break;
						}
					}

					break;
				}

				case SAssetField::eAssetFieldType_Int32:
				{
					int filterValue, assetFieldValue;

					filterValue = atoi( field.m_filterValue.c_str() );

					if( !pAsset->GetAssetFieldValue( field.m_fieldName.c_str(), &assetFieldValue ) )
					{
						bAssetIsVisible = false;
						continue;
					}

					switch( filterCondition )
					{
						case SAssetField::eAssertFilterCondition_Equal: bAssetIsVisible = ( assetFieldValue == filterValue ); break;
						case SAssetField::eAssertFilterCondition_Greater: bAssetIsVisible = ( assetFieldValue > filterValue ); break;
						case SAssetField::eAssertFilterCondition_Less: bAssetIsVisible = ( assetFieldValue < filterValue ); break;
						case SAssetField::eAssertFilterCondition_GreaterOrEqual: bAssetIsVisible = ( assetFieldValue >= filterValue ); break;
						case SAssetField::eAssertFilterCondition_LessOrEqual: bAssetIsVisible = ( assetFieldValue <= filterValue ); break;
						case SAssetField::eAssertFilterCondition_Not: bAssetIsVisible = ( assetFieldValue != filterValue ); break;
						case SAssetField::eAssertFilterCondition_InsideRange:
						{
							int maxFilterValue = atoi( field.m_maxFilterValue.c_str() );
							bAssetIsVisible = ( assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue ); 
							break;
						}
					}

					break;
				}

				case SAssetField::eAssetFieldType_Int64:
				{
					__int64 filterValue, assetFieldValue;

					filterValue = _atoi64( field.m_filterValue.c_str() );

					if( !pAsset->GetAssetFieldValue( field.m_fieldName.c_str(), &assetFieldValue ) )
					{
						bAssetIsVisible = false;
						continue;
					}

					switch( filterCondition )
					{
						case SAssetField::eAssertFilterCondition_Equal: bAssetIsVisible = ( assetFieldValue == filterValue ); break;
						case SAssetField::eAssertFilterCondition_Greater: bAssetIsVisible = ( assetFieldValue > filterValue ); break;
						case SAssetField::eAssertFilterCondition_Less: bAssetIsVisible = ( assetFieldValue < filterValue ); break;
						case SAssetField::eAssertFilterCondition_GreaterOrEqual: bAssetIsVisible = ( assetFieldValue >= filterValue ); break;
						case SAssetField::eAssertFilterCondition_LessOrEqual: bAssetIsVisible = ( assetFieldValue <= filterValue ); break;
						case SAssetField::eAssertFilterCondition_Not: bAssetIsVisible = ( assetFieldValue != filterValue ); break;
						case SAssetField::eAssertFilterCondition_InsideRange:
						{
							__int64 maxFilterValue = _atoi64( field.m_maxFilterValue.c_str() );
							bAssetIsVisible = ( assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue ); 
							break;
						}
					}

					break;
				}

				case SAssetField::eAssetFieldType_Float:
				{
					float filterValue, assetFieldValue;

					filterValue = atof( field.m_filterValue.c_str() );

					if( !pAsset->GetAssetFieldValue( field.m_fieldName.c_str(), &assetFieldValue ) )
					{
						bAssetIsVisible = false;
						continue;
					}

					switch( filterCondition )
					{
						case SAssetField::eAssertFilterCondition_Equal: bAssetIsVisible = ( assetFieldValue == filterValue ); break;
						case SAssetField::eAssertFilterCondition_Greater: bAssetIsVisible = ( assetFieldValue > filterValue ); break;
						case SAssetField::eAssertFilterCondition_Less: bAssetIsVisible = ( assetFieldValue < filterValue ); break;
						case SAssetField::eAssertFilterCondition_GreaterOrEqual: bAssetIsVisible = ( assetFieldValue >= filterValue ); break;
						case SAssetField::eAssertFilterCondition_LessOrEqual: bAssetIsVisible = ( assetFieldValue <= filterValue ); break;
						case SAssetField::eAssertFilterCondition_Not: bAssetIsVisible = ( assetFieldValue != filterValue ); break;
						case SAssetField::eAssertFilterCondition_InsideRange:
						{
							float maxFilterValue = atof( field.m_maxFilterValue.c_str() );
							bAssetIsVisible = ( assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue ); 
							break;
						}
					}

					break;
				}

				case SAssetField::eAssetFieldType_Double:
				{
					double filterValue, assetFieldValue;

					filterValue = atof( field.m_filterValue.c_str() );

					if( !pAsset->GetAssetFieldValue( field.m_fieldName.c_str(), &assetFieldValue ) )
					{
						bAssetIsVisible = false;
						continue;
					}

					switch( filterCondition )
					{
						case SAssetField::eAssertFilterCondition_Equal: bAssetIsVisible = ( assetFieldValue == filterValue ); break;
						case SAssetField::eAssertFilterCondition_Greater: bAssetIsVisible = ( assetFieldValue > filterValue ); break;
						case SAssetField::eAssertFilterCondition_Less: bAssetIsVisible = ( assetFieldValue < filterValue ); break;
						case SAssetField::eAssertFilterCondition_GreaterOrEqual: bAssetIsVisible = ( assetFieldValue >= filterValue ); break;
						case SAssetField::eAssertFilterCondition_LessOrEqual: bAssetIsVisible = ( assetFieldValue <= filterValue ); break;
						case SAssetField::eAssertFilterCondition_Not: bAssetIsVisible = ( assetFieldValue != filterValue ); break;
						case SAssetField::eAssertFilterCondition_InsideRange:
						{
							double maxFilterValue = atof( field.m_maxFilterValue.c_str() );
							bAssetIsVisible = ( assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue ); 
							break;
						}
					}

					break;
				}

				case SAssetField::eAssetFieldType_String:
				{
					string assetFieldValue;
					const string& filterValue = field.m_filterValue;

					if( !pAsset->GetAssetFieldValue( field.m_fieldName.c_str(), &assetFieldValue ) )
					{
						bAssetIsVisible = false;
						continue;
					}

					switch( filterCondition )
					{
						case SAssetField::eAssertFilterCondition_Contains: bAssetIsVisible = ( NULL != strstr( assetFieldValue.c_str(), filterValue.c_str() ) ); break;
						case SAssetField::eAssertFilterCondition_Match: bAssetIsVisible = ( assetFieldValue == filterValue ); break;
						case SAssetField::eAssertFilterCondition_StartsWith: bAssetIsVisible = ( 0 == assetFieldValue.find( filterValue ) ); break;
						case SAssetField::eAssertFilterCondition_EndsWith: bAssetIsVisible = ( assetFieldValue.substr( assetFieldValue.size() - filterValue.size(), filterValue.size() ) == filterValue ); break;
						case SAssetField::eAssertFilterCondition_Equal: bAssetIsVisible = ( assetFieldValue == filterValue ); break;
						case SAssetField::eAssertFilterCondition_Greater: bAssetIsVisible = ( assetFieldValue > filterValue ); break;
						case SAssetField::eAssertFilterCondition_Less: bAssetIsVisible = ( assetFieldValue < filterValue ); break;
						case SAssetField::eAssertFilterCondition_GreaterOrEqual: bAssetIsVisible = ( assetFieldValue >= filterValue ); break;
						case SAssetField::eAssertFilterCondition_LessOrEqual: bAssetIsVisible = ( assetFieldValue <= filterValue ); break;
						case SAssetField::eAssertFilterCondition_Not: bAssetIsVisible = ( assetFieldValue != filterValue ); break;
						case SAssetField::eAssertFilterCondition_InsideRange:
						{
							bAssetIsVisible = ( assetFieldValue >= filterValue && assetFieldValue <= field.m_maxFilterValue ); 
							break;
						}
					}

					break;
				}
			}
		}

		pAsset->SetFlag( IAssetDisplay::eAssetFlags_Visible, bAssetIsVisible );
	}
}

void CAssetTextureDatabase::ClearFilters()
{
	for( TFilenameAssetMap::iterator iterAsset = m_assets.begin(), iterAssetsEnd = m_assets.end(); iterAsset != iterAssetsEnd; ++iterAsset )
	{
		IAssetDisplay* pAsset = iterAsset->second;
		
		pAsset->SetFlag( IAssetDisplay::eAssetFlags_Visible, true );
	}
}

void CAssetTextureDatabase::CacheFieldsInfoForAlreadyLoadedAssets()
{
	ISystem *pSystem = GetISystem();
	IRenderer *pRenderer = pSystem->GetIRenderer();

	int nTexCount = *(int*)pRenderer->EF_Query(EFQ_GetAllTextures,0);
	if (nTexCount > 0)
	{
		std::vector<ITexture*> pTextures;
		pTextures.resize(nTexCount);
		pRenderer->EF_Query( EFQ_GetAllTextures,(INT_PTR)&pTextures[0] );
		for (int i = 0; i < nTexCount; i++)
		{
			ITexture *pTexture = pTextures[i];
			int nTexSize = pTexture->GetDataSize();
			if (nTexSize > 0)
			{
				CString textureName = pTexture->GetName();
				Path::ConvertSlashToBackSlash(textureName);
				TFilenameAssetMap::iterator itr = m_assets.find(textureName.GetBuffer());
				if ( m_assets.end() != itr )
				{
					itr->second->SetUsedInLevel(true);
					static_cast<CAssetTextureItem*>(itr->second)->CacheFieldsInfoForLoadedTex(pTexture);
				}
			}
		}
	}
}
