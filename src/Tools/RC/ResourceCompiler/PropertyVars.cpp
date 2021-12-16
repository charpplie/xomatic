////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek GmbH, 2010.
// -------------------------------------------------------------------------
//  File name:   PropertyVars.cpp
//  Version:     v1.00
//  Created:     02/02/2010 by Timur.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "PropertyVars.h"
#include "IRCLog.h"

CPropertyVars::CPropertyVars( IResourceCompiler *pRC )
{
	m_pRC = pRC;
}

//////////////////////////////////////////////////////////////////////////
void CPropertyVars::SetProperty( const string &name,const string &value )
{
	m_properties[name] = value;
}

//////////////////////////////////////////////////////////////////////////
void CPropertyVars::RemoveProperty( const string &name )
{
	m_properties.erase(name);
}

//////////////////////////////////////////////////////////////////////////
void CPropertyVars::ClearProperties()
{
	m_properties.clear();
}

//////////////////////////////////////////////////////////////////////////
void CPropertyVars::ExpandProperties( string &str )
{
	ExpandPropertiesRecursive( str,str );
}

void CPropertyVars::ExpandPropertiesRecursive( string &str,string &original )
{
	int startpos = str.find( "${" );
	if (startpos < 0)
		return;

	int endpos = str.find( '}',startpos+2 );
	if (endpos < 0)
		return;

	// Find property.
	string propName = str.substr(startpos+2,endpos-startpos-2);
	PropertyMap::const_iterator it = m_properties.find(propName);
	if (it == m_properties.end())
	{
		RCLogError( "Unknown property name ${%s} in input string %s",propName.c_str(),original.c_str() );
		return;
	}
	string value = it->second;
	
	str = str.replace( startpos,endpos-startpos+1,value );

	// Repeat with rest of the string.
	if (str.find('$') >= 0)
		ExpandPropertiesRecursive(str,original);
}
