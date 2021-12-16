#include "stdafx.h"
#include "InternalCommon.h"
#include "SCDataPersistence.h"

#include "ISWVersionControl.h" // for Checkout_save_submit()
#include "SegmentedWorldDoc.h" // for TVerionMgr
#include "CryMemBlock.h"
#include <ISourceControl.h>

using namespace sw;


CSCDataPersistence::CSCDataPersistence( TVersionControlPtr pVC, const char* pszPath)
	: m_pVC(pVC)
	, m_sLevelPath(pszPath)
{
	Path::ConvertSlashToBackSlash(m_sLevelPath);
	m_pVC->SetPersistentStorage(this);
}


bool CSCDataPersistence::CheckoutSave( sw::Versioned& vd, CMemoryBlock& data, int64 nVersion, sw::EVersionType vt )
{
	assert(!TCheckOutContext());
	if (!m_pVC->CheckOut(vd,vt,nVersion))
	{ assert(0); return false;}

	if (!Save(vd, data))
	{ assert(0); return false;}

	if (!m_pVC->Add(vd))
	{ assert(0); return false;}

	return true;
}


bool CSCDataPersistence::CheckoutSaveSubmit( sw::Versioned& vd, CMemoryBlock& data, int64 nVersion, sw::EVersionType vt )
{
	if (!CheckoutSave(vd,data,nVersion,vt))
		return false;

	//if (!m_pVC->Submit(vd) )
	//{ assert(0); return false;}

	return true;
}


bool CSCDataPersistence::LoadRevision( Versioned& vd, int64 nHRID, CMemoryBlock& data )
{
	//uint64 attr = CFileUtil::GetAttributes(vd.GetFileName(*this));
	//if(attr & SCC_FILE_ATTRIBUTE_MANAGED)
	//{
	//	bool bOK = m_pVC->GetRevision(vd,VT_CURRENT,nHRID);
	//	assert(bOK);
	//}
	return Load(vd, data);
}

bool CSCDataPersistence::Save( sw::Versioned &vd, CMemoryBlock &data)
{
	TCheckOutContext &ctx = vd.GetCheckOutCtx();
	if (!ctx)
	{
		CRY_ASSERT_MESSAGE(0,"for DB version, the data must has been checked-out");
		return false;
	}

	CString name = vd.GetFileName(*this);

#ifndef SW_NO_DATACOMPRESS
	if (!SaveMemBlockToFile(name, data))
		return false;
#else
	if (vd.m_vdt == VDT_LAYER || vd.m_vdt == VDT_EXTOBJECT || vd.m_vdt == VDT_SEGTRUNK_COMP_HEAD)
	{
		if (!CFileUtil::OverwriteFile(name) || !SaveRawDataToFile(name,data))	// don't need file header if no compress
			return false;
	}
	else if (vd.m_vdt == VDT_WORLD)
	{
		if (!CFileUtil::OverwriteFile(name) || !SaveRawDataToFile(name, data))
			return false;
	}
	else
	{
		if (!SaveMemBlockToFile(name,data))
			return false;
	}
#endif
	//{
	//	AUTO_LOCK(g_cs);
	//	SetAppliedHRID(hrid);
	//	Versioned::SetLoadedVersion(hrid);
	//}
	vd.OnSavedDataToDB(ctx.m_nRev);
	return true;
}

bool CSCDataPersistence::Add( sw::Versioned & vd )
{
	return m_pVC->DoCreateData(vd);
}

bool sw::CSCDataPersistence::Delete( sw::Versioned& vd )
{
	if (vd.IsMissingInPersist())		// already deleted or not write to file yet
		return true;
	return m_pVC->Delete(vd);
}


bool CSCDataPersistence::Load( const Versioned &vd, CMemoryBlock& data )
{
	CString name = vd.GetFileName(*this);
#ifndef SW_NO_DATACOMPRESS
	return LoadMemBlockFromFile(name,data);
#else
	if (vd.m_vdt == VDT_LAYER || vd.m_vdt == VDT_EXTOBJECT)
		return LoadRawDataFromFile(name, data);
	else if (vd.m_vdt == VDT_WORLD)
		return /*LoadRawDataFromScry*/LoadRawDataFromFile(name, data);
	else
		return LoadMemBlockFromFile(name,data);
#endif
}

CString CSCDataPersistence::GetEditorDataPath(TWorldCoords const& wc)
{
	CString path;
	path.Format("%s" SW_EditorDataDirPath SW_SegmentDirPattern "\\", Path::AddPathSlash(m_sLevelPath), wc.wx, wc.wy);
	return path;
}

void CSCDataPersistence::GetPakPath( CString &strSegmentPakFileName, CString &strSegmentPakPath, CString &strSegmentFileBlockPath, TWorldCoords const& wc, bool bMultiPak, bool bCreateDir )
{
	if(bMultiPak)
	{
		GetMultiPakPath(strSegmentPakFileName, strSegmentPakPath, wc.wx, wc.wy, bCreateDir);
		strSegmentFileBlockPath = strSegmentPakPath;
	}
	else
	{
		GetSinglePakPath(strSegmentPakFileName, strSegmentPakPath);
		strSegmentFileBlockPath.Format("%s" SW_SegmentDirPattern "\\", strSegmentPakPath, wc.wx, wc.wy);
	}
}

void CSCDataPersistence::GetSinglePakPath( CString &strSegmentPakFileName, CString &strSegmentPakPath )
{
	CString& sLevelPath = m_sLevelPath;

	strSegmentPakPath.Format("%s" SW_SegmentsDirPath, Path::AddPathSlash(sLevelPath));
	strSegmentPakFileName.Format("%s""level.pak", sLevelPath);
}

void CSCDataPersistence::GetMultiPakPath( CString &strSegmentPakFileName, CString &strSegmentPakPath, int wx, int wy, bool bCreateDir )
{
	CString& sLevelPath = m_sLevelPath;

	strSegmentPakPath.Format("%s" SW_SegmentsDirPath SW_SegmentDirPattern "\\", Path::AddPathSlash(sLevelPath), wx, wy);
	strSegmentPakFileName.Format( "%s%s", strSegmentPakPath, SW_SegPakFile);

	if (bCreateDir && !CFileUtil::PathExists(strSegmentPakPath))
		CFileUtil::CreateDirectory(strSegmentPakPath);
}
