#ifndef __PROPERTYGRIDITEM_H_
#define __PROPERTYGRIDITEM_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "PropertyGrid.h"
#include "ClassProfile.h"

namespace CryGame
{
	class CClassProfile;
	class CPropertyProfile;

	//////////////////////////////////////////////////////////////////////////
	// Base
	//////////////////////////////////////////////////////////////////////////

	class CPropertyGridItem : public CXTPPropertyGridItem, public IProfileListener
	{
		DECLARE_DYNAMIC(CPropertyGridItem)

	protected:
		enum
		{
			MENU_ADD = 1,
			MENU_DELETE,
			MENU_RESET,
			MENU_GROUP,
			MENU_UNGROUP,
			MENU_RENAMEGROUP,
			MENU_EDIT_DESCRIPTION,
			MENU_EXPAND_ALL,
			MENU_COLLAPSE_ALL,
			MENU_EDITOR_TYPE,
			MENU_NULL,
			MENU_CLASSES_BEGIN = 100,
			MENU_CLASSES_END = 199,
			MENU_CATEGORIES_BEGIN = 200,
			MENU_CATEGORIES_END = 299,
			MENU_EDITORS_BEGIN = 300,
			MENU_EDITORS_END = 399
		};

	public:
		CPropertyGridItem();
		CPropertyGridItem(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, bool isElement, const char* name, const char* value = NULL);
		virtual ~CPropertyGridItem();

		void OnCreate(CPropertyGrid* pGrid, CXTPPropertyGridItem* pParent, CReflectedObject* pObject, IProperty* pProperty, bool isElement, const char* name, const char* value = NULL, const char* variation = NULL);

		void Init();
		void InitChildren();
		virtual void OnInit() {}
		void Copy(CPropertyGridItem* pOther);

		CPropertyGridItem* GetParent();
		void SetParent(CPropertyGridItem* pParent);

		CReflectedObject* GetObject() const { return m_data.m_pObject; }
		IProperty* GetProperty() const { return m_data.m_pProperty; }
		bool IsElement() { return m_data.m_isElement; }

		CClassProfile* GetClassProfile(bool createIfMissing = false, bool resolveObjects = false) const;
		CPropertyProfile* GetPropertyProfile(bool createIfMissing = false, bool resolveObjects = false) const;
		CPropertyProfile* GetBasePropertyProfile(bool createIfMissing = false) const;

		virtual void OnValueChanged(CString strValue) override;
		virtual void OnChildValueChanged(CPropertyGridItem* pChild) {}
		virtual void SetValueFromText(const string& text) {};
		virtual void ToString(string& out) {};

		bool HasChanged();
		virtual void Reset();
		void Deselect() { OnDeselect(); };

		virtual void SetPropertyValue(const Value& value);
		virtual void OnPropertyValueChanged();
		virtual void GetPropertyValue(Value& value);
		virtual int GetPropertyType() const;

		int GetRelativeIndex();
		int GetPropertyProfileIndex();

		void UpdateDirtyStatus();
		void UpdateDirtyRefCount(int nDelta);
		void UpdateParentDirtyRefCount(int nDelta);
		void UpdateDirtyMetrics(bool dirty);

		// Helper functions for getting a caption for a value or object
		static void GetValueCaption(const Value& value, string& caption);
		static void GetObjectCaption(CReflectedObject* pObject, string& caption);
		static void GetObjectCategory(CReflectedObject* pObject, string& category);

		virtual void ConvertTo(CRuntimeClass* pEditorClass, const char* variation = NULL);

		void SaveState(SPropertyGridItemState& state);
		void RestoreState(const SPropertyGridItemState& state);

		bool IsGroupHeader() const { return m_bIsGroupHeader; }

	protected:
		virtual void UpdateText();
		void UpdateIcon();

		// Events
		void SendEvent(const EPropertyGridEvents& event);

		// Sub menu
		virtual void OnRButtonDown(UINT nFlags, CPoint point) override;
		virtual bool BuildMenu(CMenu& menu);
		void BuildEditorMenu(CMenu& menu, CRuntimeClass* pCurrentEditorClass);
		bool BuildClassMenu(CMenu& menu, CReflectedObject* pObject, const char* szFormat, bool showNULL, bool IsCategories = false);
		void AddMenuSeparator(CMenu& menu);
		virtual void HandleMenuSelection(int nSelection);

		// CClassProfileManager::IProfileListener
		virtual void OnProfileEvent(uint32 event, CClassProfileManager* _dispatcher, void* data) override;
		// ~CClassProfileManager::IProfileListener

	protected:
		CPropertyGrid* m_pOwnerGrid;
		CPropertyGridData m_data;

		// Dirty flags
		bool m_dirty;
		int m_iDirtyRefCount;

		DynArray<IClass*> m_subClasses;  // Cached sub classes for eVType_Objects

		string m_variation;

		bool m_initialized;
		bool m_bIsGroupHeader;
	};

	class CPropertyGridGroupItem : public CPropertyGridItem
	{
	public:
		CPropertyGridGroupItem(CPropertyGrid* pGrid, CPropertyGridItem* pParent, const char* name)
			: CPropertyGridItem(pGrid, NULL, NULL, NULL, false, name)
		{
		}

		virtual void OnInit() override
		{
			m_bIsGroupHeader = true;

			SetReadOnly(TRUE);

			CXTPPropertyGridItemMetrics* metrics = GetMetrics(false);
			metrics->m_clrBack.SetStandardValue(m_pOwnerGrid->GetBorderBgColor());
			metrics->m_clrLine.SetStandardValue(m_pOwnerGrid->GetBorderBgColor());

			metrics = GetValueMetrics();
			metrics->m_clrBack.SetStandardValue(m_pOwnerGrid->GetBorderBgColor());
			metrics->m_clrLine.SetStandardValue(m_pOwnerGrid->GetBorderBgColor());

			metrics = GetCaptionMetrics();

			Expand();

			m_pGrid->Invalidate();
			m_pGrid->UpdateWindow();
		}

	private:
		virtual BOOL OnDrawItemValue(CDC& dc, CRect rc) override
		{
			CRect rcText(rc);
			rcText.left = max(1, GetIndent()) * XTP_PGI_EXPAND_BORDER;

			CBrush b(m_pOwnerGrid->GetBorderBgColor());
			dc.FillRect(rcText, &b);

			return TRUE;
		}

	};
}

#endif //  __PROPERTYGRIDITEM_H_
