/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   SignalConditions.h
$Id$
$DateTime$
Description: Small class to store and manage the LUA code to execute
every time certain signal is received. In most cases, this code will
just change a value (or more than one value) in the puppet's LUA 
table (CPersonalActivationConditions).
---------------------------------------------------------------------
History:
- 04:07:2007 : Created by Ricardo Pillosu

*********************************************************************/

#ifndef _SIGNAL_CONDITIONS_H_
#define _SIGNAL_CONDITIONS_H_

/*$1- Basic value entry ------------------------------------------------------*/
struct SSignalReaction
{
	string sSignal;
	SSignalReaction const* pParent;
	std::vector<int> vReactions;

	SSignalReaction( const char* sSignal, SSignalReaction* pParent );
	void AddReaction( const string& sReaction );
};

/*$1- Main Class that gives all functionality --------------------------------*/
class CSignalConditions
{
	typedef std::map<unsigned int, string> TSignalParents;
	typedef std::map<unsigned int, SSignalReaction *> TSignalReactions;

public:

	/*$1-Basics --------------------------------------------------------------*/
	CSignalConditions( XmlNodeRef& xmlTable );
	virtual ~CSignalConditions();

	/*$1-Utils ---------------------------------------------------------------*/
	bool IsInit() const
	{
		return( m_bInit );
	}

	SSignalReaction* GetSignalReaction( const char* sSignal ) const;

private:
	bool Reload( XmlNodeRef& xmlTable );
	void AssignParents( const TSignalParents& vParents );
	void CheckParentsRecursivity();
	void Shutdown();

	/*$1- Members --------------------------------------------------------------*/
private:
	bool m_bInit;

	//using string hash number as key
	TSignalReactions m_mapReactions;
};

#endif // _SIGNAL_CONDITIONS_H_
