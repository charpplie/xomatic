//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "InputDialog.h"

#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRegExp>
#include <QRegExpValidator>

AddEntryNodeDialog::AddEntryNodeDialog(const DatabaseEntryNodePtr pParentEntryNode)
	: QDialog(NULL)
	, m_pParentEntryNode(pParentEntryNode)
{

}

void AddEntryNodeDialog::Initialize()
{
	m_pNameLabel = new QLabel("Name");
	m_pOkButton = new QPushButton("OK");
	m_pOkButton->setEnabled(false);
	m_pCancelButton = new QPushButton("Cancel");

	m_pNameText = new QLineEdit();

	QRegExpValidator* pValidator = new QRegExpValidator(QRegExp("([a-zA-Z]|[0-9]|[_])+"));
	m_pNameText->setValidator(pValidator);

	QGridLayout *pGridLayout = new QGridLayout();
	pGridLayout->setColumnStretch(1, 2);
	pGridLayout->addWidget(m_pNameLabel, 0, 0);
	pGridLayout->addWidget(m_pNameText, 0, 1);

	QHBoxLayout *pButtonLayout = new QHBoxLayout();
	pButtonLayout->addWidget(m_pOkButton);
	pButtonLayout->addWidget(m_pCancelButton);

	pGridLayout->addLayout(pButtonLayout, 2, 1, Qt::AlignRight);

	QVBoxLayout *pMainLayout = new QVBoxLayout();
	pMainLayout->addLayout(pGridLayout);
	setLayout(pMainLayout);

	connect(m_pOkButton, SIGNAL(clicked()), this, SLOT(accept()));
	connect(m_pCancelButton, SIGNAL(clicked()), this, SLOT(reject()));

	connect(m_pNameText, SIGNAL(textChanged(QString)), this, SLOT(OnSignalTextEdited(QString)));

	OnInitialized();
}

QString AddEntryNodeDialog::GetProvidedName() const
{
	return m_pNameText->text().toLower();
}

void AddEntryNodeDialog::OnInitialized()
{

}

void AddEntryNodeDialog::OnSignalTextEdited(const QString& text)
{

}

//////////////////////////////////////////////////////////////////////////

AddEntryNodeLeafDialog::AddEntryNodeLeafDialog(const DatabaseEntryNodePtr pParentEntryNode)
	: AddEntryNodeDialog(pParentEntryNode)
{

}

void AddEntryNodeLeafDialog::OnInitialized()
{
	setWindowTitle(tr("Add a new record"));

	m_pNameText->setText(QString("new_record"));
}

void AddEntryNodeLeafDialog::OnSignalTextEdited(const QString& text)
{
	if (m_pParentEntryNode == NULL)
		return;

	bool nameAlreadyInUse = text.isEmpty();
	for(size_t i = 0, childCount = m_pParentEntryNode->children.size(); i < childCount; ++i)
	{
		DatabaseEntryNode* pEntryNode = m_pParentEntryNode->children[i];
		nameAlreadyInUse |= ((pEntryNode->type == DatabaseEntryNode::Leaf) && (text.compare(QString(pEntryNode->name), Qt::CaseInsensitive) == 0));
	}

	m_pOkButton->setEnabled(!nameAlreadyInUse);
}

//////////////////////////////////////////////////////////////////////////

CloneEntryNodeLeafDialog::CloneEntryNodeLeafDialog(const DatabaseEntryNodePtr pParentEntryNode)
	: AddEntryNodeLeafDialog(pParentEntryNode)
{

}

void CloneEntryNodeLeafDialog::OnInitialized()
{
	setWindowTitle(tr("Cloned record"));

	m_pNameText->setText(QString("cloned_record"));
}

//////////////////////////////////////////////////////////////////////////

ConfirmationDialog::ConfirmationDialog( const char* dialogTitle, const char* dialogMessage, const Options options /*= OPTIONS_YES_OR_NO*/ )
	: QDialog(NULL)
{
	m_pNameLabel = new QLabel(dialogMessage);
	
	if ( options == OPTIONS_YES_OR_NO )
	{
		m_pOkButton = new QPushButton("Yes");
		m_pCancelButton = new QPushButton("No");
	}
	else if ( options == OPTIONS_OK )
	{
		m_pOkButton = new QPushButton("OK");
		m_pCancelButton = NULL;
	}

	QGridLayout *pGridLayout = new QGridLayout();
	pGridLayout->setColumnStretch(1, 2);
	pGridLayout->addWidget(m_pNameLabel, 0, 0);

	QHBoxLayout *pButtonLayout = new QHBoxLayout();
	pButtonLayout->addWidget(m_pOkButton);
	pButtonLayout->addWidget(m_pCancelButton);

	pGridLayout->addLayout(pButtonLayout, 2, 1, Qt::AlignRight);

	QVBoxLayout *pMainLayout = new QVBoxLayout();
	pMainLayout->addLayout(pGridLayout);
	setLayout(pMainLayout);

	if (m_pOkButton)
	{
		connect(m_pOkButton, SIGNAL(clicked()), this, SLOT(accept()));
	}
	if (m_pCancelButton)
	{
		connect(m_pCancelButton, SIGNAL(clicked()), this, SLOT(reject()));
	}

	setWindowTitle(dialogTitle);
}

#include <moc_InputDialog.cpp>

