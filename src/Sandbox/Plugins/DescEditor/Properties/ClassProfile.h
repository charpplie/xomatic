#ifndef __PROFILES_H_
#define __PROFILES_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include <../DescEditor.h>

namespace CryGame
{
	//////////////////////////////////////////////////////////////////////////
	//
	//////////////////////////////////////////////////////////////////////////
	enum
	{
		ePType_Class,
		ePType_Profile,
	};

	//////////////////////////////////////////////////////////////////////////
	//
	//////////////////////////////////////////////////////////////////////////
#define PROFILE_PROPERTY(_name, _type)																		\
public:                                                                                                     \
	inline const _type& Get##_name() const { return m_##_name; }                                            \
	inline void Set##_name( const _type& value ) { m_##_name = value; SetDirty(); }                         \
protected:                                                                                                  \
	_type m_##_name;                                                                                        \
public:

	//////////////////////////////////////////////////////////////////////////
	//
	//////////////////////////////////////////////////////////////////////////
	class CPropertyProfile
	{
		friend class CClassProfile;

	public:
		void SetIndex(int nIndex);
		int GetIndex() const { return m_nIndex; }

		PROFILE_PROPERTY(Name, string);
		PROFILE_PROPERTY(Description, string);
		PROFILE_PROPERTY(EditorClass, string);
		PROFILE_PROPERTY(EditorClassVariation, string);
		PROFILE_PROPERTY(Group, string);

	private:
		CPropertyProfile();  // ctor
		CPropertyProfile(const string& name, CClassProfile* pParentClassProfile);  // init ctor
		CPropertyProfile(const CPropertyProfile&);  // copy ctor
		~CPropertyProfile() {}  // dtor

		void FromXML(XmlNodeRef xmlNode);
		void ToXML(XmlNodeRef xmlParent);

		void SetDirty();

	private:
		int m_nIndex;

		CClassProfile* m_pParentClassProfile;
		bool m_dirty;
	};

	//////////////////////////////////////////////////////////////////////////
	//
	//////////////////////////////////////////////////////////////////////////
	class CClassProfile
	{
		friend class CClassProfileManager;
		friend class CPropertyProfile;

	public:
		PROFILE_PROPERTY(Category, string)

		const string& GetName() const { return m_name; }
		CPropertyProfile* GetPropertyProfile(string name, bool createIfMissing = true);
		CPropertyProfile* GetPropertyProfile(int nIndex) { return m_propertyProfiles[nIndex]; }
		int GetPropertyProfileCount() const { return m_propertyProfiles.size(); }
		CPropertyProfile* GetBasePropertyProfile(string name, bool createIfMissing);

	private:
		CClassProfile();  // ctor
		CClassProfile(const string& name);  // init ctor
		CClassProfile(const CClassProfile&);  // copy ctor
		~CClassProfile();  // dtor

		void FromXML(XmlNodeRef xmlNode);
		void ToXML(XmlNodeRef xmlParent);

		void MakeLatest(CPropertyProfile* pPropertyProfile);
		void SetDirty() { m_dirty = true; }

	private:
		string m_name;

		typedef std::vector<CPropertyProfile*> PropertyProfileVector;
		PropertyProfileVector m_propertyProfiles;

		bool m_dirty;
	};

	//////////////////////////////////////////////////////////////////////////
	//
	//////////////////////////////////////////////////////////////////////////
	struct SPropertyEditorData
	{
		SPropertyEditorData(const string& name, CRuntimeClass* pEditorClass, int nValueType, const string& category)
			: Name(name)
			, Category(category)
			, EditorClass(pEditorClass)
			, ValueType(nValueType)
		{}

		string Name;
		string Category;
		CRuntimeClass* EditorClass;
		int ValueType;
	};
	typedef std::vector<SPropertyEditorData> PropertyEditorDataVector;

	//////////////////////////////////////////////////////////////////////////
	//
	//////////////////////////////////////////////////////////////////////////
	DECLARE_DISPATCHER(Profile, CClassProfileManager, void)
	class CClassProfileManager : public IProfileDispatcher
	{
	public:
		bool Init();
		void DeInit();

		bool Save();

		CClassProfile* GetClassProfile(string name, bool createIfMissing = false);
		CPropertyProfile* GetPropertyProfile(const string& className, const string& propertyName, bool createIfMissing = false);

		void RegisterPropertyEditor(CRuntimeClass* pEditorClass, const char* name, int nValueType, const char* category = "");
		const PropertyEditorDataVector& GetPropertyEditors(int nValueType);
		const SPropertyEditorData* const GetDefaultPropertyEditor(int nValueType);

		static CClassProfileManager* GetInstance()
		{
			if (sm_pInstance == NULL)
			{
				sm_pInstance = new CClassProfileManager();
			}

			return sm_pInstance;
		}

		CDescEditor* GetDescEditor() const;

	private:
		typedef std::map<string, CClassProfile*> ClassProfileMap;
		ClassProfileMap m_classProfiles;

		typedef std::map<int, PropertyEditorDataVector> TypeToPropertyEditorMap;
		TypeToPropertyEditorMap m_propertyEditors;
		TypeToPropertyEditorMap m_propertyEditorsDynamic;

		static CClassProfileManager* sm_pInstance;
	};

	//////////////////////////////////////////////////////////////////////////
	//
	//////////////////////////////////////////////////////////////////////////
	struct IEditorPropertyRegistrar
	{
		IEditorPropertyRegistrar(CRuntimeClass* pClass, const char* name, int nValueType, const char* category)
		{
			CRY_ASSERT_MESSAGE(pClass, "Unable to register missing editor class!  (Missing DECLARE/IMPLEMENT_DYNCREATE in .h/.cpp?)");
			if (pClass)
			{
				CClassProfileManager::GetInstance()->RegisterPropertyEditor(pClass, name, nValueType, category);
			}
		}
	};

#define REGISTER_PROPERTY_EDITOR(_editorClass, _name, _valueType, _category) \
	IEditorPropertyRegistrar RegisterEditor##_editorClass##_name##_valueType(RUNTIME_CLASS(_editorClass), #_name, _valueType, _category);

	void ExpandCamelCase(string& name);
}

#endif