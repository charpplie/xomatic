#ifndef IAIOBJECTMANAGER
#define IAIOBJECTMANAGER

#if _MSC_VER > 1000
#pragma once
#endif


#include "IAIObject.h"


struct IAIObjectManager
{
	virtual IAIObject *CreateAIObject(unsigned short type, IAIObject *pAssociation, EntityId entityID) = 0;
	virtual void RemoveObject(tAIObjectID objectID) = 0;//same as destroy??

	virtual IAIObject *GetAIObject(tAIObjectID aiObjectID) = 0;
	virtual IAIObject *GetAIObjectByName(unsigned short type, const char *pName) const = 0;


	// Returns AIObject iterator for first match, see EGetFirstFilter for the filter options.
	// The parameter 'n' specifies the type, group id or species based on the selected filter.
	// It is possible to iterate over all objects by setting the filter to OBJFILTER_TYPE
	// passing zero to 'n'.
	virtual IAIObjectIter* GetFirstAIObject(IAISystem::EGetFirstFilter filter, short n) = 0;
	// Iterates over AI objects within specified range.
	// Parameter 'pos' and 'rad' specify the enclosing sphere, for other parameters see GetFirstAIObject.
	virtual IAIObjectIter* GetFirstAIObjectInRange(IAISystem::EGetFirstFilter filter, short n, const Vec3& pos, float rad, bool check2D) = 0;
};


#endif