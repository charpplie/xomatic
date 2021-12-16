#pragma once

#include <Cry_Math.h>
#include "AnimEvent.h"
#include "Shared/AnimSettings.h"
#include "BlendSpace.h"

struct IDefaultSkeleton;
struct IAnimationSet;

namespace CharacterTool {

struct AnimationContent
{
	enum Type {
		ANIMATION,
		BLEND_SPACE,
		COMBINED_BLEND_SPACE,
		AIMPOSE,
		LOOKPOSE,
		ANM
	};

	enum EImportState
	{
		NOT_SET,
		NEW_ANIMATION,
		WAITING_FOR_CHRPARAMS_RELOAD,
		IMPORTED,
	};

	Type type;
	
	EImportState importState;
	int size;
	bool loadedInEngine;
	bool loadedAsAdditive;
	int animationId;
	SAnimSettings settings;
	BlendSpace blendSpace;
	CombinedBlendSpace combinedBlendSpace;
	string newAnimationSkeleton;
	std::vector<AnimEvent> events;

	AnimationContent();

	void ApplyToCharacter(bool* triggerPreview, ICharacterInstance* characterInstance, const char* animationPath);
	void UpdateBlendSpaceMotionParameters(IAnimationSet* animationSet, IDefaultSkeleton* defaultSkeleton);

	void Serialize(Serialization::IArchive& ar);

	bool HasAudioEvents() const;
};

}
