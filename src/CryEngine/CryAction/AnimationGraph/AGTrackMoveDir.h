#ifndef __AGTrackMoveDir_H__
#define __AGTrackMoveDir_H__

#pragma once

#include "IAnimationStateNode.h"
//#include "AnimationGraphCVars.h"


class CAGTrackMoveDirFactory : public IAnimationStateNodeFactory
{
	friend class CAGTrackMoveDir;

public:
	CAGTrackMoveDirFactory() {}

	// IAnimationStateNodeFactory
	virtual bool Init( const XmlNodeRef& node, IAnimationGraphPtr );
	virtual void Release();
	virtual IAnimationStateNode * Create();
	virtual const char * GetCategory();
	virtual const char * GetName();
	virtual const Params * GetParameters();
	virtual void SerializeAsFile(bool reading, AG_FILE *file);

	bool IsLessThan( IAnimationStateNodeFactory * pFactory )
	{
		AG_LT_BEGIN(CAGTrackMoveDirFactory);
		AG_LT_ELEM(m_trackingTime);
		AG_LT_END();
	}
	// ~IAnimationStateNodeFactory

	virtual void GetFactoryMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

private:

	float m_trackingTime;
};

class CAGTrackMoveDir : public IAnimationStateNode
{
public:
	CAGTrackMoveDir( CAGTrackMoveDirFactory * pFactory );

	// IAnimationStateNode
	virtual void EnterState( SAnimationStateData& data, bool dueToRollback );
	virtual EHasEnteredState HasEnteredState( SAnimationStateData& data ) { return eHES_Instant; }
	virtual bool CanLeaveState( SAnimationStateData& data );
	virtual void LeaveState( SAnimationStateData& data );
	virtual void Update( SAnimationStateData& data );

	void DebugRender( SAnimationStateData &data, Vec3 newMoveDir );
	virtual void GetCompletionTimes( SAnimationStateData& data, CTimeValue start, CTimeValue& hard, CTimeValue& sticky );
	virtual IAnimationStateNodeFactory * GetFactory() { return m_pFactory; }
	virtual void EnteredState(SAnimationStateData& data ) {}
	virtual void LeftState(SAnimationStateData& data, bool wasEntered) {delete this;}
	virtual void DebugDraw( SAnimationStateData& data, IRenderer * pRenderer, int x, int& y, int yIncrement ) {}
	// ~IAnimationStateNode

	virtual void GetStateMemoryStatistics(ICrySizer * s)
	{
		s->Add(*this);
	}

private:

	Vec3 GetMoveDir( SAnimationStateData& data );

	CAGTrackMoveDirFactory * m_pFactory;

	Vec3	m_interpolatedMoveDir;
	int		m_lastFrameId;

};


#endif // __AGTrackMoveDir_H__
