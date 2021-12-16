#ifndef __PROPERTYGRIDOBJECTITEM_H_
#define __PROPERTYGRIDOBJECTITEM_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "PropertyGridItem.h"

namespace CryGame
{
	//////////////////////////////////////////////////////////////////////////
	// Object
	//////////////////////////////////////////////////////////////////////////

	class CPropertyGridObjectItem : public CPropertyGridItem
	{
	public:
		CPropertyGridObjectItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, bool isElement, const char* name);

		virtual void OnInit() override;

		virtual void OnPropertyValueChanged() override;
		virtual void ToString(string& out) override;

		virtual void Reset() override;

		virtual bool BuildMenu(CMenu& menu) override;
		virtual void HandleMenuSelection(int nSelection) override;
	};
}

#endif  // __PROPERTYGRIDOBJECTITEM_H_