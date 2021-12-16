/************************************************************************/
/* UI Tag Names
/************************************************************************/

#ifndef UI_TAG_NAMES_H
#define UI_TAG_NAMES_H

#include "HUD/UI/UIElement.h"
#include "HUD/HUDEventDispatcher.h"

//////////////////////////////////////////////////////////////////////////

#define HUD_MAX_TAGNAMES 16

//////////////////////////////////////////////////////////////////////////

class CActor;

typedef CUIElement TTagNamesParent;

//////////////////////////////////////////////////////////////////////////


class CTagnameCommon{

protected :

	CTagnameCommon();

	static bool                  CheckActorIsInValidStateToShowTagName(IActor * pActor);
	virtual void                 ZSort( void ) = 0;
	virtual void                 IgnoreEntity( EntityId entity_id, bool ignore, float time ) = 0;

private :

protected :

	int                 m_zsortedTagInfos[HUD_MAX_TAGNAMES];
};

//////////////////////////////////////////////////////////////////////////

class CUITagNames : public TTagNamesParent
	, public IHUDEventListener
	, public CTagnameCommon
{

protected :

	struct SIgnoreEnts
	{
		enum eIE_IgnoreState
		{
			eIEIS_ignore,
			eIEIS_dontIgnore,
			eIEIS_forceShow,
		};

		SIgnoreEnts()
			:	m_entityId(0)
			, m_time(-1.0f)
			, m_time_dontIgnore(-1.0f)
			, m_ignoreForever(false)
		{
			//...
		}

		EntityId m_entityId;
		float m_time;
		float m_time_dontIgnore;
		bool m_ignoreForever;
	};

	const static float k_ignoreEntityForever;

public :

	struct STagInfo
	{
		STagInfo() {}
		explicit STagInfo(EntityId entityId, bool friendly, const Vec3& pos, float size, float distance)
		{
			m_entityId = entityId;
			m_friendly = friendly;
			m_pos = pos;
			m_size = size;
			m_distance = distance;
		}

		const STagInfo& operator=( const STagInfo& _info )
		{
			m_entityId = _info.m_entityId;
			m_friendly = _info.m_friendly;
			m_pos      = _info.m_pos;
			m_size     = _info.m_size;
			m_distance = _info.m_distance;
			return *this;
		}

		void set(EntityId entityId, bool friendly, const Vec3& pos, float size, float distance)
		{
			m_entityId = entityId;
			m_friendly = friendly;
			m_pos = pos;
			m_size = size;
			m_distance = distance;
		}

		EntityId m_entityId;
		bool m_friendly;
		Vec3 m_pos;
		float m_size;
		float m_distance;
	};

	/*
	struct STagInfoDLL //double linked list for info z sorting
	{
	explicit STagInfoDLL( STagInfo* info, STagInfoLink* prev, STagInfoLink* next )
	: m_info(info)
	, m_prev(prev)
	,	m_next(next)
	{
	//...
	}

	STagInfoDLL operator=( const &STagInfoDLL _info )
	{
	m_info = _info.m_info;
	m_prev = _info.m_prev;
	m_next = _info.m_next;
	}

	void insert_before( STagInfo* new_prev )
	{
	if( m_prev != NULL )
	{
	m_prev->m_next = this;
	}
	m_prev = new_prev;
	}

	private:

	STagInfo*     m_info;
	STagInfoDLL* m_prev;
	STagInfoDLL* m_next;
	};
	*/

public:
	CUITagNames();
	virtual ~CUITagNames();

	void Initialize(const IItemParamsNode* xmlElement, IUIElement* parent);

	void Update(float frameTime);
	void Draw(void) const;

	//IHUDEventListener
	virtual void OnHUDEvent(const SHUDEvent& event);
	//~IHUDEventListener

	//CTagnameCommon
	virtual void ZSort( void );
	virtual void IgnoreEntity( EntityId entity_id, bool ignore, float time );
	//~CTagnameCommon

protected:

	SIgnoreEnts::eIE_IgnoreState ShouldIgnoreEntity( EntityId entity_id ) const;
	void                         UpdateIgnoreEntities(float frameTime);

protected:

	STagInfo            m_tagInfo[HUD_MAX_TAGNAMES];
	SIgnoreEnts         m_ignoreEnts[HUD_MAX_TAGNAMES];
	int                 m_tagsCount;

	IFFont*             m_font;

	EntityId            m_lookAt;
	EntityId            m_enemyToLockOnTo;
	float               m_timeLockedOnEnemy;
	float               m_alpha;

	bool                m_enabled;
};

//////////////////////////////////////////////////////////////////////////

bool GetTagNamePositionScreen(IEntity* pEntity, Vec3* pOut_ScreenSpace, float* pOut_Distance );
bool GetTagNamePositionWorld(IEntity* pEntity, Vec3* pOut_WorldSpace, float* pOut_Distance );
bool GetTagNamePosition(IEntity* pEntity, Vec3* pPos, float* pSize, float* pDistance);


#endif