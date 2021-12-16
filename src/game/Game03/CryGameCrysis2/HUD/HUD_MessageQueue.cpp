#include "StdAfx.h"
#include "HUD/HUD_MessageQueue.h"

//////////////////////////////////////////////////////////////////////////


CHUD_MessageQueue::CHUD_MessageQueue()
: m_objectRoot(NULL)
, m_updateNextFrame(false)
{
	m_messages.reserve(MAX_MESSAGES_IN_QUEUE);
}



CHUD_MessageQueue::~CHUD_MessageQueue()
{
}



void CHUD_MessageQueue::Init()
{
	HUD_FLASVAROBJ_REG( GetAsset2D(), "_root", m_objectRoot);
}



void CHUD_MessageQueue::PreDelete()
{
	HUD_FLASHOBJ_SAFERELEASE(m_objectRoot);
}



void CHUD_MessageQueue::Update(float frameTime)
{
	const int size = m_messages.size();
	if(!size && !m_updateNextFrame)
		return;

	for(int i=0; i<size; ++i)
	{
		m_messages[i].m_timer -= frameTime;
	}

	if(m_updateNextFrame)
	{
		CryFixedWStringT<384> result(L"");
		for(int i=0; i<size; ++i)
		{
			if(i>0)
			{
				result.append(L"\n");
			}
			result.append(m_messages[i].m_message);
		}
		m_objectRoot->SetMember("IncomingComms", result.c_str());
		m_updateNextFrame = false;
	}

	if (0 < size)
	{
		if(m_messages[size-1].m_timer <= 0.0f)
		{
			m_messages.pop_back();
			m_updateNextFrame = true;
		}
	}
}



void CHUD_MessageQueue::OnHUDEvent(const SHUDEvent& event)
{
	CRY_ASSERT(event.eventType == eHUDEvent_AddOnScreenMessage);
	const char* message = static_cast<const char*>(event.GetData(0).GetPtr());
	const char* param1 = NULL;
	const char* param2 = NULL;
	if(event.GetDataSize()>1)
	{
		param1 = static_cast<const char*>(event.GetData(1).GetPtr());
	}
	if(event.GetDataSize()>2)
	{
		param2 = static_cast<const char*>(event.GetData(2).GetPtr());
	}
	OnAddMessage(message, param1, param2);
}



void CHUD_MessageQueue::OnAddMessage(const char* msg, const char* param1, const char* param2)
{
	SHUDMessage message(g_pGame->GetHUD()->LocalizeStringW(msg, param1, param2), 5.0f);
	if(m_messages.size()>=MAX_MESSAGES_IN_QUEUE)
	{
		m_messages.pop_back();
	}
	m_messages.insert(m_messages.begin(), 1, message);
	m_updateNextFrame = true;
}


//////////////////////////////////////////////////////////////////////////