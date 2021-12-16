
#ifndef __MAYADAGPATHPOOL_H__
#define __MAYADAGPATHPOOL_H__

#include "mayaIncludes.h"

class mayaDagPathPool
{
public:
	mayaDagPathPool();
	virtual ~mayaDagPathPool();

	void *getPathPointer( MDagPath &path );

private:
	std::vector< MDagPath* > m_dagPathPool;
};

#endif // __MAYADAGPATHPOOL_H__