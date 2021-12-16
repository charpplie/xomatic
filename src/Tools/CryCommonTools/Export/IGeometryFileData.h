#ifndef __IGEOMETRYFILE_H__
#define __IGEOMETRYFILE_H__

#include "ExportFileType.h"
#include <string>

class IGeometryFileData
{
public:
	struct SProperties
	{
		int  filetypeInt;
		bool bDoNotMerge;
		
		SProperties() 			
			: filetypeInt(CRY_FILE_TYPE_NONE)
			, bDoNotMerge(false) 
		{
		}
	};
public:
	virtual int AddGeometryFile(void* handle, const char* name, const SProperties &properties) = 0;
	virtual const SProperties& GetProperties(int geometryFileIndex) const = 0;
	virtual int GetGeometryFileCount() const = 0;
	virtual void* GetGeometryFileHandle(int geometryFileIndex) const = 0;
	virtual const char* GetGeometryFileName(int geometryFileIndex) const = 0;
};

#endif //__IGEOMETRYFILE_H__
