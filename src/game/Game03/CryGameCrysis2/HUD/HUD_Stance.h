#ifndef __HUDSTANCE_H__
#define __HUDSTANCE_H__

#include "HUDObject.h"
#include "Actor.h"

//////////////////////////////////////////////////////////////////////////


class CHUD_Stance : public CHUDObject
{

public:

	CHUD_Stance();
	virtual ~CHUD_Stance();

	virtual void Update(float frameTime);
	virtual void Init();
	virtual void PreDelete();

private:

	IFlashVariableObject* m_objectStance;

	EStance								m_currentStance;
};


//////////////////////////////////////////////////////////////////////////

#endif

