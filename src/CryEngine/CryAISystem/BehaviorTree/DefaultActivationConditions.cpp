/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   DefaultActivationConditions.cpp
$Id$
$DateTime$
Description: Manages the LUA table with the default values for the state
of each puppet that wants to use the Behavior Tree.
---------------------------------------------------------------------
History:
- 04:07:2007 : Created by Ricardo Pillosu

*********************************************************************/

#include "StdAfx.h"
#include "DefaultActivationConditions.h"
#include "ActivationConditionsTable.h"

// Scope names
static char const* g_szScopeNames[eACS_COUNT] =
{
	"Global",	// eACS_Global
	"Actor",	// eACS_Actor
};

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CDefaultActivationConditions::CDefaultActivationConditions( XmlNodeRef& xmlTable, const string& sProfile, EActCondScopes eScope )
: m_bInit( false )
, m_eDefScope( eScope )
, m_sProfile( sProfile )
{
	CRY_ASSERT( m_eDefScope < eACS_COUNT );
	m_bInit = Reload( xmlTable );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CDefaultActivationConditions::~CDefaultActivationConditions()
{
	Shutdown();
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
void CDefaultActivationConditions::Shutdown()
{
	for( uint32 uIndex = 0; uIndex < m_vecScopes.size(); ++uIndex )
	{
		SAFE_DELETE( m_vecScopes[uIndex] );
	}

	m_vecScopes.clear();
}

// Description:
//   AddLocation
// Arguments:
//
// Return:
//
bool CDefaultActivationConditions::Reload( XmlNodeRef& xmlTable )
{
	bool bRet = false;

	Shutdown();

	bool bFirstSkipped = false;
	for( int iRow = 0; iRow < xmlTable->getChildCount(); ++iRow )
	{
		XmlNodeRef	xmlRow = xmlTable->getChild( iRow );

		if( xmlRow->isTag("Row") == false )
		{
			continue;
		}

		if( bFirstSkipped == false )
		{
			bFirstSkipped = true;
			continue;
		}

		bRet = true;

		string sName;
		string sScope;
		string sDefValue;

		for( int iCell = 0; iCell < xmlRow->getChildCount(); ++iCell )
		{
			XmlNodeRef xmlCell = xmlRow->getChild( iCell );
			XmlNodeRef xmlCellData = xmlCell->findChild( "Data" );

			if( xmlCell->isTag("Cell") == false || !xmlCellData )
			{
				continue;
			}

			string sType = xmlCellData->getAttr( "ss:Type" );

			switch( iCell )
			{
			case 0:
				sName = xmlCellData->getContent();
				sName.Trim();
				break;

			case 1:
				sDefValue = xmlCellData->getContent();
				sDefValue.Trim();

				if( sType.compareNoCase("Boolean") == 0 )
				{
					sDefValue.Format( "%s", (sDefValue[0] == '0' ? "false" : "true") );
				}

				CreateEntry( sName, g_szScopeNames[m_eDefScope], sDefValue );
				break;
			}
		}
	}

	return( bRet );
}

// Description:
//   AddLocation
// Arguments:
//
// Return:
//
void CDefaultActivationConditions::CreateEntry( const string& sName, const string& sScope, const string& sDefValue, const char* sParentTable )
{
	CActivationConditionTable *pScope = GetValuesFromScope( sScope );

	if( pScope == NULL )
	{
		pScope = new CActivationConditionTable( sScope, sParentTable, m_sProfile );
		m_vecScopes.push_back( pScope );
	}

	ScriptAnyValue Dumb;
	if( Dumb.GetVarType() == svtNull )
	{
		pScope->CreateValue( sName, sDefValue );
	}
}

// Description:
//   AddLocation
// Arguments:
//
// Return:
//
CActivationConditionTable const* CDefaultActivationConditions::GetValuesFromScope( const char* sScope ) const
{
	assert( sScope != NULL );

	CActivationConditionTable const* pScope = NULL;

	for( uint32 uIndex = 0; uIndex < m_vecScopes.size(); ++uIndex )
	{
		if( m_vecScopes[uIndex]->GetName().compareNoCase(sScope) == 0 )
		{
			pScope = m_vecScopes[uIndex];
			break;
		}
	}

	return( pScope );
}

// Description:
//   AddLocation
// Arguments:
//
// Return:
//
CActivationConditionTable const* CDefaultActivationConditions::GetValuesFromScope( EActCondScopes eScope ) const
{
	CRY_ASSERT( eScope < eACS_COUNT );
	return( GetValuesFromScope(g_szScopeNames[eScope]) );
}

// Description:
//   AddLocation
// Arguments:
//
// Return:
//
CActivationConditionTable* CDefaultActivationConditions::GetValuesFromScope( const string& sScope )
{
	CActivationConditionTable *pScope = NULL;

	for( uint32 uIndex = 0; uIndex < m_vecScopes.size(); ++uIndex )
	{
		if( m_vecScopes[uIndex]->GetName().compareNoCase(sScope) == 0 )
		{
			pScope = m_vecScopes[uIndex];
			break;
		}
	}

	return( pScope );
}
