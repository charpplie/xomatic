/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: AssetModelDatabase.h,v 1.0 2009/04/15 11:00:00 PauloZaffari Exp wwwrun $
$DateTime$
Description: Source file for the class implementing IAssetDisplay
interface. It declares the headers of the actual used 
functions.
-------------------------------------------------------------------------
History:
- 15:04:2009   11:00 : Created by Paulo Zaffari

*************************************************************************/
#include "stdafx.h"

#include "AssetModelDatabase.h"
#include "AssetModelItem.h"
#include "Include/IAssetViewer.h"

REGISTER_CLASS_DESC(CAssetModelDatabase);

CAssetModelDatabase::CAssetModelDatabase()
{
	m_poAssociatedViewer = NULL;
	m_ref = 1;

	// add fields
	static const int kFilenameColWidth = 150;
	static const int kFileSizeColWidth = 50;
	static const int kRelativePathColWidth = 150;
	static const int kRefCountColWidth = 30;
	static const int kTextureSizeColWidth = 50;
	static const int kTriangleCountColWidth = 50;
	static const int kVertexCountColWidth = 50;
	static const int kLodCountColWidth = 50;
	static const int kSubmeshCountColWidth = 50;
	static const int kPhysicsTriCountColWidth = 50;
	static const int kPhysicsSizeColWidth = 40;
	static const int kLODsTrisColWidth = 100;
	static const int kSplitLODsColWidth = 40;
	static const int kBBoxColWidth = 50;
	static const int kUsedInLevelColWidth = 40;

	m_assetFields.push_back( SAssetField( "filename", "Filename", SAssetField::eAssetFieldType_String, kFilenameColWidth ) );
	m_assetFields.push_back( SAssetField( "filesize", "Filesize", SAssetField::eAssetFieldType_Int32, kFileSizeColWidth ) );
	m_assetFields.push_back( SAssetField( "relativepath", "Path", SAssetField::eAssetFieldType_String, kRelativePathColWidth ) );
	m_assetFields.push_back( SAssetField( "nref", "Refs", SAssetField::eAssetFieldType_Int32, kRefCountColWidth ) );
	m_assetFields.push_back( SAssetField( "texturesize", "Texture size (KB)", SAssetField::eAssetFieldType_Int32, kTextureSizeColWidth ) );
	m_assetFields.push_back( SAssetField( "trianglecount", "Triangles", SAssetField::eAssetFieldType_Int32, kTriangleCountColWidth ) );
	m_assetFields.push_back( SAssetField( "vertexcount", "Vertices", SAssetField::eAssetFieldType_Int32, kVertexCountColWidth ) );
	m_assetFields.push_back( SAssetField( "lodcount", "LODs", SAssetField::eAssetFieldType_Int32, kLodCountColWidth ) );
	m_assetFields.push_back( SAssetField( "submeshcount", "Submeshes", SAssetField::eAssetFieldType_Int32, kSubmeshCountColWidth ) );
	m_assetFields.push_back( SAssetField( "phytricount", "Physics tris", SAssetField::eAssetFieldType_Int32, kPhysicsTriCountColWidth ) );
	m_assetFields.push_back( SAssetField( "physize", "Physics size (KB)", SAssetField::eAssetFieldType_Int32, kPhysicsSizeColWidth ) );
	m_assetFields.push_back( SAssetField( "lodstricount", "LODs tris", SAssetField::eAssetFieldType_String, kLODsTrisColWidth ) );
	m_assetFields.push_back( SAssetField( "splitlods", "Split LODs", SAssetField::eAssetFieldType_String, kSplitLODsColWidth ) );
	m_assetFields.push_back( SAssetField( "bbox_x", "Size (X)", SAssetField::eAssetFieldType_Float, kBBoxColWidth ) );
	m_assetFields.push_back( SAssetField( "bbox_y", "Size (Y)", SAssetField::eAssetFieldType_Float, kBBoxColWidth ) );
	m_assetFields.push_back( SAssetField( "bbox_z", "Size (Z)", SAssetField::eAssetFieldType_Float, kBBoxColWidth ) );
	m_assetFields.push_back( SAssetField( "usedinlevel", "Used in level", SAssetField::eAssetFieldType_String, kUsedInLevelColWidth ) );
}

CAssetModelDatabase::~CAssetModelDatabase()
{
}

HRESULT STDMETHODCALLTYPE CAssetModelDatabase::QueryInterface( const IID &riid, void **ppvObj ) 
{ 
	if(riid == __uuidof(IAssetDisplayDatabase)/* && m_pIntegrator*/)
	{
		*ppvObj = this;
		return S_OK;
	}
	return E_NOINTERFACE ; 
}

ULONG STDMETHODCALLTYPE CAssetModelDatabase::AddRef()
{
	return ++m_ref;
};

ULONG STDMETHODCALLTYPE CAssetModelDatabase::Release() 
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

void CAssetModelDatabase::FreeData()
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

const char* CAssetModelDatabase::GetDatabaseTypeExt()
{
	return "cgf"; 
};

IAssetDisplayDatabase::TAssetFields& CAssetModelDatabase::GetAssetFields()
{
	return m_assetFields;
}

SAssetField* CAssetModelDatabase::GetAssetFieldByName( const char* pFieldName )
{
	for( size_t i = 0, iCount = m_assetFields.size(); i < iCount; ++i )
	{
		if( m_assetFields[i].m_fieldName == pFieldName )
			return &m_assetFields[i];
	}

	return NULL;
}

bool CAssetModelDatabase::SetAssociatedViewer( IAssetViewer* poAssociatedViewer )
{
	// Hot changing of associated viewer is not allowed, for now.
	if( m_poAssociatedViewer )
	{
		return false;
	}

	m_poAssociatedViewer = poAssociatedViewer;

	return true;
}

IAssetViewer* CAssetModelDatabase::GetAssociatedViewer()
{
	return m_poAssociatedViewer;
}

const char* CAssetModelDatabase::GetDatabaseName()
{
	return "Models";
}

void CAssetModelDatabase::Refresh()
{
	if( !m_poAssociatedViewer )
	{
		return;
	}

	FreeData();

	CFileUtil::FileArray	cFiles;
	int										nTotalFiles = 0;
	int										nCurrentFile = 0;
	string								strPathOnly, strFileNameOnly;

	// search for models
	CFileUtil::ScanDirectory( PathUtil::GetGameFolder().c_str(), "*.cgf", cFiles, true );

	nTotalFiles = cFiles.size();

	for( nCurrentFile = 0; nCurrentFile < nTotalFiles; ++nCurrentFile )
	{
		CFileUtil::FileDesc& rstFileDescriptor = cFiles[nCurrentFile];
		CString strIntermediateFilename = rstFileDescriptor.filename.GetBuffer();
		strIntermediateFilename.MakeLower();
		CString strOutputModelName = strIntermediateFilename;
		Path::ConvertSlashToBackSlash( strOutputModelName );

		// No need to load files already loaded by the engine...
		if( m_assets.find( strOutputModelName.GetBuffer() ) != m_assets.end() )
		{
			continue;
		}

		CString strExtension = Path::GetExt( strOutputModelName.GetBuffer() );
		const char* szModelName = strOutputModelName.GetBuffer();

		if( !szModelName )
		{
			continue;
		}

		strFileNameOnly = Path::GetFile( strOutputModelName );
		strPathOnly = Path::GetPath( strOutputModelName );

		CAssetModelItem* poModelDatabaseItem = new CAssetModelItem();

		poModelDatabaseItem->SetFileSize( rstFileDescriptor.size );
		poModelDatabaseItem->SetFilename( strFileNameOnly.c_str() );
		poModelDatabaseItem->SetRelativePath( strPathOnly.c_str() );
		poModelDatabaseItem->SetOwnerDisplayDatabase( this );
		poModelDatabaseItem->SetFileExtension( strExtension.GetBuffer() );
		poModelDatabaseItem->SetFlag( IAssetDisplay::eAssetFlags_Visible, true );
		m_assets[strOutputModelName.GetBuffer()] = poModelDatabaseItem;
	}
}

IAssetDisplayDatabase::TFilenameAssetMap&	CAssetModelDatabase::GetAssets()
{
	return m_assets;
}

void CAssetModelDatabase::ApplyFilters( const TAssetFieldFiltersMap& rFieldFilters )
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

void CAssetModelDatabase::ClearFilters()
{
	for( TFilenameAssetMap::iterator iterAsset = m_assets.begin(), iterAssetsEnd = m_assets.end(); iterAsset != iterAssetsEnd; ++iterAsset )
	{
		IAssetDisplay* pAsset = iterAsset->second;

		pAsset->SetFlag( IAssetDisplay::eAssetFlags_Visible, true );
	}
}

void CAssetModelDatabase::CacheFieldsInfoForAlreadyLoadedAssets()
{
	ISystem *pSystem = GetISystem();
	I3DEngine *p3DEngine = pSystem->GetI3DEngine();

	// iterate through all IStatObj
	{
		int nObjCount = 0;

		p3DEngine->GetLoadedStatObjArray(0,nObjCount);
		if (nObjCount > 0)
		{
			std::vector<IStatObj*> pObjects;
			pObjects.resize(nObjCount);
			p3DEngine->GetLoadedStatObjArray(&pObjects[0],nObjCount);
			
			for (int nCurObj = 0; nCurObj < nObjCount; nCurObj++)
			{
				CString statObjName = pObjects[nCurObj]->GetFilePath();
				Path::ConvertSlashToBackSlash(statObjName);
				TFilenameAssetMap::iterator itr = m_assets.find(statObjName.GetBuffer());
				if ( m_assets.end() != itr )
				{
					itr->second->SetUsedInLevel(true);
					static_cast<CAssetModelItem*>(itr->second)->CacheFieldsInfoForLoadedStatObj(pObjects[nCurObj]);
				}
			}
		}
	}

}
