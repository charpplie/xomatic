#pragma once

#include <memory>
#include <QWidget>
#include "DisplayParameters.h"
#include "../EditorCommon/QPropertyTree/ContextList.h"

class QPropertyTree;

namespace CharacterTool
{
class CharacterDocument;

class DisplayParametersPanel : public QWidget
{
	Q_OBJECT
public:
	DisplayParametersPanel(QWidget* parent, CharacterDocument* document, Serialization::SContextLink* context);
	~DisplayParametersPanel();
	QSize sizeHint() const override { return QSize(240, 100); }
	void Serialize(Serialization::IArchive& ar);
	
public slots:
	void OnDisplayOptionsUpdated();
	void OnPropertyTreeChanged();

private:
	QPropertyTree* m_propertyTree;
	std::unique_ptr<DisplayParameters> m_displayParameters;
	CharacterDocument* m_document;
};

}