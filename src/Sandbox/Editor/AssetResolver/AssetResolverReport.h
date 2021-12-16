////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
// -------------------------------------------------------------------------
//  File name:   AssetResolverReport.h
//  Version:     v1.00
//  Created:     11/12/2012 by Paul Reindell.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __AssetResolverReport_H__
#define __AssetResolverReport_H__

#include "Util/Variable.h"
#include "IAssetSearcher.h"

class CMissingAssetMessage;

class CMissingAssetRecord
{
public:
	enum EState
	{
		ESTATE_PENDING,
		ESTATE_NOT_RESOLVED,
		ESTATE_AUTO_RESOLVED,
		ESTATE_ACCEPTED,
		ESTATE_CANCELLED,
	};

	uint32 id;
	EState state;
	int assetTypeId;
	int searcherId;
	CString orgname;
	CString extension;
	std::vector<CString> substitutions;
	CMissingAssetMessage* pRecordMessage;
	bool needUpdate;
	bool needRemove;
	std::vector<TMissingAssetResolveCallback> requests;
	IAssetSearcher::TAssetSearchId searchRequestId;

	CMissingAssetRecord(const TMissingAssetResolveCallback& request, const char* _orgname, uint32 _id, int _searcherId, int _assetTypeId)
		: id(_id)
		, searcherId(_searcherId)
		, assetTypeId(_assetTypeId)
		, state(ESTATE_PENDING)
		, orgname(_orgname)
		, pRecordMessage(NULL)
		, needUpdate(false)
		, needRemove(false)
		, searchRequestId(IAssetSearcher::INVALID_ID)
	{
		requests.push_back(request);
		InitExtension(_orgname);
	}

	CMissingAssetRecord()
	{
		state = ESTATE_PENDING;
		assetTypeId = -1;
	}

	inline void UpdateState(EState newState)
	{
		state = newState;
		needUpdate = true;
	}

	inline void SetUpdated() { needUpdate = false; }
	inline bool NeedUpdate() const { return needUpdate; }
	inline bool NeedRemove() const { return needRemove; }

	struct SNotifier
	{
		SNotifier(uint32 _id, const std::vector<TMissingAssetResolveCallback>& reqs, const char* oFile, const char* nFile, bool _success )
			: id(_id)
			, requests(reqs)
			, orgFile(oFile)
			, newFile(nFile)
			, success(_success)
		{}

		inline void Notify()
		{
			for (std::vector<TMissingAssetResolveCallback>::const_iterator it = requests.begin(), end = requests.end(); it != end; ++it)
				(*it)(id, success, orgFile.GetString(), newFile.GetString());
		}

	private:
		uint32 id;
		std::vector<TMissingAssetResolveCallback> requests;
		CString orgFile;
		CString newFile;
		bool success;
	};

	SNotifier GetNotifier();

	void InitExtension(const char* file);
};

class CMissingAssetReport : public _i_reference_target_t
{
	typedef std::map<uint32, CMissingAssetRecord> TRecords;
public:
	CMissingAssetReport();

	uint32 AddRecord(const TMissingAssetResolveCallback& request, const char* filename, int searcherId, int assetTypeId);
	void FlagRemoveRecord(uint32 id);
	void FlagRemoveRecord(const TMissingAssetResolveCallback& request, uint32 reportId = 0);
	void FlushRemoved();

	int GetCount() const { return m_records.size(); };
	CMissingAssetRecord* GetRecord(uint32 id);
	CMissingAssetRecord* GetRecord(CMissingAssetMessage* pRecord);

	bool NeedUpdate() const { return m_needUpdate; }
	void SetUpdated(bool updated) { m_needUpdate = !updated; }

	struct SRecordIterator
	{
	public:
		CMissingAssetRecord* Next()
		{
			if (m_iterator != m_end)
				return &((m_iterator++)->second);
			return NULL;
		}

	private:
		friend class CMissingAssetReport;
		SRecordIterator(TRecords& records)
			: m_iterator(records.begin())
			, m_end(records.end())
		{}

		TRecords::iterator m_iterator;
		TRecords::iterator m_end;
		TRecords m_copy;
	};

	SRecordIterator GetIterator() { return SRecordIterator(m_records); }

private:
	bool IsNotInList( const char* filename, uint32& id ) const;
	uint32 GetNextFreeId() const;

private:
	TRecords m_records;
	bool m_needUpdate;
};


#endif // __AssetResolverReport_H__