/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
*************************************************************************/

#include "StdAfx.h"
#include "GameAudio.h"
#include "Game.h"
#include "Scriptbind_GameAudio.h"
#include <IEntitySystem.h>
#include <ISoundMoodManager.h>
#include <StlUtils.h>
#include "TypeInfo_impl.h"
 
const CGameAudio::SSoundSemanticTranslationTableEntry CGameAudio::SoundSemanticTranslationTable[]=
{
	{ "None",                 eSoundSemantic_None },
	{ "OnlyVoice",            eSoundSemantic_OnlyVoice },
	{ "NoVoice",              eSoundSemantic_NoVoice },
	{ "Sandbox",              eSoundSemantic_Sandbox },
	{ "Unused1",              eSoundSemantic_Unused1 },

	{ "Unused2",              eSoundSemantic_Unused2 },
	{ "Unused3",              eSoundSemantic_Unused3 },
	{ "Ambience",             eSoundSemantic_Ambience },
	{ "Ambience_OneShot",     eSoundSemantic_Ambience_OneShot },

	{ "Physics_Collision",    eSoundSemantic_Physics_Collision },
	{ "Dialog",               eSoundSemantic_Dialog },
	{ "MP_Chat",              eSoundSemantic_MP_Chat },
	{ "Physics_Footstep",     eSoundSemantic_Physics_Footstep },

	{ "Physics_General",      eSoundSemantic_Physics_General },
	{ "HUD",                  eSoundSemantic_HUD },
	{ "Unused4",              eSoundSemantic_Unused4 },
	{ "FlowGraph",            eSoundSemantic_FlowGraph },

	{ "Player_Foley_Voice",   eSoundSemantic_Player_Foley_Voice },
	{ "Living_Entity",        eSoundSemantic_Living_Entity },
	{ "Mechanic_Entity",      eSoundSemantic_Mechanic_Entity },
	{ "NanoSuit",             eSoundSemantic_NanoSuit },

	{ "SoundSpot",            eSoundSemantic_SoundSpot },
	{ "Particle",             eSoundSemantic_Particle },
	{ "AI_Pain_Death",        eSoundSemantic_AI_Pain_Death },
	{ "AI_Readability",       eSoundSemantic_AI_Readability },

	{ "AI_Readability_Response", eSoundSemantic_AI_Readability_Response },
	{ "TrackView",            eSoundSemantic_TrackView },
	{ "Projectile",           eSoundSemantic_Projectile },
	{ "Vehicle",              eSoundSemantic_Vehicle },

	{ "Weapon",               eSoundSemantic_Weapon },
	{ "Explosion",            eSoundSemantic_Explosion },
	{ "Player_Foley",         eSoundSemantic_Player_Foley },
	{ "Animation",            eSoundSemantic_Animation },

	{ "",                     eSoundSemantic_None },
};

const CGameAudio::SSoundFlagTranslationTableEntry CGameAudio::SoundFlagTranslationTable[]=
{
	{ "voice",                 FLAG_SOUND_VOICE },
};

#define MOODS_FILE "Libs/GameAudio/Moods.xml"
#define SIGNALS_FILE "Libs/GameAudio/AudioSignals.xml"
#define SIGNALS_FILE_MP "Libs/GameAudio/cw2_AudioSignals.xml"
#define SIGNALS_FILE_BATTLECHATTER "Libs/GameAudio/BattlechatterSignals.xml"
#define SIGNALS_FILE_AREA_ANNOUNCEMENTS "Libs/GameAudio/AreaAnnouncements.xml"

#define XMLSIGNALS_ERROR(XmlNode, error, ...) CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Reading File: %s Line: %d. " error, SIGNALS_FILE, XmlNode->getLine(), __VA_ARGS__ );

//////////////////////////////////////////////////////////////////////////

CGameAudio::CGameAudio()
: m_pScriptbind( NULL )
{
	CRY_ASSERT(gEnv->pSoundSystem != 0);

	LoadMoodsFromXML( MOODS_FILE );
	LoadSignalsFromXML( SIGNALS_FILE );
	LoadSignalsFromXML( SIGNALS_FILE_MP );
	LoadSignalsFromXML( SIGNALS_FILE_BATTLECHATTER );
	LoadSignalsFromXML( SIGNALS_FILE_AREA_ANNOUNCEMENTS );
	
	m_pScriptbind = new CScriptbind_GameAudio;
}


//////////////////////////////////////////////////////////////////////////

CGameAudio::~CGameAudio()
{
	SAFE_DELETE( m_pScriptbind );
}


//////////////////////////////////////////////////////////////////////////

TAudioSignalID CGameAudio::GetSignalID( const char* pSignalName, bool outputWarning)
{
	TAudioSignalID ID = INVALID_AUDIOSIGNAL_ID;

	TNameToSignalIndMap::const_iterator iter = m_NameToSignalIndMap.find( CONST_TEMP_STRING(pSignalName) );
	if (iter!=m_NameToSignalIndMap.end())
	{
		ID = iter->second;
	}
#if !defined(_RELEASE)
	else if(outputWarning)
	{
		CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "AudioSignal: '%s' not found", pSignalName);
	}	
#endif
	return ID;
}

//////////////////////////////////////////////////////////////////////////

void CGameAudio::Reset()
{
	m_pScriptbind->Reset();
}

//////////////////////////////////////////////////////////////////////////

void CGameAudio::LoadMoodsFromXML( const char* xmlFilename )
{
	XmlNodeRef xmlNode = GetISystem()->LoadXmlFile(xmlFilename);

	if (xmlNode==NULL)
	{
		CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "CGameAudio::LoadMoodsFromXML: Failed to load'%s'", xmlFilename);
		return;
	}

	const uint32 moodCount = xmlNode->getChildCount();

	for (uint32 moodIndex = 0; moodIndex < moodCount; ++moodIndex)
	{
		const XmlNodeRef currentMoodNode = xmlNode->getChild(moodIndex); 

		CMood newMood;
		newMood.m_name = currentMoodNode->getAttr("name");
		currentMoodNode->getAttr("fadeInTimeMSecs", newMood.m_fadeInTimeMSecs);
		currentMoodNode->getAttr("fadeOutTimeMSecs", newMood.m_fadeOutTimeMSecs);

		m_moodsLibrary.push_back( newMood );
	}
}


//////////////////////////////////////////////////////////////////////////

void CGameAudio::LoadSignalsFromXML( const char* xmlFilename )
{
	MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Other, 0, "Audio Signals XML (%s)", xmlFilename);

	XmlNodeRef xmlNode = GetISystem()->LoadXmlFile(xmlFilename);

	if (xmlNode==NULL)
	{
		CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "CGameAudio::LoadSignalsFromXML: Failed to load'%s'", xmlFilename);
		return;
	}

	const uint32 signalCount = xmlNode->getChildCount();

	for (uint32 signalIndex = 0; signalIndex < signalCount; ++signalIndex)
	{
		const XmlNodeRef currentSignalNode = xmlNode->getChild(signalIndex); 
		
		CAudioSignal audioSignal;
		audioSignal.m_signalName = currentSignalNode->getAttr("signal");
		
		const uint32 signalCommandCount = currentSignalNode->getChildCount();
		for (uint32 commandIndex = 0; commandIndex < signalCommandCount; ++commandIndex)
		{
			const XmlNodeRef currentCommandNode = currentSignalNode->getChild(commandIndex);

			string lineTag = currentCommandNode->getTag();

			if (lineTag == "Sound")
			{
				CSound newSound;
				newSound.m_name = currentCommandNode->getAttr("name");
				newSound.m_semantic = TranslateNameToSemantic( currentCommandNode->getAttr("semantic") );
				newSound.m_flags = TranslateXMLToFlags(currentCommandNode);
				audioSignal.m_sounds.push_back(newSound);
				EPrecacheResult result = gEnv->pSoundSystem->Precache(newSound.m_name, 0u, FLAG_SOUND_PRECACHE_EVENT_DEFAULT);
				if(result == ePrecacheResult_Error)
				{
					CryWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "AudioSignal: %s failed to Precache", newSound.m_name.c_str());
				}
			}

			if (lineTag == "SetSoundMood")
			{
				TryToAddMoodCommandForSignal( currentCommandNode->getAttr("name"), audioSignal, CMoodCommand::eCM_Set );
			}

			if (lineTag == "RemoveSoundMood")
			{
				TryToAddMoodCommandForSignal( currentCommandNode->getAttr("name"), audioSignal, CMoodCommand::eCM_Remove );
			}

			if (lineTag == "AddSoundMood")
			{
				TryToAddMoodCommandForSignal( currentCommandNode->getAttr("name"), audioSignal, CMoodCommand::eCM_Add );
			}
		}

		m_audioSignals.push_back(audioSignal);
		
		std::pair< TNameToSignalIndMap::iterator, bool > val = m_NameToSignalIndMap.insert( std::make_pair( audioSignal.m_signalName, m_audioSignals.size()-1 ) );
		if (!val.second)
		{
			XMLSIGNALS_ERROR( xmlNode, "Duplicated signal: %s", audioSignal.m_signalName.c_str());
		}
	}
}

//////////////////////////////////////////////////////////////////////////
uint32 CGameAudio::TranslateXMLToFlags(const XmlNodeRef currentCommandNode)
{
	uint32 flags = FLAG_SOUND_EVENT;

	const uint32 flagTranslationCount = ARRAY_COUNT(SoundFlagTranslationTable);
	for (uint32 index = 0; index < flagTranslationCount; index++)
	{
		const SSoundFlagTranslationTableEntry& translationTableEntry = SoundFlagTranslationTable[index];

		if (currentCommandNode->haveAttr(translationTableEntry.name))
		{
			int enable = 0;
			currentCommandNode->getAttr(translationTableEntry.name, enable);
			if(enable)
			{
				flags |= translationTableEntry.flag;
			}
		}
	}

	return flags;
}

//////////////////////////////////////////////////////////////////////////

void CGameAudio::TryToAddMoodCommandForSignal( const string& moodName, CAudioSignal& audioSignal, CMoodCommand::ECommand command )
{
	if (audioSignal.m_moodCommand.m_command!=CMoodCommand::eCM_NoCommand)
		return;

	const CMood* pMood = FindMoodByName(moodName);
	if (pMood)
	{
		audioSignal.m_moodCommand.Init( pMood, command );
	}
}

//////////////////////////////////////////////////////////////////////////

ESoundSemantic CGameAudio::TranslateNameToSemantic( const string& name )
{
	const uint32 semanticTranslationCount = ARRAY_COUNT(SoundSemanticTranslationTable);
	for (uint32 index = 0; index < semanticTranslationCount; index++)
	{
		const SSoundSemanticTranslationTableEntry& translationTableEntry = SoundSemanticTranslationTable[index];

		if (translationTableEntry.name == name)
		{
			return translationTableEntry.semantic;
		}
	}

	return eSoundSemantic_None;
}


//////////////////////////////////////////////////////////////////////////

const CGameAudio::CMood* CGameAudio::FindMoodByName( const string& name ) const
{
	for (uint32 moodIndex = 0; moodIndex < m_moodsLibrary.size(); ++moodIndex)
	{
		const CMood* currentMood = &m_moodsLibrary[moodIndex];
		if (currentMood->m_name == name)
		{
			return currentMood;
		}
	}

	return 0;
}


//////////////////////////////////////////////////////////////////////////

const CGameAudio::CAudioSignal* CGameAudio::GetAudioSignal( TAudioSignalID signalID )
{
	if (signalID<m_audioSignals.size())
	{
		return &m_audioSignals[signalID];
	}
	else
		return NULL;
}

void CGameAudio::GetMemoryUsage( ICrySizer *pSizer ) const
{
	pSizer->AddObject(m_audioSignals);	
	pSizer->AddObject(m_moodsLibrary);	
	pSizer->AddObject(m_activeMoods);	
	pSizer->AddObject(m_NameToSignalIndMap);	
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CGameAudio::CMoodCommand::EngineAddSoundMood( const CMood* pMood ) const
{
	ISoundMoodManager* moodManager = gEnv->pSoundSystem->GetIMoodManager();
	moodManager->RegisterSoundMood( pMood->m_name.c_str() );
	moodManager->UpdateSoundMood( pMood->m_name.c_str(), 1.0f, pMood->m_fadeInTimeMSecs );
}

void CGameAudio::CMoodCommand::EngineRemoveSoundMood( const CMood* pMood ) const
{
	ISoundMoodManager* moodManager = gEnv->pSoundSystem->GetIMoodManager();
	moodManager->UpdateSoundMood( pMood->m_name.c_str(), 0.0f, pMood->m_fadeOutTimeMSecs );
}


void CGameAudio::CMoodCommand::Execute() const
{
	CGameAudio* pGameAudio = g_pGame->GetGameAudio();

	switch( m_command )
	{
	case eCM_Add:
		EngineAddSoundMood( m_pMood );
		stl::push_back_unique( pGameAudio->m_activeMoods, m_pMood );
		break;

	case eCM_Remove:
		EngineRemoveSoundMood( m_pMood );
		stl::find_and_erase( pGameAudio->m_activeMoods, m_pMood);
		break;

	case eCM_Set:
		{
			CGameAudio* pGameAudio = g_pGame->GetGameAudio();
			for (uint32 moodIndex=0; moodIndex < pGameAudio->m_activeMoods.size(); ++moodIndex)
			{
				EngineRemoveSoundMood( pGameAudio->m_activeMoods[moodIndex] );
			}

			pGameAudio->m_activeMoods.clear();
			pGameAudio->m_activeMoods.push_back( m_pMood );

			EngineAddSoundMood( m_pMood );
			break;
		}
	}
}


void CGameAudio::CMoodCommand::Init( const CMood* pMood, ECommand command )
{
	m_pMood = pMood;
	m_command = command;
}
