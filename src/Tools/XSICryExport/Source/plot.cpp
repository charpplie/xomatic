
#include "stdafx.h"
#include "plugin_stub.h"

#include <commdlg.h>

void Plot(CString &in_Name, float in_StartFrame, float in_EndFrame, float in_Step)
{
	Application app;
	Selection l_Selection( app.GetSelection() );

	LONG l_lCount = l_Selection.GetCount();
	if (!l_lCount)
	{
		Application app;
		UIToolkit l_UIToolkit = app.GetUIToolkit();
		LONG l_Result;
		l_UIToolkit.MsgBox(L"No object(s) selected!", siMsgOkOnly, L"CryExport", l_Result);
		return;
	}

	CString l_ModelList = L"";
	CString l_InputObjList = L"";
	CString l_Types[3] = {L"pos", L"rot", L"scl"};
	CString l_Axis[3] = {L"x", L"y", L"z"};
	for (LONG i = 0; i < l_lCount; i++)
	{
		SIObject l_Obj(l_Selection[i]);

		if (i != 0)
		{
			l_ModelList += L",";
		}
		l_ModelList += l_Obj.GetFullName();

		for (LONG j = 0; j < 3; j++)
		{
			for (LONG k = 0; k < 3; k++)
			{
				l_InputObjList += l_Obj.GetFullName();
				l_InputObjList += L".kine.local.";
				l_InputObjList += l_Types[j];
				l_InputObjList += l_Axis[k];
				l_InputObjList += L",";
			}
		}
	}
	XSI::COMMANDS::PlotAndApplyAction(l_ModelList, l_InputObjList, in_Name, in_StartFrame, in_EndFrame, in_Step, 20, 1, L"False", L"", L"True", L"False", L"False", L"False", L"");
}

_XSI_EXTERN_ CStatus CreateCryPlot_Execute( CRef& in_ref )
{
	CStatus status = CStatus::OK;
	Application app;

	Property prop = app.GetActiveSceneRoot().GetProperties().GetItem( L"CryPlotUI" ) ;

	if (!prop.IsValid())
	{
		prop = app.GetActiveSceneRoot().AddProperty( L"CryPlotUI" ) ;
	}

	CValue retval = XSI::COMMANDS::InspectObj(L"CryPlotUI",CValue(),L"CryPlot",(LONG)siModal,false);
	if (!retval)
	{
		CString l_ActionName = prop.GetParameterValue(L"ActionName");
		float l_StartFrame = prop.GetParameterValue(L"StartFrame");
		float l_EndFrame = prop.GetParameterValue(L"EndFrame");
		float l_Step = prop.GetParameterValue(L"StepValue");

		Plot(l_ActionName, l_StartFrame, l_EndFrame, l_Step);
	}

	return status;
}

void SetPlotObjects(X3DObject &in_Object, CStringArray &in_AutoPlotObjects, bool in_Add)
{
	CRefArray l_Children = in_Object.GetChildren();
	for (LONG j = 0; j < l_Children.GetCount(); j++)
	{
		X3DObject l_Obj(l_Children[j]);
		CString l_ObjName = l_Obj.GetFullName();
		
		X3DObject l_Child(l_Children[j]);
		if (l_Obj.IsNodeAnimated( siAnySource ) || in_Add )
		{
			if (l_Obj.GetType() != L"Texture Support")
			{
				in_AutoPlotObjects.Add(l_ObjName);
				SetPlotObjects(l_Child, in_AutoPlotObjects, true);
			}
		}
		else
		{
			SetPlotObjects(l_Child, in_AutoPlotObjects, false);
		}
	}
}

void AutoPlot(X3DObject &in_CryExportNode, CString &in_Name, float in_StartFrame, float in_EndFrame, float in_Step)
{
	Application app;

	CStringArray l_AutoPlotObjects;
	X3DObject l_Object(in_CryExportNode);
	SetPlotObjects(l_Object, l_AutoPlotObjects, false);

	CString l_ModelList = L"";
	CString l_InputObjList = L"";
	CString l_Types[3] = {L"pos", L"rot", L"scl"};
	CString l_Axis[3] = {L"x", L"y", L"z"};
	for (LONG i = 0; i < l_AutoPlotObjects.GetCount(); i++)
	{
		CString l_ObjName(l_AutoPlotObjects[i]);

		if (i != 0)
		{
			l_ModelList += L",";
		}
		l_ModelList += l_ObjName;

		for (LONG j = 0; j < 3; j++)
		{
			for (LONG k = 0; k < 3; k++)
			{
				l_InputObjList += l_ObjName;
				l_InputObjList += L".kine.local.";
				l_InputObjList += l_Types[j];
				l_InputObjList += l_Axis[k];
				l_InputObjList += L",";
			}
		}
	}
	XSI::COMMANDS::PlotAndApplyAction(l_ModelList, l_InputObjList, in_Name, in_StartFrame, in_EndFrame, in_Step, 20, 1, L"False", L"", L"True", L"False", L"False", L"False", L"");
}

/****************************************************************************************
CryPlotUI
****************************************************************************************/
_XSI_EXTERN_ CStatus CryPlotUI_Define( const CRef & in_Ctx )
{
	CStatus status = CStatus::OK;
	Application app ;
	CustomProperty prop = Context(in_Ctx).GetSource() ;
	Parameter param ;

	// Default capabilities for most of these parameters
	int caps = siPersistable;
	CValue dft ;	// Used for arguments we don't want to set

	prop.AddParameter( L"ActionName",CValue::siString, caps,
		L"ActionName", L"",
		dft, param ) ;

	prop.AddParameter( L"StartFrame",CValue::siFloat, caps,
		L"StartFrame", L"",
		1.0f, param ) ;

	prop.AddParameter( L"EndFrame",CValue::siFloat, caps,
		L"EndFrame", L"",
		1.0f, param ) ;

	prop.AddParameter( L"StepValue",CValue::siFloat, caps,
		L"Step Value", L"",
		1.0f, param ) ;

	return status;
}

CStatus CryPlotUI_RebuildLayout(PPGLayout &in_Layout, CustomProperty &in_CustomProp)
{
	CStatus status = CStatus::OK;

	PPGItem item ;

	in_Layout.Clear();	

	in_Layout.AddRow();
	in_Layout.AddItem(L"ActionName", L"Action Name", L"Action Name");
	in_Layout.EndRow();

	in_Layout.AddRow();

	in_Layout.AddGroup( L"Time span options" );
	in_Layout.AddItem(L"StartFrame", L"StartFrame", L"StartFrame");
	in_Layout.AddItem(L"EndFrame", L"End Frame", L"End Frame");
	in_Layout.AddItem(L"StepValue", L"Step Value", L"Step Value");
	in_Layout.EndGroup();
	in_Layout.EndRow();

// 	in_Layout.AddRow();
// 	item = in_Layout.AddButton(L"Ok", L"Ok");
// 	in_Layout.EndRow();

	return status;
}

_XSI_EXTERN_ CStatus CryPlotUI_DefineLayout( const CRef & in_Ctx )
{
	CStatus status = CStatus::OK;

	// XSI will call this to define the visual appearance of the CustomProperty
	// The layout is shared between all instances of the CustomProperty
	// and is cached.  You can force the code to re-execute by using the 
	// XSIUtils.Refresh feature.

	PPGLayout oLayout = Context( in_Ctx ).GetSource() ;

	CustomProperty cusProp;
	status = CryPlotUI_RebuildLayout(oLayout, cusProp);

	return status;
}

_XSI_EXTERN_ CStatus CryPlotUI_PPGEvent( const CRef& io_Ctx )
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

		if ( paramName == L"ActionName" )
		{
		}
	}
	if ( eventID == PPGEventContext::siButtonClicked )
	{
		CustomProperty prop = ctx.GetSource();

		CValue l_ButtonName = ctx.GetAttribute( L"Button" );

// 		if (l_ButtonName.GetAsText() == L"Ok")
// 		{
// 			app.LogMessage(L"CryPlot OK button!");
// 
// 			CString l_ActionName = prop.GetParameterValue(L"ActionName");
// 			float l_StartFrame = prop.GetParameterValue(L"StartFrame");
// 			float l_EndFrame = prop.GetParameterValue(L"EndFrame");
// 			float l_Step = prop.GetParameterValue(L"StepValue");
// 			Plot(l_ActionName, l_StartFrame, l_EndFrame, l_Step);
// 		}
	}
	if( eventID == PPGEventContext::siOnInit )
	{
		CustomProperty prop = ctx.GetSource();
		PPGLayout oPPGLayout = prop.GetPPGLayout();		
		CryPlotUI_RebuildLayout(oPPGLayout, prop);

		//Redraw the PPG to show the new combo items
		ctx.PutAttribute(L"Refresh",true);
	}

	return status;
}
