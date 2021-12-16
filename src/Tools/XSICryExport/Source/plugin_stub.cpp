//***************************************************************************************
//
// File supervisor: Softimage Rendering & Pipeline team
//
// (c) Copyright 2001-2005 Avid Technology, Inc. . All rights reserved.
//
//***************************************************************************************

/****************************************************************************************
THIS CODE IS PUBLISHED AS A SAMPLE ONLY AND IS PROVIDED "AS IS".
IN NO EVENT SHALL SOFTIMAGE, AVID TECHNOLOGY, INC. AND/OR THEIR RESPECTIVE
SUPPLIERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS CODE .

COPYRIGHT NOTICE. Copyright © 1999-2005 Avid Technology Inc. . All rights reserved. 

SOFTIMAGE is a registered trademark of Avid Technology Inc. or its subsidiaries 
or divisions. Windows NT is a registered trademark of Microsoft Corp. All other
trademarks contained herein are the property of their respective owners. 
****************************************************************************************/

/*! \file plugin_stub.cpp   
    XSI plugin stub functions. This is where the plugin glue is implemented
*/

#include "stdafx.h"
#include "plugin_stub.h"

#include "Export/Export.h"

#include <commdlg.h>

#include "check.h"
#include "Crosswalk/helper.h"
#include "rc.h"

#include "UI/EULADialog.h"

#include "EngineSettingsManager.h"
#include "Export/ExportNodeHelpers.h"

XSI::CString	gOldPath;

#ifndef unix
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
    return TRUE;
}
#endif

/****************************************************************************************
XSI SDK registration functions
****************************************************************************************/
_XSI_EXTERN_ CStatus XSILoadPlugin( PluginRegistrar& in_reg )
{
	bool shouldLoadPlugin = true;

/*
	// Check whether the user accepted the EULA. This is stored in the registry.
	// Note this means that the user could technically use the plugin without
	// accepting the EULA if they workarounded the registry. Not sure how much of a
	// problem that is. I don't think it's feasible to fix it.
	CEngineSettingsManager settingsMgr(_T("XSICryExport"));
	int userHasAcceptedEULA = false;
	const TCHAR* EULAAcceptedKey = _T("acceptedEULA");
	settingsMgr.GetModuleSpecificEntry(EULAAcceptedKey, userHasAcceptedEULA);

	// Ask the user to accept the EULA.
	if (!userHasAcceptedEULA && EULADialog::Show(600, 400, _T("EULA")) == EULADialog::UserResponseAccept)
	{
		userHasAcceptedEULA = true;
		settingsMgr.SetModuleSpecificEntry(EULAAcceptedKey, userHasAcceptedEULA);
	}

	if (!userHasAcceptedEULA)
		shouldLoadPlugin = false;
*/

	in_reg.PutAuthor( L"Crytek" );
	wstring name = L"Crytek Export Plug-In";
	if (!shouldLoadPlugin)
		name += L" (Inactive)";
	in_reg.PutName(name.c_str());
	in_reg.PutVersion( VERSION_MAJOR, VERSION_MINOR );

	if (shouldLoadPlugin)
	{
		HMODULE hLib = LoadLibrary(L"Crosswalk_2_5.dll");
		if (hLib != NULL)	
		{
			FreeLibrary(hLib);
		}
		else
		{
			Application app;
			UIToolkit l_UIToolkit = app.GetUIToolkit();
			LONG l_Result;
			CString l_Error = L"Crosswalk version error! Crosswalk version 2.5 required!";
			l_UIToolkit.MsgBox(l_Error, siMsgOkOnly, L"CryExport", l_Result);
			return CStatus::Fail;
		}

		// Note: Multiple commands can be implemented in the same 
		// plugin

		in_reg.RegisterCommand( L"CreateCryExportCrosswalkOptions", L"CreateCryExportCrosswalkOptions" );
		in_reg.RegisterCommand( L"CreateCryExportMaterialsCrosswalkOptions", L"CreateCryExportMaterialsCrosswalkOptions" );
		in_reg.RegisterCommand( L"CryExportCrosswalk", L"CryExportCrosswalk" );
		in_reg.RegisterCommand( L"ConvertGeometryToCryExportNode", L"ConvertGeometryToCryExportNode" );

		// register the properties for Export
		in_reg.RegisterProperty( L"CryExportCrosswalkOptions" );
		in_reg.RegisterProperty( L"CryExportMaterialsCrosswalkOptions" );

		// register the properties for Export
		in_reg.RegisterProperty( L"CryExportNodeProperties" );

		// register the properties for Export
		in_reg.RegisterProperty( L"CryNodeProperties" );

		// register the properties for Export
		in_reg.RegisterProperty( L"CryObjectProperties" );

		// register command to return version number
		in_reg.RegisterCommand( L"CryCrosswalkVersion", L"CryCrosswalkVersion" );

		in_reg.RegisterCommand( L"MaterialIDChange", L"MaterialIDChange" );

		// register the properties for animaton clip selection
		in_reg.RegisterCommand( L"CreateCryAnimationClipSelectionUI", L"CreateCryAnimationClipSelectionUI" );
 		in_reg.RegisterProperty( L"CryAnimationClipSelectionUI" );
		in_reg.RegisterProperty( L"CryPlotUI" );

 		in_reg.RegisterProperty( L"CryPropertiesUI" );

		in_reg.RegisterProperty( L"CrytekDisplayOptions" );

		in_reg.RegisterMenu( siMenuMainTopLevelID, L"Crytek", true, true );

		in_reg.RegisterCommand( L"CryExport", L"CryExport" );
		in_reg.RegisterCommand( L"CryExportAll", L"CryExportAll" );
		in_reg.RegisterCommand( L"CryExportMaterials", L"CryExportMaterials" );
		in_reg.RegisterCommand( L"CryExportAllMaterials", L"CryExportAllMaterials" );
		in_reg.RegisterCommand( L"ApplyCryShaderToMaterials", L"ApplyCryShaderToMaterials" );
		in_reg.RegisterCommand( L"CreateCryExportNode", L"CreateCryExportNode" );
		in_reg.RegisterCommand( L"CreateCryJointNode", L"CreateCryJointNode" );
		in_reg.RegisterCommand( L"CreateCryNodeProperty", L"CreateCryNodeProperty" );
		in_reg.RegisterCommand( L"AddCryObjectProperty", L"AddCryObjectProperty" );
		in_reg.RegisterCommand( L"CryAnimationClipSelection", L"CryAnimationClipSelection" );
		in_reg.RegisterCommand( L"CreateCryPlot", L"CreateCryPlot" );
		in_reg.RegisterCommand( L"DeleteMAXNormals", L"DeleteMAXNormals" );
		in_reg.RegisterCommand( L"CryDiagnostics", L"CryDiagnostics" );
		in_reg.RegisterCommand( L"CryProperties", L"CryProperties" );
		in_reg.RegisterCommand( L"CryVertexColorPainting", L"CryVertexColorPainting" );
		in_reg.RegisterCommand( L"CryDisplayOptions", L"CryDisplayOptions" );
		in_reg.RegisterCommand( L"CryNewExport", L"CryNewExport" );
		in_reg.RegisterProperty( L"CryExportAbout" );
		in_reg.RegisterDisplayPass( L"CrytekDisplay" );

		Application app;
		CString l_PrefFile = app.GetInstallationPath( siUserAddonPath ) + L"\\Crytek Addon\\XSICryExport.xsipref";
		app.GetPreferences().Import( l_PrefFile );
	}

	return CStatus::OK;	
}

/****************************************************************************************

EXPORT SECTION

****************************************************************************************/
XSIPLUGINCALLBACK XSI::CStatus Crytek_Init( XSI::CRef& in_ref )
{
	Context ctxt = in_ref;
	Menu menu = ctxt.GetSource();

	MenuItem item;
	CStatus st;

	st = menu.AddCallbackItem(L"Export Geometry", L"CryNewExport_Execute", item );
	if (st != CStatus::OK) return st;

	menu.AddSeparatorItem();

	st = menu.AddCallbackItem(L"Create CryExportNode", L"CreateCryExportNode_Execute", item );
	if (st != CStatus::OK) return st;

	st = menu.AddCallbackItem(L"Create CryJointNode", L"CreateCryJointNode_Execute", item );
	if (st != CStatus::OK) return st;

// 	st = menu.AddCallbackItem(L"Create CryJointProperty", L"CreateCryNodeProperty_Execute", item );
// 	if (st != CStatus::OK) return st;

	st = menu.AddCallbackItem(L"Node User Properties", L"AddCryObjectProperty_Execute", item );
	if (st != CStatus::OK) return st;

	menu.AddSeparatorItem();

	st = menu.AddCallbackItem(L"Convert selected materials to CryShader", L"ApplyCryShaderToMaterials_Execute", item );
	if (st != CStatus::OK) return st;

 	menu.AddSeparatorItem();

	st = menu.AddCallbackItem(L"Export Animation", L"CryNewExportAnims_Execute", item );
	if (st != CStatus::OK) return st;

 	st = menu.AddCallbackItem(L"Select Animation Clips for Export", L"CryAnimationClipSelection_Execute", item );
 	if (st != CStatus::OK) return st;
 
 	st = menu.AddCallbackItem(L"Create CryPlot", L"CreateCryPlot_Execute", item );
 	if (st != CStatus::OK) return st;

	menu.AddSeparatorItem();

	st = menu.AddCallbackItem(L"Export Options", L"CryExport_Execute", item );
	if (st != CStatus::OK) return st;

	st = menu.AddCallbackItem(L"Export Materials", L"CryExportAllMaterials_Execute", item );
	if (st != CStatus::OK) return st;

 	menu.AddSeparatorItem();
 
 	st = menu.AddCallbackItem(L"Diagnostics", L"CryDiagnostics_Execute", item );
 	if (st != CStatus::OK) return st;
 
	menu.AddSeparatorItem();

	st = menu.AddCallbackItem(L"Vertex Painting Options", L"CryVertexColorPainting_Execute", item );
	if (st != CStatus::OK) return st;

	st = menu.AddCallbackItem(L"Exporter Properties", L"CryProperties_Execute", item );
	if (st != CStatus::OK) return st;
	
	menu.AddSeparatorItem();

	st = menu.AddCallbackItem(L"About XSICryExporter", L"CryExportAbout_Execute", item );
	if (st != CStatus::OK) return st;

	return CStatus::OK;
}

/****************************************************************************************
XSI plugin export CreateOptions Command definition functions
****************************************************************************************/
_XSI_EXTERN_ CStatus CryCrosswalkVersion_Init( const CRef& in_context )
{
	CStatus status = CStatus::OK;
	Context ctx(in_context);
	Command cmd(ctx.GetSource());

	cmd.EnableReturnValue( true ) ; 

	// See xsi_command.h and xsi_argument.h for further
	// attributes you may wish to set. For example Command.PutTooltip(),
	// and ArgumentArray.AddWithHandler()	

	Application app;

	return status;
}

_XSI_EXTERN_ CStatus CryCrosswalkVersion_Execute( CRef& in_context )
{
	CStatus status = CStatus::OK;
	Application app;
	Context ctxt(in_context);

#ifdef USE_CROSSWALK_EXPORTING
	CValue l_Value(CROSSWALK_VERSION_NUMBER);
	ctxt.PutAttribute( L"ReturnValue", l_Value );
#endif //USE_CROSSWALK_EXPORTING

	return status;
}

_XSI_EXTERN_ CStatus CrySeparator_Init( CRef& in_ref )
{
	CStatus status = CStatus::OK;

	Context ctxt = in_ref;
	Menu menu = ctxt.GetSource();

	CStatus st;
	MenuItem item;

	menu.AddItem(L" ", siMenuItemSeparator , item);

	return status;
}

_XSI_EXTERN_ CStatus MaterialIDChange_Init( CRef& in_ref )
{
	CStatus status = CStatus::OK;

 	Context ctxt = in_ref;
 	Command cmd = ctxt.GetSource();

	ArgumentArray args = cmd.GetArguments();
 	args.Add( L"Arg0" );

	return status;
}

_XSI_EXTERN_ CStatus MaterialIDChange_Execute( CRef& in_context )
{
	CStatus status = CStatus::OK;
	Application app;
	Context ctxt(in_context);
	Command cmd = ctxt.GetSource();
	ArgumentArray argarray = cmd.GetArguments();

	Argument arg = argarray.GetItem(L"Arg0");

	Material l_Material;
	CString l_MatNameFull = arg.GetValue();
	GetMaterialByName(l_MatNameFull, l_Material);
	MaterialLibrary l_MaterialLibrary;
	GetMaterialLibraryByMaterial(l_Material, l_MaterialLibrary);
	LONG l_UnusedMaterialID = GetUnusedMaterialIDByMaterialLibrary(l_MaterialLibrary);

	Parameter l_CryParameter = l_Material.GetParameter(L"surface");
	Shader l_CryShader = l_CryParameter.GetSource();
	LONG l_MaterialID = l_CryShader.GetParameterValue( L"MaterialID" );

	if (IsFreeMaterialIDByMaterialLibrary(l_Material, l_MaterialID, l_MaterialLibrary))
	{
		XSI::COMMANDS::Undo();
		UIToolkit l_UIToolkit = app.GetUIToolkit();
		LONG l_Result;
		CString l_Error = L"MaterialID ";
		l_Error += CValue(l_MaterialID+1);
		l_Error += L" used!";
		l_UIToolkit.MsgBox(l_Error, siMsgOkOnly, L"CryExport", l_Result);
	}
	else
	{
		CString l_MatName = l_Material.GetName();
		const char *l_strMatName = l_MatName.GetAsciiString();

		if (strlen(l_strMatName) > 4)
		{
			char l_strID[3];
			l_strID[0] = l_strMatName[1];
			l_strID[1] = l_strMatName[2];
			l_strID[2] = 0;
			LONG l_ID = atoi(l_strID);
			if (l_strMatName[0] == '_' && l_strMatName[3] == '_' && l_ID > 0 && l_ID < 33)
			{
				l_MatName.PutAsciiString(&l_strMatName[4]);
			}
		}

		CString l_MatIDName = L"_";
		if (l_MaterialID+1 < 10)
			l_MatIDName += L"0";
		l_MatIDName += CValue(l_MaterialID+1);
		l_MatIDName += L"_";
		l_MatName = l_MatIDName + l_MatName;
		l_Material.PutName(l_MatName);
	}

// '			OldMaterialName = oItem.Parent.Name
// '			NewMaterialID = Left(OldMaterialName,4)
// '			NewMaterialName = Right(OldMaterialName,Len(OldMaterialName)-4)
// 
// '			MatIDString = ""
// '			MatID = pset.MaterialID+1
// '			if MatID < 10 then
// '				MatIDString = "0"
// '			end if
// '			MatIDString = MatIDString & MatID
// '			oItem.Parent.Name = "_" & MatIDString & "_" & NewMaterialName

	return status;
}

_XSI_EXTERN_ CStatus CryNewExport_Execute( CRef& in_ref )
{
	XSI::CRefArray exportNodes = Export::FindAllExportNodes();
	Export::Export(exportNodes);
	return CStatus::OK;
}

_XSI_EXTERN_ CStatus CryNewExportAnims_Execute( CRef& in_ref )
{
	Export::ExportAnims();
	return CStatus::OK;
}
