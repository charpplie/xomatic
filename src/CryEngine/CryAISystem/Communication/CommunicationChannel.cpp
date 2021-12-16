#include "StdAfx.h"
#include "CommunicationChannel.h"


CommunicationChannel::CommunicationChannel(const CommunicationChannel::Ptr& parent,
																					 const SCommunicationChannelParams& params)
: m_parent(parent)
, m_minSilence(params.minSilence)
, m_silence(0.0f)
, m_occupied(0)
{
}

void CommunicationChannel::Update(float updateTime)
{
	if (!m_occupied && (m_silence > 0.0f))
		m_silence -= updateTime;
}

void CommunicationChannel::Occupy(bool occupy)
{
	assert((occupy && IsFree()) || (!occupy && !IsFree()));

	if (m_occupied && !occupy)
		m_silence = m_minSilence;

	m_occupied += occupy ? 1 : -1;

	if (m_parent)
		m_parent->Occupy(occupy);

	assert(m_occupied < 2);
}

bool CommunicationChannel::IsFree() const
{
	if (m_parent && !m_parent->IsFree())
		return false;
	
	return !m_occupied && (m_silence <= 0.0f);
}

void CommunicationChannel::Clear()
{
	m_occupied = 0;
	m_silence = 0.0;
}