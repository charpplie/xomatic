/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description:	Implementation of the ILevelInfo interface. 
					MiniLevelInfo provides a basic implementation
  
 -------------------------------------------------------------------------
  History:
  - 24:2:2010	15:30 : Created by Christian Helmich

*************************************************************************/

#ifndef	__MINI_LEVEL_INFO_H__
#define	__MINI_LEVEL_INFO_H__

#include <ILevelSystem.h>
#include "../../GameCore/GameConfig.h"
#include "../../GameGlobals/GameCVars.h"

namespace simpleframework
{
	class CMiniLevelInfo :
		public ILevelInfo
	{
	public:
		typedef std::vector<ILevelInfo::TGameTypeInfo> TGameTypeInfoVector;
		friend class CMiniLevelSystem;

	public:
		CMiniLevelInfo();
		VIRTUAL	~CMiniLevelInfo();

		//ILevelInfo implementation
		//- level data
		VIRTUAL	const char*		GetName() const;
		VIRTUAL	const char*		GetPath() const;
		VIRTUAL	const char*		GetPaks() const;
		VIRTUAL	const char*		GetDisplayName() const;
		VIRTUAL	const ILevelInfo::TStringVec& GetMusicLibs() const;
		VIRTUAL	int				GetHeightmapSize() const;

		//- game type
		VIRTUAL	int				GetGameTypeCount() const;
		VIRTUAL	bool			SupportsGameType(const char* gameTypeName) const;
		VIRTUAL	const ILevelInfo::TGameTypeInfo*	GetGameType(int gameType) const;
		VIRTUAL	const ILevelInfo::TGameTypeInfo*	GetDefaultGameType() const;
		//ILevelInfo implementation/

		//- memory info
		void		GetMemoryStatistics(ICrySizer * );

	protected:
		//- utility
		void        ReadMetaData();
		bool        ReadInfo();

	private:
		string				m_levelName;
		string				m_levelPath;
		string				m_levelPaks;
		string				m_levelDisplayName;
		TStringVec			m_musicLibs;
		TStringVec			m_gamerules;
		int					m_heightmapSize;
		TGameTypeInfoVector	m_gameTypes;		
	};
} //namespace simpleframework

#endif //__MINI_LEVEL_INFO_H__