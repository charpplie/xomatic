/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: GenericOverwriteDialog.h,v 1.1 2008/12/05 17:56:00 PauloZaffari Exp wwwrun $
$DateTime$
Description:  These are helper classes for containing the data from the 
generic overwrite dialog.
-------------------------------------------------------------------------
History:
- 05:12:2008   17:56: Created by Paulo Zaffari
*************************************************************************/
#include "stdafx.h"
#include "UserOptions.h"

//////////////////////////////////////////////////////////////////////////
CUserOptions::CUserOptionsReferenceCountHelper::CUserOptionsReferenceCountHelper(CUserOptions& roUserOptions):
m_roReferencedUserOptionsObject(roUserOptions)
{
	m_roReferencedUserOptionsObject.IncRef();
}
//////////////////////////////////////////////////////////////////////////
CUserOptions::CUserOptionsReferenceCountHelper::~CUserOptionsReferenceCountHelper()
{
	m_roReferencedUserOptionsObject.DecRef();
}
//////////////////////////////////////////////////////////////////////////
CUserOptions::CUserOptions()
{
	m_boToAll=false;
	m_nCurrentOption=ENotSet;
}
//////////////////////////////////////////////////////////////////////////
bool	CUserOptions::IsOptionValid()
{
	return m_nCurrentOption!=ENotSet;
}
//////////////////////////////////////////////////////////////////////////
int	CUserOptions::GetOption()
{
	return m_nCurrentOption;
}
//////////////////////////////////////////////////////////////////////////
bool	CUserOptions::IsOptionToAll()
{
	return m_boToAll;
}
//////////////////////////////////////////////////////////////////////////
void	CUserOptions::SetOption(int	nNewOption,bool boToAll)
{
	m_nCurrentOption=nNewOption;
	m_boToAll=boToAll;
}
//////////////////////////////////////////////////////////////////////////
int CUserOptions::DecRef()
{
	if (m_nNumberOfReferences>=1)
	{
		--m_nNumberOfReferences;
		if (m_nNumberOfReferences==0)
		{
			SetOption(CUserOptions::ENotSet,false);
		}
	}
	return m_nNumberOfReferences;
}
//////////////////////////////////////////////////////////////////////////
int CUserOptions::IncRef()
{
	++m_nNumberOfReferences;
	return m_nNumberOfReferences;
}
//////////////////////////////////////////////////////////////////////////
