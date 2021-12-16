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
#include "Include/IAssetItemDatabase.h"
#include "Include/IAssetViewer.h"
#include "ImageExtensionHelper.h"
#include "AssetBrowserManager.h"

namespace AssetBrowser
{
const char* kThumbnailsRoot = "AssetBrowser/Thumbs/";
}

inline char WildcardCmpChar(bool bCaseSensitive, char aChar )
{
	return (bCaseSensitive ? aChar : tolower(aChar));
}

static bool WildcardCompareString(const char* pWildcard, const char* pText, bool bCaseSensitive = false)
{
	const char* cp = NULL, *mp = NULL;

	while ((*pText) && (*pWildcard != '*'))
	{
		if ((WildcardCmpChar(bCaseSensitive, *pWildcard) != WildcardCmpChar(bCaseSensitive, *pText))
		    && (*pWildcard != '?'))
		{
			return false;
		}

		pWildcard++;
		pText++;
	}

	while (*pText)
	{
		if (*pWildcard == '*')
		{
			if (!*(++pWildcard))
			{
				return true;
			}

			mp = pWildcard;
			cp = pText + 1;
		}
		else if ((WildcardCmpChar(bCaseSensitive, *pWildcard) == WildcardCmpChar(bCaseSensitive, *pText))
		         || (*pWildcard == '?'))
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

	while (*pWildcard == '*')
	{
		pWildcard++;
	}

	return (!*pWildcard);
}

static bool SearchTextWithWildcard(const char* pFindWhat, const char* pSearchWhere)
{
	// no wildcards found, we should search if contains
	if (!strstr(pFindWhat, "*") && !strstr(pFindWhat, "?"))
	{
		return CryStringUtils::stristr(pSearchWhere, pFindWhat);
	}

	return WildcardCompareString(pFindWhat, pSearchWhere);
}

CAssetItemDatabase::CAssetItemDatabase()
{
	m_ref = 1;
}

CAssetItemDatabase::~CAssetItemDatabase()
{
	// emtpty, call FreeData() first
}

void CAssetItemDatabase::PrecacheFieldsInfoFromFileDB(const XmlNodeRef& db)
{
}

void CAssetItemDatabase::FreeData()
{
	if (m_assets.empty())
	{
		return;
	}

	Log("Release database %s asset items...", GetDatabaseName());

	for (TFilenameAssetMap::iterator iter = m_assets.begin(), iterEnd = m_assets.end(); iter != iterEnd; ++iter)
	{
		SAFE_RELEASE(iter->second);
	}

	m_assets.clear();
}

const char* CAssetItemDatabase::GetSupportedExtensions() const
{
	return "";
};

IAssetItemDatabase::TAssetFields& CAssetItemDatabase::GetAssetFields()
{
	return m_assetFields;
}

SAssetField* CAssetItemDatabase::GetAssetFieldByName(const char* pFieldName)
{
	for (size_t i = 0, iCount = m_assetFields.size(); i < iCount; ++i)
	{
		if (m_assetFields[i].m_fieldName == pFieldName)
		{
			return &m_assetFields[i];
		}
	}

	return NULL;
}

const char* CAssetItemDatabase::GetDatabaseName() const
{
	return "";
}

void CAssetItemDatabase::Refresh()
{
	// empty
}

IAssetItemDatabase::TFilenameAssetMap&	CAssetItemDatabase::GetAssets()
{
	return m_assets;
}

IAssetItem* CAssetItemDatabase::GetAsset(const char* pAssetFilename)
{
	TFilenameAssetMap::iterator iter = m_assets.find(pAssetFilename);

	if (iter != m_assets.end())
	{
		return iter->second;
	}

	return NULL;
}

void CAssetItemDatabase::ApplyTagFilters(const TAssetFieldFiltersMap& rFieldFilters, CAssetBrowserManager::StrVector& assetList)
{
	CAssetBrowserManager::StrVector tags;

	for (auto iter = rFieldFilters.begin(), iterEnd = rFieldFilters.end(); iter != iterEnd; ++iter)
	{
		const SAssetField& field = iter->second;

		if (field.m_fieldName == "tags")
		{
			if (field.m_filterCondition != SAssetField::eCondition_Contains)
			{
				continue;
			}

			CString filterValue(field.m_filterValue);

			if (filterValue.IsEmpty())
			{
				continue;
			}

			tags.push_back(filterValue);
		}
	}

	for (auto item = tags.begin(), end = tags.end(); item != end; ++item)
	{
		if (item->IsEmpty())
		{
			continue;
		}

		CAssetBrowserManager::Instance()->GetAssetsForTag(assetList, (*item));
		CAssetBrowserManager::Instance()->GetAssetsWithDescription(assetList, (*item));
	}
}

void CAssetItemDatabase::ApplyFilters(const TAssetFieldFiltersMap& rFieldFilters)
{
	CAssetBrowserManager::StrVector assetList;
	TFilenameAssetMap tagFilteredAssetList;
	bool bFoundAssetTags = false;

	// loop through all field filters and abort if one of them does not comply
	for (auto iter = rFieldFilters.begin(), iterEnd = rFieldFilters.end(); iter != iterEnd; ++iter)
	{
		const SAssetField& field = iter->second;

		if (field.m_fieldName == "tags")
		{
			ApplyTagFilters(rFieldFilters, assetList);
			bFoundAssetTags = !assetList.empty();
			break;
		}
	}

	if (bFoundAssetTags)
	{
		for (auto item = assetList.begin(), end = assetList.end(); item != end; ++item)
		{
			for (auto iterAsset = m_assets.begin(), iterAssetsEnd = m_assets.end(); iterAsset != iterAssetsEnd; ++iterAsset)
			{
				bool found = false;
				IAssetItem* pAsset = iterAsset->second;

				if (iterAsset->first.CompareNoCase((*item)) == 0)
				{
					found = true;
				}

				pAsset->SetFlag(IAssetItem::eFlag_Visible, found);
			}
		}

		return;
	}

	std::map<CString, char*> cFieldFiltersValueRawData, cFieldFiltersMinValueRawData, cFieldFiltersMaxValueRawData;
	bool bAssetIsVisible;
	bool bAssetIsVisibleByPostFilters;
	bool bAssetIsVisibleByFilters;
	TAssetFieldFiltersMap::const_iterator iterFullSearchText = rFieldFilters.find("fullsearchtext");
	bool bHasFullSearchText = iterFullSearchText != rFieldFilters.end();

	for (auto iterAsset = m_assets.begin(), iterAssetsEnd = m_assets.end(); iterAsset != iterAssetsEnd; ++iterAsset)
	{
		IAssetItem* pAsset = iterAsset->second;
		pAsset->SetFlag(IAssetItem::eFlag_Visible, true);
		bAssetIsVisibleByPostFilters = true;
		bAssetIsVisibleByFilters = true;
		bAssetIsVisible = true;

		// loop through all field filters and abort if one of them does not comply
		for (auto iter = rFieldFilters.begin(), iterEnd = rFieldFilters.end(); iter != iterEnd; ++iter)
		{
			bAssetIsVisible = true;

			// if asset is not visible even after "post" filters, just abort, its hidden
			if (!bAssetIsVisibleByPostFilters)
			{
				break;
			}

			const SAssetField& field = iter->second;

			if (field.m_fieldName == "")
			{
				continue;
			}

			// skip special field, treated after this loop
			if (field.m_fieldName == "fullsearchtext")
			{
				continue;
			}

			// if this field is not from all databases
			if (!field.m_parentDatabaseName.IsEmpty())
			{
				// skip fields that are not handled by this database
				if (field.m_parentDatabaseName != GetDatabaseName())
				{
					continue;
				}
			}

			SAssetField::EAssetFilterCondition filterCondition = field.m_filterCondition;

			switch (field.m_fieldType)
			{
			case SAssetField::eType_None:
			{
				assert(!"eType_None is not a permitted type for a field, you must initialize it to a field type");
				continue;
			}

			case SAssetField::eType_Bool:
			{
				bool filterValue, assetFieldValue;

				filterValue = (field.m_filterValue == "Yes");

				if (!pAsset->GetAssetFieldValue(field.m_fieldName, &assetFieldValue))
				{
					continue;
				}

				switch (filterCondition)
				{
				case SAssetField::eCondition_Equal:
					bAssetIsVisible = (assetFieldValue == filterValue);
					break;

				case SAssetField::eCondition_Greater:
					bAssetIsVisible = (assetFieldValue > filterValue);
					break;

				case SAssetField::eCondition_Less:
					bAssetIsVisible = (assetFieldValue < filterValue);
					break;

				case SAssetField::eCondition_GreaterOrEqual:
					bAssetIsVisible = (assetFieldValue >= filterValue);
					break;

				case SAssetField::eCondition_LessOrEqual:
					bAssetIsVisible = (assetFieldValue <= filterValue);
					break;

				case SAssetField::eCondition_Not:
					bAssetIsVisible = (assetFieldValue != filterValue);
					break;

				case SAssetField::eCondition_InsideRange:
					break;// no sense to use range for a bool field
				}

				break;
			}

			case SAssetField::eType_Int8:
			{
				char filterValue, assetFieldValue;

				filterValue = atoi(field.m_filterValue);

				if (!pAsset->GetAssetFieldValue(field.m_fieldName, &assetFieldValue))
				{
					continue;
				}

				switch (filterCondition)
				{
				case SAssetField::eCondition_Equal:
					bAssetIsVisible = (assetFieldValue == filterValue);
					break;

				case SAssetField::eCondition_Greater:
					bAssetIsVisible = (assetFieldValue > filterValue);
					break;

				case SAssetField::eCondition_Less:
					bAssetIsVisible = (assetFieldValue < filterValue);
					break;

				case SAssetField::eCondition_GreaterOrEqual:
					bAssetIsVisible = (assetFieldValue >= filterValue);
					break;

				case SAssetField::eCondition_LessOrEqual:
					bAssetIsVisible = (assetFieldValue <= filterValue);
					break;

				case SAssetField::eCondition_Not:
					bAssetIsVisible = (assetFieldValue != filterValue);
					break;

				case SAssetField::eCondition_InsideRange:
				{
					char maxFilterValue = atoi(field.m_maxFilterValue);
					bAssetIsVisible = (assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue);
					break;
				}
				}

				break;
			}

			case SAssetField::eType_Int16:
			{
				short int filterValue, assetFieldValue;

				filterValue = atoi(field.m_filterValue);

				if (!pAsset->GetAssetFieldValue(field.m_fieldName, &assetFieldValue))
				{
					continue;
				}

				switch (filterCondition)
				{
				case SAssetField::eCondition_Equal:
					bAssetIsVisible = (assetFieldValue == filterValue);
					break;

				case SAssetField::eCondition_Greater:
					bAssetIsVisible = (assetFieldValue > filterValue);
					break;

				case SAssetField::eCondition_Less:
					bAssetIsVisible = (assetFieldValue < filterValue);
					break;

				case SAssetField::eCondition_GreaterOrEqual:
					bAssetIsVisible = (assetFieldValue >= filterValue);
					break;

				case SAssetField::eCondition_LessOrEqual:
					bAssetIsVisible = (assetFieldValue <= filterValue);
					break;

				case SAssetField::eCondition_Not:
					bAssetIsVisible = (assetFieldValue != filterValue);
					break;

				case SAssetField::eCondition_InsideRange:
				{
					short int maxFilterValue = atoi(field.m_maxFilterValue);
					bAssetIsVisible = (assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue);
					break;
				}
				}

				break;
			}

			case SAssetField::eType_Int32:
			{
				int filterValue, assetFieldValue;

				filterValue = atoi(field.m_filterValue);

				if (!pAsset->GetAssetFieldValue(field.m_fieldName, &assetFieldValue))
				{
					continue;
				}

				switch (filterCondition)
				{
				case SAssetField::eCondition_Equal:
					bAssetIsVisible = (assetFieldValue == filterValue);
					break;

				case SAssetField::eCondition_Greater:
					bAssetIsVisible = (assetFieldValue > filterValue);
					break;

				case SAssetField::eCondition_Less:
					bAssetIsVisible = (assetFieldValue < filterValue);
					break;

				case SAssetField::eCondition_GreaterOrEqual:
					bAssetIsVisible = (assetFieldValue >= filterValue);
					break;

				case SAssetField::eCondition_LessOrEqual:
					bAssetIsVisible = (assetFieldValue <= filterValue);
					break;

				case SAssetField::eCondition_Not:
					bAssetIsVisible = (assetFieldValue != filterValue);
					break;

				case SAssetField::eCondition_InsideRange:
				{
					int maxFilterValue = atoi(field.m_maxFilterValue);
					bAssetIsVisible = (assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue);
					break;
				}
				}

				break;
			}

			case SAssetField::eType_Int64:
			{
				__int64 filterValue, assetFieldValue;

				filterValue = _atoi64(field.m_filterValue);

				if (!pAsset->GetAssetFieldValue(field.m_fieldName, &assetFieldValue))
				{
					continue;
				}

				switch (filterCondition)
				{
				case SAssetField::eCondition_Equal:
					bAssetIsVisible = (assetFieldValue == filterValue);
					break;

				case SAssetField::eCondition_Greater:
					bAssetIsVisible = (assetFieldValue > filterValue);
					break;

				case SAssetField::eCondition_Less:
					bAssetIsVisible = (assetFieldValue < filterValue);
					break;

				case SAssetField::eCondition_GreaterOrEqual:
					bAssetIsVisible = (assetFieldValue >= filterValue);
					break;

				case SAssetField::eCondition_LessOrEqual:
					bAssetIsVisible = (assetFieldValue <= filterValue);
					break;

				case SAssetField::eCondition_Not:
					bAssetIsVisible = (assetFieldValue != filterValue);
					break;

				case SAssetField::eCondition_InsideRange:
				{
					__int64 maxFilterValue = _atoi64(field.m_maxFilterValue);
					bAssetIsVisible = (assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue);
					break;
				}
				}

				break;
			}

			case SAssetField::eType_Float:
			{
				float filterValue, assetFieldValue;

				filterValue = atof(field.m_filterValue);

				if (!pAsset->GetAssetFieldValue(field.m_fieldName, &assetFieldValue))
				{
					continue;
				}

				switch (filterCondition)
				{
				case SAssetField::eCondition_Equal:
					bAssetIsVisible = (assetFieldValue == filterValue);
					break;

				case SAssetField::eCondition_Greater:
					bAssetIsVisible = (assetFieldValue > filterValue);
					break;

				case SAssetField::eCondition_Less:
					bAssetIsVisible = (assetFieldValue < filterValue);
					break;

				case SAssetField::eCondition_GreaterOrEqual:
					bAssetIsVisible = (assetFieldValue >= filterValue);
					break;

				case SAssetField::eCondition_LessOrEqual:
					bAssetIsVisible = (assetFieldValue <= filterValue);
					break;

				case SAssetField::eCondition_Not:
					bAssetIsVisible = (assetFieldValue != filterValue);
					break;

				case SAssetField::eCondition_InsideRange:
				{
					float maxFilterValue = atof(field.m_maxFilterValue);
					bAssetIsVisible = (assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue);
					break;
				}
				}

				break;
			}

			case SAssetField::eType_Double:
			{
				double filterValue, assetFieldValue;

				filterValue = atof(field.m_filterValue);

				if (!pAsset->GetAssetFieldValue(field.m_fieldName, &assetFieldValue))
				{
					continue;
				}

				switch (filterCondition)
				{
				case SAssetField::eCondition_Equal:
					bAssetIsVisible = (assetFieldValue == filterValue);
					break;

				case SAssetField::eCondition_Greater:
					bAssetIsVisible = (assetFieldValue > filterValue);
					break;

				case SAssetField::eCondition_Less:
					bAssetIsVisible = (assetFieldValue < filterValue);
					break;

				case SAssetField::eCondition_GreaterOrEqual:
					bAssetIsVisible = (assetFieldValue >= filterValue);
					break;

				case SAssetField::eCondition_LessOrEqual:
					bAssetIsVisible = (assetFieldValue <= filterValue);
					break;

				case SAssetField::eCondition_Not:
					bAssetIsVisible = (assetFieldValue != filterValue);
					break;

				case SAssetField::eCondition_InsideRange:
				{
					double maxFilterValue = atof(field.m_maxFilterValue);
					bAssetIsVisible = (assetFieldValue >= filterValue && assetFieldValue <= maxFilterValue);
					break;
				}
				}

				break;
			}

			case SAssetField::eType_String:
			{
				if (field.m_fieldName == "tags")
				{
					if (!field.m_filterValue.IsEmpty())
					{
						bool foundInTagAssetList = false;

						for (auto item = assetList.begin(), end = assetList.end(); item != end; ++item)
						{
							if (iterAsset->first == (*item))
							{
								foundInTagAssetList = true;
							}
						}

						if (!foundInTagAssetList)
						{
							bAssetIsVisible = false;
						}

						break;
					}
					else
					{
						bAssetIsVisible = true;
						break;
					}
				}

				CString assetFieldValue;
				const CString& filterValue = field.m_filterValue;

				if (!pAsset->GetAssetFieldValue(field.m_fieldName, &assetFieldValue))
				{
					break;
				}

				switch (filterCondition)
				{
				case SAssetField::eCondition_Contains:
					bAssetIsVisible = SearchTextWithWildcard(filterValue, assetFieldValue);
					break;

				case SAssetField::eCondition_ContainsOneOfTheWords:
				{
					std::vector<CString> words;

					SplitString((CString&)filterValue, words, ' ');

					for (size_t w = 0, wCount = words.size(); w < wCount; ++w)
					{
						bAssetIsVisible = SearchTextWithWildcard(words[w].GetBuffer(), assetFieldValue.GetBuffer());

						// break if we find one word which is contained by the field value
						if (bAssetIsVisible)
						{
							break;
						}
					}

					break;
				}

				case SAssetField::eCondition_StartsWith:
					bAssetIsVisible = (0 == assetFieldValue.Find(filterValue));
					break;

				case SAssetField::eCondition_EndsWith:
					bAssetIsVisible = (assetFieldValue.Mid(assetFieldValue.GetLength() - filterValue.GetLength(), filterValue.GetLength()) == filterValue);
					break;

				case SAssetField::eCondition_Equal:
					bAssetIsVisible = (assetFieldValue == filterValue);
					break;

				case SAssetField::eCondition_Greater:
					bAssetIsVisible = (assetFieldValue > filterValue);
					break;

				case SAssetField::eCondition_Less:
					bAssetIsVisible = (assetFieldValue < filterValue);
					break;

				case SAssetField::eCondition_GreaterOrEqual:
					bAssetIsVisible = (assetFieldValue >= filterValue);
					break;

				case SAssetField::eCondition_LessOrEqual:
					bAssetIsVisible = (assetFieldValue <= filterValue);
					break;

				case SAssetField::eCondition_Not:
					bAssetIsVisible = (assetFieldValue != filterValue);
					break;

				case SAssetField::eCondition_InsideRange:
				{
					bAssetIsVisible = (assetFieldValue >= filterValue && assetFieldValue <= field.m_maxFilterValue);
					break;
				}
				}

				break;
			}
			}

			//
			// if this field is an 'post filter', then remember its status
			//
			if (field.m_bPostFilter)
			{
				if (!bAssetIsVisible)
				{
					bAssetIsVisibleByPostFilters = false;
				}
			}
			else
			{
				if (!bAssetIsVisible)
				{
					bAssetIsVisibleByFilters = false;
				}
			}
		}

		//
		// check special 'fullsearchtext' field (this is provided when the user searches in the full text search edit box in the asset browser)
		//

		bool bAssetIsVisibleByFullTextSearch = true;

		if (bHasFullSearchText && iterFullSearchText->second.m_filterValue != "")
		{
			CString assetFieldValue;

			// get the filename asset field value to check against
			if (pAsset->GetAssetFieldValue("filename", &assetFieldValue))
			{
				bAssetIsVisibleByFullTextSearch = SearchTextWithWildcard(iterFullSearchText->second.m_filterValue, assetFieldValue);

				// lets try searching without extension
				if (!bAssetIsVisibleByFullTextSearch)
				{
					CString fileExt = Path::GetExt(assetFieldValue);

					assetFieldValue.Replace(("." + fileExt), "");
					bAssetIsVisibleByFullTextSearch = SearchTextWithWildcard(iterFullSearchText->second.m_filterValue, assetFieldValue);
				}
			}

			// try path then
			if (!bAssetIsVisibleByFullTextSearch)
			{
				// get the asset relative path field value to check against
				if (pAsset->GetAssetFieldValue("relativepath", &assetFieldValue))
				{
					bAssetIsVisibleByFullTextSearch = SearchTextWithWildcard(iterFullSearchText->second.m_filterValue, assetFieldValue);
					CString assetFilename;
					CString tmp;
					pAsset->GetAssetFieldValue("filename", &assetFilename);

					// lets try searching with whole path+filename (no ext)
					if (!bAssetIsVisibleByFullTextSearch)
					{
						CString fileExt = Path::GetExt(assetFieldValue);

						tmp = assetFilename;
						tmp.Replace(("." + fileExt), "");
						tmp = assetFieldValue + tmp;
						bAssetIsVisibleByFullTextSearch = SearchTextWithWildcard(iterFullSearchText->second.m_filterValue, tmp);

						// lets try searching with whole path+filename+ext
						if (!bAssetIsVisibleByFullTextSearch)
						{
							assetFieldValue += assetFilename;
							bAssetIsVisibleByFullTextSearch = SearchTextWithWildcard(iterFullSearchText->second.m_filterValue, assetFieldValue);
						}
					}
				}
			}

			if (!bAssetIsVisibleByFullTextSearch)
			{
				std::vector<CString> words;

				// prepare the tags words
				SplitString((CString&)iterFullSearchText->second.m_filterValue, words, ' ');

				// get the tags asset field value to check against
				if (pAsset->GetAssetFieldValue("tags", &assetFieldValue))
				{
					// search the tags
					for (size_t w = 0, wCount = words.size(); w < wCount; ++w)
					{
						bAssetIsVisibleByFullTextSearch = SearchTextWithWildcard(words[w], assetFieldValue);

						// break if we find one word which is contained by the field value
						if (bAssetIsVisibleByFullTextSearch)
						{
							break;
						}
					}
				}
			}
		}

		bAssetIsVisible = bAssetIsVisibleByFilters && bAssetIsVisibleByPostFilters && bAssetIsVisibleByFullTextSearch;
		pAsset->SetFlag(IAssetItem::eFlag_Visible, bAssetIsVisible);
	}
}

void CAssetItemDatabase::ClearFilters()
{
	for (TFilenameAssetMap::iterator iterAsset = m_assets.begin(), iterAssetsEnd = m_assets.end(); iterAsset != iterAssetsEnd; ++iterAsset)
	{
		IAssetItem* pAsset = iterAsset->second;

		pAsset->SetFlag(IAssetItem::eFlag_Visible, true);
	}
}

CDialog* CAssetItemDatabase::CreateDbFilterDialog(CWnd* pParent, IAssetViewer* pViewerCtrl)
{
	return NULL;
}

void CAssetItemDatabase::UpdateDbFilterDialogUI(CDialog* pDlg)
{
}

const char* CAssetItemDatabase::GetTransactionFilename() const
{
	return "commonTransactions.xml";
}

bool CAssetItemDatabase::AddMetaDataChangeListener(IAssetItemDatabase::MetaDataChangeListener callBack)
{
	return stl::push_back_unique(m_metaDataChangeListeners, callBack);
}

bool CAssetItemDatabase::RemoveMetaDataChangeListener(IAssetItemDatabase::MetaDataChangeListener callBack)
{
	return stl::find_and_erase(m_metaDataChangeListeners, callBack);
}

void CAssetItemDatabase::OnMetaDataChange(const IAssetItem* pAssetItem)
{
	for (size_t i = 0; i < m_metaDataChangeListeners.size(); ++i)
	{
		m_metaDataChangeListeners[i](pAssetItem);
	}
}

HRESULT STDMETHODCALLTYPE CAssetItemDatabase::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == __uuidof(IAssetItemDatabase)/* && m_pIntegrator*/)
	{
		*ppvObj = this;
		return S_OK;
	}

	return E_NOINTERFACE ;
}

ULONG STDMETHODCALLTYPE CAssetItemDatabase::AddRef()
{
	return ++m_ref;
};

ULONG STDMETHODCALLTYPE CAssetItemDatabase::Release()
{
	if ((--m_ref) == 0)
	{
		FreeData();
		delete this;
		return 0;
	}
	else
	{
		return m_ref;
	}
}

//---

CAssetItem::CAssetItem():
	m_flags(0),
	m_nFileSize(0),
	m_oDrawingRectangle(0, 0, 0, 0),
	m_pOwnerDatabase(NULL),
	m_ref(1),
	m_assetIndex(0),
	m_hPreviewDC(0),
	m_pUncachedThumbBmp(0)
{
}

CAssetItem::~CAssetItem()
{
	// empty, call FreeData first
}

void CAssetItem::FreeData()
{
}

uint32 CAssetItem::GetHash() const
{
	return m_hash;
}

void CAssetItem::SetHash(uint32 hash)
{
	m_hash = hash;
}

IAssetItemDatabase* CAssetItem::GetOwnerDatabase() const
{
	return m_pOwnerDatabase;
}

void CAssetItem::SetOwnerDatabase(IAssetItemDatabase* piOwnerDisplayDatabase)
{
	m_pOwnerDatabase = piOwnerDisplayDatabase;
}

const IAssetItem::TAssetDependenciesMap& CAssetItem::GetDependencies() const
{
	return m_dependencies;
}

void CAssetItem::SetFileSize(unsigned __int64 aSize)
{
	m_nFileSize = aSize;
}

void CAssetItem::SetFileExtension(const char* pExt)
{
	m_strExtension = pExt;
}

const char* CAssetItem::GetFileExtension() const
{
	return m_strExtension;
}

unsigned __int64 CAssetItem::GetFileSize() const
{
	return m_nFileSize;
}

void CAssetItem::SetFilename(const char* pName)
{
	m_strFilename = pName;
}

const char* CAssetItem::GetFilename() const
{
	return m_strFilename;
}

void CAssetItem::SetRelativePath(const char* pPath)
{
	m_strRelativePath = pPath;
}

const char* CAssetItem::GetRelativePath() const
{
	return m_strRelativePath;
}

UINT CAssetItem::GetFlags() const
{
	return m_flags;
}

void CAssetItem::SetFlags(UINT aFlags)
{
	m_flags = aFlags;
}

void CAssetItem::SetFlag(EAssetFlags aFlag, bool bSet)
{
	if (bSet)
		m_flags |= aFlag;
	else
		m_flags &= ~aFlag;
}

bool CAssetItem::IsFlagSet(EAssetFlags aFlag) const
{
	return 0 != (m_flags & (UINT)aFlag);
}

void CAssetItem::SetIndex(UINT aIndex)
{
	m_assetIndex = aIndex;
}

UINT CAssetItem::GetIndex() const
{
	return m_assetIndex;
}

bool CAssetItem::GetAssetFieldValue(const char* pFieldName, void* pDest)
{
	if (AssetViewer::IsFieldName(pFieldName, "filename"))
	{
		*(CString*)pDest = m_strFilename;
		return true;
	}
	else if (AssetViewer::IsFieldName(pFieldName, "dccfilename"))
	{
		*(CString*)pDest = m_strDccFilename;
		return true;
	}
	else if (AssetViewer::IsFieldName(pFieldName, "relativepath"))
	{
		*(CString*)pDest = m_strRelativePath;
		return true;
	}
	else if (AssetViewer::IsFieldName(pFieldName, "extension"))
	{
		*(CString*)pDest = m_strExtension;
		return true;
	}
	else if (AssetViewer::IsFieldName(pFieldName, "filesize"))
	{
		*(int*)pDest = m_nFileSize;
		return true;
	}
	else if (AssetViewer::IsFieldName(pFieldName, "fullfilepath"))
	{
		*(CString*)pDest = m_strRelativePath + m_strFilename;
		return true;
	}
	else if (AssetViewer::IsFieldName(pFieldName, "usedinlevel"))
	{
		*(bool*)pDest = (0 != (m_flags & eFlag_UsedInLevel));
		return true;
	}
	else if (AssetViewer::IsFieldName(pFieldName, "tags"))
	{
		CString path = GetRelativePath();
		path += GetFilename();

		CString description;
		CAssetBrowserManager::Instance()->GetAssetDescription(path, description);

		CAssetBrowserManager::StrVector tags;
		CAssetBrowserManager::Instance()->GetTagsForAsset(tags, path);

		CString result = description;

		for (size_t i = 0; i < tags.size(); ++i)
		{
			result += ",";
			result += tags[i];
		}

		*(CString*)pDest = result;

		return true;
	}

	return false;
}

bool CAssetItem::SetAssetFieldValue(const char* pFieldName, void* pSrc)
{
	return false;
}

void CAssetItem::GetDrawingRectangle(CRect& rstDrawingRectangle) const
{
	rstDrawingRectangle = m_oDrawingRectangle;
}

void	CAssetItem::SetDrawingRectangle(const CRect& crstDrawingRectangle)
{
	m_oDrawingRectangle = crstDrawingRectangle;
}

bool CAssetItem::Cache()
{
	CString str;
	CString strUserFolder =	Path::GetUserSandboxFolder();

	str.Format("%s%s/t%u.jpg", strUserFolder.GetBuffer(), AssetBrowser::kThumbnailsRoot, m_hash);

	return m_cachedThumbBmp.Save(str.GetBuffer());
}

bool CAssetItem::ForceCache()
{
	SetFlag(eFlag_Cached, false);
	SetFlag(eFlag_ThumbnailLoaded, false);
	return Cache();
}

bool CAssetItem::LoadThumbnail()
{
	if (IsFlagSet(eFlag_ThumbnailLoaded))
	{
		return true;
	}

	CString str;
	CString strUserFolder =	Path::GetUserSandboxFolder();

	str.Format("%s%s\\t%u.jpg", strUserFolder.GetBuffer(), AssetBrowser::kThumbnailsRoot, m_hash);
		
	if (m_cachedThumbBmp.Load(str.GetBuffer()))
	{
		SetFlag(eFlag_ThumbnailLoaded, true);
		return true;
	}
	
	SetFlag(eFlag_ThumbnailLoaded, false);

	return false;
}

void CAssetItem::UnloadThumbnail()
{
	if (!IsFlagSet(eFlag_ThumbnailLoaded))
	{
		return;
	}

	SetFlag(eFlag_ThumbnailLoaded, false);
	m_cachedThumbBmp.Free();
}

void CAssetItem::OnBeginPreview(const HWND hQuickPreviewWnd, const HDC hMemDC)
{
	m_hPreviewDC = hMemDC;
}

CDialog* CAssetItem::GetCustomPreviewPanelHeader(CWnd* pParentWnd)
{
	return NULL;
}

CDialog* CAssetItem::GetCustomPreviewPanelFooter(CWnd* pParentWnd)
{
	return NULL;
}

void CAssetItem::OnEndPreview()
{
	m_hPreviewDC = 0;
}

void CAssetItem::PreviewRender(
	const HWND hRenderWindow,
	const CRect& rstViewport,
	int aMouseX, int aMouseY,
	int aMouseDeltaX, int aMouseDeltaY,
	int aMouseWheelDelta, UINT aKeyFlags)
{
}

void CAssetItem::OnPreviewRenderKeyEvent(bool bKeyDown, UINT aChar, UINT aKeyFlags)
{
}

void CAssetItem::OnThumbClick(const CPoint& point, UINT aKeyFlags)
{
}

void CAssetItem::OnThumbDblClick(const CPoint& point, UINT aKeyFlags)
{
}

bool CAssetItem::DrawThumbImage(const HDC hDC, const CRect& rRect)
{
	CDC dc;
	CAlphaBitmap* pSrcBmp = (m_flags & eFlag_ThumbnailLoaded) ? &m_cachedThumbBmp : m_pUncachedThumbBmp;

	if (!m_pUncachedThumbBmp)
	{
		return false;
	}

	dc.Attach(hDC);
	dc.StretchBlt(rRect.left, rRect.top, rRect.Width(), rRect.Height(),
		&(pSrcBmp->GetDC()),
		0, 0, pSrcBmp->GetWidth(), pSrcBmp->GetHeight(),
		SRCCOPY);
	dc.Detach();

	return true;
}

bool CAssetItem::HitTest(int nX, int nY) const
{
	return m_oDrawingRectangle.PtInRect(CPoint(nX, nY)) == TRUE;
}

bool CAssetItem::HitTest(const CRect& roTestRect) const
{
	CRect oIntersection;

	return oIntersection.IntersectRect(&m_oDrawingRectangle, &roTestRect) == TRUE;
}

void* CAssetItem::CreateInstanceInViewport(float aX, float aY, float aZ)
{
	return NULL;
}

bool CAssetItem::MoveInstanceInViewport(const void* pDraggedObject, float aNewX, float aNewY, float aNewZ)
{
	return false;
}

void CAssetItem::AbortCreateInstanceInViewport(const void* pDraggedObject)
{
}

void CAssetItem::DrawTextOnReportImage(CAlphaBitmap& rDestBmp) const
{
}

bool CAssetItem::SaveReportImage(const char* pFilePath) const
{
	return false;
}

bool CAssetItem::SaveReportText(const char* pFilePath) const
{
	return false;
}

void CAssetItem::ToXML(XmlNodeRef& node) const
{
}

void CAssetItem::FromXML(const XmlNodeRef& node)
{
}

HRESULT CAssetItem::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == __uuidof(IAssetItem)/* && m_pIntegrator*/)
	{
		*ppvObj = this;
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG CAssetItem::AddRef()
{
	return ++m_ref;
};

ULONG CAssetItem::Release()
{
	if ((--m_ref) == 0)
	{
		FreeData();
		delete this;
		return 0;
	}
	else
	{
		return m_ref;
	}
}
