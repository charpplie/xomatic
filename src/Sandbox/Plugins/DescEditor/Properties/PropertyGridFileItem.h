#ifndef __PROPERTYGRIDFILEITEM_H_
#define __PROPERTYGRIDFILEITEM_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "PropertyGridStringItem.h"

namespace CryGame
{
	//////////////////////////////////////////////////////////////////////////
	// Desc
	//////////////////////////////////////////////////////////////////////////

	class CPropertyGridFileItem : public CPropertyGridStringItem
	{
		DECLARE_SERIAL(CPropertyGridFileItem)

	public:
		CPropertyGridFileItem();
		CPropertyGridFileItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, bool isElement, const char* name);

		virtual void UpdateText() override;

		virtual void OnInplaceButtonDown(CXTPPropertyGridInplaceButton* /*pButton*/) override;
	};

	class CPropertyGridSoundItem : public CPropertyGridFileItem
	{
		DECLARE_SERIAL(CPropertyGridSoundItem)

	public:
		virtual void UpdateText() override { CPropertyGridItem::UpdateText(); }
		virtual void OnInplaceButtonDown(CXTPPropertyGridInplaceButton* /*pButton*/) override;
	};
}

#endif  // __PROPERTYGRIDFILEITEM_H_