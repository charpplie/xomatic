/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   FormationDataManager.h
Description: 
---------------------------------------------------------------------
History:
- 02:04:2008 : Created by mieszko

*********************************************************************/
#ifndef _FORMATIONDATAMANAGER_H_
	#define _FORMATIONDATAMANAGER_H_

	#include "FormationSet.h"

class CFormationDataManager
{
		typedef std::map<string, CFormationSet> tFormationSetsMap;

	public:
		// Get the singleton
		static CFormationDataManager*		GetInstance();

		/*$1- Basics -------------------------------------------------------------*/
		CFormationDataManager();
		~ CFormationDataManager();

		/*$1- Util ---------------------------------------------------------------*/
		void										ClearFormationData();
		void										PushFormationSet( CFormationSet& formationSet );
		const CFormationData*		GetFormationData( string sFormationSetName, int nMembersCount ) const;

	private:
		static CFormationDataManager*		s_pInstance;	// The singleton

		/*$1- Members ------------------------------------------------------------*/
		bool bInitialized;
		tFormationSetsMap m_mFormationSets;

/*$1- Static debug data ------------------------------------------------------*/
};
#endif // _FORMATIONDATAMANAGER_H_
