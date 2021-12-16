////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2011.
// -------------------------------------------------------------------------
//  File name:   OCMExporter.h
//  Version:     v1.00
//  Created:     24 June 2011 by Sergiy Shaykin.
//  Compilers:   Visual Studio 2008
//  Description: Export geometry OCM file format
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __COCMExporter_h__
#define __COCMExporter_h__
#pragma once

#include <IExportManager.h>

class CFileEndianWriter;

struct SOCMeshInfo
{
	Matrix44	m_OBBMat;
	uint32		m_Offset;
	size_t		m_MeshHash;
	bool	operator==(const SOCMeshInfo& rOther)const{return m_MeshHash==rOther.m_MeshHash;}
};
typedef std::vector<SOCMeshInfo>	tdMeshOffset;

class COCMExporter : public IExporter
{
public:
	virtual const char* GetExtension() const;
	virtual const char* GetShortDescription() const;
	virtual bool ExportToFile(const char* filename, const Export::IData* pExportData);
	virtual bool ImportFromFile(const char* filename, Export::IData* pData){return false;};
	virtual void Release();

private:
	const char* TrimFloat(float fValue) const;
	void SaveInstance(CFileEndianWriter& rWriter,const Export::Object* pInstance,const SOCMeshInfo& rMeshInfo);
	size_t SaveMesh(CFileEndianWriter& rWriter,const Export::Object* pMesh,Matrix44& rOBBMat);

	void Extends(const Matrix44& rTransform,const Export::Object* pMesh,f32& rMinX,f32& rMaxX,f32& rMinY,f32& rMaxY,f32& rMinZ,f32& rMaxZ)	const;
	Matrix44 CalcOBB(const Export::Object* pMesh);
};


#endif // __COCMExporter_h__
