// Copyright (c) 1999-2014 Crytek.

#pragma once
#include "OptionDialog.h"
#include "SceneGraph.h"
#include "ui_StaticMeshDialog.h"

// Initialization context for the dialog
struct SStaticMeshContext
{
	// The valid origins that can be selected
	QList<CSceneGraph::SOriginNode> origins;

	// The number of centimeters per logical unit
	double dUnitInCm;
};

// Known unit
struct KnownUnit
{
	double dUnitInCm;
	QString rcName;
	QString uiName;
};

// Dialog for static mesh specific import options
class CStaticMeshDialog : public QDialog, public IOptionDialog<SStaticMeshContext>, Ui_CStaticMeshDialog
{
public:
	// Constructor
	CStaticMeshDialog(QWidget *pParent);

private:
	// Populate the dialog
	void Populate(SStaticMeshContext &context);

	// Apply context for the dialog matching the current request
	void SetContext(SImportRequest *pRequest, SStaticMeshContext *pContext) override
	{
		m_pRequest = pRequest;
		Populate(*pContext);
	}

	// Get the target file of the import
	QString GetTargetFile() const override
	{
		return m_pCGFText->text();
	}

	// Event handlers
	void OnCGFBrowse();
	void OnScaleChanged(int iVal);
	void OnScaleSpin(double dVal);
	void OnImport();

	// Async completion handler
	struct SContextSelectFile {};
	void OnCGFSelected(const QString &targetFile, SContextSelectFile &context);

	// Finish the dialog if it's closed
	void closeEvent(QCloseEvent *pEvent)
	{
		Finish(false);
	}
	
	// Because we have two controls modifying the current scale that sync to each other
	// We need to only handle changed events once (from the user) and not from subsequent sync
	bool m_bHandleScaleChanged;

	// For the default suggested file name we need to warn the file exists before import
	// For files selected through the file dialog, there's no need (the dialog will prompt for that)
	bool m_bWarnedFileExists;

	// The context of the import
	SImportRequest *m_pRequest;
	
	// The origins for this file
	QList<CSceneGraph::SOriginNode> m_origins;

	// The unit used in the file
	KnownUnit m_fileUnit;
};
