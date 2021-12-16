/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   EmotionalInfo.cpp
Description: Stores in memory all data from each XML file. This data,
basically, connects events to how they affect emotional state
---------------------------------------------------------------------
History:
- 08:05:2008 : Created by ricardo

*********************************************************************/
#include "StdAfx.h"
#include "EmotionalInfo.h"
#include "EmotionalSystem.h"
#include "IGameFramework.h"

// Description:
//   Constructor
// Arguments:
//
// Return:
//
CEmotionalInfo::CEmotionalInfo() : m_bInit( false ), m_bRegistered( false )
{
}

// Description:
//   Destructor
// Arguments:
//
// Return:
//
CEmotionalInfo::~CEmotionalInfo()
{
	Shutdown();
}

// Description:
//
// Arguments:
//
// Return:
//
bool CEmotionalInfo::Init( const string& sFile )
{
	m_bInit = Reload( sFile );
	return( m_bInit );
}

// Description:
//
// Arguments:
//
// Return:
//
void CEmotionalInfo::Shutdown()
{
	std::vector<SEmotionalReaction *>::iterator itC = m_vecReactions.begin();
	std::vector<SEmotionalReaction *>::iterator itEnd = m_vecReactions.end();
	for( ; itC != itEnd; ++itC )
	{
		SAFE_DELETE( *itC );
	}

	m_vecReactions.clear();
}

// Description:
//
// Arguments:
//
// Return:
//
bool CEmotionalInfo::Reload( const string& sFile )
{
	bool bRet = false;

	string	sPath( EMOTIONAL_INFO_PATH );
	sPath.append( sFile );

	XmlNodeRef xmlFile = GetISystem()->LoadXmlFile( sPath );

	if( xmlFile )
	{
		// Wipe all current data
		m_sConfigFile = sFile;
		Shutdown();

		// Now look for the proper area of the XML to load data
		for( int iSheet = 0; iSheet < xmlFile->getChildCount(); ++iSheet )
		{
			XmlNodeRef xmlSheet = xmlFile->getChild( iSheet );

			if( xmlSheet->isTag("Worksheet") == false )
			{
				continue;
			}

			string sSheetName = xmlSheet->getAttr( "ss:Name" );
			XmlNodeRef xmlTable = xmlSheet->findChild( "Table" );

			if( xmlTable && sSheetName.compareNoCase("Events") == 0 )
			{
				// do something here
				bRet = LoadSheet( xmlTable );
			}
		}
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CEmotionalInfo::LoadSheet( XmlNodeRef xmlNode )
{
	assert( static_cast<bool>(xmlNode) );

	bool bRet = false;

	bool bFirstSkipped = false;
	for( int iRow = 0; iRow < xmlNode->getChildCount(); ++iRow )
	{
		XmlNodeRef xmlRow = xmlNode->getChild( iRow );

		if( xmlRow->isTag("Row") == false )
		{
			continue;
		}

		if( bFirstSkipped == false )
		{
			bFirstSkipped = true;
			continue;
		}

		bRet = LoadRow( xmlRow );

		if( bRet == false )
		{
			break;
		}
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
bool CEmotionalInfo::LoadRow( XmlNodeRef xmlRow )
{
	assert( static_cast<bool>(xmlRow) );

	bool bRet = false;

	SEmotionalReaction*		pReaction = NULL;

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
				pReaction = new SEmotionalReaction( xmlCellData->getContent() );
				break;

			case 1:
				if( pReaction != NULL )
				{
					pReaction->m_iValue = atoi( xmlCellData->getContent() );
				}

				break;

			case 2:
				if( pReaction != NULL )
				{
					const char*		pData = xmlCellData->getContent();
					if( pData != NULL && strlen(pData) > 0 )
					{
						if( pData[0] == '=' )
						{
							pReaction->m_bForcePleasure = true;
							pReaction->m_fPleasure = (float)atof( &pData[1] );
						}
						else
						{
							pReaction->m_fPleasure = (float)atof( pData );
						}
					}
				}

				break;

			case 3:
				if( pReaction != NULL )
				{
					const char*		pData = xmlCellData->getContent();
					if( pData != NULL && strlen(pData) > 0 )
					{
						if( pData[0] == '=' )
						{
							pReaction->m_bForceClarity = true;
							pReaction->m_fClarity = (float)atof( &pData[1] );
						}
						else
						{
							pReaction->m_fClarity = (float)atof( pData );
						}
					}
				}

				break;

			case 4:
				if( pReaction != NULL )
				{
					const char*		pData = xmlCellData->getContent();
					if( pData != NULL && strlen(pData) > 0 )
					{
						if( pData[0] == '=' )
						{
							pReaction->m_bForceArousal = true;
							pReaction->m_fArousal = (float)atof( &pData[1] );
						}
						else
						{
							pReaction->m_fArousal = (float)atof( pData );
						}
					}
				}

				break;

			case 5:
				if( pReaction != NULL )
				{
					pReaction->m_fOverTime = (float)atof( xmlCellData->getContent() );
				}

				break;
		}
	}

	if( pReaction != NULL )
	{
		m_vecReactions.push_back( pReaction );
		bRet = true;
		AILogLoading(
			"Emotional Reaction from table [%s] loaded: [%s:%d] %0.3f%s %0.3f%s %0.3f%s %0.3f",
			this->GetName().c_str(),
			pReaction->m_sEvent.c_str(),
			pReaction->m_iValue,
			pReaction->m_fPleasure,
			(pReaction->m_bForcePleasure ? "*" : ""),
			pReaction->m_fClarity,
			(pReaction->m_bForceClarity ? "*" : ""),
			pReaction->m_fArousal,
			(pReaction->m_bForceArousal ? "*" : ""),
			pReaction->m_fOverTime );
	}
	else
	{
		AIError( "Error reading entries in Emotional Reaction table [%s]", this->GetName().c_str() );
	}

	return( bRet );
}

// Description:
//
// Arguments:
//
// Return:
//
void CEmotionalInfo::OnEditorSetGameMode( bool bGameMode )
{
	assert( m_bInit == true );

	Reload( m_sConfigFile );
}

// Description:
//
// Arguments:
//
// Return:
//
SEmotionalReaction const* CEmotionalInfo::FindReaction( const char* sReaction, int iValue ) const
{
	assert( m_bInit == true );
	assert( sReaction != NULL );

	SEmotionalReaction const*													pRet = NULL;

	std::vector<SEmotionalReaction *>::const_iterator itC = m_vecReactions.begin();
	std::vector<SEmotionalReaction *>::const_iterator itEnd = m_vecReactions.end();
	for( ; itC != itEnd; ++itC )
	{
		if( (*itC)->m_sEvent.compare(sReaction) == 0 && (iValue < 0 || iValue == (*itC)->m_iValue) )
		{
			pRet = *itC;
			break;
		}
	}

	return( pRet );
}
