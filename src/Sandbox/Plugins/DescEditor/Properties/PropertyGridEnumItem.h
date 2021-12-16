#ifndef __PROPERTYGRIDENUMITEM_H_
#define __PROPERTYGRIDENUMITEM_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "PropertyGridItem.h"

namespace CryGame
{
	//////////////////////////////////////////////////////////////////////////
	// Enum
	//////////////////////////////////////////////////////////////////////////

	class CPropertyGridEnumItem : public CPropertyGridItem
	{
	public:
		CPropertyGridEnumItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, bool isElement, const char* name);

		virtual void OnInit() override;

		virtual void SetValueFromText(const string& text) override;
		virtual void ToString(string& out) override;

	private:
		string GetEnumString(IProperty* pProperty, int index);

	private:
		int m_CharsToTrim;
	};
}

#endif  // __PROPERTYGRIDENUMITEM_H_