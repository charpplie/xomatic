#include "StdAfx.h"
#include "PlayerPlugin.h"
#include "Player.h"

#if PLAYER_PLUGIN_DEBUGGING
AUTOENUM_BUILDNAMEARRAY(CPlayerPlugin::s_playerPluginEventNames, PlayerPluginEventList);
#endif

CPlayerPlugin::CPlayerPlugin()
{
	m_ownerPlayer = NULL;
	m_entered = false;
	m_InRequiredSuitMode = false;
	m_requiredSuitMode = eNanoSuitMode_Last;
}

CPlayerPlugin::~CPlayerPlugin()
{
	CRY_ASSERT_MESSAGE(!m_entered, ("[PLAYER PLUG-IN] <%s %s \"%s\"> %s", m_ownerPlayer->IsClient() ? "Local" : "Remote", m_ownerPlayer->GetEntity()->GetClass()->GetName(), m_ownerPlayer->GetEntity()->GetName(), string().Format("Player plug-in is being destroyed without having been shut down cleanly!").c_str()));
	CRY_ASSERT_MESSAGE(!m_InRequiredSuitMode, ("[PLAYER PLUG-IN] <%s %s \"%s\"> %s", m_ownerPlayer->IsClient() ? "Local" : "Remote", m_ownerPlayer->GetEntity()->GetClass()->GetName(), m_ownerPlayer->GetEntity()->GetName(), string().Format("Player plug-in being destroyed without having been told it's no longer active!").c_str()));
}

void CPlayerPlugin::SetIsNowActive(bool nowOn)
{
	if (nowOn != m_InRequiredSuitMode)
	{
		PlayerPluginAssert (m_entered, "Shouldn't activate/deactivate a player plug-in which hasn't been entered!");
		m_InRequiredSuitMode = nowOn;
		InformActiveHasChanged();
	}
}

void CPlayerPlugin::Enter()
{
	PlayerPluginLog ("Entering");
	PlayerPluginAssert(m_entered == false, "Entering a player plug-in which is entered already!");
	PlayerPluginAssert(m_InRequiredSuitMode == false, "Already active even though only just being entered!");

	m_entered = true;

	HandleEvent(EPE_Reset);

	const SNanoSuitGameParameters& suitParams = m_ownerPlayer->GetActorSuitGameParameters();

	ENanoSuitState suitState = suitParams.GetState();
	HandleEvent(EPE_ChangedSuitState, &suitState);

	if(InRequiredSuitMode())
	{
		ENanoSuitMode suitMode = suitParams.GetMode();
		HandleEvent(EPE_ChangedSuitMode, &suitMode);
	}
}

void CPlayerPlugin::Leave()
{
	PlayerPluginLog ("Leaving");
	PlayerPluginAssert(m_entered == true, "Leaving a player plug-in which is not entered!");
	SetIsNowActive(false);
	m_entered = false;
}

void CPlayerPlugin::HandleEvent(EPlayerPlugInEvent theEvent, void * data)
{
	switch(theEvent)
	{
		case EPE_ChangedSuitMode:
		case EPE_ChangedSuitState:
		{
			if(m_ownerPlayer->HasNanoSuit())
			{
				const SNanoSuitGameParameters& nanosuitParams = m_ownerPlayer->GetActorSuitGameParameters();
				if(m_requiredSuitMode == nanosuitParams.GetMode() || m_requiredSuitMode == eNanoSuitMode_Last)
				{
					SetIsNowActive((nanosuitParams.GetState()&~eNanoSuitState_Disabled) != 0);
				}
				else
				{
					SetIsNowActive(false);
				}
			}
			break;
		}
	}
}

void CPlayerPlugin::SetRequiredSuitMode(ENanoSuitMode suitMode)
{
	m_requiredSuitMode = suitMode;
}

const void* CPlayerPlugin::DefaultData(EPlayerPlugInData dataType)
{
	const static bool m_defaultBool = false;
	const static EntityId m_defaultEntityId = 0;

	switch(dataType)
	{
		case EPD_SelfDestructMinigame:
		case EPD_Stamp:
		case EPD_MuteFootsteps:
		case EPD_MuteJumping:
		case EPD_ThreatIcons:
		{
			return &m_defaultBool;
		}
		case EPD_SelfDestructKiller:
		{
			return &m_defaultEntityId;
		}
	}

	CRY_ASSERT_MESSAGE(NULL, "We always want to provide default data");
	return NULL;
}

