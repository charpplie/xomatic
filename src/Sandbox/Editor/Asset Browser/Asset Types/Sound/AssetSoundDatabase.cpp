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
#include "IRenderer.h"
#include "IMusicSystem.h"
#include "ImageExtensionHelper.h"
#include "Include/IAssetViewer.h"
#include "StringUtils.h"
#include "Util/IndexedFiles.h"
#include "SoundAssetDbFilterDlg.h"

REGISTER_CLASS_DESC(CAssetSoundDatabase);

CAssetSoundDatabase::CAssetSoundDatabase() : CAssetItemDatabase()
{
	// add fields
	static const int kFilenameColWidth = 150;
	static const int kDccFilenameColWidth = 50;
	static const int kFileSizeColWidth = 50;
	static const int kRelativePathColWidth = 50;
	static const int kLengthColWidth = 50;
	static const int kUsedInLevelColWidth = 40;
	static const int kLoadedInLevelColWidth = 40;
	static const int kTagsColWidth = 60;

	m_assetFields.push_back(SAssetField("filename", "Filename", SAssetField::eType_String, kFilenameColWidth));
	m_assetFields.push_back(SAssetField("relativepath", "Path", SAssetField::eType_String, kRelativePathColWidth));
	m_assetFields.push_back(SAssetField("length", "Length (Msec)", SAssetField::eType_Int32, kLengthColWidth));
	m_assetFields.push_back(SAssetField("loopsound", "Looping", SAssetField::eType_Bool, kLengthColWidth));
	m_assetFields.push_back(SAssetField("usedinlevel", "Used in level", SAssetField::eType_Bool, kUsedInLevelColWidth));
	m_assetFields.push_back(SAssetField("loadedinlevel", "Loaded in level", SAssetField::eType_Bool, kLoadedInLevelColWidth));
	m_assetFields.push_back(SAssetField("dccfilename", "DCC Filename", SAssetField::eType_String, kDccFilenameColWidth));
	m_assetFields.push_back(SAssetField("tags", "Tags", SAssetField::eType_String, kTagsColWidth));
}

CAssetSoundDatabase::~CAssetSoundDatabase()
{
	// empty, call FreeData first
}

void CAssetSoundDatabase::PrecacheFieldsInfoFromFileDB(const XmlNodeRef& db)
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
HRESULT STDMETHODCALLTYPE CAssetSoundDatabase::QueryInterface(const IID& riid, void** ppvObj)
{
	if (riid == __uuidof(IAssetItemDatabase))
	{
		*ppvObj = this;
		return S_OK;
	}

	return E_NOINTERFACE ;
}

ULONG STDMETHODCALLTYPE CAssetSoundDatabase::AddRef()
{
	return ++m_ref;
};

ULONG STDMETHODCALLTYPE CAssetSoundDatabase::Release()
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

void CAssetSoundDatabase::FreeData()
{
	CAssetItemDatabase::FreeData();
}

const char* CAssetSoundDatabase::GetSupportedExtensions() const
{
	return "fdp,fsb";
};

const char* CAssetSoundDatabase::GetTransactionFilename() const
{
	return "soundAssetTransactions.xml";
}

CDialog* CAssetSoundDatabase::CreateDbFilterDialog(CWnd* pParent, IAssetViewer* pViewerCtrl)
{
	CSoundAssetDbFilterDlg* pDlg = new CSoundAssetDbFilterDlg();

	pDlg->SetAssetViewer(pViewerCtrl);
	pDlg->Create(IDD_ASSET_BROWSER_SOUND_DB_FILTER, pParent);
	pDlg->ShowWindow(SW_SHOW);

	return pDlg;
}

void CAssetSoundDatabase::UpdateDbFilterDialogUI(CDialog* pDlg)
{
	CSoundAssetDbFilterDlg* pFilterDlg = (CSoundAssetDbFilterDlg*)pDlg;

	pFilterDlg->UpdateFilterUI();
}

const char* CAssetSoundDatabase::GetDatabaseName() const
{
	return "Sounds";
}

void CAssetSoundDatabase::CollectCachedEventgroup(XmlNodeRef& gr, const CString& Block, const CString& Path, int level)
{
	const char* pNameGr = "";
	XmlNodeRef name = gr->findChild("name");

	if (name)
	{
		pNameGr = name->getContent();
	}

	CString NewPath = Path + pNameGr;

	for (size_t j = 0, jCount = gr->getChildCount(); j < jCount; ++j)
	{
		XmlNodeRef ev = gr->getChild(j);

		if (!strcmp(ev->getTag(), "event"))
		{
			const char* pNameEv = "";
			XmlNodeRef name = ev->findChild("name");

			if (name)
			{
				pNameEv = name->getContent();
			}

			CAssetSoundItem*			poSoundDatabaseItem = NULL;
			poSoundDatabaseItem = new CAssetSoundItem();

			if (!poSoundDatabaseItem)
			{
				return;
			}

			CString fpath = Block;
			fpath += ":";
			fpath += NewPath;
			fpath += ":";

			poSoundDatabaseItem->SetFileSize(0);
			poSoundDatabaseItem->SetFilename(pNameEv);
			poSoundDatabaseItem->SetRelativePath(fpath);
			poSoundDatabaseItem->SetOwnerDatabase(this);
			poSoundDatabaseItem->SetFileExtension("fsb");
			poSoundDatabaseItem->SetFlag(IAssetItem::eFlag_Visible, true);
			fpath += pNameEv;
			poSoundDatabaseItem->SetHash(AssetBrowser::HashStringSbdm(fpath));
			m_assets[fpath] = poSoundDatabaseItem;
		}
		else if (!strcmp(ev->getTag(), "eventgroup"))
		{
			CollectCachedEventgroup(ev, Block, NewPath + "/", level + 1);
		}
	}
}

void CAssetSoundDatabase::Refresh()
{
	FreeData();

	const char* szSoundName = NULL;
	CString strExtension;
	CString strFilename;
	CFileUtil::FileArray cFiles;
	int nTotalFiles = 0;
	int nCurrentFile = 0;
	CString strIntermediateFilename;
	CString strOutputSoundName, strPathOnly, strFileNameOnly;
	CCryFile file;
	CAssetSoundItem* poSoundDatabaseItem = NULL;

	// search for sound files
	std::vector<CString> tags;

	tags.push_back("fdp");
	CIndexedFiles::GetDB().GetFilesWithTags(cFiles, tags);

	nTotalFiles = cFiles.size();

	for (nCurrentFile = 0; nCurrentFile < nTotalFiles; ++nCurrentFile)
	{
		CFileUtil::FileDesc& rstFileDescriptor = cFiles[nCurrentFile];

		// if not a real .fdp file
		if (Path::GetExt(rstFileDescriptor.filename) != "fdp")
		{
			continue;
		}

		strIntermediateFilename = rstFileDescriptor.filename.GetBuffer();
		strIntermediateFilename.MakeLower();
		strOutputSoundName = strIntermediateFilename;
		Path::ConvertBackSlashToSlash(strOutputSoundName);

		XmlNodeRef root = XmlHelpers::LoadXmlFromFile(strIntermediateFilename);

		char path[_MAX_PATH];
		strcpy(path, strIntermediateFilename);
		char* ch;

		while (ch = strchr(path, '\\'))
		{
			*ch = '/';
		}

		if (ch = strrchr(path, '/'))
		{
			*ch = 0;
		}

		if (root)
		{
			const char* pName = "";

			XmlNodeRef name = root->findChild("name");

			if (name)
			{
				pName = name->getContent();
			}

			for (size_t i = 0, iCount = root->getChildCount(); i < iCount; ++i)
			{
				XmlNodeRef gr = root->getChild(i);

				if (!strcmp(gr->getTag(), "eventgroup"))
				{
					CollectCachedEventgroup(gr, path, "", 1);
				}
			}
		}
	}
}
