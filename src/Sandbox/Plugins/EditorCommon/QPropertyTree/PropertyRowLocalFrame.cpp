#include "PropertyRowLocalFrame.h"
#include <QIcon>
#include <QAction>
#include <QMenu>
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include <Serialization/Decorators/IGizmoSink.h>
#include <Serialization/Decorators/LocalFrame.h>
#include "Serialization/ClassFactory.h"
#include "PropertyDrawContext.h"
#include "Serialization.h"
#include <Serialization/Decorators/LocalFrameImpl.h>

using Serialization::LocalPosition;

void LocalFrameMenuHandler::onMenuReset()
{
	self->reset(tree);
}

PropertyRowLocalFrameBase::PropertyRowLocalFrameBase()
: m_sink(0)
, m_gizmoIndex(-1)
, m_handle(0)
, m_reset(false)
{
}

PropertyRowLocalFrameBase::~PropertyRowLocalFrameBase()
{
	m_sink = 0;
}

bool PropertyRowLocalFrameBase::onActivate(const PropertyActivationEvent& e)
{
	if (e.reason == e.REASON_RELEASE)
		return false;
	return false;
}

string PropertyRowLocalFrameBase::valueAsString() const
{
	return string();
}

bool PropertyRowLocalFrameBase::onContextMenu(QMenu &menu, QPropertyTree* tree)
{
	Serialization::SharedPtr<PropertyRow> selfPointer(this);

	LocalFrameMenuHandler* handler = new LocalFrameMenuHandler(tree, this);
	
	QAction* action = menu.addAction("Reset", handler, SLOT(onMenuReset()));

	tree->addMenuHandler(handler);	
	return true;
}

void PropertyRowLocalFrameBase::reset(QPropertyTree* tree)
{
	tree->model()->rowAboutToBeChanged(this);
	m_reset = true;
	tree->model()->rowChanged(this);
}

void PropertyRowLocalFrameBase::redraw(const PropertyDrawContext& context)
{
	static QIcon gizmo("Editor/Icons/animation/gizmo_location.png");
	gizmo.paint(context.painter, context.widgetRect.adjusted(1, 1, 1, 1), Qt::AlignRight);
}

static void ResetTransform(Vec3* v) { *v = ZERO; }
static void ResetTransform(Quat* t) { *t = IDENTITY; }
static void ResetTransform(QuatT* t) { *t = IDENTITY; }

template<class TLocal>
class PropertyRowLocalFrameImpl : public PropertyRowLocalFrameBase
{
public:
	void setValueAndContext(const Serialization::SStruct& ser, Serialization::IArchive& ar) override
	{
		serializer_ = ser;

		TLocal* value = (TLocal*)ser.pointer();
		m_handle = value->handle;
		m_reset = false;

		if (label() && label()[0])
		{
			m_sink = ar.FindContext<Serialization::IGizmoSink>();
			if (m_sink)
				m_gizmoIndex = m_sink->Write(*value, m_gizmoFlags, m_handle);
		}
	}

	void closeNonLeaf(const Serialization::SStruct& ser, Serialization::IArchive& ar) override
	{
		if (label() && label()[0] && ar.IsInput())
		{
			TLocal& value = *((TLocal*)ser.pointer());
			if (m_sink) 
			{
				if (m_sink->CurrentGizmoIndex() == m_gizmoIndex)
					m_sink->Read(&value, &m_gizmoFlags, m_handle);
				else
					m_sink->SkipRead();
			}

		}
	}
	
	bool assignTo(const Serialization::SStruct& ser) const
	{
		if (m_reset)
		{
			TLocal& value = *((TLocal*)ser.pointer());
			ResetTransform(value.value);
		}		
		return false; 
	}

};

typedef PropertyRowLocalFrameImpl<Serialization::LocalPosition> PropertyRowLocalPosition;
typedef PropertyRowLocalFrameImpl<Serialization::LocalOrientation> PropertyRowLocalOrientation;
typedef PropertyRowLocalFrameImpl<Serialization::LocalFrame> PropertyRowLocalFrame;

REGISTER_PROPERTY_ROW(Serialization::LocalPosition, PropertyRowLocalPosition); 
REGISTER_PROPERTY_ROW(Serialization::LocalOrientation, PropertyRowLocalOrientation); 
REGISTER_PROPERTY_ROW(Serialization::LocalFrame, PropertyRowLocalFrame); 

#include <QPropertyTree/moc_PropertyRowLocalFrame.cpp>
