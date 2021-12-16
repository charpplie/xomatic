// Copyright (c) 1999-2014 Crytek.

#pragma once
#include "RCCapabilities.h"
#include <QList>

// Describes an import option for RC
struct SImportOption
{
	// The name of the option
	QString optionName;

	// The value of the option
	QString optionValue;
};

// Describes an item to be imported by RC
struct SImportItem
{
	// The path inside the FBX file that identifies the node
	// The path must be RC escaped
	QString pathInsideFbxToNode;

	// Options that apply to a single item
	QList<SImportOption> itemOptions;

	// Add attribute path
	// The path needs to be RC escaped
	void AddAttributeOption(const QString &attributePath);
};

// Describes an entire request for RC
struct SImportRequest
{
	// The type of import requested
	EImportAssetType type;

	// The FBX file to be imported
	QString pathToFbx;

	// Options that apply to the entire request
	QList<SImportOption> globalOptions;

	// The items to be imported
	QList<SImportItem> items;

	// Add scaling option
	void AddScaleOption(double scale);

	// Add origin options
	// The node name needs to be RC escaped
	void AddOriginOption(const QString &originNodePath);

	// Add source unit
	void AddSourceUnitOption(const QString &sourceUnit);
};

// Creates XML content that describes an import request for RC
// This XML can be passed to RC to perform the processing
extern QString CreateImportXml(const SImportRequest &request);