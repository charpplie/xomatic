////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   TempFilePakExtraction.h
//  Version:     v1.00
//  Created:     14/5/2008 by Doug Binks (douglas).
//  Compilers:   Visual Studio.NET 2005
//  Description: Opens a temporary file for read only access, where the file could be
//				 located in a zip or pak file. Note that if the file specified
//				 already exists it does not delete it when finished.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __TEMPFILEPAKEXTRACTION_H__
#define __TEMPFILEPAKEXTRACTION_H__

class IPakSystem;

class TempFilePakExtraction
{
public:
	TempFilePakExtraction(const char* filename, IPakSystem* pPakSystem);
	~TempFilePakExtraction();

	const char* GetTempName() const
	{
		return m_strTempFileName.c_str();
	}
	const char* GetOriginalName() const
	{
		return m_strOriginalFileName.c_str();
	}

private:
	string m_strTempFileName;
	string m_strOriginalFileName;
};

#endif //__TEMPFILEPAKEXTRACTION_H__
