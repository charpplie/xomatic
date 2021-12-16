#include "StdAfx.h"

#include "DisplayParameters.h"
#include "Serialization.h"

namespace CharacterTool
{

SERIALIZATION_ENUM_BEGIN(CharacterMovement, "Character Movement")
SERIALIZATION_ENUM(CHARACTER_MOVEMENT_INPLACE, "inplace", "In place (Only grid moves)")
SERIALIZATION_ENUM(CHARACTER_MOVEMENT_REPEATED, "repeated", "Repeated")
SERIALIZATION_ENUM(CHARACTER_MOVEMENT_CONTINUOUS, "continuous", "Continuous (Animation driven)")
SERIALIZATION_ENUM_END()


SERIALIZATION_ENUM_BEGIN(CompressionPreview, "Compression Preview")
SERIALIZATION_ENUM(COMPRESSION_PREVIEW_COMPRESSED, "compressed", "Preview Compressed Only")
SERIALIZATION_ENUM(COMPRESSION_PREVIEW_BOTH, "both", "Side by Side (Original and Compressed)")
SERIALIZATION_ENUM_END()

void DisplayCharacterOptions::Serialize(Serialization::IArchive& ar)
{
	ar(attachmentAndPoseModifierGizmos, "attachmentAndPoseModifierGizmos", "Attachment/Modifier Gizmos");
	ar(bindPose, "bindPose", "Bind Pose");

	ar(showSkeleton, "showSkeleton", "Skeleton");

	ar(showDynamicProxies, "showDynamicProxies", "Dynamic Proxies");
	ar(showAuxiliaryProxies, "showAuxiliaryProxies", "Auxiliary Proxies");
	ar(showClothProxies, "showClothProxies", "Cloth Proxies");
	ar(showRagdollProxies, "showRagdollProxies", "Ragdoll Proxies");

	ar(showJointNames, "showJointNames", "Joint Names");
	ar(showSkeletonBoundingBox, "showSkeletonBoundingBox", "Bounding Box");
	ar(showEdges, "showEdges", 0);
	ar(showPhysicalProxies, "showPhysicalProxies", "Physical Proxies");
	ar(showDccToolOrigin, "showDccToolOrigin", "DCC Tool Origin");
}

void DisplayFollowJointSettings::Serialize(Serialization::IArchive& ar)
{
	ar(JointName(jointName), "jointName", "Joint");
	ar(lockAlign, "lockAlign", "Align");
	ar(usePosition, "usePosition", "Position");
	ar(useRotation, "useRotation", "Orientation");
}

void DisplayParameters::Serialize(Serialization::IArchive& ar)
{	
	ar(animation, "animation", "+Animation");

	if (ar.OpenBlock("character", "+Character"))
	{
		character.Serialize(ar);
		viewport.debug.Serialize(ar);
		ar.CloseBlock();
	}

	if (ar.OpenBlock("camera", "Camera"))
	{
		viewport.camera.Serialize(ar);

		ar(followJoint, "followJoint", "+Follow Joint");
		ar.CloseBlock();
	}

	ar(viewport.grid, "grid", "Grid");
	ar(viewport.lighting, "ligthing", "Ligthing");
	ar(viewport.background, "background", "Background");
}

}
