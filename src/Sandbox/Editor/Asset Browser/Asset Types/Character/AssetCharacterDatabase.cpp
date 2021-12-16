////////////////////////////////////////////////////////////////////////////
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2013
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AssetCharacterDatabase.h"
#include "AssetCharacterItem.h"
#include "Include/IAssetViewer.h"
#include "StringUtils.h"
#include "Util/IndexedFiles.h"
#include "CharacterAssetDbFilterDlg.h"

REGISTER_CLASS_DESC(CAssetCharacterDatabase);

CAssetCharacterDatabase::CAssetCharacterDatabase() : CAssetItemDatabase()
{
	// add fields
	static const int kFilenameColWidth = 150;
	static const int kFileSizeColWidth = 50;
	static const int kRelativePathColWidth = 50;
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
	static const int kLoadedInLevelColWidth = 40;
	static const int kDccFilenameColWidth = 50;
	static const int kTagsColWidth = 60;

	m_assetFields.push_back(SAssetField("filename", "Filename", SAssetField::eType_String, kFilenameColWidth));
	m_assetFields.push_back(SAssetField("filesize", "Filesize (Bytes)", SAssetField::eType_Int32, kFileSizeColWidth));
	m_assetFields.push_back(SAssetField("relativepath", "Path", SAssetField::eType_String, kRelativePathColWidth));
	m_assetFields.push_back(SAssetField("nref", "Refs", SAssetField::eType_Int32, kRefCountColWidth));
	m_assetFields.push_back(SAssetField("texturesize", "Texture size (Bytes)", SAssetField::eType_Int32, kTextureSizeColWidth));
	m_assetFields.push_back(SAssetField("trianglecount", "Triangles", SAssetField::eType_Int32, kTriangleCountColWidth));
	m_assetFields.push_back(SAssetField("vertexcount", "Vertices", SAssetField::eType_Int32, kVertexCountColWidth));
	m_assetFields.push_back(SAssetField("lodcount", "LODs", SAssetField::eType_Int32, kLodCountColWidth));
	m_assetFields.push_back(SAssetField("submeshcount", "Submeshes", SAssetField::eType_Int32, kSubmeshCountColWidth));
	m_assetFields.push_back(SAssetField("phytricount", "Physics tris", SAssetField::eType_Int32, kPhysicsTriCountColWidth));
	m_assetFields.push_back(SAssetField("physize", "Physics size (Bytes)", SAssetField::eType_Int32, kPhysicsSizeColWidth));
	m_assetFields.push_back(SAssetField("lodstricount", "LODs tris", SAssetField::eType_String, kLODsTrisColWidth));
	m_assetFields.push_back(SAssetField("splitlods", "Split LODs", SAssetField::eType_Bool, kSplitLODsColWidth));
	m_assetFields.push_back(SAssetField("bbox_x", "Size(X)", SAssetField::eType_Float, kBBoxColWidth));
	m_assetFields.push_back(SAssetField("bbox_y", "Size(Y)", SAssetField::eType_Float, kBBoxColWidth));
	m_assetFields.push_back(SAssetField("bbox_z", "Size(Z)", SAssetField::eType_Float, kBBoxColWidth));
	m_assetFields.push_back(SAssetField("mtlcount", "Materials", SAssetField::eType_Int32, kBBoxColWidth));
	m_assetFields.push_back(SAssetField("usedinlevel", "Used in level", SAssetField::eType_Bool, kUsedInLevelColWidth));
	m_assetFields.push_back(SAssetField("loadedinlevel", "Loaded in level", SAssetField::eType_Bool, kLoadedInLevelColWidth));
	m_assetFields.push_back(SAssetField("dccfilename", "DCC Filename", SAssetField::eType_String, kDccFilenameColWidth));
	m_assetFields.push_back(SAssetField("tags", "Tags", SAssetField::eType_String, kTagsColWidth));
}

CAssetCharacterDatabase::~CAssetCharacterDatabase()
{
	// empty call FreeData() first
}

HRESULT STDMETHODCALLTYPE CAssetCharacterDatabase::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == __uuidof(IAssetItemDatabase))
	{
		*ppvObj = this;
		return S_OK;
	}

	return E_NOINTERFACE ;
}

ULONG STDMETHODCALLTYPE CAssetCharacterDatabase::AddRef()
{
	return ++m_ref;
};

ULONG STDMETHODCALLTYPE CAssetCharacterDatabase::Release()
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

const char* CAssetCharacterDatabase::GetSupportedExtensions() const
{
	return "chr,skin,cdf";
};

const char* CAssetCharacterDatabase::GetDatabaseName() const
{
	return "Characters";
}

void CAssetCharacterDatabase::Refresh()
{
	FreeData();

	CFileUtil::FileArray	cFiles;
	int nTotalFiles = 0;
	int nCurrentFile = 0;
	CString strPathOnly, strFileNameOnly;

	// search for models
	std::vector<CString> tags;
	tags.push_back("chr");
	CIndexedFiles::GetDB().GetFilesWithTags(cFiles, tags);

	nTotalFiles = cFiles.size();

	for (nCurrentFile = 0; nCurrentFile < nTotalFiles; ++nCurrentFile)
	{
		CFileUtil::FileDesc& rstFileDescriptor = cFiles[nCurrentFile];
		CString strIntermediateFilename = rstFileDescriptor.filename.GetBuffer();
		strIntermediateFilename.MakeLower();
		Path::ConvertBackSlashToSlash(strIntermediateFilename);
		CString strOutputModelName = strIntermediateFilename;

		if (Path::GetExt(strOutputModelName) != "chr")
		{
			continue;
		}

		// No need to load files already loaded by the engine...
		if (m_assets.find(strOutputModelName.GetBuffer()) != m_assets.end())
		{
			continue;
		}

		CString strExtension = Path::GetExt(strOutputModelName.GetBuffer());
		const char* szModelName = strOutputModelName.GetBuffer();

		if (!szModelName)
		{
			continue;
		}

		strFileNameOnly = Path::GetFile(strOutputModelName);
		strPathOnly = Path::GetPath(strOutputModelName);

		CAssetCharacterItem* poModelDatabaseItem = new CAssetCharacterItem();

		poModelDatabaseItem->SetFileSize(rstFileDescriptor.size);
		poModelDatabaseItem->SetFilename(strFileNameOnly);
		poModelDatabaseItem->SetRelativePath(strPathOnly);
		poModelDatabaseItem->SetOwnerDatabase(this);
		poModelDatabaseItem->SetFileExtension(strExtension);
		poModelDatabaseItem->SetFlag(IAssetItem::eFlag_Visible, true);
		poModelDatabaseItem->SetHash(AssetBrowser::HashStringSbdm(strOutputModelName));
		poModelDatabaseItem->CheckIfItsLod();
		m_assets[strOutputModelName] = poModelDatabaseItem;
	}
}

void CAssetCharacterDatabase::CacheFieldsInfoForAlreadyLoadedAssets()
{
	ISystem *pSystem = GetISystem();
	I3DEngine *p3DEngine = pSystem->GetI3DEngine();

	// iterate through all IStatObj
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
				static_cast<CAssetCharacterItem*>(itr->second)->CacheFieldsInfoForLoadedStatObj(pObjects[nCurObj]);
			}
		}
	}
}

const char* CAssetCharacterDatabase::GetTransactionFilename() const
{
	return "characterAssetTransactions.xml";
}

CDialog* CAssetCharacterDatabase::CreateDbFilterDialog(CWnd* pParent, IAssetViewer* pViewerCtrl)
{
	CCharacterAssetDbFilterDlg* pDlg = new CCharacterAssetDbFilterDlg();

	pDlg->SetAssetViewer(pViewerCtrl);
	pDlg->Create(IDD_ASSET_BROWSER_MODEL_DB_FILTER, pParent);
	pDlg->ShowWindow(SW_SHOW);

	return pDlg;
}

void CAssetCharacterDatabase::UpdateDbFilterDialogUI(CDialog* pDlg)
{
	CCharacterAssetDbFilterDlg* pFilterDlg = (CCharacterAssetDbFilterDlg*)pDlg;

	pFilterDlg->UpdateFilterUI();
}

void CAssetCharacterDatabase::PrecacheFieldsInfoFromFileDB(const XmlNodeRef& db)
{
	assert(db->isTag(GetDatabaseName()));

	for (int i = 0; i < db->getChildCount(); ++i)
	{
		XmlNodeRef entry = db->getChild(i);
		CString fileName = entry->getAttr("fileName");
		TFilenameAssetMap::iterator assetIt = m_assets.find(fileName);
		bool bAssetFound = m_assets.end() != assetIt;

		if (bAssetFound)
		{
			assetIt->second->FromXML(entry);
		}
	}
}