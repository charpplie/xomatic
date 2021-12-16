#ifndef __PROPERTYGRIDBOOLITEM_H_
#define __PROPERTYGRIDBOOLITEM_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "PropertyGridItem.h"

namespace CryGame
{
	//////////////////////////////////////////////////////////////////////////
	// Bool
	//////////////////////////////////////////////////////////////////////////

	class CPropertyGridBoolItem : public CPropertyGridItem
	{
		DECLARE_SERIAL(CPropertyGridBoolItem)

	public:
		CPropertyGridBoolItem() {}
		CPropertyGridBoolItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, bool isElement, const char* name);

		virtual void OnInit() override;

		virtual void SetValueFromText(const string& text) override;
		virtual void ToString(string& out) override;
	};
}

#endif  // __PROPERTYGRIDBOOLITEM_H_
