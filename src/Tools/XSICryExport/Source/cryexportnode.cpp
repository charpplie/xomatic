#include "stdafx.h"
#include "plugin_stub.h"


#include <commdlg.h>

#include "check.h"

_XSI_EXTERN_ CStatus CreateCryExportNode_Execute( CRef& in_ref )
{
	CStatus status = CStatus::OK;
	Application app;

	Model l_Model;
	if ((status=app.GetActiveProject().GetActiveScene().GetRoot().AddModel(CRefArray(), L"CryExportNode", l_Model)) == CStatus::Fail)
	{
		app.LogMessage(L"Failed to add a new CryExportNode!", siErrorMsg);
		return status;
	}

	//	l_Model.GetKinematics().GetLocal().SetLock(siLockLevelAll);
	CString l_ModelName = l_Model.GetName();

	XSI::COMMANDS::Lock( l_ModelName + L".kine.local.sclx", siLockLevelAll);
	XSI::COMMANDS::Lock( l_ModelName + L".kine.local.scly", siLockLevelAll);
	XSI::COMMANDS::Lock( l_ModelName + L".kine.local.sclz", siLockLevelAll);
	XSI::COMMANDS::Lock( l_ModelName + L".kine.local.rotx", siLockLevelAll);
	XSI::COMMANDS::Lock( l_ModelName + L".kine.local.roty", siLockLevelAll);
	XSI::COMMANDS::Lock( l_ModelName + L".kine.local.rotz", siLockLevelAll);

	CustomProperty l_CustomProperty;
	l_CustomProperty = l_Model.AddProperty(L"CryExportNodeProperties", false, L"ExportProperties");

	CValueArray args(5);
	args[0] = l_CustomProperty;
	args[2] = L"";
	args[3] = (LONG)siModal;
	args[4] = false;
	CValue retval;
	status = app.ExecuteCommand( L"InspectObj", args, retval );

	CString l_Filename = l_CustomProperty.GetParameterValue(L"Filename");
	FixCryExportNodeName(l_Model);

	if (!retval && l_Filename != L"")
	{
	}
	else
	{
		XSI::COMMANDS::DeleteObj(l_Model.GetFullName());
		app.LogMessage(L"Missing filename! CryExportNode deleted.", siErrorMsg);
	}

	return status;
}

/****************************************************************************************
CryExportNodeProperties
****************************************************************************************/
_XSI_EXTERN_ CStatus CryExportNodeProperties_Define( const CRef & in_Ctx )
{
	CStatus status = CStatus::OK;
	Application app ;
	CustomProperty prop = Context(in_Ctx).GetSource() ;
	Parameter param ;

	// Default capabilities for most of these parameters
	int caps = siPersistable;
	CValue dft ;	// Used for arguments we don't want to set


	Parameter l_Parameter;
	prop.AddParameter(L"Filetype", CValue::siUInt4, 0, L"Filetype", L"", CRY_FILE_TYPE_CGF, l_Parameter);
	prop.AddParameter(L"Filename", CValue::siString, 0, L"Filename", L"", L"", l_Parameter);
	prop.AddParameter(L"Exportable", CValue::siBool, 0, L"", L"", TRUE, l_Parameter);
	prop.AddParameter(L"MergeObjects", CValue::siBool, 0, L"", L"", TRUE, l_Parameter);


	return status;
}

CStatus CryExportNodeProperties_RebuildLayout(PPGLayout &in_Layout, CustomProperty &in_CustomProp)
{
	CStatus status = CStatus::OK;

	PPGItem item ;

	in_Layout.Clear();	

	CValueArray l_Items(6);

	l_Items[0] = L"CGF (Complete CryExportNode)" ; l_Items[1] = CRY_FILE_TYPE_CGF;

	l_Items[2] = L"CGA|ANM (Mesh|Animation Clips)" ; l_Items[3] = CRY_FILE_TYPE_CGA|CRY_FILE_TYPE_ANM;
	l_Items[4] = L"CHR|CAF (Mesh & Skeleton|Animation Clips)" ; l_Items[5] = CRY_FILE_TYPE_CHR|CRY_FILE_TYPE_CAF;

	PPGItem l_Item = in_Layout.AddEnumControl(L"Filetype", l_Items, L"Filetype", siControlCombo);
	in_Layout.AddItem(L"Filename", L"Filename", L"Filename");
	in_Layout.AddItem(L"Exportable", L"Exportable", L"Exportable");
	in_Layout.AddItem(L"MergeObjects", L"MergeObjects", L"MergeObjects");

	return status;
}

_XSI_EXTERN_ CStatus CryExportNodeProperties_DefineLayout( const CRef & in_Ctx )
{
	CStatus status = CStatus::OK;

	// XSI will call this to define the visual appearance of the CustomProperty
	// The layout is shared between all instances of the CustomProperty
	// and is cached.  You can force the code to re-execute by using the 
	// XSIUtils.Refresh feature.

	PPGLayout oLayout = Context( in_Ctx ).GetSource() ;

	CustomProperty cusProp;
	status = CryExportNodeProperties_RebuildLayout(oLayout, cusProp);

	return status;
}

_XSI_EXTERN_ CStatus CryExportNodeProperties_PPGEvent( const CRef& io_Ctx )
{
	CStatus status = CStatus::OK;
	// This callback is called when events happen in the user interface
	// This is where the "logic" code is implemented.

	Application app ;

	PPGEventContext ctx( io_Ctx ) ;

	PPGEventContext::PPGEvent eventID = ctx.GetEventID() ;

	if( eventID == PPGEventContext::siOnInit )
	{
		ctx.PutAttribute(L"Refresh",true);
	}

	else if ( eventID == PPGEventContext::siParameterChange )
	{		
		Parameter changed = ctx.GetSource() ;	
		CustomProperty prop = changed.GetParent() ;	
		CString   paramName = changed.GetScriptName() ; 

		if ( paramName == L"Filename" )
		{
			CString l_Filename = prop.GetParameterValue(L"Filename");
			app.LogMessage(l_Filename);

			X3DObject l_Parent;
			l_Parent = X3DObject(prop.GetParent());

			FixCryExportNodeName(l_Parent);
		}
	}

	return status;
}

_XSI_EXTERN_ CStatus ConvertGeometryToCryExportNode_Execute( CRef& in_ref )
{
	CStatus status = CStatus::OK;
	Application app;

	Selection l_Selection( app.GetSelection() );
	LONG l_lCount = l_Selection.GetCount();
	for (int i = 0; i < l_lCount; i++)
	{
		SIObject l_Object(l_Selection[i]);
		if (l_Object.GetType() != L"polymsh")
			continue;

		Model l_Model;
		if ((status=app.GetActiveProject().GetActiveScene().GetRoot().AddModel(CRefArray(), L"CryExportNode", l_Model)) == CStatus::Fail)
		{
			app.LogMessage(L"Failed to add a new CryExportNode!", siErrorMsg);
			return status;
		}

		//	l_Model.GetKinematics().GetLocal().SetLock(siLockLevelAll);
		CString l_ModelName = l_Model.GetName();

		XSI::COMMANDS::Lock( l_ModelName + L".kine.local.sclx", siLockLevelAll);
		XSI::COMMANDS::Lock( l_ModelName + L".kine.local.scly", siLockLevelAll);
		XSI::COMMANDS::Lock( l_ModelName + L".kine.local.sclz", siLockLevelAll);
		XSI::COMMANDS::Lock( l_ModelName + L".kine.local.rotx", siLockLevelAll);
		XSI::COMMANDS::Lock( l_ModelName + L".kine.local.roty", siLockLevelAll);
		XSI::COMMANDS::Lock( l_ModelName + L".kine.local.rotz", siLockLevelAll);

		CustomProperty l_CustomProperty;
		l_CustomProperty = l_Model.AddProperty(L"CryExportNodeProperties", false, L"ExportProperties");

		l_CustomProperty.PutParameterValue(L"Filename", l_Object.GetName());
		FixCryExportNodeName(l_Model);
		l_ModelName = l_Model.GetName();
		XSI::COMMANDS::Lock( l_ModelName + L".Name", siLockLevelAll);

		XSI::COMMANDS::CopyPaste(l_Object.GetFullName(), L"", l_Model.GetFullName(), 1);
	}

	return status;
}
