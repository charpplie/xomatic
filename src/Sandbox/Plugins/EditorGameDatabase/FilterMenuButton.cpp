//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.

#include "StdAfx.h"
#include "FilterMenuButton.h"

#include <Descriptor/ObjectTypeInfo.h>
#include <QMenu>

#include "AppDocument.h"


FilterMenuButton::FilterMenuButton( const AppDocument& appDocument )
	: m_selectedIndex(0)
	, m_selectedTypeName("")
{
	InitMenu(appDocument);
}

void FilterMenuButton::OnSignalMenuSelectionChanged( bool enabled )
{
	int newSelection = 0;
	QAction* pSenderAction = qobject_cast<QAction*>(sender());
	if (pSenderAction)
	{
		newSelection = pSenderAction->data().toInt();
	}

	m_selectedIndex = newSelection;

	UpdateMenu();

	SignalSelectedFilterChanged( m_selectedTypeName );
}

void FilterMenuButton::InitMenu( const AppDocument& appDocument )
{
	setMenu(new QMenu(this));

	const size_t typesCount = appDocument.GetDatabase().GetTypesCount();
	m_menuActions.reserve(typesCount  + 1);

	const char* everythingText = "All Types";
	{
		QAction* pAction = new QAction(everythingText, menu());
		pAction->setCheckable(true);
		pAction->setChecked(true);
		pAction->setData(QVariant(0));
		connect(pAction, SIGNAL(triggered(bool)), this, SLOT(OnSignalMenuSelectionChanged(bool)));
		
		m_menuActions.push_back(Action(everythingText, pAction));
	}

	for (size_t i = 0; i < typesCount; ++i)
	{
		const ObjectTypeInfoEntry& typeEntry = appDocument.GetDatabase().GetTypeAt(i);

		const char* actionText = typeEntry.pTypeInfo->name.c_str();
		QAction* pAction = new QAction(actionText, menu());
		pAction->setCheckable(true);
		pAction->setChecked(false);			
		connect(pAction, SIGNAL(triggered(bool)), this, SLOT(OnSignalMenuSelectionChanged(bool)));
		
		m_menuActions.push_back(Action(actionText, pAction));
	}

	CRY_ASSERT(m_menuActions.size() > 0);

	menu()->addAction(m_menuActions[0].pAction);
	menu()->addSeparator();

	// Actions
	if (m_menuActions.size() > 1)
	{
		std::sort(m_menuActions.begin()+1, m_menuActions.end(), Action::Compare());
		
		for (size_t i = 1, actionCount = m_menuActions.size(); i < actionCount; ++i)
		{
			m_menuActions[i].pAction->setData(QVariant(i));
			menu()->addAction(m_menuActions[i].pAction);
		}
	}

	const QString typeSelection = m_menuActions[0].pAction->text();
	setText(typeSelection);

	m_selectedTypeName = "";

	setToolTip(QString("Filter by type"));
}

void FilterMenuButton::UpdateMenu()
{
	for (size_t i = 0, actionCount = m_menuActions.size(); i < actionCount; ++i)
	{
		m_menuActions[i].pAction->setChecked(IsFilterTypeSelected(i));
	}

	setText(m_menuActions[m_selectedIndex].name.c_str());

	m_selectedTypeName = (m_selectedIndex > 0) ? m_menuActions[m_selectedIndex].name.c_str() : "";
}

bool FilterMenuButton::IsFilterTypeSelected( const int selection ) const
{
	return (m_selectedIndex == selection);
}

#include <moc_FilterMenuButton.cpp>
