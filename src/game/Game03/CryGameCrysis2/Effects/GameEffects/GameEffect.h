#ifndef _GAME_EFFECT_
#define _GAME_EFFECT_

#pragma once

// Forward declares
struct SGameEffectParams;

//==================================================================================================
// Name: EGameEffectFlags
// Desc: Game effect flags
// Author: James Chilvers
//==================================================================================================
enum EGameEffectFlags
{
	GAME_EFFECT_INITIALISED											= (1<<0),
	GAME_EFFECT_RELEASED												=	(1<<1),
	GAME_EFFECT_AUTO_UPDATES_WHEN_ACTIVE				= (1<<2),
	GAME_EFFECT_AUTO_UPDATES_WHEN_NOT_ACTIVE		= (1<<3),
	GAME_EFFECT_REGISTERED											= (1<<4),
	GAME_EFFECT_ACTIVE													= (1<<5)
};//------------------------------------------------------------------------------------------------

//==================================================================================================
// Name: IGameEffect
// Desc: Interface for all game effects
// Author: James Chilvers
//==================================================================================================
struct IGameEffect
{
	friend class CGameEffectsSystem;
public:
	virtual ~IGameEffect() {}

	virtual void	Initialise(const SGameEffectParams* gameEffectParams = NULL) = 0;
	virtual void	Release() = 0;
	virtual void	Update(float frameTime) = 0;

	virtual void	SetActive(bool isActive) = 0;

	virtual void	SetFlag(uint32 flag,bool state) = 0;
	virtual bool	IsFlagSet(uint32 flag) const = 0;

	virtual void GetMemoryUsage( ICrySizer *pSizer ) const=0;
private:
	virtual IGameEffect*	Next() const = 0;
	virtual IGameEffect*	Prev() const = 0;
	virtual void					SetNext(IGameEffect* newNext) = 0;
	virtual void					SetPrev(IGameEffect* newPrev) = 0;
};//------------------------------------------------------------------------------------------------

//==================================================================================================
// Name: CGameEffect
// Desc: Game effect - Ideal for handling a specific visual game feature
// Author: James Chilvers
//==================================================================================================
class CGameEffect : public IGameEffect
{
public:
	CGameEffect();
	virtual ~CGameEffect();

	virtual void	Initialise(const SGameEffectParams* gameEffectParams = NULL);
	virtual void	Release();
	virtual void	Update(float frameTime);

	virtual void	SetActive(bool isActive);

	void					SetFlag(uint32 flag,bool state);
	bool					IsFlagSet(uint32 flag) const;

	virtual void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));	
	}
	
private:
	IGameEffect*	Next() const;
	IGameEffect*	Prev() const;
	void					SetNext(IGameEffect* newNext);
	void					SetPrev(IGameEffect* newPrev);

	IGameEffect*	m_prev;
	IGameEffect*	m_next;
	uchar					m_flags;
};//------------------------------------------------------------------------------------------------

//==================================================================================================
// Name: SGameEffectParams
// Desc: Game effect parameters
// Author: James Chilvers
//==================================================================================================
struct SGameEffectParams
{
	friend class CGameEffect;

// Make constructor private to stop SGameEffectParams ever being created, should always inherit this
// for each effect to avoid casting problems
protected:
	SGameEffectParams()
	{
		autoUpdatesWhenActive = true;
		autoUpdatesWhenNotActive = false;
	}

public:
	bool	autoUpdatesWhenActive;
	bool	autoUpdatesWhenNotActive;
	
};//------------------------------------------------------------------------------------------------

#endif // _GAME_EFFECT_
