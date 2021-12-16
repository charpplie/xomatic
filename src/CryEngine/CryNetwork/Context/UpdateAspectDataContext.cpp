#include "StdAfx.h"
#include "UpdateAspectDataContext.h"
#include "NetContext.h"

CUpdateAspectDataContext::CUpdateAspectDataContext( CNetContext * pNetContext, SContextObject& obj, SContextObjectEx& objx, CTimeValue localPhysicsTime ) 
: m_pNetContext(pNetContext)
, m_obj(obj)
, m_objx(objx)
, m_ref(&obj, &objx)
, m_localPhysicsTime(localPhysicsTime)
, m_takenAspects(0)
, m_fetchAspects(0)
{
	m_allowedAspects = 0;
	if (obj.bOwned)
		m_allowedAspects = objx.nAspectsEnabled;
	else if (obj.bControlled)
		m_allowedAspects = m_pNetContext->DelegatableAspects() & objx.nAspectsEnabled;
}

void CUpdateAspectDataContext::RequestFetchAspects( const NetworkAspectType fetchAspects )
{
	m_fetchAspects = fetchAspects & m_allowedAspects;

#if ENABLE_ASPECT_HASHING
	m_hashAspects = fetchAspects & objx.nAspectsEnabled;
#endif

	for (NetworkAspectID i=0; i<NumAspects; i++)
	{
		const NetworkAspectType aspectBit = (1<<i);

		bool enabled = (m_objx.nAspectsEnabled & aspectBit) != 0;
		m_oldHdls[i] = CMementoMemoryManager::InvalidHdl;
		if (enabled)
		{
			if (m_objx.vAspectData[i] == CMementoMemoryManager::InvalidHdl)
			{
				// object is enabled, but we don't have data for it
				// so we'd better fetch it
				m_fetchAspects |= aspectBit;
			}
			else if ((aspectBit&m_fetchAspects) && (aspectBit&m_allowedAspects))
			{
				m_oldHdls[i] = m_objx.vAspectData[i];
				m_objx.vAspectData[i] = CMementoMemoryManager::InvalidHdl;
			}
		}
		else if (m_objx.vAspectData[i] != CMementoMemoryManager::InvalidHdl)
		{
			// object is disabled, but we have data for it...
			// drop that data
			m_oldHdls[i] = m_objx.vAspectData[i];
			m_objx.vAspectData[i] = CMementoMemoryManager::InvalidHdl;
			m_fetchAspects &= ~aspectBit;
		}
	}
}

void CUpdateAspectDataContext::FetchDataFromGame( NetworkAspectID i )
{
	const NetworkAspectType aspectBit = 1<<i;

	CMementoStreamAllocator streamAllocatorForNewState(&MMM());
	// assume the size of data taken will be the same as last time, or 8 bytes if we've never fetched anything
	size_t sizeHint = 8;
	if (m_oldHdls[i] != CMementoMemoryManager::InvalidHdl)
		sizeHint = MMM().GetHdlSize(m_oldHdls[i]);
	CByteOutputStream stm( &streamAllocatorForNewState, sizeHint );
	NET_ASSERT(m_objx.vAspectData[i] == CMementoMemoryManager::InvalidHdl);
#if LOG_BUFFER_UPDATES
//	if (CNetCVars::Get().LogBufferUpdates)
//		NetLog("[buf] pull %s:%s", id.GetText(), m_pNetContext->GetAspectName(i));
#endif
	// timestamped aspects need the timestamp as the first part of the stored buffer
	if (m_pNetContext->TimestampedAspects() & aspectBit)
	{
		stm.PutTyped<CTimeValue>() = m_localPhysicsTime;
	}
	CCompressionManager& cman = CNetwork::Get()->GetCompressionManager();
	uint8 readProfile = (m_obj.vAspectProfiles[i] >= MaxProfilesPerAspect? 0 : m_obj.vAspectProfiles[i]);
	ChunkID readChunkID = GetChunkIDFromObject(m_ref, i);
	// actually fetch the data from game
	bool gameReadResult = cman.GameContextRead( m_pNetContext->GetGameContext(), &stm, m_obj.userID, aspectBit, readProfile, readChunkID );
	if (gameReadResult && TakeChange(i, stm, streamAllocatorForNewState))
	{
		m_objx.vAspectData[i] = streamAllocatorForNewState.GetHdl();
		MMM().ResizeHdl( m_objx.vAspectData[i], stm.GetSize() );
		m_objx.vAspectDataVersion[i] ++;
		m_takenAspects |= aspectBit;
	}
	else
	{
		MMM().FreeHdl(streamAllocatorForNewState.GetHdl());
		m_objx.vAspectData[i] = m_oldHdls[i];
		m_oldHdls[i] = CMementoMemoryManager::InvalidHdl;
	}
}

CUpdateAspectDataContext::~CUpdateAspectDataContext()
{
	for (int i=0; i<NumAspects; i++)
	{
		if (m_oldHdls[i] != CMementoMemoryManager::InvalidHdl)
		{
			MMM().FreeHdl(m_oldHdls[i]);
		}
#if ENABLE_ASPECT_HASHING
		const NetworkAspectType aspectBit = 1<<i;
		if (m_hashAspects & aspectBit)
		{
			ASSERT_PRIMARY_THREAD;
			m_objx.hash[i] = m_pNetContext->GetGameContext()->HashAspect( m_obj.userID, aspectBit );
		}
#endif
	}
}

bool CUpdateAspectDataContext::TakeChange( NetworkAspectID i, CByteOutputStream& stm, CMementoStreamAllocator& streamAllocatorForNewState )
{
	const NetworkAspectType aspectBit = 1<<i;

	bool take = true;
	if (m_oldHdls[i] != CMementoMemoryManager::InvalidHdl)
	{
		if (stm.GetSize() == MMM().GetHdlSize(m_oldHdls[i]))
		{
			int ofs = 0;
			if (m_pNetContext->TimestampedAspects() & aspectBit)
				ofs += sizeof(CTimeValue);
			if(stm.GetSize() == ofs)
				take = false;
			else
			{
				const char * pOld = (const char *)MMM().PinHdl(m_oldHdls[i]);
				const char * pNew = (const char *)MMM().PinHdl(streamAllocatorForNewState.GetHdl());
				int cmpSz = stm.GetSize() - ofs;
				if (cmpSz > 0 && 0 == memcmp(pOld + ofs, pNew + ofs, cmpSz))
					take = false;
				else if (!cmpSz)
					take = false;
			}
		}
	}
	return take;
}
