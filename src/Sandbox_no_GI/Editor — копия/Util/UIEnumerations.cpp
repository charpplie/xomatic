/*************************************************************************
Crytek Source File.
Copyright (C), Crytek Studios, 2001-2004.
-------------------------------------------------------------------------
$Id: GeneralProxy.cpp  ,v 1.1 2009/01/05 17:48:02 PauloZaffari Exp wwwrun $
$DateTime$
Description:  This file implements the container for the assotiaon of 
enumeration name to enumeration values.
-------------------------------------------------------------------------
History:
- 05:01:2009   17:48 : Created by Paulo Zaffari

*************************************************************************/
#include "StdAfx.h"
#include "UIEnumerations.h"

//////////////////////////////////////////////////////////////////////////
CUIEnumerations& CUIEnumerations::GetUIEnumerationsInstance()
{
	static CUIEnumerations	oGeneralProxy;
	return oGeneralProxy;
}
//////////////////////////////////////////////////////////////////////////
CUIEnumerations::TDSelectedAnimations&	CUIEnumerations::GetSelectedAnimations()
{
	static TDSelectedAnimations	cSelectedAnimations;
	return cSelectedAnimations;
}
//////////////////////////////////////////////////////////////////////////
CUIEnumerations::TDValuesContainer&	CUIEnumerations::GetStandardNameContainer()
{
	static TDValuesContainer	cValuesContainer;
	static bool								boInit(false);

	if (!boInit)
	{
		boInit=true;

		XmlNodeRef							oRootNode;
		XmlNodeRef							oEnumaration;
		XmlNodeRef							oEnumerationItem;

		int											nNumberOfEnumarations(0);
		int											nCurrentEnumaration(0);

		int											nNumberOfEnumerationItems(0);
		int											nCurrentEnumarationItem(0);

		oRootNode=GetISystem()->GetXmlUtils()->LoadXmlFile("Editor\\PropertyEnumerations.xml");
		nNumberOfEnumarations=oRootNode ? oRootNode->getChildCount():0;

		for (nCurrentEnumaration=0;nCurrentEnumaration<nNumberOfEnumarations;++nCurrentEnumaration)
		{
			TDValues	cValues;
			oEnumaration=oRootNode->getChild(nCurrentEnumaration);

			nNumberOfEnumerationItems=oEnumaration->getChildCount();
			for (nCurrentEnumarationItem=0;nCurrentEnumarationItem<nNumberOfEnumerationItems;++nCurrentEnumarationItem)
			{
				oEnumerationItem=oEnumaration->getChild(nCurrentEnumarationItem);

				const char*	szKey(NULL);
				const char*	szValue(NULL);
				oEnumerationItem->getAttributeByIndex(0,&szKey,&szValue);

				cValues.push_back(szValue);
			}

			const char*	szKey(NULL);
			const char*	szValue(NULL);
			oEnumaration->getAttributeByIndex(0,&szKey,&szValue);

			cValuesContainer.insert(TDValuesContainer::value_type(szValue,cValues));
		}
	}

	return cValuesContainer;
}
//////////////////////////////////////////////////////////////////////////
