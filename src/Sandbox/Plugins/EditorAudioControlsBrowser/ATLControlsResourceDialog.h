// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#pragma once

#include <QDialog>
#include "QTreeWidgetFilter.h"
#include "AudioControlFilters.h"

class QAudioControlTreeWidget;

namespace AudioControls
{
	class ATLControlsDialog : public QDialog
	{
		Q_OBJECT
	public:
		ATLControlsDialog(QWidget* parent, EACBControlType type);

	private slots:
		void ControlChanged();
		void SetTextFilter(QString filter);

	public:
		void SetScope(string sScope);
		const char* ChooseItem(const char* currentValue);
		QSize sizeHint() const override;

	private:
		string m_sControlName;
		QAudioControlTreeWidget* m_pControlTree;
		QTreeWidgetFilter m_filter;
		SNameFilter m_nameFilter;
		SScopeFilter m_scopeFilter;
		STypeFilter m_typeFilter;
	};
}