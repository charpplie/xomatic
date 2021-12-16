// Copyright (c) 1999-2014 Crytek.

#include "StdAfx.h"
#include "ImportXml.h"

// XML tag and attribute names
// Note: These names need to be XML escaped and Unicode normalized (form NFC), see also http://unicode.org/reports/tr15/#Norm_Forms
#define ROOT_TAG "rc"
#define ITEM_TAG "item"
#define ASSET_TYPE_ATTRIBUTE "assettype"
#define FBX_TYPE_ATTRIBUTE "filetype"
#define FBX_FILE_ATTRIBUTE "filename"
#define FBX_OBJECT_ATTRIBUTE "node"
#define FBX_ATTRIBUTE_ATTRIBUTE "attribute"
#define THIS_PLUGIN_TYPE "fbx"
#define SCALE_ATTRIBUTE "scale"
#define ORIGIN_ATTRIBUTE "origin"
#define SOURCE_UNIT_ATTRIBUTE "sourceunitsize"

// Helper macro
#define WIDESTRING__(x) L##x
#define WIDESTRING(x) WIDESTRING__(x)

// The normalization form to apply
static const QString::NormalizationForm unicodeNormalization = QString::NormalizationForm_C;

// Get import type string
static QString GetImportTypeName(EImportAssetType type)
{
	// Note: These names need to be XML escaped and Unicode normalized
	switch (type)
	{
	case eIAT_Animation:
		return QStringLiteral("animation");
	case eIAT_Material:
		return QStringLiteral("material");
	case eIAT_Skeleton:
		return QStringLiteral("skeleton");
	case eIAT_SkinnedMesh:
		return QStringLiteral("skinned_mesh");
	case eIAT_StaticMesh:
		return QStringLiteral("static_mesh");
	default:
		assert(false);
		return QStringLiteral("unknown");
	}
}

// Escape XML string
static void EscapeXml(QString &str)
{
	str.replace(QChar(L'&'), QStringLiteral("&amp;"));
	str.replace(QChar(L'"'), QStringLiteral("&quot;"));
	str.replace(QChar(L'\''), QStringLiteral("&apos;"));
	str.replace(QChar(L'<'), QStringLiteral("&lt;"));
	str.replace(QChar(L'>'), QStringLiteral("&gt;"));
}

// Append already escaped attribute to the string
static void PushEscapedAttribute(QString &xml, const QString &attributeName, const QString &attributeValue)
{
	xml += QStringLiteral(" ") + attributeName + QStringLiteral("=\"") + attributeValue + QStringLiteral("\"");
}

// Append option tokens to an already opened tag
static void PushOptionTokens(QString &xml, const QList<SImportOption> &options)
{
	for(QList<SImportOption>::const_iterator i = options.begin(); i != options.end(); ++i)
	{
		// Normalize string
		const SImportOption &option = *i;
		QString name = option.optionName.normalized(unicodeNormalization);
		QString value = option.optionValue.normalized(unicodeNormalization);

		// Skip empty options
		if (name.isEmpty()) continue;
		
		// Skip names that cannot be valid attribute names because they are not ASCII or XML-safe
		EscapeXml(name);
		if (name != option.optionName) continue;
		
		// Add XML option
		EscapeXml(value);
		PushEscapedAttribute(xml, name, value);
	}
}

// Convert to XML
// Since the format is fairly simple and the Qt XML libraries are not available in this branch, just use string concatenation
QString CreateImportXml(const SImportRequest &request)
{
	QString pathToFbxEscaped = request.pathToFbx.normalized(unicodeNormalization);
	EscapeXml(pathToFbxEscaped);
	QString typeNameEscaped = GetImportTypeName(request.type);

	// Generate header with global options
	QString xml = QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<" WIDESTRING(ROOT_TAG));
	PushEscapedAttribute(xml, QStringLiteral(ASSET_TYPE_ATTRIBUTE), typeNameEscaped);
	PushEscapedAttribute(xml, QStringLiteral(FBX_TYPE_ATTRIBUTE), QStringLiteral(THIS_PLUGIN_TYPE));
	PushEscapedAttribute(xml, QStringLiteral(FBX_FILE_ATTRIBUTE), pathToFbxEscaped);
	PushOptionTokens(xml, request.globalOptions);
	xml += QStringLiteral(">\n");

	// Generate item nodes
	for (QList<SImportItem>::const_iterator it = request.items.begin(); it != request.items.end(); ++it)
	{
		QString nodePathEscaped = it->pathInsideFbxToNode.normalized(unicodeNormalization);
		if (nodePathEscaped.isEmpty()) continue;
		xml += QStringLiteral("\t<" WIDESTRING(ITEM_TAG));
		EscapeXml(nodePathEscaped);
		PushEscapedAttribute(xml, QStringLiteral(FBX_OBJECT_ATTRIBUTE), nodePathEscaped);
		PushOptionTokens(xml, it->itemOptions);
		xml += QStringLiteral("/>\n");
	}
	return xml + QStringLiteral("</" WIDESTRING(ROOT_TAG) WIDESTRING(">\n"));
}

void SImportItem::AddAttributeOption(const QString &attributePath)
{
	SImportOption option;
	option.optionName = QStringLiteral(FBX_ATTRIBUTE_ATTRIBUTE);
	option.optionValue = attributePath;
	itemOptions.push_back(option);
}

void SImportRequest::AddScaleOption(double scale)
{
	SImportOption option;
	option.optionName = QStringLiteral(SCALE_ATTRIBUTE);
	option.optionValue = QString::number(scale, 'g', 17); // Precision of 17 should ensure there is no loss of precision possible
	globalOptions.push_back(option);
}

void SImportRequest::AddOriginOption(const QString &originNodePath)
{
	SImportOption option;
	option.optionName = QStringLiteral(ORIGIN_ATTRIBUTE);
	option.optionValue = originNodePath;
	globalOptions.push_back(option);
}

void SImportRequest::AddSourceUnitOption(const QString &sourceUnit)
{
	SImportOption option;
	option.optionName = QStringLiteral(SOURCE_UNIT_ATTRIBUTE);
	option.optionValue = sourceUnit;
	globalOptions.push_back(option);
}