#include "stdafx.h"
#include "plugin_stub.h"

#include "crynodeproperties.h"

_XSI_EXTERN_ CStatus CreateCryJointNode_Execute( CRef& in_ref )
{
	CStatus status = CStatus::OK;
	Application app;

	Null l_Null;
	if ((status = app.GetActiveProject().GetActiveScene().GetRoot().AddNull(L"CryJointNode", l_Null)) == CStatus::Fail)
	{
		app.LogMessage(L"Failed to add a new CryJointNode!", siErrorMsg);
		return status;
	}

	l_Null.PutParameterValue(L"primary_icon", (LONG)4);
	l_Null.GetParameter(L"primary_icon").PutCapabilityFlag(siReadOnly, TRUE);
	l_Null.GetParameter(L"size").PutCapabilityFlag(siReadOnly, TRUE);

	l_Null.PutParameterValue(L"shadow_icon", (LONG)9);
	l_Null.GetParameter(L"shadow_icon").PutCapabilityFlag(siReadOnly, TRUE);
	//	l_Null.PutParameterValue(L"rotx", -90.0f, 0);

	status = AddCryNodeProperty( l_Null );
	if (status == CStatus::Fail)
		XSI::COMMANDS::DeleteObj(l_Null.GetFullName());

	return status;
}
