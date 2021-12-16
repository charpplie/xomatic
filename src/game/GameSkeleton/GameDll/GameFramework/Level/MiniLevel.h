/*************************************************************************
  Crytek Source File.
  Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
	$Id$
	$DateTime$
	Description:	Implementation of the ILevel interface. 
					MiniLevel provides a basic implementation
  
 -------------------------------------------------------------------------
  History:
  - 25:2:2010	18:30 : Created by Christian Helmich

*************************************************************************/

#ifndef __MINI_LEVEL_H__
#define __MINI_LEVEL_H__

#include <ILevelSystem.h>
#include "../../GameCore/GameConfig.h"
#include "../../GameGlobals/GameCVars.h"
#include "MiniLevelInfo.h"

namespace simpleframework
{
	class CMiniLevel :
		public ILevel
	{
	public:
		friend class CMiniLevelSystem;

	private:
		//hide ctor/dtor implementation
		CMiniLevel(CMiniLevelInfo& levelInfo);
		VIRTUAL	~CMiniLevel();

	public:
		static simpleframework::CMiniLevel*	Create(CMiniLevelInfo& levelInfo);
		static void		Destroy(CMiniLevel* pLevel);
		static void		Recreate(CMiniLevel** ppLevel, CMiniLevelInfo& levelInfo);

		//ILevel implementation
		VIRTUAL void		Release();
		VIRTUAL ILevelInfo*	GetLevelInfo();
		//ILevel implementation/

	private:
		CMiniLevelInfo	m_levelInfo;
	};

} //namespace simpleframework

#endif	//__MINI_LEVEL_H__