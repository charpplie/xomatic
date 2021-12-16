#ifndef Command_Buffer_h
#define Command_Buffer_h

class CCharacterModel;
class CCharInstance;

namespace Command {

// TODO: Needs rename
enum 
{
	TmpBuffer = 0,
	TargetBuffer = 3,
};

class CState
{
public:
	CState()
	{
		Reset();
	}

public:
	bool Initialize(CCharInstance* pInstance);

	ILINE void Reset()
	{
		m_lod = 0;

		m_pJointMask = NULL;
		m_jointMaskCount = 0;
	}

	ILINE bool IsJointActive(uint32 nameCrc32) const
	{
		if (!m_pJointMask)
			return true;

		uint32* pValue = std::lower_bound(&m_pJointMask[0], &m_pJointMask[m_jointMaskCount], nameCrc32);
		if (pValue == &m_pJointMask[m_jointMaskCount] || *pValue != nameCrc32)
			return false;

		return true;
	}

public:
	CCharacterModel* m_pModel;

	Skeleton::CPoseData* m_pPoseData;
	Skeleton::CLocator* m_pLocator;

	uint32 m_jointCount;

	uint32 m_lod;

	f32 m_timeDelta;

	uint32* m_pJointMask;
	uint32 m_jointMaskCount;

	bool m_bFootPlants : 1;

	// NOTE: Do not access this, here only for PoseModifier back-compat.
	CCharInstance* m_pInstance;
};

class CBuffer
{
public:
	ILINE uint32 GetLengthTotal() const { return sizeof(m_pBuffer); }
	ILINE uint32 GetLengthUsed() const { return m_pCommands - m_pBuffer; }
	ILINE uint32 GetLengthFree() const { return GetLengthTotal() - GetLengthUsed(); }

	bool Initialize(CCharInstance* pInstance);

	void SetPoseData(Skeleton::CPoseData& poseData, Skeleton::CLocator& locator);

	template <class Type>
	SPU_NO_INLINE Type* CreateCommand()
	{
		assert((sizeof(Type) & 3) == 0);

		uint32 lengthFree = GetLengthFree();
		if (lengthFree < sizeof(Type))
		{
			CryFatalError("CryAnimation: CommandBuffer overflow!");
			return NULL;
		}

		Type* pCommand = (Type*)m_pCommands;
		*(uint8*)pCommand = Type::ID;
		m_pCommands += sizeof(Type);
		m_commandCount++;
		return pCommand;
	}

	void Execute();

private:
	void DebugDraw();

private:
	uint8 m_pBuffer[2048];

	SPU_DOMAIN_LOCAL uint8* m_pCommands;
	uint8 m_commandCount;

	CState m_state;

	// NOTE: Do not access this, here only for PoseModifier back-compat.
	CCharInstance* m_pInstance;
};

} // namespace Command

#endif // Command_Buffer_h
