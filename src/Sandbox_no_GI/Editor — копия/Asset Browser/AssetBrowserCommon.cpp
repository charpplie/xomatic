////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetBrowserCommon.cpp
//  Version:	v1.00
//  Created:	21/04/2010 by Nicusor Nedelcu
//  Description:	Implementation of AssetBrowserCommon.h
//					
// -------------------------------------------------------------------------  
//  History:
//		12/03/2010	12:48	:	Nicusor Nedelcu - refactored
//		08/07/2010	17:53	:	Nicusor Nedelcu - cleaned and commented, added consts
//
//////////////////////////////////////////////////////////////////////////// 

#include "stdafx.h"
#include "AssetBrowserCommon.h"
#include "Include/IAssetViewer.h"
#include "StringUtils.h"
#include "Util/MemoryBlock.h"
#include "Util/Image.h"
#include "Util/ImageUtil.h"
#include "Util/PathUtil.h"
#include "Include/IAssetDisplayDatabase.h"
#include "Include/IAssetViewer.h"
#include "ImageExtensionHelper.h"
#include "AssetTags.h"
#include "AssetMetaDataFileDB.h"

static bool WildcardCompareString( const char *pWildcard, const char *pText, bool bCaseSensitive = false )
{
	const char *cp = NULL, *mp = NULL;

	#define WILDCARD_CMP_CHAR( aChar ) ( bCaseSensitive ? aChar : tolower( aChar ) )

	while( ( *pText ) && ( *pWildcard != '*' ) )
	{
		if( ( WILDCARD_CMP_CHAR( *pWildcard ) != WILDCARD_CMP_CHAR( *pText ) ) && ( *pWildcard != '?' ) )
		{
			return false;
		}

		pWildcard++;
		pText++;
	}

	while( *pText )
	{
		if( *pWildcard == '*' )
		{
			if( !*(++pWildcard) )
			{
				return true;
			}

			mp = pWildcard;
			cp = pText + 1;
		}
		else
		if( ( WILDCARD_CMP_CHAR( *pWildcard ) == WILDCARD_CMP_CHAR( *pText ) ) || ( *pWildcard == '?' ) )
		{
			pWildcard++;
			pText++;
		}
		else
		{
			pWildcard = mp;
			pText = cp++;
		}
	}

	while( *pWildcard == '*' )
	{
		pWildcard++;
	}

	return ( !*pWildcard );
}

static bool SearchTextWithWildcard( const char* pFindWhat, const char* pSearchWhere )
{
	// no wildcards found, we should search if contains
	if( !strstr( pFindWhat, "*" ) && !strstr( pFindWhat, "?" ) )
	{
		return CryStringUtils::stristr( pSearchWhere, pFindWhat );
	}

	return WildcardCompareString( pFindWhat, pSearchWhere );
}

CAssetDisplayDatabase::CAssetDisplayDatabase()
{
	m_poAssociatedViewer = NULL;
	m_ref = 1;
}

CAssetDisplayDatabase::~CAssetDisplayDatabase()
{
	// emtpty, call FreeData() first
}

void CAssetDisplayDatabase::CacheFieldsInfoForAlreadyLoadedAssets()
{
}

void CAssetDisplayDatabase::PrecacheFieldsInfoFromFileDB(const XmlNodeRef& db)
{
}

void CAssetDisplayDatabase::FreeData()
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

bool CAssetDisplayDatabase::SetAssociatedViewer(IAssetViewer* poAssociatedViewer)
{
	m_poAssociatedViewer = poAssociatedViewer;

	return true;
}

IAssetViewer* CAssetDisplayDatabase::GetAssociatedViewer() const
{
	return m_poAssociatedViewer;
}

const char* CAssetDisplayDatabase::GetDatabaseTypeExt() const
{
	return ""; 
};

IAssetDisplayDatabase::TAssetFields& CAssetDisplayDatabase::GetAssetFields()
{
	return m_assetFields;
}

SAssetField* CAssetDisplayDatabase::GetAssetFieldByName( const char* pFieldName )
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

const char* CAssetDisplayDatabase::GetDatabaseName() const
{
	return "";
}

void CAssetDisplayDatabase::Refresh()
{
	// empty
}

IAssetDisplayDatabase::TFilenameAssetMap&	CAssetDisplayDatabase::GetAssets()
{
	return m_assets;
}

IAssetDisplay* CAssetDisplayDatabase::GetAsset( const char* pAssetFilename )
{
	TFilenameAssetMap::iterator iter = m_assets.find( pAssetFilename );

	if( iter != m_assets.end() )
	{
		return iter->second;
	}

	return NULL;
}

void CAssetDisplayDatabase::ApplyFilters( const TAssetFieldFiltersMap& rFieldFilters )
{
	std::map<CString,char*> cFieldFiltersValueRawData, cFieldFiltersMinValueRawData, cFieldFiltersMaxValueRawData;
	bool bAssetIsVisible;
	bool bAssetIsVisibleByPostFilters;
	bool bAssetIsVisibleByFilters;
	TAssetFieldFiltersMap::const_iterator iterFullSearchText = rFieldFilters.find( "fullsearchtext" );
	bool bHasFullSearchText = iterFullSearchText != rFieldFilters.end();

	for( TFilenameAssetMap::iterator iterAsset = m_assets.begin(), iterAssetsEnd = m_assets.end(); iterAsset != iterAssetsEnd; ++iterAsset )
	{
		IAssetDisplay* pAsset = iterAsset->second;

		bAssetIsVisibleByPostFilters = true;
		bAssetIsVisibleByFilters = true;
		bAssetIsVisible = true;

		// loop through all field filters and abort if one of them does not comply
		for( TAssetFieldFiltersMap::const_iterator iter = rFieldFilters.begin(), iterEnd = rFieldFilters.end(); iter != iterEnd; ++iter )
		{
			bAssetIsVisible = true;

			// if asset is not visible even after "post" filters, just abort, its hidden
			if( !bAssetIsVisibleByPostFilters )
				break;

			const SAssetField& field = iter->second;

			// skip special field, treated after this loop
			if( field.m_fieldName == "fullsearchtext" )
				continue;

			// if this field is not from all databases
			if( !field.m_parentDatabaseName.IsEmpty() )
			{
				// skip fields that are not handled by this database
				if( field.m_parentDatabaseName != GetDatabaseName() )
				{
					continue;
				}
			}

			SAssetField::EAssetFilterCondition filterCondition = field.m_filterCondition;

			switch( field.m_fieldType )
			{
				case SAssetField::eAssetFieldType_None:
				{
					assert( !"eAssetFieldType_None is not a permitted type for a field, you must initialize it to a field type" );
					continue;
				}

				case SAssetField::eAssetFieldType_Bool:
				{
					bool filterValue, assetFieldValue;

					filterValue = ( field.m_filterValue == "Yes" );

					if( !pAsset->GetAssetFieldValue( field.m_fieldName, &assetFieldValue ) )
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
						case SAssetField::eAssertFilterCondition_InsideRange: break;// no sense to use range for a bool field
					}

					break;
				}

				case SAssetField::eAssetFieldType_Int8:
				{
					char filterValue, assetFieldValue;

					filterValue = atoi( field.m_filterValue );

					if( !pAsset->GetAssetFieldValue( field.m_fieldName, &assetFieldValue ) )
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
							char maxFilterValue = atoi( field.m_maxFilterValue );
							bAssetIsVisible = ( assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue ); 
							break;
						}
					}

					break;
				}

				case SAssetField::eAssetFieldType_Int16:
				{
					short int filterValue, assetFieldValue;

					filterValue = atoi( field.m_filterValue );

					if( !pAsset->GetAssetFieldValue( field.m_fieldName, &assetFieldValue ) )
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
							short int maxFilterValue = atoi( field.m_maxFilterValue );
							bAssetIsVisible = ( assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue ); 
							break;
						}
					}

					break;
				}

				case SAssetField::eAssetFieldType_Int32:
				{
					int filterValue, assetFieldValue;

					filterValue = atoi( field.m_filterValue );

					if( !pAsset->GetAssetFieldValue( field.m_fieldName, &assetFieldValue ) )
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
							int maxFilterValue = atoi( field.m_maxFilterValue );
							bAssetIsVisible = ( assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue ); 
							break;
						}
					}

					break;
				}

				case SAssetField::eAssetFieldType_Int64:
				{
					__int64 filterValue, assetFieldValue;

					filterValue = _atoi64( field.m_filterValue );

					if( !pAsset->GetAssetFieldValue( field.m_fieldName, &assetFieldValue ) )
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
							__int64 maxFilterValue = _atoi64( field.m_maxFilterValue );
							bAssetIsVisible = ( assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue ); 
							break;
						}
					}

					break;
				}

				case SAssetField::eAssetFieldType_Float:
				{
					float filterValue, assetFieldValue;

					filterValue = atof( field.m_filterValue );

					if( !pAsset->GetAssetFieldValue( field.m_fieldName, &assetFieldValue ) )
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
							float maxFilterValue = atof( field.m_maxFilterValue );
							bAssetIsVisible = ( assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue ); 
							break;
						}
					}

					break;
				}

				case SAssetField::eAssetFieldType_Double:
				{
					double filterValue, assetFieldValue;

					filterValue = atof( field.m_filterValue );

					if( !pAsset->GetAssetFieldValue( field.m_fieldName, &assetFieldValue ) )
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
							double maxFilterValue = atof( field.m_maxFilterValue );
							bAssetIsVisible = ( assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue ); 
							break;
						}
					}

					break;
				}

				case SAssetField::eAssetFieldType_String:
				{
					CString assetFieldValue;
					const CString& filterValue = field.m_filterValue;

					if( !pAsset->GetAssetFieldValue( field.m_fieldName, &assetFieldValue ) )
					{
						bAssetIsVisible = false;
						continue;
					}

					switch( filterCondition )
					{
						case SAssetField::eAssertFilterCondition_Contains: bAssetIsVisible = SearchTextWithWildcard( filterValue, assetFieldValue ); break;
						case SAssetField::eAssertFilterCondition_ContainsOneOfTheWords:
						{
							std::vector<CString> words;
							
							SplitString( (CString&)filterValue, words, ' ' );

							for( size_t w = 0, wCount = words.size(); w < wCount; ++w )
							{
								bAssetIsVisible = SearchTextWithWildcard( words[w].GetBuffer(), assetFieldValue.GetBuffer() );
								
								// break if we find one word which is contained by the field value
								if( bAssetIsVisible )
									break;
							}
							
							break;
						}
						case SAssetField::eAssertFilterCondition_StartsWith: bAssetIsVisible = ( 0 == assetFieldValue.Find( filterValue ) ); break;
						case SAssetField::eAssertFilterCondition_EndsWith: bAssetIsVisible = ( assetFieldValue.Mid( assetFieldValue.GetLength() - filterValue.GetLength(), filterValue.GetLength() ) == filterValue ); break;
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

			//
			// if this field is an 'post filter', then remember its status
			//
			if( field.m_bPostFilter )
			{
				if( !bAssetIsVisible )
				{
					bAssetIsVisibleByPostFilters = false;
				}
			}
			else
			{
				if( !bAssetIsVisible )
				{
					bAssetIsVisibleByFilters = false;
				}
			}
		}

		//
		// check special 'fullsearchtext' field (this is provided when the user searches in the full text search edit box in the asset browser)
		//

		bool bAssetIsVisibleByFullTextSearch = true;

		if( bHasFullSearchText && iterFullSearchText->second.m_filterValue != "" )
		{
			CString assetFieldValue;

			// get the filename asset field value to check against
			if( pAsset->GetAssetFieldValue( "filename", &assetFieldValue ) )
			{
				bAssetIsVisibleByFullTextSearch = SearchTextWithWildcard( iterFullSearchText->second.m_filterValue, assetFieldValue );
			}

			if( !bAssetIsVisibleByFullTextSearch )
			{
				std::vector<CString> words;

				// prepare the tags words
				SplitString( (CString&)iterFullSearchText->second.m_filterValue, words, ' ' );

				// get the tags asset field value to check against
				if( pAsset->GetAssetFieldValue( "tags", &assetFieldValue ) )
				{
					// search the tags
					for( size_t w = 0, wCount = words.size(); w < wCount; ++w )
					{
						bAssetIsVisibleByFullTextSearch = SearchTextWithWildcard( words[w], assetFieldValue );

						// break if we find one word which is contained by the field value
						if( bAssetIsVisibleByFullTextSearch )
							break;
					}
				}
			}
		}

		bAssetIsVisible = bAssetIsVisibleByFilters && bAssetIsVisibleByPostFilters && bAssetIsVisibleByFullTextSearch;
		pAsset->SetFlag( IAssetDisplay::eAssetFlags_Visible, bAssetIsVisible );
	}
}

void CAssetDisplayDatabase::ClearFilters()
{
	for( TFilenameAssetMap::iterator iterAsset = m_assets.begin(), iterAssetsEnd = m_assets.end(); iterAsset != iterAssetsEnd; ++iterAsset )
	{
		IAssetDisplay* pAsset = iterAsset->second;
		
		pAsset->SetFlag( IAssetDisplay::eAssetFlags_Visible, true );
	}
}

const char* CAssetDisplayDatabase::GetTransactionFilename() const
{
	return "commonTransactions.xml";
}

bool CAssetDisplayDatabase::AddMetaDataChangeListener(IAssetDisplayDatabase::MetaDataChangeListener callBack)
{
	return stl::push_back_unique(m_metaDataChangeListeners, callBack);
}

bool CAssetDisplayDatabase::RemoveMetaDataChangeListener(IAssetDisplayDatabase::MetaDataChangeListener callBack)
{
	return stl::find_and_erase(m_metaDataChangeListeners, callBack);
}

void CAssetDisplayDatabase::OnMetaDataChange(const IAssetDisplay* pAssetItem)
{
	for(size_t i=0; i<m_metaDataChangeListeners.size(); ++i)
	{
		m_metaDataChangeListeners[i](pAssetItem);
	}
}

//---

CAssetDisplay::CAssetDisplay():
	m_flags(0),
	m_nFileSize(0),
	m_oDrawingRectangle(0, 0, 0, 0),
	m_piOwnerDatabase(NULL),
	m_ref(1),
	m_assetIndex(0),
	m_hPreviewDC(0)
{
}

CAssetDisplay::~CAssetDisplay()
{
	// empty, call FreeData first
}

void CAssetDisplay::FreeData()
{
	UnCache();
	UnCacheThumbnail();
}

IAssetDisplayDatabase* CAssetDisplay::GetOwnerDisplayDatabase() const
{
	return m_piOwnerDatabase;
}

void CAssetDisplay::SetOwnerDisplayDatabase( IAssetDisplayDatabase* piOwnerDisplayDatabase )
{
	m_piOwnerDatabase = piOwnerDisplayDatabase;
}

const IAssetDisplay::TAssetDependencyCategFilenamesMap& CAssetDisplay::GetDependencies() const
{
	return m_dependencies;
}

void CAssetDisplay::SetFileSize( unsigned __int64 aSize )
{
	m_nFileSize = aSize;
}

void CAssetDisplay::SetFileExtension( const char* pExt )
{
	m_strExtension = pExt;
}

const char* CAssetDisplay::GetFileExtension() const
{
	return m_strExtension;
}

unsigned __int64 CAssetDisplay::GetFileSize() const
{
	return m_nFileSize;
}

void CAssetDisplay::SetFilename( const char* pName )
{
	m_strFilename = pName;
}

const char* CAssetDisplay::GetFilename() const
{
	return m_strFilename;
}

void CAssetDisplay::SetRelativePath( const char* pPath )
{
	m_strRelativePath = pPath;
}

const char* CAssetDisplay::GetRelativePath() const
{
	return m_strRelativePath;
}

UINT CAssetDisplay::GetFlags() const
{
	return m_flags;
}

void CAssetDisplay::SetFlags( UINT aFlags )
{
	m_flags = aFlags;
}

void CAssetDisplay::SetFlag( EAssetFlags aFlag, bool bSet )
{
	m_flags = bSet ? ( m_flags | (UINT)aFlag ) : ( m_flags & (~(UINT)aFlag) );
}

bool CAssetDisplay::IsFlagSet( EAssetFlags aFlag ) const
{
	return m_flags & (UINT) aFlag;
}

void CAssetDisplay::SetIndex( UINT aIndex )
{
	m_assetIndex = aIndex;
}

UINT CAssetDisplay::GetIndex() const
{
	return m_assetIndex;
}

bool CAssetDisplay::GetAssetFieldValue( const char* pFieldName, void* pDest )
{
	if( IS_ASSET_FIELD_NAME( "filename" ) )
	{
		*(CString*)pDest = m_strFilename;
		return true;
	}
	else
	if( IS_ASSET_FIELD_NAME( "dccfilename" ) )
	{
		*(CString*)pDest = m_strDccFilename;
		return true;
	}
	else
	if( IS_ASSET_FIELD_NAME( "relativepath" ) )
	{
		*(CString*)pDest = m_strRelativePath;
		return true;
	}
	else
	if( IS_ASSET_FIELD_NAME( "extension" ) )
	{
		*(CString*)pDest = m_strExtension;
		return true;
	}
	else
	if( IS_ASSET_FIELD_NAME( "filesize" ) )
	{
		*(int*)pDest = m_nFileSize;
		return true;
	}
	else
	if( IS_ASSET_FIELD_NAME( "fullfilepath" ) )
	{
		*(CString*)pDest = m_strRelativePath + m_strFilename;
		return true;
	}
	else
	if( IS_ASSET_FIELD_NAME( "usedinlevel" ) )
	{
		*(bool*)pDest = 0 != ( m_flags & eAssetFlags_UsedInLevel );
		return true;
	}
	else
	if( IS_ASSET_FIELD_NAME( "tags" ) )
	{
		CString path = GetRelativePath();
		path += GetFilename();

		CAssetTags::StrVector tags;
		CAssetMetaDataFileDB::GetTagManager().GetTagsForAsset(tags, path);
		CString result = "";
		for(size_t i=0; i<tags.size(); ++i)
		{
			result += tags[i];
			if(i < tags.size() - 1)
				result += ",";
		}
		*(CString*)pDest = result;
		return true;
	}
	else
	if( IS_ASSET_FIELD_NAME( "favourite" ) )
	{
		*(bool*)pDest = 0 != ( m_flags & eAssetFlags_Favourite );
		return true;
	}

	return false;
}

bool CAssetDisplay::SetAssetFieldValue( const char* pFieldName, void* pSrc )
{
	return false;
}


void CAssetDisplay::GetDrawingRectangle( CRect& rstDrawingRectangle ) const
{ 
	rstDrawingRectangle = m_oDrawingRectangle;
}

void	CAssetDisplay::SetDrawingRectangle( const CRect& crstDrawingRectangle )
{
	m_oDrawingRectangle = crstDrawingRectangle;
}

bool CAssetDisplay::Cache()
{
	return false;
}

bool CAssetDisplay::CacheFieldsInfo()
{
	return false;
}

bool CAssetDisplay::UnCache()
{
	return false;
}

bool CAssetDisplay::UnCacheThumbnail()
{
	return false;
}

void CAssetDisplay::OnBeginPreview( const HWND hQuickPreviewWnd, const HDC hMemDC )
{
	m_hPreviewDC = hMemDC;
}

CDialog* CAssetDisplay::GetCustomPreviewPanel( CWnd* pParentWnd )
{
	return NULL;
}

void CAssetDisplay::OnEndPreview()
{
	m_hPreviewDC = 0;
}

void CAssetDisplay::InteractiveRender(	const HWND hRenderWindow, const CRect& rstViewport, int aMouseX, int aMouseY,
																				int aMouseDeltaX, int aMouseDeltaY, int aMouseWheelDelta, UINT aKeyFlags )
{
	Render( hRenderWindow, rstViewport, false );
}

void CAssetDisplay::OnInteractiveRenderKeyEvent( bool bKeyDown, UINT aChar, UINT aKeyFlags )
{
}

void CAssetDisplay::OnThumbClick( const CPoint& point, UINT aKeyFlags )
{
}

void CAssetDisplay::OnThumbDblClick( const CPoint& point, UINT aKeyFlags )
{
}

bool CAssetDisplay::Render( const HWND hRenderWindow, const CRect& rstViewport, bool bCacheThumbnail )
{
	return false;
}

bool CAssetDisplay::DrawThumbImage( const HDC hDC, const CRect& rRect )
{
	return false;
}

bool CAssetDisplay::HitTest( int nX, int nY ) const
{
	return m_oDrawingRectangle.PtInRect( CPoint( nX, nY ) ) == TRUE;
}

bool CAssetDisplay::HitTest( const CRect& roTestRect ) const
{
	CRect oIntersection;

	return oIntersection.IntersectRect( &m_oDrawingRectangle, &roTestRect ) == TRUE;
}

void* CAssetDisplay::CreateInstanceInViewport( float aX, float aY, float aZ )
{
	return NULL;
}

bool CAssetDisplay::MoveInstanceInViewport( const void* pDraggedObject, float aNewX, float aNewY, float aNewZ )
{
	return false;
}

void CAssetDisplay::AbortCreateInstanceInViewport( const void* pDraggedObject )
{
}

void CAssetDisplay::DrawTextOnReportImage( CAlphaBitmap &rDestBmp ) const
{
}

bool CAssetDisplay::SaveReportImage( const char *pFilePath ) const
{
		return false;
}

bool CAssetDisplay::SaveReportText( const char *pFilePath ) const
{
	return false;
}

void CAssetDisplay::ToXML(XmlNodeRef& node) const
{
}

void CAssetDisplay::FromXML(const XmlNodeRef& node)
{
}