/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   FormationSet.h
Description: 
---------------------------------------------------------------------
History:
- 02:04:2008 : Created by mieszko

*********************************************************************/
#ifndef _FORMATIONSET_H_
	#define _FORMATIONSET_H_

	#include "FormationData.h"

class CFormationDataManager;

class CFormationSet
{
		friend class CFormationDataManager;
	public:

		/*$1- Basics -------------------------------------------------------------*/
		CFormationSet();
		CFormationSet( int nMaxSize, const char* pszName = NULL );
		~ CFormationSet();

		/*$1- Util ---------------------------------------------------------------*/
		void	SetName( const char* pszName );
		bool	CreateFormationSpec( uint32 nFormationSize, std::list<float> lstCoords );

		const CFormationData* GetFormationDataForSize( uint32 nSize ) const;
		string GetName() const;

	private:		

		/*$1- Members ------------------------------------------------------------*/
		string											m_sName;
		std::vector<CFormationData> m_vecFormations;

/*$1- Static debug data ------------------------------------------------------*/
};

// Description:
//   
// Arguments:
//
// Return:
//
inline const CFormationData* CFormationSet::GetFormationDataForSize( uint32 nSize ) const
{
	if( nSize > m_vecFormations.size() )
	{
		return NULL;
	}

	return &m_vecFormations[ nSize - 1 ];
}

// Description:
//   
// Arguments:
//
// Return:
//
inline string CFormationSet::GetName() const
{
	return m_sName;
}

#endif // _FORMATIONSET_H_
