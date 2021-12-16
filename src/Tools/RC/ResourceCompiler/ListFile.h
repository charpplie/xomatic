////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek GmbH, 2010.
// -------------------------------------------------------------------------
//  File name:   ListFile.h
//  Version:     v1.00
//  Created:     02/02/2010 by Timur.
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __ListFile_h__
#define __ListFile_h__
#pragma once

class CListFile
{
public:
	CListFile( IResourceCompiler *pRC );
	bool Process( const string &inListFile,const string &wildcard,std::vector<string> &outFiles );

private:
	bool ParseListFile( const string &inListFile,std::vector<string> &lines );
	void ParseListFileInZip( const string &zipFilename,const string &listFilename,const string &wildcard,std::vector<string> &outFiles );

	IResourceCompiler *m_pRC;
};

#endif //__ListFile_h__