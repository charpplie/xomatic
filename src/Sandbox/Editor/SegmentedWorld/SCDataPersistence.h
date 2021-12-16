#pragma once
#include "ISWDataPersistence.h"
#include "VersionCommon.h"
#include "ISWVersionControl.h"	// for _smart_ptr


SW_NAMESPACE_BEGIN();

// A implementation of Data Persistence that supports SourceControl.
// Note: Use NullVersionControl to disable SC functionality.
class CSCDataPersistence	// : public ISWDataPersistence
	: public _i_reference_target_t
{
	//typedef class CSegmentedWorld TVersionMgr;
	typedef sw::ISWVersionControl TVersionControl;
public:

	explicit CSCDataPersistence(TVersionControlPtr pVC, const char* pszPath);
	
	//--== Interface ==--
	TVersionControlPtr GetVersionControl() { return m_pVC; }
	//TVersionControlPtr SetVersionControl(TVersionControlPtr pNew) { TVersionControlPtr pOld = m_pVC; m_pVC = pNew; return pOld; }

	bool CheckoutSave( sw::Versioned& vd, CMemoryBlock& data, int64 nVersion, sw::EVersionType vt );
	bool CheckoutSaveSubmit( sw::Versioned& vd, CMemoryBlock& data, int64 nVersion, sw::EVersionType vt );
	bool Save( sw::Versioned &vd, CMemoryBlock &data );
	bool LoadRevision( sw::Versioned& vd, int64 nHRID, CMemoryBlock& data);
	bool Load( const sw::Versioned& vd, CMemoryBlock& data );
	bool Add( sw::Versioned& vd);
	bool Delete( sw::Versioned& vd);

	CString GetEditorDataPath(TWorldCoords const& wc);
	void GetPakPath(CString &strSegmentPakFileName, CString &strSegmentPakPath, CString &strSegmentFileBlockPath, TWorldCoords const& wc, bool bMultiPak, bool bCreateDir);
	CString const& GetLevelPath() const { return m_sLevelPath; }

protected:
	void GetSinglePakPath(CString &strSegmentPakFileName, CString &strSegmentPakPath);
	void GetMultiPakPath(CString &strSegmentPakFileName, CString &strSegmentPakPath, int wx, int wy, bool bCreateDir);
	void SetLevelPath(CString const& sNew) { m_sLevelPath = sNew; }

private:
	//TVersionMgr* m_pVerMgr;
	TVersionControlPtr m_pVC;
	CString m_sLevelPath;
};

SW_NAMESPACE_END();
