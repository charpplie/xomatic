/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2009.
---------------------------------------------------------------------
File name:   GroupSystem.h
Description: Main interface to the whole Group System. The system
provides a way to create formation points (TPS queries) around a leader
and follow those points using a mixture of pathfinding and steering
behaviors.
---------------------------------------------------------------------
History:
- 06:02:2008 : Created by Ricardo Pillosu
- 2 Mar 2009 : Evgeny Adamenkov: Removed IRenderer

*********************************************************************/
#ifndef _GROUP_SYSTEM_H_
	#define _GROUP_SYSTEM_H_

	#include "EntityGroup.h"

class CGroupMember;

// -------------------------------------------------------------------
class CGroupSystem
{

	public:

		/*$1- Basics -------------------------------------------------------------*/
		CGroupSystem();
		virtual ~CGroupSystem();
		void		Init();
		void		Shutdown();
		void		Clear();
		void		Update( float fDeltaTime );
		void		UpdateDraw();

		/*$1- save/load ----------------------------------------------------------*/
		virtual void	Serialize( TSerialize ser );

		/*$1- Inherited functions ------------------------------------------------*/
		void	OnEditorSetGameMode( bool bGameMode );
		void	OnObjectRemoved( CAIObject *pObject );

		/*$1- Utils --------------------------------------------------------------*/
		CEntityGroup*		CreateGroup( EntityId idEntity );
		void						DestroyGroup( EntityGroupId idGroup );
		CEntityGroup*		GetGroup( EntityGroupId idGroup );
		CEntityGroup*		MergeGroups( EntityGroupId idGroupA, EntityGroupId idGroupB );
		bool						AddMemberToGroup( EntityGroupId idGroup, CGroupMember* pMember );

		CGroupMember*		CreateGroupMember( EntityId idEntity );
		CGroupMember*		GetGroupMember( EntityId idEntity );
		void						DestroyGroupMember( EntityId idEntity );
		void						DestroyGroupMember( CGroupMember* pMember );

		void						SetDebug( bool bDebug );

	protected:

	/*$1- Internal -------------------------------------------------------------*/
	private:

		/*$1- Members ------------------------------------------------------------*/
		std::vector<CGroupMember *>	m_vecMembers;
		std::vector<CEntityGroup *>	m_vecGroups;

		/*$1- Static debug data --------------------------------------------------*/
		bool m_bInit;
		bool m_bDebug;
};
#endif // _GROUP_SYSTEM_H_
