#ifndef __HUDMESSAGEQUEUE_H__
#define __HUDMESSAGEQUEUE_H__

#include "HUDObject.h"


#define MAX_MESSAGES_IN_QUEUE 3

//////////////////////////////////////////////////////////////////////////


class CHUD_MessageQueue : public CHUDObject
{

public:

	typedef CryFixedWStringT<128> TMessageString;

	struct SHUDMessage
	{
		SHUDMessage(TMessageString message, float timer)
			: m_timer(timer)
			, m_message(message)
		{

		}
		
		float						m_timer;
		TMessageString	m_message;
	};

	typedef std::vector<SHUDMessage> TMessages;

	CHUD_MessageQueue();
	virtual ~CHUD_MessageQueue();

	virtual void Update(float frameTime);
	virtual void Init();
	virtual void PreDelete();
	virtual void OnHUDEvent(const SHUDEvent& event);

private:

	void OnAddMessage(const char* msg, const char* param1, const char* param2);

	TMessages m_messages;
	IFlashVariableObject* m_objectRoot;
	bool m_updateNextFrame;
};


//////////////////////////////////////////////////////////////////////////

#endif

