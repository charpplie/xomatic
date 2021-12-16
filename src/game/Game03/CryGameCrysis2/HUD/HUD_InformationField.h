#ifndef __HUDINFORMATIONFIELD_H__
#define __HUDINFORMATIONFIELD_H__

#include "HUDObject.h"

//////////////////////////////////////////////////////////////////////////


class CHUD_InformationField : public CHUDObject
{

public:

	CHUD_InformationField();
	virtual ~CHUD_InformationField();

	virtual void		Init();
	virtual void		PreDelete();

private:

	IFlashVariableObject*		m_objectRoot;

};


//////////////////////////////////////////////////////////////////////////

#endif

