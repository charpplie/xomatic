#include "StdAfx.h"
#include "AssetTaggingImpl.h"
#include "AssetTagging.h"

CAssetTaggingImpl::CAssetTaggingImpl(void) :
m_initialized(FALSE),
m_ref(0)
{
}

CAssetTaggingImpl::~CAssetTaggingImpl(void)
{
	if ( m_initialized )
		AssetTagging_CloseConnection();

	m_initialized = false;
}

bool CAssetTaggingImpl::Initialize(const char * localpath)
{
	if ( AssetTagging_Initialize(localpath) )
		m_initialized = true;
	return m_initialized;
}

bool CAssetTaggingImpl::IsLocal()
{
	if ( !m_initialized )
		return true;

	return AssetTagging_IsLocal();
}

int CAssetTaggingImpl::CreateTag(const char * tag, const char * category)
{
	if ( !m_initialized )
		return 0;

	return AssetTagging_CreateTag(tag, category, GetProjectName());
}

int CAssetTaggingImpl::CreateAsset(const char * path, const char * project)
{
	if ( !m_initialized )
		return 0;

	return AssetTagging_CreateAsset(path, project);
}

int CAssetTaggingImpl::CreateProject(const char * project)
{
	if ( !m_initialized )
		return 0;

	return AssetTagging_CreateProject(project);
}

void CAssetTaggingImpl::AddAssetsToTag(const char * tag, const char * category, const char * project, char ** assets, int nAssets)
{
	if ( !m_initialized )
		return;

	AssetTagging_AddAssetsToTag(tag, category, GetProjectName(), assets, nAssets);
}

void CAssetTaggingImpl::RemoveAssetsFromTag(const char * tag, const char * category, const char * project, char ** assets, int nAssets)
{
	if ( !m_initialized )
		return;

	AssetTagging_RemoveAssetsFromTag(tag, category, GetProjectName(),assets,nAssets);
}

void CAssetTaggingImpl::RemoveTagFromAsset(const char * tag, const char * category, const char * project, const char * asset)
{
	if ( !m_initialized )
		return;

	AssetTagging_RemoveTagFromAsset(tag, category, project, asset);
}

int CAssetTaggingImpl::GetNumTagsForAsset(const char * asset)
{
	if ( !m_initialized )
		return 0;
	return AssetTagging_GetNumTagsForAsset(asset, GetProjectName());
}

int CAssetTaggingImpl::GetTagsForAsset(char ** tags, int nTags, const char * asset)
{
	if ( !m_initialized )
		return 0;

	return AssetTagging_GetTagsForAsset(tags, nTags, asset, GetProjectName());
}

int CAssetTaggingImpl::GetTagForAssetInCategory(char * tag, const char * asset, const char * category)
{
	if ( !m_initialized )
		return 0;

	return AssetTagging_GetTagForAssetInCategory(tag, asset, category, GetProjectName());
}

int CAssetTaggingImpl::GetNumAssetsForTag(const char * tag)
{
	if ( !m_initialized )
		return 0;

	return AssetTagging_GetNumAssetsForTag(tag, GetProjectName());
}

int CAssetTaggingImpl::GetAssetsForTag(char ** assets, int nAssets, const char * tag)
{
	if ( !m_initialized )
		return 0;

	return AssetTagging_GetAssetsForTag(assets, nAssets, tag, GetProjectName());
}

int CAssetTaggingImpl::GetNumAssetsWithDescription(const char * description)
{
	if ( !m_initialized )
		return 0;

	return AssetTagging_GetNumAssetsWithDescription(description);
}

int CAssetTaggingImpl::GetAssetsWithDescription(char ** assets, int nAssets, const char * description)
{
	if ( !m_initialized )
		return 0;

	return AssetTagging_GetAssetsWithDescription(assets, nAssets, description);
}

int CAssetTaggingImpl::GetNumTags()
{
	if ( !m_initialized )
		return 0;
	return AssetTagging_GetNumTags(GetProjectName());
}

int CAssetTaggingImpl::GetAllTags(char ** tags, int nTags)
{
	if ( !m_initialized )
		return 0;
	return AssetTagging_GetAllTags(tags,nTags, GetProjectName());
}

void CAssetTaggingImpl::DestroyTag(const char * tag)
{
	if ( !m_initialized )
		return;

	AssetTagging_DestroyTag(tag, GetProjectName());
}

int CAssetTaggingImpl::GetNumCategories()
{
	if ( !m_initialized )
		return 0;
	return AssetTagging_GetNumCategories(GetProjectName());
}

int CAssetTaggingImpl::GetAllCategories(char ** categories, int nCategories)
{
	if ( !m_initialized )
		return 0;
	return AssetTagging_GetAllCategories(GetProjectName(), categories, nCategories);
}

int CAssetTaggingImpl::GetNumTagsForCategory(const char * category)
{
	if ( !m_initialized )
		return 0;
	return AssetTagging_GetNumTagsForCategory(category, GetProjectName());
}

int CAssetTaggingImpl::GetTagsForCategory(const char * category, char ** tags, int nTags)
{
	if ( !m_initialized )
		return 0;
	return AssetTagging_GetTagsForCategory(category, GetProjectName(), tags, nTags);
}

int CAssetTaggingImpl::TagExists(const char * tag, const char * category)
{
	if ( !m_initialized )
		return 0;

	return AssetTagging_TagExists(tag, category, GetProjectName());
}

int CAssetTaggingImpl::AssetExists(const char * relpath)
{
	if ( !m_initialized )
		return 0;

	return AssetTagging_AssetExists(relpath,GetProjectName());
}

int CAssetTaggingImpl::ProjectExists(const char* project)
{
	if ( !m_initialized )
		return 0;

	return AssetTagging_ProjectExists(project);
}

int CAssetTaggingImpl::GetErrorString(char * errorString, int nLen)
{
	if ( !m_initialized )
		return 0;
	return AssetTagging_GetErrorString(errorString, nLen);
}

int CAssetTaggingImpl::GetMaxStringLen()
{
	if ( !m_initialized )
		return 0;
	return AssetTagging_MaxStringLen();
}

CString CAssetTaggingImpl::GetProjectName()
{
	ICVar * pCvar = gEnv->pConsole->GetCVar("sys_game_folder");
	if ( pCvar && pCvar->GetString())
		return CString(pCvar->GetString());

	return CString("unknown");
}

bool CAssetTaggingImpl::GetAssetDescription(const char* relpath, char * description, int nChars)
{
	if ( !m_initialized )
		return 0;

	return AssetTagging_GetAssetDescription(relpath, GetProjectName(), description, nChars);
}

void CAssetTaggingImpl::SetAssetDescription(const char* relpath, const char * project, const char * description)
{
	if ( !m_initialized )
		return;

	return AssetTagging_SetAssetDescription(relpath,project,description);
}

bool CAssetTaggingImpl::AutoCompleteDescription(const char * partdesc, char * description, int nChars)
{
	if ( !m_initialized )
		return false;
	return AssetTagging_AutoCompleteDescription(partdesc,description,nChars);
}