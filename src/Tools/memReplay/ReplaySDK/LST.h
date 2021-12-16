#pragma once

#include "SizeInfo.h"
#include "ReplayLogReader.h"
#include "CallstackTable.h"

struct AllocInfo
{
	AllocInfo()
		: freed(1)
	{}
	AllocInfo(u32 threadId, u32 callstackId, s32 sizeRequested, s32 sizeConsumed, s32 sizeGlobal)
		: threadId(threadId)
		, freed(0)
		, callstackId(callstackId)
		, sizeRequested(sizeRequested)
		, sizeConsumed(sizeConsumed)
		, sizeGlobal(sizeGlobal)
	{
	}

	u32 threadId;
	u32 freed : 1;
	u32 callstackId : 31;
	s32 sizeRequested;
	s32 sizeConsumed;
	s32 sizeGlobal;
};

class AllocSet
{
public:
	typedef std::map<TAddress, AllocInfo,
		std::less<TAddress>,
		STLPoolAllocator<std::pair<const TAddress, AllocInfo> > > MutableAllocSet;
	typedef std::vector<std::pair<TAddress, AllocInfo> > ImmutableAllocSet;

public:
	AllocSet();

	void Serialise(u8*& buffer, size_t& bufferLen);
	void Deserialise(const u8* buffer, size_t bufferLen);

	void Compact();

	void Add(const AllocSet& other);
	
	void AddAlloc(TAddress ptr, const AllocInfo& ai);
	void FreeAlloc(TAddress ptr, const AllocInfo& ai);

	const MutableAllocSet& GetAllocs() { EnsureMutable(); return *m_mutableAllocs; }

private:
	void EnsureMutable() { if (!m_mutableAllocs.IsValid()) MakeMutable(); }
	void MakeMutable();
	void MakeImmutable();

private:
	SharedPtr<MutableAllocSet> m_mutableAllocs;
	SharedPtr<ImmutableAllocSet> m_immutableAllocs;
};

class AllocSetLST : public IReplayListener
{
public:
	explicit AllocSetLST(const TCHAR* filename);
	~AllocSetLST();

	bool Restore();

	void ReplayBegin();
	void Replay(ReplayRange range);
	void ReplayEnd(u64 position);

	void Add(AllocSet& set, u64 begin, u64 end);

	CallstackTable& GetCallstackTable() { return *m_callstackTable; }
	u32 GetAllocEventSplit() const { return SplitAlignment; }

private:
	enum
	{
		SplitShift = 14,
		SplitAlignment = 1<<14,
	};

	struct AllocSetHeader
	{
		AllocSetHeader()
			: allocEvBegin(0)
			, allocEvEnd(0)
			, serialisedIndex(0)
			, serialisedSize(0)
		{}

		AllocSetHeader(u64 evBegin, u64 evEnd)
			: allocEvBegin(evBegin)
			, allocEvEnd(evEnd)
			, serialisedIndex(0)
			, serialisedSize(0)
		{}

		AllocSetHeader(u64 evBegin, u64 evEnd, int serIndex, int serSize)
			: allocEvBegin(evBegin)
			, allocEvEnd(evEnd)
			, serialisedIndex(serIndex)
			, serialisedSize(serSize)
		{}

		u64 allocEvBegin;
		u64 allocEvEnd;
		int serialisedIndex;
		int serialisedSize;

		AllocSet set;
		SizeInfo totalSize;
	};

	typedef std::map<TAddress, AllocInfo> AllocIndex;


private:
	AllocSetLST(const AllocSetLST&);
	AllocSetLST& operator = (const AllocSetLST&);

	void ApplyFree(AllocSetHeader*& activeSet, TAddress ptr, ptrdiff_t sizeGlobal);

	void FinaliseTree();
	void Serialise(size_t layer, size_t index);

private:
	std::basic_string<TCHAR> m_filename;
	//HZIP m_zip;

	SharedPtr<CallstackTable> m_callstackTable;
	std::vector<SharedPtr<std::vector<AllocSetHeader> > > m_tree;

	// Capture state
	u64 m_currentAllocEvIdx;
	AllocIndex m_allocs;
};
