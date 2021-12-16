/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios
-------------------------------------------------------------------------

	Plays Announcements based upon AreaBox triggers placed in levels

History:
- 25:02:2010		Created by Ben Parbury
*************************************************************************/

#ifndef __AREAANNOUNCER_H__
#define __AREAANNOUNCER_H__

#include <ILevelSystem.h>
#include "GameRulesModules/IGameRulesRevivedListener.h"
#include "Utility/CryFixedArray.h"
#include "Audio/AudioSignalPlayer.h"

class CAreaAnnouncer : public ILevelSystemListener, IGameRulesRevivedListener
{
public:
	CAreaAnnouncer();
	~CAreaAnnouncer();

	void Init();
	void Reset();

	void Update(const float dt);

	virtual void OnLevelNotFound(const char *levelName){};
	virtual void OnLoadingStart(ILevelInfo *pLevel){};
	virtual void OnLoadingComplete(ILevel *pLevel);
	virtual void OnLoadingError(ILevelInfo *pLevel, const char *error){};
	virtual void OnLoadingProgress(ILevelInfo *pLevel, int progressAmount){};

	virtual void EntityRevived(EntityId entityId);

#if !defined(_RELEASE)
	static void CmdPlay(IConsoleCmdArgs* pCmdArgs);
	static void CmdReload(IConsoleCmdArgs* pCmdArgs);
#endif

protected:

	struct SAnnouncement
	{
		SAnnouncement();
		void AddParts(TAudioSignalID prefix, TAudioSignalID area);
		void AddPart(TAudioSignalID signal);
		void Update(const float dt);
		bool IsPlaying();
		void Play();
		void Clear();

		const static int k_maxParts = 4;
		int m_parts;
		TAudioSignalID m_part[k_maxParts];
		float m_partTimes[k_maxParts];
		float m_playedTime;
	};

	struct SAnnouncementArea
	{
#if !defined(_RELEASE)
		const static int k_maxNameLength = 32;
		char m_name[k_maxNameLength];
#endif
		EntityId m_areaProxyId;
		TAudioSignalID m_signal;
	};

	bool BuildAnnouncement(const EntityId clientId, SAnnouncement* announcement);
	void GenerateAnnouncement(const int* actorCount, const int k_areaCount, SAnnouncement* announcement);
	void PlayAnnouncement(SAnnouncement* announcement);

	const static int k_maxAnnouncementAreas = 16;
	CryFixedArray<SAnnouncementArea, k_maxAnnouncementAreas> m_areaList;

	TAudioSignalID m_multipleContactSignal;
	TAudioSignalID m_andSignal;

	SAnnouncement m_currentAnnouncement;
};

#endif // __AREAANNOUNCER_H__