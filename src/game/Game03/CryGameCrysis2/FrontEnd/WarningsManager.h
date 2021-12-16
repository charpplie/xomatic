#ifndef __HUDWARNINGSMANAGER_H__
#define __HUDWARNINGSMANAGER_H__


//////////////////////////////////////////////////////////////////////////


#include "IFlashPlayer.h"
#include "IHardwareMouse.h"
#include "StringUtils.h"


//////////////////////////////////////////////////////////////////////////


struct IFlashPlayer;



typedef uint32 THUDWarningId;



struct IHUDWarningsListener
{
	virtual bool OnWarningReturn(THUDWarningId id, const char* returnValue) { return true; }
	virtual void OnWarningRemoved(THUDWarningId id) {}
};

enum EHUDWarningIcon
{
	eHUDWI_none,
	eHUDWI_attention,
	eHUDWI_question,
	eHUDWI_loading,
};

struct SHUDWarning
{
	SHUDWarning()
		: m_title("")
		, m_message("")
		, m_btn1("")
		, m_btn1return("")
		, m_btn2("")
		, m_btn2return("")
		, m_icon(eHUDWI_none)
		, m_priority(0)
	{
	}

	void GetMemoryUsage(ICrySizer *pSizer) const{}

	THUDWarningId				m_id;

	CryFixedStringT<64> m_title;
	CryFixedStringT<128> m_message;
	CryFixedStringT<64> m_btn1;
	CryFixedStringT<64> m_btn1return;
	CryFixedStringT<64> m_btn2;
	CryFixedStringT<64> m_btn2return;
	EHUDWarningIcon			m_icon;
	int									m_priority;
};



struct SHUDWarningInstance
{
	SHUDWarningInstance()
		: m_pListener(NULL)
		, m_pPlayer (NULL)
		, m_priority(0)
	{
	}

	void GetMemoryUsage(ICrySizer *pSizer) const{}

	THUDWarningId					m_id;

	IHUDWarningsListener*	m_pListener;
	IFlashPlayer*					m_pPlayer;
	int										m_priority;

	bool operator ==( const SHUDWarningInstance& compare ) const
	{
		return m_priority == compare.m_priority;
	}

	bool operator <( const SHUDWarningInstance& compare ) const
	{
		return m_priority > compare.m_priority;
	}
};


//////////////////////////////////////////////////////////////////////////


class CWarningsManager : public IFSCommandHandler, IHardwareMouseEventListener
{
public:

	CWarningsManager();
	virtual ~CWarningsManager();

	// IHardwareMouseEventListener
	virtual void OnHardwareMouseEvent(int iX,int iY, EHARDWAREMOUSEEVENT eHardwareMouseEvent, int wheelDelta = 0);
	// ~IHardwareMouseEventListener

	// IFSCommandHandler
	virtual void HandleFSCommand(const char* pCommand, const char* pArgs, void* pUserData = 0);
	// ~IFSCommandHandler


	virtual void Update	(float frameTime);
	virtual void Draw		();

	void OnInputEvent(const SInputEvent &rInputEvent);

	bool IsActive() const;

	void AddWarning(const char* stringId, const char* paramMessage, IHUDWarningsListener* pListener)
	{
		AddWarning(GetWarningId(stringId), pListener, paramMessage);
	}

	void AddWarning(const char* stringId, IHUDWarningsListener* pListener)
	{
		AddWarning(GetWarningId(stringId), pListener);
	}

	void RemoveWarning(const char* stringId)
	{
		RemoveWarning(GetWarningId(stringId));
	}

	THUDWarningId GetWarningId(const char* stringId) const
	{
		return CryStringUtils::CalculateHash(stringId);
	}

	void RemoveWarning(const THUDWarningId id);
	void AddWarning(const THUDWarningId id, IHUDWarningsListener* pListener, const char *paramMessage = NULL);

	void ClearCurrentWarnings();
	void LoadWarnings();
	
	void GetMemoryUsage( ICrySizer *pSizer ) const;
private:

	const SHUDWarning* GetWarning(const THUDWarningId id) const;
	const SHUDWarningInstance* GetWarningInstance(const THUDWarningId id) const;
	void DeleteWarning(const THUDWarningId id);

	typedef std::vector<SHUDWarning> TWarnings;
	TWarnings m_warnings;

	typedef std::vector<SHUDWarningInstance> TWarningsStack;
	TWarningsStack m_stack;

	typedef std::vector<THUDWarningId> TRemoveSchedule;
	TRemoveSchedule m_remove;
};


//////////////////////////////////////////////////////////////////////////

#endif

