/************************************************************************/
/* UI BattleLog for Component HUD, Tim Furnish, 2009  									*/
/************************************************************************/

#ifndef UI_OBJECTIVES_H
#define UI_OBJECTIVES_H

#include "HUD/UI/UIElement.h"
#include "HUD/HUDEventDispatcher.h"

#include "GameRulesTypes.h"

class ScreenLayoutManager;

#if defined(_RELEASE)
#define INCLUDE_OBJECTIVE_ICON_DEBUG_CODE		0
#elif defined(PROFILE)
#define INCLUDE_OBJECTIVE_ICON_DEBUG_CODE		0
#else // debug build
#define INCLUDE_OBJECTIVE_ICON_DEBUG_CODE		1
#endif

typedef bool (* ShouldRenderIconCallbackFunc)();

#define ObjectiveIconRenderModeList(f)                                                                                      \
	f(kOIRM_singleTexture) /* Keep this one first in the list so it's the default (when the XML doesn't specify a mode) */    \
	f(kOIRM_draining)                                                                                                         \
	f(kOIRM_flashing)

AUTOENUM_BUILDENUMWITHTYPE_WITHNUM(EObjectiveIconRenderMode, ObjectiveIconRenderModeList, kOIRM_num);

struct SMissionObjectiveIconGroup
{
	int m_numUsed;
	int m_maxNum;

#if INCLUDE_OBJECTIVE_ICON_DEBUG_CODE
	bool m_assertWhenRunOutOfSlots;
#endif

	SMissionObjectiveIconGroup()
	{
		memset (this, 0, sizeof(SMissionObjectiveIconGroup));
	}
};

struct SMissionObjectiveType
{
	EObjectiveIconRenderMode m_renderMode;
	char m_textureOne[32];
	char m_textureTwo[32];
	char m_arrowsTextureOne[40];
	char m_arrowsTextureTwo[40];
	float zOffset;
	float m_onScreenSize;
	float m_atEdgeSize;
	float m_onScreenShrinkWithDistanceVal;
	float m_atEdgeShrinkWithDistanceVal;

	struct
	{
		ColorF colour;
		float x;
		float y;
		float width;
		float height;
	} m_text;

	struct
	{
		ColorF colour;
		float x;
		float y;
		float width;
		float height;
	} m_dist;

	struct
	{
		float opaqueWithinDistance;
		float vanishAfterDistance;
	} m_fadeOut;

	// TODO: Collapse into a bitfield?
	bool m_doShowEntityName;
	bool m_doFadeOut;
	bool m_doShrinkWithDistance;
	bool m_showDistanceTo;
	bool m_hasDistElement;

	SMissionObjectiveIconGroup * m_group;
	ShouldRenderIconCallbackFunc m_shouldRenderFunc;

	SMissionObjectiveType()
	{
		memset (this, 0, sizeof(SMissionObjectiveType));
	};
};

struct SMissionObjectiveIconData
{
	const SMissionObjectiveType *m_type;
	ITexture *textureOne;
	ITexture *textureTwo;
	ITexture *arrowsTextureOne;
	ITexture *arrowsTextureTwo;
	float m_progress;
};

struct SMissionObjectiveData
{
	static const int k_numPriorities = 2;

	SMissionObjectiveIconData m_details[k_numPriorities];

	EntityId targetId;

	SMissionObjectiveData()
	{
		memset (this, 0, sizeof(SMissionObjectiveData));
	};

	const SMissionObjectiveIconData *GetData() const
	{
		for (int i = k_numPriorities - 1; i >= 0; --i)
		{
			if (m_details[i].m_type)
			{
				return &m_details[i];
			}
		}
		return NULL;
	}

	SMissionObjectiveIconData *GetData()
	{
		for (int i = k_numPriorities - 1; i >= 0; --i)
		{
			if (m_details[i].m_type)
			{
				return &m_details[i];
			}
		}
		return NULL;
	}
};

struct SOnScreenIcon
{
	SOnScreenIcon(const SMissionObjectiveData * data, ITexture *pTextureOne, ITexture *pTextureTwo, float _x, float _y, float _size, float _distance, 
		float dot, int positionId, const char *_name,
		float _textX, float _textY, float _textWidth, float _textHeight,
		bool showDistElement, float distEleX, float distEleY, float distEleW, float distEleH)
	{
		m_data = data;
		x = _x;
		y = _y;
		m_renderSize = _size;
		distance = _distance;
		m_dot = dot;
		m_positionId = positionId;
		name = _name;
		textX = _textX;
		textY = _textY;
		fontWidth = _textWidth;
		fontHeight = _textHeight;
		m_textureOne = pTextureOne;
		m_textureTwo = pTextureTwo;
		m_showDistElement = showDistElement;
		m_distEleX = distEleX;
		m_distEleY = distEleY;
		m_distEleW = distEleW;
		m_distEleH = distEleH;
	}

	const SMissionObjectiveData * m_data;
	ITexture * m_textureOne;
	ITexture * m_textureTwo;
	float x;
	float y;
	float m_renderSize;
	float distance;
	float m_dot;
	int m_positionId;
	float textX;
	float textY;
	float fontWidth;
	float fontHeight;
	const char *name;
	bool m_showDistElement;
	float m_distEleX;
	float m_distEleY;
	float m_distEleW;
	float m_distEleH;

	bool operator ==( const SOnScreenIcon& compare ) const
	{
		return distance == compare.distance;
	}

	bool operator <( const SOnScreenIcon& compare ) const
	{
		return distance > compare.distance;
	}
};

class CUIMissionObjectives : public CUIElement, public IHUDEventListener
{
public :
	CUIMissionObjectives();
	~CUIMissionObjectives( void );

	virtual void Initialize( const IItemParamsNode* element, IUIElement* parent );

	virtual void Update(float frameTime);
	virtual void Draw(void) const;

	//IHUDEventListener
	virtual void OnHUDEvent(const SHUDEvent& event);
	//~IHUDEventListener

	void GetMemoryUsage(ICrySizer *pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));	
		pSizer->AddContainer(m_onScreenIcons);
		CUIElement::GetInternalMemoryUsage(pSizer); // collect memory of parent class
	}
private :
	// Not impd virtual const float	GetPosX() const;
	// Not impd virtual const float	GetPosY() const;
	void NewMissionObjective( const EntityId targetId, const EGameRulesMissionObjectives objectiveType, float progress, int priority);
	void RemoveMissionObjective( EntityId targetId, int priority );
	void ClearMissionObjectives();
	void UpdateMissionObjective(SMissionObjectiveData *objective);

	bool IsMissionObjectiveTarget(EntityId targetId) const;

private :
	ScreenLayoutManager* m_layoutManager;

	int                          m_maxIcons;
	SMissionObjectiveData*       m_missionObjectives;
	std::vector<SOnScreenIcon>   m_onScreenIcons;
	int                          m_clampToOval;
	float                        m_scaleSafeZone;
	float                        m_iconBrightnessWhenBehindCamera;

	SMissionObjectiveIconGroup * m_groups;

#if INCLUDE_OBJECTIVE_ICON_DEBUG_CODE
	int                          m_numGroups;
	int                          m_dbgNeverUseEdgeTextures;
	int                          m_dbgListUsedIconSlots;
#endif

	SMissionObjectiveType m_missionObjectiveTypes[EGRMO_Max];

	void NewMissionObjectiveInternal( const EntityId targetId, const SMissionObjectiveType * theType, float progress, int priority);
	SMissionObjectiveData *GetObjectiveSlot( const EntityId targetId );
	void ReadIconDef(const IItemParamsNode * xmlElement, int groupNum);
};

#endif // UI_OBJECTIVES_H
