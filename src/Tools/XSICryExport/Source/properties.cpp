
#include "stdafx.h"

#include "plugin_stub.h"
#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_menu.h>
#include <xsi_property.h>
#include <xsi_model.h>
#include <xsi_command.h>
#include <xsi_argument.h>
#include <xsi_ppglayout.h>
#include <xsi_time.h>
#include <xsi_parameter.h>
#include <xsi_ppgeventcontext.h>
#include <xsi_selection.h>
#include <xsi_uitoolkit.h>
#include <xsi_griddata.h>
#include <xsi_color.h> 

#include <xsi_material.h>
#include <xsi_shader.h>
#include <xsi_kinematics.h>
#include <xsi_imageclip.h>
#include <xsi_actionsource.h>
#include <xsi_mixer.h>
#include <xsi_materiallibrary.h>
#include <xsi_preferences.h>
#include <xsi_null.h>
#include <xsi_geometry.h>
#include <xsi_primitive.h>
#include <xsi_polygonmesh.h>
#include <xsi_geometryaccessor.h>

#include <commdlg.h>

_XSI_EXTERN_ CStatus CryProperties_Execute( CRef& in_ref )
{
	CStatus status = CStatus::OK;
	Application app;

	Property prop = app.GetActiveSceneRoot().GetProperties().GetItem( L"CryPropertiesUI" ) ;

	if (!prop.IsValid())
	{
		prop = app.GetActiveSceneRoot().AddProperty( L"CryPropertiesUI" ) ;
	}

	// Let's build our label for the property page
	wchar_t l_wLabel[1024];
	swprintf(l_wLabel,
#ifdef unix
		sizeof(l_wLabel),
		L"Crosswalk Export v%ls build %s %s",
#else
		L"Crosswalk Export v%s build %S %S",
#endif
		TOOL_VERSION, __DATE__, __TIME__);

	XSI::COMMANDS::InspectObj(L"CryPropertiesUI",CValue(),l_wLabel,(LONG)siRecycle,false);

	return status;
}

_XSI_EXTERN_ CStatus CryProperties_Init( CRef& in_ref )
{
	CStatus status = CStatus::OK;

	Context ctxt = in_ref;
	Menu menu = ctxt.GetSource();

	CStatus st;
	MenuItem item;

	// Here we add our menu item in the export menu
	menu.AddCallbackItem(L"CryProperties", L"CryProperties_Execute", item);

	return status;
}

/****************************************************************************************
CryPropertiesUI
****************************************************************************************/
_XSI_EXTERN_ CStatus CryPropertiesUI_Define( const CRef & in_Ctx )
{
	CStatus status = CStatus::OK;
	Application app ;
	CustomProperty prop = Context(in_Ctx).GetSource() ;
	Parameter param ;

	// Default capabilities for most of these parameters
	int caps = siPersistable;
	CValue dft ;	// Used for arguments we don't want to set

	prop.AddParameter( L"Bin32Path",CValue::siString, caps,
		L"Bin32Path", L"",
		dft, param ) ;

	prop.AddParameter( L"ExportPath",CValue::siString, caps,
		L"ExportPath", L"",
		dft, param ) ;

	prop.AddParameter( L"ANMSampleStep",CValue::siInt2, caps,
		L"ANMSampleStep", L"The ANM animation keyframe sample step for the timeline",
		2, param ) ;

	return status;
}

CStatus CryPropertiesUI_RebuildLayout(PPGLayout &in_Layout, CustomProperty &in_CustomProp)
{
	CStatus status = CStatus::OK;

	PPGItem item ;

	in_Layout.Clear();	

	in_Layout.AddRow();
	in_Layout.AddGroup( L"Bin32 Path" );
	item = in_Layout.AddItem( L"Bin32Path",L"Bin32 Path",siControlFolder );
	//	item = in_Layout.AddItem( L"RCFilename",L"File name",siControlFilePath );
	// 	item.PutAttribute( siUIFileFilter, L"Executable (*.exe)|*.exe||" );
	// 	item.PutAttribute( siUIFileMustExist, true );
	// 	item.PutAttribute( siUIOpenFile, true );
	item.PutAttribute( siUINoLabel, true );
	in_Layout.EndGroup();
	in_Layout.EndRow();

	in_Layout.AddRow();
	in_Layout.AddGroup( L"Export Path" );
	item = in_Layout.AddItem( L"ExportPath",L"Export Path",siControlFolder );
	item.PutAttribute( siUINoLabel, true );
	in_Layout.EndGroup();
	in_Layout.EndRow();

	Application app;
	CString l_PrefFile = app.GetInstallationPath( siUserAddonPath ) + L"\\Crytek Addon\\XSICryExport.xsipref";
	app.GetPreferences().Import( l_PrefFile );

	CValue l_Value;
	app.GetPreferences().GetPreferenceValue(L"XSICryExport.Bin32Path", l_Value);
	in_CustomProp.PutParameterValue( L"Bin32Path", l_Value.GetAsText());

	in_Layout.AddRow();
	in_Layout.AddGroup( L"ANM animations" );
	in_Layout.AddItem( L"ANMSampleStep", L"KeySampleStep", siControlNumber );

	return status;
}

_XSI_EXTERN_ CStatus CryPropertiesUI_DefineLayout( const CRef & in_Ctx )
{
	CStatus status = CStatus::OK;

	// XSI will call this to define the visual appearance of the CustomProperty
	// The layout is shared between all instances of the CustomProperty
	// and is cached.  You can force the code to re-execute by using the 
	// XSIUtils.Refresh feature.

	PPGLayout oLayout = Context( in_Ctx ).GetSource() ;

	CustomProperty cusProp;
	status = CryPropertiesUI_RebuildLayout(oLayout, cusProp);

	return status;
}

_XSI_EXTERN_ CStatus CryPropertiesUI_PPGEvent( const CRef& io_Ctx )
{
	CStatus status = CStatus::OK;
	// This callback is called when events happen in the user interface
	// This is where the "logic" code is implemented.

	Application app ;

	PPGEventContext ctx( io_Ctx ) ;
	//Redraw the PPG to show the new combo items
	ctx.PutAttribute(L"Refresh",true);

	PPGEventContext::PPGEvent eventID = ctx.GetEventID() ;

	if ( eventID == PPGEventContext::siParameterChange )
	{		
		Parameter changed = ctx.GetSource() ;	
		CustomProperty prop = changed.GetParent() ;	
		CString   paramName = changed.GetScriptName() ;

		if ( paramName == L"Bin32Path" )
		{
			eventID = PPGEventContext::siOnInit;
			CString l_Bin32Path = prop.GetParameterValue(L"Bin32Path");
			CStatus l_Status = app.GetPreferences().SetPreferenceValue(L"XSICryExport.Bin32Path", l_Bin32Path);
			CString l_PrefFile = app.GetInstallationPath( siUserAddonPath ) + L"\\Crytek Addon\\XSICryExport.xsipref";
			app.GetPreferences().Export( l_PrefFile, L"XSICryExport" );
		}
	}
	if( eventID == PPGEventContext::siOnInit )
	{
		CustomProperty prop = ctx.GetSource();
		PPGLayout oPPGLayout = prop.GetPPGLayout();		
		CryPropertiesUI_RebuildLayout(oPPGLayout, prop);

		//Redraw the PPG to show the new combo items
		ctx.PutAttribute(L"Refresh",true);
	}

	return status;
}

CString GetExportPath()
{
	Application app;
	Property prop = app.GetActiveSceneRoot().GetProperties().GetItem( L"CryPropertiesUI" );
	CString l_ExportPath = prop.GetParameterValue( L"ExportPath", 0.0f );
	wchar_t inputPath[1024];
	wcscpy(inputPath, l_ExportPath.GetWideString());

	// Handle relative paths.
	wchar_t path[1024] = L"";
	if (!(inputPath[0] && inputPath[0] == '\\' || inputPath[0] == '/' || inputPath[1] == ':'))
	{
		Project proj = app.GetActiveProject();
		Scene scn = proj.GetActiveScene();
		CParameterRefArray sceneParams = scn.GetParameters();
		Parameter p = sceneParams.GetItem(L"Filename");
		CString sRealName = p.GetValue(double(1));

		wchar_t scenePath[1024];
		wcscpy(scenePath, sRealName.GetWideString());
		size_t i;
		for (i = wcslen(scenePath); i > 0 && scenePath[i] != '/' && scenePath[i] != '\\'; --i);
		scenePath[i] = 0;

		wcscpy(path, scenePath);
		wcscat(path, L"\\");
	}

	wcscat(path, inputPath);

	wchar_t fullPath[1024];
	GetFullPathName(path, sizeof(fullPath) / sizeof(fullPath[0]), fullPath, 0);
	return CString(fullPath);
}
