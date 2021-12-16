/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2008.
---------------------------------------------------------------------
File name:   EmotionalInfo.h
Description: Stores in memory all data from each XML file. This data,
basically, connects events to how they affect emotional state
---------------------------------------------------------------------
History:
- 08:05:2008 : Created by ricardo

*********************************************************************/
#ifndef _EMOTIONALINFO_H_
	#define _EMOTIONALINFO_H_

	#include "IFileChangeMonitor.h"

/*$1- Every entry in the XML file goes here ----------------------------------*/
struct SEmotionalReaction
{
	string	m_sEvent;
	int			m_iValue;
	bool		m_bForcePleasure : 1;
	bool		m_bForceClarity : 1;
	bool		m_bForceArousal : 1;
	float		m_fPleasure;
	float		m_fClarity;
	float		m_fArousal;
	float		m_fOverTime;

	SEmotionalReaction (const char*sEvent)
	:
	m_sEvent( sEvent ),
	m_fPleasure( 0.0f ),
	m_iValue( -1 ),
	m_fClarity( 0.0f ),
	m_fArousal( 0.0f ),
	m_fOverTime( 0.0f ),
	m_bForcePleasure( false ),
	m_bForceClarity( false ),
	m_bForceArousal( false )
	{
		assert( sEvent != NULL );
	}
};

class CEmotionalInfo
{

	public:

		/*$1- Basics -------------------------------------------------------------*/
		CEmotionalInfo();
		~			CEmotionalInfo();
		bool	Init( const string& sFile );
		void	Shutdown();
		bool	Reload( const string& sFile );

		/*$1- Inherited functions ------------------------------------------------*/
		void	OnEditorSetGameMode( bool bGameMode );

		/*$1- Utils --------------------------------------------------------------*/
		string											GetName() const;
		SEmotionalReaction const*		FindReaction( const char* sReaction, int iValue = -1 ) const;

	protected:

		/*$1- Internal -----------------------------------------------------------*/
		bool	LoadSheet( XmlNodeRef xmlNode );
		bool	LoadRow( XmlNodeRef xmlRow );

	private:

		/*$1- Members ------------------------------------------------------------*/
		bool															m_bInit;
		bool															m_bRegistered;
		string														m_sConfigFile;
		std::vector<SEmotionalReaction *> m_vecReactions;
};

/*$1- GetName : filename without the leading ".xml" --------------------------*/
inline string CEmotionalInfo::GetName() const
{
	return( m_sConfigFile.substr(0, (m_sConfigFile.length() - 4)) );
}

#endif // _EMOTIONALINFO_H_
