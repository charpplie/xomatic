#include "stdafx.h"
#include "plugin_stub.h"

#include "Properties.h"
#include "cmdstubs.h"
#include "check.h"
#include "rc.h"

#include "Crosswalk/helper.h"
#include "Export/Export.h"

extern XSI::CString	gOldPath;

/****************************************************************************************
XSI plugin custom properties function
****************************************************************************************/
using namespace XSI;


/****************************************************************************************
XSI plugin menu event functions
****************************************************************************************/
_XSI_EXTERN_ CStatus CryExport_Execute( CRef& in_ref )
{
	CStatus status = CStatus::OK;
	Application app;

	Property prop = app.GetActiveSceneRoot().GetProperties().GetItem( L"CryExportCrosswalkOptions" ) ;

	if (!prop.IsValid())
	{
		prop = app.GetActiveSceneRoot().AddProperty( L"CryExportCrosswalkOptions" ) ;
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

	XSI::COMMANDS::InspectObj(L"CryExportCrosswalkOptions",CValue(),l_wLabel,(LONG)siRecycle,false);

	return status;
}

_XSI_EXTERN_ CStatus CryExport_Init( CRef& in_ref )
{
	CStatus status = CStatus::OK;

	Context ctxt = in_ref;
	Menu menu = ctxt.GetSource();

	CStatus st;
	MenuItem item;

	// Here we add our menu item in the export menu
	menu.AddCallbackItem(L"CryExport", L"CryExport_Execute", item);

	return status;
}


_XSI_EXTERN_ CStatus CryExportCrosswalkOptions_Define( const CRef & in_Ctx )
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
#endif //USE_CROSSWALK_EXPORTING


	// ================================================================================
	// Plot animation
	// ================================================================================
	prop.AddParameter(	L"PlotAnimation",CValue::siBool, caps, 
		L"PlotAnimation", L"", 
		dft, param ) ;	
	// CrySpecific: Set PlotAnimation to true
	param.PutValue(CValue(false));

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
	prop.AddParameter(	L"CryExportNodeList",CValue::siString, caps,
		L"CryExportNodeList", L"",
		L"", param ) ;

	prop.AddParameter(	L"FilenameSubset",CValue::siString , caps, 
		L"FilenameSubset", L"", 
		L"", param ) ;	
	param.PutCapabilityFlag( siReadOnly, true );

	prop.AddParameter(	L"FilenameAll",CValue::siString , caps, 
		L"FilenameAll", L"", 
		L"", param ) ;	
	param.PutCapabilityFlag( siReadOnly, true );

	prop.AddParameter(	L"Filename",CValue::siString , caps, 
		L"Filename", L"", 
		L"", param ) ;	
	param.PutCapabilityFlag( siReadOnly, true );

	prop.AddParameter(	L"Type",CValue::siUInt4 , caps, 
		L"Type", L"", 
		dft, param ) ;	

	return status;
}

CStatus CryExportCrosswalkOptions_RebuildLayout(PPGLayout &in_Layout, CustomProperty &in_CustomProp)
{
	CStatus status = CStatus::OK;

	PPGItem item ;

	in_Layout.Clear();	

	// ================================================================================
	// General plugin description string
	// ================================================================================
	wchar_t l_wLabel[1024];
	swprintf(l_wLabel, L"%d.%d (%S, %S)", /*VERSION_MAJOR*/0, /*VERSION_MINOR*/0, __DATE__, __TIME__);
	CString l_Version(L"CryExport ");
	CString l_CrosswalkVersion(L"Based on Crosswalk ");
	l_Version += l_wLabel;

#ifdef USE_CROSSWALK_EXPORTING
	CString l_CrosswalkVersionNumber(CROSSWALK_VERSION_NUMBER);
	l_CrosswalkVersion += l_CrosswalkVersionNumber;
#endif

	in_Layout.AddGroup();
	in_Layout.AddRow();
	in_Layout.AddStaticText(l_Version);
	in_Layout.EndRow();
	in_Layout.AddRow();
	in_Layout.AddStaticText(l_CrosswalkVersion);
	in_Layout.EndRow();
	in_Layout.EndGroup();

	// ================================================================================
	// CryExportNodes List
	// ================================================================================
	CValueArray l_CryExportNodeList;
	CRefArray l_AllCryExportNodes = GetAllCryExportNodes();

	int loop;

	for (loop = 0; loop < l_AllCryExportNodes.GetCount(); loop++)
	{
		X3DObject l_Object(l_AllCryExportNodes.GetItem(loop));

		l_CryExportNodeList.Add(GetCryExportNodeListName(l_Object));
		l_CryExportNodeList.Add(l_Object.GetName());
	}

	in_Layout.AddGroup(L"CryExportNodes");
	item = in_Layout.AddEnumControl( L"CryExportNodeList", l_CryExportNodeList, L"", siControlListBox ) ;
	item.PutAttribute( siUIMultiSelectionListBox, true );
	item.PutAttribute( siUICY, (LONG)200 ) ;
	item.PutAttribute( siUIStyle, (LONG)0x10200101 ) ;
	//	item.PutAttribute( siUIStyle, (LONG)0x0101 ) ;
	item.PutAttribute( siUINoLabel, true ) ;
	item = in_Layout.AddButton(L"Refresh", L"Refresh");
	in_Layout.EndGroup();

	// The new export button.
	in_Layout.AddGroup(L"");
	item = in_Layout.AddButton(L"ExportSelected", L"Export Selected");
	item = in_Layout.AddButton(L"ExportAll", L"Export All");
	in_Layout.EndGroup();

	// The Crosswalk export buttons.
	// ================================================================================
	// The Export button
	// ================================================================================
	in_Layout.AddGroup(L"");
	item = in_Layout.AddButton(L"ExportSubsetCrosswalk", L"Export Subset via Crosswalk");
	item = in_Layout.AddString(L"FilenameSubset",L"File Name");
	in_Layout.EndGroup();

	// ================================================================================
	// The Export All button
	// ================================================================================
	in_Layout.AddGroup(L"");
	item = in_Layout.AddButton(L"ExportAllCrosswalk", L"Export All via Crosswalk");
	item = in_Layout.AddString(L"FilenameAll",L"File Name");
	in_Layout.EndGroup();

	// ================================================================================
	// Note
	// ================================================================================
	in_Layout.AddGroup(L"");
	in_Layout.AddStaticText(L"Note: To change the destination folder, edit");
	in_Layout.AddStaticText(L"the Crytek preferences via the Crytek menu.");
	in_Layout.EndGroup();

	// ================================================================================
	// help
	// ================================================================================
	//in_Layout.PutAttribute( siUIHelpFile, L"<FactoryPath>/Doc/<DocLangPref>/XSI/xsidocs.chm::/menubar20.htm");


	return status;
}

_XSI_EXTERN_ CStatus CryExportCrosswalkOptions_DefineLayout( const CRef & in_Ctx )
{
	CStatus status = CStatus::OK;

	// XSI will call this to define the visual appearance of the CustomProperty
	// The layout is shared between all instances of the CustomProperty
	// and is cached.  You can force the code to re-execute by using the 
	// XSIUtils.Refresh feature.

	PPGLayout oLayout = Context( in_Ctx ).GetSource() ;

	CustomProperty cusProp;
	status = CryExportCrosswalkOptions_RebuildLayout(oLayout, cusProp);

	return status;
}

_XSI_EXTERN_ CStatus CryExportCrosswalkOptions_PPGEvent( const CRef& io_Ctx )
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

		bool l_bPlot = prop.GetParameterValue( L"PlotAnimation" );
		bool l_bFit = prop.GetParameterValue( L"PlotFit" );

		Parameter start = prop.GetParameters().GetItem( L"PlotStartFrame" ) ;
		Parameter end = prop.GetParameters().GetItem( L"PlotEndFrame" ) ;
		Parameter step = prop.GetParameters().GetItem( L"PlotStepFrame" ) ;
		Parameter tolerance = prop.GetParameters().GetItem( L"PlotFitTolerance" ) ;

		if ( paramName == L"PlotAnimation" )
		{
			start.PutCapabilityFlag( siReadOnly, !l_bPlot );
			end.PutCapabilityFlag( siReadOnly, !l_bPlot );
			step.PutCapabilityFlag( siReadOnly, !l_bPlot );
		}
		if ( paramName == L"PlotFit" )
		{
			tolerance.PutCapabilityFlag( siReadOnly, !l_bFit);
		}
		if ( paramName == L"Format" )
		{
			PPGLayout oPPGLayout = prop.GetPPGLayout();		

			CryExportCrosswalkOptions_RebuildLayout(oPPGLayout, prop);

			//Redraw the PPG to show the new combo items
			ctx.PutAttribute(L"Refresh",true);
		}
		if ( paramName == L"Target" )
		{
			PPGLayout oPPGLayout = prop.GetPPGLayout();		

			CryExportCrosswalkOptions_RebuildLayout(oPPGLayout, prop);

			//Redraw the PPG to show the new combo items
			ctx.PutAttribute(L"Refresh",true);
		}
	}
	else if ( eventID == PPGEventContext::siButtonClicked )
	{
		CustomProperty prop = ctx.GetSource();

		CValue l_ButtonName = ctx.GetAttribute( L"Button" );

		if (l_ButtonName.GetAsText() == L"ExportSelected" || l_ButtonName.GetAsText() == L"ExportAll")
		{
			XSI::CRefArray nodesToExport;
			if (l_ButtonName.GetAsText() == L"ExportAll")
			{
				nodesToExport = Export::FindAllExportNodes();
			}
			else
			{
				CString l_StringCryExportNodeList = prop.GetParameterValue(L"CryExportNodeList");

				if (l_StringCryExportNodeList.Length() == 0)
				{
					app.LogMessage(L"You must specify the CryExportNode(s) - Export Subset was canceled", siErrorMsg);
					return CStatus::Fail;
				}
				else
				{
					CStringArray l_SplitList = l_StringCryExportNodeList.Split(L";");

					if (!l_SplitList.GetCount())
					{
						app.LogMessage(L"You must select a CryExportNode(s) - Export Subset was canceled", siErrorMsg);
						return CStatus::Fail;
					}

					for (int loop = 0; loop < l_SplitList.GetCount(); loop++)
					{
						CString l_CryExportNodeName = l_SplitList[loop];
						CStringArray l_CryExportNodeNameSplit = l_CryExportNodeName.Split(L":");

						if (l_CryExportNodeNameSplit.GetCount() == 1)
						{
							X3DObject l_Object = GetCryExportNodeByName(l_SplitList[loop]);
							nodesToExport.Add(l_Object.GetRef());
						}
						else
						{
							X3DObject l_Object = GetCryExportNodeByName(l_CryExportNodeNameSplit[0]);

							if (l_CryExportNodeNameSplit[1] == L"CHR" || l_CryExportNodeNameSplit[1] == L"CGF" || l_CryExportNodeNameSplit[1] == L"CGA")
								nodesToExport.Add(l_Object.GetRef());
						}
					}
				}
			}

			Export::Export(nodesToExport);
		}

		if (l_ButtonName.GetAsText() == L"ExportSubsetCrosswalk")
		{

#ifdef USE_CROSSWALK_EXPORTING
			CValue			retValue;
			CValueArray		args(1);

			CString l_Filename = prop.GetParameterValue(L"FilenameSubset");

			prop.PutParameterValue(L"Filename", l_Filename);

			// For the sake of simplicity here the command takes an CryExportCrosswalkOptions property
			// as its only parameter
			args[0] = prop.GetFullName();
			prop.PutParameterValue(L"Type",(short)EXPORT_SUBSET);

			if (Diagnostics(false))
			{
				if (app.ExecuteCommand(L"CryExportCrosswalk", args, retValue) == CStatus::OK)
					RunResourceCompiler(l_Filename, true);
			}
#endif //USE_CROSSWALK_EXPORTING

		}
		else if (l_ButtonName.GetAsText() == L"ExportAllCrosswalk")
		{

#ifdef USE_CROSSWALK_EXPORTING
			CValue			retValue;
			CValueArray		args(1);

			CString l_Filename = prop.GetParameterValue(L"FilenameAll");

			prop.PutParameterValue(L"Filename", l_Filename);

			// For the sake of simplicity here the command takes an CryExportCrosswalkOptions property
			// as its only parameter
			args[0] = prop.GetFullName();
			prop.PutParameterValue(L"Type",(short)EXPORT_ALL);

			if (Diagnostics(false))
			{
				if (app.ExecuteCommand(L"CryExportCrosswalk", args, retValue) == CStatus::OK)
					RunResourceCompiler(l_Filename, true);
			}
#endif //USE_CROSSWALK_EXPORTING

		}
		else if (l_ButtonName.GetAsText() == L"http://softimage.wiki.avid.com/index.php/Category:Crosswalk")
		{
			CValue			retValue;
			CValueArray		args(1);

			// For the sake of simplicity here the command takes an CryExportCrosswalkOptions property
			// as its only parameter
			args[0] = L"http://softimage.wiki.avid.com/index.php/Category:Crosswalk";

			app.ExecuteCommand(L"OpenNetView", args, retValue);
		}
		else if (l_ButtonName.GetAsText() == L"Refresh")
		{
			eventID = PPGEventContext::siOnInit;
		}
	}
	if( eventID == PPGEventContext::siOnInit )
	{
		CustomProperty prop = ctx.GetSource() ;
		PPGLayout oPPGLayout = prop.GetPPGLayout();		
		PPGItem oPPGItem;

		/////////////////////////////////////////////////////////////////////////
		// Update the CryExportNode list
		/////////////////////////////////////////////////////////////////////////
		CValueArray l_CryExportNodeList;
		CRefArray l_AllCryExportNodes = GetAllCryExportNodes();

		int loop;

		for (loop = 0; loop < l_AllCryExportNodes.GetCount(); loop++)
		{
			X3DObject l_Object(l_AllCryExportNodes.GetItem(loop));

			if (GetCryExportNodeFiletype(l_Object) == (CRY_FILE_TYPE_CHR|CRY_FILE_TYPE_CAF))
			{
				l_CryExportNodeList.Add(GetCryExportNodeListName(l_Object, L"CHR"));
				l_CryExportNodeList.Add(l_Object.GetName() + L":CHR");

				l_CryExportNodeList.Add(GetCryExportNodeListName(l_Object, L"CAF"));
				l_CryExportNodeList.Add(l_Object.GetName() + L":CAF");
			}
			else if (GetCryExportNodeFiletype(l_Object) == (CRY_FILE_TYPE_CGA|CRY_FILE_TYPE_ANM))
			{
				l_CryExportNodeList.Add(GetCryExportNodeListName(l_Object, L"CGA"));
				l_CryExportNodeList.Add(l_Object.GetName() + L":CGA");

				l_CryExportNodeList.Add(GetCryExportNodeListName(l_Object, L"ANM"));
				l_CryExportNodeList.Add(l_Object.GetName() + L":ANM");
			}
			else
			{
				l_CryExportNodeList.Add(GetCryExportNodeListName(l_Object));
				l_CryExportNodeList.Add(l_Object.GetName());
			}
		}

		oPPGItem = oPPGLayout.GetItem(L"CryExportNodeList");
		oPPGItem.PutUIItems(l_CryExportNodeList);

		/////////////////////////////////////////////////////////////////////////
		// Update the filenames
		/////////////////////////////////////////////////////////////////////////
		CValue l_Value;
		CString l_FolderPath = GetExportPath();
		l_FolderPath += L"\\";

		Scene l_Scene = app.GetActiveProject().GetActiveScene();
		CString l_FullFilename = l_Scene.GetParameterValue(L"Filename");
		CStringArray l_FullFilenameSplit = l_FullFilename.Split( L"\\");
		const char *l_pFilename = l_FullFilenameSplit[l_FullFilenameSplit.GetCount()-1].GetAsciiString();

		CString l_Filename = l_FolderPath;
		CString l_FilenameSubset = l_FolderPath;

		if (l_pFilename && strlen(l_pFilename) > 4)
		{
			char *l_pTemp = _strdup(l_pFilename);
			CString l_Temp;
			l_pTemp[strlen(l_pTemp)-4] = NULL;

			l_Temp.PutAsciiString(l_pTemp);

			l_Filename += l_Temp;
			l_Filename += L".dae";

			l_FilenameSubset += l_Temp;
			l_FilenameSubset += L"_Subset.dae";
		}

		prop.PutParameterValue(L"FilenameSubset", l_FilenameSubset);
		prop.PutParameterValue(L"FilenameAll", l_Filename);

		//Redraw the PPG to show the new combo items
		ctx.PutAttribute(L"Refresh",true);
	}

	return status;
}

/****************************************************************************************
XSI plugin export CreateOptions Command definition functions
****************************************************************************************/
_XSI_EXTERN_ CStatus CreateCryExportCrosswalkOptions_Init( const CRef& in_context )
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
	args.Add( L"arg1", L"CryExportCrosswalkOptions\0" );

	return status;
}

_XSI_EXTERN_ CStatus CreateCryExportCrosswalkOptions_Execute( CRef& in_context )
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
		argOptionName = L"CryExportCrosswalkOptions";
	}

	if (!argObjectName.IsEmpty())
	{
		X3DObject  l_XSIObject = app.GetActiveSceneRoot().FindChild(argObjectName, CString(), CStringArray() );

		if(l_XSIObject.IsValid())
		{
			l_Property = l_XSIObject.AddProperty( L"CryExportCrosswalkOptions" ) ;
		}
	}
	else
	{
		l_Property = app.GetActiveSceneRoot().AddProperty( L"CryExportCrosswalkOptions" ) ;
	}

	if (l_Property.IsValid())
	{
		l_Property.PutName(argOptionName);
	}

	CValue l_Value(l_Property);
	ctxt.PutAttribute( L"ReturnValue", l_Value );

	return status;
}
