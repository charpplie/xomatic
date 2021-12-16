////////////////////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
// ----------------------------------------------------------------------------------------
//  File name:   InputDialog
//  Description: Different dialogs to get some user input
//
//////////////////////////////////////////////////////////////////////////////////////////// 

#ifndef _INPUT_DIALOG_H_
#define _INPUT_DIALOG_H_

#pragma once

#include <QDialog>
#include <QString>

#include "Database.h"

class QLineEdit;
class QLabel;
class QPushButton;

class AddEntryNodeDialog : public QDialog
{
	Q_OBJECT

public:
	AddEntryNodeDialog(const DatabaseEntryNodePtr pParentEntryNode);
	
	void Initialize();
	QString GetProvidedName() const;

	virtual void OnInitialized();

public slots:
	virtual void OnSignalTextEdited(const QString& text);

protected:

	QLabel*    m_pNameLabel;
	QLineEdit* m_pNameText;
	
	QPushButton* m_pOkButton;
	QPushButton* m_pCancelButton;

	const DatabaseEntryNodePtr m_pParentEntryNode;
};

class AddEntryNodeLeafDialog : public AddEntryNodeDialog
{
public:
	AddEntryNodeLeafDialog(const DatabaseEntryNodePtr pParentEntryNode);

protected:
	virtual void OnInitialized() OVERRIDE;
	virtual void OnSignalTextEdited(const QString& text) OVERRIDE;
};

class CloneEntryNodeLeafDialog : public AddEntryNodeLeafDialog
{
public:
	CloneEntryNodeLeafDialog(const DatabaseEntryNodePtr pParentEntryNode);

protected:
	virtual void OnInitialized() OVERRIDE;
};

//////////////////////////////////////////////////////////////////////////

class ConfirmationDialog : public QDialog
{
	Q_OBJECT

public:
	enum Options
	{
		OPTIONS_YES_OR_NO = 0,
		OPTIONS_OK
	};

	ConfirmationDialog( const char* dialogTitle, const char* dialogMessage, const Options options = OPTIONS_YES_OR_NO );

protected:

	QLabel*    m_pNameLabel;

	QPushButton* m_pOkButton;
	QPushButton* m_pCancelButton;
};



#endif // _INPUT_DIALOG_H_

