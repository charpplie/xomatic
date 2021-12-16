// Copyright (c) 1999-2014 Crytek.

#pragma once
#include <QString>

// Node type class, for classifying node types
// This is used to pick the icons in the UI for example
enum ENodeClass
{
	// Unknown node class
	eNC_Unknown = 0,

	// Any type of geometry
	eNC_Geometry,

	// Any type of skeleton
	eNC_Skeleton,

	// Any type of animation
	eNC_Animation,

	// Any type of material
	eNC_Material,

	// Other known, but irrelevant types
	eNC_Misc,

	// No class, just nodes without attributes
	eNC_None,

	// The number of node classes
	eNC_COUNT
};

// The types of import potentially supported by the RC
enum EImportAssetType
{
	eIAT_StaticMesh,
	eIAT_SkinnedMesh,
	eIAT_Skeleton,
	eIAT_Animation,
	eIAT_Material,
	eIAT_COUNT
};

// The default extension of the imported asset of the specified type
extern QString GetDefaultExtension(EImportAssetType type);

// The default folder for an imported asset of the specified type
extern QString GetDefaultSubfolder(EImportAssetType type);

// Check if importing a specified node is supported by RC
// Note: The implementation of this function should be updated to match RC, or somehow needs to be be obtained from RC
extern bool IsImportSupported(ENodeClass type);

// Check if dummy nodes can be selected for the specified class of FBX types
extern bool CanSelectDummyNode(ENodeClass nodeClass);

// Escape the name of a node for RC
extern void EscapeNodeName(QString &nodeName);

// The node separator character for RC
extern QChar GetNodeNameSeparator();
