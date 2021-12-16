#pragma once

#include <QWidget>
#include "Serialization/Strings.h"

class QLabel;
class QBoxLayout;
class QSlider;
class QDoubleSpinBox;
class QBoxLayout;
class QViewport;

struct SRenderContext;
namespace Serialization
{
	class IArchive;
}

namespace CharacterTool
{
using Serialization::string;

class CharacterDocument;

class BlendSpacePreview : public QWidget
{
	Q_OBJECT
public:
	BlendSpacePreview(QWidget* parent, CharacterDocument* document);

	void IdleUpdate();
	void Serialize(Serialization::IArchive& ar);
	QViewport* GetViewport() const { return m_viewport; }
protected slots:

	void OnRender(const SRenderContext& context);
	void OnResetView();
private:

	QBoxLayout* m_layout;
	CharacterDocument* m_document;
	QAction* m_actionShowGrid;
	QSlider* m_characterScaleSlider;
	QViewport* m_viewport;
};

}
