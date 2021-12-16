#include "../EditorCommon/QPropertyTree/QPropertyTree.h"
#include "Expected.h"
#include "Serialization.h"
#include "DisplayParametersPanel.h"
#include <QBoxLayout>
#include <QViewport.h>
#include <ICryAnimation.h>
#include "CharacterDocument.h"


namespace CharacterTool
{



DisplayParametersPanel::DisplayParametersPanel(QWidget* parent, CharacterDocument* document, Serialization::SContextLink* context)
	: QWidget(parent)
	, m_displayParameters(new DisplayParameters())
	, m_document(document)
{
	EXPECTED(connect(document, SIGNAL(SignalDisplayOptionsChanged(const DisplayParameters&)), this, SLOT(OnDisplayOptionsUpdated())));
	EXPECTED(connect(document, SIGNAL(SignalCharacterLoaded()), this, SLOT(OnDisplayOptionsUpdated())));

	QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
	layout->setMargin(0);
	layout->setSpacing(0);

	m_propertyTree = new QPropertyTree(this);
	m_propertyTree->setSizeHint(QSize(220, 100));
	m_propertyTree->setExpandLevels(0);
	m_propertyTree->setSliderUpdateDelay(5);
	m_propertyTree->setAutoRevert(false);
	m_propertyTree->setArchiveContext(context);
	m_propertyTree->setValueColumnWidth(0.6f);
	m_propertyTree->attach(Serialization::SStruct(*m_displayParameters));
	EXPECTED(connect(m_propertyTree, SIGNAL(signalChanged()), this, SLOT(OnPropertyTreeChanged())));
	EXPECTED(connect(m_propertyTree, SIGNAL(signalContinuousChange()), this, SLOT(OnPropertyTreeChanged())));
	layout->addWidget(m_propertyTree, 1);
}

DisplayParametersPanel::~DisplayParametersPanel()
{
	
}

void DisplayParametersPanel::Serialize(Serialization::IArchive& ar)
{
	if (ar.Filter(SERIALIZE_STATE))
	{
		ar(*m_propertyTree, "propertyTree");
	}
}

void DisplayParametersPanel::OnDisplayOptionsUpdated()
{
	IDefaultSkeleton* skeleton = 0;
	if (m_document)
		*m_displayParameters = m_document->GetDisplayOptions();
	
	m_propertyTree->revertNoninterrupting();
}

void DisplayParametersPanel::OnPropertyTreeChanged()
{
	if (m_document)
		m_document->SetDisplayOptions(*m_displayParameters);
}

}

#include <CharacterTool/moc_DisplayParametersPanel.cpp>
