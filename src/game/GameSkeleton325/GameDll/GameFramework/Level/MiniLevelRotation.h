/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description:	Implementation of the ILevelRotation interface. 
					MiniLevelRotation provides a basic implementation
  
 -------------------------------------------------------------------------
  History:
  - 24:2:2010	15:30 : Created by Christian Helmich

*************************************************************************/

#include <ILevelSystem.h>
#include "../../GameCore/GameConfig.h"
#include "../../GameGlobals/GameCVars.h"

#ifndef	__MINI_LEVEL_ROTATION_H__
#define	__MINI_LEVEL_ROTATION_H__

namespace simpleframework
{
	class CMiniLevelRotation : 
		public ILevelRotation
	{
	public:
		struct SLevelRotationEntry;
		typedef std::vector<SLevelRotationEntry> TLevelRotationVector;

	public:
		CMiniLevelRotation();
		VIRTUAL ~CMiniLevelRotation();

		//ILevelRotation implementation
		//- load/save
		VIRTUAL bool		Load(ILevelRotationFile* file);
		VIRTUAL bool		Save(ILevelRotationFile* file);

		//- rotation modification
		VIRTUAL void		Reset();
		VIRTUAL int			AddLevel(const char* level, const char* gamerules);
		VIRTUAL void		AddSetting(int level, const char* setting);

		//- iteration through levels
		VIRTUAL bool		First();
		VIRTUAL bool		Advance();

		//- next data
		VIRTUAL const char*	GetNextLevel() const;
		VIRTUAL const char* GetNextGameRules() const;
		VIRTUAL int			GetNextSettingsNum() const;
		VIRTUAL const char*	GetNextSetting(int idx);

		VIRTUAL int			GetLength() const;
		VIRTUAL int			GetNext() const;

		//- randomizer
		VIRTUAL void		SetRandom(bool rnd);
		VIRTUAL bool		IsRandom()const;

		//- change level
		VIRTUAL void		ChangeLevel(IConsoleCmdArgs* pArgs = NULL);
		//ILevelRotation implementation/

	protected:
		//- utility
		void    Shuffle();

	private:
		TLevelRotationVector m_rotation;
		bool                 m_random;
		int                  m_next;
		std::vector<int>     m_shuffle;
	};


	struct CMiniLevelRotation::SLevelRotationEntry
	{
		string levelName;
		string gameRulesName;
		std::vector<string> settings;
	};
} //namespace simpleframework

#endif	//__MINI_LEVEL_ROTATION_H__
