#ifndef __EXPORTFUNCTIONS_H__
#define __EXPORTFUNCTIONS_H__

#include "MayaCryExport.h"

struct SExportMetaData;

namespace ExportFunctions
{
	void Export( cryExportOptions &options );
	void GetMetaData(SExportMetaData& metaData);
}

#endif //__EXPORTFUNCTIONS_H__
