#include "stdafx.h"
#include "plugin_stub.h"

#include "Crosswalk/helper.h"

_XSI_EXTERN_ CStatus CryAnimationClipSelection_Execute( CRef& in_ref )
{
	CStatus status = CStatus::OK;
	Application app;

	Property prop = app.GetActiveSceneRoot().GetProperties().GetItem( L"CryAnimationClipSelectionUI" ) ;

	if (!prop.IsValid())
	{
		prop = app.GetActiveSceneRoot().AddProperty( L"CryAnimationClipSelectionUI" ) ;
	}

	prop.PutParameterValue ( L"Nodes", CValue((LONG)0));

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

	XSI::COMMANDS::InspectObj(L"CryAnimationClipSelectionUI",CValue(),l_wLabel,(LONG)siRecycle ,false);

	return status;
}

_XSI_EXTERN_ CStatus CryAnimationClipSelection_Init( CRef& in_ref )
{
	CStatus status = CStatus::OK;

	Context ctxt = in_ref;
	Menu menu = ctxt.GetSource();

	CStatus st;
	MenuItem item;

	// Here we add our menu item in the export menu
	menu.AddCallbackItem(L"CryAnimationClipSelection", L"CryAnimationClipSelection_Execute", item);

	return status;
}

CValueArray l_ExportList;
CValueArray l_DoNotExportList;
bool l_Changes = false;
LONG l_ActItem = 0;

LONG GetExportListIDByName(CString in_Name)
{
	for (LONG i = 0; i < l_ExportList.GetCount(); i++)
	{
		if (in_Name == l_ExportList[i])
			return i;
	}
	return -1;
}

LONG GetDoNotExportListIDByName(CString in_Name)
{
	for (LONG i = 0; i < l_DoNotExportList.GetCount(); i++)
	{
		if (in_Name == l_DoNotExportList[i])
			return i;
	}
	return -1;
}

void LogExportLists()
{
	Application app;
	app.LogMessage(L"ExportList:");
	for (LONG i = 0; i < l_ExportList.GetCount(); i++)
	{
		app.LogMessage(l_ExportList[i]);
	}
	app.LogMessage(L"DoNotExportList:");
	for (LONG i = 0; i < l_DoNotExportList.GetCount(); i++)
	{
		app.LogMessage(l_DoNotExportList[i]);
	}
}

CStatus FillAnimationLists(CustomProperty &in_CustomProp)
{
	CStatus l_Status = CStatus::OK;

	Application app;

	l_ExportList.Clear();
	l_DoNotExportList.Clear();

	LONG l_lNode = in_CustomProp.GetParameter(L"Nodes").GetValue();

	X3DObject l_X3DObject = GetAllCryExportNodes().GetItem(l_lNode);
	Model l_Model = l_X3DObject.GetModel();
	l_Model = (Model)l_X3DObject;
	CRefArray l_XSISources = l_Model.GetSources();


	int loop;

	for (loop = 0; loop < l_XSISources.GetCount(); loop++)
	{
		if (l_XSISources.GetItem(loop).IsA(siActionSourceID))
		{
			ActionSource l_XSIActionSource = (ActionSource)l_XSISources.GetItem(loop);

			CRefArray l_Properties = l_XSIActionSource.GetProperties();

			int loop2;

			bool l_bDoNotExport = false;

			for (loop2 = 0; loop2 < l_Properties.GetCount(); loop2++)
			{
				if (l_Properties.GetItem(loop2).IsA(siCustomPropertyID))
				{
					CustomProperty l_CustomProperty(l_Properties.GetItem(loop2));
					CString l_Name = l_CustomProperty.GetName();

					const char *asciiname = l_Name.GetAsciiString();
					if (asciiname && strstr(asciiname, "DoNotExport"))
					{
						l_bDoNotExport = true;
						break;
					}
				}
			}

			if (l_bDoNotExport)
			{
				l_DoNotExportList.Add(CValue(l_XSIActionSource.GetName()));
				l_DoNotExportList.Add(CValue(l_XSIActionSource.GetName()));
			}
			else
			{
				l_ExportList.Add(CValue(l_XSIActionSource.GetName()));
				l_ExportList.Add(CValue(l_XSIActionSource.GetName()));
			}
		}
	}

	return l_Status;
}

CStatus ApplyAnimations(CustomProperty &in_CustomProp)
{
	CStatus l_Status = CStatus::OK;

	LONG l_lNode = in_CustomProp.GetParameter(L"Nodes").GetValue();

	X3DObject l_X3DObject = GetAllCryExportNodes().GetItem(l_lNode);
	Model l_Model = l_X3DObject.GetModel();
	l_Model = (Model)l_X3DObject;
	CRefArray l_XSISources = l_Model.GetSources();

	int loop;
	for (loop = 0; loop < l_XSISources.GetCount(); loop++)
	{
		if (l_XSISources.GetItem(loop).IsA(siActionSourceID))
		{
			ActionSource l_XSIActionSource = (ActionSource)l_XSISources.GetItem(loop);

			CRefArray l_Properties = l_XSIActionSource.GetProperties();

			bool l_bDoNotExport = false;
			int loop2;
			for (loop2 = 0; loop2 < l_Properties.GetCount(); loop2++)
			{
				if (l_Properties.GetItem(loop2).IsA(siCustomPropertyID))
				{
					CustomProperty l_CustomProperty(l_Properties.GetItem(loop2));
					CString l_Name = l_CustomProperty.GetName();

					const char *asciiname = l_Name.GetAsciiString();
					if (asciiname && strstr(asciiname, "DoNotExport"))
					{
						l_bDoNotExport = true;
						if (GetExportListIDByName(l_XSIActionSource.GetName()) != -1)
						{
							//							l_Properties.Remove(l_CustomProperty);
							XSI::COMMANDS::DeleteObj( l_Model.GetName() + L".Mixer." + l_XSIActionSource.GetName() + L"." + l_CustomProperty.GetName() );
						}
					}
				}
			}

			if (GetDoNotExportListIDByName(l_XSIActionSource.GetName()) != -1)
			{
				if( !l_bDoNotExport )
					l_XSIActionSource.AddProperty(CString(L"CustomProperty"), false, L"DoNotExport");
			}
		}
	}

	return l_Status;
}


CStatus MoveToDoNotExport(CustomProperty &in_CustomProp)
{
	CStatus l_Status = CStatus::OK;
	Application app;
	CString l_AnimList = in_CustomProp.GetParameterValue(L"ExportList");
	if (l_AnimList.Length() == 0 || l_AnimList == L"-1")
	{
		return l_Status;
	}
	else
	{
		CStringArray l_SplitList = l_AnimList.Split(L";");

		for (LONG i = 0; i < l_SplitList.GetCount(); i++)
		{
			l_DoNotExportList.Add( l_SplitList[i] );
			l_DoNotExportList.Add( l_SplitList[i] );
		}

		CValueArray l_NewArray;
		l_NewArray.Clear();

		for (LONG i = 0; i < l_ExportList.GetCount(); i++)
		{
			bool l_Found = false;
			for (LONG j = 0; j < l_SplitList.GetCount(); j++)
			{
				if (l_ExportList[i] == l_SplitList[j])
				{
					l_Found = true;
					l_Changes = true;
					break;
				}
			}
			if (!l_Found)
			{
				l_NewArray.Add(l_ExportList[i]);
			}
		}

		l_ExportList.Clear();
		for (LONG i = 0; i < l_NewArray.GetCount(); i++)
		{
			l_ExportList.Add(l_NewArray[i]);
		}
	}

	in_CustomProp.PutParameterValue(L"ExportList", L"");
	in_CustomProp.PutParameterValue(L"DoNotExportList", L"");

	return l_Status;
}

CStatus MoveToDoExport(CustomProperty &in_CustomProp)
{
	CStatus l_Status = CStatus::OK;
	Application app;
	CString l_AnimList = in_CustomProp.GetParameterValue(L"DoNotExportList");
	if (l_AnimList.Length() == 0 || l_AnimList == L"-1")
	{
		return l_Status;
	}
	else
	{
		CStringArray l_SplitList = l_AnimList.Split(L";");

		for (LONG i = 0; i < l_SplitList.GetCount(); i++)
		{
			l_ExportList.Add( l_SplitList[i] );
			l_ExportList.Add( l_SplitList[i] );
		}

		CValueArray l_NewArray;
		l_NewArray.Clear();

		for (LONG i = 0; i < l_DoNotExportList.GetCount(); i++)
		{
			bool l_Found = false;
			for (LONG j = 0; j < l_SplitList.GetCount(); j++)
			{
				if (l_DoNotExportList[i] == l_SplitList[j])
				{
					l_Found = true;
					l_Changes = true;
					break;
				}
			}
			if (!l_Found)
			{
				l_NewArray.Add(l_DoNotExportList[i]);
			}
		}

		l_DoNotExportList.Clear();
		for (LONG i = 0; i < l_NewArray.GetCount(); i++)
		{
			l_DoNotExportList.Add(l_NewArray[i]);
		}
	}

	in_CustomProp.PutParameterValue(L"ExportList", L"");
	in_CustomProp.PutParameterValue(L"DoNotExportList", L"");

	return l_Status;
}

/****************************************************************************************
CryAnimationClipSelectionUI
****************************************************************************************/
_XSI_EXTERN_ CStatus CryAnimationClipSelectionUI_Define( const CRef & in_Ctx )
{
	CStatus status = CStatus::OK;
	Application app ;
	CustomProperty prop = Context(in_Ctx).GetSource() ;
	Parameter param ;

	// Default capabilities for most of these parameters
	int caps = siPersistable;
	CValue dft ;	// Used for arguments we don't want to set


	// CrySpecific: Set list
	dft = (LONG)1;
	prop.AddParameter(	L"ExportList",CValue::siString, caps,
		L"ExportList", L"",
		dft, param ) ;

	prop.AddParameter(	L"DoNotExportList",CValue::siString , caps, 
		L"DoNotExportList", L"", 
		dft, param ) ;

	prop.AddParameter( L"Nodes",CValue::siString, caps, L"Nodes", L"", dft, param );

	FillAnimationLists(prop);

	return status;
}

CStatus CryAnimationClipSelectionUI_RebuildLayout(PPGLayout &in_Layout, CustomProperty &in_CustomProp)
{
	CStatus status = CStatus::OK;

	PPGItem item ;

	in_Layout.Clear();	

	// ================================================================================
	// General 
	// ================================================================================
	in_Layout.AddGroup();
	in_Layout.AddStaticText(L"Animation Clips Export Properties");
	in_Layout.EndGroup(); 

	CRefArray l_Nodes = GetAllCryExportNodes();

	in_Layout.AddRow();
	in_Layout.AddGroup(L"CryExport Nodes") ;
	CValueArray l_Items;
	for (int i = 0; i < l_Nodes.GetCount(); i++)
	{
		X3DObject l_Object(l_Nodes.GetItem(i));

		l_Items.Add(GetCryExportNodeListName(l_Object));
		l_Items.Add(i);
	}

	PPGItem l_Item = in_Layout.AddEnumControl(L"Nodes", l_Items, L"Nodes", siControlCombo);
	in_Layout.EndGroup();
	in_Layout.EndRow();
	in_Layout.AddRow();


	// ================================================================================
	// Export
	// ================================================================================	
	in_Layout.AddGroup(L"Export") ;
	item = in_Layout.AddEnumControl( L"ExportList", l_ExportList, L"", siControlListBox ) ;
	item.PutAttribute( siUICY, (LONG)160 ) ;
	item.PutAttribute( siUIStyle, (LONG)0x10200101 ) ;
	//	item.PutAttribute( siUIStyle, (LONG)0x0101 ) ;
	item.PutAttribute( siUINoLabel, true ) ;
	item.PutAttribute( siUIMultiSelectionListBox, true );
	item.PutAttribute( siUIItems, l_ExportList );
	item = in_Layout.AddButton(L"Move_To_DoNotExport", L">>>");
	in_Layout.EndGroup();

	// ================================================================================
	// Do Not Export
	// ================================================================================	
	in_Layout.AddGroup(L"Do Not Export") ;
	//	item = in_Layout.AddEnumControl( L"DoNotExportList", l_DoNotExportList, L"", siControlListBox ) ;
	item = in_Layout.AddItem( L"DoNotExportList", L"", siControlListBox ) ;
	item.PutUIItems(l_DoNotExportList);
	item.PutAttribute( siUICY, (LONG)160 ) ;
	item.PutAttribute( siUIStyle, (LONG)0x10200101 ) ;
	//	item.PutAttribute( siUIStyle, (LONG)0x0101 ) ;
	item.PutAttribute( siUINoLabel, true ) ;
	item.PutAttribute( siUIMultiSelectionListBox, true );
	item.PutAttribute( siUIItems, l_DoNotExportList );
	item = in_Layout.AddButton(L"Move_To_Export", L"<<<");
	in_Layout.EndGroup();

	//item = in_Layout.AddItem( L"ExportList",L"Export List",siControlListBox ) ;
	//item = in_Layout.AddItem( L"DoNotExportList",L"Do Not Export List",siControlListBox ) ;
	//	item.PutAttribute( siUINoLabel, true ) ;
	in_Layout.EndRow();


	in_Layout.EndGroup();

	in_Layout.AddRow();
	in_Layout.AddGroup(L"");
	in_Layout.AddButton(L"Apply", L"Apply");
	in_Layout.AddButton(L"Cancel", L"Cancel");
	in_Layout.EndGroup();
	in_Layout.EndRow();

	return status;
}

_XSI_EXTERN_ CStatus CryAnimationClipSelectionUI_DefineLayout( const CRef & in_Ctx )
{
	CStatus status = CStatus::OK;

	// XSI will call this to define the visual appearance of the CustomProperty
	// The layout is shared between all instances of the CustomProperty
	// and is cached.  You can force the code to re-execute by using the 
	// XSIUtils.Refresh feature.

	PPGLayout oLayout = Context( in_Ctx ).GetSource() ;

	CustomProperty cusProp;
	status = CryAnimationClipSelectionUI_RebuildLayout(oLayout, cusProp);

	return status;
}

_XSI_EXTERN_ CStatus CryAnimationClipSelectionUI_PPGEvent( const CRef& io_Ctx )
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
		// For this event the Source of the event is the parameter
		// itself
		Parameter changed = ctx.GetSource() ;	

		CustomProperty prop = changed.GetParent() ;	

		CString   paramName = changed.GetScriptName() ; 

		if (paramName == L"Nodes")
		{
			if (l_Changes)
			{
				UIToolkit l_UIToolKit = app.GetUIToolkit();
				LONG l_Result;
				l_UIToolKit.MsgBox(L"Save your selections?", siMsgYesNoCancel, L"CryExport", l_Result);
				if (l_Result == siMsgYes)
				{
					ApplyAnimations(prop);
				}
				if (l_Result == siMsgNo)
				{
				}
				if (l_Result == siMsgCancel)
				{
					prop.GetParameter(L"Nodes").PutValue( CValue(l_ActItem) );
					return status;
				}
				l_Changes = false;
				l_ActItem = prop.GetParameter(L"Nodes").GetValue();
			}
			PPGLayout oPPGLayout = prop.GetPPGLayout();		

			FillAnimationLists(prop);
			CryAnimationClipSelectionUI_RebuildLayout(oPPGLayout, prop);

			//Redraw the PPG to show the new combo items
			ctx.PutAttribute(L"Refresh",true);
		}
	}
	if ( eventID == PPGEventContext::siButtonClicked )
	{
		CustomProperty prop = ctx.GetSource();

		CValue l_ButtonName = ctx.GetAttribute( L"Button" );

		if (l_ButtonName.GetAsText() == L"Move_To_Export")
		{
			PPGLayout oPPGLayout = prop.GetPPGLayout();		
			MoveToDoExport(prop);

			CryAnimationClipSelectionUI_RebuildLayout(oPPGLayout, prop);
			//Redraw the PPG to show the new combo items
			ctx.PutAttribute(L"Refresh",true);
		}
		else if (l_ButtonName.GetAsText() == L"Move_To_DoNotExport")
		{
			PPGLayout oPPGLayout = prop.GetPPGLayout();		

			MoveToDoNotExport(prop);

			CryAnimationClipSelectionUI_RebuildLayout(oPPGLayout, prop);
			//Redraw the PPG to show the new combo items
			ctx.PutAttribute(L"Refresh",true);
		}
		else if (l_ButtonName.GetAsText() == L"Apply")
		{
			//			LogExportLists();
			ApplyAnimations(prop);
		}
		else if (l_ButtonName.GetAsText() == L"Cancel")
		{
			PPGLayout oPPGLayout = prop.GetPPGLayout();		

			FillAnimationLists(prop);
			CryAnimationClipSelectionUI_RebuildLayout(oPPGLayout, prop);

			//Redraw the PPG to show the new combo items
			ctx.PutAttribute(L"Refresh",true);
		}
	}
	if( eventID == PPGEventContext::siOnInit )
	{
		CustomProperty prop = ctx.GetSource();
		PPGLayout oPPGLayout = prop.GetPPGLayout();		
		FillAnimationLists(prop);
		CryAnimationClipSelectionUI_RebuildLayout(oPPGLayout, prop);

		//Redraw the PPG to show the new combo items
		ctx.PutAttribute(L"Refresh",true);
	}

	return status;
}

/****************************************************************************************
CreateCryAnimationClipSelectionUI
****************************************************************************************/
_XSI_EXTERN_ CStatus CreateCryAnimationClipSelectionUI_Init( const CRef& in_context )
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
	args.Add( L"arg1", L"CryAnimationClipSelectionUI\0" );

	return status;
}

_XSI_EXTERN_ CStatus CreateCryAnimationClipSelectionUI_Execute( CRef& in_context )
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
		argOptionName = L"CryAnimationClipSelectionUI";
	}

	if (!argObjectName.IsEmpty())
	{
		X3DObject  l_XSIObject = app.GetActiveSceneRoot().FindChild(argObjectName, CString(), CStringArray() );

		if(l_XSIObject.IsValid())
		{
			l_Property = l_XSIObject.AddProperty( L"CryAnimationClipSelectionUI" ) ;
		}
	}
	else
	{
		l_Property = app.GetActiveSceneRoot().AddProperty( L"CryAnimationClipSelectionUI" ) ;
	}

	if (l_Property.IsValid())
	{
		l_Property.PutName(argOptionName);
	}

	CValue l_Value(l_Property);
	ctxt.PutAttribute( L"ReturnValue", l_Value );

	return status;
}
