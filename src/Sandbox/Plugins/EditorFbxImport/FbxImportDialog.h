// Copyright (c) 1999-2014 Crytek.

#pragma once
#include "ui_FbxImportDialog.h"
#include "SceneGraph.h"
#include "ProgressDialog.h"
#include "ImportXml.h"
#include <QAbstractItemModel>

// Forward declaration
class CSceneGraphModel;

// Supported import types
enum EImportType
{
	eIT_StaticMesh,
	eIT_Skin,
	eIT_Skeleton,
	eIT_Animation,
	eIT_COUNT
};

// The import dialog
class CFbxImportDialog : public QMainWindow, Ui_FbxImportDialog
{
public:
	// Create an import dialog
	CFbxImportDialog(QWidget *pParent = nullptr);

	// The user-selected import type
	EImportType GetImportType();

private:
	// Raised when a scene node is selected
	void SceneNodeSelected(const QModelIndex &index);
	
	// Raised when the currently selected node should be exported
	void ImportClicked();
	void ImportToFile(const QString &targetFile, SImportRequest &context);

	// Raised when the input file has been selected
	void InputFileChanged(const QString &);

	// Raised when the user clicked the browse for FBX file button
	struct SContextSelectFile
	{
	};
	void SelectInputFile(const QString &fbxFile, SContextSelectFile &context);
	void BrowseFileClicked();

	// Qt drag/drop event handling, so the user can drop an FBX on the dialog
	virtual void dragEnterEvent(QDragEnterEvent *pEvent) override;
	virtual void dropEvent(QDropEvent *pEvent) override;

	// Apply view options
	void ApplyViewOptions();

	// Run RC to import from FBX asynchronously
	struct SContextRunRC
	{
		QString xmlFileName;
		QString sourceFile;
		QString targetFile;
		int numObjects;
	};
	bool AsyncRunRC(ITaskContext *pContext, SContextRunRC &job);
	void FinishRunRC(bool bResult, SContextRunRC &job);

	// Load FBX asynchronously
	struct SContextLoadFbx
	{
		QString file;
		CSceneGraph *pModel;
	};
	bool AsyncLoadFbx(ITaskContext *pContext, SContextLoadFbx &job);
	void FinishLoadFbx(bool bResult, SContextLoadFbx &job);

	// The current scene graph model
	QScopedPointer<CSceneGraphModel> m_model;

	// The current loaded path
	QString m_loadedPath;
};