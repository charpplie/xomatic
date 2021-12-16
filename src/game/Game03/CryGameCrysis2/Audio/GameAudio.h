/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2009.
*************************************************************************/

#pragma once

#ifndef GAME_AUDIO_H
#define GAME_AUDIO_H

#include "ISound.h"

typedef uint32 TAudioSignalID;
const TAudioSignalID INVALID_AUDIOSIGNAL_ID = -1;
class CScriptbind_GameAudio;

class CGameAudio
{
	friend class CAudioSignalPlayer;
	
public:
	CGameAudio();
	~CGameAudio();
	
	TAudioSignalID GetSignalID (const char* pSignalName, bool outputWarning = true);
	void Reset();

	void GetMemoryUsage( ICrySizer *pSizer ) const;

private:

	//.............................
	class CMood
	{
	public:
		CMood()
			: m_fadeInTimeMSecs(0)
			, m_fadeOutTimeMSecs(0)
		{}

		void GetMemoryUsage( ICrySizer *pSizer ) const
		{
			pSizer->AddObject(m_name);
		}
		string m_name;
		uint32 m_fadeInTimeMSecs;
		uint32 m_fadeOutTimeMSecs;
	};

	//.................................
	class CMoodCommand
	{
	public:
		enum ECommand
		{
			eCM_Add,
			eCM_Remove,
			eCM_Set,
			eCM_NoCommand = -1
		};

		CMoodCommand() 
			: m_pMood( 0 )
			, m_command( eCM_NoCommand )
		{}
		void Init( const CMood* pMood, ECommand command );
		void Execute() const;
		bool IsACommand() const { return m_command!=eCM_NoCommand; }

	private:	
		void EngineAddSoundMood( const CMood* pMood ) const;
		void EngineRemoveSoundMood( const CMood* pMood ) const;

	public:	
		const CMood* m_pMood;
		ECommand m_command;
	};		

	//..................................
	class CSound
	{
	public:
		CSound()
			: m_semantic( eSoundSemantic_None ),
			m_flags(FLAG_SOUND_EVENT)
		{}

		void GetMemoryUsage( ICrySizer *pSizer ) const
		{
			pSizer->AddObject(m_name);
		}

		string m_name;
		ESoundSemantic m_semantic;
		uint32 m_flags;
	};

	//..................................
	class CAudioSignal
	{
	public:
		void GetMemoryUsage( ICrySizer *pSizer ) const
		{
			pSizer->AddObject(m_signalName);
			pSizer->AddObject(m_sounds);			
		}
		string m_signalName;
		std::vector<CSound> m_sounds;
		CMoodCommand m_moodCommand;
	};
		
private:

	void LoadSignalsFromXML( const char* xmlFilename );
	void LoadMoodsFromXML( const char* xmlFilename );
	const CMood* FindMoodByName(const string& name) const;
	void TryToAddMoodCommandForSignal(const string& moodName, CAudioSignal& audioSignal, CMoodCommand::ECommand command );
	ESoundSemantic TranslateNameToSemantic( const string& name );
	uint32 TranslateXMLToFlags(const XmlNodeRef currentCommandNode);
	const CAudioSignal* GetAudioSignal( TAudioSignalID signalID );
	

private:

	std::vector<CAudioSignal> m_audioSignals;
	std::vector<CMood> m_moodsLibrary;
	std::vector<const CMood*> m_activeMoods;
	typedef std::map<string, uint32> TNameToSignalIndMap;
	TNameToSignalIndMap m_NameToSignalIndMap;  // just to optimize searchs into m_audioSignals.

	struct SSoundSemanticTranslationTableEntry
	{
		const char* name;
		const ESoundSemantic semantic;
	};
	static const SSoundSemanticTranslationTableEntry SoundSemanticTranslationTable[];
	
	struct SSoundFlagTranslationTableEntry
	{
		const char* name;
		const uint32 flag;
	};
	static const SSoundFlagTranslationTableEntry SoundFlagTranslationTable[];

	CScriptbind_GameAudio* m_pScriptbind;
};


#endif
