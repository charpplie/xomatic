#include "StdAfx.h"
#include "CET_CVars.h"
#include "GameServerChannel.h"
#include "NetHelpers.h"

class CConsoleVarSender : public ICVarDumpSink
{
public:
	CConsoleVarSender( IConsoleVarSink * pSink ) : m_pSink(pSink) {}

	virtual void OnElementFound( ICVar *pCVar )
	{
		if (pCVar->GetFlags() & (VF_NOT_NET_SYNCED_INTERNAL | VF_REQUIRE_APP_RESTART))
			return;
		m_pSink->OnAfterVarChange(pCVar);
	}

private:
	IConsoleVarSink * m_pSink;
};

class CCET_CVarSync : public CCET_Base
{
public:
	CCET_CVarSync( CGameServerChannel* pChannel ) : m_pChannel(pChannel) {}

	const char * GetName() { return "CVarSync"; }

	EContextEstablishTaskResult OnStep( SContextEstablishState& state )
	{
		CBatchSyncVarsSink pSink(m_pChannel);
		CConsoleVarSender cvarSender( &pSink );
		gEnv->pConsole->DumpCVars( &cvarSender, 0 );
		pSink.OnAfterVarChange(0);
		return eCETR_Ok;
	}

private:
	CGameServerChannel* m_pChannel;
};

void AddCVarSync( IContextEstablisher * pEst, EContextViewState state, CGameServerChannel* pChannel )
{
	pEst->AddTask( state, new CCET_CVarSync(pChannel) );
}


CBatchSyncVarsSink::CBatchSyncVarsSink(CGameServerChannel* pChannel)
{
	m_pChannel = pChannel;
}

// IConsoleVarSink
bool CBatchSyncVarsSink::OnBeforeVarChange( ICVar *pVar,const char *sNewValue )
{
	return true;
}

void CBatchSyncVarsSink::OnAfterVarChange( ICVar *pVar )
{
	if ((0 == pVar) || (0 == (pVar->GetFlags() & VF_NOT_NET_SYNCED_INTERNAL)))
	{
		if (m_pChannel->GetNetChannel() && !m_pChannel->GetNetChannel()->IsLocal())
		{
			if ((0 == pVar) || !m_params.Add(pVar->GetName(), pVar->GetString()))
			{
				INetSendablePtr pSendable = new CSimpleNetMessage<SClientBatchConsoleVariablesParams>(m_params, CGameClientChannel::SetBatchConsoleVariables);
				pSendable->SetGroup('cvar');
				m_pChannel->GetNetChannel()->AddSendable( pSendable, 1, &m_consoleVarSendable, &m_consoleVarSendable );

				m_params.Reset();
				if (pVar != 0)
					m_params.Add(pVar->GetName(), pVar->GetString());
			}
		}
	}
}

