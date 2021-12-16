#pragma once

#include "IXml.h"
#include "SWFwd.h"

//////////////////////////////////////////////////////////////////////////
// Auto-Adaptive Data Upgrading with Rollback support

// The key to keep each UpgradePipe working against every level version and with every code version is keeping old code
// that are used in any upgrading code when you are going to change it.
// So please do the following:
// 
// !!! COPY THE FOLLOWING WARNING TO EVERY FUNCTIONS THAT ARE USED IN UPGRADE PIPES !!!
// WARNING: This function is used in DataUpgrade, don't forget to "Copy-on-change" to DataUpgrade module.

//////////////////////////////////////////////////////////////////////////

SW_NAMESPACE_BEGIN();


class CBaseUpgradePipe;

class CDataUpgrade
{
public:
	typedef TSWDataStructureVersion TVersion;
	typedef std::list<CBaseUpgradePipe*> TUpgradePipeline;

	enum EUpgradeResult{
		UR_SUCCEEDED = 0,
		UR_NOTIMPL,		// not implemented

		UR_UNKNOWNERROR		= (int)0x80000000,
		UR_FAILED,
		UR_BROKEN,
		UR_NOTSUITABLE,
	};

public:
	~CDataUpgrade(){}
	static CDataUpgrade& Get(){return s_singleton;}

	TVersion DetectSWLevelVersion( sw::ISWDataPersistence* pPersist );
	EUpgradeResult Upgrade( TVersion* pResultVersionOut, sw::ISWDataPersistence* pPersist );
	CBaseUpgradePipe* RegisterUpgradePipe( CBaseUpgradePipe* pPipe, bool bForceReplace = false);

	// Utilities
	static TVersion GetSWDataStructureVersion(XmlNodeRef xmlLevel);

protected:
	CDataUpgrade(){}
	CBaseUpgradePipe* FindBestPipeFromVer(TVersion fromVerion, bool* pbMatch = NULL)const;
	//void RegisterDefaultPipes();	

protected:
	typedef std::map<TVersion, CBaseUpgradePipe*> TUpgradePipeLookup;

	static CDataUpgrade s_singleton;
	TUpgradePipeLookup m_mapUpgradePipeLookup;

	friend CBaseUpgradePipe;
};

class CBaseUpgradePipe
{
protected:
	struct TTransactionContext
	{
	protected:
		typedef std::list< std::pair<CString,CString> > TRenameList;
		TRenameList m_lstTemporarilyRenamed;
		TRenameList m_lstPermanentlyRenamed;

		struct _TUncommitted{
			bool bTemp;
			CString strOld;
			CString strNew;
			bool IsNull()const { return strOld.IsEmpty() && strNew.IsEmpty(); }
		} m_unconfirmed;

	public:
		// Rollback the transaction
		void Rollback();
		// Commit the transaction, so that all changes will be permanent
		void Commit();

		// Helps user get the new name of a file in the transaction, if there is any.
		// Returns empty string if deleted.
		const CString& GetRemappedFileName(const CString& strOriginal)const;
		// Rename/remap a file without caring if it has already been renamed once(or more), the function handles the magic for you.
		// Returns the suggested new file name, must be used instead
		const CString& TryRemapFileName(const CString& strOld, const CString& strNew, bool bTemp = false);
		// You must confirm when there is actually a file stored under the name.
		// This: get-name, create-file, confirm mechanism must be used because the create-file step might be failed.
		void ConfirmRemappedFileName();
		bool IsFileRemapped(const CString& strOriginal)const;
		bool IsARemapTarget(const CString& strNew)const;
	};

public:
	virtual const char* GetSegsDirPath() const { return SW_EditorDataDirPath; }
	virtual CDataUpgrade::TVersion GetFromVersion()const	{ return m_nFromVersion; }
	virtual CDataUpgrade::TVersion GetToVersion()const		{ return m_nToVersion; }
	virtual CDataUpgrade::TVersion TryGetVersionFromLevel(const char* pszLevelPath) {return SWDStructVer_Invalid;}

	// This function is overridable but you should try override DoUpgradeGlobals or DoUpgradeSegment first.
	virtual CDataUpgrade::EUpgradeResult Upgrade(CDataUpgrade::TVersion nInitalVer, CDataUpgrade::TUpgradePipeline::const_iterator iterPrev, const CDataUpgrade::TUpgradePipeline& pipeline, CSCDataPersistence* pPersist);

	// template method for doing the actual
	virtual CDataUpgrade::EUpgradeResult DoUpgradeGlobals( const CString& strLevelPath, TTransactionContext& transaction){return CDataUpgrade::UR_SUCCEEDED;}
	// template method for doing the actual upgrade work for segment data
	virtual CDataUpgrade::EUpgradeResult DoUpgradeSegment( const CString& strSegPath, TTransactionContext& transaction ){return CDataUpgrade::UR_SUCCEEDED;}
	virtual CDataUpgrade::EUpgradeResult UpdateGlobalVersionNumber( const CString& strLevelPath, TTransactionContext& transaction ) =0;
	virtual CDataUpgrade::EUpgradeResult UpdateSegVersionNumber( const CString& strSegPath, TTransactionContext& transaction ) =0;

protected:
	CBaseUpgradePipe():m_nFromVersion(0),m_nToVersion(0),m_bRistrict(true){}
	explicit CBaseUpgradePipe(CDataUpgrade::TVersion verFrom, CDataUpgrade::TVersion verTo, bool bRistrict = true)
		:m_nFromVersion(verFrom),m_nToVersion(verTo),m_bRistrict(bRistrict)
	{
		CBaseUpgradePipe* pOld = CDataUpgrade::Get().RegisterUpgradePipe(this);
		assert(pOld == NULL);
	}

protected:
	CDataUpgrade::TVersion	m_nFromVersion;
	CDataUpgrade::TVersion	m_nToVersion;
	bool					m_bRistrict;

};

SW_NAMESPACE_END();
