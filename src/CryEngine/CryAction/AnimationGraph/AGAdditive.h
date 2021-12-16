#ifndef __AGADDITIVE_H__
#define __AGADDITIVE_H__

#pragma once

#include "IAnimationStateNode.h"

class CAnimationRandomizer;

class CAGAdditive : public IAnimationStateNodeFactory, public IAnimationStateNode
{
public:
	CAGAdditive();
	virtual ~CAGAdditive();

	// IAnimationStateNode
	virtual void EnterState( SAnimationStateData& data, bool dueToRollback );
	virtual EHasEnteredState HasEnteredState( SAnimationStateData& data ) { return eHES_Instant; }
	virtual bool CanLeaveState( SAnimationStateData& data );
	virtual void LeaveState( SAnimationStateData& data );
	virtual void EnteredState( SAnimationStateData& data );
	virtual void LeftState( SAnimationStateData& data, bool wasEntered );
	virtual void Update( SAnimationStateData& data );
	void GetCompletionTimes( SAnimationStateData& data, CTimeValue start, CTimeValue& hard, CTimeValue& sticky ) 
	{
		hard = sticky = 0.0f;
	}
	virtual const Params * GetParameters();
	virtual IAnimationStateNodeFactory * GetFactory() { return this; }
	virtual void DebugDraw( SAnimationStateData& data, IRenderer * pRenderer, int x, int& y, int yIncrement );
	// ~IAnimationStateNode

	// IAnimationStateNodeFactory
	virtual bool Init( const XmlNodeRef& node, IAnimationGraphPtr );
	virtual void Release();
	virtual IAnimationStateNode * Create();
	virtual const char * GetCategory();
	virtual const char * GetName();
	virtual bool IsLessThan( IAnimationStateNodeFactory * pFactory )
	{
		AG_LT_BEGIN_FACTORY(CAGAdditive);
			AG_LT_ELEM(m_playMode);
			AG_LT_ELEM(m_fadeOutOneLeaving);
			AG_LT_ELEM(m_layer);
			AG_LT_ELEM(m_fadeOutTime);
			AG_LT_ELEM(m_blendInTime);
			AG_LT_ELEM(m_assetWeight);
			AG_LT_ELEM(m_assetSpeed);
			//AG_LT_ELEM(m_animToken);
			AG_LT_ELEM(m_assetName);
		AG_LT_END();
	}
	virtual void SerializeAsFile(bool reading, AG_FILE *file);
	// ~IAnimationStateNodeFactory


	virtual void GetStateMemoryStatistics(ICrySizer * s) {}

	virtual void GetFactoryMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
		s->Add(m_assetName);
	}

private:
	enum EPlayModes
	{
		PLAY_ONCE = 0,
		LOOP_ASSET = 1,
		STOP_LAYER = 2,
	};

	// remember to update IsLessThan() and SerializeAsFile()
	int					m_playMode;
	bool				m_fadeOutOneLeaving;
	int					m_layer;
	float				m_fadeOutTime;
	float				m_blendInTime;
	float				m_assetWeight;
	float				m_assetSpeed;
	string			m_assetName;

	uint32			m_animToken;

	CryCharAnimationParams m_params;
	CAnimationRandomizer* m_animRandomizer;
};

#endif
