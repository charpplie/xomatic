#ifndef __HUD_RADARICON_H__
#define __HUD_RADARICON_H__


//////////////////////////////////////////////////////////////////////////


struct IFlashVariableObject;
struct IHUDAsset;
class CHUD_Radar;


//////////////////////////////////////////////////////////////////////////


class CHUD_RadarIcon
{

public:

	enum DirtyFlags
	{
		eDF_None						= 0,
		eDF_PosX						= BIT(0),
		eDF_PosY						= BIT(1),
		eDF_Rot							= BIT(2),
		eDF_Icon						= BIT(3),
		eDF_Height					= BIT(4),
		eDF_Alertness				= BIT(5),
		eDF_Firing					= BIT(6),
		eDF_Alpha						= BIT(7),
		eDF_All							= 0xFFFF ^ eDF_Firing,		// Don't update firing, it would trigger the firing animation
	};

	enum RepresentationFlags
	{
		eRF_None						= 0,
		eRF_NoRot						= BIT(0),
		eRF_Edge						= BIT(1),
		eRF_HasEdge					= BIT(2),
		eRF_NoUpdatingPos		= BIT(3),
		eRF_HasAlpha				= BIT(4),
		eRF_HasAlertness		= BIT(5),
	};

	enum EHeightDisplay
	{
		eHD_Even		= 1,
		eHD_Above		= 2,
		eHD_Below		= 3
	};

	enum EIcon
	{
		eIcon_None = 0,						//Frame:
		eIcon_EnemyArrow,					//1
		eIcon_EnemyArrowEdge,			//2
		eIcon_EnemyNoArrow,				//3
		eIcon_EnemyNoArrowEdge,		//4
		eIcon_FriendlyArrow,			//5
		eIcon_FriendlyArrowEdge,	//6
		eIcon_Flag,								//7
		eIcon_FlagEdge,						//8
		eIcon_Objective,					//9
		eIcon_ObjectiveEdge,			//10
		eIcon_Player,							//11
		eIcon_PlayerEdge,					//12
		eIcon_Big,								//13
		eIcon_BigEdge,						//14
		eIcon_End
	};

	enum EAlertness
	{
		eAlertness_MP = 0,
		eAlertness_Idle,
		eAlertness_Alerted,
		eAlertness_Combat,
		eAlertness_End
	};


	struct SFlashData
	{

		struct SFlashIcon
		{

			struct SFlashAlertness
			{
				SFlashAlertness();

				void Init(IHUDAsset* pAsset, const int index);
				void PreDelete();

				IFlashVariableObject* m_up;
				IFlashVariableObject* m_down;			
				IFlashVariableObject* m_even;
			};

			SFlashIcon();

			void Init(IHUDAsset* pAsset, const int index);
			void PreDelete();

			SFlashAlertness					m_alertnessStates[eAlertness_End];
			IFlashVariableObject*		m_alertnessObject;
			IFlashVariableObject*		m_firing;			
		};

		SFlashData();

		void Init(IHUDAsset* pAsset, const int index);
		void PreDelete();

		SFlashIcon							m_iconStates[CHUD_RadarIcon::eIcon_End];
		IFlashVariableObject*		m_object;
		IFlashVariableObject*		m_icon;
	};

	Vec3									m_noUpdatingPos;
	Vec2									m_offset;

	EntityId							m_entityId;

	SFlashData*						m_pFlashData;
	CHUD_Radar*						m_pRadar;

	float									m_posX;
	float									m_posY;
	float									m_timeVisible;
	float									m_alpha;
	float									m_rotation;
	int										m_alertnessValue;

	uint16								m_dirty;
	uint16								m_flags;

	EIcon									m_icon;
	EIcon									m_tempIcon;

	EHeightDisplay				m_heightDisplay;

	CHUD_RadarIcon();
	virtual ~CHUD_RadarIcon();

	void PreDelete();
	void CopyFrom(const CHUD_RadarIcon& source);
	void Init(CHUD_Radar* pRadar, IHUDAsset* pAsset, const int index, const Vec2& innerOffset);
	void Update(float frameTime);
	void Clear();
};


//////////////////////////////////////////////////////////////////////////


#endif

