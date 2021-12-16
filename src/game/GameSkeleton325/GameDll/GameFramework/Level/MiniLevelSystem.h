/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description:	Implementation of the ILevelSystem interface. 
					MiniLevelSystem provides a basic implementation
  
 -------------------------------------------------------------------------
  History:
  - 24:2:2010	11:55 : Created by Christian Helmich

*************************************************************************/

#ifndef __MINI_LEVEL_SYSTEM_H__
#define __MINI_LEVEL_SYSTEM_H__

#include <ILevelSystem.h>
#include "../../GameCore/GameConfig.h"
#include "../../GameGlobals/GameCVars.h"
#include "MiniLevel.h"
#include "MiniLevelInfo.h"
#include "MiniLevelRotation.h"

namespace simpleframework
{
	class CMiniLevelSystem :
		public ILevelSystem,
		public ISystem::ILoadingProgressListener
	{
	public:
		typedef	std::vector<CMiniLevelInfo>			TLevelInfoVector;
		typedef	std::vector<ILevelSystemListener*>	TLevelSystemListenerVector;		
		class CLevelNameAutoComplete;

	public:
		CMiniLevelSystem(const char* levelsFolder);
		VIRTUAL	~CMiniLevelSystem();

		//ILevelSystemListener (ILevelSystem::parent) implementation
		VIRTUAL	void	OnLevelNotFound(const char* levelName);
		VIRTUAL	void	OnLoadingStart(ILevelInfo* pLevel);
		VIRTUAL	void	OnLoadingComplete(ILevel* pLevel);
		VIRTUAL	void	OnLoadingError(ILevelInfo* pLevel, const char* error);
		VIRTUAL	void	OnLoadingProgress(ILevelInfo* pLevel, int progressAmount);
		//ILevelSystemListener (ILevelSystem::parent) implementation/

		//ILevelSystem implementation
		//- level information
		VIRTUAL	void		Rescan(const char* levelsFolder = 0);
		VIRTUAL	void		LoadRotation();
		VIRTUAL	int			GetLevelCount();
		VIRTUAL	ILevelInfo*	GetLevelInfo(int level);
		VIRTUAL	ILevelInfo*	GetLevelInfo(const char* levelName);

		//- listener
		VIRTUAL	void		AddListener(ILevelSystemListener* pListener);
		VIRTUAL	void		RemoveListener(ILevelSystemListener* pListener);

		//- level loading
		VIRTUAL	ILevel*		GetCurrentLevel() const;
		VIRTUAL	ILevel*		LoadLevel(const char* levelName);
		VIRTUAL void UnLoadLevel() {}
		VIRTUAL	ILevel*		SetEditorLoadedLevel(const char* levelName);
		VIRTUAL	bool		IsLevelLoaded();
		VIRTUAL	void		PrepareNextLevel(const char* levelName);
		VIRTUAL	float		GetLastLevelLoadTime();

		//- level rotation
		VIRTUAL	ILevelRotation*	GetLevelRotation();		
		//ILevelSystem implementation/

		//ISystem::ILoadingProgressListener implementation
		VIRTUAL	void OnLoadingProgress(int steps);
		//ISystem::ILoadingProgressListener implementation/

		//- public utility
		void		PrecacheLevelRenderData();
		void		GetMemoryStatistics(ICrySizer* s);
		void		SaveOpenedFilesList();

	protected:
		//- utility	
		void		SwitchToConsoleLoadingScreen();
		string&		UnifyName(string& name);	// lowercase string and replace backslashes with forward slashes
		void		ScanFolder(const char* subfolder);
		void		LogLoadingTime();
		bool		LoadLevelInfo(CMiniLevelInfo& levelInfo);
		bool		LoadLevelMusicLibs(CMiniLevelInfo* pLevelInfo);
		bool		LoadMissionXml(CMiniLevelInfo* pLevelInfo);
		bool		LoadMovieSystem(CMiniLevelInfo* pLevelInfo);

		//- internal level info getters
		CMiniLevelInfo*	GetLevelInfoInternal(int level);
		CMiniLevelInfo*	GetLevelInfoInternal(const char* levelName);

	private:
		TLevelSystemListenerVector	m_listeners;
		TLevelInfoVector	m_levelInfos;
		string				m_levelsFolder;

		CMiniLevel*			m_pCurrentLevel;	//can be CMiniLevelInfo as well
		CMiniLevelInfo*		m_pLoadingLevelInfo;
		CMiniLevelRotation	m_levelRotation;

		string				m_lastLevelName;
		float				m_fLastLevelLoadTime;
		float				m_fFilteredProgress;
		float				m_fLastTime;

		bool				m_bLevelLoaded;
		bool				m_bRecordingFileOpens;

		int					m_nLoadedLevelsCount;

		CTimeValue			m_tLevelLoadingStart;

		static CLevelNameAutoComplete	s_LevelNameAutoComplete;
	};

	class CMiniLevelSystem::CLevelNameAutoComplete :
		public IConsoleArgumentAutoComplete,
		public ILevelInfo::TStringVec
	{		
	public:
		CLevelNameAutoComplete();
		VIRTUAL ~CLevelNameAutoComplete();
		VIRTUAL int			GetCount() const;
		VIRTUAL const char*	GetValue(int nIndex) const;
	};	

} //namespace simpleframework

#endif //__MINI_LEVEL_SYSTEM_H__