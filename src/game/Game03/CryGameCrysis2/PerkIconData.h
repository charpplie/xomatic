/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009.
-------------------------------------------------------------------------
Description: Perk icon data
-------------------------------------------------------------------------
History:
- 2:10:2009: Created by Tim Furnish (split out of Perk.cpp)

*************************************************************************/

#ifndef __PERK_ICON_DATA_H__
#define __PERK_ICON_DATA_H__

#include "Perk.h"
#include "HUD/HUDControllerInputIcons.h"

#define k_numberOfPerkIcons							15		// Should bee able to reduce again later... single player has more defaults but doesn't need to support additional perks appearing with load-outs

class CPerkIconData
{
	// TODO: Move ticking to be a function in this class rather than in CUIPerkIconSet and add required
	// accessors member functions, then get rid of this friendship [TF]
	friend class CUIPerkIconSet;

public:
	CPerkIconData();
	~CPerkIconData();

	struct SSinglePerkIconInfo
	{
		enum infoFlags{
			eSSPIIF_Ticked = BIT(1),
			eSSPIIF_Vanishing = BIT(2),
			eSSPIIF_Usable = BIT(3),
		};

		CControllerInputRenderInfo  m_promptInfo;
		EPerks                      m_perkID;
		float                       m_drainAmount;
		float                       m_pulse;
		float                       m_alpha;
		int8                        m_flags;

		void Clear();
	};

	void Reset();
	void AddIcon(EPerks perk);
	void RemoveIcon(EPerks perk);
	void Tick(float dt);
	void SetIconDrainAmount(EPerks perk, float drainAmount, bool usable = true);
	SSinglePerkIconInfo * Find(EPerks perk);
	CControllerInputRenderInfo * GetIconInputPrompt(EPerks perk);

	static void SetLocalPlayerEntityId(EntityId who);
	static CPerkIconData * GetForEntity(EntityId who);

	static ILINE CPerkIconData * GetLocalDataToRender()
	{
		assert (s_localPlayerIconData);
		return s_localPlayerIconData;
	}

private:
	SSinglePerkIconInfo			m_icons[k_numberOfPerkIcons];

	static CPerkIconData *  s_localPlayerIconData;
	static EntityId         s_localPlayerEntityId;
};

#endif // __PERK_ICON_DATA_H__
