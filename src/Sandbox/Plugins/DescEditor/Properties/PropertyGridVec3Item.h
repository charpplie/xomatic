#ifndef __PROPERTYGRIDVEC3ITEM_H_
#define __PROPERTYGRIDVEC3ITEM_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "PropertyGridItem.h"
#include "PropertyGridNumberItem.h"

namespace CryGame
{
	//////////////////////////////////////////////////////////////////////////
	// Vec3
	//////////////////////////////////////////////////////////////////////////

	template<class ComponentItem>
	class CPropertyGridVec3Item : public CPropertyGridItem
	{
	public:
		CPropertyGridVec3Item() {}
		CPropertyGridVec3Item(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, bool isElement, const char* name)
			: CPropertyGridItem(pGrid, pParent, pObject, pProperty, isElement, name)
		{
		}

		virtual void OnInit() override
		{
			static const char* COMPONENT_NAMES[] = {"x", "y", "z"};

			// NOTE: This only works so nicely because Vec3 has the [] operator, allowing IProperty::GetElementAt() to retieve the value as expected.
			//       In other cases, the grid, object, and property should be NULL and they should not be set as elements and a custom class override
			//       should be created for the components.
			for (int i = 0; i < 3; ++i)
			{
				m_componentProperty[i] = new ComponentItem(m_pOwnerGrid, this, COMPONENT_NAMES[i]);
			}

			SetReadOnly();  // Disable editing
		}

		virtual void OnChildValueChanged(CPropertyGridItem* pChild) override
		{
			ComponentItem::ComponentType value;
			for (int i = 0; i < 3; ++i)
				m_componentProperty[i]->GetNumericalValue(value[i]);
			SetPropertyValue(value);

			CPropertyGridItem::OnValueChanged("");
		}

		virtual void Reset() override
		{
			// Reset components
			for (int i = 0; i < 3; ++i)
				m_componentProperty[i]->Reset();
		}

	private:
		ComponentItem* m_componentProperty[3];
	};

	//////////////////////////////////////////////////////////////////////////////////////

	class CPropertyGridVec3fComponentItem : public CPropertyGridFloatItem
	{
	public:
		CPropertyGridVec3fComponentItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, const char* name);

		virtual void SetPropertyValue(const Value& value) override;
		virtual void GetPropertyValue(Value& value) override;

		typedef Vec3 ComponentType;
	};

	class CPropertyGridVec3fItem : public CPropertyGridVec3Item<CPropertyGridVec3fComponentItem>
	{
		DECLARE_SERIAL(CPropertyGridVec3fItem)

	public:
		virtual void ToString(string& out) override
		{
			Value value;
			GetPropertyValue(value);

			Vec3 v = value;
			out.Format("{%f, %f, %f}", v.x, v.y, v.z);
		}
	};

	//////////////////////////////////////////////////////////////////////////////////////

	class CPropertyGridVec3iComponentItem : public CPropertyGridInt32Item
	{
	public:
		CPropertyGridVec3iComponentItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, const char* name);

		virtual void SetPropertyValue(const Value& value) override;
		virtual void GetPropertyValue(Value& value) override;

		typedef Vec3i ComponentType;
	};

	class CPropertyGridVec3iItem : public CPropertyGridVec3Item<CPropertyGridVec3iComponentItem>
	{
		DECLARE_SERIAL(CPropertyGridVec3iItem)

	public:
		virtual void ToString(string& out) override
		{
			Value value;
			GetPropertyValue(value);

			Vec3i v = value;
			out.Format("{%d, %d, %d}", v.x, v.y, v.z);
		}
	};
}

#endif  // __PROPERTYGRIDVEC3ITEM_H_