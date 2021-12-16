#ifndef __PROPERTYGRIDSTRINGITEM_H_
#define __PROPERTYGRIDSTRINGITEM_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "PropertyGridItem.h"

namespace CryGame
{
	//////////////////////////////////////////////////////////////////////////
	// String
	//////////////////////////////////////////////////////////////////////////

	class CPropertyGridStringItem : public CPropertyGridItem
	{
		DECLARE_SERIAL(CPropertyGridStringItem)

	public:
		CPropertyGridStringItem() {}
		CPropertyGridStringItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, bool isElement, const char* name)
			: CPropertyGridItem(pGrid, pParent, pObject, pProperty, isElement, name)
		{}

		virtual void SetValueFromText(const string& text) override
		{
			SetPropertyValue(text);
		}

		virtual void ToString(string& out)
		{
			Value value;
			GetPropertyValue(value);

			out = (string)value;
		}
	};
}

#endif  // __PROPERTYGRIDSTRINGITEM_H_