#pragma once

#include "EditorCommonAPI.h"
#include <QObject>

namespace Serialization
{
		class StringList;
}

// Private class, should not be used directly
class QWidget;
class QPropertyDialog;
class CBatchFileDialog : public QObject
{
	Q_OBJECT
public slots:
	void OnSelectAll();
	void OnSelectNone();
	void OnLoadList();
public:
	QPropertyDialog* m_dialog;
	struct SContent;
	SContent* m_content;
};
// ^^^

struct SBatchFileSettings
{
	const char* extension;
	const char* folder;
	const char* title;
	const char* descriptionText;
	const char* listLabel;
	const char* stateFilename;
	bool useCryPak;

	SBatchFileSettings()
	: useCryPak(true)
	, folder("")
	, descriptionText("Batch Selected Files")
	, listLabel("Files")
	, stateFilename("batchFileDialog.state")
	, title("Batch Files")
	, extension("*")
	{
	}
};

bool EDITOR_COMMON_API ShowBatchFileDialog(Serialization::StringList* filenames, const SBatchFileSettings& settings, QWidget* parent);
