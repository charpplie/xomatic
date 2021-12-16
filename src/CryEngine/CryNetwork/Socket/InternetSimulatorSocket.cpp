#include "StdAfx.h"
#include "InternetSimulatorSocket.h"

#if INTERNET_SIMULATOR

#include "Network.h"

CInternetSimulatorSocket::CInternetSimulatorSocket( IDatagramSocketPtr pChild ) : m_pChild(pChild), m_packetLossRateAccumulator(0.0f)
{
}

CInternetSimulatorSocket::~CInternetSimulatorSocket()
{
	for (std::set<NetTimerId>::iterator iter = m_pendingSends.begin(); iter != m_pendingSends.end(); ++iter)
	{
		NetTimerId oldTimer = *iter;
		SPendingSend * pPS = static_cast<SPendingSend*>(TIMER.CancelTimer(oldTimer));
		delete[] pPS->pBuffer;
		delete pPS;
	}
}

void CInternetSimulatorSocket::SimulatorUpdate( NetTimerId tid, void * p, CTimeValue t )
{
	SPendingSend * pPS = static_cast<SPendingSend*>(p);

	if (!pPS->pThis->IsDead())
	{
		if (ePT_Data == pPS->eType)
			pPS->pThis->m_pChild->Send( pPS->pBuffer, pPS->nLength, pPS->to );
		else if (ePT_Voice == pPS->eType)
			pPS->pThis->m_pChild->SendVoice( pPS->pBuffer, pPS->nLength, pPS->to );
		else
			NetError("[CInternetSimulatorSocket]: Unsupported payload type [%d]", pPS->eType);
	}

	delete[] pPS->pBuffer;
	delete pPS;
}

void CInternetSimulatorSocket::Die()
{
	m_pChild->Die();
}

bool CInternetSimulatorSocket::IsDead()
{
	return m_pChild->IsDead();
}

void CInternetSimulatorSocket::GetSocketAddresses( TNetAddressVec& addrs )
{
	m_pChild->GetSocketAddresses(addrs);
}

//static CMTRand_int32 r;
ESocketError CInternetSimulatorSocket::Send( const uint8 * pBuffer, size_t nLength, const TNetAddress& to )
{
	//local server context can't handle buffering of packets on local connection
	//  buffering can result in packets arriving next frame for object unbound this frame 
	//   => will cause disconnect and context destruction
	if(to.GetPtr<TLocalNetAddress>())
	{
		return m_pChild->Send( pBuffer, nLength, to );
	}

	m_packetLossRateAccumulator += CVARS.PacketLossRate; // * r.GenerateFloat();
	if (m_packetLossRateAccumulator >= 1.0f)
	{
		m_packetLossRateAccumulator -= 1.0f;
		return eSE_Ok;
	}

	if(CVARS.PacketExtraLag == 0)
	{
		return m_pChild->Send( pBuffer, nLength, to );		
	}
	else
	{
		SPendingSend * pPS = new SPendingSend;
		CTimeValue sendTime = g_time + CVARS.PacketExtraLag; // * r.GenerateFloat();
		pPS->eType = ePT_Data;
		pPS->pBuffer = new uint8[nLength];
		memcpy( pPS->pBuffer, pBuffer, nLength );
		pPS->nLength = nLength;
		pPS->to = to;
		pPS->pThis = this;
		NetTimerId timer = TIMER.AddTimer( sendTime, SimulatorUpdate, pPS );
		return eSE_Ok;
	}
}

ESocketError CInternetSimulatorSocket::SendVoice( const uint8 * pBuffer, size_t nLength, const TNetAddress& to )
{
	if(to.GetPtr<TLocalNetAddress>())
	{
		return m_pChild->SendVoice( pBuffer, nLength, to );
	}

	m_packetLossRateAccumulator += CVARS.PacketLossRate;
	if (m_packetLossRateAccumulator >= 1.0f)
	{
		m_packetLossRateAccumulator -= 1.0f;
		return eSE_Ok;
	}

	if(CVARS.PacketExtraLag == 0)
	{
		return m_pChild->SendVoice( pBuffer, nLength, to );
	}
	else
	{
		SPendingSend * pPS = new SPendingSend;
		CTimeValue sendTime = g_time + CVARS.PacketExtraLag;
		pPS->eType = ePT_Voice;
		pPS->pBuffer = new uint8[nLength];
		memcpy( pPS->pBuffer, pBuffer, nLength );
		pPS->nLength = nLength;
		pPS->to = to;
		pPS->pThis = this;
		NetTimerId timer = TIMER.AddTimer( sendTime, SimulatorUpdate, pPS );
		return eSE_Ok;
	}
}

void CInternetSimulatorSocket::SetListener( IDatagramListener * pListener )
{
	m_pChild->SetListener(pListener);
}

SOCKET CInternetSimulatorSocket::GetSysSocket()
{
	return m_pChild->GetSysSocket();
}

#endif
