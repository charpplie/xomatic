//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "MaxSceneProperties.h"

MaxSceneProperties::MaxSceneProperties(Interface* pMaxInterface)
:	pMaxInterface(pMaxInterface)
{
}

MaxSceneProperties::~MaxSceneProperties()
{
}

int MaxSceneProperties::PropertyCount()
{
	return this->pMaxInterface->GetNumProperties(PROPSET_USERDEFINED);
}

void MaxSceneProperties::GetProperty(int i, std::string& sName, std::string& sValue)
{
	const PROPSPEC*		pPropSpec	= this->pMaxInterface->GetPropertySpec(PROPSET_USERDEFINED, i);
	const PROPVARIANT*	pPropVar	= this->pMaxInterface->GetPropertyVariant(PROPSET_USERDEFINED, i);

	if (pPropSpec->ulKind == PRSPEC_LPWSTR) //we support only the string
	{
		char szBuffer[1024];
		WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR, pPropSpec->lpwstr, -1, szBuffer, sizeof(szBuffer), 0, 0);
		sName = szBuffer;
		this->VariantToString(pPropVar, szBuffer, sizeof(szBuffer));
		sValue = sName;
	}
	else
	{
		sName = "<<ERROR>>";
		sValue = "Invalid Property Type";
	}
}

void MaxSceneProperties::VariantToString(const PROPVARIANT* pProp, TCHAR* szString, int bufSize)
{
	switch (pProp->vt) 
	{
	case VT_LPWSTR:
		WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR, pProp->pwszVal, -1, szString, bufSize, 0, 0);
		break;
	case VT_LPSTR:
		_tcscpy(szString, TSTR(pProp->pszVal));
		break;
	case VT_I4:
		_stprintf(szString, "%ld", pProp->lVal);
		break;
	case VT_R4:
		_stprintf(szString, "%f", pProp->fltVal);
		break;
	case VT_R8:
		_stprintf(szString, "%lf", pProp->dblVal);
		break;
	case VT_BOOL:
		_stprintf(szString, "%d", pProp->boolVal ? 1  : 0);
		break;
	case VT_FILETIME:
		SYSTEMTIME sysTime;
		FileTimeToSystemTime(&pProp->filetime, &sysTime);
		GetDateFormat(LOCALE_SYSTEM_DEFAULT, DATE_SHORTDATE, &sysTime, NULL, szString, bufSize);
		break;
	default:
		_tcscpy(szString, "");	
		break;
	}
}
