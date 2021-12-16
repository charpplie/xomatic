#ifndef __EXPORTNODEHELPERS_H__
#define __EXPORTNODEHELPERS_H__

#include "Export/IGeometryFileData.h"

namespace XSI {class SceneItem;}
class IExportContext;

namespace ExportNodeHelpers
{
	bool IsCryExportNode(const XSI::SceneItem& in_ProjectItem);
	bool IsMergeGeometry(const XSI::SceneItem& in_ProjectItem);
	std::string GetGeometryFileName(const XSI::SceneItem& object);
	int GetContentTypeInt(const XSI::SceneItem& object);
}

#endif //__EXPORTNODEHELPERS_H__
