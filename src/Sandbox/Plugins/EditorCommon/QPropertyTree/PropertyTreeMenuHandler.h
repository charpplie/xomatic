#pragma once

#include "PropertyRow.h"

struct PropertyTreeMenuHandler : PropertyRowMenuHandler
{
	Q_OBJECT
public:
	PropertyRow* row;
	QPropertyTree* tree;

	string filterName;
	string filterValue;
	string filterType;

public slots:
	void onMenuFilter();
	void onMenuFilterByName();
	void onMenuFilterByValue();
	void onMenuFilterByType();

	void onMenuUndo();

	void onMenuCopy();
	void onMenuPaste();
};
