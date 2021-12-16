#pragma once

#include "SkeletonParameters.h"

namespace CharacterTool
{

class ExplorerFileList;

struct SkeletonContent
{
	SkeletonParameters skeletonParameters;

	AnimationSetFilter includedAnimationSetFilter;

	void UpdateIncludedAnimationSet(ExplorerFileList* skeletonList);
	void ComposeCompleteAnimationSetFilter(AnimationSetFilter* outFilter, ExplorerFileList* skeletonList) const;

	void GetDependencies(vector<string>* paths) const;
	void Serialize(Serialization::IArchive& ar);
};

}
