#include "CharacterToolSystem.h"
#include "EntryList.h"
#include "ExplorerFileList.h"
#include "SkeletonContent.h"
#include "SkeletonList.h"

namespace CharacterTool
{

void SkeletonContent::Serialize(IArchive& ar)
{
	ar(skeletonParameters.includes, "includes", "+Includes");
	if (ar.IsEdit() && ar.IsOutput())
	{
		System* system = ar.FindContext<System>();
		UpdateIncludedAnimationSet(system->skeletonList.get());
		ar(includedAnimationSetFilter, "includedAnimationSetFilter", "+!Included Animation Set Filter");
	}
	ar(skeletonParameters.animationSetFilter, "animationSetFilter", "+[+]Animation Set Filter");
	ar(ResourceFilePath(skeletonParameters.animationEventDatabase, "Animation Events File (.animevents)|*.animevents"), "animationEventDatabase", "<Events");

	ar(ResourceFolderPath(skeletonParameters.dbaPath, "Animations"), "dbaPath", "<DBA Path");
	ar(skeletonParameters.individualDBAs, "individualDBAs", "Individual DBAs");
}

void SkeletonContent::GetDependencies(vector<string>* deps) const
{
	for (size_t i = 0; i < skeletonParameters.includes.size(); ++i)
		deps->push_back(skeletonParameters.includes[i].filename);		
}

static void ExpandIncludes(AnimationSetFilter* outFilter, const std::vector<string>& includeStack, const vector<SkeletonParametersInclude>& includes, ExplorerFileList* skeletonList)
{
	std::vector<string> stack = includeStack;
	std::vector<AnimationFilterFolder> includedFolders;
	for (size_t i = 0; i < includes.size(); ++i)
	{
		const string& filename = includes[i].filename;
		if (stl::find(includeStack, filename))
		{
			CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "Recursive inclusion of CHRAPARAMS: '%s'", includes[i].filename.c_str());
			continue;
		}

		SEntry<SkeletonContent>* entry = skeletonList->GetEntryByPath<SkeletonContent>(filename.c_str());
		if (entry)
		{
			skeletonList->LoadOrGetChangedEntry(entry->id);

			includedFolders.insert(includedFolders.end(), 
				entry->content.skeletonParameters.animationSetFilter.folders.begin(), 
				entry->content.skeletonParameters.animationSetFilter.folders.end());

			AnimationSetFilter filter;
			stack.push_back(filename);
			ExpandIncludes(&filter, stack, entry->content.skeletonParameters.includes, skeletonList);
			stack.pop_back();
			includedFolders.insert(includedFolders.end(),
														 filter.folders.begin(), filter.folders.end());
		}
	}

	outFilter->folders.insert(outFilter->folders.begin(),
														includedFolders.begin(), includedFolders.end());
}

void SkeletonContent::UpdateIncludedAnimationSet(ExplorerFileList* skeletonList)
{
	includedAnimationSetFilter = AnimationSetFilter();
	
	ExpandIncludes(&includedAnimationSetFilter, std::vector<string>(), skeletonParameters.includes, skeletonList);
}

void SkeletonContent::ComposeCompleteAnimationSetFilter(AnimationSetFilter* outFilter, ExplorerFileList* skeletonList) const
{
	*outFilter = skeletonParameters.animationSetFilter;

	ExpandIncludes(&*outFilter, vector<string>(), skeletonParameters.includes, skeletonList);
}


}
