#ifndef __SEQUENCETRACK_H__
#define __SEQUENCETRACK_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include "IMovieSystem.h"
#include "AnimTrack.h"

class CSequenceTrack : public TAnimTrack<ISequenceKey>
{
public:
	EAnimTrackType GetType() {return ATRACK_SEQUENCE;}
	EAnimValue GetValueType() {return AVALUE_SEQUENCE;}

	void GetKeyInfo(int key,const char* &description,float &duration);
	void SerializeKey(ISequenceKey &key,XmlNodeRef &keyNode,bool bLoading);

	virtual void GetMemoryUsage( ICrySizer *pSizer ) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
};

#endif // __SEQUENCETRACK_H__