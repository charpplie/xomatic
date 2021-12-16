// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#ifndef TreePanel_h
#define TreePanel_h

#pragma once

#include <QDockWidget>

#include "BehaviorTreeDocument.h"

#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QDialog>

class QPropertyTree;

class TreePanel : public QDockWidget
{
	Q_OBJECT

public:
	TreePanel();

	void Reset();

	void OnWindowEvent_NewFile();
	void OnWindowEvent_OpenFile();
	void OnWindowEvent_Save();
	void OnWindowEvent_SaveToFile();

	bool HandleCloseEvent();

public slots:
		void OnPropertyTreeDataChanged();

private:
	bool CheckForUnsavedDataAndSave();

	QPropertyTree* m_propertyTree;

	BehaviorTreeDocument m_behaviorTreeDocument;
	bool propertiesAttachedToDocument;
};

struct UnsavedChangesDialog : public QDialog
{
	enum Result
	{
		Yes,
		No,
		Cancel
	};

	Result result;

	UnsavedChangesDialog()
		: result( Cancel )
	{
		setWindowTitle( "Save before closing" );

		QLabel* label = new QLabel( "There are unsaved changes. Do you want to save them?" );

		QPushButton* yesButton = new QPushButton("Yes");
		QPushButton* noButton = new QPushButton("No");
		QPushButton* cancelButton = new QPushButton("Cancel");

		QGridLayout *gridLayout = new QGridLayout();
		gridLayout->addWidget( label, 0, 0 );

		QHBoxLayout *buttonLayout = new QHBoxLayout();
		buttonLayout->addWidget( yesButton );
		buttonLayout->addWidget( noButton );
		buttonLayout->addWidget( cancelButton );

		gridLayout->addLayout( buttonLayout, 1, 0, Qt::AlignCenter );

		QVBoxLayout *mainLayout = new QVBoxLayout();
		mainLayout->addLayout( gridLayout );
		setLayout( mainLayout );

		connect(yesButton, SIGNAL(clicked()), this, SLOT(clickedYes()));
		connect(noButton, SIGNAL(clicked()), this, SLOT(clickedNo()));
		connect(cancelButton, SIGNAL(clicked()), this, SLOT(clickedCancel()));
	}

	Q_OBJECT

public Q_SLOTS:
	void clickedYes()
	{
		result = Yes;
		accept();
	}

	void clickedNo()
	{
		result = No;
		reject();
	}

	void clickedCancel()
	{
		result = Cancel;
		close();
	}
};

#endif // TreePanel_h