#include "pch.h"
#include "ClassProfile.h"

#include "PropertyGridDescItem.h"
#include "PropertyGridAncestorItem.h"

// Editor
#include <Util\EditorUtils.h>

// GameDll
#include <..\..\Game_Hunt\GameDll\Game_P1\Core\IGameInterface.h>
#include <..\..\Game_Hunt\GameDll\Game_P1\Core\IObjectDesc.h>

using namespace CryGame;

// Profile types
static const string STR_DEFAULT("Default");
static const string STR_CLASS_PROFILE("CClassProfile");
static const string STR_PROPERTY_PROFILE("CPropertyProfile");
// Profiles path
#define PROFILES_XML_PATH "Editor/Profiles.xml"

CClassProfileManager* CClassProfileManager::sm_pInstance = NULL;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CPropertyProfile::CPropertyProfile(const string& name, CClassProfile* pParentClassProfile)
	: m_Name(name)
	, m_nIndex(-1)
	, m_pParentClassProfile(pParentClassProfile)
{}

void CPropertyProfile::FromXML(XmlNodeRef xmlNode)
{
	if (xmlNode == NULL)
		return;  // Invalid xml node

	CString strTemp;

	// Annotation
	if (xmlNode->getAttr("Description", strTemp))
	{
		m_Description = (LPCSTR)strTemp;
	}

	// Editor Class
	if (xmlNode->getAttr("EditorClass", strTemp))
	{
		m_EditorClass = (LPCSTR)strTemp;
	}

	// Editor Class Variation
	if (xmlNode->getAttr("EditorClassVariation", strTemp))
	{
		m_EditorClassVariation = (LPCSTR)strTemp;
	}

	// Group
	if (xmlNode->getAttr("Group", strTemp))
	{
		m_Group = (LPCSTR)strTemp;
	}

	// Index relative to parent
	xmlNode->getAttr("Index", m_nIndex);
}

void CPropertyProfile::ToXML(XmlNodeRef xmlParent)
{
	if (m_dirty)
	{
		CClassProfileManager* pProfileManger = CClassProfileManager::GetInstance();
		pProfileManger->SendProfileEvent(ePType_Profile, pProfileManger, this);
		m_dirty = false;
	}

	XmlNodeRef xmlNode = XmlHelpers::CreateXmlNode(STR_PROPERTY_PROFILE);

	xmlNode->setAttr("Name", CString(m_Name.c_str()));
	if (m_Description.empty() == false)           xmlNode->setAttr("Description", CString(m_Description.c_str()));
	if (m_EditorClass.empty() == false)           xmlNode->setAttr("EditorClass", CString(m_EditorClass.c_str()));
	if (m_EditorClassVariation.empty() == false)  xmlNode->setAttr("EditorClassVariation", CString(m_EditorClassVariation.c_str()));
	if (m_Group.empty() == false)                 xmlNode->setAttr("Group", CString(m_Group.c_str()));
	if (m_nIndex >= 0)                            xmlNode->setAttr("Index", m_nIndex);

	if (xmlNode->getNumAttributes() > 1)
	{
		// If more than just the name was written out, add the child node
		xmlParent->addChild(xmlNode);
	}
}

void CPropertyProfile::SetIndex(int nIndex)
{
	m_nIndex = nIndex;
	m_pParentClassProfile->MakeLatest(this);

	SetDirty();
}

void CPropertyProfile::SetDirty()
{
	m_dirty = true;
	m_pParentClassProfile->SetDirty();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CClassProfile::CClassProfile(const string& name)
	: m_name(name)
{}

CClassProfile::~CClassProfile()
{
	// Destroy property profiles
	for (int i = 0; i < m_propertyProfiles.size(); ++i)
	{
		delete m_propertyProfiles[i];
	}
	m_propertyProfiles.clear();
}

CPropertyProfile* CClassProfile::GetPropertyProfile(string name, bool createIfMissing)
{
	CPropertyProfile* pProfile = NULL;

	name.MakeLower();  // Normalize name

	// Check if we already have a profile for this property...
	for (int i = 0; i < m_propertyProfiles.size(); ++i)
	{
		if (m_propertyProfiles[i] != NULL && m_propertyProfiles[i]->GetName() == name)
		{
			// Existing profile found
			pProfile = m_propertyProfiles[i];
			break;
		}
	}

	if (pProfile == NULL && createIfMissing)
	{
		// Existing profile not found; Create it now.
		pProfile = new CPropertyProfile(name, this);
		m_propertyProfiles.push_back(pProfile);
	}

	return pProfile;
}

// Used almost exclusively for descriptions since you want the inherited propertyprofile to store the index, editor, etc.
CPropertyProfile* CClassProfile::GetBasePropertyProfile(string name, bool createIfMissing)
{
	CPropertyProfile* pProfile = NULL;
	CClassProfileManager* pProfileManger = CClassProfileManager::GetInstance();
	IClassRegistry* pClassRegistry = ((CGameInterface*)GetISystem()->GetIGame()->GetGameInterface())->GetClassRegistry();
	if (pClassRegistry && pProfileManger)
	{
		// Find base class of the property "name"
		IClass* pClass = pClassRegistry->GetClass(GetName());
		while (!pProfile && pClass && pClass->GetName() != "CReflectedObject")
		{
			CClassProfile* pParentProfile = pProfileManger->GetClassProfile(pClass->GetName(), createIfMissing);
			if (pParentProfile)
			{
				// Iterate over properties to see if name exists
				CPropertyComponent* properties = ((CPropertyComponent*)pClass->GetComponent(eCCType_Properties));
				for (int i = 0; i < properties->GetPropertyCount(); ++i)
				{
					IProperty* p = properties->GetPropertyAt(i);
					if (p->GetName() == name)
					{
						pProfile = pParentProfile->GetPropertyProfile(name, createIfMissing);
						break;
					}
				}
			}

			pClass = pClass->GetBaseClass();
		}
	}

	if (pProfile == NULL && createIfMissing)
	{
		// Existing profile not found; Create it now.
		pProfile = new CPropertyProfile(name, this);
		m_propertyProfiles.push_back(pProfile);
	}

	return pProfile;
}

void CClassProfile::FromXML(XmlNodeRef xmlNode)
{
	if (xmlNode == NULL)
		return;  // Invalid xml node

	// Class categories ( used to create submenus in large context menus )
	CString category;
	if (xmlNode->getAttr("Category", category))
	{
		m_Category = category;
	}

	// Property Profiles
	int const nCount = xmlNode->getChildCount();
	for (int i = 0; i < nCount; ++i)
	{
		XmlNodeRef xmlChild = xmlNode->getChild(i);
		if (xmlChild == NULL)
			continue;  // Invalid child node

		CString strTemp;
		if (xmlChild->getAttr("Name", strTemp))
		{
			CPropertyProfile* pProfile = GetPropertyProfile(LPCSTR(strTemp), true);
			pProfile->FromXML(xmlChild);
		}
	}
}

void CClassProfile::ToXML(XmlNodeRef xmlParent)
{
	if (m_dirty)
	{
		CClassProfileManager* pProfileManger = CClassProfileManager::GetInstance();
		pProfileManger->SendProfileEvent(ePType_Class, pProfileManger, this);
		m_dirty = false;
	}

	if (m_propertyProfiles.empty() && m_Category.empty())
			return;  // Do not write out empty classes unless they have a category

	XmlNodeRef xmlNode = XmlHelpers::CreateXmlNode(STR_CLASS_PROFILE);

	xmlNode->setAttr("Name", CString(m_name.c_str()));
	xmlNode->setAttr("Category", CString(m_Category.c_str()));

	// Save property profiles
	for (int i = 0; i < m_propertyProfiles.size(); ++i)
	{
		if (m_propertyProfiles[i] != NULL)
		{
			m_propertyProfiles[i]->ToXML(xmlNode);
		}
	}

	if (xmlNode->getChildCount() > 0 || !m_Category.empty())
	{
		xmlParent->addChild(xmlNode);
	}
}

void CClassProfile::MakeLatest(CPropertyProfile* pPropertyProfile)
{
	// Move the profile to the back of the list so on load it is the last to be evaluated
	// overwriting any previous profile modifications

	PropertyProfileVector::iterator iter = std::find(m_propertyProfiles.begin(), m_propertyProfiles.end(), pPropertyProfile);
	if (iter != m_propertyProfiles.end())
	{
		CPropertyProfile* pTemp = (*iter);
		m_propertyProfiles.erase(iter);
		m_propertyProfiles.push_back(pTemp);
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

bool CClassProfileManager::Init()
{
	if (GetFileAttributes(PROFILES_XML_PATH) != INVALID_FILE_ATTRIBUTES)
	{
		XmlNodeRef xmlRoot = XmlHelpers::LoadXmlFromFile(PROFILES_XML_PATH);
		if (xmlRoot != NULL)
		{
			int const nCount = xmlRoot->getChildCount();
			for (int i = 0; i < nCount; ++i)
			{
				XmlNodeRef xmlChild = xmlRoot->getChild(i);
				if (xmlChild == NULL)
					continue;  // Invalid child node

				// Grab name to ensure it doesn't already exist
				// NOTE: All profiles should have a "Name" attribute
				CString strTemp;
				if (xmlChild->getAttr("Name", strTemp))
				{
					if (STR_CLASS_PROFILE.compareNoCase(xmlChild->getTag()) == 0)
					{
						// Class Profile
						CClassProfile* pProfile = GetClassProfile((LPCSTR)strTemp, true);
						pProfile->FromXML(xmlChild);
					}
					// TOOD: Other profile types here
				}
			}
		}
	}

	// Make sure we can find the game dll
	if (!GetISystem()->GetIGame())
	{
		gEnv->pLog->LogError("Unable to find GameDLL.  Aborting ClassProfile loading.");
		return false;
	}

	// Register all desc types
	if (GetISystem()->GetIGame()->GetGameInterface() == NULL)
		return false;

	IClassRegistry* pClassRegistry = ((CGameInterface*)GetISystem()->GetIGame()->GetGameInterface())->GetClassRegistry();
	if (pClassRegistry)
	{
		IClass* pBaseClass = pClassRegistry->GetClass("CObjectDesc");
		if (pBaseClass)
		{
			DynArray<IClass*> subClasses;
			subClasses = pBaseClass->GetSubClasses();
			for (int i = 0; i < subClasses.size(); ++i)
			{
				if (subClasses[i])
				{
					RegisterPropertyEditor(RUNTIME_CLASS(CPropertyGridDescItem), subClasses[i]->GetName(), eVType_String, "Descs");
				}
			}
		}
	}

	return true;
}

void CClassProfileManager::DeInit()
{
	// Destroy profiles
	for (ClassProfileMap::iterator iter = m_classProfiles.begin(); iter != m_classProfiles.end(); ++iter)
	{
		delete iter->second;
	}
	m_classProfiles.clear();

	// Destroy Instance
	SAFE_DELETE(sm_pInstance);
}

bool CClassProfileManager::Save()
{
	XmlNodeRef xmlRoot = XmlHelpers::CreateXmlNode("Profiles");

	// Save profiles
	for (ClassProfileMap::iterator iter = m_classProfiles.begin(); iter != m_classProfiles.end(); ++iter)
	{
		if (iter->second != NULL)
		{
			iter->second->ToXML(xmlRoot);
		}
	}

	XmlHelpers::SaveXmlNode(xmlRoot, PROFILES_XML_PATH);

	return true;
}

CClassProfile* CClassProfileManager::GetClassProfile(string name, bool createIfMissing)
{
	name.MakeLower();  // Normalize name

	CClassProfile** pProfile = &(m_classProfiles[name]);
	if ((*pProfile) == NULL)
	{
		if (createIfMissing)
		{
			(*pProfile) = new CClassProfile(name);
		}
		else
			return NULL;
	}

	return (*pProfile);
}

CPropertyProfile* CClassProfileManager::GetPropertyProfile(const string& className, const string& propertyName, bool createIfMissing)
{
	CClassProfile* pClassProfile = GetClassProfile(className, createIfMissing);
	if (pClassProfile == NULL)
		return NULL;  // Unable to find class profile

	return pClassProfile->GetPropertyProfile(propertyName, createIfMissing);
}

void CClassProfileManager::RegisterPropertyEditor(CRuntimeClass* pEditorClass, const char* name, int nValueType, const char* category /* = "" */)
{
	m_propertyEditors[nValueType].push_back(SPropertyEditorData(name, pEditorClass, nValueType, category));
}

const PropertyEditorDataVector& CClassProfileManager::GetPropertyEditors(int nValueType)
{
	if (nValueType == eVType_String)
	{
		// Create a dynamic list including ancestors
		m_propertyEditorsDynamic = m_propertyEditors;
		std::map<string, IClass*> ancestors;

		GetDescEditor()->GetPropertyAncestors(ancestors);
		for (std::map<string, IClass*>::iterator it = ancestors.begin(); it != ancestors.end(); ++it)
		{
			m_propertyEditorsDynamic[nValueType].push_back(SPropertyEditorData(it->first, RUNTIME_CLASS(CPropertyGridAncestorItem), eVType_String, "Ancestor"));
		}

		return m_propertyEditorsDynamic[nValueType];
	}

	return m_propertyEditors[nValueType];
}

const SPropertyEditorData* const CClassProfileManager::GetDefaultPropertyEditor(int nValueType)
{
	// Find and return "Default" editor
	const PropertyEditorDataVector& editors = m_propertyEditors[nValueType];
	for (int i = 0; i < editors.size(); ++i)
	{
		if (editors[i].Name.compareNoCase(STR_DEFAULT) == 0)
			return &editors[i];
	}

	return NULL;
}

CDescEditor* CClassProfileManager::GetDescEditor() const
{
	// Find the Desc Editor
	if (IClassDesc* pClassDesc = GetIEditor()->GetClassFactory()->FindClass(DESC_EDITOR_TOOL_NAME))
	{
		IViewPaneClass *pViewPaneClass = NULL;
		pClassDesc->QueryInterface( __uuidof(IViewPaneClass),(void**)&pViewPaneClass);
		if (pViewPaneClass)
		{
			if (CDescEditorViewClass* pView = (CDescEditorViewClass*)pViewPaneClass)
			{
				return pView->GetDescEditor();
			}
		}
	}

	return NULL;
}

namespace CryGame
{
	void ExpandCamelCase(string& name)
	{
		// Go through and look for camel case, and break the string up at those points.  If
		// there are two capital letters in a row, don't break it, so we don't split up stuff
		// like "ID".
		for (int i = name.length() - 1; i > 0; --i)
		{
			if (::isalpha(name[i]) && name[i] == ::toupper(name[i]) && name[i-1] == ::tolower(name[i-1]))
				name.insert(i, ' ');
		}
	}
}
