#ifndef __PROPERTYGRIDDESCITEM_H_
#define __PROPERTYGRIDDESCITEM_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "PropertyGridStringItem.h"

namespace CryGame
{
	//////////////////////////////////////////////////////////////////////////
	// Desc
	//////////////////////////////////////////////////////////////////////////

	class CPropertyGridDescItem : public CPropertyGridStringItem
	{
		DECLARE_SERIAL(CPropertyGridDescItem)

	public:
		CPropertyGridDescItem() {}
		CPropertyGridDescItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, bool isElement, const char* name);

		virtual void OnInit() override;
		virtual void UpdateText() override;

	private:
		IClass* GetDescClass();
	};
}

#endif  // __PROPERTYGRIDDESCITEM_H_