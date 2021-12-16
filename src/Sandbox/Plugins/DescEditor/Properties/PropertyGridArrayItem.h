#ifndef __PROPERTYGRIDARRAYITEM_H_
#define __PROPERTYGRIDARRAYITEM_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "PropertyGridItem.h"

namespace CryGame
{
	//////////////////////////////////////////////////////////////////////////
	// Array
	//////////////////////////////////////////////////////////////////////////

	class CPropertyGridArrayItem : public CPropertyGridItem
	{
	public:
		CPropertyGridArrayItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, const char* name);

		virtual void OnInit() override;

		virtual void ToString(string& out) override;

		void AddElement();
		void AddElement(Value& value);
		void RemoveElement(CPropertyGridItem* pPropertyItem);

	protected:
		void UpdateChildCaptions(int nIndex);

		virtual void Reset() override;

		// Sub menu
		virtual bool BuildMenu(CMenu& menu) override;
		virtual void HandleMenuSelection(int nSelection) override;
	};
}

#endif  // __PROPERTYGRIDARRAYITEM_H_