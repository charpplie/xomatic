#ifndef __PROPERTYGRIDPATHAGENTITEM_H_
#define __PROPERTYGRIDPATHAGENTITEM_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "PropertyGridStringItem.h"

namespace CryGame
{
	//////////////////////////////////////////////////////////////////////////
	// Desc
	//////////////////////////////////////////////////////////////////////////

	class CPropertyGridPathAgentItem : public CPropertyGridStringItem
	{
		DECLARE_SERIAL(CPropertyGridPathAgentItem)

	public:
		CPropertyGridPathAgentItem() {}
		CPropertyGridPathAgentItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, bool isElement, const char* name)
			: CPropertyGridStringItem(pGrid, pParent, pObject, pProperty, isElement, name)
		{}

		virtual void OnInit() override;

		virtual void SetValueFromText(const string& text) override;
		virtual void ToString(string& out) override;
	};
}

#endif  // __PROPERTYGRIDPATHAGENTITEM_H_