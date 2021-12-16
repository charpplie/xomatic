#include "StdAfx.h"
#include "GeometryFileData.h"

int GeometryFileData::AddGeometryFile(void* handle, const char* name, const SProperties &properties)
{
	const int geometryFileIndex = int(m_geometryFiles.size());
	m_geometryFiles.push_back(GeometryFileEntry(handle, name, properties));
	return geometryFileIndex;
}

int GeometryFileData::GetGeometryFileCount() const
{
	return int(m_geometryFiles.size());
}

void* GeometryFileData::GetGeometryFileHandle(int geometryFileIndex) const
{
	return m_geometryFiles[geometryFileIndex].handle;
}

const char* GeometryFileData::GetGeometryFileName(int geometryFileIndex) const
{
	return m_geometryFiles[geometryFileIndex].name.c_str();
}

//////////////////////////////////////////////////////////////////////////
const IGeometryFileData::SProperties& GeometryFileData::GetProperties( int geometryFileIndex ) const
{
	assert(geometryFileIndex >= 0 && geometryFileIndex < (int)m_geometryFiles.size());
	return m_geometryFiles[geometryFileIndex].properties;
}
