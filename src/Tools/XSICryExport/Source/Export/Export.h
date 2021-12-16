#ifndef __EXPORT_H__
#define __EXPORT_H__

#include <xsi_ref.h>

struct SExportMetaData;

namespace Export
{
	void Export(const XSI::CRefArray& exportNodes);
	void ExportAnims();
	XSI::CRefArray FindAllExportNodes();

	//////////////////////////////////////////////////////////////////////////
	void GetMetaData(SExportMetaData& metaData);
}

#endif //__EXPORT_H__
