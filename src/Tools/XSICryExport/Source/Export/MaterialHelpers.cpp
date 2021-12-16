#include "StdAfx.h"
#include "MaterialHelpers.h"
#include "StringHelpers.h"
#include "plugin_stub.h"
#include <xsi_material.h>

MaterialHelpers::MaterialInfo MaterialHelpers::GetInfo(XSI::Material& material)
{
	MaterialInfo info;

	info.physicalize = "None";
	info.id = -1;

	CRefArray &l_Shaders = material.GetShaders();

	for (LONG k = 0; k < l_Shaders.GetCount(); k++)
	{
		Shader l_Shader(l_Shaders[k]);

		CString l_ProgID = l_Shader.GetProgID();

		const char* l_strProgID = l_ProgID.GetAsciiString();

		if (strstr(l_strProgID, "CryShader.1"))
		{
			LONG const l_Physics = l_Shader.GetParameterValue(L"Physics");

			switch (l_Physics)
			{
			case 1:
				info.physicalize = "Default";
				break;
			case 2:
				info.physicalize = "ProxyNoDraw";
				break;
			case 3:
				info.physicalize = "NoCollide";
				break;
			case 4:
				info.physicalize = "Obstruct";
				break;
			default:
				info.physicalize = "None";
				break;
			}

			LONG const l_MaterialID = l_Shader.GetParameterValue(L"MaterialID");
			info.id = l_MaterialID;

			break;
		}
	}


//	std::string fullName = material.GetFullName().GetAsciiString();
	std::string fullName = material.GetName().GetAsciiString();

	bool valid = true;
	size_t firstNonUnderscore = fullName.find_first_not_of("_");
	valid = valid && firstNonUnderscore != string::npos;
	size_t firstDigit = fullName.find_first_of("0123456789", firstNonUnderscore);
	valid = valid && firstDigit != string::npos;
	size_t underscoreAfterDigits = fullName.find_first_of("_", firstDigit);
	valid = valid && underscoreAfterDigits != string::npos;
	size_t firstNonUnderscoreAfterUnderscore = fullName.find_first_not_of("_", underscoreAfterDigits);
	valid = valid && firstNonUnderscoreAfterUnderscore != string::npos;

	if (valid)
	{
		std::string digitString = StringHelpers::Trim(fullName.substr(firstDigit, underscoreAfterDigits - firstDigit));
		std::string nameString = StringHelpers::Trim(fullName.substr(firstNonUnderscoreAfterUnderscore));

		info.name = nameString;
		info.id = std::strtol(digitString.c_str(), 0, 10) - 1;
	}
	else
	{
		info.name = fullName;
	}

	if ((info.id < 0) || (info.id > 31))
	{
		info.id = 0;
	}

	return info;
}
