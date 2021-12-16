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

#ifndef UserOptions_h__
#define UserOptions_h__

#pragma once

// Small helper class.
// Hint: have one for files and other for directories.
// Hint: used a CUserOptionsReferenceCountHelper to automatically control the reference counts
// of any CUserOptions variable: usefull for recursion when you don't want to use
// only static variables. See example in FileUtill.cpp, function CopyTree.
class CUserOptions
{
	//////////////////////////////////////////////////////////////////////////
	// Types & typedefs
public:
	enum EOption
	{
		ENotSet,
		EYes=IDYES,
		ENo=IDNO,
		ECancel=IDCANCEL,
	};

	class CUserOptionsReferenceCountHelper
	{
		public:
			CUserOptionsReferenceCountHelper(CUserOptions& roUserOptions);
			virtual ~CUserOptionsReferenceCountHelper();
		protected:
			CUserOptions&	m_roReferencedUserOptionsObject;
	};
protected:
private:
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Methods
public:
	CUserOptions();

	bool		IsOptionValid();

	int			GetOption();

	bool		IsOptionToAll();

	void		SetOption(int	nNewOption,bool boToAll);

	int DecRef();
	int IncRef();
protected:
private:
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Data
public:
protected:
	int							m_nCurrentOption;
	bool						m_boToAll;
	int							m_nNumberOfReferences;
private:
	//////////////////////////////////////////////////////////////////////////
};
#endif // UserOptions_h__
