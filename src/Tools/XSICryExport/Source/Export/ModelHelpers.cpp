#include "StdAfx.h"
#include "ModelHelpers.h"
#include "UserPropertyHelpers.h"
#include <xsi_application.h>
#include <xsi_model.h>
#include <xsi_kinematics.h>
#include <cctype>

string ModelHelpers::GetModelColladaName(XSI::X3DObject& object)
{
	string colladaName = object.GetName().GetAsciiString();
	UserPropertyHelpers::UserPropertyReader propertyReader(object);
	string nameOverride = propertyReader.GetProperty<string>("name");
	if (!nameOverride.empty())
		colladaName = nameOverride;
	return colladaName;
}
