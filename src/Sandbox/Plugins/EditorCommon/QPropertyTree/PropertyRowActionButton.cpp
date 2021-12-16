#include <QIcon>

#include "Serialization/ClassFactory.h"
#include "PropertyDrawContext.h"
#include "PropertyRowImpl.h"
#include "QPropertyTree.h"
#include "PropertyTreeModel.h"
#include "Serialization.h"
#include "Color.h"	
#include "Unicode.h"
#include "Serialization/Decorators/ActionButton.h"
using Serialization::ActionButton;

class PropertyRowActionButton : public PropertyRow
{
public:
	PropertyRowActionButton() : underMouse_(), pressed_(), minimalWidth_() {}
	bool isLeaf() const override{ return true; }
	bool isStatic() const override{ return false; }
	bool isSelectable() const override{ return true; }

	bool onActivate(const PropertyActivationEvent& e) override
	{
		if (e.reason == PropertyActivationEvent::REASON_KEYBOARD)
		{
			if (callback_)
				callback_();
		}
		return true;
	}

	bool onMouseDown(QPropertyTree* tree, QPoint point, bool& changed) override
	{
		if(widgetRect(tree).contains(point)){
			underMouse_ = true;
			pressed_ = true;
			tree->update();
			return true;
		}
		return false;
	}
	
	void onMouseDrag(const PropertyDragEvent& e) override
	{
		printf("onMouseDrag %d %d\n", e.pos.x(), e.pos.y());
		bool underMouse = widgetRect(e.tree).contains(e.pos);
		if(underMouse != underMouse_){
			underMouse_ = underMouse;
			e.tree->update();
		}
	}

	void onMouseUp(QPropertyTree* tree, QPoint point) override
	{
		if(widgetRect(tree).contains(point)){
			pressed_ = false;
			if (callback_)
				callback_();
			tree->update();
		}
	}
	void setValueAndContext(const Serialization::SStruct& ser, Serialization::IArchive& ar) override
	{
		ActionButton* value = (ActionButton*)(ser.pointer());
		callback_ = value->callback;
		icon_ = value->icon.empty() ? QIcon() : QIcon(QString::fromLocal8Bit(value->icon.c_str()));
	}
	bool assignTo(const Serialization::SStruct& ser) const override { return true; }
	wstring valueAsWString() const override{ return L""; }
	WidgetPlacement widgetPlacement() const override{ return WIDGET_INSTEAD_OF_TEXT; }
	void serializeValue(Serialization::IArchive& ar) override { }

	int widgetSizeMin(const QPropertyTree* tree) const override
	{ 
		if (minimalWidth_ == 0) {
			QFontMetrics fm(tree->font());
			minimalWidth_ = (int)fm.width(QString::fromLocal8Bit(labelUndecorated())) + 6 + (icon_.isNull() ? 0 : 18);
		}
		return minimalWidth_;
	}

	void redraw(const PropertyDrawContext& context)
	{
		QRect rect = context.widgetRect.adjusted(-1,-1,1,1);
		bool pressed = pressed_ && underMouse_;

		wstring text = toWideChar(labelUndecorated());
		if (icon_.isNull()) {
			int buttonFlags = BUTTON_CENTER;
			if (pressed)
				buttonFlags |= BUTTON_PRESSED;
			if (selected())
				buttonFlags |= BUTTON_FOCUSED;
			if (userReadOnly())
				buttonFlags |= BUTTON_DISABLED;
			context.drawButton(rect, text.c_str(), buttonFlags, &context.tree->font());
		}
		else {
			context.drawButtonWithIcon(icon_, rect, text.c_str(), selected(), pressed, selected(), !userReadOnly(), true, &context.tree->font());
		}
	}
	bool isFullRow(const QPropertyTree* tree) const override
	{
		if (PropertyRow::isFullRow(tree))
			return true;
		return !userFixedWidget();
	}
protected:
	mutable int minimalWidth_;
	bool underMouse_;
	bool pressed_;
	QIcon icon_;
	std::function<void()> callback_;
};

REGISTER_PROPERTY_ROW(ActionButton, PropertyRowActionButton); 
