#include "StdAfx.h"

#include "mayaDagPathPool.h"

mayaDagPathPool::mayaDagPathPool() {};

mayaDagPathPool::~mayaDagPathPool()
{
	int i;
	for( i = 0;i<m_dagPathPool.size();i++ )
	{
		if( m_dagPathPool[i] )
		{
			delete m_dagPathPool[i];
			m_dagPathPool[i] = 0;
		}
	}	
}

void *mayaDagPathPool::getPathPointer( MDagPath &path )
{
	// Not a real pool, just an array of paths
	MDagPath *newPath = new MDagPath( path );
	this->m_dagPathPool.push_back( newPath );
	return (void*)newPath;
}