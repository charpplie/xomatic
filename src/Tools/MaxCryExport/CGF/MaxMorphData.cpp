//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "MaxMorphData.h"
#include "MorpherUtils.h"

MaxMorphData::MaxMorphData(MorphR3* pMorpher, float fMinOffset)
:	pMorpher(pMorpher),
	pChannelArray(0),
	fMinOffset(fMinOffset)
{
	this->pChannelArray = new ChannelArray();
	this->CreateChannels(this->pChannelArray, this->pMorpher);
}

MaxMorphData::~MaxMorphData()
{
	delete this->pChannelArray;
	this->pChannelArray = 0;
}

IMorphChannelArray* MaxMorphData::GetMorphChannels()
{
	return this->pChannelArray;
}

float MaxMorphData::GetMinOffset()
{
	return this->fMinOffset;
}

void MaxMorphData::CreateChannels(ChannelArray* pChannelArray, MorphR3* pMorpher)
{
	int nNumChannnels = pMorpher->chanBank.size();
	for (int nChannel = 0; nChannel < nNumChannnels; ++nChannel)
	{
		morphChannel& rChannel = pMorpher->chanBank[nChannel];
		if (rChannel.mActive)
		{
			rChannel.rebuildChannel();
			pChannelArray->AddChannel(new Channel(rChannel));
		}
	}
}

MaxMorphData::Channel::Channel(morphChannel& rChannel)
:	rChannel(rChannel)
{
	this->positions.resize(rChannel.mNumPoints);
	{for (int i = 0; i < rChannel.mNumPoints; ++i)
	{
		this->positions[i].x = rChannel.mPoints[i].x;
		this->positions[i].y = rChannel.mPoints[i].y;
		this->positions[i].z = rChannel.mPoints[i].z;
	}}

	this->deltas.resize(rChannel.mNumPoints);
	{for (int i = 0; i < rChannel.mNumPoints; ++i)
	{
		this->deltas[i].x = rChannel.mDeltas[i].x;
		this->deltas[i].y = rChannel.mDeltas[i].y;
		this->deltas[i].z = rChannel.mDeltas[i].z;
	}}
}

int MaxMorphData::Channel::GetNumPoints()
{
	return this->rChannel.mNumPoints;
}

std::string MaxMorphData::Channel::GetName()
{
	return rChannel.mName.data();
}

const Vec3& MaxMorphData::Channel::GetPosition(int i)
{
	return this->positions[i];
}

const Vec3& MaxMorphData::Channel::GetDelta(int i)
{
	return this->deltas[i];
}

MaxMorphData::ChannelArray::ChannelArray()
{
}

MaxMorphData::ChannelArray::~ChannelArray()
{
	for (std::vector<Channel*>::iterator itChannel = this->channels.begin(); itChannel != this->channels.end(); ++itChannel)
		delete *itChannel;
	this->channels.clear();
}

int MaxMorphData::ChannelArray::Count()
{
	return int(this->channels.size());
}

IMorphChannel* MaxMorphData::ChannelArray::Get(int i)
{
	return this->channels[i];
}

void MaxMorphData::ChannelArray::AddChannel(Channel* pChannel)
{
	this->channels.push_back(pChannel);
}
