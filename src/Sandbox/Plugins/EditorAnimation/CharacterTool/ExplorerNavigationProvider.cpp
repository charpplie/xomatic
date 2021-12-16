#include "Explorer.h"
#include "CharacterDocument.h"
#include "ExplorerFileList.h"
#include "Serialization/Decorators/INavigationProvider.h"
#include "IResourceSelectorHost.h"
#include "CharacterToolSystem.h"
#include "SceneContent.h"
#include "AnimationList.h"
#include <IEditor.h>

namespace CharacterTool
{

class ExplorerNavigationProvider : public Serialization::INavigationProvider
{
public:
	ExplorerNavigationProvider(System* system)
	: m_system(system)
	{}

	static EExplorerEntryType ReferenceTypeToExplorerType(const char* type)
	{
		return strcmp(type, "Character") == 0 ? ENTRY_CHARACTER :
				   strcmp(type, "Animation") == 0 ? ENTRY_ANIMATION :
				   strcmp(type, "AnimationAlias") == 0 ? ENTRY_ANIMATION :
				   strcmp(type, "Skeleton") == 0 ? ENTRY_SKELETON :
				   strcmp(type, "SkeletonParams") == 0 ? ENTRY_SKELETON :
				   strcmp(type, "CharacterPhysics") == 0 ? ENTRY_PHYSICS :
				   strcmp(type, "CharacterRig") == 0 ? ENTRY_RIG :
					 ENTRY_NONE;
	}

	bool IsRegistered(const char* typeName) const override
	{
		return ReferenceTypeToExplorerType(typeName) != ENTRY_NONE;
	}

	ExplorerEntry* FindEntry(const char* type, const char* path) const
	{
		ExplorerEntry* entry = 0;
		EExplorerSubtree subtree = ReferenceTypeToExplorerSubtree(type);
		if (subtree == SUBTREE_ANIMATIONS && strcmp(type, "AnimationAlias") == 0)
		{
			unsigned int entryId = m_system->animationList->FindIdByAlias(path);
			if (entryId)
				entry = m_system->explorer->FindEntryById(SUBTREE_ANIMATIONS, entryId);
		}
		else
		{
			entry = m_system->explorer->FindEntryByPath(subtree, path);
		}

		if (!entry)
		{
			string canonicalPath = m_system->explorer->GetCanonicalPath(subtree, path);
			if (!canonicalPath.empty())
				entry = m_system->explorer->FindEntryByPath(subtree, canonicalPath.c_str());
		}
		return entry;
	}

	bool CanSelect(const char* type, const char* path, int index) const override
	{ 
		EExplorerEntryType entryType = ReferenceTypeToExplorerType(type);
		EExplorerSubtree subtree = ReferenceTypeToExplorerSubtree(type);
		if (entryType != ENTRY_NONE && subtree != NUM_SUBTREES)
		{
			if (index >= 0)
				return true;
		}
		return FindEntry(type, path) != 0;
	}

	static EExplorerSubtree ReferenceTypeToExplorerSubtree(const char* type)
	{
		return strcmp(type, "Character") == 0 ? SUBTREE_CHARACTERS :
				   strcmp(type, "Animation") == 0 ? SUBTREE_ANIMATIONS :
				   strcmp(type, "AnimationAlias") == 0 ? SUBTREE_ANIMATIONS :
				   strcmp(type, "Skeleton") == 0 ? SUBTREE_SKELETONS :
				   strcmp(type, "SkeletonParams") == 0 ? SUBTREE_SKELETONS :
				   strcmp(type, "CharacterPhysics") == 0 ? SUBTREE_PHYSICS :
				   strcmp(type, "CharacterRig") == 0 ? SUBTREE_RIGS :
					 NUM_SUBTREES;
	}

	const char* GetIcon(const char* type, const char* path) const override
	{
		ExplorerEntry* entry = path && path[0] ? FindEntry(type, path) : 0;
		return Explorer::IconForEntry(ReferenceTypeToExplorerType(type), entry);
	}

	static const char* GetMaskForType(const char* type) 
	{
		return strcmp(type, "Character") == 0 ? "Character Definition (cdf)|*.cdf" :
					 strcmp(type, "Animation") == 0 ? "Animation, BlendSpace (i_caf, bspace, comb)|*.i_caf;*.bspace;*.comb" :
					 strcmp(type, "Skeleton") == 0 ? "Skeleton (skel, chr, cga)|*.skel;*.chr;*.cga" :
					 strcmp(type, "SkeletonParams") == 0 ? "Skeleton Parameters (chrparams)|*.chrparams" :
					 strcmp(type, "CharacterPhysics") == 0 ? "Character Physics (phys)|*.phys" :
					 strcmp(type, "CharacterRig") == 0 ? "Character Rig (rig)|*.rig" :
					 "";
	}

	const char* GetFileSelectorMaskForType(const char* type) const override { return GetMaskForType(type); }

	const char* GetEngineTypeForInputType(const char* extension) const override
	{
		return strcmp(extension, "i_caf") == 0 ? "caf" : extension;
	}

	bool IsActive(const char* type, const char* path, int index) const override
	{
		if (strcmp(type, "Animation") == 0 && index != -1) 
			return m_system->scene->layers.activeLayer == index;
		return false;
	}

	bool IsSelected(const char* type, const char* path, int index) const override
	{
		if (!m_system->document->HasSelectedExplorerEntries())
			if (strcmp(type, "Animation") == 0 && index != -1) 
				if (m_system->scene->layers.activeLayer == index)
					return true;
		ExplorerEntry* entry = FindEntry(type, path);
		if (!entry)
			return false;
		return m_system->document->IsExplorerEntrySelected(entry);;
	}

	bool IsModified(const char* type, const char* path, int index) const override
	{
		ExplorerEntry* entry = m_system->explorer->FindEntryByPath(ReferenceTypeToExplorerSubtree(type), path);
		if (!entry)
			return false;
		return entry->modified;
	}

	bool Select(const char* type, const char* path, int index) const override
	{
		if (strcmp(type, "Animation") == 0)
		{
			if (index != -1)
				m_system->scene->layers.activeLayer = index;
			m_system->scene->PlaybackLayersChanged(false);
			m_system->scene->SignalChanged(false);
		}

		ExplorerEntry* entry = FindEntry(type, path);
		if (!entry)
		{
			m_system->document->SetSelectedExplorerEntries(ExplorerEntries(), 0);
			return false;
		}

		ExplorerEntries entries(1, entry);
		m_system->document->SetSelectedExplorerEntries(entries, 0);
		return true;
	}

	bool CanPickFile(const char* type, int index) const override
	{
		if (strcmp(type, "Animation") == 0 && index >= 0)
			return false;
		if (strcmp(type, "AnimationAlias") == 0 && index >= 0)
			return false;
		return true;
	}

	bool CanCreate(const char* type, int index) const override
	{
		return strcmp(type, "CharacterPhysics") == 0 || strcmp(type, "CharacterRig") == 0;
	}

	bool Create(const char* type, const char* path, int index) const override
	{
		if (strcmp(type, "CharacterPhysics") == 0)
			return m_system->physicsList->AddAndSaveEntry(path);
		else if (strcmp(type, "CharacterRig") == 0)
			return m_system->rigList->AddAndSaveEntry(path);
		return false;
	}

private:
	System* m_system;
};

Serialization::INavigationProvider* CreateExplorerNavigationProvider(System* system)
{
	return new ExplorerNavigationProvider(system);
}

dll_string FileSelector(const SResourceSelectorContext& x, const char* previousValue)
{
	string filter = ExplorerNavigationProvider::GetMaskForType(x.typeName);
	size_t filterLen = filter.size();
	if (filterLen < 2 || filter[filterLen-1] != '|' || filter[filterLen-2] != '|')
		filter += "||";
	string filename = GetIEditor()->SelectFile(filter.c_str(), "", previousValue);
	if (!filename.empty())
		return filename.c_str();
	else
		return previousValue;
}
REGISTER_RESOURCE_SELECTOR("Animation", FileSelector, "Editor/Icons/animation/animation.png")
REGISTER_RESOURCE_SELECTOR("Skeleton", FileSelector, "Editor/Icons/animation/skeleton.png")
REGISTER_RESOURCE_SELECTOR("SkeletonParams", FileSelector, "Editor/Icons/animation/skeleton.png")
REGISTER_RESOURCE_SELECTOR("Character", FileSelector, "Editor/Icons/animation/character.png")
REGISTER_RESOURCE_SELECTOR("CharacterRig", FileSelector, "Editor/Icons/animation/rig.png")
REGISTER_RESOURCE_SELECTOR("CharacterPhysics", FileSelector, "Editor/Icons/animation/physics.png")

}
