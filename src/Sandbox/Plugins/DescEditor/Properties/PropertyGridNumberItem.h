#ifndef __PROPERTYGRIDNUMBERITEM_H_
#define __PROPERTYGRIDNUMBERITEM_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "PropertyGridItem.h"

namespace CryGame
{
	//////////////////////////////////////////////////////////////////////////
	// Number
	//////////////////////////////////////////////////////////////////////////

	template<typename T, int TPropertyType>
	class CPropertyGridNumberItem : public CPropertyGridItem
	{
	public:
		CPropertyGridNumberItem() {}
		CPropertyGridNumberItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, bool isElement, const char* name)
			: CPropertyGridItem(pGrid, pParent, pObject, pProperty, isElement, name)
		{}

		virtual int GetPropertyType() const override { return TPropertyType; }

		virtual void SetValueFromText(const string& text) override
		{
			T value = (T)0;
			if (GetNumericalValue(text, value) == true)
			{
				SetPropertyValue(value);
			}
		}

		virtual void ToString(string& out) override
		{
			Value value;
			GetPropertyValue(value);

			switch (GetPropertyType())
			{
			case eVType_Float:
				out.Format("%0.2f", (float)value);
				break;
			case eVType_UInt:
				out.Format("%u", (uint32)value);
				break;
			case eVType_Int:
				out.Format("%d", (int32)value);
				break;
			default:
				CRY_ASSERT_MESSAGE(0, "Unhandled type!");
			}
		}

		bool GetNumericalValue(const char* value, T& out)
		{
			char* pEnd;
			int nType = GetPropertyType();
			switch (nType)
			{
			case eVType_Float:
				out = (T)strtod(value, &pEnd);
				break;
			case eVType_UInt:
				out = (T)strtoul(value, &pEnd, 0);
				break;
			case eVType_Int:
				out = (T)strtol(value, &pEnd, 0);
				break;
			default:
				CRY_ASSERT_MESSAGE(0, "Unhandled type!");
			}

			return (*pEnd == 0 || *pEnd == ' ');
		}

		bool GetNumericalValue(T& out)
		{
			string temp((LPCSTR)GetValue());
			return GetNumericalValue(temp, out);
		}
	};
	//////////////////////////////////////////////////////////////////////////
	// Float, Int, UInt
	//////////////////////////////////////////////////////////////////////////
	class CPropertyGridFloatItem : public CPropertyGridNumberItem<float, eVType_Float>
	{
		DECLARE_SERIAL(CPropertyGridFloatItem)

	public:
		CPropertyGridFloatItem() {}
		CPropertyGridFloatItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, bool isElement, const char* name)
			: CPropertyGridNumberItem<float, eVType_Float>(pGrid, pParent, pObject, pProperty, isElement, name)
		{}
	};

	class CPropertyGridInt32Item : public CPropertyGridNumberItem<int32, eVType_Int>
	{
		DECLARE_SERIAL(CPropertyGridInt32Item)

	public:
		CPropertyGridInt32Item() {}
		CPropertyGridInt32Item(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, bool isElement, const char* name)
			: CPropertyGridNumberItem<int32, eVType_Int>(pGrid, pParent, pObject, pProperty, isElement, name)
		{}
	};

	class CPropertyGridUInt32Item : public CPropertyGridNumberItem<uint32, eVType_UInt>
	{
		DECLARE_SERIAL(CPropertyGridUInt32Item)

	public:
		CPropertyGridUInt32Item() {}
		CPropertyGridUInt32Item(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, bool isElement, const char* name)
			: CPropertyGridNumberItem<uint32, eVType_UInt>(pGrid, pParent, pObject, pProperty, isElement, name)
		{}
	};

}

#endif //  __PROPERTYGRIDNUMBERITEM_H_
