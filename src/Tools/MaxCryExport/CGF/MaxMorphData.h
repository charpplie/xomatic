//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __MAXMORPHDATA_H__
#define __MAXMORPHDATA_H__

#include "IMorphData.h"
#include "IMorphChannel.h"
#include "IMorphChannelArray.h"

#include <vector>

class MorphR3;
class morphChannel;

class MaxMorphData : public IMorphData
{
public:
	MaxMorphData(MorphR3* pMorpher, float fMinOffset);
	~MaxMorphData();

	// IMorphData
	virtual IMorphChannelArray* GetMorphChannels();
	virtual float GetMinOffset();

private:
	class Channel : public IMorphChannel
	{
	public:
		Channel(morphChannel& rChannel);

		// IMorphChannel
		virtual int GetNumPoints();
		virtual std::string GetName();
		virtual const Vec3& GetPosition(int i);
		virtual const Vec3& GetDelta(int i);

	private:
		morphChannel& rChannel;
		std::vector<Vec3> positions;
		std::vector<Vec3> deltas;
	};

	class ChannelArray : public IMorphChannelArray
	{
	public:
		ChannelArray();
		~ChannelArray();

		// IMorphChannelArray
		virtual int Count();
		virtual IMorphChannel* Get(int i);

		void AddChannel(Channel* pChannel);

	private:
		std::vector<Channel*> channels;
	};

	void CreateChannels(ChannelArray* pChannelArray, MorphR3* pMorpher);

	MorphR3* pMorpher;
	ChannelArray* pChannelArray;
	float fMinOffset;
};

#endif //__MAXMORPHDATA_H__
