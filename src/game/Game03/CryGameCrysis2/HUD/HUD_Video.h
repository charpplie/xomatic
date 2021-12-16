#ifndef __HUDVIDEO_H__
#define __HUDVIDEO_H__

#include "HUDObject.h"

//////////////////////////////////////////////////////////////////////////


class CHUD_Video : public CHUDObject
{

public:

	CHUD_Video();
	virtual ~CHUD_Video();

	virtual void		Init();
	virtual void		PreDelete();

private:

	IFlashVariableObject*		m_objectRoot;
	IFlashVariableObject*		m_objectComs;

};


//////////////////////////////////////////////////////////////////////////

#endif

