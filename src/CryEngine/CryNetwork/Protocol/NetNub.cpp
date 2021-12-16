/*************************************************************************
 Crytek Source File.
 Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
 $Id$
 $DateTime$
 Description:  implements multiplexing and connecting/disconnecting 
               channels
 -------------------------------------------------------------------------
 History:
 - 26/07/2004   : Created by Craig Tiller
*************************************************************************/
#include "StdAfx.h"
#include "Config.h"
#include "NetNub.h"
#include "NetChannel.h"
#include "Serialize.h"
#include "Network.h"
#include "INubMember.h"
#include "INetwork.h"
#include "INetworkService.h"
#include "Context/ClientContextView.h"
#include "IEntitySystem.h"
#include "Network.h"
#include "Lobby/CryLobby.h"
#include "Lobby/CryMatchMaking.h"

#if USE_GAMESPY_SDK
#include "Services/GameSpy/GSNetworkProfile.h"
#endif

static const float STATS_UPDATE_INTERVAL_NUB = 0.25f;
static const float KESTIMEOUT = 30.0f;
static const int VERSION_SIZE = 6;

#if USE_DEFENCE
struct SAddProtectedFile
{
	SAddProtectedFile( CNetChannel * pChannel ) : m_pChannel(pChannel) {}
	CNetChannel * m_pChannel;
	void operator()( const string& file ) const
	{
		m_pChannel->AddProtectedFile( file );
	}
};
#endif //NOT_USE_DEFENCE


void TraceUnrecognizedPacket( const char *inszTxt, const uint8 * cBuf, size_t nCount, const TNetAddress &ip)
{
#ifdef _DEBUG
	OutputDebugString("\n");
	OutputDebugString(inszTxt);
	OutputDebugString("\n");

	static char sTemp[1024];
	::OutputDebugString("-------------------------------\n");
	sprintf(sTemp,"INVALID PACKET FROM [%s]\n",RESOLVER.ToString(ip).c_str());
	::OutputDebugString(sTemp);
	for(size_t n=0;n<nCount;n++)
	{
		sprintf(sTemp,"%02X ",cBuf[n]);
		::OutputDebugString(sTemp);
		if(n && (n%16)==0)
			::OutputDebugString("\n");
	}
#endif
}

template <class Map>
typename Map::iterator LookupAddress( const TNetAddress& addr, Map& m, uint32 flags )
{
#if defined(WIN32) || defined(WIN64)
	if (flags & eCLF_HandleBrokenWindowsICMP)
	{
		// icmp messages returned via GetQueuedCompletionStatus have bogus ports
		if (const SIPv4Addr * pAddr = addr.GetPtr<SIPv4Addr>())
		{
			int count = 0;
			Map::iterator itOut = m.end();
			for (typename Map::iterator it = m.begin(); it != m.end(); ++it)
			{
				if (const SIPv4Addr * pKey = it->first.GetPtr<SIPv4Addr>())
				{
					if (pKey->addr == pAddr->addr)
					{
						if (pKey->port == pAddr->port)
							return it;
						itOut = it;
						count ++;
					}
				}
			}
			if (count == 1)
				return itOut;
		}
	}
#endif
	return m.find(addr);
}

class CDefaultSecurity : public IGameSecurity
{
public:
	bool IsIPBanned(uint32 ip) { return false; }
	void OnPunkDetected(const char * ip, EPunkType punkType) {}
};

static CDefaultSecurity s_defaultSecurity;


class NatNegListener : public INatNegListener
{
public:
    enum EState
    {
        eS_None,
        eS_Idle,
        eS_Negotiating,
        eS_Finished
    };
private:
    struct NegProcess
    {
        NegProcess():        
        m_success(false),
        m_state(eS_Idle)
        {}
        int         m_cookie;
        EState      m_state;
        bool        m_success;
        sockaddr_in m_addr;
    };
    typedef std::vector<NegProcess> NegVector;
public:

    NatNegListener()
    {

    }

    void OnDetected(bool success, bool compatible)
    {
        //just do nothing
    }

    NegVector::iterator FindNeg(int cookie)
    {
      for(NegVector::iterator it = m_negotiations.begin(),end=m_negotiations.end();it!=end;++it)
        if(it->m_cookie == cookie)
          return it;
      return m_negotiations.end();
    }

    NegVector::const_iterator FindNeg(int cookie)const
    {
      for(NegVector::const_iterator it = m_negotiations.begin(),end=m_negotiations.end();it!=end;++it)
        if(it->m_cookie == cookie)
          return it;
      return m_negotiations.end();

    }
    
    void OnCompleted(int cookie, bool success, sockaddr_in* addr)
    {
        NegVector::iterator it = FindNeg(cookie);
        if(it!=m_negotiations.end())
        {
            NegProcess& pr = *it;

            pr.m_success = success;
            if(addr)//can be 0 if not succeeded
               pr.m_addr = *addr;
            pr.m_state = eS_Finished;
        }
    }

    void NewNegotiation(int cookie)
    {
        m_negotiations.push_back(NegProcess());
        m_negotiations.back().m_cookie = cookie;
    }

    void EndNegotiation(int cookie)
    {
        NegVector::iterator it = FindNeg(cookie);
        if(it!=m_negotiations.end())
            m_negotiations.erase(it);
    }

    EState      GetState(int cookie)const
    {
        NegVector::const_iterator it = FindNeg(cookie);
        if(it==m_negotiations.end())
            return eS_None;
        else
            return it->m_state;
    }
    void SetState(int cookie, EState state)
    {
        NegVector::iterator it = FindNeg(cookie);
        if(it!=m_negotiations.end())
            it->m_state = state;
    }
    bool        GetResult(int cookie)const
    {
        NegVector::const_iterator it = FindNeg(cookie);
        if(it==m_negotiations.end())
            return false;
        else
            return it->m_success;    
    }
    void        GetAddr(int cookie, sockaddr_in& a)const
    {
        NegVector::const_iterator it = FindNeg(cookie);
        if(it!=m_negotiations.end())
            a = it->m_addr;
    }
private:
    NegVector m_negotiations; //FIFO
};

NatNegListener g_NatNegListener;

CNetNub::CNetNub( const TNetAddress& addr, IGameNub * pGameNub, 
								  IGameSecurity * pNetSecurity, IGameQuery * pGameQuery ) :
	INetworkMember(eNMUO_Nub),
	m_pNetwork(NULL),
	m_pSecurity(pNetSecurity? pNetSecurity : &s_defaultSecurity),
	m_pGameQuery(pGameQuery),
	m_pGameNub(pGameNub),
	m_bDead(false),
	m_addr(addr),
	m_cleanupChannel(0),
  m_natNeg(0),
  m_serverReport(0),
  m_connectingLocks(0),
	m_keepAliveLocks(0)
{
	m_pLobby = (CCryLobby*)gEnv->pNetwork->GetLobby();
	m_statsTimer = TIMER.AddTimer( g_time, TimerCallback, this );
	++g_objcnt.nub;
}

CNetNub::~CNetNub()
{
  SCOPED_GLOBAL_LOCK;
	--g_objcnt.nub;
	TIMER.CancelTimer( m_statsTimer );
	if (m_pSocketMain)
		m_pLobby->ExternalSocketDie(m_pSocketMain);
  if(m_natNeg && !m_natCookies.empty())
  {
      for(uint32 i=0;i<m_natCookies.size();++i)
      {
          m_natNeg->CancelNegotiation(m_natCookies[i]);
          g_NatNegListener.EndNegotiation(m_natCookies[i]);
      }
  }
}

void CNetNub::DeleteNub()
{
	SCOPED_GLOBAL_LOCK;
	for (TChannelMap::iterator i = m_channels.begin(); i != m_channels.end(); ++i)
	{
		DisconnectChannel( eDC_NubDestroyed, &i->first, NULL, "Nub destroyed" );
	}

	m_bDead = true;
	if (m_pSocketMain)
		m_pLobby->ExternalSocketSetListener(m_pSocketMain, NULL);
	TIMER.CancelTimer( m_statsTimer );
}

bool CNetNub::ConnectTo( const char * address, const char * connectionString )
{
	SCOPED_GLOBAL_LOCK; 
  CNubConnectingLock conlk(this);
	NetLogAlways("connection requested to: %s", address);
  
  static string nat_prefix = "<nat>";

  if (!strncmp(address,nat_prefix.c_str(),nat_prefix.size()))
  {
    string cookie_str = address + nat_prefix.size();
		size_t orPos = cookie_str.find('|');
		string altip;
		if (orPos != string::npos)
		{
			altip = cookie_str.substr(orPos+1);
			cookie_str = cookie_str.substr(0, orPos);
		}
    int cookie = atoi(cookie_str);
    g_NatNegListener.NewNegotiation(cookie);
    m_natCookies.push_back(cookie);
    GN_Lazy_ContinueNat(cookie,connectionString,altip,conlk);
  }
  else
  {
		static string sessionPrefix = "<session>";

		CrySessionHandle session = CrySessionInvalidHandle;
		string addressStr = address;
		string currentStr;

		while (true)
		{
			size_t pos = addressStr.find(',');

			if (pos != string::npos)
			{
				// Found a comma
				currentStr = addressStr.substr(0, pos);
				addressStr = addressStr.substr(pos + 1);

				if (strnicmp(currentStr, sessionPrefix.c_str(), sessionPrefix.size()) == 0)
				{
					// Found a session handle
					session = atoi(currentStr.substr(sessionPrefix.size()));
				}
			}
			else
			{
				// No more commas in the address string only ip:port should be left
				break;
			}
		}

		CNameRequestPtr pReq = RESOLVER.RequestNameLookup(addressStr);

    string sConStr = connectionString;
    GN_Lazy_ContinueConnectTo(pReq, session, sConStr, conlk);
  }

	return true;
}

void CNetNub::GN_Lazy_ContinueConnectTo( CNameRequestPtr pReq, CrySessionHandle session, string connectionString, CNubConnectingLock conlk )
{
	SCOPED_GLOBAL_LOCK;

	if (!pReq->TimedWait(0.01f))
	{
		FROM_GAME(&CNetNub::GN_Loop_ContinueConnectTo, this, pReq, session, connectionString, conlk);
		return;
	}

  //m_connecting = false;//initial step is over

	TNetAddressVec ips;
	if (pReq->GetResult(ips) != eNRR_Succeeded)
	{
		m_pGameNub->FailedActiveConnect( eDC_ResolveFailed, "Failed to lookup address" );
		return;
	}

	if (!ips.empty())
		for (uint32 i=0; i<ips.size(); i++)
			NetLogAlways("resolved as: %s", RESOLVER.ToNumericString(ips[i]).c_str());

	for (TNetAddressVec::const_iterator it = ips.begin(); it != ips.end(); ++it)
	{
		if ( !RESOLVER.IsPrivateAddr(*it) )
		{
			INetworkServicePtr pGameSpy = CNetwork::Get()->GetService("GameSpy");
			if (0 == pGameSpy)
				NetLogAlways("There is no GameSpy service available");

			if ( pGameSpy && !pGameSpy->GetNetworkProfile()->IsLoggedIn() )
			{
				m_pGameNub->FailedActiveConnect( eDC_PunkDetected, "Cannot connect to public server unless you are logged into GameSpy network" );
				return;
			}
		}
	}

	//TNetAddress& ip = ips[0];
	DoConnectTo(ips,session,connectionString,conlk);
}

void CNetNub::GN_Loop_ContinueConnectTo( CNameRequestPtr pReq, CrySessionHandle session, string connectionString, CNubConnectingLock conlk )
{
	TO_GAME_LAZY(&CNetNub::GN_Lazy_ContinueConnectTo, this, pReq, session, connectionString, conlk);
}

void CNetNub::DoConnectTo(const TNetAddressVec& ips, CrySessionHandle session, string connectionString, CNubConnectingLock conlk)
{
	// cannot connect to an already connected nub
	bool isAlreadyConnected = false;
	{
		CryAutoLock<CryCriticalSection> lock(m_lockChannels);
		for (TNetAddressVec::const_iterator it = ips.begin(); it != ips.end() && !isAlreadyConnected; ++it)
		{
			if (m_channels.find(*it) != m_channels.end())
				isAlreadyConnected = true;
		}
	}
	if (isAlreadyConnected)
		return;

	SPendingConnection pc;
	pc.connectionString = connectionString;
	pc.to = ips[0]; // initially use the first entry
	pc.tos = ips;
	pc.kes = eKES_SetupInitiated;
	pc.kesStart = g_time;
	pc.conlk = conlk;
	pc.session = session;

	if (SendPendingConnect(pc))
		m_pendingConnections.push_back(pc);

	//if (pc.pChannel != NULL)
	//{
	//	if (SendPendingConnect(pc))
	//	m_pendingConnections.push_back(pc);
	//}
}

void CNetNub::FailNatNegotiation( EDisconnectionCause cause, string reason, int cookie, string connectionString, string altip, CNubConnectingLock conlk )
{
	stl::find_and_erase(m_natCookies,cookie);
	g_NatNegListener.EndNegotiation(cookie);

	if (altip.empty())
		m_pGameNub->FailedActiveConnect(eDC_ProtocolError, reason.c_str());
	else
	{
		NetLog("NAT negotiation failed, trying direct connection");
		CNameRequestPtr pReq = RESOLVER.RequestNameLookup(altip);
		GN_Lazy_ContinueConnectTo(pReq, CrySessionInvalidHandle, connectionString, conlk);
	}
}

void CNetNub::GN_Lazy_ContinueNat( int cookie, string connectionString, string altip, CNubConnectingLock conlk )
{
	if(m_natNeg && !m_natNeg->IsAvailable())
	{
		g_NatNegListener.EndNegotiation(cookie);
		return;
	}


	if(!m_natNeg)//need service
	{
		INetworkService* gs = gEnv->pNetwork->GetService("GameSpy");
		ENetworkServiceState state;

		if (gs)
			state = gs->GetState();
		else
			state = eNSS_Failed;

		switch(state)
		{
		case eNSS_Failed:
			NetLog("Nat negotiation failed, gamespy service unavailable");
			FailNatNegotiation( eDC_NatNegError, "Nat negotiation failed", cookie, connectionString, altip, conlk );
			return;//gamespy service unavailable
		case eNSS_Initializing:
			{
				SCOPED_GLOBAL_LOCK;
				TO_GAME_LAZY(&CNetNub::GN_Lazy_ContinueNat, this, cookie, connectionString, altip, conlk);
			}
			return;
		case eNSS_Ok:
			{
				m_natNeg = gs->GetNatNeg(&g_NatNegListener);
			}
			break;
		}
	}

	if(m_natNeg && m_natNeg->IsAvailable())
	{
		switch(g_NatNegListener.GetState(cookie))//either we got result or not started yet
		{
    case NatNegListener::eS_None://Not found
      return;
      break;
		case NatNegListener::eS_Idle:
			g_NatNegListener.SetState(cookie,NatNegListener::eS_Negotiating);
			m_natNeg->StartNegotiation(cookie,false,m_pSocketMain->GetSysSocket());
			break;
		case NatNegListener::eS_Negotiating:
			break;
		case NatNegListener::eS_Finished:
			g_NatNegListener.SetState(cookie,NatNegListener::eS_Idle);
			if(g_NatNegListener.GetResult(cookie))
			{
				TNetAddress ip;
				SIPv4Addr addr;
				sockaddr_in saddr;
				g_NatNegListener.GetAddr(cookie,saddr);
				addr.addr = ntohl(S_ADDR_IP4(saddr));
				addr.port = ntohs(saddr.sin_port);
				ip = TNetAddress(addr);
				NetLog("Nat negotiation complete connecting to %d.%d.%d.%d:%d",(addr.addr>>24)&0xff,(addr.addr>>16)&0xff,(addr.addr>>8)&0xff,(addr.addr&0xff),addr.port);
				SCOPED_GLOBAL_LOCK;
				TNetAddressVec ips;
				ips.push_back(ip);
				DoConnectTo(ips,CrySessionInvalidHandle,connectionString, conlk);
			}
			else
			{
				NetLog("Nat negotiation failed");
				FailNatNegotiation( eDC_NatNegError, "Nat negotiation failed", cookie, connectionString, altip, conlk );
				return;
			}
			g_NatNegListener.EndNegotiation(cookie);
			stl::find_and_erase(m_natCookies,cookie);
			return;
		}
		SCOPED_GLOBAL_LOCK;
		TO_GAME_LAZY(&CNetNub::GN_Lazy_ContinueNat, this, cookie, connectionString, altip, conlk);
	}
}

void CNetNub::OnNatCookieReceived( int cookie )//initializes Nat negotiation on server size
{
	NetLog("NAT cookie received %d",cookie);
	if(!m_natNeg)
	{
		//the service is available now... so just get it
		INetworkService *serv=gEnv->pNetwork->GetService("GameSpy");
		ENetworkServiceState state = serv->GetState();

		if(state!=eNSS_Ok)
			return;
		m_natNeg = serv->GetNatNeg(&g_NatNegListener);
	}

	if(m_natNeg && m_natNeg->IsAvailable())
	{
		g_NatNegListener.NewNegotiation(cookie);
		m_natCookies.push_back(cookie);
		m_natNeg->StartNegotiation(cookie,true,m_pSocketMain->GetSysSocket());
		return;
	}
}

void CNetNub::GN_Lazy_ServerNat( int cookie )
{
	switch(g_NatNegListener.GetState(cookie))
	{
  case NatNegListener::eS_None:
    break;
	case NatNegListener::eS_Finished:// finished successfully
		g_NatNegListener.EndNegotiation(cookie);
		stl::find_and_erase(m_natCookies,cookie);
		break;
	case NatNegListener::eS_Idle:// not active
		g_NatNegListener.EndNegotiation(cookie);
		stl::find_and_erase(m_natCookies,cookie);
		break;
	case NatNegListener::eS_Negotiating://still in progres
		{
			SCOPED_GLOBAL_LOCK;
			TO_GAME_LAZY(&CNetNub::GN_Lazy_ServerNat,this,cookie);
		}
		break;
	}
}

bool CNetNub::IsConnecting()
{
	return m_connectingLocks > 0;
}

void CNetNub::OnCDKeyAuthResult(TNetChannelID plr_id, bool success, const char* description)
{

#if !ALWAYS_CHECK_CD_KEYS
  if(CNetCVars::Get().CheckCDKeys == 2)
    return;
#endif

	if(!success)//yey! we got pirate here
	{
		for (TChannelMap::iterator i = m_channels.begin(); i != m_channels.end(); ++i)
		{
			CNetChannel* pCh = i->second->GetNetChannel();
			if(pCh->GetLocalChannelID() == plr_id)
			{
				SCOPED_GLOBAL_LOCK;
				DisconnectChannel(eDC_AuthenticationFailed,0,pCh,description);
				break;
			}
		}
	}
}

void CNetNub::DisconnectPlayer(EDisconnectionCause cause, EntityId plr_id, const char* reason)
{
  for (TChannelMap::iterator i = m_channels.begin(); i != m_channels.end(); ++i)
  {
    CNetChannel* pCh = i->second->GetNetChannel();
    if(pCh->GetContextView() && pCh->GetContextView()->HasWitness(plr_id))
    {
      SCOPED_GLOBAL_LOCK;
      DisconnectChannel(cause,0,pCh,reason);
      break;
    }
  }
}

void CNetNub::TimerCallback(NetTimerId, void* p, CTimeValue)
{
	CNetNub * pThis = static_cast<CNetNub*>(p);

	SStatistics stats;
	for (TChannelMap::iterator i = pThis->m_channels.begin(); i != pThis->m_channels.end(); ++i)
	{
		if (CNetChannel * pNetChannel = i->second->GetNetChannel())
		{
			INetChannel::SStatistics chanStats = pNetChannel->GetStatistics();
			stats.bandwidthUp += chanStats.bandwidthUp;
			stats.bandwidthDown += chanStats.bandwidthDown;
		}
	}
	pThis->m_statistics = stats;
	pThis->m_ackDisconnectSet.clear();

	if (!pThis->m_bDead)
		pThis->m_statsTimer = TIMER.AddTimer( g_time + STATS_UPDATE_INTERVAL_NUB, TimerCallback, pThis );
}

bool CNetNub::SendPendingConnect( SPendingConnection& pc )
{
	// NOTE: a channel will be created after private key is established
	// so we might not have a valid channel here
	if (pc.pChannel && pc.pChannel->IsDead())
	{
		AddDisconnectEntry(pc.to, pc.session, eDC_CantConnect, "Dead channel");
		return false;
	}
	if (pc.pChannel && pc.pChannel->IsConnectionEstablished())
		return false;
	if (pc.lastSend + 0.25f > g_time)
		return true;

	if (pc.kes == eKES_SetupInitiated)
	{
		if (g_time - pc.kesStart >= KESTIMEOUT)
		{
			NetWarning("Connection to %s timed out", RESOLVER.ToString(pc.to).c_str());
			TO_GAME( &CNetNub::GC_FailedActiveConnect, this, eDC_Timeout, string().Format("Connection attempt to %s timed out", RESOLVER.ToString(pc.to).c_str()), CNubKeepAliveLock(this) );
			CCryMatchMaking* pMatchMaking = NULL;
			if (m_pLobby)
			{
				pMatchMaking = (CCryMatchMaking*)m_pLobby->GetMatchMaking();
				if (pMatchMaking)
				{
					pMatchMaking->SessionDisconnectRemoteConnection(pc.session, pc.to);
				}
			}
			return false; // staying in this state too long
		}

		SFileVersion ver = gEnv->pSystem->GetProductVersion();
		uint32 v[VERSION_SIZE];
		for (int i=0; i<4; i++)
			v[i] = htonl(ver.v[i]);
		v[4] = htonl(PROTOCOL_VERSION);
		v[5] = htonl(CNetwork::Get()->GetSocketIOManager().caps);

		uint32 profile = 0;
		uint32 tokenId = 0;

#if USE_GAMESPY_SDK
		if(INetworkService* serv = CNetwork::Get()->GetService("GameSpy"))
		{
			if(serv->GetState() == eNSS_Ok)
			{
				if( INetworkProfile* p = serv->GetNetworkProfile())
				{
					if(p->IsLoggedIn())
					{
						CGSNetworkProfile* prof = static_cast<CGSNetworkProfile*>(p);
						const SProfileInfo& pi = prof->GetProfileInfo();
						pc.profile = htonl(pi.m_profile);
						profile = htonl(pi.m_profile);
					}				
				}
			}
		}
#endif

		if(ICVar* pP = gEnv->pConsole->GetCVar("cl_profile"))
		{
			pc.profile = pP->GetIVal();
			profile = htonl( pc.profile );
		}
		if (ICVar* pTid = gEnv->pConsole->GetCVar("cl_tokenid"))
			tokenId = htonl(pTid->GetIVal());

		size_t cslen = pc.connectionString.length();

		PREFAST_SUPPRESS_WARNING(6255) uint8 * pBuffer = (uint8*) alloca(1 + (VERSION_SIZE)*sizeof(uint32) + 2*sizeof(uint32) + sizeof(CrySessionHandle) + cslen);

		uint32 cur_size = 0;
		pBuffer[cur_size] = Frame_IDToHeader[eH_ConnectionSetup];
		cur_size ++;
		memcpy( pBuffer + cur_size, v, VERSION_SIZE*sizeof(uint32) );
		cur_size += VERSION_SIZE*sizeof(uint32);
		memcpy( pBuffer + cur_size, &profile, sizeof(uint32));
		cur_size += sizeof(uint32);
		memcpy( pBuffer + cur_size, &tokenId, sizeof(uint32));
		cur_size += sizeof(uint32);		
		CrySessionHandle session = htonl(pc.session);
		memcpy( pBuffer + cur_size, &session, sizeof(CrySessionHandle));
		cur_size += sizeof(CrySessionHandle);
		memcpy( pBuffer + cur_size, pc.connectionString.data(), cslen );
		cur_size += cslen;

		bool bSuccess = SendTo( pBuffer, cur_size, pc.to );

		pc.lastSend = g_time;
		pc.connectCounter ++;
		if ((pc.connectCounter % 10) == 0)
			NetWarning("Retrying connection to %s (%d)", RESOLVER.ToString(pc.to).c_str(), pc.connectCounter);

		if (!bSuccess)
			AddDisconnectEntry(pc.to, pc.session, eDC_CantConnect, "Failed sending connect packet");

		return bSuccess;
	}

	if (pc.kes == eKES_KeyEstablished)
	{
		// if we are in this state, a channel must have been created
		// this pending connection will be removed when data starts flowing on the channel or channel inactivity timeout

		// we do it the simple way: keep sending KeyExchange1 until we are removed by channel connection establishment
		// good thing about it is that we don't need to do the same on the passive connection side, the passive connection side
		// just need to ignore this packet when it's already in eKES_KeyEstablished state, until NetChannel is actively sending data
		pc.lastSend = g_time;
		if ((pc.connectCounter % 10) == 0)
			NetWarning("Resending KeyExchange1 to %s (%d)", RESOLVER.ToString(pc.to).c_str(), pc.connectCounter);
		pc.connectCounter ++;
		return SendKeyExchange1(pc, true);
	}

	AddDisconnectEntry( pc.to, pc.session, eDC_ProtocolError, "Key establishment handshake ordering error trying to send pending connect" );

	// should never come here
	NET_ASSERT(0);
	return false;
}

void CNetNub::PerformRegularCleanup()
{
	// cleanup inactive entries in connecting map
	static TConnectingMap connectingMap;
	connectingMap.clear();
	for (TConnectingMap::iterator iter = m_connectingMap.begin(); iter != m_connectingMap.end(); ++iter)
	{
		if (iter->second.kes == eKES_SentKeyExchange && g_time - iter->second.kesStart >= KESTIMEOUT)
			NetWarning("Removing inactive pre-mature connection from %s", RESOLVER.ToString(iter->first).c_str());
		else
			connectingMap.insert(*iter); // save entries that are still active
	}
	connectingMap.swap(m_connectingMap);

	// zeroth pass: pending connections
	static TPendingConnectionSet pendingConnections;
	pendingConnections = m_pendingConnections;
	m_pendingConnections.resize(0);
	for (TPendingConnectionSet::iterator iter = pendingConnections.begin(); iter != pendingConnections.end(); ++iter)
	{
		if (SendPendingConnect(*iter))
			m_pendingConnections.push_back(*iter);
	}

	// first pass: notify channels
	if (!m_channels.empty())
	{
		size_t n = m_cleanupChannel % m_channels.size();
		TChannelMap::iterator iter = m_channels.begin();
		while (n > 0)
		{
			++iter;
			n--;
		}
		if (!iter->second->IsDead())
			iter->second->PerformRegularCleanup();

		m_cleanupChannel ++;
	}

	// second pass: clear out very old disconnects
	for (TDisconnectMap::iterator iter = m_disconnectMap.begin(); iter != m_disconnectMap.end(); )
	{
		TDisconnectMap::iterator next = iter;
		++next;

		if (iter->second.when + 60.0f < g_time)
			m_disconnectMap.erase(iter);

		iter = next;
	}
}

const INetNub::SStatistics& CNetNub::GetStatistics()
{
	//SCOPED_GLOBAL_LOCK;
	// TODO: define statistics
	// TODO: update statistics here
	return m_statistics;
}

bool CNetNub::Init( CNetwork * pNetwork )
{
	TNetAddress ipLocal;

	uint32 flags = 0;
	if (m_pGameQuery)
		flags |= eSF_BroadcastReceive;
	flags |= eSF_BigBuffer;

	m_pSocketMain = m_pLobby->ExternalSocketCreate( m_addr, flags );
	if (!m_pSocketMain)
		return false;
	m_pLobby->ExternalSocketSetListener(m_pSocketMain, this);

	m_pNetwork = pNetwork;

#if USE_DEFENCE
	m_defenceData.Populate();
#endif

	return true;
}

#if ENABLE_BUFFER_VERIFICATION
class CVerifyBuffer
{
public:
	CVerifyBuffer( const uint8 * pBuffer, unsigned nLength )
	{
		m_nLength = nLength;
		m_vBuffer = new uint8[nLength];
		m_vOrigBuffer = pBuffer;
		memcpy( m_vBuffer, pBuffer, nLength );
	}
	~CVerifyBuffer()
	{
		NET_ASSERT( 0 == memcmp(m_vBuffer, m_vOrigBuffer, m_nLength) );
		delete[] m_vBuffer;
	}

private:
	const uint8 * m_vOrigBuffer;
	uint8 * m_vBuffer;
	unsigned m_nLength;
};
#endif

void CNetNub::SyncWithGame( ENetworkGameSync type )
{
	// first pass: garbage collection & defence refresh - only once per frame
	if (type == eNGS_FrameStart)
	{
#if USE_DEFENCE
		bool refreshDefences = false;
		if (m_defenceData.level != CNetCVars::Get().CheatProtectionLevel)
		{
			m_defenceData.Populate();
			refreshDefences = true;
		}
#endif

		static std::vector<TNetAddress> svDeadChannels;
		for (TChannelMap::iterator i = m_channels.begin(); i != m_channels.end(); ++i)
		{
			if (i->second->IsDead())
				svDeadChannels.push_back(i->first);
#if USE_DEFENCE
			else if (refreshDefences)
			{
				if (CNetChannel * pChannel = i->second->GetNetChannel())
				{
					pChannel->ClearProtectedFiles();
					std::for_each( m_defenceData.m_listProt.begin(), m_defenceData.m_listProt.end(), SAddProtectedFile(pChannel) );
				}
			}
#endif			
		}
		while (!svDeadChannels.empty())
		{
			CryAutoLock<CryCriticalSection> lock(m_lockChannels);
			m_channels.find(svDeadChannels.back())->second->Die();
			m_channels.find(svDeadChannels.back())->second->RemovedFromNub();
			m_channels.erase( svDeadChannels.back() );
			svDeadChannels.pop_back();
		}
	}

	// second pass: queued messages
	for (TChannelMap::iterator i = m_channels.begin(); i != m_channels.end(); ++i)
	{
		i->second->SyncWithGame(type);
	}
}

bool CNetNub::SendTo( const uint8 * pData, size_t nSize, const TNetAddress& to )
{
  //if(m_disconnectMap.find(to) != m_disconnectMap.end())
  //  return false;

	ESocketError se = eSE_Ok;

#if NETWORK_REBROADCASTER
	// Can we talk to this destination directly?
	TNetChannelID fromChannelID = 0;
	TNetChannelID toChannelID = 0;
	TNetChannelID routeChannelID = 0;
	CCryRebroadcaster* pRebroadcaster = NULL;
	CCryLobby* pLobby = (CCryLobby*)CCryLobby::GetLobby();
	if (pLobby)
	{
		pRebroadcaster = pLobby->GetRebroadcaster();
	}

	if (pRebroadcaster)
	{
		fromChannelID = pRebroadcaster->GetLocalChannelID();
		toChannelID = pRebroadcaster->GetChannelIDForAddress(to);
		routeChannelID = pRebroadcaster->FindRoute(fromChannelID, toChannelID);
	}

	if (CNetwork::Get()->IsRebroadcasterEnabled() && fromChannelID != 0 && toChannelID != 0)
	{
		if (routeChannelID == 0)
		{
			// If the routeChannelID is 0, it means the destination is effectively unreachable.
			if (gEnv->bHostMigrating)
			{
				// There's a host migration event in progress; ignore this packet
				return true;
			}
			else
			{
				se = eSE_Unrecoverable;
			}
		}
	}

	if (routeChannelID != toChannelID)
	{
		if (se != eSE_Unrecoverable)
		{
			if (Frame_HeaderToID[pData[0]] == eH_Rebroadcaster)
			{
				// This is a rebroadcaster packet en-route, so bump the hop count
				SRebroadcasterFrameHeader* pHeader = reinterpret_cast<SRebroadcasterFrameHeader*>(const_cast<uint8*>(pData));
				++pHeader->hopCount;
			}
			else
			{
				// Make sure there's enough space in the packet buffer to prepend a rebroadcaster frame header
				STATIC_CHECK(RESERVED_PACKET_BYTES >= sizeof(SRebroadcasterFrameHeader), INSUFFICIENT_RESERVED_PACKET_BYTES);

				// Can't send directly so wrap in a rebroadcaster frame
				SRebroadcasterFrameHeader* pHeader = reinterpret_cast<SRebroadcasterFrameHeader*>(&m_rebroadcasterPacketBuffer);
				pHeader->frameHeaderID = Frame_IDToHeader[eH_Rebroadcaster]; // Frame header id signifying a rebroadcaster packet
				pHeader->hopCount = 1;
				pHeader->sourceID = fromChannelID;		// where the packet is originally from
				pHeader->destinationID = toChannelID;	// where the packet should ultimately end up

				// Prevent potential buffer overrun
				size_t copySize = nSize < (sizeof(m_rebroadcasterPacketBuffer) - sizeof(SRebroadcasterFrameHeader)) ? nSize : 0;
				if (copySize > 0)
				{
					memcpy(m_rebroadcasterPacketBuffer + sizeof(SRebroadcasterFrameHeader), pData, nSize);
					nSize += sizeof(SRebroadcasterFrameHeader);
				}
				else
				{
					NetWarning("Rebroadcaster: packet too large to reroute - discarding");
					return true;
				}
			}

			TNetAddress rerouteTo;
			if (pRebroadcaster && pRebroadcaster->GetAddressForChannelID(routeChannelID, rerouteTo))
			{
				se = m_pSocketMain->Send(m_rebroadcasterPacketBuffer, nSize, rerouteTo);

#if NETWORK_REBROADCASTER_LOG
				static CTimeValue lastUpdate(0.0f);
				if (g_time - lastUpdate > 0.5f)
				{
					NetQuickLog(false, 5, "Rebroadcaster: packet rerouted on machine %i: originally from %i to %i; routing via %i", fromChannelID, fromChannelID, toChannelID, routeChannelID);
					lastUpdate = g_time;
				}
#endif
			}
			else
			{
				NetWarning("Rebroadcaster: *WARNING* unable to determine address for channel id %i in CNetNub::SendTo() - packet discarded", routeChannelID);
			}

		}
	}
	else
#endif
	{
		se = m_pSocketMain->Send(pData, nSize, to);
	}

	if (se != eSE_Ok)
	{
		if (se >= eSE_Unrecoverable)
		{
			CNetChannel * pChannel = GetChannelForIP(to, 0);
			if (pChannel)
      {
				switch(se)
				{
				case eSE_UnreachableAddress:
					pChannel->Disconnect(eDC_ICMPError, "Host unreachable");
					break;
				case eSE_ConnectionRefused:
					pChannel->Disconnect(eDC_ICMPError, "Connection refused");
					break;
				case eSE_ConnectionReset:
					pChannel->Disconnect(eDC_ICMPError, "Connection reset by peer");
					break;
				default:
					pChannel->Disconnect(eDC_ICMPError, "Unrecoverable error on send");
				}
      }
			return false;
		}
		return true;
	}
	return true;
}

void CNetNub::OnPacket( const TNetAddress& from, const uint8 * pData, uint32 nLength )
{
	NET_ASSERT(nLength);

	uint8 nType = Frame_HeaderToID[pData[0]];

	bool processed = false;

	if ( (nType>=eH_TransportSeq0 && nType<=eH_TransportSeq_Last)
		|| (nType>=eH_SyncTransportSeq0 && nType<=eH_SyncTransportSeq_Last) )
		processed = ProcessTransportPacketFrom(from, pData, nLength);
	else switch (nType)
	{
	case eH_KeepAlive:
	case eH_KeepAliveReply:
	case eH_BackOff:
		ProcessSimplePacket(from, pData, nLength);
		processed = true;
		break;
	case eH_CryLobby:
		m_pLobby->OnPacket(from, pData, nLength);
		processed = true;
		break;
	case eH_GameSpyCD:
	case eH_GameSpyGeneral:
		{
			if(!m_serverReport)
			{
				INetworkService* serv = gEnv->pNetwork->GetService("GameSpy");
				if(!serv)
					break;
				m_serverReport = serv->GetServerReport();
			}
			sockaddr_in inAddr;
			if (m_serverReport && ConvertAddr(from, &inAddr))
			{
				m_serverReport->ProcessQuery((char*)pData,nLength,&inAddr);
			}
		}
		processed = true;
		break;
	case eH_GameSpyNAT:
		{     
			sockaddr_in inAddr;
			if (m_natNeg && ConvertAddr(from, &inAddr))
			{
				m_natNeg->OnPacket((char*)pData,nLength, &inAddr);
			}
		}
		processed = true;
		break;
	case eH_PingServer:
		processed = ProcessPingQuery(from, pData, nLength);
		break;
	case eH_QueryLan:
		ProcessLanQuery(from);
		processed = true;
		break;






	case eH_ConnectionSetup:
		ProcessSetup(from, pData, nLength);
		processed = true;
		break;
	case eH_ConnectionSetup_KeyExchange_0:
		ProcessKeyExchange0(from, pData, nLength);
		processed = true;
		break;
	case eH_ConnectionSetup_KeyExchange_1:
		ProcessKeyExchange1(from, pData, nLength);
		processed = true;
		break;
	case eH_Disconnect:
		ProcessDisconnect(from, pData, nLength);
		processed = true;
		break;
	case eH_DisconnectAck:
		ProcessDisconnectAck(from);
		processed = true;
		break;
	case eH_AlreadyConnecting:
		ProcessAlreadyConnecting(from, pData, nLength);
		processed = true;
		break;
#if NETWORK_REBROADCASTER
	case eH_Rebroadcaster:
		{
			// Intercept rebroadcaster header and redirect as appropriate
			const SRebroadcasterFrameHeader* pHeader = reinterpret_cast<const SRebroadcasterFrameHeader*>(pData);
			CCryRebroadcaster* pRebroadcaster = NULL;
			CCryLobby* pLobby = (CCryLobby*)CCryLobby::GetLobby();
			if (pLobby)
			{
				pRebroadcaster = pLobby->GetRebroadcaster();
			}

			TNetChannelID localChannelID;
			if (pRebroadcaster)
			{
				localChannelID = pRebroadcaster->GetLocalChannelID();
			}

			if (pHeader->destinationID != localChannelID)
			{
				if (CNetwork::Get()->IsRebroadcasterEnabled())
				{
					// This packet still needs forwarding
					TNetChannelID routeChannelID = 0;
					if (pRebroadcaster)
					{
						routeChannelID = pRebroadcaster->FindRoute(localChannelID, pHeader->destinationID);
					}

					if (routeChannelID != 0)
					{
						TNetAddress to;
						if (pRebroadcaster && pRebroadcaster->GetAddressForChannelID(routeChannelID, to))
						{
							SendTo(pData, nLength, to);

#if NETWORK_REBROADCASTER_LOG
							static CTimeValue lastUpdate(0.0f);
							if (g_time - lastUpdate > 0.5f)
							{
								NetQuickLog(false, 5, "Rebroadcaster: packet rerouted on machine %i: originally from %i to %i; routing %s %i", localChannelID, pHeader->sourceID, pHeader->destinationID, ((routeChannelID == pHeader->destinationID) ? "to" : "via"), routeChannelID);
								lastUpdate = g_time;
							}
#endif
						}
						else
						{
							NetWarning("Rebroadcaster: *WARNING* unable to determine address for channel id %i in CNetNub::OnPacket() (forwarding) - packet discarded", routeChannelID);
						}
					}
				}

				// If the packet can't be routed on for some reason, simply consume it here 
			}
			else
			{
				// Fudge the 'from' address to look like it came directly from the source
				TNetAddress source;
				if (pRebroadcaster && pRebroadcaster->GetAddressForChannelID(pHeader->sourceID, source))
				{
#if NETWORK_REBROADCASTER_LOG
					static CTimeValue lastUpdate(0.0f);
					if (g_time - lastUpdate > 0.5f)
					{
						NetQuickLog(false, 5, "Rebroadcaster: packet arrived on machine %i after %i hops: originally from %i", pHeader->destinationID, pHeader->hopCount, pHeader->sourceID);
						lastUpdate = g_time;
					}
#endif

					// Strip the rebroadcaster header
					pData += sizeof(SRebroadcasterFrameHeader);
					nLength -= sizeof(SRebroadcasterFrameHeader);

					OnPacket(source, pData, nLength);
				}
				else
				{
					NetWarning("Rebroadcaster: *WARNING* unable to determine address for channel id %i in CNetNub::OnPacket() (forwarding) - packet discarded", pHeader->sourceID);
				}

			}

			processed = true;
		}
		break;
#endif
	}

	if (!processed)
	{
		TraceUnrecognizedPacket( "ProcessPacketFrom", (uint8*)pData, nLength, from );
	}
}

void CNetNub::ProcessSimplePacket( const TNetAddress& from, const uint8 * pData, uint32 nLength )
{
	if (nLength != 1)
		return;

	uint8 nType = Frame_HeaderToID[pData[0]];

	TDisconnectMap::iterator iterDis = m_disconnectMap.find(from);
	if (iterDis != m_disconnectMap.end())
		SendDisconnect( from, iterDis->second );
	else if (CNetChannel * pNetChannel = GetChannelForIP(from, 0))
		pNetChannel->ProcessSimplePacket( (EHeaders)nType );
}

bool CNetNub::ProcessTransportPacketFrom( const TNetAddress& from, const uint8 * pData, uint32 nLength )
{
//	NET_ASSERT(  == CTPFrameHeader );

//	uint8 type = GetFrameType(((uint8*)MMM_PACKETDATA.PinHdl(hdl))[0]);
//	NET_ASSERT( type == CTPFrameHeader || type == CTPSyncFrameHeader );
	uint8 nType = Frame_HeaderToID[pData[0]];

	TDisconnectMap::iterator iterDis = m_disconnectMap.find(from);
	if (iterDis != m_disconnectMap.end())
	{
		SendDisconnect( from, iterDis->second );
		return true;
	}

	CNetChannel * pNetChannel = GetChannelForIP(from, 0);

	if (pNetChannel)
	{
		pNetChannel->ProcessPacket( (nType>=eH_SyncTransportSeq0 && nType<=eH_SyncTransportSeq_Last), pData, nLength );
		return true;
	}

	// legal packet, but we don't know what to do with it yet
	return true;
}

void CNetNub::ProcessDisconnectAck( const TNetAddress& from )
{
	ASSERT_GLOBAL_LOCK;
	m_disconnectMap.erase(from);
}

void CNetNub::ProcessDisconnect( const TNetAddress& from, const uint8 * pData, uint32 nSize )
{
	CNetChannel * pNetChannel = GetChannelForIP(from, 0);
	uint8 cause = eDC_Unknown;
	string reason;

#if NETWORK_REBROADCASTER
	if (pNetChannel)
	{
		CCryRebroadcaster* pRebroadcaster = NULL;
		CCryLobby* pLobby = (CCryLobby*)CCryLobby::GetLobby();
		if (pLobby)
		{
			pRebroadcaster = pLobby->GetRebroadcaster();
		}

		if (pRebroadcaster)
		{
			pRebroadcaster->DeleteConnection(pNetChannel, true, gEnv->bServer);
		}
	}
#endif
	
	if (nSize < 2)
	{
		// this is definitely a corrupted packet, disconnect the channel, if any
		cause = eDC_ProtocolError;
		reason = "Corrupted disconnect packet";
		if (pNetChannel && !pNetChannel->IsDead())
			DisconnectChannel(eDC_ProtocolError, &from, pNetChannel, reason.c_str());
	}
	else
	{
		std::vector<string> r;
		cause = pData[1];
		STATIC_CHECK((eDC_Timeout == 0), eDC_Timeout_NOT_EQUAL_0);//fix compiler warning
		if (/*cause < eDC_Timeout || */cause > eDC_Unknown)//first term not required anymore (due to previous line)
		{
			cause = eDC_ProtocolError;
			reason = "Illegal disconnect packet";
		}
		else
		{
			string backlog((char*)pData+2, (char*)pData+nSize);
			ParseBacklogString(backlog, reason, r);
			if (reason.empty())
				reason = "No reason";
		}
		if (pNetChannel && !pNetChannel->IsDead())
		{
			NetLog( "Received disconnect packet from %s: %s", RESOLVER.ToString(from).c_str(), reason.c_str() );
			for (size_t i = 0; i < r.size(); ++i)
				NetLog("  [remote] %s", r[i].c_str());
			DisconnectChannel( (EDisconnectionCause)cause, &from, pNetChannel, ("Remote disconnected: " + reason).c_str() );
		}
	}

	// if we received a disconnect packet without a channel, we should try removing pre-channel record possibly in
	// m_pendingConnections or m_connectingMap
	if (!pNetChannel)
	{
		// active connect, need to inform GameNub
		for (TPendingConnectionSet::iterator iter = m_pendingConnections.begin(); iter != m_pendingConnections.end(); ++iter)
		{
			if (iter->to == from)
			{
				NetWarning("Connection to %s refused", RESOLVER.ToString(from).c_str());
				NetWarning("%s", reason.c_str());
				TO_GAME( &CNetNub::GC_FailedActiveConnect, this, (EDisconnectionCause)cause, RESOLVER.ToString(from).c_str() + string(" refused connection\n") + reason, CNubKeepAliveLock(this) );
				m_pendingConnections.erase(iter);
				break;
			}
		}

		TConnectingMap::iterator iter = m_connectingMap.find(from);
		if (iter != m_connectingMap.end())
		{
			NetWarning("%s disconnected pre-maturely", RESOLVER.ToString(from).c_str());
			NetWarning("%s", reason.c_str());
			m_connectingMap.erase(iter);
			// no need to add a disconnect entry, since it's a remote disconnect
		}
	}

	AckDisconnect( from );
}

void CNetNub::AckDisconnect( const TNetAddress& to )
{
	TAckDisconnectSet::iterator it = m_ackDisconnectSet.lower_bound(to);
	if (it == m_ackDisconnectSet.end() || !equiv(to, *it))
	{
		SendTo( &Frame_IDToHeader[eH_DisconnectAck], 1, to );
		m_ackDisconnectSet.insert( it, to );
	}
}

void CNetNub::GC_FailedActiveConnect(EDisconnectionCause cause, string description, CNubKeepAliveLock)
{
	// this function is executed on the game thread with the global lock held
	//SCOPED_GLOBAL_LOCK; - no need to lock

	m_pGameNub->FailedActiveConnect(cause, description.c_str());
}

void CNetNub::ProcessAlreadyConnecting( const TNetAddress& from, const uint8 * pData, uint32 nSize )
{
	NetLog("Started connection to %s [waiting for data]", RESOLVER.ToString(from).c_str());
	for (TPendingConnectionSet::iterator iter = m_pendingConnections.begin(); iter != m_pendingConnections.end(); ++iter)
	{
		if (iter->to == from)
		{
			iter->connectCounter = -5;
			break;
		}
	}
}

void CNetNub::DisconnectChannel( EDisconnectionCause cause, const TNetAddress* pFrom, CNetChannel * pChannel, const char * reason )
{
	if (!pFrom)
	{
		NET_ASSERT(pChannel);
		for (TChannelMap::const_iterator iter = m_channels.begin(); iter != m_channels.end(); ++iter)
			if (iter->second == pChannel)
				pFrom = &iter->first;
	}
	if (!pChannel)
	{
		NET_ASSERT(pFrom);
		pChannel = GetChannelForIP(*pFrom, 0);
	}

#if NETWORK_REBROADCASTER
	if (pChannel)
	{
		CCryRebroadcaster* pRebroadcaster = NULL;
		CCryLobby* pLobby = (CCryLobby*)CCryLobby::GetLobby();
		if (pLobby)
		{
			pRebroadcaster = pLobby->GetRebroadcaster();
		}

		if (pRebroadcaster)
		{
			pRebroadcaster->DeleteConnection(pChannel, false, gEnv->bServer);
		}
	}
#endif
	if (pChannel)
	{
		if (!pChannel->GetContextView()->IsPeer())
		{
			// Only disconnect the game if we're not a peer
			pChannel->DisconnectGame( cause, reason );
		}
		pChannel->Die();

		for (TPendingConnectionSet::iterator iter = m_pendingConnections.begin(); iter != m_pendingConnections.end(); ++iter)
		{
			if (iter->pChannel == pChannel)
			{
				m_pendingConnections.erase(iter);
				break;
			}
		}
	}

	if (cause == eDC_ContextCorruption && CVARS.LogLevel > 0)
	{
		CNetwork::Get()->BroadcastNetDump(eNDT_ObjectState);
		CNetwork::Get()->AddExecuteString("es_dump_entities");
	}

	if (pFrom)
	{
		AddDisconnectEntry( *pFrom, pChannel ? pChannel->GetSession() : CrySessionInvalidHandle, cause, reason );
	}
}

void CNetNub::DisconnectChannel(EDisconnectionCause cause, CrySessionHandle session, const char * reason)
{
	CCryMatchMaking* matchmaking = (CCryMatchMaking*)m_pLobby->GetMatchMaking();

	if (matchmaking)
	{
		CryMatchMakingConnectionUID uid = matchmaking->GetConnectionUIDFromGameSessionHandle(session);

		for (TChannelMap::const_iterator iter = m_channels.begin(); iter != m_channels.end(); ++iter)
		{
			CNetChannel* channel = iter->second->GetNetChannel();

			if (matchmaking->GetConnectionUIDFromGameSessionHandle(channel->GetSession()) == uid)
			{
				DisconnectChannel(cause, &iter->first, channel, reason);
			}
		}
	}
}

void CNetNub::NetDump( ENetDumpType type )
{
	for (TChannelMap::const_iterator iter = m_channels.begin(); iter != m_channels.end(); ++iter)
		iter->second->NetDump(type);
}

void CNetNub::SendDisconnect( const TNetAddress& to, SDisconnect& dis )
{
	CTimeValue now = g_time;
	if (dis.lastNotify + 0.5f >= now)
		return;

	uint8 buffer[2 + sizeof(dis.info) + sizeof(dis.backlog)]; // frame + cause + reason + backlog
	NET_ASSERT(sizeof(buffer) <= MAX_TRANSMISSION_PACKET_SIZE);
	buffer[0] = Frame_IDToHeader[eH_Disconnect];
	buffer[1] = dis.cause;
	memcpy( buffer+2, dis.info, dis.infoLength );
	memcpy( buffer+2+dis.infoLength, dis.backlog, dis.backlogLen );
	SendTo( buffer, 2+dis.infoLength+dis.backlogLen, to );

	dis.lastNotify = now;
}

bool CNetNub::ProcessPingQuery( const TNetAddress& from, const uint8 * pData, uint32 nSize )
{
	//NET_ASSERT( GetFrameType(((uint8*)MMM_PACKETDATA.PinHdl(hdl))[0]) == QueryFrameHeader );

	if (!m_pGameQuery)
	{
		return false;
	}

	static const size_t BufferSize = 64;
	uint8 buffer[BufferSize];
	if (nSize > BufferSize)
	{
		return false;
	}
	memcpy( buffer, pData, nSize );
	buffer[1] = 'O'; // PONG
	m_pSocketMain->Send( buffer, nSize, from );
	return true;
}

void CNetNub::ProcessLanQuery( const TNetAddress& from )
{
	NET_ASSERT( m_pGameQuery );

	XmlNodeRef xml = m_pGameQuery->GetGameState();
	XmlString str = xml->getXML();
	m_pSocketMain->Send( (const uint8*)str.c_str(), str.length(), from );
}

// EvenBalance - M. Quinn
bool CNetNub::ProcessPunkBuster( const TNetAddress& from, const uint8 * pData, uint32 nSize )
{











































	return false;


}

void CNetNub::OnError( const TNetAddress& from, ESocketError err )
{
	NetWarning( "%d from %s", err, RESOLVER.ToString(from).c_str() );
	CNetChannel * pNetChannel = GetChannelForIP(from, eCLF_HandleBrokenWindowsICMP);
	if (pNetChannel)
	{
		if (err == eSE_FragmentationOccured)
			pNetChannel->FragmentedPacket();
		else if (err == eSE_UnreachableAddress)
			pNetChannel->Disconnect( eDC_ICMPError, ("Unreachable address " + RESOLVER.ToString(from)).c_str() );
		else if (err >= eSE_Unrecoverable)
			pNetChannel->Disconnect( eDC_Unknown, "Unknown unrecoverable error" );
	}

	for (TPendingConnectionSet::iterator iter = m_pendingConnections.begin(); iter != m_pendingConnections.end(); ++iter)
	{
		bool done = false;
		for (TNetAddressVec::const_iterator it = iter->tos.begin(); it != iter->tos.end(); ++it)
		{
			if (*it == from)
			{
				TO_GAME( &CNetNub::GC_FailedActiveConnect, this, eDC_CantConnect, string().Format("Connection attempt to %s failed", RESOLVER.ToString(from).c_str()), CNubKeepAliveLock(this) );
				m_pendingConnections.erase(iter);
				done = true;
				break;
			}
		}
		if (done)
			break;
	}

	TConnectingMap::iterator iterCon = LookupAddress(from, m_connectingMap, eCLF_HandleBrokenWindowsICMP);
	if (iterCon != m_connectingMap.end())
		m_connectingMap.erase(iterCon);
}

bool CNetNub::SendKeyExchange0(const TNetAddress& to, SConnecting& con, bool doNotRegenerate)
{
	static const size_t KS = sizeof(CExponentialKeyExchange::KEY_TYPE);
	uint8 pktbuf[1 + 3*KS + sizeof(uint32)];

/*
#pragma pack(push, 1)
	struct SKeyExchange0
	{
		uint8 frameHeader;
		CExponentialKeyExchange::KEY_TYPE g, p, A;
	};
#pragma pack(pop)
*/

	pktbuf[0] = Frame_IDToHeader[eH_ConnectionSetup_KeyExchange_0];
	if (doNotRegenerate)
	{
	}
	else
	{
		con.exchange.Generate(con.g, con.p, con.A);
	}
	memcpy(pktbuf+1, &con.g, KS);
	memcpy(pktbuf+1+KS, &con.p, KS);
	memcpy(pktbuf+1+2*KS, &con.A, KS);
	uint32 socketCaps = htonl(CNetwork::Get()->GetSocketIOManager().caps);
	memcpy(pktbuf+1+3*KS, &socketCaps, sizeof(uint32));
	return SendTo(pktbuf, sizeof(pktbuf), to);
}

bool CNetNub::SendKeyExchange1(SPendingConnection& pc, bool doNotRegenerate)
{
	static const size_t KS = sizeof(CExponentialKeyExchange::KEY_TYPE);
	uint8 pktbuf[1 + KS];

	pktbuf[0] = Frame_IDToHeader[eH_ConnectionSetup_KeyExchange_1];
	if (doNotRegenerate)
	{
	}
	else
	{
		pc.exchange.Generate(pc.B, pc.g, pc.p, pc.A);
	}
	assert(sizeof(pc.B) == KS);
	memcpy(pktbuf+1, &pc.B, KS);
	return SendTo(pktbuf, sizeof(pktbuf), pc.to);
}

void CNetNub::ProcessKeyExchange0(const TNetAddress& from, const uint8 * pData, uint32 nSize)
{
	if (GetChannelForIP(from,0) != NULL)
		return;

	for (TPendingConnectionSet::iterator iter = m_pendingConnections.begin(); iter != m_pendingConnections.end(); ++iter)
	{
		if ( !(iter->to == from) )
		{
			for (TNetAddressVec::const_iterator it = iter->tos.begin(); it != iter->tos.end(); ++it)
			{
				if (*it == from)
				{
					iter->to = from;
					goto L_continue;
				}
			}

			continue;
		}

L_continue:

		if (iter->kes != eKES_SetupInitiated)
			break;

		if (nSize != 1+3*CExponentialKeyExchange::KEY_SIZE+sizeof(uint32))
		{
			NetWarning("processing KeyExchange0 with illegal sized packet");
			break;
		}

		++pData;
		memcpy(iter->g.key, pData, CExponentialKeyExchange::KEY_SIZE);
		pData += CExponentialKeyExchange::KEY_SIZE;
		memcpy(iter->p.key, pData, CExponentialKeyExchange::KEY_SIZE);
		pData += CExponentialKeyExchange::KEY_SIZE;
		memcpy(iter->A.key, pData, CExponentialKeyExchange::KEY_SIZE);
		pData += CExponentialKeyExchange::KEY_SIZE;
		memcpy(&iter->socketCaps, pData, sizeof(uint32));
		pData += sizeof(uint32);

		iter->socketCaps = ntohl(iter->socketCaps);

		if ( !SendKeyExchange1(*iter) )
		{
			NetWarning("failed sending KeyExchange1");
			break;
		}

		iter->kes = eKES_KeyEstablished;
		iter->kesStart = g_time;
		CreateChannel( iter->to, iter->exchange.GetKey(), "" /* NULL */, iter->socketCaps, iter->profile, CrySessionInvalidHandle, iter->conlk ); // the empty string ("") will be translated to NULL when creating GameChannel
		break;
	}
}

void CNetNub::ProcessKeyExchange1(const TNetAddress& from, const uint8 * pData, uint32 nSize)
{
	if (GetChannelForIP(from,0) != NULL)
		return;

	TConnectingMap::iterator iterCon = m_connectingMap.find(from);
	if (iterCon == m_connectingMap.end())
	{
		AddDisconnectEntry( from, CrySessionInvalidHandle, eDC_CantConnect, "KeyExchange1 with no connection" );
		return; // ignore silently
	}

	if (iterCon->second.kes != eKES_SentKeyExchange)
		return;

	{
		if (nSize != 1+CExponentialKeyExchange::KEY_SIZE)
		{
			NetWarning("processing KeyExchange1 with illegal sized packet");
			AddDisconnectEntry( from, iterCon->second.session, eDC_CantConnect, "processing KeyExchange1 with illegal sized packet" );
			return;
		}

		++pData;
		memcpy(iterCon->second.B.key, pData, CExponentialKeyExchange::KEY_SIZE);
	}

	iterCon->second.exchange.Calculate(iterCon->second.B);
	iterCon->second.kes = eKES_KeyEstablished;
	iterCon->second.kesStart = g_time;

	// now that we have established the private key, let's go create the channel
	CreateChannel(from, iterCon->second.exchange.GetKey(), iterCon->second.connectionString, iterCon->second.socketCaps, iterCon->second.profile, iterCon->second.session, CNubConnectingLock());
}

void CNetNub::ProcessSetup( const TNetAddress& from, const uint8 * pData, uint32 nSize )
{
	//	if (m_pSecurity->IsIPBanned(from.GetAsUINT()))
	//	{
	//		NetWarning("Setup discarded for %s (BANNED)", RESOLVER.ToString(from));
	//		return;
	//	}

	if (GetChannelForIP(from,0) != NULL)
		return;

	if (nSize < 1+VERSION_SIZE*sizeof(uint32)+2*sizeof(uint32)+sizeof(CrySessionHandle))
	{
		AddDisconnectEntry(from, CrySessionInvalidHandle, eDC_ProtocolError, "Too short connection packet");
		return;
	}

	SFileVersion ver = gEnv->pSystem->GetProductVersion();
	uint32 v[VERSION_SIZE];
	memcpy(v, pData+1, VERSION_SIZE*sizeof(uint32));

	uint32 profile_data = 0;
	memcpy(&profile_data,(char*)pData+1+VERSION_SIZE*sizeof(uint32), sizeof(uint32));
	uint32 profile = ntohl(profile_data);

	memcpy(&profile_data,(char*)pData+1+VERSION_SIZE*sizeof(uint32)+sizeof(uint32), sizeof(uint32));
	uint32 tokenId = ntohl(profile_data);

	CrySessionHandle session;
	memcpy(&session, (char*)pData + 1 + VERSION_SIZE*sizeof(uint32) + 2*sizeof(uint32), sizeof(CrySessionHandle));
	session = ntohl(session);

	for (int i=0; i<4; i++)
	{
		if (v[i] != ntohl(ver.v[i]))
		{
			AddDisconnectEntry(from, session, eDC_VersionMismatch, "Build version mismatch");
			m_connectingMap.erase(from);
			return;
		}
	}
	if (v[4] != ntohl(PROTOCOL_VERSION))
	{
		AddDisconnectEntry(from, session, eDC_VersionMismatch, "Protocol version mismatch");
		m_connectingMap.erase(from);
		return;
	}

	if (!from.GetPtr<TLocalNetAddress>())
	{
		INetworkServicePtr pNetTokens = CNetwork::Get()->GetService("NetProfileTokens");

		if (pNetTokens && (eNSS_Ok == pNetTokens->GetState()))
		{
			if (!pNetTokens->GetNetProfileTokens()->IsValid(profile, tokenId))
			{
				AddDisconnectEntry(from, session, eDC_VersionMismatch, "Client token id mismatch");
				m_connectingMap.erase(from);
				return;
			}
		}
	}
	string connectionString( (char*)pData+1+VERSION_SIZE*sizeof(uint32)+2*sizeof(uint32) + sizeof(CrySessionHandle), (char*)pData+nSize );

	bool doNotRegenerate = false;
	TConnectingMap::iterator iterCon = m_connectingMap.find(from);
	if (iterCon != m_connectingMap.end())
	{
		switch (iterCon->second.kes)
		{
		case eKES_NotStarted:
		case eKES_SetupInitiated:
			AddDisconnectEntry( from, session, eDC_ProtocolError, "Key establishment handshake ordering error" );
			return;
		case eKES_SentKeyExchange:
			break;
		case eKES_KeyEstablished:
			return; // ignore
		}

		// we are in eKES_SentKeyExchange state already while we get a ConnectionSetup message, probably
		// our KeyExchange0 message was lost, resend it
		doNotRegenerate = true; // do not regenerate keys
	}
	else
		iterCon = m_connectingMap.insert( std::make_pair(from, SConnecting()) ).first;

	SendKeyExchange0( from, iterCon->second, doNotRegenerate );

	CCryMatchMaking* pMatchMaking = (CCryMatchMaking*)m_pLobby->GetMatchMaking();

	if (pMatchMaking && gEnv->bMultiplayer)
	{
		TNetChannelID id;
		bool addressFound = pMatchMaking->GetChannelIDFromAddress(from, &id);

		if (addressFound)
		{
			// m_disconnectMap can sometimes still hold this address if client disconnects and connect again quickly and some of the disconnect packets got lost.
			// If this happens this client will be disconnected straight away as we think it should have been disconnected.
			// m_disconnectMap isn't really needed when CryMatchMaking is in use as CryMatchMaking handles connects and disconnects
			// but we still need to keep it in case CryMatchMaking isn't in use.
			// What we can say is if CryMatchMaking knows of this address then it doesn't want to be disconnected because of m_disconnectMap so make sure it is not stored.
			m_disconnectMap.erase(from);
		}

		if (session == CrySessionInvalidHandle)
		{
			// If there is a CCryMatchMaking service in use then CryLobby is being used on this server.
			// If session is invalid then the client connecting isn't using CryLobby.

			if (addressFound)
			{
				// If CryLobby knows about this address then use id associated with it.
				session = id;
			}
			else
			{
				// If not generate a session handle with a valid CryMatchMakingConnectionUID now.
				session = pMatchMaking->CreateGameSessionHandle(CrySessionInvalidHandle, pMatchMaking->CreateConnectionUID());
			}
		}
	}

	iterCon->second.connectionString = connectionString;
	iterCon->second.kes = eKES_SentKeyExchange;
	iterCon->second.socketCaps = ntohl(v[5]);
	iterCon->second.profile = profile;
	iterCon->second.session = session;
	if (!doNotRegenerate)
		iterCon->second.kesStart = g_time;
}

CNetChannel * CNetNub::GetChannelForIP( const TNetAddress& ip, uint32 flags )
{
	ASSERT_GLOBAL_LOCK;
	TChannelMap::iterator i = LookupAddress( ip, m_channels, flags );	
	return i == m_channels.end() ? NULL : i->second->GetNetChannel();
}

TNetAddress CNetNub::GetIPForChannel( const INetChannel* ch)
{
  for(TChannelMap::iterator i = m_channels.begin(),e=m_channels.end();i!=e;++i)
  {
    if(i->second->GetNetChannel()==ch)
      return i->first;
  }
  return TNetAddress();
}

void CNetNub::GetMemoryStatistics(ICrySizer *pSizer)
{
	SIZER_COMPONENT_NAME(pSizer, "CNetNub");

	if ( !pSizer->Add(*this) )
		return;
	pSizer->AddContainer(m_channels);
	for (TChannelMap::iterator i = m_channels.begin(); i != m_channels.end(); ++i)
	{
		if (CNetChannel * pNetChannel = i->second->GetNetChannel())
			pNetChannel->GetMemoryStatistics( pSizer, true );
	}
	m_pSocketMain->GetMemoryStatistics(pSizer);
}

void CNetNub::CreateChannel(const TNetAddress& ip, const CExponentialKeyExchange::KEY_TYPE& key, const string& connectionString, uint32 remoteSocketCaps, uint32 proifle, CrySessionHandle session, CNubConnectingLock conlk)
{
	// this function is executed on the network thread

	CNetChannel * pNetChannel = new CNetChannel( ip, key, remoteSocketCaps, session ); // I don't want to pass the 32 bytes key by value between threads
	pNetChannel->SetProfileId(proifle);

	TO_GAME( &CNetNub::GC_CreateChannel, this, pNetChannel, connectionString, conlk );
}

void CNetNub::GC_CreateChannel( CNetChannel* pNetChannel, string connectionString, CNubConnectingLock conlk )
{
	// this function is executed on the game thread with the global lock held
	// see CSystem::Update -> CNetwork::SyncWithGame
	//SCOPED_GLOBAL_LOCK; - no need to lock

	TConnectingMap::iterator iterCon = m_connectingMap.find( pNetChannel->GetIP() );
	bool fromRequest = iterCon != m_connectingMap.end();
  
  if (!fromRequest)
  {
    bool pending = false;
    for (TPendingConnectionSet::iterator iter = m_pendingConnections.begin(); iter != m_pendingConnections.end(); ++iter)
    {
      if (iter->to == pNetChannel->GetIP())
      {
        iter->pChannel = pNetChannel;
        pending = true;
        break;
      }
    }

    if (!pending)
    {
      // the IP associated with the NetChannel is not tracked (we didn't find it in either list), probably it gets removed pre-maturely
			AddDisconnectEntry( pNetChannel->GetIP(), pNetChannel->GetSession(), eDC_CantConnect, "Pending connection not found" );
      delete pNetChannel;
      return;
    }
  }

	SCreateChannelResult res = m_pGameNub->CreateChannel( pNetChannel, connectionString.empty() ? NULL : connectionString.c_str() );
	if (!res.pChannel)
	{
		if (fromRequest)
			m_connectingMap.erase(iterCon);
		AddDisconnectEntry( pNetChannel->GetIP(), pNetChannel->GetSession(), res.cause, res.errorMsg);
		delete pNetChannel;
		return;
	}

#if NETWORK_REBROADCASTER
	if (reinterpret_cast<INetChannel*>(res.pChannel) == pNetChannel)
	{
		// This is a rebroadcaster channel
		res.pChannel = NULL;
	}

	if (!gEnv->bServer)
	{
		// Pure clients need to associate the INetChannel* with the address stored
		// in the rebroadcaster mesh (different on each client)
		CNetwork::Get()->AddRebroadcasterConnection(pNetChannel, 0);
	}
#endif

	pNetChannel->Init( this, res.pChannel );

#if USE_DEFENCE
	std::for_each( m_defenceData.m_listProt.begin(),
		m_defenceData.m_listProt.end(), SAddProtectedFile(pNetChannel) );
#endif //NOT_USE_DEFENCE

	ASSERT_GLOBAL_LOCK;
	m_channels.insert( std::make_pair(pNetChannel->GetIP(), pNetChannel) );
	if (fromRequest)
		m_connectingMap.erase(iterCon);
}

void CNetNub::SendConnecting( const TNetAddress& to, SConnecting& con )
{
	CTimeValue now = g_time;
	if (con.lastNotify + 0.5f >= now)
		return;

	SendTo( Frame_IDToHeader + eH_AlreadyConnecting, 1, to );

	con.lastNotify = now;
}

void CNetNub::AddDisconnectEntry( const TNetAddress& ip, CrySessionHandle session, EDisconnectionCause cause, const char * reason )
{
	CCryMatchMaking* pMatchmaking = (CCryMatchMaking*)m_pLobby->GetMatchMaking();
	SDisconnect dc;
	dc.when = g_time;
	dc.lastNotify = 0.0f;
	dc.infoLength = strlen(reason);
	dc.cause = cause;
	if (dc.infoLength > sizeof(dc.info))
		dc.infoLength = sizeof(dc.info);
	memcpy(dc.info, reason, dc.infoLength);
	string backlog = GetBacklogAsString();
	dc.backlogLen = backlog.size();
	if (dc.backlogLen > sizeof(dc.backlog))
		dc.backlogLen = sizeof(dc.backlog);
	memcpy(dc.backlog, backlog.data(), dc.backlogLen);

	TDisconnectMap::iterator iterDis = m_disconnectMap.insert( std::make_pair(ip, dc) ).first;

	if (cause != eDC_ICMPError)	// If we are here because of eDC_ICMPError from a send then the send here will bring us here again and cause a stack overflow.
	{
		SendDisconnect( ip, iterDis->second );
	}

#if NETWORK_HOST_MIGRATION
	// Retain all matchmaking information across host migration
	bool autoMigrate = (gEnv->pConsole->GetCVar("net_automigrate_host")->GetIVal() != 0);
	bool isHost = gEnv->bServer;
	TNetChannelID hostChannelID = 0;

	if (pMatchmaking)
	{
		hostChannelID = (TNetChannelID)pMatchmaking->GetHostConnectionUID();
	}

	bool isHostStillPlaying = false;
	CCryRebroadcaster* pRebroadcaster = ((CCryLobby*)CCryLobby::GetLobby())->GetRebroadcaster();
	if (pRebroadcaster)
	{
		isHostStillPlaying = pRebroadcaster->IsInRebroadcaster(hostChannelID);
	}

	// Only do disconnects when not host migrating, not a server, or a server that's still playing...
	if ((cause == eDC_ICMPError) || (!gEnv->bHostMigrating && ((isHost && ((!isHostStillPlaying && !autoMigrate) || (isHostStillPlaying))) || (!isHost))))
#endif
	{
		if (pMatchmaking)
		{
			pMatchmaking->SessionDisconnectRemoteConnection(session, ip);
		}
	}
}

bool CNetNub::IsSuicidal()
{
	if (IsDead())
		return true;
	if (m_keepAliveLocks)
		return true;
	if (m_bDead)
	{
		bool allChannelsSuicidal = true;
		for (TChannelMap::iterator iter = m_channels.begin(); iter != m_channels.end(); ++iter)
		{
			allChannelsSuicidal &= iter->second->IsSuicidal();
		}
		if (allChannelsSuicidal)
			return true;
	}
	return false;
}

CNetChannel * CNetNub::FindFirstClientChannel()
{
	for (TChannelMap::iterator iter = m_channels.begin(); iter != m_channels.end(); ++iter)
	{
		if (CNetChannel * pChan = iter->second->GetNetChannel())
			if (!pChan->IsServer())
				return pChan;
	}
	return 0;
}

#include UNIQUE_VIRTUAL_WRAPPER(IGameSecurity)
#include UNIQUE_VIRTUAL_WRAPPER(INetNub)
