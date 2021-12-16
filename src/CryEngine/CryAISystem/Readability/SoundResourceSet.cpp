/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   SoundResourceMgr.cpp
Description: 
---------------------------------------------------------------------
History:
- 08:07:2008 : Created by mieszko

*********************************************************************/
#include "StdAfx.h"

#include "SoundResourceSet.h"

using namespace Readabilities;

// ********************************************************************* //
//	SSoundResource implementation
// ********************************************************************* //

SSoundResource::SSoundResource( const char* name, const char* fileName ) 
: nResourceId( -1 ), nBlockedBy( 0 ), bUsed( false ), sName( name ), sFileName( fileName )
{	
}

// ********************************************************************* //
//	CSoundResourceSet implementation
// ********************************************************************* //

CSoundResourceSet::CSoundResourceSet( const string& sSetName )
: m_sSetName( sSetName )
{
	ResizeResourceContainer( 0 );	
}

bool CSoundResourceSet::LoadFromXmlNode( XmlNodeRef& xmlNode )
{
	static const int s_nCellsToRead = 2;

	bool  bFirstSkipped = false;
	int nRowsCount = xmlNode->getChildCount();

	// make resource vector big enough to contain all resources
	ResizeResourceContainer( nRowsCount - 1 );	// we know the first row is skipped
	
	for( int iRow = 0; iRow < nRowsCount; ++iRow )
	{
		XmlNodeRef  xmlRow = xmlNode->getChild( iRow );

		if( xmlRow->isTag("Row") == false )
		{
			continue;
		}

		if( bFirstSkipped == false )
		{
			bFirstSkipped = true;
			continue;
		}

		string sData[s_nCellsToRead];
		string* pData = sData;
		// for every row
		for( int nCell = 0, nRead = 0; nCell < xmlRow->getChildCount() && nRead < s_nCellsToRead; ++nCell )
		{
			XmlNodeRef  xmlCell = xmlRow->getChild( nCell );
			XmlNodeRef  xmlCellData = xmlCell->findChild( "Data" );

			if( xmlCell->isTag("Cell") == false || !xmlCellData )
			{
				continue;
			}

			*pData = xmlCellData->getContent();
			pData->Trim();

			++pData;
			++nRead;
		}

		if( sData[0].length() > 0 && sData[1].length() > 0)
		{
			AddResource( SSoundResource( sData[0], sData[1] ) );
		}
	}

	// shrink the container down to size
	FitResourceContainer();

	// if we got after first 'regular' row it's considered load succeeded
	return bFirstSkipped;
}

// ------------------------------------------------------------
// Description:
//	Clears all runtime usage marks on all owned sound resources
// ------------------------------------------------------------
void CSoundResourceSet::Reset()
{
	std::vector<SSoundResource>::iterator itRes = m_resources.begin();
	std::vector<SSoundResource>::iterator itEnd = m_resources.end();
	for( ; itRes != itEnd; ++itRes )
	{
		itRes->Reset();
	}
}

int CSoundResourceSet::AddResource( const SSoundResource& resource )
{
	assert( m_itFirstFree != m_resources.end() && "Adding more resources to the resource container than initialy declared!" );

	// check if resource with this name is already registered
	int resId = GetResourceId( resource.sName );
	
	// if this resource haven't been registered yet
	if( resId < 0 )
	{
		// register it
		SSoundResource& registeredResource = *m_itFirstFree;
		registeredResource = resource;
		resId = registeredResource.nResourceId = (int)(m_itFirstFree - m_resources.begin());

		++m_itFirstFree;
		
		m_mapNameToId.insert( std::make_pair( registeredResource.sName, registeredResource.nResourceId ) );
	}
	
	return resId;
}

// --------------------------------------------------------------------------------------------
// Description: Depending on bBlock parameter this method:
//		true	- blocks given resource and saves information on who blocked it, so there will be no
//						unblocking of others' resources
//		false	- unblocks given resource provided that nSessionId parameter contains id of 
//						readability session that blocked given resource in the first place
// Arguments:
//		nSessionId	-	CReadabilitySession instance id that requested blocking of this resource
//		nResourceId	-	resource to be blocked/unblocked
//		bBlock			-	see description
// --------------------------------------------------------------------------------------------
void CSoundResourceSet::SetBlockResource( uint32 nSessionId, int nResourceId, bool bBlock )
{
	assert( nResourceId < (int)m_resources.size() && "Invalid resource ID!" );

	SSoundResource& resource = m_resources[nResourceId];

	if( bBlock == true )
	{
		assert( resource.nBlockedBy == 0 && "Resource already blocked!" );

		resource.nBlockedBy = nSessionId;
	}
	else
	{
		assert( resource.nBlockedBy == nSessionId && "Trying to unblock resource blocked by other readability session!" );

		resource.nBlockedBy = 0;
	}
}

// --------------------------------------------------------------------------------------------
// Description: Depending on bBlock parameter this method:
//		true	- blocks resources given in a list of ids and saves information on who blocked it, 
//						so there will be no unblocking of others' resources
//		false	- unblocks resources given in a list of ids provided that nSessionId parameter 
//						contains id of readability session that blocked given resource in the first place
// Arguments:
//		nSessionId	-	CReadabilitySession instance id that requested blocking of this resource
//		lstResources-	list of id of resources to be blocked/unblocked
//		bBlock			-	see description
// --------------------------------------------------------------------------------------------
void CSoundResourceSet::SetBlockResource( uint32 nSessionId, const std::list<int>& lstResources , bool bBlock )
{	
	int nResourceId;

	if( bBlock == true )
	{	
		std::list<int>::const_iterator itC = lstResources.begin();
		std::list<int>::const_iterator itEnd = lstResources.end();
		for( ; itC != itEnd; ++itC )
		{
			nResourceId = *itC;			
			assert( nResourceId < (int)m_resources.size() && "Invalid resource ID!" );
			SSoundResource& resource = m_resources[nResourceId];
			assert( resource.nBlockedBy == 0 && "Resource already blocked!" );
			resource.nBlockedBy = nSessionId;			
		}
	}
	else
	{
		std::list<int>::const_iterator itC = lstResources.begin();
		std::list<int>::const_iterator itEnd = lstResources.end();
		for( ; itC != itEnd; ++itC )
		{
			nResourceId = *itC;
			assert( nResourceId < (int)m_resources.size() && "Invalid resource ID!" );
			SSoundResource& resource = m_resources[nResourceId];
			assert( resource.nBlockedBy == nSessionId && "Trying to unblock resource blocked by other readability session!" );
			resource.nBlockedBy = 0;
		}
	}	
}

// --------------------------------------------------------------------------------------------
// Description: Marks given resource as already used preventing it from being used again until
//							the readability system is reseted
// Arguments:
//		nResourceId	-	resource to be marked as used
// --------------------------------------------------------------------------------------------
void CSoundResourceSet::MarkResourceUsed( int nResourceId )
{
	assert( nResourceId < (int)m_resources.size() && "Invalid resource ID!" );		
	SSoundResource& resource = m_resources[nResourceId];
	resource.bUsed = true;
}

// ------------------------------------------------------------
// Description:
//		Resizes 
//		NOTE: results in all data inside m_resources container to be 
//					no longer save from overwriting
// Arguments:
//		nNewSize
// ------------------------------------------------------------
void CSoundResourceSet::ResizeResourceContainer( uint32 nNewSize )
{
	m_resources.resize( nNewSize );
	m_itFirstFree = m_resources.begin();
}

// ------------------------------------------------------------
// Description:
//		Shrinks container down to fit number of real elements inside   
// ------------------------------------------------------------
void CSoundResourceSet::FitResourceContainer()
{
	uint32 nNumberOfElements = (uint32)(m_itFirstFree - m_resources.begin());
	m_resources.resize( nNumberOfElements );	
}
