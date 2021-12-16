#include "StdAfx.h"
#include "InternalCommon.h"
#include "DataUpgrade.h"

#include "SegmentedWorldManager.h"
#include "WorldData.h"
#include "ISWDataPersistence.h"




using namespace sw;
SW_NAMESPACE_BEGIN();


// !!! COPY THE FOLLOWING WARNING TO EVERY FUNCTIONS THAT ARE USED IN UPGRADE PIPES !!!
// WARNING: This function is used in DataUpgrade, don't forget to "Copy-on-change" to DataUpgrade module.

//////////////////////////////////////////////////////////////////////////
// Version 0 To Version 1
//////////////////////////////////////////////////////////////////////////
class CUpgradePipe_Version0ToVersion1 : public CBaseUpgradePipe
{
	enum{WDB_XMLCOUNT_V0 = 4, WDB_XMLCOUNT_V1 = 5};
	static const char* s_szWDBNameV0[WDB_XMLCOUNT_V0];
	static const char* s_szWDBFileNameV1[WDB_XMLCOUNT_V1];
public:
	static const char* s_szSegsDirPath;
public:
	CUpgradePipe_Version0ToVersion1()
		:CBaseUpgradePipe(0,1)
	{
	}

	virtual CDataUpgrade::TVersion TryGetVersionFromLevel( const char* pszLevelPath );
	virtual CDataUpgrade::EUpgradeResult DoUpgradeGlobals( const CString& strLevelPath, TTransactionContext& transaction);
	virtual CDataUpgrade::EUpgradeResult DoUpgradeSegment( const CString& strSegPath, TTransactionContext& transaction );


	virtual CDataUpgrade::EUpgradeResult UpdateGlobalVersionNumber( const CString& strLevelPath, TTransactionContext& transaction )
	{
		return DoUpdateGlobalVersionNumber(strLevelPath, m_nToVersion, transaction);
	}

	virtual CDataUpgrade::EUpgradeResult UpdateSegVersionNumber( const CString& strSegPath, TTransactionContext& transaction ) 
	{
		return DoUpdateSegVersionNumber(strSegPath, m_nToVersion, transaction);
	}

	// This function can be reused by other upgradePipe
	static CDataUpgrade::EUpgradeResult DoUpdateGlobalVersionNumber( const CString& strLevelPath, CDataUpgrade::TVersion nVer, TTransactionContext& transaction );
	static CDataUpgrade::EUpgradeResult DoUpdateSegVersionNumber( const CString& strSegPath, CDataUpgrade::TVersion nVer, TTransactionContext &transaction );
protected:
	// keep the old implementation
	bool WorldData_MemBlockToXml( CMemoryBlock& mem, CXmlArchive &xmlAr )
	{
		CMemFile mf((BYTE *) mem.GetBuffer(), mem.GetSize());

		CArchive ar(&mf, CArchive::load);

		CString str;
		ar >> str;

		xmlAr.pNamedData->Serialize( ar );

		xmlAr.root = XmlHelpers::LoadXmlFromBuffer( str, str.GetLength() );
		if (!xmlAr.root)
			return false;
		return true;
	}

	virtual const char* GetSegsDirPath() const { return s_szSegsDirPath; }

}static _upgrader_0To1;

const char* CUpgradePipe_Version0ToVersion1::s_szSegsDirPath = "seg\\";
const char* CUpgradePipe_Version0ToVersion1::s_szWDBNameV0[WDB_XMLCOUNT_V0] = {
	"Level",
	"GDTerrainLayers",
	"GDVegetation",
	"GDTimeOfDay"
};
const char* CUpgradePipe_Version0ToVersion1::s_szWDBFileNameV1[WDB_XMLCOUNT_V1] = {
	"Level.xml",
	"GDTerrainLayers.xml",
	"GDVegetation.xml",
	"GDTimeOfDay.xml",
	"LevelMisc.dat"
};


CDataUpgrade::EUpgradeResult CUpgradePipe_Version0ToVersion1::DoUpgradeGlobals( const CString& strLevelPath, TTransactionContext& transaction )
{
	//===== upgrade WorldData ======
	{
		COMPILE_TIME_ASSERT(WDB_XMLCOUNT_V0 == 4);
		CXmlArchive xmlArchives[WDB_XMLCOUNT_V0];
		for (EnumIteratorT<EWDBType> ii(0); ii < WDB_XMLCOUNT_V0; ++ii)
		{
			CString strFile = transaction.GetRemappedFileName(strLevelPath + s_szWDBNameV0[ii]);
			//transaction.lstFilesToBeDeleted.push_back(strFile);

			CMemoryBlock memBlock, memUncompressed;
			if (!LoadMemBlockFromFile( strFile, memBlock))
				return CDataUpgrade::UR_FAILED;

			memBlock.Uncompress(memUncompressed);
			if (!WorldData_MemBlockToXml(memUncompressed,xmlArchives[ii]))
				return CDataUpgrade::UR_FAILED;

			string str = xmlArchives[ii].root->getXML();
			CString strFileNew = transaction.TryRemapFileName(strFile, strLevelPath + s_szWDBFileNameV1[ii]);
			if (!SaveRawDataToFile( (const char*)strFileNew, (void*)str.c_str(), str.size()))
				return CDataUpgrade::UR_FAILED;

			//transaction.lstFilesAdded.push_back(strFile);
			transaction.ConfirmRemappedFileName();
		}

		// then save NamedData of WDB_LEVELGENERAL
		{
			CMemFile mfile;
			CArchive ar(&mfile, CArchive::store);
			xmlArchives[WDB_LEVELGENERAL].pNamedData->Serialize(ar);
			ar.Flush();
			CMemoryBlock mem; mem.Allocate(mfile.GetLength());
			mfile.SeekToBegin();
			mfile.Read(mem.GetBuffer(), mfile.GetLength());
			CString strFile = transaction.TryRemapFileName( CString(), strLevelPath + s_szWDBFileNameV1[WDB_LEVELNAMEDDATA] );
			if (!SaveRawDataToFile(strFile, mem.GetBuffer(), mem.GetSize()))
				return CDataUpgrade::UR_FAILED;

			//transaction.lstFilesAdded.push_back(strFile);
			//transaction.lstRenamings.push_back(std::make_pair(CString(),strFile));
			transaction.ConfirmRemappedFileName();
		}
	}
	//======== upgrade ObjectData ======
	{
		WIN32_FIND_DATA FindFileData;
		HANDLE hFind = FindFirstFile( strLevelPath+"{*}",&FindFileData );
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do{
				DWORD attr = FindFileData.dwFileAttributes;
				if (attr & FILE_ATTRIBUTE_DIRECTORY)
					continue;

				GUID guidObj = GuidUtil::FromString(FindFileData.cFileName);
				if (*(size_t*)&guidObj.Data4[4] == 0)	// the file name may not be a correct guid
					continue;

				CString strOldFile = transaction.GetRemappedFileName(strLevelPath + FindFileData.cFileName);
				//transaction.lstFilesToBeDeleted.push_back(strOldFile);

				CMemoryBlock mem,memUncompressed;
				if (!LoadMemBlockFromFile(strOldFile, mem))
					return CDataUpgrade::UR_FAILED;

				mem.Uncompress(memUncompressed);

				CString strNewFile = transaction.TryRemapFileName(strOldFile,strLevelPath + SW_ExternalObjectDirPath + FindFileData.cFileName);
				if (!SaveRawDataToFile(strNewFile, memUncompressed))
					return CDataUpgrade::UR_FAILED;

				//transaction.lstFilesAdded.push_back(strNewFile);
				transaction.ConfirmRemappedFileName();
			}
			while(FindNextFile(hFind,&FindFileData));
		}
	}

	return CDataUpgrade::UR_SUCCEEDED;
}

CDataUpgrade::TVersion CUpgradePipe_Version0ToVersion1::TryGetVersionFromLevel( const char* pszLevelPath )
{
	CString strLevelPath = Path::AddPathSlash(CString(pszLevelPath));
	if (!CSWManager::IsSWLevel(strLevelPath))
		return SWDStructVer_Invalid;

	// Try get Version from version1 level data structure

	bool bVersion1Positive = true, bVersion0Positive = true;
	for (EnumIteratorT<EWDBType> ii(0); ii < WDB_XMLCOUNT_V1; ++ii)
		bVersion1Positive = bVersion1Positive && CFileUtil::FileExists( strLevelPath + s_szWDBFileNameV1[ii] );
	for (EnumIteratorT<EWDBType> ii(0); ii < WDB_XMLCOUNT_V0; ++ii)
		bVersion0Positive = bVersion0Positive && CFileUtil::FileExists( strLevelPath + s_szWDBNameV0[ii] );

	if (bVersion1Positive && !bVersion0Positive)
	{
		CDataUpgrade::TVersion verLevel;
		XmlNodeRef xmlLevel = XmlHelpers::LoadXmlFromFile(strLevelPath + s_szWDBFileNameV1[WDB_LEVELGENERAL]);
		if (xmlLevel && xmlLevel->isTag(g_szLevelRootTag) && xmlLevel->getAttr(g_szSWDStructVerAttr,verLevel))
			return verLevel;

		assert(0);
	}
	// Try get version from version0 level
	else if (bVersion0Positive && !bVersion1Positive)
		return 0;

	return SWDStructVer_Invalid;	// broken level
}

CDataUpgrade::EUpgradeResult CUpgradePipe_Version0ToVersion1::DoUpgradeSegment( const CString& strSegPath, TTransactionContext& transaction )
{
	WIN32_FIND_DATA ffdSegLayer;
	HANDLE hFindSegLayer = FindFirstFile( strSegPath + "*" SW_LayerFileExtension, &ffdSegLayer);
	if (hFindSegLayer != INVALID_HANDLE_VALUE)
	do 
	{
		CString strFileLayer = transaction.GetRemappedFileName(strSegPath + ffdSegLayer.cFileName);
		//transaction.lstFilesToBeDeleted.push_back(strFileLayer);

		CMemoryBlock memUncompressed;
		CMemoryBlock mem;
		if (!LoadMemBlockFromFile(strFileLayer,mem))
			return CDataUpgrade::UR_FAILED;

		if (mem.GetUncompressedSize() > 0)	// may be it's even older data
			mem.Uncompress(memUncompressed);
		else
			memUncompressed.Attach(mem.GetBuffer(), mem.GetSize());	// fake it

		CString strFileLayerTmp = transaction.TryRemapFileName(strFileLayer, strFileLayer + SW_TempNewFileExtension, true);
		if (!SaveRawDataToFile(strFileLayerTmp, memUncompressed))
			return CDataUpgrade::UR_FAILED;

		//transaction.lstFilesAdded.push_back(strFileLayerTmp);
		//transaction.lstTempRenamed.push_back(std::make_pair(strFileLayerTmp,strFileLayer));
		transaction.ConfirmRemappedFileName();
	}
	while (FindNextFile(hFindSegLayer, &ffdSegLayer));


	return CDataUpgrade::UR_SUCCEEDED;
}


CDataUpgrade::EUpgradeResult CUpgradePipe_Version0ToVersion1::DoUpdateGlobalVersionNumber( const CString& strLevelPath, CDataUpgrade::TVersion nVer, TTransactionContext& transaction )
{
	// TODO: For any place that directly using a filename is wrong, the target file may have a different name because of the temporary files used
	//		for Rollback.

	// ===== change version number =====
	CString strLevelFile = transaction.GetRemappedFileName( strLevelPath + s_szWDBFileNameV1[WDB_LEVELGENERAL]);
	XmlNodeRef xmlLevel = XmlHelpers::LoadXmlFromFile( strLevelFile );
	if (!xmlLevel || !xmlLevel->isTag(g_szLevelRootTag))
		return CDataUpgrade::UR_FAILED;

	xmlLevel->setAttr(g_szSWDStructVerAttr, nVer);

	CString strLevelFileTmp = transaction.TryRemapFileName(strLevelFile,strLevelFile + SW_TempNewFileExtension,true);
	if (!XmlHelpers::SaveXmlNode(xmlLevel,strLevelFileTmp))
		return CDataUpgrade::UR_FAILED;

	transaction.ConfirmRemappedFileName();

	return CDataUpgrade::UR_SUCCEEDED;
}

CDataUpgrade::EUpgradeResult CUpgradePipe_Version0ToVersion1::DoUpdateSegVersionNumber( const CString& strSegPath, CDataUpgrade::TVersion nVer, TTransactionContext &transaction )
{
	CString strFileSegMain = strSegPath + SW_SegmentMainFile;
	CFile fTryRW;
	if (!CFileUtil::FileExists(strFileSegMain) || !fTryRW.Open(strFileSegMain,CFile::modeReadWrite)) // not writable
		return CDataUpgrade::UR_FAILED;
	fTryRW.Close();
	//transaction.lstFilesToBeDeleted.push_back(strFileSegMain);
	strFileSegMain = transaction.GetRemappedFileName(strFileSegMain);

	XmlNodeRef xmlSeg;
	uint64 cbFileSegMain = 0;
	if (!CFileUtil::GetDiskFileSize(strFileSegMain,cbFileSegMain) || cbFileSegMain <= 0) // empty file
		xmlSeg = XmlHelpers::CreateXmlNode(g_szSegmentMainTag);
	else
		xmlSeg = XmlHelpers::LoadXmlFromFile(strFileSegMain);

	if (!(xmlSeg && xmlSeg->isTag(g_szSegmentMainTag)))
		return CDataUpgrade::UR_FAILED;

	xmlSeg->setAttr(g_szSWDStructVerAttr,nVer);
	CString strFileSegMainTmp = transaction.TryRemapFileName(strFileSegMain, strFileSegMain + SW_TempNewFileExtension, true);
	if (!XmlHelpers::SaveXmlNode(xmlSeg, strFileSegMainTmp))
		return CDataUpgrade::UR_FAILED;

	//transaction.lstFilesAdded.push_back(strFileSegMainTmp);
	//transaction.lstTempRenamed.push_back(std::make_pair(strFileSegMainTmp,strFileSegMain));
	transaction.ConfirmRemappedFileName();

	return CDataUpgrade::UR_SUCCEEDED;
}

//////////////////////////////////////////////////////////////////////////
// Version 1 to version 2
//////////////////////////////////////////////////////////////////////////
class CUpgradePipe_Version1ToVersion2 : public CBaseUpgradePipe
{
public:
	CUpgradePipe_Version1ToVersion2()
		:CBaseUpgradePipe(1,2)
	{
	}

	virtual CDataUpgrade::EUpgradeResult DoUpgradeGlobals( const CString& strLevelPath, TTransactionContext& transaction ) 
	{
		//====== upgrade lookup tables =========
		CString strFileName = transaction.GetRemappedFileName(strLevelPath + "segmentinfo.xml");
		//transaction.lstFilesToBeDeleted.push_back(strFileName);

		XmlNodeRef xmlSegs = XmlHelpers::LoadXmlFromFile(strFileName);

		if (!xmlSegs || !xmlSegs->isTag("Segments"))
			return CDataUpgrade::UR_FAILED;

		TRectI rcWorldBound(0,0,0,0);
		for (int ii=0; ii < xmlSegs->getChildCount(); ++ii)
		{
			Vec2 vCoords;
			xmlSegs->getChild(ii)->getAttr("wc",vCoords);
			rcWorldBound.DoUnite(TRectI(vCoords.x,vCoords.y,vCoords.x,vCoords.y));
		}

		XmlNodeRef xmlSegInfo = XmlHelpers::CreateXmlNode("SegmentInfo");
		XmlNodeRef xmlWorldBounds = xmlSegInfo->newChild("WorldBounds");
		xmlWorldBounds->setAttr("Min",rcWorldBound.Min);
		xmlWorldBounds->setAttr("Max",rcWorldBound.Max);
		xmlSegInfo->addChild(xmlSegs);

		CString strTmpFileName = transaction.TryRemapFileName(strFileName, strFileName + SW_TempNewFileExtension, true);
		if (!XmlHelpers::SaveXmlNode(xmlSegInfo,strTmpFileName))
			return CDataUpgrade::UR_FAILED;

		//transaction.lstTempRenamed.push_back(std::make_pair(strTmpFileName,strFileName));
		transaction.ConfirmRemappedFileName();

		return CDataUpgrade::UR_SUCCEEDED;
	}

	virtual CDataUpgrade::EUpgradeResult UpdateGlobalVersionNumber( const CString& strLevelPath, TTransactionContext& transaction ) 
	{
		return CUpgradePipe_Version0ToVersion1::DoUpdateGlobalVersionNumber(strLevelPath, m_nToVersion, transaction);
	}

	virtual CDataUpgrade::EUpgradeResult UpdateSegVersionNumber( const CString& strSegPath, TTransactionContext& transaction ) 
	{
		return CUpgradePipe_Version0ToVersion1::DoUpdateSegVersionNumber(strSegPath, m_nToVersion, transaction);
	}

	virtual const char* GetSegsDirPath() const { return CUpgradePipe_Version0ToVersion1::s_szSegsDirPath; }

}_upgrader_1To2;


//////////////////////////////////////////////////////////////////////////
// Version 2 to Version 3
//////////////////////////////////////////////////////////////////////////
class CUpgradePipe_Version2ToVersion3 : public CBaseUpgradePipe
{
public:
	CUpgradePipe_Version2ToVersion3()
		:CBaseUpgradePipe(2,3)
	{}

	virtual CDataUpgrade::EUpgradeResult UpdateGlobalVersionNumber( const CString& strLevelPath, TTransactionContext& transaction ) 
	{
		return CUpgradePipe_Version0ToVersion1::DoUpdateGlobalVersionNumber(strLevelPath, m_nToVersion, transaction);
	}

	virtual CDataUpgrade::EUpgradeResult UpdateSegVersionNumber( const CString& strSegPath, TTransactionContext& transaction ) 
	{
		return CUpgradePipe_Version0ToVersion1::DoUpdateSegVersionNumber(strSegPath, m_nToVersion, transaction);
	}

	virtual CDataUpgrade::EUpgradeResult DoUpgradeGlobals( const CString& strLevelPath, TTransactionContext& transaction ) 
	{
		CString strSourceDir = strLevelPath + CUpgradePipe_Version0ToVersion1::s_szSegsDirPath;
		CString strTargetDir = strLevelPath + GetSegsDirPath();

		if (!CFileUtil::CreatePath(strTargetDir))
			return CDataUpgrade::UR_FAILED;
		
		// This simple action should not fail. But if it fails, since we cannot track directories in transaction, we can't do anything with it.
		// So we have to call it a broken result in this case.
		CFileUtil::ECopyTreeResult ret = CFileUtil::MoveTree(strSourceDir, strTargetDir);
		if (ret == CFileUtil::ETREECOPYFAIL)
			return CDataUpgrade::UR_BROKEN;

		return CDataUpgrade::UR_SUCCEEDED;
	}


}_upgrader_2to3;

SW_NAMESPACE_END();



