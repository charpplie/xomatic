#include "StdAfx.h"

#include "mayaNamePool.h"

mayaNamePool::mayaNamePool() {};

mayaNamePool::~mayaNamePool()
{
	int i;
	for( i = 0;i<m_namePool.size();i++ )
	{
		if( m_namePool[i] )
		{
			delete m_namePool[i];
			m_namePool[i] = 0;
		}
	}	
}

void *mayaNamePool::getPointer( const char *name )
{
	// Not a real pool at the moment. Just new a string and keep a pointer to it.
	// This could be done better with a hash table to save memory.
	std::string *newString = new std::string;
	newString->assign( name );
	this->m_namePool.push_back( newString );
	return (void*)newString;
}

void *mayaNamePool::getPointer( const std::string &name )
{
	return this->getPointer( name.c_str() );
}