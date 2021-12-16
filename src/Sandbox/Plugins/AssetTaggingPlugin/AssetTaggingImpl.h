#ifndef __AssetTaggingImpl_h__
#define __AssetTaggingImpl_h__

#pragma once
#include "IAssetTagging.h"
#include "IEditorClassFactory.h"

class CAssetTaggingImpl : public IAssetTagging, public IClassDesc
{
public:
	CAssetTaggingImpl(void);
	virtual ~CAssetTaggingImpl(void);
	
	bool Initialize(const char * localpath);
	bool IsLocal();
		
	int CreateTag(const char * tag, const char * category);
	int CreateAsset(const char * path, const char * project);
	int CreateProject(const char * project);
	
	void AddAssetsToTag(const char * tag, const char * category, const char * project, char ** assets, int nAssets);
	void RemoveAssetsFromTag(const char * tag, const char * category, const char * project, char ** assets, int nAssets);
	void RemoveTagFromAsset(const char * tag, const char * category, const char * project, const char * asset);

	int GetNumTagsForAsset(const char * asset);
	int GetTagsForAsset(char ** tags, int nTags, const char * asset);

	int GetTagForAssetInCategory(char * tag, const char * asset, const char * category);
	
	int GetNumAssetsForTag(const char * tag);
	int GetAssetsForTag(char ** assets, int nAssets, const char * tag);
	int GetNumAssetsWithDescription(const char * description);
	int GetAssetsWithDescription(char ** assets, int nAssets, const char * description);

	int GetNumTags();
	int GetAllTags(char ** tags, int nTags);
	void DestroyTag(const char * tag);
	
	int GetNumCategories();
	int GetAllCategories(char ** categories, int nCategories);

	int GetNumTagsForCategory(const char * category);
	int GetTagsForCategory(const char * category, char ** tags, int nTags);

	int TagExists(const char * tag, const char * category);
	int AssetExists(const char * relpath);
	int ProjectExists(const char* project);
	
	int GetErrorString(char * errorString, int nLen);
	int GetMaxStringLen();

	CString GetProjectName();
	bool GetAssetDescription(const char* relpath, char * description, int nChars);
	void SetAssetDescription(const char* relpath, const char * project, const char * description);
	bool AutoCompleteDescription(const char * partdesc, char * description, int nChars);
		
	// from IClassDesc
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_ASSET_TAGGING; };
	REFGUID ClassID()
	{
		// {3D534CCD-747D-4065-B336-846C07861235}
		static const GUID guid = { 0x3d534ccd, 0x747d, 0x4065, { 0xb3, 0x36, 0x84, 0x6c, 0x7, 0x86, 0x12, 0x35 } };
		return guid;
	}
	virtual const char* ClassName() { return "Asset Tagging"; };
	virtual const char* Category() { return "AssetTagging"; };
	virtual CRuntimeClass* GetRuntimeClass(){return 0;};
	virtual void ShowAbout() {};

	// from IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface( const IID &riid, void **ppvObj ) 
	{ 
		if(riid == __uuidof(IAssetTagging))
		{
			*ppvObj = this;
			return S_OK;
		}
		return E_NOINTERFACE ; 
	}
	ULONG STDMETHODCALLTYPE AddRef() { return ++m_ref; };
	ULONG STDMETHODCALLTYPE Release() 
	{ 
		if((--m_ref) == 0)
		{
			delete this;
			return 0; 
		}
		else
			return m_ref;
	}

private:
	bool m_initialized;
	ULONG m_ref;
};


#endif //__AssetTaggingImpl_h__