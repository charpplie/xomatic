/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios
-------------------------------------------------------------------------
History:
- 21:07:2009		Created by Tim Furnish
*************************************************************************/

#ifndef __ANNOUNCER_H__
#define __ANNOUNCER_H__

#include "HUD/HUDOnScreenMessageDef.h"
#include "Utility/SingleAllocTextBlock.h"

typedef int EAnnouncementID;
typedef uint16 EAnnounceConditions;

const static int k_maxAnnouncementAudioAmount = 2;

struct SAnnouncementDef
{
	EAnnouncementID					m_announcementID;
	EAnnounceConditions			m_conditions;
	const char *						m_pName;
	const char *						m_audio[k_maxAnnouncementAudioAmount];
	uint32									m_audioFlags[k_maxAnnouncementAudioAmount];
	SOnScreenMessageDef			m_onScreenMessage;
};

class CAnnouncer
{
	public:
		CAnnouncer();
		~CAnnouncer();
		void LoadAnnouncements(const char *filename);

		EAnnouncementID NameToID(const char* announcement) const;

		void Announce(const char* announcement) const;
		void Announce(const EntityId entityID, const char* announcement) const;

		void Announce(const EntityId entityID, EAnnouncementID announcementID) const;

		static const CAnnouncer* GetInstance();

		static void CmdAnnounce(IConsoleCmdArgs* pArgs);
		int GetCount() const;
		const char* GetName(int i) const;

	private:
		bool AnnounceInternal(EntityId entityID, const char* announcement, const EAnnounceConditions conditions) const;
		bool AnnounceInternal(EntityId entityID, const EAnnouncementID announcementID, const EAnnounceConditions conditions) const;

		void AddAnnoucement(const char * pName, const EAnnounceConditions condition, const char * audio[k_maxAnnouncementAudioAmount], const SOnScreenMessageDef & onScreenMessage);

		const SAnnouncementDef * FindAnnouncementWithConditions(int announcementCRC, EAnnounceConditions conditions) const;

		EAnnounceConditions GetConditionsFromEntityId(const EntityId entityId) const;

		static CAnnouncer* s_announcer_instance;
		std::vector<SAnnouncementDef> m_annoucementList;

		mutable float m_canPlayFromTime;

		CSingleAllocTextBlock m_singleAllocTextBlock;
};

#endif // __ANNOUNCER_H__