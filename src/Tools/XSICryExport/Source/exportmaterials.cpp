#include "stdafx.h"
#include "plugin_stub.h"

#include <commdlg.h>

#include "check.h"
#include "rc.h"

#include "Properties.h"

/****************************************************************************************
XSI plugin custom properties function
****************************************************************************************/
CStringArray GetSelectedMaterials()
{
	CStringArray l_SplitList;

	Application app;
	Property prop = app.GetActiveSceneRoot().GetProperties().GetItem( L"CryExportMaterialsCrosswalkOptions" ) ;
	CString l_MaterialLibraryList = prop.GetParameterValue(L"MaterialLibraryList");

	app.LogMessage(l_MaterialLibraryList);

	if (l_MaterialLibraryList.Length() == 0)
	{
	}
	else
	{
		l_SplitList = l_MaterialLibraryList.Split(L";");
	}
	return l_SplitList;
}

bool CheckSelectedMaterials()
{
	bool l_bResult = true;
	Application app;

	CRefArray l_MaterialLibraries = app.GetActiveProject().GetActiveScene().GetMaterialLibraries();
	for (LONG i = 0; i < l_MaterialLibraries.GetCount(); i++)
	{
		MaterialLibrary l_MatLib(l_MaterialLibraries[i]);

		bool l_bExport = false;
		CStringArray l_SelectedMaterials = GetSelectedMaterials();
		for (int i = 0; i < l_SelectedMaterials.GetCount(); i++)
		{
			CString l_SelectedMaterial = l_SelectedMaterials[i];
			if (l_SelectedMaterial == l_MatLib.GetName())
				l_bExport = true;
		}
		if (!l_bExport)
			continue;

		bool l_bExportLibrary = false;
		CRefArray l_MaterialList = l_MatLib.GetItems();
		for (int loop2 = 0; loop2 < l_MaterialList.GetCount(); loop2++)
		{
			CRef l_Material = l_MaterialList.GetItem(loop2);
			Material l_XSIMaterial(l_Material);

			bool l_bCryShader = false;
			CRefArray &l_Shaders = l_XSIMaterial.GetShaders();
			for (int j = 0; j < l_Shaders.GetCount(); j++)
			{
				Shader l_Shader(l_Shaders[j]);
				CString l_ProgID = l_Shader.GetProgID();
				char* l_strProgID = (char*)l_ProgID.GetAsciiString();
				if (strstr(l_strProgID, "CryShader.1"))
				{
					l_bCryShader = true;
				}
			}

			CRefArray l_XSIMaterials = l_MatLib.GetItems();
			bool l_bRemove = true;
			for (int i = 0; i < l_XSIMaterials.GetCount(); i++)
			{
				Material l_XSIMat(l_XSIMaterials.GetItem(i));

				CRefArray l_UsedBy = l_XSIMat.GetUsedBy();
				for (int j = 0; j < l_UsedBy.GetCount(); j++)
				{
					ProjectItem l_Obj(l_UsedBy.GetItem(j));
					ProjectItem l_CryExportNode = GetCryExportNode(l_Obj);
					if (l_CryExportNode.GetName() != L"")
						l_bRemove = false;
				}
			}

			if ( l_bRemove || !l_bCryShader )
			{

			}
			else
			{
				l_bExportLibrary = true;
			}
		}

		if (!l_bExportLibrary)
		{
			Application app;
			UIToolkit l_UIToolkit = app.GetUIToolkit();
			LONG l_Result;
			CString l_Error = L"Export MaterialLibrary ";
			l_Error += l_MatLib.GetName();
			l_Error += L" skipped!";
			l_UIToolkit.MsgBox(l_Error, siMsgOkOnly, L"CryExport", l_Result);
			l_bResult = false;
		}
	}
	return l_bResult;
}

_XSI_EXTERN_ CStatus CryExportMaterialsCrosswalkOptions_Define( const CRef & in_Ctx )
{
	CStatus status = CStatus::OK;
	Application app ;
	CustomProperty prop = Context(in_Ctx).GetSource() ;
	Parameter param ;

	// Default capabilities for most of these parameters
	int caps = siPersistable;
	CValue dft ;	// Used for arguments we don't want to set


	// ================================================================================
	// File
	// ================================================================================

	// CrySpecific: Set Format to COLLADA: 1
	dft = (LONG)1;
	prop.AddParameter(	L"Format",CValue::siString, caps,
		L"Format", L"",
		dft, param ) ;

	prop.AddParameter(	L"Format",CValue::siUInt4 , caps, 
		L"Format", L"", 
		dft, param ) ;	

	param.PutValue(CValue((LONG)1L));


	Project l_XSIProject = app.GetActiveProject();
	CString l_FileName = l_XSIProject.GetPath();

#ifdef unix
	l_FileName += L"/dotXSI/default.xsi";
#else
	l_FileName += L"\\dotXSI\\default.xsi";
#endif

	if ( gOldPath.IsEmpty() )
	{
		CString l_FileName = l_XSIProject.GetPath();
		l_FileName += L"\\dotXSI\\";
		param.PutValue(l_FileName);
	} else {
		param.PutValue(gOldPath);
	}

	// ================================================================================
	// Advanced
	// ================================================================================
	prop.AddParameter(	L"Verbose",CValue::siBool, caps,
		L"Verbose", L"",
		dft, param ) ;
	param.PutValue(CValue(true));

	prop.AddParameter(	L"ExportXSIExtra",CValue::siBool, caps,
		L"ExportXSIExtra", L"",
		dft, param ) ;
	// CrySpecific: Set ExportXSIExtra to false
	param.PutValue(CValue(false));

	prop.AddParameter(	L"ExportSelectionOnly",CValue::siBool, caps, 
		L"ExportSelectionOnly", L"", 
		dft, param ) ;	
	// CrySpecific: Set ExportSelectionOnly to true
	param.PutValue(CValue(true));

	// ================================================================================
	// Geometry 
	// ================================================================================
	prop.AddParameter(	L"ApplySubdivisionToGeometry",CValue::siBool, caps, 
		L"ApplySubdivisionToGeometry", L"", 
		dft, param ) ;	
	param.PutValue(CValue(false));

	prop.AddParameter(	L"Triangulate",CValue::siBool, caps, 
		L"Triangulate", L"", 
		dft, param ) ;	
	// CrySpecific: Set Triangulate to true
	param.PutValue(CValue(true));
	param.PutCapabilityFlag( siReadOnly, true );

	prop.AddParameter(	L"ExportTangentsAsVtxColor",CValue::siBool, caps, 
		L"ExportTangentsAsVtxColor", L"", 
		dft, param ) ;	
	param.PutValue(CValue(false));

#ifdef USE_CROSSWALK_EXPORTING
	// ================================================================================
	// Shape animation
	// ================================================================================
	prop.AddParameter(	L"ShapeAnim",CValue::siUInt4 , caps, 
		L"Shapes", L"", 
		dft, param ) ;	
	param.PutValue(CValue((LONG)CShapeAnimationFromXSI::SHAPEKEY_ONLY));
#endif


	// ================================================================================
	// Plot animation
	// ================================================================================
	prop.AddParameter(	L"PlotAnimation",CValue::siBool, caps, 
		L"PlotAnimation", L"", 
		dft, param ) ;	
	// CrySpecific: Set PlotAnimation to true
	param.PutValue(CValue(true));

	// get the starting frame from XSI
	CTime time;
	float startframe;
	startframe = COMMANDS::GetValue(CString(L"PlayControl.In"), time.GetTime());

	// get the ending frame from XSI
	float endframe;
	endframe = COMMANDS::GetValue(CString(L"PlayControl.Out"), time.GetTime());

	prop.AddParameter(	L"PlotStartFrame",CValue::siInt4, caps, 
		L"PlotStartFrame", L"", 
		dft, param ) ;	
	param.PutCapabilityFlag( siReadOnly, true );
	param.PutValue(CValue((LONG)startframe));

	prop.AddParameter(	L"PlotEndFrame",CValue::siInt4, caps, 
		L"PlotEndFrame", L"", 
		dft, param ) ;	
	param.PutCapabilityFlag( siReadOnly, true );
	param.PutValue(CValue((LONG)endframe));

	prop.AddParameter(	L"PlotStepFrame",CValue::siInt4, caps, 
		L"PlotStepFrame", L"", 
		dft, param ) ;	
	param.PutCapabilityFlag( siReadOnly, true );
	param.PutValue(CValue((LONG)1));

	prop.AddParameter(	L"PlotInterpolation",CValue::siInt1, caps, 
		L"PlotInterpolation", L"", 
		dft, param ) ;	
	// CrySpecific: Set PlotInterpolation to Constant: 1
	param.PutValue(CValue((LONG)1));

	prop.AddParameter(	L"PlotFit",CValue::siBool, caps, 
		L"PlotFit", L"", 
		dft, param ) ;	
	param.PutValue(CValue(true));

	prop.AddParameter(	L"PlotFitTolerance",CValue::siFloat, caps, 
		L"PlotFitTolerance", L"", 
		dft, param ) ;	

	param.PutValue(CValue(1.0f));

	prop.AddParameter(	L"PlotProcessRotation",CValue::siBool, caps, 
		L"PlotProcessRotation", L"", 
		dft, param ) ;	
	param.PutValue(CValue(true));

	prop.AddParameter(	L"ExportXSINormals",CValue::siBool, caps, 
		L"ExportXSINormals", L"", 
		dft, param ) ;	
	// CrySpecific: Set ExportXSINormals to Constant: true
	param.PutValue(CValue(true));

	// ================================================================================
	// IK
	// ================================================================================
	prop.AddParameter(	L"PreserveIK",CValue::siBool, caps, 
		L"PreserveIK", L"", 
		dft, param ) ;	
	param.PutValue(CValue(false));

	// ================================================================================
	// Paths
	// ================================================================================
	prop.AddParameter(	L"PathRelative",CValue::siBool, caps, 
		L"PathRelative", L"", 
		dft, param ) ;	
	param.PutValue(CValue(false));


	// ================================================================================
	// CryExport
	// ================================================================================
	prop.AddParameter(	L"MaterialLibraryList",CValue::siString, caps,
		L"MaterialLibraryList", L"",
		L"", param ) ;

	prop.AddParameter(	L"Filename",CValue::siString , caps, 
		L"Filename", L"", 
		L"", param ) ;	
	param.PutCapabilityFlag( siReadOnly, true );

	prop.AddParameter(	L"Type",CValue::siUInt4 , caps, 
		L"Type", L"", 
		dft, param ) ;	

	return status;
}

CStatus CryExportMaterialsCrosswalkOptions_RebuildLayout(PPGLayout &in_Layout, CustomProperty &in_CustomProp)
{
	CStatus status = CStatus::OK;
	PPGItem item ;

	in_Layout.Clear();	

	CValueArray l_MaterialLibraryList;

	Application app;
	CRefArray l_MaterialLibraries = app.GetActiveProject().GetActiveScene().GetMaterialLibraries();
	for (LONG i = 0; i < l_MaterialLibraries.GetCount(); i++)
	{
		MaterialLibrary l_MatLib(l_MaterialLibraries[i]);

		int l_CryShader = 0;
		int l_Remove = 0;
		CRefArray l_MaterialList = l_MatLib.GetItems();
		for (int loop2 = 0; loop2 < l_MaterialList.GetCount(); loop2++)
		{
			CRef l_Material = l_MaterialList.GetItem(loop2);
			Material l_XSIMaterial(l_Material);

			CRefArray &l_Shaders = l_XSIMaterial.GetShaders();
			for (int j = 0; j < l_Shaders.GetCount(); j++)
			{
				Shader l_Shader(l_Shaders[j]);
				CString l_ProgID = l_Shader.GetProgID();
				char* l_strProgID = (char*)l_ProgID.GetAsciiString();
				if (strstr(l_strProgID, "CryShader.1"))
				{
					l_CryShader++;
				}
			}

			CRefArray l_UsedBy = l_XSIMaterial.GetUsedBy();
			for (int j = 0; j < l_UsedBy.GetCount(); j++)
			{
				ProjectItem l_Obj(l_UsedBy.GetItem(j));
				ProjectItem l_CryExportNode = GetCryExportNode(l_Obj);
				if (l_CryExportNode.GetName() == L"")
					l_Remove++;
			}

		}

		CString l_MatLibInfo = l_MatLib.GetName();
		l_MatLibInfo += L"(Materials: ";
		l_MatLibInfo += CValue(l_MaterialList.GetCount());
		l_MatLibInfo += L" CryShader: ";
		l_MatLibInfo += CValue(l_CryShader);
		l_MatLibInfo += L")";
		l_MaterialLibraryList.Add(l_MatLibInfo);
		l_MaterialLibraryList.Add(l_MatLib.GetName());
	}

	in_Layout.AddGroup(L"MaterialLibraries");
	item = in_Layout.AddEnumControl( L"MaterialLibraryList", l_MaterialLibraryList, L"", siControlListBox ) ;
	item.PutAttribute( siUIMultiSelectionListBox, true );
	item.PutAttribute( siUICY, (LONG)200 ) ;
	item.PutAttribute( siUIStyle, (LONG)0x10200101 ) ;
	item.PutAttribute( siUINoLabel, true ) ;
	item = in_Layout.AddButton(L"Refresh", L"Refresh");
	in_Layout.EndGroup();

	// ================================================================================
	// The Export Selected Materials button
	// ================================================================================
	in_Layout.AddGroup(L"");
	item = in_Layout.AddButton(L"ExportSelectedMaterials", L"Export Selected Materials");
	in_Layout.EndGroup();

	// ================================================================================
	// The Export All Materials button
	// ================================================================================
	in_Layout.AddGroup(L"");
	item = in_Layout.AddButton(L"ExportAllMaterials", L"Export All Materials");
	in_Layout.EndGroup();

	return status;
}

_XSI_EXTERN_ CStatus CryExportMaterialsCrosswalkOptions_DefineLayout( const CRef & in_Ctx )
{
	CStatus status = CStatus::OK;

	// XSI will call this to define the visual appearance of the CustomProperty
	// The layout is shared between all instances of the CustomProperty
	// and is cached.  You can force the code to re-execute by using the 
	// XSIUtils.Refresh feature.

	PPGLayout oLayout = Context( in_Ctx ).GetSource() ;

	CustomProperty cusProp;
	status = CryExportMaterialsCrosswalkOptions_RebuildLayout(oLayout, cusProp);

	return status;
}

_XSI_EXTERN_ CStatus CryExportMaterialsCrosswalkOptions_PPGEvent( const CRef& io_Ctx )
{
	CStatus status = CStatus::OK;
	// This callback is called when events happen in the user interface
	// This is where the "logic" code is implemented.

	Application app ;

	PPGEventContext ctx( io_Ctx ) ;

	PPGEventContext::PPGEvent eventID = ctx.GetEventID() ;

	if ( eventID == PPGEventContext::siParameterChange )
	{		
		// For this event the Source of the event is the parameter
		// itself
		Parameter changed = ctx.GetSource() ;	

		CustomProperty prop = changed.GetParent() ;	

		CString   paramName = changed.GetScriptName() ; 
	}
	else if ( eventID == PPGEventContext::siButtonClicked )
	{
		CustomProperty prop = ctx.GetSource();

		CValue l_ButtonName = ctx.GetAttribute( L"Button" );

		if (l_ButtonName.GetAsText() == L"ExportAllMaterials")
		{
			CValue			retValue;
			CValueArray		args(1);

			CValue l_Value;
			Property prop = app.GetActiveSceneRoot().GetProperties().GetItem( L"CryExportMaterialsCrosswalkOptions" );
			CString l_FolderPath = GetExportPath();
			l_FolderPath += L"\\";
			CString l_Filename = l_FolderPath + L"allmaterialstemp.dae";

			prop.PutParameterValue(L"Filename", l_Filename);

			// For the sake of simplicity here the command takes an CryExportCrosswalkOptions property
			// as its only parameter
			args[0] = prop.GetFullName();
			prop.PutParameterValue(L"Type",(short)EXPORT_ALLMATERIALS);

			if (Diagnostics(false))
			{
				if (app.ExecuteCommand(L"CryExportCrosswalk", args, retValue) == CStatus::OK)
					RunResourceCompiler(l_Filename, false);	

				DeleteFile(l_Filename.GetWideString());
			}
		}
		else if (l_ButtonName.GetAsText() == L"ExportSelectedMaterials")
		{
			CStringArray l_SelectedMaterials = GetSelectedMaterials();
			if (!l_SelectedMaterials.GetCount())
			{
				Application app;
				UIToolkit l_UIToolkit = app.GetUIToolkit();
				LONG l_Result;
				l_UIToolkit.MsgBox(L"No materials selected!", siMsgOkOnly, L"CryExport", l_Result);
			}
			else
			{
				CValue			retValue;
				CValueArray		args(1);

				CValue l_Value;
				Property prop = app.GetActiveSceneRoot().GetProperties().GetItem( L"CryExportMaterialsCrosswalkOptions" );
				CString l_FolderPath = GetExportPath();
				l_FolderPath += L"\\";
				CString l_Filename = l_FolderPath + L"selectedmaterialstemp.dae";

				prop.PutParameterValue(L"Filename", l_Filename);

				// For the sake of simplicity here the command takes an CryExportCrosswalkOptions property
				// as its only parameter
				args[0] = prop.GetFullName();
				prop.PutParameterValue(L"Type",(short)EXPORT_MATERIALS);

				if (Diagnostics(false) && CheckSelectedMaterials())
				{
					if (app.ExecuteCommand(L"CryExportCrosswalk", args, retValue) == CStatus::OK)
						RunResourceCompiler(l_Filename, false);	

					DeleteFile(l_Filename.GetWideString());
				}
			}
		}
		else if (l_ButtonName.GetAsText() == L"Refresh")
		{
			eventID = PPGEventContext::siOnInit;
		}
	}
	if( eventID == PPGEventContext::siOnInit )
	{
		CustomProperty prop = ctx.GetSource();
		PPGLayout oPPGLayout = prop.GetPPGLayout();		
		CryExportMaterialsCrosswalkOptions_RebuildLayout(oPPGLayout, prop);

		//Redraw the PPG to show the new combo items
		ctx.PutAttribute(L"Refresh",true);
	}

	return status;
}

/****************************************************************************************
XSI plugin export CreateOptions Command definition functions
****************************************************************************************/
_XSI_EXTERN_ CStatus CreateCryExportMaterialsCrosswalkOptions_Init( const CRef& in_context )
{
	CStatus status = CStatus::OK;
	Context ctx(in_context);
	Command cmd(ctx.GetSource());

	cmd.EnableReturnValue( true ) ; 

	// See xsi_command.h and xsi_argument.h for further
	// attributes you may wish to set. For example Command.PutTooltip(),
	// and ArgumentArray.AddWithHandler()	

	Application app;

	// Configure this code based on the number of arguments
	// you want.  You can also specific default values for these arguments.

	ArgumentArray args = cmd.GetArguments();
	args.Add( L"arg0", L"\0" );
	args.Add( L"arg1", L"CryExportMaterialsCrosswalkOptions\0" );

	return status;
}

_XSI_EXTERN_ CStatus CreateCryExportMaterialsCrosswalkOptions_Execute( CRef& in_context )
{
	CStatus status = CStatus::OK;
	Application app;
	Context ctxt(in_context);
	Property l_Property;

	// Access the arguments to the command
	CValueArray		args = ctxt.GetAttribute( L"Arguments" );
	CString			argObjectName(args[0]);
	CString			argOptionName(args[1]);

	if (argOptionName.IsEmpty())
	{
		argOptionName = L"CryExportMaterialsCrosswalkOptions";
	}

	if (!argObjectName.IsEmpty())
	{
		X3DObject  l_XSIObject = app.GetActiveSceneRoot().FindChild(argObjectName, CString(), CStringArray() );

		if(l_XSIObject.IsValid())
		{
			l_Property = l_XSIObject.AddProperty( L"CryExportMaterialsCrosswalkOptions" ) ;
		}
	}
	else
	{
		l_Property = app.GetActiveSceneRoot().AddProperty( L"CryExportMaterialsCrosswalkOptions" ) ;
	}

	if (l_Property.IsValid())
	{
		l_Property.PutName(argOptionName);
	}

	CValue l_Value(l_Property);
	ctxt.PutAttribute( L"ReturnValue", l_Value );

	return status;
}

_XSI_EXTERN_ CStatus CryExportMaterials_Execute( CRef& in_ref )
{
	CStatus status = CStatus::OK;
	Application app;

	Property prop = app.GetActiveSceneRoot().GetProperties().GetItem( L"CryExportMaterialsCrosswalkOptions" ) ;

	if (!prop.IsValid())
	{
		prop = app.GetActiveSceneRoot().AddProperty( L"CryExportMaterialsCrosswalkOptions" ) ;
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

	XSI::COMMANDS::InspectObj(L"CryExportMaterialsCrosswalkOptions",CValue(),l_wLabel,(LONG)siRecycle,false);

	return status;
}

_XSI_EXTERN_ CStatus CryExportAllMaterials_Execute( CRef& in_ref )
{
	CStatus status = CStatus::OK;
	Application app;

	Property prop = app.GetActiveSceneRoot().GetProperties().GetItem( L"CryExportMaterialsCrosswalkOptions" ) ;

	if (!prop.IsValid())
	{
		prop = app.GetActiveSceneRoot().AddProperty( L"CryExportMaterialsCrosswalkOptions" ) ;
	}

	CValue			retValue;
	CValueArray		args(1);

	CValue l_Value;
	CString l_FolderPath = GetExportPath();
	l_FolderPath += L"\\";
	CString l_Filename = l_FolderPath + L"allmaterialstemp.dae";

	prop.PutParameterValue(L"Filename", l_Filename);

	args[0] = prop.GetFullName();
	prop.PutParameterValue(L"Type",(short)EXPORT_ALLMATERIALS);

	if (Diagnostics(false))
	{
		if (app.ExecuteCommand(L"CryExportCrosswalk", args, retValue) == CStatus::OK)
			RunResourceCompiler(l_Filename, false);

		DeleteFile(l_Filename.GetWideString());
	}
	return status;
}
