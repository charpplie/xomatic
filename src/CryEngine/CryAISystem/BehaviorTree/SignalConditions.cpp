/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   SignalConditions.cpp
$Id$
$DateTime$
Description: Small class to store and manage the LUA code to execute
every time certain signal is received. In most cases, this code will
just change a value (or more than one value) in the puppet's LUA 
table (CPersonalActivationConditions).
---------------------------------------------------------------------
History:
- 04:07:2007 : Created by Ricardo Pillosu

*********************************************************************/

#include "StdAfx.h"
#include "SignalConditions.h"

#include <StringUtils.h>

// Description:
//   Constructor
// Arguments:
//
// Return:
//
SSignalReaction::SSignalReaction( const char* sSignal, SSignalReaction* pParent )
{
	assert( sSignal != NULL );

	this->sSignal = sSignal;
	this->pParent = pParent;

	this->sSignal.Trim();
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
void SSignalReaction::AddReaction( const string& sReaction )
{
	if( sReaction.empty() == false )
	{
		IScriptSystem *pScript = gEnv->pScriptSystem;
		vReactions.push_back( pScript->PreCacheBuffer(sReaction, sReaction.length(), "AI Signal Reaction") );
	}
}

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CSignalConditions::CSignalConditions( XmlNodeRef& xmlTable )
{
	m_bInit = Reload( xmlTable );
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CSignalConditions::~CSignalConditions()
{
	Shutdown();
}

// Description:
//
// Arguments:
//
// Return:
//
void CSignalConditions::Shutdown()
{
	TSignalReactions::iterator it = m_mapReactions.begin();
	TSignalReactions::iterator end = m_mapReactions.end();
	for( ; it != end; ++it)
	{
		SAFE_DELETE( it->second );
	}

	m_mapReactions.clear();
}

// Description:
//
// Arguments:
//
// Return:
//
bool CSignalConditions::Reload( XmlNodeRef& xmlTable )
{
	bool bRet = false;

	Shutdown();

	TSignalParents mapParents;

	bool bFirstSkipped = false;
	for( int iRow = 0; iRow < xmlTable->getChildCount(); ++iRow )
	{
		XmlNodeRef xmlRow = xmlTable->getChild( iRow );

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

		SSignalReaction *pReaction = NULL;
		string sParent( "" );

		for( int iCell = 0; iCell < xmlRow->getChildCount(); ++iCell )
		{
			XmlNodeRef xmlCell = xmlRow->getChild( iCell );
			XmlNodeRef xmlCellData = xmlCell->findChild( "Data" );

			if( xmlCell->isTag("Cell") == false || !xmlCellData )
			{
				continue;
			}

			uint32 iColumn = iCell;
			if( xmlCell->getAttr("ss:Index", iColumn) == true )
			{
				--iColumn;
			}

			switch( iColumn )
			{
			case 0:
				pReaction = new SSignalReaction( xmlCellData->getContent(), NULL );
				break;

			case 1:
				if( pReaction != NULL )
				{
					sParent = xmlCellData->getContent();
					sParent.Trim();
				}

				break;

			default:
				if( pReaction != NULL )
				{
					pReaction->AddReaction( xmlCellData->getContent() );
				}

				break;
			}
		}

		if( pReaction != NULL )
		{
			int hash = CryStringUtils::CalculateHash(pReaction->sSignal.c_str());

			SSignalReaction *pReactionExisting = stl::find_in_map(m_mapReactions, hash, NULL);
			assert(pReactionExisting == NULL);
			if(pReactionExisting)
				AIError( "Hash collision while adding reaction in CSignalConditions::Reload" );

			mapParents[hash] = sParent;
			m_mapReactions[hash] = pReaction;
		}

		pReaction = NULL;
		sParent = "";
	}

	AssignParents( mapParents );
	CheckParentsRecursivity();

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
void CSignalConditions::AssignParents( const TSignalParents& vParents )
{
	TSignalReactions::iterator it = m_mapReactions.begin();
	TSignalReactions::iterator end = m_mapReactions.end();
	TSignalParents::const_iterator itParents = vParents.begin();
	TSignalParents::const_iterator endParents = vParents.end();
	for( ; itParents != endParents && it != end; ++itParents, ++it )
	{
		if( itParents->second.empty() == false )
		{
			SSignalReaction const*	pParent = GetSignalReaction( itParents->second );

			if( pParent != NULL )
			{
				it->second->pParent = pParent;
			}
			else
			{
				AIError(
					"AC: Invalid Parent signal reaction [%s] from signal [%s]",
					itParents->second.c_str(),
					it->second->sSignal.c_str() );
				assert( false && "AC: Invalid Parent signal reaction" );
			}
		}
	}
}

// Description:
//   Checks for infinite recursivity cases
// Arguments:
//
// Return:
//
void CSignalConditions::CheckParentsRecursivity()
{
	TSignalReactions::iterator it = m_mapReactions.begin();
	TSignalReactions::iterator end = m_mapReactions.end();
	for( ; it != end; ++it )
	{
		SSignalReaction const* pReaction = it->second;

		if( pReaction->pParent != NULL )
		{
			std::vector<SSignalReaction const *> vecRecordedReactions;

			do
			{
				if( std::find(vecRecordedReactions.begin(), vecRecordedReactions.end(), pReaction) != vecRecordedReactions.end() )
				{
					CryFatalError(
						"Infinite loop detected in Activation Conditions xml, check signal [%s]",
						pReaction->sSignal.c_str() );
					break;
				}

				vecRecordedReactions.push_back( pReaction );
				pReaction = pReaction->pParent;
			} while( pReaction != NULL );
		}
	}
} 

// Description:
//
// Arguments:
//
// Return:
// 
SSignalReaction* CSignalConditions::GetSignalReaction( const char* sSignal ) const
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	assert( sSignal != NULL );

	uint32 hash = CryStringUtils::CalculateHash(sSignal);
	SSignalReaction *pReaction = stl::find_in_map(m_mapReactions, hash, NULL);
	if(pReaction && stricmp(pReaction->sSignal.c_str(), sSignal))
	{
		CryWarning(VALIDATOR_MODULE_AI, VALIDATOR_ERROR, "CSignalConditions::GetSignalReaction: Retrieving signal %s failed, found signal %s for generated hash.", sSignal, pReaction->sSignal.c_str());
		return NULL;
	}

	return( pReaction );
}
