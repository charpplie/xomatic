#include "StdAfx.h"
#include "InternalCommon.h"
#include "DataUpgrade.h"

#include "SegmentedWorldManager.h"
#include "ISWDataPersistence.h"




using namespace sw;
SW_NAMESPACE_BEGIN();

CDataUpgrade CDataUpgrade::s_singleton;


//////////////////////////////////////////////////////////////////////////
// Data Upgrade manager
//////////////////////////////////////////////////////////////////////////

uint32 CDataUpgrade::GetSWDataStructureVersion( XmlNodeRef xmlLevel )
{
	assert (xmlLevel && xmlLevel->isTag(g_szLevelRootTag));
	uint32 nSWDdataStructureVersion;
	if (!xmlLevel->getAttr(g_szSWDStructVerAttr, nSWDdataStructureVersion))
		nSWDdataStructureVersion = 0;

	return nSWDdataStructureVersion;
}

CBaseUpgradePipe* CDataUpgrade::RegisterUpgradePipe( CBaseUpgradePipe* pPipe, bool bForceReplace /*= false*/)
{
	TVersion fromVersion = pPipe->GetFromVersion();
	TVersion toVersion = pPipe->GetToVersion();
	assert(toVersion > 1 && fromVersion < toVersion);

	TUpgradePipeLookup::iterator iterLower = m_mapUpgradePipeLookup.lower_bound(fromVersion);
	TUpgradePipeLookup::iterator iterUpper = m_mapUpgradePipeLookup.upper_bound(toVersion-1);

	// there are two types of conflict: having same fromVersion, having same toVersion
	// We can simply ignore the second one, it's not an unhealthy thing, e.g.: it provides
	// more customized upgrading for a special minor version to a major one
	if (iterLower != iterUpper && iterLower->first == fromVersion)
	{
		if (toVersion > iterLower->second->GetToVersion())	// replace it with better upgrader
		{
			CBaseUpgradePipe* pOld = iterLower->second;
			iterLower->second = pPipe;
			return pOld;	// return the replaced
		}
		return pPipe;		// already has a better one, return the candidate
	}
	else	// no conflict
	{
		m_mapUpgradePipeLookup.insert(iterLower,std::make_pair(fromVersion,pPipe));
		return NULL;
	}
}

CBaseUpgradePipe* CDataUpgrade::FindBestPipeFromVer( TVersion fromVerion, bool* pbMatch ) const
{
	if (pbMatch) *pbMatch = false;

	TUpgradePipeLookup::const_iterator iter = m_mapUpgradePipeLookup.lower_bound(fromVerion);
	if (iter == m_mapUpgradePipeLookup.end() )
		return NULL;

	if (iter->first == fromVerion && pbMatch)
		*pbMatch = true;

	return iter->second;
}

CDataUpgrade::EUpgradeResult CDataUpgrade::Upgrade( TVersion* pResultVersionOut, CSCDataPersistence* pPersist )
{
	const char* pszLevelPath = pPersist->GetLevelPath();

	if (pResultVersionOut) *pResultVersionOut = SWDStructVer_Invalid;

	TVersion verLevelCurrent = DetectSWLevelVersion(pPersist);
	if (verLevelCurrent == SWDStructVer_Invalid)	// cannot identify the level as SW level with any upgrader
		return UR_FAILED;


	// beginning of the real job

	if (pResultVersionOut) *pResultVersionOut = verLevelCurrent;

	TVersion verCurrentImpl = CSWManager::GetSWDataStructureVersion();

	bool isAllPerfectMatch = true;
	TUpgradePipeline customizedPipeline;
	TVersion verNextToFind = verLevelCurrent;
	do
	{
		bool isPerfectMatch = false;
		CBaseUpgradePipe* pPipe = FindBestPipeFromVer(verNextToFind,&isPerfectMatch);
		if (!pPipe)
			break;

		if (!isPerfectMatch)
			isAllPerfectMatch = false;

		customizedPipeline.push_back(pPipe);
		verNextToFind = pPipe->GetToVersion();
	}
	while(verNextToFind < verCurrentImpl);

	if ( verNextToFind != verCurrentImpl || customizedPipeline.empty())
		return UR_FAILED;						//< upgrading path not found

	if (!isAllPerfectMatch)
		if (IDYES != CSWManager::SWMsgBoxQuestion("There is no perfect data upgrading path found for the level, try the closest path anyway?\n"
										"\n"
										"PLEASE BACKUP YOUR DATA FRIST!"))
			return UR_FAILED;

	EUpgradeResult result;
	TVersion verUpgradingCur = verLevelCurrent;
	TUpgradePipeline::const_iterator iterPrev = customizedPipeline.end();
	for (TUpgradePipeline::iterator iter = customizedPipeline.begin(); iter != customizedPipeline.end(); ++iter)
	{

		result = (*iter)->Upgrade(verLevelCurrent, iterPrev, customizedPipeline, pPersist);

		if (result != UR_SUCCEEDED)
			break;

		verUpgradingCur = (*iter)->GetToVersion();
		iterPrev = iter;
	}

	if (result == UR_BROKEN)
	{
		CSWManager::SWMsgBox("The level is either broken or partially missing. You have to recover it manually");
	}

	if (pResultVersionOut) *pResultVersionOut = verUpgradingCur;

	return result;
}

CDataUpgrade::TVersion CDataUpgrade::DetectSWLevelVersion( CSCDataPersistence* pPersist )
{
	const char* pszLevelPath = pPersist->GetLevelPath();

	TVersion verLevelCurrent = SWDStructVer_Invalid;
	for (TUpgradePipeLookup::const_iterator iter = m_mapUpgradePipeLookup.begin(); iter != m_mapUpgradePipeLookup.end(); ++iter)
	{
		TVersion ver = iter->second->TryGetVersionFromLevel(pszLevelPath);
		if (ver != SWDStructVer_Invalid)
			verLevelCurrent = ver;
	}

	return verLevelCurrent;
}

//////////////////////////////////////////////////////////////////////////
// Base class of Upgrade Pipes
//////////////////////////////////////////////////////////////////////////

CDataUpgrade::EUpgradeResult CBaseUpgradePipe::Upgrade( CDataUpgrade::TVersion nInitalVer, CDataUpgrade::TUpgradePipeline::const_iterator iterPrev, const CDataUpgrade::TUpgradePipeline& pipeline, CSCDataPersistence* pPersist )
{
	CDataUpgrade::TVersion from = (iterPrev == pipeline.end())? nInitalVer : (*iterPrev)->GetToVersion();	// the "to" version of the previous pipe is the version that we are actually from.

	if (m_bRistrict && from != m_nFromVersion)
		return CDataUpgrade::UR_NOTSUITABLE;

	CString strLevelPath = pPersist->GetLevelPath();
	strLevelPath = Path::AddPathSlash(strLevelPath);

	CDataUpgrade::EUpgradeResult result = CDataUpgrade::UR_FAILED;
	SW_TRANSACTION_BEGIN(TTransactionContext, transaction)
	{
		if ( CDataUpgrade::UR_SUCCEEDED != (result = DoUpgradeGlobals(strLevelPath, transaction)))
			SW_TRANSACTION_THROW();

		//========== upgrade Segment Data =======
		size_t nFailedOp = 0;

		WIN32_FIND_DATA ffdata;
		HANDLE hFind = FindFirstFile( strLevelPath+ GetSegsDirPath() + "*",&ffdata );
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do{
				DWORD attr = ffdata.dwFileAttributes;
				if (!(attr & FILE_ATTRIBUTE_DIRECTORY)
					|| 0 == strcmp(ffdata.cFileName, ".")
					|| 0 == strcmp(ffdata.cFileName, ".."))
					continue;
				CString strSegPath = strLevelPath + GetSegsDirPath() + ffdata.cFileName + "\\";

				if (CDataUpgrade::UR_SUCCEEDED != (result = DoUpgradeSegment(strSegPath, transaction)))
				{
					++nFailedOp;
					break;
				}

				// change version number
				if (CDataUpgrade::UR_SUCCEEDED != (result = UpdateSegVersionNumber(strSegPath, transaction)))
				{
					++nFailedOp;
					break;
				}

			}
			while(FindNextFile(hFind,&ffdata));
		}

		if (nFailedOp)
			SW_TRANSACTION_THROW();

		// update global version number
		if (CDataUpgrade::UR_SUCCEEDED != (result = UpdateGlobalVersionNumber(strLevelPath, transaction)))
			SW_TRANSACTION_THROW();

	}
	SW_TRANSACTION_CATCH
	{
		// rollback as much as we can even if it's broken
		transaction.Rollback();
		return result;
	}
	SW_TRANSACTION_END;


	return CDataUpgrade::UR_SUCCEEDED;
}


void CBaseUpgradePipe::TTransactionContext::Rollback()
{
	for (TRenameList::const_iterator iter = m_lstPermanentlyRenamed.begin(); iter != m_lstPermanentlyRenamed.end(); ++iter)
		CFileUtil::DeleteFile(iter->second);

	for (TRenameList::const_iterator iter = m_lstTemporarilyRenamed.begin(); iter != m_lstTemporarilyRenamed.end(); ++iter)
		CFileUtil::DeleteFile(iter->second);
}

void CBaseUpgradePipe::TTransactionContext::Commit()
{
	assert(m_unconfirmed.IsNull());

	for (TRenameList::const_iterator iter = m_lstPermanentlyRenamed.begin(); iter != m_lstPermanentlyRenamed.end(); ++iter)
		CFileUtil::DeleteFile(iter->first);

	// temporary list must be proceeded later
	for (TRenameList::reverse_iterator iter = m_lstTemporarilyRenamed.rbegin(); iter != m_lstTemporarilyRenamed.rend(); ++iter)
	{
		CFileUtil::DeleteFile(iter->first);				// delete original
		CFileUtil::MoveFile(iter->second,iter->first);	// rename back
	}
}

const CString& CBaseUpgradePipe::TTransactionContext::TryRemapFileName( const CString& strOld, const CString& strNew, bool bTemp /*= false*/ )
{
	assert(m_unconfirmed.IsNull());

	if (IsFileRemapped(strOld))
		return GetRemappedFileName(strOld);						// remapping already exists

	if (strOld == strNew)
		return strNew;

	if (bTemp)
	{
		if (IsARemapTarget(strOld))			// already been remapped, no need to create temp file
			return strOld;

		assert(!strOld.IsEmpty() && !strNew.IsEmpty());
		//this->m_lstTemporarilyRenamed.push_back(std::make_pair(strOld,strNew));
		m_unconfirmed.strOld = strOld;
		m_unconfirmed.strNew = strNew;
		m_unconfirmed.bTemp = bTemp;
	}
	else
	{
		//this->m_lstPermanentlyRenamed.push_back(std::make_pair(strOld,strNew));
		m_unconfirmed.strOld = strOld;
		m_unconfirmed.strNew = strNew;
		m_unconfirmed.bTemp = bTemp;
	}

	return strNew;
}

void CBaseUpgradePipe::TTransactionContext::ConfirmRemappedFileName()
{
	if (m_unconfirmed.IsNull())		// no change to confirm
		return;

	if (m_unconfirmed.bTemp)
		m_lstTemporarilyRenamed.push_back(std::make_pair(m_unconfirmed.strOld, m_unconfirmed.strNew));
	else
		m_lstPermanentlyRenamed.push_back(std::make_pair(m_unconfirmed.strOld, m_unconfirmed.strNew));

	m_unconfirmed.strOld.Empty();
	m_unconfirmed.strNew.Empty();
}


const CString& CBaseUpgradePipe::TTransactionContext::GetRemappedFileName( const CString& strOriginal ) const
{
	if (strOriginal.IsEmpty())
		return strOriginal;

	const CString* strCur = &strOriginal;
	for (TRenameList::const_iterator iter = m_lstTemporarilyRenamed.begin(); iter != m_lstTemporarilyRenamed.end(); ++iter)
	{
		if (iter->first == *strCur)
		{
			strCur = &iter->second;
			break;
		}
	}

	// permanent renaming chain is allowed
	for (TRenameList::const_iterator iter = m_lstPermanentlyRenamed.begin(); iter != m_lstPermanentlyRenamed.end(); ++iter)
	{
		if (iter->first == *strCur)
			strCur = &iter->second;
	}

	return *strCur;		// for non-remapped file, the name is the original name
}

bool CBaseUpgradePipe::TTransactionContext::IsFileRemapped( const CString& strOriginal ) const
{
	if (strOriginal.IsEmpty())
		return false;

	for (TRenameList::const_iterator iter = m_lstTemporarilyRenamed.begin(); iter != m_lstTemporarilyRenamed.end(); ++iter)
	{
		if (iter->first == strOriginal)
			return true;
	}

	for (TRenameList::const_iterator iter = m_lstPermanentlyRenamed.begin(); iter != m_lstPermanentlyRenamed.end(); ++iter)
	{
		if (iter->first == strOriginal)
			return true;
	}

	return false;
}

bool CBaseUpgradePipe::TTransactionContext::IsARemapTarget( const CString& strNew ) const
{
	if (strNew.IsEmpty())
		return false;

	for (TRenameList::const_iterator iter = m_lstTemporarilyRenamed.begin(); iter != m_lstTemporarilyRenamed.end(); ++iter)
	{
		if (iter->second == strNew)
			return true;
	}

	for (TRenameList::const_iterator iter = m_lstPermanentlyRenamed.begin(); iter != m_lstPermanentlyRenamed.end(); ++iter)
	{
		if (iter->second == strNew)
			return true;
	}

	return false;
}

SW_NAMESPACE_END();
