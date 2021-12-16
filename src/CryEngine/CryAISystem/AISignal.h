#ifndef _AISIGNAL_H_
#define _AISIGNAL_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "IAISystem.h"
#include <ISerialize.h>
#include <PoolAllocator.h>

struct AISignalExtraData : public IAISignalExtraData
{
public:
	AISignalExtraData();
	AISignalExtraData( const AISignalExtraData& other ) : sObjectName(0) { *this = other; }
	virtual ~AISignalExtraData();

	AISignalExtraData& operator = ( const AISignalExtraData& other );
	VIRTUAL void Serialize( TSerialize ser );

	inline void* operator new(size_t size)
	{
		return m_signalExtraDataAlloc.Allocate();
	}

	inline void operator delete(void* p)
	{
		return m_signalExtraDataAlloc.Deallocate(p);
	}

	VIRTUAL const char* GetObjectName() const { return sObjectName ? sObjectName : ""; }
	VIRTUAL void SetObjectName(const char* objectName);

	// To/from script table
	VIRTUAL void ToScriptTable(SmartScriptTable &table) const;
	VIRTUAL void FromScriptTable(const SmartScriptTable &table);

private:
	char* sObjectName;

	typedef stl::PoolAllocator<sizeof(IAISignalExtraData) + sizeof(void*), 
		stl::PoolAllocatorSynchronizationSinglethreaded> SignalExtraDataAlloc;
	static SignalExtraDataAlloc m_signalExtraDataAlloc;
};

#endif
