/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2009-2010.
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:
	Part of Player that controls stealth killing.

-------------------------------------------------------------------------
History:
- 15:10:2009: Created by Sven Van Soom

*************************************************************************/

#pragma once

#ifndef __STEALTH_KILL_H__
#define __STEALTH_KILL_H__

class CActor;
class CPlayer;
struct ICharacterInstance;
struct AnimEventInstance;
struct IItemParamsNode;

#if !defined(_RELEASE)
	#define STEALTH_KILL_DEBUG
#endif

class CStealthKill
{
private:
	struct SParams
	{
		SParams()
			: pEnemyClass(NULL)
			, impulseScale(1.0f)
			, playerAnimation("")
			, enemyAnimation("")
			, impulseBone("")
		{

		}

		IEntityClass* pEnemyClass;
		float impulseScale;

		string playerAnimation;
		string enemyAnimation;
		string impulseBone;
	};

	typedef std::vector<CStealthKill::SParams> TStealthKillParams;

public:
	CStealthKill()
		: m_pPlayer(NULL), m_isBusy(false), m_pTarget(NULL), m_isDeathBlowDone(false)
	{

	}

	void Init(CPlayer* pPlayer);
	ILINE bool IsBusy() const { return (m_isBusy); }
	void Enter(int enemyEntityId);
	void Update();
	bool AnimationEvent(ICharacterInstance *pCharacter, const AnimEventInstance &event, const uint32 eventNameCRC);
	bool CanExecuteOn(CActor* pTarget) const;

	void ReadXmlData(const IItemParamsNode* pRootNode, bool reloading = false);

private:
	void Leave();
	void DeathBlow();

	void SetStatsViewMode(bool followCameraBone);

	static const CStealthKill::SParams* GetParamsForClass(IEntityClass* pTargetClass);
	static bool CanExecuteOnEnemyClass(IEntityClass* pTargetClass);

	CPlayer* m_pPlayer;

	CActor* m_pTarget;
	bool m_isBusy;
	bool m_isDeathBlowDone;

	static TStealthKillParams s_stealthKillParams;
	static bool s_dataLoaded;
};


#endif // __STEALTH_KILL_H__