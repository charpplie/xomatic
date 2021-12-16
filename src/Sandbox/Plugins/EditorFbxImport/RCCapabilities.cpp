// Copyright (c) 1999-2014 Crytek.

#include "StdAfx.h"
#include "RCCapabilities.h"

// Check if import of this type is supported by RC
// This controls the behavior of the dialog
bool IsImportSupported(ENodeClass type)
{
	switch (type)
	{
		// Note: Add more types as FBX support in RC improves
		// This is currently just placeholder "guess" of what we will have soon(TM)
		// TODO: Update this to match actual RC
	case eNC_Geometry:
		return true;
	case eNC_Skeleton:
		return false;
	case eNC_Animation:
		return false;
	case eNC_Material:
		return false;
	default:
		return false;
	}
}

// The default extension the RC will use
// This is used for naming the name.ext.xml files
QString GetDefaultExtension(EImportAssetType type)
{
	switch (type)
	{
	case eIAT_Animation:
		return QStringLiteral("caf");
	case eIAT_Material:
		return QStringLiteral("mtl");
	case eIAT_Skeleton:
		return QStringLiteral("chr");
	case eIAT_SkinnedMesh:
		return QStringLiteral("skin");
	case eIAT_StaticMesh:
		return QStringLiteral("cgf");
	default:
		assert(false);
		return QStringLiteral("unknown");
	}
}

// The default folder the RC expects certain assets during builds
// This is used for suggesting default import locations
QString GetDefaultSubfolder(EImportAssetType type)
{
	switch (type)
	{
	case eIAT_Animation:
		return QStringLiteral("Animation");
	case eIAT_Material:
		return QStringLiteral("Materials");
	case eIAT_Skeleton:
	case eIAT_StaticMesh:
	case eIAT_SkinnedMesh:
	default:
		return QStringLiteral("Objects");
	}
}

// Whether or not selection of dummy nodes is possible for the specified type class
bool CanSelectDummyNode(ENodeClass nodeClass)
{
	// Only for geometry
	return nodeClass == eNC_Geometry;
}

// RC escape sequences
#define RC_ESCAPE_CHAR ^
#define RC_NAME_SEPARATOR /

// Preprocessor helpers
#define STRINGIZE__(x) #x
#define STRINGIZE(x) STRINGIZE__(x)
#define WIDESTRING__(x) L##x
#define WIDESTRING(x) WIDESTRING__(x)

// QWIDE_CHAR(/) -> QChar(L"/"[0])
#define QWIDE_CHAR__(x) QChar(WIDESTRING(STRINGIZE(x))[0])
#define QWIDE_CHAR(x) QWIDE_CHAR__(x)

// RC_ESCAPED(/) -> QStringLiteral("^/")
#define RC_ESCAPED__(e, x) QStringLiteral(STRINGIZE(e##x))
#define RC_ESCAPED(x) RC_ESCAPED__(RC_ESCAPE_CHAR, x)

// str.RC_ESCAPE(/) -> str.replace(QChar(L"/"[0]), QStringLiteral("^/"))
#define RC_ESCAPE(x) replace(QWIDE_CHAR(x), RC_ESCAPED(x))

// Separator character for nodes
QChar GetNodeNameSeparator()
{
	return QWIDE_CHAR(RC_NAME_SEPARATOR);
}

// Escape node name for RC
void EscapeNodeName(QString &str)
{
	str.RC_ESCAPE(RC_ESCAPE_CHAR);
	str.RC_ESCAPE(RC_NAME_SEPARATOR);
}
