#include "stdafx.h"
#include "ResourceSelectorHost.h"

class CResourceSelectorHost : public IResourceSelectorHost
{
public:
	CResourceSelectorHost()
	{
		RegisterModuleResourceSelectors(this);
	}

	dll_string SelectResource(const SResourceSelectorContext& context, const char* previousValue) override
	{
		if (!context.typeName)
		{
			assert(0 && "SResourceSelectorContext::typeName is not specified");
			return dll_string();
		}

		if (!previousValue)
		{
			assert(0 && "previousValue is null");
			return dll_string();
		}

		TTypeMap::iterator it = m_typeMap.find(context.typeName);
		if (it == m_typeMap.end())
		{
			CString str;
			str.Format("No Resource Selector is registered for resource type \"%s\"", context.typeName);
			AfxMessageBox(str);
			return previousValue;
		}

		dll_string result = previousValue;
		if (it->second->function)
			result = it->second->function(context, previousValue);
		else if (it->second->functionWithContext)
			result = it->second->functionWithContext(context, previousValue, context.contextObject);

		return result;
	}

	const char* ResourceIconPath(const char* typeName) const override
	{
		TTypeMap::const_iterator it = m_typeMap.find(typeName);
		if (it != m_typeMap.end())
			return it->second->iconPath;
		return "";
	}
	
	Serialization::TypeID ResourceContextType(const char* typeName) const override
	{
		TTypeMap::const_iterator it = m_typeMap.find(typeName);
		if (it != m_typeMap.end())
			return it->second->contextType;
		return Serialization::TypeID();
	}

	void RegisterResourceSelector(const SStaticResourceSelectorEntry* entry) override
	{
		m_typeMap[entry->typeName] = entry;
	}

private:
	typedef std::map<string, const SStaticResourceSelectorEntry*, stl::less_stricmp<string> > TTypeMap;
	TTypeMap m_typeMap;
};

// ---------------------------------------------------------------------------

IResourceSelectorHost* CreateResourceSelectorHost()
{
	return new CResourceSelectorHost();
}

// ---------------------------------------------------------------------------

dll_string SoundFileSelector(const SResourceSelectorContext& x, const char* previousValue)
{
	// start in Sounds folder if no sound is selected
	CString startPath = Path::GetPath(previousValue);
	if (previousValue[0] == '\0')
		startPath = Path::GetGameFolder()+"/Sounds/";

	CString relativeFilename = previousValue;
	if (CFileUtil::SelectSingleFile(EFILE_TYPE_SOUND, relativeFilename, "", startPath))
		return relativeFilename.GetBuffer();
	else
		return previousValue;
}
REGISTER_RESOURCE_SELECTOR("Sound", SoundFileSelector, "")

// ---------------------------------------------------------------------------
dll_string ModelFileSelector(const SResourceSelectorContext& x, const char* previousValue)
{
	CString relativeFilename = previousValue;
	CString startPath = Path::GetPath(previousValue);
	if (previousValue[0] == '\0')
			startPath = Path::GetGameFolder()+"/Objects/";
	if (CFileUtil::SelectSingleFile(EFILE_TYPE_GEOMETRY, relativeFilename, "", startPath))
		return relativeFilename.GetBuffer();
	else
		return previousValue;
}
REGISTER_RESOURCE_SELECTOR("Model", ModelFileSelector, "")

// ---------------------------------------------------------------------------
dll_string GeomCacheFileSelector(const SResourceSelectorContext& x, const char* previousValue)
{
	CString relativeFilename = previousValue;
	CString startPath = Path::GetPath(previousValue);
	if (previousValue[0] == '\0')
		startPath = Path::GetGameFolder()+"/Objects/";
	if (CFileUtil::SelectSingleFile(EFILE_TYPE_GEOMCACHE, relativeFilename, "", startPath))
		return relativeFilename.GetBuffer();
	else
		return previousValue;
}

REGISTER_RESOURCE_SELECTOR("GeomCache", GeomCacheFileSelector, "")


