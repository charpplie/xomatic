/*************************************************************************
 Crytek Source File.
 Copyright (C), Crytek Studios, 2001-2004.
 -------------------------------------------------------------------------
 $Id$
 $DateTime$
 Description:  common anti-cheat defence data
 -------------------------------------------------------------------------
 History:
 - 10/08/2004   : Created by Craig Tiller, based on CDefenceWall by Timur
*************************************************************************/

#ifndef __DEFENCEDATA_H__
#define __DEFENCEDATA_H__

#pragma once

#include "Config.h"

#if USE_DEFENCE

#include "IDataProbe.h"
#include <list>

class CDefenceData
{
public:
	CDefenceData();
		
	static void UnifyFilename( string& sFilename );
	static void FoldersToFiles( const std::map<string, bool>& mapFolders, std::vector<string>& vecFiles );
	static void WalkFolderAndAddToVector( const string& toppath, bool recurse, std::vector<string>& where );

	// list of all protected files
	typedef std::set<string> TProtectedFiles;
	TProtectedFiles m_vProtectedFiles;
	typedef std::list<string> TProtectedFileList;
	TProtectedFileList m_listProt;

	int version;
	int level;

	const string * AddProtectedFile( const char * filename );
	void ClearProtectedFiles();

	void Populate();
};
#endif //NOT_USE_DEFENCE

#endif
