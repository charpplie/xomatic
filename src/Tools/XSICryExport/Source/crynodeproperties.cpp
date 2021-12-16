#include "stdafx.h"
#include "plugin_stub.h"

#include <commdlg.h>

/****************************************************************************************
CryNodeProperties
****************************************************************************************/
CStatus AddCryNodeProperty( SceneItem& in_ref )
{
	CStatus status = CStatus::OK;
	SceneItem l_Model = in_ref;

	bool l_bProp = true;
	CustomProperty l_CustomProperty(l_Model.GetProperties().GetItem(L"JointProperties"));
	if (!l_CustomProperty.IsValid())
	{
		l_bProp = false;
		l_CustomProperty = l_Model.AddProperty(L"CryNodeProperties", false, L"JointProperties");
	}

	l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Entity"));
	l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Box"));
	l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Mass"));
	l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Density"));
	l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Pieces"));
	//	l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Props"));

	l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Limit"));
	l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Bend"));
	l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Twist"));
	l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Pull"));
	l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Push"));
	l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Shift"));

	// 	l_CustomProperty.GetParameter(L"Entity").PutCapabilityFlag(siReadOnly, TRUE);
	// 	l_CustomProperty.GetParameter(L"Box").PutCapabilityFlag(siReadOnly, TRUE);
	// 	l_CustomProperty.GetParameter(L"Mass").PutCapabilityFlag(siReadOnly, TRUE);
	// 	l_CustomProperty.GetParameter(L"Density").PutCapabilityFlag(siReadOnly, TRUE);
	// 	l_CustomProperty.GetParameter(L"Pieces").PutCapabilityFlag(siReadOnly, TRUE);

	Application app;

	CValueArray args(5);
	args[0] = l_CustomProperty;
	args[2] = L"";
	args[3] = (LONG)siModal;
	args[4] = false;
	CValue retval;
	status = app.ExecuteCommand( L"InspectObj", args, retval );

	if (retval && !l_bProp)
	{
		XSI::COMMANDS::DeleteObj(l_CustomProperty.GetFullName());
		status = CStatus::Fail;
	}
	return status;
}

_XSI_EXTERN_ CStatus CreateCryNodeProperty_Execute( CRef& in_ref )
{
	Application app;

	Selection l_Selection( app.GetSelection() );

	LONG l_lCount = l_Selection.GetCount();

	SceneItem l_Model;

	if (l_lCount == 1)
		l_Model = l_Selection[0];
	else
		return CStatus::Fail;

	return AddCryNodeProperty( l_Model );
}

_XSI_EXTERN_ CStatus AddCryObjectProperty_Execute( CRef& in_ref )
{
	CStatus status = CStatus::OK;
	Application app;

	Selection l_Selection( app.GetSelection() );

	LONG l_lCount = l_Selection.GetCount();
	if (l_lCount != 1)
		return status;
	for (LONG i=0; i<l_lCount; i++)
	{
		SceneItem l_Model(l_Selection[i]);

		bool l_bProp = true;
		CustomProperty l_CustomProperty(l_Model.GetProperties().GetItem(L"ObjectProperties"));
		if (!l_CustomProperty.IsValid())
		{
			l_bProp = false;
			l_CustomProperty = l_Model.AddProperty(L"CryNodeProperties", false, L"ObjectProperties");
			l_CustomProperty.PutParameterValue ( L"Pieces", CValue(-1));
		}

		l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Limit"));
		l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Bend"));
		l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Twist"));
		l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Pull"));
		l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Push"));
		l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Shift"));

		l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Entity"));
		l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Box"));
		l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Mass"));
		l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Density"));
		l_CustomProperty.RemoveParameter(l_CustomProperty.GetParameter(L"Pieces"));

		l_CustomProperty.GetParameter(L"Pieces").PutCapabilityFlag(siReadOnly, TRUE);

		//		l_CustomProperty.GetParameter(L"Limit").PutCapabilityFlag(siReadOnly, TRUE);
		// 		l_CustomProperty.GetParameter(L"Bend").PutCapabilityFlag(siReadOnly, TRUE);
		// 		l_CustomProperty.GetParameter(L"Twist").PutCapabilityFlag(siReadOnly, TRUE);
		// 		l_CustomProperty.GetParameter(L"Pull").PutCapabilityFlag(siReadOnly, TRUE);
		// 		l_CustomProperty.GetParameter(L"Push").PutCapabilityFlag(siReadOnly, TRUE);
		// 		l_CustomProperty.GetParameter(L"Shift").PutCapabilityFlag(siReadOnly, TRUE);

		CValueArray args(5);
		args[0] = l_CustomProperty;
		args[2] = L"";
		args[3] = (LONG)siModal;
		args[4] = false;
		CValue retval;
		status = app.ExecuteCommand( L"InspectObj", args, retval );

		if (retval && !l_bProp)
			XSI::COMMANDS::DeleteObj(l_CustomProperty.GetFullName());
	}

	return status;
}

_XSI_EXTERN_ CStatus CryNodeProperties_Define( const CRef & in_Ctx )
{
	CStatus status = CStatus::OK;
	Application app ;
	CustomProperty prop = Context(in_Ctx).GetSource();
	Parameter param ;

	// Default capabilities for most of these parameters
	int caps = siPersistable;
	CValue dft ;	// Used for arguments we don't want to set

	Parameter l_Parameter;
	prop.AddParameter(L"Entity", CValue::siBool, 0, L"", L"", 0, l_Parameter);
	prop.AddParameter(L"Box", CValue::siBool, 0, L"", L"", 0, l_Parameter);
	prop.AddParameter(L"Mass", CValue::siInt4, 0, L"", L"", -1, l_Parameter);
	prop.AddParameter(L"Density", CValue::siInt4, 0, L"", L"", -1, l_Parameter);
	prop.AddParameter(L"Limit", CValue::siInt4, 0, L"", L"", -1, -1, 10000, -1, 10000, l_Parameter);
	prop.AddParameter(L"Bend", CValue::siInt4, 0, L"", L"", -1, l_Parameter);
	prop.AddParameter(L"Twist", CValue::siInt4, 0, L"", L"", -1, l_Parameter);
	prop.AddParameter(L"Pull", CValue::siInt4, 0, L"", L"", -1, l_Parameter);
	prop.AddParameter(L"Push", CValue::siInt4, 0, L"", L"", -1, l_Parameter);
	prop.AddParameter(L"Shift", CValue::siInt4, 0, L"", L"", -1, l_Parameter);
	prop.AddParameter(L"Pieces",CValue::siString, siPersistable, L"Pieces", L"", -1, l_Parameter ) ;

	prop.AddParameter(L"Props",CValue::siString, siPersistable, L"", L"", L"", l_Parameter ) ;

	return status;
}

void GetNodesUnderNode(X3DObject in_Object, CRefArray &out_NodeList)
{
	CRefArray l_Children = in_Object.GetChildren();
	for (LONG j = 0; j < l_Children.GetCount(); j++)
	{
		SIObject l_Obj(l_Children[j]);
		X3DObject l_Child(l_Obj);
		out_NodeList.Add( l_Child );
		GetNodesUnderNode( l_Child, out_NodeList );
	}
}

void GetNodeNamesUnderNode(X3DObject in_Object, CStringArray &out_NodeList)
{
	CRefArray l_Children = in_Object.GetChildren();
	for (LONG j = 0; j < l_Children.GetCount(); j++)
	{
		SIObject l_Obj(l_Children[j]);
		X3DObject l_Child(l_Obj);
		out_NodeList.Add( l_Child.GetName() );
		GetNodeNamesUnderNode( l_Child, out_NodeList );
	}
}

CStatus CryNodeProperties_RebuildLayout(PPGLayout &in_Layout, CustomProperty &in_CustomProp)
{
	CStatus status = CStatus::OK;

	SIObject l_Parent;
	if (in_CustomProp.IsValid())
	{
		l_Parent = SIObject(in_CustomProp.GetParent());
	}

	PPGItem item ;

	in_Layout.Clear();	

	in_Layout.AddItem(L"Entity", L"Entity", L"Entity");
	in_Layout.AddItem(L"Box", L"Box", L"Box");
	in_Layout.AddItem(L"Mass", L"Mass", L"Mass");
	in_Layout.AddItem(L"Density", L"Density", L"Density");
	in_Layout.AddItem(L"Limit", L"Limit", L"Limit");
	in_Layout.AddItem(L"Bend", L"Bend", L"Bend");
	in_Layout.AddItem(L"Twist", L"Twist", L"Twist");
	in_Layout.AddItem(L"Pull", L"Pull", L"Pull");
	in_Layout.AddItem(L"Push", L"Push", L"Push");
	in_Layout.AddItem(L"Shift", L"Shift", L"Shift");

	in_Layout.AddGroup(L"Selected pieces (should be start with '-')") ;

	Application app;

	if (l_Parent.GetFullName() != L"")
	{
		ProjectItem l_Obj(l_Parent);
		X3DObject l_X3DObject(l_Obj);

		CRefArray l_Nodes;
		GetNodesUnderNode( l_X3DObject, l_Nodes );

		CValueArray l_Items;
		if (l_X3DObject.GetName() != L"")
		{
			for (int i = 0; i < l_Nodes.GetCount(); i++)
			{
				SIObject l_Node = l_Nodes.GetItem(i);
				CString l_Nodename = l_Node.GetName();
				const char *l_strNodename = l_Nodename.GetAsciiString();
				if (!l_Node.IsEqualTo(l_Parent) && l_strNodename[0] == '-' && !strstr(l_strNodename, "-LOD"))
				{
					l_Items.Add( l_Node.GetName() );
					l_Items.Add( l_Node.GetName() );
				}
			}
		}
		PPGItem l_Item = in_Layout.AddEnumControl( L"Pieces", l_Items, L"", siControlListBox ) ;
		l_Item.PutAttribute( siUICY, (LONG)160 ) ;
		l_Item.PutAttribute( siUIStyle, (LONG)0x10200101 ) ;
		l_Item.PutAttribute( siUINoLabel, true ) ;
		l_Item.PutAttribute( siUIMultiSelectionListBox, true );
		l_Item.PutAttribute( siUIItems, l_Items );
	}

	in_Layout.EndGroup();

	in_Layout.AddGroup(L"Properties:") ;
	PPGItem l_Item = in_Layout.AddString(L"Props",L" ", true, 200);
	in_Layout.EndGroup();

	return status;
}

_XSI_EXTERN_ CStatus CryNodeProperties_DefineLayout( const CRef & in_Ctx )
{
	CStatus status = CStatus::OK;

	// XSI will call this to define the visual appearance of the CustomProperty
	// The layout is shared between all instances of the CustomProperty
	// and is cached.  You can force the code to re-execute by using the 
	// XSIUtils.Refresh feature.

	PPGLayout oLayout = Context( in_Ctx ).GetSource() ;

	CustomProperty cusProp;
	status = CryNodeProperties_RebuildLayout(oLayout, cusProp);

	return status;
}

_XSI_EXTERN_ CStatus CryNodeProperties_PPGEvent( const CRef& io_Ctx )
{
	CStatus status = CStatus::OK;
	// This callback is called when events happen in the user interface
	// This is where the "logic" code is implemented.

	Application app ;

	PPGEventContext ctx( io_Ctx ) ;

	PPGEventContext::PPGEvent eventID = ctx.GetEventID() ;

	if( eventID == PPGEventContext::siOnInit )
	{
		CustomProperty prop = ctx.GetSource() ;
		PPGLayout oPPGLayout = prop.GetPPGLayout();		
		PPGItem oPPGItem = oPPGLayout.GetItem(L"Mass");

		CryNodeProperties_RebuildLayout(oPPGLayout, prop);

		ctx.PutAttribute(L"Refresh",true);
	}

	return status;
}
