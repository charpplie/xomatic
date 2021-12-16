////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2010.
// -------------------------------------------------------------------------
//  File name:	AssetMaterialDatabase.cpp
//  Version:	v1.00
//  Created:	12/07/2010 by Nicusor Nedelcu
//  Description:	Implements AssetMaterialDatabase.h
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AssetMaterialDatabase.h"
#include "AssetMaterialItem.h"
#include "IMaterial.h"
#include "IRenderer.h"
#include "ImageExtensionHelper.h"
#include "Include/IAssetViewer.h"
#include "StringUtils.h"
#include "Util/IndexedFiles.h"
#include "MaterialAssetDbFilterDlg.h"

REGISTER_CLASS_DESC(CAssetMaterialDatabase);

CAssetMaterialDatabase::CAssetMaterialDatabase() : CAssetItemDatabase()
{
	// add fields
	static const int kFilenameColWidth = 150;
	static const int kDccFilenameColWidth = 50;
	static const int kFileSizeColWidth = 50;
	static const int kRelativePathColWidth = 50;
	static const int kUsedInLevelColWidth = 40;
	static const int kLoadedInLevelColWidth = 40;
	static const int kTagsColWidth = 60;

	m_assetFields.push_back(SAssetField("filename", "Filename", SAssetField::eType_String, kFilenameColWidth));
	m_assetFields.push_back(SAssetField("relativepath", "Path", SAssetField::eType_String, kRelativePathColWidth));
	m_assetFields.push_back(SAssetField("usedinlevel", "Used in level", SAssetField::eType_Bool, kUsedInLevelColWidth));
	m_assetFields.push_back(SAssetField("loadedinlevel", "Loaded in level", SAssetField::eType_Bool, kLoadedInLevelColWidth));
	m_assetFields.push_back(SAssetField("dccfilename", "DCC Filename", SAssetField::eType_String, kDccFilenameColWidth));
	m_assetFields.push_back(SAssetField("tags", "Tags", SAssetField::eType_String, kTagsColWidth));
}

CAssetMaterialDatabase::~CAssetMaterialDatabase()
{
	// empty, call FreeData first
}

void CAssetMaterialDatabase::PrecacheFieldsInfoFromFileDB(const XmlNodeRef& db)
{
	assert(db->isTag(GetDatabaseName()));
	int foundAssets = 0;

	for (int i = 0; i < db->getChildCount(); ++i)
	{
		XmlNodeRef entry = db->getChild(i);
		const char* fileName = entry->getAttr("fileName");
		TFilenameAssetMap::iterator assetIt = m_assets.find(fileName);
		bool bAssetFound = m_assets.end() != assetIt;
		
		if (bAssetFound)
		{
			assetIt->second->FromXML(entry);
			foundAssets++;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE CAssetMaterialDatabase::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == __uuidof(IAssetItemDatabase))
	{
		*ppvObj = this;
		return S_OK;
	}

	return E_NOINTERFACE ;
}

ULONG STDMETHODCALLTYPE CAssetMaterialDatabase::AddRef()
{
	return ++m_ref;
};

ULONG STDMETHODCALLTYPE CAssetMaterialDatabase::Release()
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

void CAssetMaterialDatabase::FreeData()
{
	CAssetItemDatabase::FreeData();
}

const char* CAssetMaterialDatabase::GetSupportedExtensions() const
{
	return "mtl";
};

const char* CAssetMaterialDatabase::GetTransactionFilename() const
{
	return "materialAssetTransactions.xml";
}

CDialog* CAssetMaterialDatabase::CreateDbFilterDialog(CWnd* pParent, IAssetViewer* pViewerCtrl)
{
	CMaterialAssetDbFilterDlg* pDlg = new CMaterialAssetDbFilterDlg();

	pDlg->SetAssetViewer(pViewerCtrl);
	pDlg->Create(IDD_ASSET_BROWSER_MATERIAL_DB_FILTER, pParent);
	pDlg->ShowWindow(SW_SHOW);

	return pDlg;
}

void CAssetMaterialDatabase::UpdateDbFilterDialogUI(CDialog* pDlg)
{
	CMaterialAssetDbFilterDlg* pFilterDlg = (CMaterialAssetDbFilterDlg*)pDlg;

	pFilterDlg->UpdateFilterUI();
}

const char* CAssetMaterialDatabase::GetDatabaseName() const
{
	return "Materials";
}

void CAssetMaterialDatabase::Refresh()
{
	FreeData();

	CString strExtension;
	CFileUtil::FileArray cFiles;
	int nTotalFiles = 0;
	int nCurrentFile = 0;
	CString strIntermediateFilename;
	CString strOutputMaterialName, strPathOnly, strFileNameOnly;
	CAssetMaterialItem* poMaterialDatabaseItem = NULL;

	// search for Material files
	std::vector<CString> tags;

	tags.push_back("mtl");
	CIndexedFiles::GetDB().GetFilesWithTags(cFiles, tags);

	nTotalFiles = cFiles.size();

	for (nCurrentFile = 0; nCurrentFile < nTotalFiles; ++nCurrentFile)
	{
		CFileUtil::FileDesc& rstFileDescriptor = cFiles[nCurrentFile];

		// if not a real .mtl file
		if (Path::GetExt(rstFileDescriptor.filename) != "mtl")
		{
			continue;
		}

		strIntermediateFilename = rstFileDescriptor.filename.GetBuffer();
		strIntermediateFilename.MakeLower();
		strOutputMaterialName = strIntermediateFilename;
		Path::ConvertBackSlashToSlash(strOutputMaterialName);
		strFileNameOnly = Path::GetFile(strOutputMaterialName);
		strPathOnly = Path::GetPath(strOutputMaterialName);

		poMaterialDatabaseItem = new CAssetMaterialItem();

		if (!poMaterialDatabaseItem)
		{
			return;
		}

		poMaterialDatabaseItem->SetFileSize(rstFileDescriptor.size);
		poMaterialDatabaseItem->SetFilename(strFileNameOnly.GetBuffer());
		poMaterialDatabaseItem->SetRelativePath(strPathOnly.GetBuffer());
		poMaterialDatabaseItem->SetOwnerDatabase(this);
		poMaterialDatabaseItem->SetFileExtension(strExtension.GetBuffer());
		poMaterialDatabaseItem->SetFlag(IAssetItem::eFlag_Visible, true);
		poMaterialDatabaseItem->SetHash(AssetBrowser::HashStringSbdm(strOutputMaterialName.GetBuffer()));
		m_assets[strOutputMaterialName.GetBuffer()] = poMaterialDatabaseItem;
	}
}