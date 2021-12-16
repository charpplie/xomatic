#ifndef __MANN_DOPE_SHEET__H__
#define __MANN_DOPE_SHEET__H__

#include "../SequencerDopeSheet.h"

class CMannDopeSheet : public CSequencerDopeSheet
{
	DECLARE_DYNAMIC(CMannDopeSheet)

public:
	CMannDopeSheet()
	{
	}

	~CMannDopeSheet()
	{
	}

	bool IsDraggingTime() const
	{
		//--- Dammit
		return m_mouseMode == 4;
	}

	float GetTime() const
	{
		return m_currentTime;
	}
};

#endif
