#include "stdafx.h"
#include "plugin_stub.h"
#include "helper.h"
#include "StringHelpers.h"
#include <commdlg.h>

void GetMaterialByName(CString& in_MaterialName, Material &out_Material)
{
	Application app;
	CRefArray l_MaterialLibraries = app.GetActiveProject().GetActiveScene().GetMaterialLibraries();
	for (LONG i = 0; i < l_MaterialLibraries.GetCount(); i++)
	{
		MaterialLibrary l_MatLib(l_MaterialLibraries[i]);
		CRefArray l_Materials = l_MatLib.GetItems();
		for (LONG j = 0; j < l_Materials.GetCount(); j++)
		{
			Material l_Mat(l_Materials[j]);
			if (l_Mat.GetFullName() == in_MaterialName)
			{
				out_Material = l_Mat;
				return;
			}
		}
	}
}

void GetMaterialLibraryByMaterial(Material &in_Material, MaterialLibrary &out_MaterialLibrary)
{
	Application app;
	CRefArray l_MaterialLibraries = app.GetActiveProject().GetActiveScene().GetMaterialLibraries();
	for (LONG i = 0; i < l_MaterialLibraries.GetCount(); i++)
	{
		MaterialLibrary l_MatLib(l_MaterialLibraries[i]);
		CRefArray l_Materials = l_MatLib.GetItems();
		for (LONG j=0; j < l_Materials.GetCount(); j++)
		{
			Material l_Mat(l_Materials[j]);
			if (l_Mat == in_Material)
				out_MaterialLibrary = l_MatLib;
		}
	}
}

bool IsFreeMaterialIDByMaterialLibrary(Material &in_Material, LONG in_MaterialID, MaterialLibrary &in_MaterialLibrary)
{
	CValueArray l_UsedMaterialIDs;
	for (LONG i = 0; i < 32; i++)
	{
		l_UsedMaterialIDs.Add(false);
	}

	Application app;
	CRefArray l_MaterialLibraries = app.GetActiveProject().GetActiveScene().GetMaterialLibraries();
	for (LONG i = 0; i < l_MaterialLibraries.GetCount(); i++)
	{
		MaterialLibrary l_MatLib(l_MaterialLibraries[i]);
		if (l_MatLib != in_MaterialLibrary )
			continue;
		CRefArray l_Materials = l_MatLib.GetItems();
		for (LONG j=0; j < l_Materials.GetCount(); j++)
		{
			Material l_Mat(l_Materials[j]);
			if (l_Mat == in_Material)
				continue;

			CRefArray &l_Shaders = l_Mat.GetShaders();
			for (LONG k = 0; k < l_Shaders.GetCount(); k++)
			{
				Shader l_Shader(l_Shaders[k]);

				CString l_ProgID = l_Shader.GetProgID();
				char* l_strProgID = (char*)l_ProgID.GetAsciiString();
				if (strstr(l_strProgID, "CryShader.1"))
				{
					LONG l_MaterialID = l_Shader.GetParameterValue(L"MaterialID");
					l_UsedMaterialIDs[l_MaterialID] = true;
				}
			}
		}
	}

	return l_UsedMaterialIDs[in_MaterialID];
}

LONG GetUnusedMaterialIDByMaterialLibrary(MaterialLibrary &in_MaterialLibrary)
{
	CValueArray l_UsedMaterialIDs;
	for (LONG i = 0; i < 32; i++)
	{
		l_UsedMaterialIDs.Add(false);
	}

	Application app;
	CRefArray l_MaterialLibraries = app.GetActiveProject().GetActiveScene().GetMaterialLibraries();
	for (LONG i = 0; i < l_MaterialLibraries.GetCount(); i++)
	{
		MaterialLibrary l_MatLib(l_MaterialLibraries[i]);
		if (l_MatLib != in_MaterialLibrary )
			continue;
		CRefArray l_Materials = l_MatLib.GetItems();
		for (LONG j=0; j < l_Materials.GetCount(); j++)
		{
			Material l_Mat(l_Materials[j]);
			CRefArray &l_Shaders = l_Mat.GetShaders();
			for (LONG k = 0; k < l_Shaders.GetCount(); k++)
			{
				Shader l_Shader(l_Shaders[k]);

				CString l_ProgID = l_Shader.GetProgID();
				char* l_strProgID = (char*)l_ProgID.GetAsciiString();
				if (strstr(l_strProgID, "CryShader.1"))
				{
					LONG l_MaterialID = l_Shader.GetParameterValue(L"MaterialID");
					l_UsedMaterialIDs[l_MaterialID] = true;
				}
			}
		}
	}

	for (LONG i = 0; i < 32; i++)
	{
		if (l_UsedMaterialIDs[i] == false)
			return i;
	}

	UIToolkit l_UIToolkit = app.GetUIToolkit();
	LONG l_Result;
	CString l_Error = L"Free MaterialID not found in ";
	l_Error += in_MaterialLibrary.GetFullName();
	l_Error += L"!";
	l_UIToolkit.MsgBox(l_Error, siMsgOkOnly, L"CryExport", l_Result);

	return -1;
}

bool IsCryExportNode(ProjectItem &in_ProjectItem)
{
	bool l_bIsCryExportNode = false;

	if (in_ProjectItem.IsValid())
	{
		// 		if (in_ProjectItem.GetType() == L"#model")
		// 		{
		// 			SceneItem l_SceneItem = (SceneItem)in_ProjectItem;
		// 		
		// 			CustomProperty l_Property = l_SceneItem.GetProperties().GetItem(L"ExportProperties");
		// 			if (l_Property.IsValid())
		// 			{
		// 				l_bIsCryExportNode = true;
		// 			}
		// 		}

		CString l_Name = in_ProjectItem.GetName();
		const char *asciiname = l_Name.GetAsciiString();

		if (asciiname && strstr(asciiname, "CryExportNode_"))
		{
			l_bIsCryExportNode = true;
		}
	}

	return l_bIsCryExportNode;
}

ProjectItem GetCryExportNode( ProjectItem &in_ProjectItem )
{
	Application app;

	ProjectItem l_ProjectItem = in_ProjectItem;
	ProjectItem l_CryExportNode;

	while( l_ProjectItem.IsValid() )
	{
		if (IsCryExportNode(l_ProjectItem))
		{
			l_CryExportNode = l_ProjectItem;
			break;
		}

		// if we reached the Scene_Root
		if (app.GetActiveSceneRoot().IsEqualTo(l_ProjectItem))
		{
			break;
		}

		l_ProjectItem = l_ProjectItem.GetParent3DObject();
	}

	return l_CryExportNode;
}

void GetAllCryExportNodesRecursively(X3DObject in_Object, CRefArray &out_CryExportNodeList)
{
	int loop;

	if (IsCryExportNode(in_Object))
	{
		out_CryExportNodeList.Add(in_Object);
	}

	CRefArray	l_Children = in_Object.GetChildren();

	for(loop = 0; loop < l_Children.GetCount(); loop++)
	{
		X3DObject l_ChildObject(l_Children.GetItem(loop));

		GetAllCryExportNodesRecursively(l_ChildObject, out_CryExportNodeList);
	}
}


CRefArray GetAllCryExportNodes()
{
	CRefArray l_CryExportNodeList;

	Application app;

	Model l_SceneRoot = app.GetActiveSceneRoot();

	GetAllCryExportNodesRecursively(l_SceneRoot, l_CryExportNodeList);

	return l_CryExportNodeList;
}

X3DObject GetCryExportNodeByNameRecursively(X3DObject in_Object, CString in_Name)
{
	int loop;
	X3DObject l_FoundCryExportNode;

	CString l_Name = in_Object.GetName();

	if (IsCryExportNode(in_Object) && (in_Object.GetName() == in_Name))
	{
		return in_Object;
	}

	CRefArray	l_Children = in_Object.GetChildren();

	for(loop = 0; loop < l_Children.GetCount(); loop++)
	{
		X3DObject l_ChildObject(l_Children.GetItem(loop));

		l_FoundCryExportNode = GetCryExportNodeByNameRecursively(l_ChildObject, in_Name);

		if (l_FoundCryExportNode.IsValid())
			break;
	}

	return l_FoundCryExportNode;
}


X3DObject GetCryExportNodeByName(CString in_Name)
{
	CRefArray l_CryExportNodeList;

	Application app;

	Model l_SceneRoot = app.GetActiveSceneRoot();

	return GetCryExportNodeByNameRecursively(l_SceneRoot, in_Name);
}

CString GetCryExportNodeListName(X3DObject in_Object, CString in_TypeString)
{
	CString l_ListName;

	CustomProperty l_Property = in_Object.GetProperties().GetItem(L"ExportProperties");

	if (l_Property.IsValid())
	{
		l_ListName = l_Property.GetParameterValue(L"Filename");

		l_ListName += L"  [";

		if (in_TypeString.Length() > 0)
		{
			l_ListName += in_TypeString;
		}
		else
		{
			int const intType = l_Property.GetParameterValue(L"Filetype");
			CString cstrType;
			cstrType.PutAsciiString( StringHelpers::MakeUpperCase( std::string(ExportFileTypeHelpers::CryFileTypeToString(intType)) ).c_str() );
			l_ListName += cstrType;
		}

		l_ListName += L"]";

		l_ListName += L" (";
		l_ListName += in_Object.GetName();
		l_ListName += L" )";
	}

	return l_ListName;
}

short GetCryExportNodeFiletype(X3DObject in_Object)
{
	short l_Filetype = CRY_FILE_TYPE_NONE;

	if (in_Object.IsValid())
	{
		CustomProperty l_Property = in_Object.GetProperties().GetItem(L"ExportProperties");

		if (l_Property.IsValid())
		{
			l_Filetype = l_Property.GetParameterValue(L"Filetype");
		}
	}

	return l_Filetype;
}

bool TraverseAndDeleteMAXNormals(X3DObject &in_Object)
{
	bool l_bResult = true;

	Application app;
	if (in_Object.GetType() == L"polymsh")
	{
		CString l_MAXNormal = in_Object.GetFullName() + L".polymsh.cls.NodeProperties.MAXNormal";
		app.LogMessage(l_MAXNormal);
		XSI::COMMANDS::DeleteObj(l_MAXNormal);
	}

	CRefArray l_Children = in_Object.GetChildren();
	app.LogMessage(CValue(l_Children.GetCount()));
	for (LONG j = 0; j < l_Children.GetCount(); j++)
	{
		SIObject l_Obj(l_Children[j]);
		CString l_ObjName = l_Obj.GetFullName();
		X3DObject l_Child(l_Children[j]);
		app.LogMessage(l_ObjName);

		if (!TraverseAndDeleteMAXNormals(l_Child))
			l_bResult = false;
	}
	return l_bResult;
}

_XSI_EXTERN_ CStatus DeleteMAXNormals_Execute( CRef& in_ref )
{
	CStatus status = CStatus::OK;

// 	CRefArray l_CryExportNodes = GetAllCryExportNodes();
// 	for (LONG i = 0; i < l_CryExportNodes.GetCount(); i++)
// 	{
// 		X3DObject l_Object(l_CryExportNodes[i]);
// 		TraverseAndDeleteMAXNormals(l_Object);
// 	}

	Application app;
	Selection l_Selection( app.GetSelection() );

	LONG l_lCount = l_Selection.GetCount();
	if (!l_lCount)
	{
		Application app;
		UIToolkit l_UIToolkit = app.GetUIToolkit();
		LONG l_Result;
		l_UIToolkit.MsgBox(L"No object(s) selected!", siMsgOkOnly, L"CryExport", l_Result);
		return CStatus::Fail;
	}

	for (LONG i = 0; i < l_lCount; i++)
	{
		X3DObject l_Obj(l_Selection[i]);
		TraverseAndDeleteMAXNormals(l_Obj);
	}

	return status;
}

CString g_HidedPolygons;

void UnhideClusters(X3DObject &in_Object)
{
	CRefArray l_Children = in_Object.GetChildren();
	for (LONG j = 0; j < l_Children.GetCount(); j++)
	{
		SIObject l_Obj(l_Children[j]);
		CString l_ObjName = l_Obj.GetFullName();
		X3DObject l_Child(l_Children[j]);

		if (l_Obj.GetType() == L"polymsh")
		{
			X3DObject l_X3DObject(l_Obj);
			PolygonMesh l_pm = l_X3DObject.GetActivePrimitive().GetGeometry();
			CGeometryAccessor l_ga = l_pm.GetGeometryAccessor();

			CRefArray l_XSIClusters = l_ga.GetClusters(siClusterPolygonType);
			Cluster l_XSICluster = (Cluster)l_XSIClusters.GetItem(L"InvisiblePolygons");

			CBitArray l_XSIClusterElemDesc;
			l_XSICluster.GetGeometryElementFlags(l_XSIClusterElemDesc);

			if (l_XSIClusterElemDesc.GetTrueCount())
			{
				g_HidedPolygons += l_ObjName + L".poly[";

				LONG idx = 0;
				LONG pcount = 0;
				while( l_XSIClusterElemDesc.GetNextTrueBit( idx ) )
				{
					if (pcount)
						g_HidedPolygons += L",";
					g_HidedPolygons += CValue(idx);
					pcount++;
				}

				g_HidedPolygons += L"],";

				CString l_InvisiblePolygons = l_ObjName + L".polymsh.cls.InvisiblePolygons";
				XSI::COMMANDS::ToggleVisibility(l_InvisiblePolygons,L"",FALSE);
			}
		}
		UnhideClusters(l_Child);
	}
}

void UnhideAllClusters()
{
	g_HidedPolygons = L"";
	CRefArray l_CryExportNodes = GetAllCryExportNodes();
	for (LONG i = 0; i < l_CryExportNodes.GetCount(); i++)
	{
		X3DObject l_Object(l_CryExportNodes[i]);
		UnhideClusters(l_Object);
	}
}

void HideClusters()
{
	Application app;
	if (g_HidedPolygons.Length())
		XSI::COMMANDS::ToggleVisibility(g_HidedPolygons,L"",FALSE);
}

// Sokov: commented out because it's unused. Should we delete it?
















































































bool TraverseAndAddRealBoneNames(X3DObject &in_Object, siBranchFlag in_BranchFlag)
{
	bool l_bResult = true;
	Application app;

	CustomProperty l_Property = in_Object.GetProperties().GetItem(L"RealBoneName");
	if (!l_Property.IsValid())
	{
		CustomProperty prop = in_Object.AddProperty(CString(L"CustomProperty"), false, L"RealBoneName");
		char l_strName[256];
		strcpy(l_strName, in_Object.GetName().GetAsciiString());
		for (DWORD j = 0; j < strlen(l_strName); j++)
		{
			if (l_strName[j] == '_')
				l_strName[j] = ' ';
		}
		CString l_Name;
		l_Name.PutAsciiString(l_strName);
		Parameter l_Parameter;
		prop.AddParameter(L"BoneName",CValue::siString, siPersistable, L"BoneName", L"", l_Name, l_Parameter ) ;

		if (in_BranchFlag == siBranch)
		{
			CRefArray l_Children = in_Object.GetChildren();
			for (LONG j = 0; j < l_Children.GetCount(); j++)
			{
				X3DObject l_Child(l_Children[j]);

				if (!TraverseAndAddRealBoneNames(l_Child, siBranch))
					l_bResult = false;
			}
		}
	}
	return l_bResult;
}

_XSI_EXTERN_ CStatus AddRealBoneNames_Execute( CRef& in_ref )
{
	CStatus status = CStatus::OK;
	Application app;
	Selection l_Selection( app.GetSelection() );

	LONG l_lCount = l_Selection.GetCount();
	if (!l_lCount)
	{
		Application app;
		UIToolkit l_UIToolkit = app.GetUIToolkit();
		LONG l_Result;
		l_UIToolkit.MsgBox(L"No object(s) selected!", siMsgOkOnly, L"CryExport", l_Result);
		return CStatus::Fail;
	}

	for (LONG i = 0; i < l_lCount; i++)
	{
		X3DObject l_Obj(l_Selection[i]);
		siBranchFlag l_BranchFlag = l_Obj.GetBranchFlag();
		TraverseAndAddRealBoneNames(l_Obj, l_BranchFlag);
	}
	return status;
}
 