////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek GmbH, 2010.
// -------------------------------------------------------------------------
//  File name:   PropertyVars.h
//  Version:     v1.00
//  Created:     02/02/2010 by Timur.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __PropertyVars_h__
#define __PropertyVars_h__
#pragma once

class CPropertyVars
{
public:
	CPropertyVars( IResourceCompiler *pRC );
	
	void SetProperty( const string &name,const string &value );
	void RemoveProperty( const string &name );
	void ClearProperties();

	// Expand variables as ${propertyName} with the value of propertyName variable
	// Return number of replaced properties
	void ExpandProperties( string &str );

private:
	void ExpandPropertiesRecursive( string &str,string &original );

	typedef std::map<string,string> PropertyMap;
	PropertyMap m_properties;

	IResourceCompiler *m_pRC;
};

#endif //__PropertyVars_h__