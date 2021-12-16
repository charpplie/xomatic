#include "StdAfx.h"
#include "InternalCommon.h"
#include "WorldData.h"
#include "ObjectData.h"
#include "SegmentedWorldManager.h"
#include "SegmentedWorldDoc.h"
#include "LayerData.h"
#include "SegmentData.h"
#include "ISWDataPersistence.h"
#include "ISWVersionControl.h"

#include "CryMemBlock.h"
#include "Terrain/Heightmap.h"

using namespace sw;

/////////////////////////////////////////////////////////////////////////////////////////
TWorldData::TWorldData()
:Versioned(VDT_WORLD)
{
	AUTO_LOCK(g_cs);
	Versioned::InitVersion();
	m_eType = WDB_LEVELGENERAL;
	m_xmlAr = NULL;
}

TWorldData::~TWorldData()
{
	SAFE_DELETE(m_xmlAr);
}

bool TWorldData::Load(int64 nHRID)
{
//#if !defined(USE_LOCAL_SAVE) || defined(USE_NATIVE_VC) || defined(USE_NULL_VC)
	if (nHRID <= 0) {
		assert(!"Invalid version requested");
		return false;
	}

	{
		AUTO_LOCK(g_cs);
		if (m_nLoadedHRID == nHRID)
		{
			// already loaded
			GoToStateAtLeast(S_Loaded);	// mark it as loaded
			return true;
		}
	}

	// if already loaded then it's a reloading
	if (IsStateAtLeast(S_Loaded))
		GoToState(S_Reloading);

	auto psw = GetInternalSWDoc();

	{
		if (!psw->GetPersistence()->LoadRevision(*this,nHRID,m_data))
		{
			Unload();
			return false;
		}

		// copy to xml archive, if it's a xml block
		if (m_eType < WDB_XMLCOUNT)
		{
			SAFE_DELETE(m_xmlAr);
			m_xmlAr = new CXmlArchive();
			m_xmlAr->bLoading = true;

			CMemoryBlock mem;
			//wd.Uncompress(mem);
			this->GetData(mem);
			WorldData_MemBlockToXml(mem,(*m_xmlAr));
			if ( m_eType != WDB_LEVELNAMEDDATA && !m_xmlAr->root)	// there is no interface to check if m_xmlAr.pNamedData is empty
			{
				assert(0);
				return false;
			}

			assert( m_eType == WDB_LEVELNAMEDDATA || m_xmlAr->root->isTag(GetWorldDataBlockName(m_eType)));
		}


		//m_nLoadedHRID = nHRID;
		Versioned::OnVersionLoaded(nHRID);
		return true;
	}
	return false;
}

bool TWorldData::DoSave( sw::ESaveFlag sflag, sw::EVersionType vt, int64 nVersion )
{
	if (!PrepareData())
		return false;

	//if (nVersion < 0)
	//	nVersion = GetVersionControl()->CreateChangeList(vt);
	assert(nVersion > 0);

	if (!SaveData( nVersion, vt))
	{
		////bRes = false;
		//if (arrSaves[i] == LockResult_Succeeded)
		//	m_WorldData[i].Unlock();
		return false;
	}
	//bVersionHistorySaved = true;

	bool bModified = UpdateVersionsToLoaded(vt);
#ifndef USE_NULL_VC
	assert(bModified);
#endif
	bool bOK = UpdateRevHead(vt);
	assert(bOK);
	//if (vt == VT_OFFICIAL)
	//	Unlock();

	//assert(vt != VT_OFFICIAL);
	//bRes = true;
	return true;
}

bool TWorldData::DoIntegrate( sw::EVersionType vt, int64 nVersion)
{
	bool bModified = Versioned::DoIntegrateOnly(vt,nVersion);

	Unlock();
	return bModified;
}

bool TWorldData::SaveData( int64 nVersion, sw::EVersionType vt )
{
	//assert(s_nSWMode != 0 || CSegmentedWorld::IsMapConverting() );
	auto psw = GetInternalSWDoc();

//#if !defined(USE_LOCAL_SAVE) || defined(USE_NATIVE_VC) || defined(USE_NULL_VC)

	//assert(g_db->GetCachedConn());
	bool bSucceeded = psw->GetPersistence()->CheckoutSaveSubmit(*this,m_data, nVersion, vt);
	if (!bSucceeded)
		return false;
	return true;
//#else
//	CString name = GetFileName();
//	if (!SaveMemBlockToFile(name, m_data))
//		return false;
//
//	Versioned::OnSavedDataToDB(1);
//	return true;
//#endif
}

bool TWorldData::CreateData()
{
	GetInternalSWDoc()->GetPersistence()->Add(*this);

	return true;
}

EVersionType TWorldData::GetWorldDataNeededVersionType()
{
	auto psw = GetInternalSWDoc();
	if (true/*psw->GetVersionType() != VT_OFFICIAL*/)
		return psw->GetVersionType();
	{	
		AUTO_LOCK(g_cs);
		if (m_nLockedBy != psw->GetVersionControl()->GetUserID())
			return psw->GetVersionType();
	}
	return VT_CURRENT;
}

void TWorldData::Unload()
{
	AUTO_LOCK(g_cs);
	m_nLoadedHRID = HRID_MISSING;
	m_data.Free();
}

void TWorldData::SetAppliedHRID(int64 nAppliedHRID)
{
	//if (nAppliedHRID > 0)
	//	m_nHRIDBeforeModify = nAppliedHRID;
	//m_nAppliedHRID = nAppliedHRID;
	assert(0);// see what's needed here -RL
}

CString TWorldData::GetFileName(CSCDataPersistence& persist) const
{
	assert(m_eType >= 0 && m_eType < sw::WDB_COUNT);
	CString sFileName = GetInternalSWDoc()->GetLevelPath() + GetWorldDataFileName(m_eType);
	return sFileName;
}

// TODO: the level.xml file has XML text *and* named raw data, both are with size header, not good looking for manual editing.
// WARNING: This function is used in DataUpgrade, don't forget to "Copy-on-change" to DataUpgrade module.
bool TWorldData::WorldData_XmlToMemBlock( CXmlArchive &xmlAr, CMemoryBlock& mem )
{
	auto psw = GetInternalSWDoc();

	if (!xmlAr.root && m_eType != WDB_LEVELNAMEDDATA)
		return false;

	if (psw->GetSpecialOperationType() == CSegmentedWorldDoc::SW_SO_XMLS_EXEC_SCRIPT_AND_SAVE)
		psw->CallScript(xmlAr.root);

	//CMemFile mf;
	//CArchive ar(&mf, CArchive::store);
	//XmlString str1 = xmlAr.root->getXML();
	//CString str = str1.c_str();
	//ar << str;
	//xmlAr.pNamedData->Serialize(ar);
	//ar.Flush();
	//int iSize = (int) mf.GetLength();

	////CMemoryBlock mem;
	//mem.Free();
	//mem.Allocate(iSize);
	//mf.SeekToBegin();
	//mf.Read(mem.GetBuffer(), iSize);
	if (m_eType == WDB_LEVELNAMEDDATA)
	{
		assert(!xmlAr.root || xmlAr.root->isTag(GetWorldDataBlockName(WDB_LEVELNAMEDDATA)));
		CMemFile mfile;
		CArchive ar(&mfile, CArchive::store);
		xmlAr.pNamedData->Serialize(ar);
		ar.Flush();

		mem.Free();
		mem.Allocate(mfile.GetLength());
		mfile.SeekToBegin();
		mfile.Read(mem.GetBuffer(),mfile.GetLength());
	}
	else
	{
		{
			CMemFile mfile;
			CArchive ar(&mfile, CArchive::store);
			xmlAr.pNamedData->Serialize(ar);
			ar.Flush();
			assert(4 == mfile.GetLength());		// make sure there are no data in namedData
		}

		XmlString str = xmlAr.root->getXML();
		mem.Free();
		mem.Allocate(str.size());
		mem.Copy((void*)str.c_str(), str.size());
	}

	return true;
}

// WARNING: This function is used in DataUpgrade, don't forget to "Copy-on-change" to DataUpgrade module.
bool TWorldData::WorldData_MemBlockToXml( CMemoryBlock& mem, CXmlArchive &xmlAr )
{
	//CMemFile mf((BYTE *) mem.GetBuffer(), mem.GetSize());
	//
	//CArchive ar(&mf, CArchive::load);
	//
	//CString str;
	//ar >> str;
	//
	//xmlAr.pNamedData->Serialize( ar );

	//xmlAr.root = XmlHelpers::LoadXmlFromBuffer( str, str.GetLength() );
	//if (!xmlAr.root)
	//	return false;
	//return true;

	if (m_eType == WDB_LEVELNAMEDDATA)
	{
		CMemFile mfile((BYTE *) mem.GetBuffer(), mem.GetSize());
		CArchive ar(&mfile, CArchive::load);
		xmlAr.pNamedData->Serialize(ar);
		return true;
	}
	else
	{
		if (xmlAr.root = XmlHelpers::LoadXmlFromBuffer((const char*)mem.GetBuffer(),mem.GetSize()))
			return true;
		return false;
	}
}

bool TWorldData::PrepareData()
{
	auto psw = GetInternalSWDoc();
	//assert( CSWManager::IsAutomate() || m_WorldData[i].IsLoadedDataACache());
	assert(m_xmlAr != NULL);

	CMemoryBlock mem;
	if (!WorldData_XmlToMemBlock(*m_xmlAr, mem))
	{
		//if (arrSaves[i] == LockResult_Succeeded)
		//	m_WorldData[i].Unlock();
		return false;
	}
	this->SetData(mem);

	GoToState(S_ModifSerialized);
	return true;
}

void TWorldData::SetData( CMemoryBlock& mem )
{
#ifndef SW_NO_DATACOMPRESS
	mem.Compress(m_data);
#else
	m_data.Free();
	m_data = mem;
#endif
}

void TWorldData::GetData( CMemoryBlock& mem ) const
{
#ifndef SW_NO_DATACOMPRESS
	m_data.Uncompress(mem);
#else
	mem.Free();
	mem = m_data;
#endif
}
