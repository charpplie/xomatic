#include "StdAfx.h"
#include "ExportNodeHelpers.h"

#include <xsi_application.h>
#include <xsi_scene.h>
#include <xsi_project.h>
#include <xsi_sceneitem.h>
#include <xsi_customproperty.h>


bool ExportNodeHelpers::IsCryExportNode(const XSI::SceneItem &in_ProjectItem)
{
	bool l_bIsCryExportNode = false;

	if (in_ProjectItem.IsValid())
	{
		XSI::CString l_Name = in_ProjectItem.GetName();
		const char *asciiname = l_Name.GetAsciiString();

		if (asciiname && strstr(asciiname, "CryExportNode_"))
		{
			l_bIsCryExportNode = true;
		}
	}

	return l_bIsCryExportNode;
}

std::string ExportNodeHelpers::GetGeometryFileName(const XSI::SceneItem& object)
{
	XSI::CustomProperty properties(object.GetProperties().GetItem(L"ExportProperties"));
	XSI::CString filename = properties.GetParameterValue(L"Filename");
	return filename.GetAsciiString();
}

//////////////////////////////////////////////////////////////////////////
int ExportNodeHelpers::GetContentTypeInt(const XSI::SceneItem& object)
{
	int fileTypeInt = CRY_FILE_TYPE_NONE;
	if (object.IsValid())
	{
		XSI::CustomProperty property = object.GetProperties().GetItem(L"ExportProperties");

		if (property.IsValid())
			fileTypeInt = property.GetParameterValue(L"Filetype");
	}
	return fileTypeInt;
}

//////////////////////////////////////////////////////////////////////////
bool ExportNodeHelpers::IsMergeGeometry( const XSI::SceneItem& object )
{
	bool bMerge = true;	

	if (object.IsValid())
	{
		XSI::CustomProperty property = object.GetProperties().GetItem(L"ExportProperties");
		if (property.IsValid())
			bMerge = property.GetParameterValue(L"MergeObjects");
	}

	return bMerge;
}