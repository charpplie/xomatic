#ifndef __PROPERTYGRIDANCESTORITEM_H_
#define __PROPERTYGRIDANCESTORITEM_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "PropertyGridStringItem.h"

namespace CryGame
{
	class CPropertyGridAncestorItem : public CPropertyGridStringItem
	{
		DECLARE_SERIAL(CPropertyGridAncestorItem)

	public:
		virtual void OnInit() override;
		virtual void UpdateText() override;

	private:
		void HandleActorStateDescs(std::vector<string>& list);
	};
}
#endif  // __PROPERTYGRIDANCESTORITEM_H_