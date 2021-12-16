//--------------------------------------------
// Based on IMechanic from Actor2 (just for testing...)
//
// 16-08-2008
//--------------------------------------------------
#ifndef __PingerMechanic_h__
#define __PingerMechanic_h__



//--------------------------------------------------------------------------------
//Mechanic input parameters
enum EMechanicInputParamsID
{
	eMIP_INVALID = -1,
	eMIP_PINGER = 0,
	eMIP_LAST
};

struct mi_params 
{
	mi_params():type(eMIP_INVALID){}
	int type;
};

struct mi_state
{
	mi_state():type(eMIP_INVALID){}
	int type;
};

//--------------------------------------------------------------------------------
//Mechanic interface
class CActor;

struct IMyMechanic
{
	virtual const char* GetName() const = 0;

	virtual bool Init() = 0;
	virtual void Release() = 0;

	//Better not to use Actor, but for testing should be fine
	virtual bool Update(CActor* pActor, mi_state* pState, const mi_params* pParameters) = 0;

	virtual void OnEnter(CActor* pActor, mi_state* pState) = 0;
	virtual void OnLeave(CActor* pActor, mi_state* pState) = 0;
};

//--------------------------------------------------------
//This structure stores action info (animation to play, or sound, fx... etc, similar to item actions)
//Animation name, could be an AnimationAction (sequences, locomotion, single animations...)
//All is hardcoded, but could be readed from xml file easily
struct SMyAction
{
	//Animation stuff
	string animationName;
	float  speed;
	float  blend;
	bool	 looped;
	int8	 layer;

	SMyAction(const char* name, float _speed, float _blend, bool _looped, int _layer=0 ):
	animationName(name),
	speed(_speed),
	looped(_looped),
	blend(_blend),
	layer(_layer)
	{
	}
	
};

//--------------------------------------------------------
//Sample implementations of mechanic

class CPingerMechanic_Locomotion : public IMyMechanic
{
public:
	CPingerMechanic_Locomotion() {};
	~CPingerMechanic_Locomotion() {};

	//IMyMechanic
	virtual const char* GetName() const { return "PingerMechanic_Locomotion"; }

	virtual bool Init();
	virtual void Release() { delete this; }
	virtual bool Update(CActor* pActor, mi_state* pState, const mi_params* pParameters); 
	
	virtual void OnEnter(CActor* pActor, mi_state* pState);
	virtual void OnLeave(CActor* pActor, mi_state* pState);
	//~IMyMechanic

private:

	void ExtractDesiredMoveDirection(CActor* pActor, const mi_params* pParameters, Vec3& dir, float& speed);

	struct SPingerLocomotionParams
	{
		float maxSpeed;
		float maxturnSpeed;
		float accelerationTime;
	};

	SPingerLocomotionParams m_lmParams;
};

//--------------------------------------------------------
class CPingerMechanic_AnimAction : public IMyMechanic
{
public:
	CPingerMechanic_AnimAction() {};
	~CPingerMechanic_AnimAction() {};

	//IMyMechanic
	virtual const char* GetName() const { return "PingerMechanic_AnimAction"; }

	virtual bool Init();
	virtual void Release() { delete this; }
	virtual bool Update(CActor* pActor, mi_state* pState, const mi_params* pParameters); 

	virtual void OnEnter(CActor* pActor, mi_state* pState);
	virtual void OnLeave(CActor* pActor, mi_state* pState);
	//~IMyMechanic

};

//--------------------------------------------------------
class CPingerMechanic_Combat : public IMyMechanic
{
public:
	CPingerMechanic_Combat() {};
	~CPingerMechanic_Combat() {};

	//IMyMechanic
	virtual const char* GetName() const { return "PingerMechanic_Combat"; }

	virtual bool Init();
	virtual void Release() { delete this; }
	virtual bool Update(CActor* pActor, mi_state* pState, const mi_params* pParameters); 

	virtual void OnEnter(CActor* pActor, mi_state* pState);
	virtual void OnLeave(CActor* pActor, mi_state* pState);
	//~IMyMechanic

private:

	void ExtractDesiredLookDirection(CActor* pActor, const mi_params* pParameters, Vec3& dir);

	int ShouldStartTurning(const Vec2 currentDir, const Vec2 targetDir);
	int ShouldContinueTurning(const Vec2 currentDir, const Vec2 targetDir);
	int ShouldStrafe(const Vec2 moveDir, const Vec2 lookDir);

	struct SCombatParams
	{
		float turnIdleThreshold;
		float turnLimitThreshold;
		float strafeLimitThreshold;
	};

	enum EPingerLocomotionDirection
	{
		ePLD_None = 0,
		ePLD_Left,
		ePLD_Right, 
	};

	SCombatParams m_cbParams;

};

//-------------------------------------------------------
//Pretty simple registry 
//Stores available mechanics and actions (should be separated probably, but this test is ok)
class MyMechanicRegistry
{
public:
	static MyMechanicRegistry& ref();
	~MyMechanicRegistry() { Destroy(); };

	IMyMechanic* GetMechanicByName(const char* mechanicName) const;
	const SMyAction& GetActionByName(const char* actionName) const;

	void AddRef()
	{
		m_refs++;
	}
	void DeleteRef()
	{
		m_refs--;
		assert(m_refs>=0);
		if(m_refs==0)
		{
			delete m_sThis;
			m_sThis = NULL;
		}
	}

private:

	MyMechanicRegistry():m_refs(0) { Init(); };

	void Init();
	void Destroy();

	static MyMechanicRegistry* m_sThis;
	uint32		m_refs;

	typedef std::map<string, IMyMechanic*> TMechanicsMap;
	TMechanicsMap m_allMechanics;

	typedef std::map<string, SMyAction> TActionMap;
	TActionMap m_allActions;

};

#endif // __PingerMechanic_h__