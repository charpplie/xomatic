////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2011.
// -------------------------------------------------------------------------
//  File name:   OBJExporter.h
//  Version:     v1.00
//  Created:     24 June 2011 by Sergiy Shaykin.
//  Compilers:   Visual Studio 2008
//  Description: Export geometry OBJ file format
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __COBJExporter_h__
#define __COBJExporter_h__
#pragma once


#include <IExportManager.h>


class COBJExporter : public IExporter
{
public:
	virtual const char* GetExtension() const;
	virtual const char* GetShortDescription() const;
	virtual bool ExportToFile(const char* filename, const Export::IData* pExportData);
	virtual bool ImportFromFile(const char* filename, Export::IData* pData){return false;};
	virtual void Release();

private:
	const char* TrimFloat(float fValue) const;
	CString MakeRelativePath(const char* pMainFileName, const char* pFileName) const;
};


#endif // __COBJExporter_h__
