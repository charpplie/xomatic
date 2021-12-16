//==================================================================================================
// Name: CGameEffect
// Desc: Game effect - Ideal for handling a specific visual game feature
// Author: James Chilvers
//==================================================================================================

// Includes
#include "StdAfx.h"
#include "GameEffect.h"
#include "Effects/GameEffectsSystem.h"

//--------------------------------------------------------------------------------------------------
// Name: CGameEffect
// Desc: Constructor
//--------------------------------------------------------------------------------------------------
CGameEffect::CGameEffect()
{
	m_prev	= NULL;
	m_next	= NULL;
	m_flags = 0;
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: ~CGameEffect
// Desc: Destructor
//--------------------------------------------------------------------------------------------------
CGameEffect::~CGameEffect()
{
	CRY_ASSERT_MESSAGE(	(((m_flags & GAME_EFFECT_RELEASED) || !(m_flags & GAME_EFFECT_INITIALISED)) ? true : false ),
											"Effect being destroyed without being released first");

	GAME_FX_SYSTEM.UnRegisterEffect(this); // -> Effect should have been released and been unregistered, but to avoid
																				 //    crashes call unregister here too
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Initialise
// Desc: Initializes game effect
//--------------------------------------------------------------------------------------------------
void CGameEffect::Initialise(const SGameEffectParams* gameEffectParams)
{
	if(!IsFlagSet(GAME_EFFECT_INITIALISED))
	{
		SGameEffectParams params;
		if(gameEffectParams)
		{
			params = *gameEffectParams;
		}

		SetFlag(GAME_EFFECT_AUTO_UPDATES_WHEN_ACTIVE,params.autoUpdatesWhenActive);
		SetFlag(GAME_EFFECT_AUTO_UPDATES_WHEN_NOT_ACTIVE,params.autoUpdatesWhenNotActive);

		GAME_FX_SYSTEM.RegisterEffect(this);

		SetFlag(GAME_EFFECT_INITIALISED,true);
		SetFlag(GAME_EFFECT_RELEASED,false);
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Release
// Desc: Releases game effect
//--------------------------------------------------------------------------------------------------
void CGameEffect::Release()
{
	if(IsFlagSet(GAME_EFFECT_ACTIVE))
	{
		SetActive(false);
	}
	GAME_FX_SYSTEM.UnRegisterEffect(this);
	SetFlag(GAME_EFFECT_INITIALISED,false);
	SetFlag(GAME_EFFECT_RELEASED,true);
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: SetActive
// Desc: Sets active status
//--------------------------------------------------------------------------------------------------
void CGameEffect::SetActive(bool isActive)
{
	CRY_ASSERT_MESSAGE(IsFlagSet(GAME_EFFECT_INITIALISED),"Effect changing active status without being initialised first");
	CRY_ASSERT_MESSAGE((IsFlagSet(GAME_EFFECT_RELEASED)==false),"Effect changing active status after being released");

	SetFlag(GAME_EFFECT_ACTIVE,isActive);
	GAME_FX_SYSTEM.RegisterEffect(this); // Re-register effect with game effects system
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Update
// Desc: Updates game effect
//--------------------------------------------------------------------------------------------------
void CGameEffect::Update(float frameTime)
{
	CRY_ASSERT_MESSAGE(IsFlagSet(GAME_EFFECT_INITIALISED),"Effect being updated without being initialised first");
	CRY_ASSERT_MESSAGE((IsFlagSet(GAME_EFFECT_RELEASED)==false),"Effect being updated after being released");
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Next
// Desc: Gets next effect
//--------------------------------------------------------------------------------------------------
IGameEffect* CGameEffect::Next() const
{
	return m_next;
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: Prev
// Desc: Gets previous effect
//--------------------------------------------------------------------------------------------------
IGameEffect* CGameEffect::Prev() const
{
	return m_prev;
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: SetNext
// Desc: Sets the next effect
//--------------------------------------------------------------------------------------------------
void CGameEffect::SetNext(IGameEffect* newNext)
{
	m_next = newNext;
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: SetPrev
// Desc: Sets the previous effect
//--------------------------------------------------------------------------------------------------
void CGameEffect::SetPrev(IGameEffect* newPrev)
{
	m_prev = newPrev;
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: SetFlag
// Desc: Sets a game effect flag
//--------------------------------------------------------------------------------------------------
void CGameEffect::SetFlag(uint32 flag,bool state)
{
	if(state)
	{
		m_flags |= flag;
	}
	else
	{
		m_flags &= ~flag;
	}
}//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
// Name: IsFlagSet
// Desc: Checks a game effect flag
//--------------------------------------------------------------------------------------------------
bool CGameEffect::IsFlagSet(uint32 flag) const
{
	return (m_flags & flag)?true:false;
}//-------------------------------------------------------------------------------------------------
