/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   FormationData.h
Description: 
---------------------------------------------------------------------
History:
- 02:04:2008 : Created by mieszko

*********************************************************************/
#ifndef _FORMATIONDATA_H_
	#define _FORMATIONDATA_H_

class CFormationData
{

	public:

		/*$1- Basics -------------------------------------------------------------*/
		CFormationData();
		~ CFormationData();

		/*$1- Util ---------------------------------------------------------------*/
		void	Clear();
		void	SetSize( uint32 nSize );
		void	SetPoint( uint32 nIndex, float x, float y );

		const std::vector< Vec3 >& GetPoints() const;
		
	private:

		/*$1- Members ------------------------------------------------------------*/
		uint32	m_nSize;
		std::vector< Vec3 > m_vecPoints;

/*$1- Static debug data ------------------------------------------------------*/
};

// Description:
//
// Arguments:
//
// Return:
//
inline void CFormationData::SetSize( uint32 nSize )
{
	assert( nSize > 1 );

	m_nSize = nSize - 1; // no point for leader
	m_vecPoints.resize( m_nSize );
}

// Description:
//
// Arguments:
//
// Return:
//
inline const std::vector< Vec3 >& CFormationData::GetPoints() const 
{ 
	return m_vecPoints; 
}

#endif // _FORMATIONDATA_H_
