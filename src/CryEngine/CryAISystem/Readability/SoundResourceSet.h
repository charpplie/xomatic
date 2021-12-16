/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   SoundResourceSet.h
Description: 
---------------------------------------------------------------------
History:
- 08:07:2008 : Created by mieszko

*********************************************************************/
#ifndef SOUNDRESOURCESET_H
#define SOUNDRESOURCESET_H

#include <vector>
#include <list>
#include <map>

namespace Readabilities
{

// ----------------------------------------------------------------
// SSoundResource declaration
// ----------------------------------------------------------------

struct SSoundResource
{
	uint32		nBlockedBy;	// Id of readability play session that blocked this resource
	bool			bUsed;			// if this sound resource has been already used
	
	string		sName;
	string		sFileName;
	int				nResourceId;

	SSoundResource( const char* name = NULL, const char* fileName = NULL );

	bool	IsBlocked() const;
	bool	IsBlockedBy( uint32 nBlocker ) const;
	bool	IsUsed() const;

	void	Reset();	// removes any sign of runtime usage
	void	Clear();	// zeros instance member variables
};

// ----------------------------------------------------------------
// CSoundResourceSet declaration
// ----------------------------------------------------------------

class CSoundResourceSet
{

	typedef std::map< string, int >	tMapToId;
	typedef std::vector< SSoundResource >	tResources;

public:

	CSoundResourceSet( const string& sSetName );
	
	bool	LoadFromXmlNode( XmlNodeRef& xmlNode );

	void	Reset();

	int		AddResource( const SSoundResource& resource );

	const SSoundResource* GetResource( int nResourceId ) const;
	const SSoundResource* GetResource( const string& sResourceName ) const;
	int		GetResourceId( const string& sResourceName ) const;

	void	SetBlockResource( uint32 nSessionId, int nResourceId, bool bBlock = true );
	void	SetBlockResource( uint32 nSessionId, const string& sResName, bool bBlock = true );
	void	SetBlockResource( uint32 nSessionId, const std::list<int>& lstResources , bool bBlock = true );
	void	MarkResourceUsed( int nResourceId );

	uint32			GetResourceCount() const;
	const char*	GetName() const;

private:

	// these methods are used only during initialization and loading data
	void	ResizeResourceContainer( uint32 nNewSize );
	void	FitResourceContainer();

	// ---------------------------------------------------
	// member variables
	// ---------------------------------------------------
	tResources						m_resources;
	tMapToId							m_mapNameToId;
	tResources::iterator	m_itFirstFree;
	string								m_sSetName;	
};

// ---------------------------------------------------
// SSoundResource inline function definitions
// ---------------------------------------------------

inline bool SSoundResource::IsBlocked() const 
{	
	return nBlockedBy > 0; 
}

inline bool SSoundResource::IsBlockedBy( uint32 nBlocker ) const 
{	
	return nBlockedBy == nBlocker; 
}

inline bool SSoundResource::IsUsed() const	
{	
	return bUsed; 
}

inline void SSoundResource::Clear() 
{
	nResourceId = -1;
	nBlockedBy = 0;
	bUsed = false;
	sName = "";
	sFileName = "";
}

inline void SSoundResource::Reset() 
{
	nBlockedBy = 0;
	bUsed = false;
}

// ---------------------------------------------------
// CSoundResourceSet inline function definitions
// ---------------------------------------------------

inline const SSoundResource* CSoundResourceSet::GetResource( int nResourceId ) const
{ 
	return &m_resources[nResourceId]; 
}

inline const SSoundResource* CSoundResourceSet::GetResource( const string& sResourceName  ) const
{ 
	int nId = GetResourceId( sResourceName );
	return nId >= 0 ? &m_resources[nId] : NULL; 
}

inline int CSoundResourceSet::GetResourceId( const string& sResourceName ) const 
{ 
	tMapToId::const_iterator itFound = m_mapNameToId.find( sResourceName );
	return ( itFound == m_mapNameToId.end() ? -1 : itFound->second ); 
}

inline uint32	CSoundResourceSet::GetResourceCount() const
{
	return (uint32)m_resources.size();
}

inline const char*	CSoundResourceSet::GetName() const
{
	return m_sSetName.c_str();
}

inline void	CSoundResourceSet::SetBlockResource( uint32 nSessionId, const string& sResName, bool bBlock )
{
	int nId = GetResourceId( sResName );
	SetBlockResource( nSessionId, nId, bBlock );
}

} // namespace Readabilities

#endif // SOUNDRESOURCESET_H