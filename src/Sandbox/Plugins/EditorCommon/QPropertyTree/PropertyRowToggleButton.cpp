#include <QIcon>

#include "Serialization/ClassFactory.h"
#include "PropertyDrawContext.h"
#include "PropertyRowImpl.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include "Serialization.h"
#include "Color.h"	
#include "Unicode.h"
#include "Serialization/Decorators/ToggleButton.h"
using Serialization::ToggleButton;
using Serialization::RadioButton;

class PropertyRowToggleButton : public PropertyRow{
public:

	bool isLeaf() const{ return true; }
	bool isStatic() const{ return false; }
	bool isSelectable() const{ return false; }

	bool onActivate(const PropertyActivationEvent& e)
	{
		return false;
	}
	void setValueAndContext(const Serialization::SStruct& ser, Serialization::IArchive& ar) override { }
	bool assignTo(const Serialization::SStruct& ser) const override { return false; }
	wstring valueAsWString() const override{ return L""; }
	WidgetPlacement widgetPlacement() const override{ return WIDGET_INSTEAD_OF_TEXT; }
	void serializeValue(Serialization::IArchive& ar) {}
	int widgetSizeMin(const QPropertyTree* tree) const override{ return 36; }

	void redraw(const PropertyDrawContext& context) override
	{
		QRect rect = context.widgetRect;

		wstring text = toWideChar(labelUndecorated());
		int buttonFlags = BUTTON_CENTER;
		if (context.pressed)
			buttonFlags |= BUTTON_PRESSED;
		if (selected())
			buttonFlags |= BUTTON_FOCUSED;
		if (userReadOnly())
			buttonFlags |= BUTTON_DISABLED;
		context.drawButton(rect, text.c_str(), buttonFlags, &context.tree->font());
	}
protected:
	bool m_value;
};

class PropertyRowRadioButton : public PropertyRow{
public:

	bool isLeaf() const override{ return true; }
	bool isStatic() const override{ return false; }
	bool isSelectable() const override{ return false; }

	bool onActivate(const PropertyActivationEvent& e) override
	{
		if (!m_justSet)
		{
			e.tree->model()->rowAboutToBeChanged(this);
			m_justSet = true;
			e.tree->model()->rowChanged(this);
		}
		return true;
	}
	void setValueAndContext(const Serialization::SStruct& ser, Serialization::IArchive& ar) override
	{
		RadioButton* value = (RadioButton*)(ser.pointer());
		m_value = value->buttonValue;
		m_toggled = m_value == *value->value;
		m_justSet = false;
	}
	bool assignTo(const Serialization::SStruct& ser) const override
	{
		if (m_justSet)
			*((RadioButton*)ser.pointer())->value = m_value;
		return true;
	}
	wstring valueAsWString() const override{ return L""; }
	WidgetPlacement widgetPlacement() const override{ return WIDGET_INSTEAD_OF_TEXT; }
	void serializeValue(Serialization::IArchive& ar) override
	{
		bool oldToggled = m_toggled;
		ar(m_toggled, "toggled");
		if (m_toggled && !oldToggled)
			m_justSet = true;
		ar(m_value, "value");
	}
	int widgetSizeMin(const QPropertyTree* tree) const override{ return 40; }

	void redraw(const PropertyDrawContext& context)
	{
		QRect rect = context.widgetRect;
		bool pressed = context.pressed || m_toggled || m_justSet;

		wstring text = toWideChar(labelUndecorated());
		int buttonFlags = BUTTON_CENTER;
		if (pressed)
			buttonFlags |= BUTTON_PRESSED;
		if (selected())
			buttonFlags |= BUTTON_FOCUSED;
		if (userReadOnly())
			buttonFlags |= BUTTON_DISABLED;
		context.drawButton(rect, text.c_str(), buttonFlags, &context.tree->font());
	}
protected:
	bool m_toggled;
	bool m_justSet;
	int m_value;
};

REGISTER_PROPERTY_ROW(ToggleButton, PropertyRowToggleButton); 
REGISTER_PROPERTY_ROW(RadioButton, PropertyRowRadioButton); 
