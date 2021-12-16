////////////////////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
// ----------------------------------------------------------------------------------------
//  File name:   FilterMenuButton.h
//  Description: Menu for explorer type based filtering
//
//////////////////////////////////////////////////////////////////////////////////////////// 

#ifndef _FILTER_MENU_BUTTON_H_
#define _FILTER_MENU_BUTTON_H_

#pragma once

#include <QPushButton>

class QAction;
class AppDocument;

class FilterMenuButton : public QPushButton
{
	Q_OBJECT

private:
	struct Action
	{
		struct Compare
		{
			bool operator() (const Action& action1, const Action& action2) const
			{
				return action1.name < action2.name;
			}
		};

		Action( const char* _name, QAction* _pAction )
			: name(_name)
			, pAction(_pAction)
		{
		}

		string   name;
		QAction* pAction;
	};
	typedef std::vector<Action> MenuActions;

public:
	FilterMenuButton( const AppDocument& appDocument );

signals:
	void SignalSelectedFilterChanged( const QString& selectionName );

protected slots:
	void OnSignalMenuSelectionChanged( bool enabled );

private:

	void InitMenu( const AppDocument& appDocument );
	void UpdateMenu();

	bool IsFilterTypeSelected( const int selection ) const;

	MenuActions  m_menuActions;

	int          m_selectedIndex;
	QString      m_selectedTypeName;
};

#endif // _FILTER_MENU_BUTTON_H_