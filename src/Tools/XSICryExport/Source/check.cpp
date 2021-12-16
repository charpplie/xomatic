#include "stdafx.h"
#include "plugin_stub.h"
#include "Properties.h"

#include "Crosswalk/helper.h"

#include <commdlg.h>

bool CheckMaterialID()
{
	bool l_bResult = true;

	Application app;
	UIToolkit l_UIToolkit = app.GetUIToolkit();
	LONG l_Result;

	CRefArray l_MaterialLibraries = app.GetActiveProject().GetActiveScene().GetMaterialLibraries();
	for (LONG i = 0; i < l_MaterialLibraries.GetCount(); i++)
	{
		CValueArray l_MaterialIDs;
		CValueArray l_MaterialNames;
		l_MaterialIDs.Clear();
		l_MaterialNames.Clear();

		MaterialLibrary l_MatLib(l_MaterialLibraries[i]);

		if (l_MatLib.GetName() == L"DefaultLib")
		{
			CString l_Error = L"Change the name of the Material Library DefaultLib to the name of the Material .mtl file!";
			l_UIToolkit.MsgBox(l_Error, siMsgOkOnly, L"CryExport", l_Result);
			l_bResult = false;
		}

		CRefArray l_Materials = l_MatLib.GetItems();
		for (LONG j=0; j < l_Materials.GetCount(); j++)
		{
			Material l_Mat(l_Materials[j]);
			//			app.LogMessage(l_Mat.GetFullName());

			CRefArray &l_Shaders = l_Mat.GetShaders();
			for (LONG k = 0; k < l_Shaders.GetCount(); k++)
			{
				Shader l_Shader(l_Shaders[k]);
				CString l_ProgID = l_Shader.GetProgID();
				char* l_strProgID = (char*)l_ProgID.GetAsciiString();
				if (strstr(l_strProgID, "CryShader.1"))
				{
					/*
					CString l_MatName = l_Mat.GetName();
					const char* l_strMatName = l_MatName.GetAsciiString();
					bool l_bMatOk = false;
					if (strlen(l_strMatName) > 4)
					{
					char l_strID[3];
					l_strID[0] = l_strMatName[1];
					l_strID[1] = l_strMatName[2];
					l_strID[2] = 0;
					LONG l_ID = atoi(l_strID);
					if (l_strMatName[0] == '_' && l_strMatName[3] == '_' && l_ID > 0 && l_ID < 33)
					l_bMatOk = true;
					}
					if (!l_bMatOk)
					{
					CString l_Error = L"Fix the material name! ";
					l_Error += l_MatName;
					l_Error += L"\nFormat: <_ID_MaterialName>";
					l_UIToolkit.MsgBox(l_Error, siMsgOkOnly, L"CryExport", l_Result);
					l_bResult = false;
					}
					*/

					LONG l_MaterialID = l_Shader.GetParameterValue(L"MaterialID");
					l_MaterialIDs.Add(l_MaterialID);
					l_MaterialNames.Add(l_Mat.GetFullName());
				}
			}
		}

		LONG l_MaterialIDsCount = l_MaterialIDs.GetCount();
		for (LONG j = 0; j < l_MaterialIDsCount; j++)
		{
			LONG l_MatID = l_MaterialIDs[j];
			CString l_Error = L"MaterialID error: ";
			l_Error += l_MaterialNames[j];
			l_Error += L" (Material ID: ";
			l_Error += l_MaterialIDs[j];
			l_Error += L"):";
			bool l_bSameMatError = false;
			for (LONG k = 0; k < l_MaterialIDsCount; k++)
			{
				LONG l_MatID2 = l_MaterialIDs[k];
				if (j!=k && l_MatID==l_MatID2)
				{
					l_Error += L"\n";
					l_Error += l_MaterialNames[k];
					l_bSameMatError = true;
				}
			}
			if (l_bSameMatError)
			{
				l_UIToolkit.MsgBox(l_Error, siMsgOkOnly, L"CryExport", l_Result);
				l_bResult = false;
				break;
			}
		}
		bool l_bError = false;
		for (LONG j = 0; j < l_MaterialIDsCount; j++)
		{
			LONG l_MatID = l_MaterialIDs[j];
			if ( l_MatID >= l_MaterialIDsCount)
			{
				l_bError = true;
				break;
			}
		}
		if (l_bError)
		{
			CString l_Error = L"There is a hole in MaterialIDs list in ";
			l_Error += l_MatLib.GetFullName();
			l_UIToolkit.MsgBox(l_Error, siMsgOkOnly, L"CryExport", l_Result);
			l_bResult = false;
		}
	}
	return l_bResult;
}

bool CheckFilenames()
{
	bool l_bResult = true;

	CValueArray l_CryExportNodeList;
	CRefArray l_AllCryExportNodes = GetAllCryExportNodes();

	int loop;

	for (loop = 0; loop < l_AllCryExportNodes.GetCount(); loop++)
	{
		X3DObject l_Object(l_AllCryExportNodes.GetItem(loop));

		CustomProperty l_Property = l_Object.GetProperties().GetItem(L"ExportProperties");

		if (l_Property.IsValid())
		{
			CString l_Filename = l_Property.GetParameterValue(L"Filename");
			const char* l_strFilename = l_Filename.GetAsciiString();

			bool l_bFilename = true;
			if (strlen(l_strFilename) == 0)
				l_bFilename = false;
			for (DWORD i = 0; i < strlen(l_strFilename); i++)
			{
				char l_char = l_strFilename[i];
				if (!isalpha(l_char) && !isdigit(l_char) && l_char != '_' )
					l_bFilename = false;
			}

			if (!l_bFilename)
			{
				Application app;
				UIToolkit l_UIToolkit = app.GetUIToolkit();
				LONG l_Result;
				CString l_Error = L"Filename error! \"" + l_Filename + L"\" in \"" + l_Object.GetFullName() + L"\" node.\n Only alphanumeric characters and underscore are allowed!";
				l_UIToolkit.MsgBox(l_Error, siMsgOkOnly, L"CryExport", l_Result);

				l_bResult = false;
			}
		}
	}
	return l_bResult;
}

bool Traverse(X3DObject &in_Object)
{
	bool l_bResult = true;

	CRefArray l_Children = in_Object.GetChildren();
	for (LONG j = 0; j < l_Children.GetCount(); j++)
	{
		SIObject l_Obj(l_Children[j]);
		CString l_ObjName = l_Obj.GetFullName();
		X3DObject l_Child(l_Children[j]);

		if (l_Obj.GetType() == L"polymsh")
		{
			X3DObject l_X3DObject(l_Obj);
			CRefArray l_Materials = l_Child.GetMaterials();

			for (LONG l = 0; l < l_Materials.GetCount(); l++)
			{
				Material l_Material(l_Materials.GetItem(l));
				CRefArray &l_Shaders = l_Material.GetShaders();
				for (LONG k = 0; k < l_Shaders.GetCount(); k++)
				{
					Shader l_Shader(l_Shaders[k]);
					CString l_ProgID = l_Shader.GetProgID();
					char* l_strProgID = (char*)l_ProgID.GetAsciiString();
					if (!strstr(l_strProgID, "CryShader.1"))
					{
						Application app;
						UIToolkit l_UIToolkit = app.GetUIToolkit();
						LONG l_Result;
						CString l_Error = L"No CryShader on \"";
						l_Error += l_Child.GetFullName();
						l_Error += L"\" object! Material: ";
						l_Error += l_Material.GetName();
						l_UIToolkit.MsgBox(l_Error, siMsgOkOnly, L"CryExport", l_Result);

						l_bResult = false;
					}
				}
			}
		}
		if (!Traverse(l_Child))
			l_bResult = false;
	}
	return l_bResult;
}

bool CheckCryShaders()
{
	bool l_bResult = true;

	CRefArray l_CryExportNodes = GetAllCryExportNodes();
	for (LONG i = 0; i < l_CryExportNodes.GetCount(); i++)
	{
		X3DObject l_Object(l_CryExportNodes[i]);
		if (!Traverse(l_Object))
			l_bResult = false;
	}

	return l_bResult;
}

bool CheckCryExportNodes()
{
	bool l_bResult = true;

	CString l_Empty = L"";

	CRefArray l_CryExportNodes = GetAllCryExportNodes();
	if (!l_CryExportNodes.GetCount())
	{
		Application app;
		UIToolkit l_UIToolkit = app.GetUIToolkit();
		LONG l_Result;
		l_UIToolkit.MsgBox(L"No CryExportNode found!", siMsgOkOnly, L"CryExport", l_Result);
		l_bResult = false;
	}
	for (LONG i = 0; i < l_CryExportNodes.GetCount(); i++)
	{
		X3DObject l_Object(l_CryExportNodes[i]);
		CString l_Name = l_Object.GetName();

		CRefArray l_Children = l_Object.GetChildren();
		if (l_Children.GetCount() == 0)
		{
			l_Empty += L"\n";
			l_Empty += l_Name;
		}

		CustomProperty l_Property = l_Object.GetProperties().GetItem(L"ExportProperties");
		if (!l_Property.IsValid())
		{
			Application app;
			UIToolkit l_UIToolkit = app.GetUIToolkit();
			LONG l_Result;
			CString l_Error = L"Change the Properties to ExportProperties!: ";
			l_Error += l_Name;
			l_UIToolkit.MsgBox(l_Error, siMsgOkOnly, L"CryExport", l_Result);

			l_bResult = false;
		}


		char *l_strNodename = (char*)(l_Name.GetAsciiString());
		char *l_strCEN = strstr(l_strNodename, "CryExportNode_");
		if (!l_strCEN || l_strCEN != l_strNodename)
		{
			Application app;
			UIToolkit l_UIToolkit = app.GetUIToolkit();
			LONG l_Result;
			CString l_Error = L"Wrong CryExportNode name: ";
			l_Error += l_Name;
			l_Error += L". The name should be started with CryExportNode_.)";
			l_UIToolkit.MsgBox(l_Error, siMsgOkOnly, L"CryExport", l_Result);

			l_bResult = false;
		}

	}

	if (l_Empty.Length())
	{
		Application app;
		UIToolkit l_UIToolkit = app.GetUIToolkit();
		LONG l_Result;
		CString l_Error = L"Empty CryExportNode found!: ";
		l_Error += l_Empty;
		l_UIToolkit.MsgBox(l_Error, siMsgOkOnly, L"CryExport", l_Result);

		l_bResult = false;
	}

	return l_bResult;
}

void FixCryExportNodeName(X3DObject &in_Object)
{
	CustomProperty l_Property = in_Object.GetProperties().GetItem(L"ExportProperties");
	CString l_Filename;

	if (l_Property.IsValid())
		l_Filename = l_Property.GetParameterValue(L"Filename");

	char l_strFilename[MAX_PATH];
	strcpy(l_strFilename, l_Filename.GetAsciiString());
//	_strlwr(l_strFilename);
	l_Filename.PutAsciiString(l_strFilename);

	CRefArray l_CryExportNodes = GetAllCryExportNodes();
	for (LONG i = 0; i < l_CryExportNodes.GetCount(); i++)
	{
		X3DObject l_CryExportNode(l_CryExportNodes[i]);

		if (l_CryExportNode == in_Object)
			continue;

		CustomProperty l_OtherProperty = l_CryExportNode.GetProperties().GetItem(L"ExportProperties");
		CString l_OtherFilename;

		if (l_OtherProperty.IsValid())
			l_OtherFilename = l_OtherProperty.GetParameterValue(L"Filename");

		if (l_Filename == l_OtherFilename)
		{
			Application app;

			UIToolkit l_UIToolkit = app.GetUIToolkit();
			LONG l_Result;
			CString l_Error = CString(L"CryExportNode_") + l_Filename + L" already exist in scene!";
			l_UIToolkit.MsgBox(l_Error, siMsgOkOnly, L"CryExport", l_Result);

			CString l_ObjectName = in_Object.GetName();
			char l_strObjectName[MAX_PATH];
			const char *szSrcObjectName = l_ObjectName.GetAsciiString();

			if (strncmp(szSrcObjectName,"CryExportNode_",strlen("CryExportNode_")) == 0)
			{
				strcpy(l_strObjectName, szSrcObjectName+strlen("CryExportNode_"));
			}
			else
			{
				strcpy(l_strObjectName, szSrcObjectName);
			}
			l_ObjectName.PutAsciiString(l_strObjectName);
			l_Property.PutParameterValue(L"Filename", l_ObjectName, 0.0f);
		}
	}

	Parameter l_Param = in_Object.GetParameter(L"Name");

	CString l_NewObjectName = CString(L"CryExportNode_") + l_Filename;
	if (l_Param.IsLocked())
		l_Param.UnSetLock(siLockLevelAll, L"");
	in_Object.PutName(l_NewObjectName);
	if (!l_Param.IsLocked())
		l_Param.SetLock(siLockLevelAll);

}

bool TraverseRenderProperties(X3DObject &in_Object)
{
	bool l_bResult = true;
	CRefArray l_Children = in_Object.GetChildren();
	for (LONG j = 0; j < l_Children.GetCount(); j++)
	{
		SceneItem l_Obj(l_Children[j]);
		CString l_ObjName = l_Obj.GetFullName();

		Application app;

		CustomProperty l_RenderProperties(l_Obj.GetProperties().GetItem(L"RenderProperties"));
		if (l_RenderProperties.IsValid())
		{
			UIToolkit l_UIToolkit = app.GetUIToolkit();
			LONG l_Result;
			CString l_Error = L"Change the RenderProperty to ObjectProperty!: ";
			l_Error += l_ObjName;
			l_UIToolkit.MsgBox(l_Error, siMsgOkOnly, L"CryExport", l_Result);

			l_bResult = false;
		}

		X3DObject l_Child(l_Children[j]);
		if (!TraverseRenderProperties(l_Child))
			l_bResult = false;
	}
	return l_bResult;
}

void TraverseAndFixLODNames(X3DObject &in_Object)
{
	CRefArray l_Children = in_Object.GetChildren();
	for (LONG j = 0; j < l_Children.GetCount(); j++)
	{
		SceneItem l_Obj(l_Children[j]);
		CString l_ObjName = l_Obj.GetName();
		char l_strObjName[MAX_PATH];
		strcpy(l_strObjName, l_ObjName.GetAsciiString());

		if (l_strObjName && strlen(l_strObjName) > 6)
		{
			if (!strncmp(l_strObjName, "-LOD1-",6) || !strncmp(l_strObjName, "-LOD2-",6) || !strncmp(l_strObjName, "-LOD3-",6) || !strncmp(l_strObjName, "-LOD4-",6))
			{
				l_strObjName[0] = '_';
				l_strObjName[5] = '_';
			}
			l_ObjName.PutAsciiString(l_strObjName);
			l_Obj.PutName(l_ObjName);
		}

		X3DObject l_Child(l_Children[j]);
		TraverseAndFixLODNames(l_Child);
	}
}

bool CheckRenderProperties()
{
	Application app;
	Model l_SceneRoot = app.GetActiveSceneRoot();
	return TraverseRenderProperties(l_SceneRoot);
}

bool TraverseObjectProperties(X3DObject &in_Object)
{
	bool l_bResult = true;
	CRefArray l_Children = in_Object.GetChildren();
	for (LONG j = 0; j < l_Children.GetCount(); j++)
	{
		SceneItem l_Obj(l_Children[j]);
		CString l_ObjName = l_Obj.GetName();

		CustomProperty l_ObjectProperties(l_Obj.GetProperties().GetItem(L"ObjectProperties"));
		if (l_ObjectProperties.IsValid())
		{
			X3DObject l_CryExportNode = GetCryExportNode(l_Obj);
			CStringArray l_NodeNames;
			CStringArray l_WrongNodeNames;
			GetNodeNamesUnderNode(l_CryExportNode, l_NodeNames);

			CString l_Props = l_ObjectProperties.GetParameterValue(L"Props");

			if (l_Props.Length() != 0 && l_Props != L"-1")
			{
				CStringArray l_SplitList = l_Props.Split(L"\n");
				for (LONG i = 0; i < l_SplitList.GetCount(); i++)
				{
					CString l_Row = l_SplitList[i];
					const char *l_strRow = l_Row.GetAsciiString();
					if (!strncmp(l_strRow,"pieces=",7))
					{
						CString l_PiecesProperties;
						CString l_Pieces;
						LONG l_AddedPieces = 0;
						l_Pieces.PutAsciiString(&l_strRow[7]);
						CStringArray l_SplitPieces = l_Pieces.Split(L",");
						if (l_SplitPieces.GetCount() == 0)
						{
							l_Pieces += L",";
							l_SplitPieces = l_Pieces.Split(L",");
						}
						for (LONG j = 0; j < l_SplitPieces.GetCount(); j++)
						{
							CString l_PieceName = l_SplitPieces[j];
							char l_strPieceName[MAX_PATH];
							strcpy(l_strPieceName, l_PieceName.GetAsciiString());
							for (DWORD l = 0; l < strlen(l_strPieceName); l++)
							{
								if (l_strPieceName[l] == 0x0d || l_strPieceName[l] == 0x0a)
									l_strPieceName[l] = 0;
							}
							l_PieceName.PutAsciiString(l_strPieceName);

							bool l_bNodeNameFound = false;
							for (LONG k = 0; k < l_NodeNames.GetCount(); k++)
							{
								if (l_NodeNames[k] == l_PieceName && l_PieceName != l_ObjName)
								{
									l_AddedPieces++;
									l_bNodeNameFound = true;
								}
							}
							if (!l_bNodeNameFound)
								l_WrongNodeNames.Add(l_PieceName);
						}
						if (l_AddedPieces && l_AddedPieces == l_SplitPieces.GetCount())
						{
						}
						else
						{
							Application app;
							UIToolkit l_UIToolkit = app.GetUIToolkit();
							LONG l_Result;
							CString l_Error = L"Error with pieces!";
							for (LONG j = 0; j < l_WrongNodeNames.GetCount(); j++)
							{
								l_Error += L"\n" + l_WrongNodeNames[j];
							}
							l_UIToolkit.MsgBox(l_Error, siMsgOkOnly, L"CryExport", l_Result);

							l_bResult = false;
						}
					}
				}
			}
		}

		X3DObject l_Child(l_Children[j]);
		if (!TraverseObjectProperties(l_Child))
			l_bResult = false;
	}
	return l_bResult;
}

bool CheckPieces()
{
	Application app;
	Model l_SceneRoot = app.GetActiveSceneRoot();
	bool l_bResult = true;

	CRefArray l_CryExportNodes = GetAllCryExportNodes();
	for (LONG i = 0; i < l_CryExportNodes.GetCount(); i++)
	{
		X3DObject l_Object(l_CryExportNodes[i]);

		if (!TraverseObjectProperties(l_Object))
			l_bResult = false;
	}
	return l_bResult;
}

bool CheckProperties()
{
	CValue l_Value;
	Application app;
	app.GetPreferences().GetPreferenceValue(L"XSICryExport.Bin32Path", l_Value);
	CString l_Bin32Path = l_Value.GetAsText();
	CString l_Process = l_Bin32Path + L"\\rc\\rc.exe";

	CString l_ExportPath = GetExportPath();

	DWORD dwReturn = GetFileAttributes(l_ExportPath.GetWideString());
	if (dwReturn == INVALID_FILE_ATTRIBUTES)
	{
		UIToolkit l_UIToolkit = app.GetUIToolkit();
		LONG l_Result;
		l_UIToolkit.MsgBox(L"Export path not found!\nSet a correct folder in the Properties!", siMsgOkOnly, L"CryExport", l_Result);
		return false;
	}
	dwReturn = GetFileAttributes(l_Process.GetWideString());
	if (dwReturn == INVALID_FILE_ATTRIBUTES)
	{
		UIToolkit l_UIToolkit = app.GetUIToolkit();
		LONG l_Result;
		l_UIToolkit.MsgBox(L"Resource Compiler (RC) not found!\nSet the correct path of the Bin32 folder in the Properties!", siMsgOkOnly, L"CryExport", l_Result);
		return false;
	}
	return true;
}

bool CheckUVSet(X3DObject &in_Object)
{
	bool l_bResult = true;
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

			CRefArray l_UVs = l_ga.GetUVs();
			if (l_UVs.GetCount() > 1)
			{
				Application app;
				UIToolkit l_UIToolkit = app.GetUIToolkit();
				LONG l_Result;
				CString l_Error = L"More texture UV found on ";
				l_Error += l_ObjName;
				l_UIToolkit.MsgBox(l_Error, siMsgOkOnly, L"CryExport", l_Result);
				l_bResult = false;
			}
		}
		if (!CheckUVSet(l_Child))
			l_bResult = false;
	}
	return l_bResult;
}

bool CheckUVSets()
{
	bool l_bResult = true;
	CRefArray l_CryExportNodes = GetAllCryExportNodes();
	for (LONG i = 0; i < l_CryExportNodes.GetCount(); i++)
	{
		X3DObject l_Object(l_CryExportNodes[i]);
		if (!CheckUVSet(l_Object))
			l_bResult = false;
	}
	return l_bResult;
}

bool Diagnostics(bool in_Message)
{
	bool l_bResult = false;
	if (CheckProperties() && CheckCryExportNodes() && CheckFilenames() && CheckMaterialID() && CheckCryShaders() && CheckRenderProperties() && CheckPieces() && CheckUVSets())
	{
		if (in_Message)
		{
			Application app;
			UIToolkit l_UIToolkit = app.GetUIToolkit();
			LONG l_Result;
			l_UIToolkit.MsgBox(L"Diagnostics OK.", siMsgOkOnly, L"CryExport", l_Result);
		}
		l_bResult = true;
	}
	return l_bResult;
}

_XSI_EXTERN_ CStatus CryDiagnostics_Execute( CRef& in_ref )
{
	CStatus status = CStatus::OK;

	Diagnostics(true);

	return status;
}

_XSI_EXTERN_ CStatus FixLODNames_Execute( CRef& in_ref )
{
	CStatus status = CStatus::OK;

	Application app;
	Model l_SceneRoot = app.GetActiveSceneRoot();
	TraverseAndFixLODNames(l_SceneRoot);

	return status;
}
