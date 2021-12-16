/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios
-------------------------------------------------------------------------
History:
- 21:07:2009		Created by Tim Furnish
*************************************************************************/

#include "StdAfx.h"
#include "Audio/Announcer.h"
#include "Audio/GameAudio.h"
#include "GameRules.h"
#include "Utility/StringUtils.h"
#include "Utility/DesignerWarning.h"
#include "Player.h"
#include <ILocalizationManager.h>
#include <ISystem.h>
#include "HUD/UI/UIButtonPromptRegion.h"
#include <IItemSystem.h>

#define CHECK_FOR_THIS_MANY_DUPLICATED_STRINGS_WHEN_READING_ANNOUNCER_XML 128

#define AnnouncerConditionList(f)   \
	f(kAnnouncerCondition_self)       \
	f(kAnnouncerCondition_teammate)   \
	f(kAnnouncerCondition_enemy)      \
	f(kAnnouncerCondition_any)        \

AUTOENUM_BUILDFLAGS_WITHZERO(AnnouncerConditionList, kAnnouncerCondition_none);

static AUTOENUM_BUILDNAMEARRAY(s_conditionNames, AnnouncerConditionList);

CRY_TODO(26, 1, 2010, "kAnnouncerCondition_any isn't really any....");

CAnnouncer* CAnnouncer::s_announcer_instance = NULL;

const CAnnouncer* CAnnouncer::GetInstance()
{
	CRY_ASSERT(s_announcer_instance);
	return s_announcer_instance;
}

struct SAnnouncerAutoComplete : public IConsoleArgumentAutoComplete
{
	virtual int GetCount() const { return CAnnouncer::GetInstance()->GetCount(); };
	virtual const char* GetValue( int nIndex ) const { return CAnnouncer::GetInstance()->GetName(nIndex); };
};

int CAnnouncer::GetCount() const
{
	return m_annoucementList.size();
}

const char* CAnnouncer::GetName(int i) const
{
	return m_annoucementList[i].m_pName;
}

static SAnnouncerAutoComplete s_perkNameAutoComplete;

CAnnouncer::CAnnouncer()
{
	CRY_ASSERT(!s_announcer_instance);
	s_announcer_instance = this;

	IConsole * console = gEnv->pConsole;

	if (console)
	{
		console->AddCommand("announce", CAnnouncer::CmdAnnounce, 0, "Trigger a local announcement");
		console->RegisterAutoComplete("announce",				& s_perkNameAutoComplete);
	}

	m_canPlayFromTime = 0.0f;
}

CAnnouncer::~CAnnouncer()
{
	CRY_ASSERT(s_announcer_instance == this);
	s_announcer_instance = NULL;
}

void CAnnouncer::LoadAnnouncements(const char *filename)
{
	CryLog ("[ANNOUNCER] LoadAnnouncements('%s')", filename);

	XmlNodeRef xmlNode = GetISystem()->LoadXmlFile( filename );
	DesignerWarning (xmlNode, "Announcement definitions file '%s' could not be read!", filename);

	IItemParamsNode *paramNode = g_pGame->GetIGameFramework()->GetIItemSystem()->CreateParams();
	paramNode->ConvertFromXML(xmlNode);

	const int childCount = paramNode->GetChildCount();
	int iChild;

	CSingleAllocTextBlock::SReuseDuplicatedStrings duplicatedStringsWorkspace[CHECK_FOR_THIS_MANY_DUPLICATED_STRINGS_WHEN_READING_ANNOUNCER_XML];
	m_singleAllocTextBlock.SetDuplicatedStringWorkspace(duplicatedStringsWorkspace, CHECK_FOR_THIS_MANY_DUPLICATED_STRINGS_WHEN_READING_ANNOUNCER_XML);

	// Calculate how much memory we'll need to store all the strings...
	for (iChild = 0; iChild < childCount; ++iChild)
	{
		const IItemParamsNode * childXML = paramNode->GetChild(iChild);
		const char * tagType = childXML->GetName();
		const char * name = childXML->GetAttribute("Name");
		bool isAnnouncement = (stricmp("Announcement", tagType) == 0);

		DesignerWarning(isAnnouncement, "While reading '%s' found a '%s' tag when only expected to find 'Announcement' tags", filename, tagType);
		DesignerWarning(name, "While reading '%s' found a '%s' tag with no name!", filename, tagType);

		if (name && isAnnouncement)
		{
			const int childCount = childXML->GetChildCount();

			m_singleAllocTextBlock.IncreaseSizeNeeded(name);

			for (int iChild = 0; iChild < childCount; ++iChild)
			{
				const IItemParamsNode * optionXML = childXML->GetChild(iChild);

				if (0 == stricmp(optionXML->GetName(), "soundOption"))
				{
					for(int i = 0; i < k_maxAnnouncementAudioAmount; i++)
					{
						m_singleAllocTextBlock.IncreaseSizeNeeded(optionXML->GetAttribute(string().Format("sound%d", i)));
					}
				}
			}
		}
	}

	// Allocate memory for strings...
	m_singleAllocTextBlock.Allocate();

	// Now use allocated memory to store strings...
	for (iChild = 0; iChild < childCount; ++iChild)
	{
		const IItemParamsNode * childXML = paramNode->GetChild(iChild);
		const char * tagType = childXML->GetName();
		const char * name = childXML->GetAttribute("Name");
		bool isAnnouncement = (stricmp("Announcement", tagType) == 0);

		if (isAnnouncement && name)
		{
			const char * storedName = m_singleAllocTextBlock.StoreText(name);
			const char * conStr = childXML->GetAttribute("condition");
			EAnnounceConditions conditions = conStr ? AutoEnum_GetBitfieldFromString(conStr, s_conditionNames, AnnouncerConditionList_numBits) : kAnnouncerCondition_any;

			SOnScreenMessageDef onScreenMessage;
			const int childCount = childXML->GetChildCount();
			DesignerWarning(childCount > 0, "While reading '%s' found an announcement (name='%s' condition='%s') with no child tags!", filename, name, conStr);
			int numAnnouncementsAdded = 0;

			for (int iChild = 0; iChild < childCount; ++iChild)
			{
				const IItemParamsNode * optionXML = childXML->GetChild(iChild);
				const char * optionName = optionXML->GetName();
				if (0 == stricmp(optionName, "displayMessage"))
				{
					onScreenMessage.Read(optionXML);
				}
				else if (0 == stricmp(optionName, "soundOption"))
				{
					const char * sounds[k_maxAnnouncementAudioAmount];
					for(int i = 0; i < k_maxAnnouncementAudioAmount; i++)
					{
						sounds[i] = m_singleAllocTextBlock.StoreText(optionXML->GetAttribute(string().Format("sound%d", i)));
					}
					AddAnnoucement(storedName, conditions, sounds, onScreenMessage);
					numAnnouncementsAdded ++;
				}
				else
				{
					DesignerWarning(false, "While reading '%s' found an unexpected tag '%s' inside an announcement (name='%s' condition='%s')", filename, optionName, name, conStr);
				}
			}

			if (numAnnouncementsAdded == 0 && !onScreenMessage.empty())
			{
				const char * sounds[k_maxAnnouncementAudioAmount];
				memset (sounds, 0, sizeof(sounds));
				AddAnnoucement(storedName, conditions, sounds, onScreenMessage);
			}
		}
	}

	m_singleAllocTextBlock.Lock();
}

void CAnnouncer::AddAnnoucement(const char * pName, const EAnnounceConditions condition, const char * audio[k_maxAnnouncementAudioAmount], const SOnScreenMessageDef & onScreenMessage)
{
	SAnnouncementDef newAnnoucement;

	Crc32Gen* pKeyGen = gEnv->pSystem->GetCrc32Gen();

	newAnnoucement.m_announcementID = pKeyGen->GetCRC32(pName);
	newAnnoucement.m_pName = pName;
	newAnnoucement.m_conditions = condition;
	for(int i = 0; i < k_maxAnnouncementAudioAmount; i++)
	{
		newAnnoucement.m_audio[i] = audio[i];
		newAnnoucement.m_audioFlags[i] = (audio[i] && !strstr(audio[i], ":")) ? FLAG_SOUND_VOICE : FLAG_SOUND_EVENT;
	}
	newAnnoucement.m_onScreenMessage = onScreenMessage;
	
	m_annoucementList.push_back(newAnnoucement);

	for(int i = 0; i < k_maxAnnouncementAudioAmount; i++)
	{
		if(newAnnoucement.m_audio[i])
		{
			EPrecacheResult result = gEnv->pSoundSystem->Precache(newAnnoucement.m_audio[i], newAnnoucement.m_audioFlags[i], FLAG_SOUND_PRECACHE_DIALOG_DEFAULT);

			if(result == ePrecacheResult_Error)
			{
				DesignerWarning("While reading announcement '%s' failed to load sound '%s'", pName, newAnnoucement.m_audio[i]);
			}
		}
	}
}

const SAnnouncementDef * CAnnouncer::FindAnnouncementWithConditions(const int announcementCRC, EAnnounceConditions conditions) const
{
	const int annoucementSize = m_annoucementList.size();
	std::vector<const SAnnouncementDef*> results;

	for(int i = 0; i < annoucementSize; i++)
	{
		if(m_annoucementList[i].m_announcementID == announcementCRC)
		{
			if(m_annoucementList[i].m_conditions & conditions)
			{
				results.push_back(&m_annoucementList[i]);
			}
		}
	}

	if(results.size() > 0)
	{
		return results[cry_rand32() % results.size()];
	}

	return NULL;
}

void CAnnouncer::Announce(const char* announcement) const
{
	AnnounceInternal(0, announcement, kAnnouncerCondition_any);
}

void CAnnouncer::Announce(const EntityId entityID, EAnnouncementID announcementID) const
{
	EAnnounceConditions conditions = GetConditionsFromEntityId(entityID);
	AnnounceInternal(entityID, announcementID, conditions);
}

void CAnnouncer::Announce(const EntityId entityID, const char* announcement) const
{
	EAnnounceConditions conditions = GetConditionsFromEntityId(entityID);
	AnnounceInternal(entityID, announcement, conditions);
}

EAnnounceConditions CAnnouncer::GetConditionsFromEntityId(const EntityId entityId) const
{
	EAnnounceConditions conditions = kAnnouncerCondition_any;
	CGameRules *pGameRules=g_pGame->GetGameRules();
	if (pGameRules)
	{
		EntityId localActorEntityID = gEnv->pGame->GetIGameFramework()->GetClientActorId();

		if (localActorEntityID == entityId)
		{
			conditions |= kAnnouncerCondition_self;
		}
		else
		{
			int localTeam = pGameRules->GetTeam(localActorEntityID);
			int entityTeam = pGameRules->GetTeam(entityId);

			if (localTeam == 0 || localTeam != entityTeam)
			{
				conditions |= kAnnouncerCondition_enemy;
			}
			else
			{
				conditions |= kAnnouncerCondition_teammate;
			}
		}
	}
	return conditions;
}

bool CAnnouncer::AnnounceInternal(EntityId entityID, const char* announcement, const EAnnounceConditions conditions) const
{
	Crc32Gen* pKeyGen = gEnv->pSystem->GetCrc32Gen();
	EAnnouncementID announcementID = pKeyGen->GetCRC32(announcement);
	return AnnounceInternal(entityID, announcementID, conditions);
}

bool CAnnouncer::AnnounceInternal(EntityId entityID, const EAnnouncementID announcementID, const EAnnounceConditions conditions) const
{
	bool doAudio = true;

	if(m_canPlayFromTime > gEnv->pTimer->GetCurrTime())
	{
		CryLog("[ANNOUNCER] CanPlay %.2f - currentTime %.2f", m_canPlayFromTime, gEnv->pTimer->GetCurrTime());
		doAudio = false;
	}

	const SAnnouncementDef * doAnnouncement = FindAnnouncementWithConditions(announcementID, conditions);

	if (doAnnouncement)
	{
		IEntity *pEntity = gEnv->pEntitySystem->GetEntity(entityID);

		for(int i = 0; i < k_maxAnnouncementAudioAmount; i++)
		{
			if (doAudio && doAnnouncement->m_audio[i])
			{
				ISound* pSound = gEnv->pSoundSystem->CreateSound(doAnnouncement->m_audio[i], doAnnouncement->m_audioFlags[i]);

				if (pSound)
				{
					pSound->SetSemantic(doAnnouncement->m_audioFlags[i] & FLAG_SOUND_VOICE ? eSoundSemantic_OnlyVoice : eSoundSemantic_SoundSpot);
					pSound->Play();
					float announcementTime = (float) pSound->GetLengthMs()/1000.0f;
					m_canPlayFromTime = gEnv->pTimer->GetCurrTime() + announcementTime;
				}
				else
				{
					CryLog("[ANNOUNCER] Unable to create sound '%s' - probably invalid sound in XML file or s_soundEnable is set to 0", doAnnouncement->m_audio[i]);
				}
			}
		}

		if (!doAnnouncement->m_onScreenMessage.empty() && g_pGame->GetIGameFramework()->GetClientActor())
		{
			CUIButtonPromptRegion::SetOnScreenMessageText("announcements", doAnnouncement->m_onScreenMessage, pEntity ? pEntity->GetName() : "??", NULL);
		}
	}

	return doAnnouncement != NULL;
}

EAnnouncementID CAnnouncer::NameToID(const char* announcement) const
{
	Crc32Gen* pKeyGen = gEnv->pSystem->GetCrc32Gen();
	return pKeyGen->GetCRC32(announcement);
}


//------------------------------------------------------------------------
void CAnnouncer::CmdAnnounce(IConsoleCmdArgs* pCmdArgs)
{
	// First argument is command itself
	if (pCmdArgs->GetArgCount() > 1)
	{
		const char * arg = pCmdArgs->GetArg(1);

		EAnnounceConditions conditions = (pCmdArgs->GetArgCount() > 2) ? atoi(pCmdArgs->GetArg(2)) : kAnnouncerCondition_self|kAnnouncerCondition_teammate|kAnnouncerCondition_enemy|kAnnouncerCondition_any;
		int annoucements = (pCmdArgs->GetArgCount() > 3) ? atoi(pCmdArgs->GetArg(3)) : 1;
		for(int i = 0; i < annoucements; i++)
		{
			bool done = GetInstance()->AnnounceInternal(0, arg, conditions);
			GetInstance()->m_canPlayFromTime = 0.0f;
			if (! done)
			{
				CryLog ("[ANNOUNCER] Failed to find an appropriate announcement (%s %u)", arg, conditions);
			}
		}
	}
	else
	{
		CryLog ("[ANNOUNCER] Wrong number of arguments to console command");
	}
}
