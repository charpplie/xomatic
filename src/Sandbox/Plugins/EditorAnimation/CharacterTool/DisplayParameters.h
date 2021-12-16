#pragma once

#include <Cry_Math.h>
#include "../EditorCommon/QViewportSettings.h"
#include "Serialization.h"

namespace CharacterTool
{

enum CharacterMovement
{
	CHARACTER_MOVEMENT_INPLACE,
	CHARACTER_MOVEMENT_REPEATED,
	CHARACTER_MOVEMENT_CONTINUOUS
};

enum CompressionPreview
{
	COMPRESSION_PREVIEW_COMPRESSED,
	COMPRESSION_PREVIEW_BOTH
};

struct DisplayAnimationOptions
{
	CharacterMovement movement;
	CompressionPreview compressionPreview;
	bool showLocator;
	bool animationEventGizmos;

	DisplayAnimationOptions() 
	: movement(CHARACTER_MOVEMENT_INPLACE)
	, compressionPreview(COMPRESSION_PREVIEW_COMPRESSED)
	, showLocator(false)
	, animationEventGizmos(true)
	{
	}

	void Serialize(IArchive& ar)
	{
		ar(movement, "movement", "Movement");
		ar(compressionPreview, "compressionPreview", "Compression");
		ar(animationEventGizmos, "animationEventGizmos", "Animation Event Gizmos");
	}
};

struct DisplayCharacterOptions
{
	bool attachmentAndPoseModifierGizmos;
	bool bindPose;
	bool showSkeleton;

	bool showDynamicProxies;
	bool showAuxiliaryProxies;
	bool showClothProxies;
	bool showRagdollProxies;

	bool showJointNames;
	bool showSkeletonBoundingBox;
	bool showEdges;
	bool showPhysicalProxies;
	bool showDccToolOrigin;

	DisplayCharacterOptions()
	: bindPose(false)
	, attachmentAndPoseModifierGizmos(true)
	, showSkeleton(false)
	, showDynamicProxies(false)
	, showAuxiliaryProxies(false)
	, showClothProxies(false)
	, showRagdollProxies(false)
	, showJointNames(false)
	, showSkeletonBoundingBox(false)
	, showEdges(false)
	, showPhysicalProxies(false)
	, showDccToolOrigin(false)
	{
	}

	void Serialize(Serialization::IArchive& ar);
};

struct DisplayFollowJointSettings
{
	string jointName;
	bool lockAlign;
	bool usePosition;
	bool useRotation;

	DisplayFollowJointSettings()
	: lockAlign(false)
	, usePosition(true)
	, useRotation(false)
	{
	}
	
	void Serialize(Serialization::IArchive& ar);
};


struct DisplayParameters
{
	DisplayAnimationOptions animation;
	DisplayCharacterOptions character;
	DisplayFollowJointSettings followJoint;
	SViewportSettings viewport;
	
	DisplayParameters()
	{
	}

	void Serialize(Serialization::IArchive& ar);
};

}
