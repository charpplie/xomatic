#ifndef __SYSTEMEVENTDISPATCHER_H__
#define __SYSTEMEVENTDISPATCHER_H__

#include <ISystem.h>

class CSystemEventDispatcher : public ISystemEventDispatcher
{
public:
	virtual ~CSystemEventDispatcher(){}

	// ISystemEventDispatcher
	VIRTUAL bool RegisterListener(ISystemEventListener *pListener);
	VIRTUAL bool RemoveListener(ISystemEventListener *pListener);

	VIRTUAL void OnSystemEvent( ESystemEvent event,UINT_PTR wparam,UINT_PTR lparam );
	// ~ISystemEventDispatcher
private:
	typedef std::list<ISystemEventListener*>	TSystemEventListeners;
	TSystemEventListeners	m_listeners;
};

#endif //__SYSTEMEVENTDISPATCHER_H__
