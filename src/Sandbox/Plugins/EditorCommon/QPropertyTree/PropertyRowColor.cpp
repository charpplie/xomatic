#include "PropertyRowColor.h"
#include "Serialization/ClassFactory.h"
#include <Cry_Color.h>
#include <QMenu>
#include <QFileDialog>
#include <QPainter>
#include <QColorDialog>

using Serialization::Vec3AsColor;
typedef SerializableColor_tpl<unsigned char> SerializableColorB;
typedef SerializableColor_tpl<float> SerializableColorF;

QColor ToQColor(const ColorB& v)
{
	return QColor(v.r,v.g,v.b,v.a);
}

void FromQColor(SerializableColorB& vColor, QColor color)
{
	vColor.r = color.red();
	vColor.g = color.green();
	vColor.b = color.blue();
	vColor.a = color.alpha();
}


QColor ToQColor(const Vec3AsColor& v)
{
	return QColor( int(v.v.x * 255.0f), int(v.v.y * 255.0f), int(v.v.z * 255.0f) );
}

void FromQColor(Vec3AsColor& vColor, QColor color)
{
	vColor.v.x = color.red() / 255.0f;
	vColor.v.y = color.green() / 255.0f;
	vColor.v.z = color.blue() / 255.0f;
}

QColor ToQColor(const SerializableColorF& v)
{	
	return QColor::fromRgbF( v.r, v.g, v.b, v.a );
}

void FromQColor(SerializableColorF& vColor, QColor color)
{
	vColor.r = color.redF();
	vColor.g = color.greenF();
	vColor.b = color.blueF();
	vColor.a = color.alphaF();
}


template <class ColorClass>
bool PropertyRowColor<ColorClass>::pickColor(QPropertyTree* tree)
{
	QColor color = QColorDialog::getColor(QColor(color_.red(), color_.green(), color_.blue(), color_.alpha()));	

	if (color.isValid())
	{
		tree->model()->rowAboutToBeChanged(this);
		color_.setRed(color.red());
		color_.setGreen(color.green());
		color_.setBlue(color.blue());
		colorChanged_ = true;
		tree->model()->rowChanged(this);
		return true;
	}

	return false;
}

template <class ColorClass>
bool PropertyRowColor<ColorClass>::onActivate(const PropertyActivationEvent& e)
{
	return pickColor(e.tree);
}

template <class ColorClass>
void PropertyRowColor<ColorClass>::setValueAndContext(const Serialization::SStruct& ser, IArchive& ar) 
{
	color_ = ToQColor(*(ColorClass*)ser.pointer());
	colorChanged_ = false;
}

template <class ColorClass>
bool PropertyRowColor<ColorClass>::assignTo(const Serialization::SStruct& ser) const 
{
	FromQColor(*((ColorClass*)ser.pointer()), color_);
	return true;
}

template<class ColorClass>
string PropertyRowColor<ColorClass>::valueAsString() const
{
	char buf[64];
	sprintf_s(buf, "%d %d %d", (int)color_.red(), (int)color_.green(), (int)color_.blue());
	return string(buf);
}

template <class ColorClass>
bool PropertyRowColor<ColorClass>::onContextMenu(QMenu &menu, QPropertyTree* tree)
{
	Serialization::SharedPtr<PropertyRowColor> selfPointer(this);
	ColorMenuHandler* handler = new ColorMenuHandler(tree,this);
	menu.addAction("Pick Color", handler, SLOT(onMenuPickColor()));
	tree->addMenuHandler(handler);	
	return true;
}

template <class ColorClass>
void PropertyRowColor<ColorClass>::redraw(const PropertyDrawContext& context)
{
	static QImage checkboardPattern;
	if (checkboardPattern.isNull())
	{
		int size = 12;		
		static vector<int> pixels(size * size);
		for (int i = 0; i < pixels.size(); ++i)
			pixels[i] = ((i / size) / (size / 2) + (i % size) / (size / 2)) % 2 ? 0xffffffff : 0x000000ff;
		checkboardPattern = QImage((unsigned char*)pixels.data(), size, size, size * 4, QImage::Format_RGBA8888);
	}

	QRect r = context.widgetRect.adjusted(0, 0, 0, -1);

	context.painter->save();
	context.painter->setPen(QPen(Qt::NoPen));
	context.painter->setRenderHint(QPainter::Antialiasing, true);
	context.painter->setBrush(context.tree->palette().color(QPalette::Dark));
	context.painter->setPen(Qt::NoPen);
	context.painter->drawRoundedRect(r, 2, 2);
	r = r.adjusted(1,1,-1,-1);
	QRect cr = r.adjusted(0, 0, -r.width() / 2, 0);
	context.painter->setBrushOrigin(cr.topRight() + QPoint(1,0));
	context.painter->setBrush(QBrush(checkboardPattern));

	context.painter->setRenderHint(QPainter::Antialiasing, false);	
	context.painter->drawRoundedRect(r, 2, 2);
	
	context.painter->setPen(QPen(Qt::NoPen));
	context.painter->setClipRect(cr);
	context.painter->setBrush(QBrush(color_));
	context.painter->drawRoundedRect(r, 2, 2);

	cr = r.adjusted(r.width() / 2, 0, 0, 0);
	context.painter->setClipRect(cr);
	context.painter->setBrush(QBrush(QColor(color_.red(), color_.green(), color_.blue(), 255)));
	context.painter->drawRoundedRect(r, 2, 2);
	context.painter->restore();
}

template <class ColorClass>
void PropertyRowColor<ColorClass>::closeNonLeaf(const Serialization::SStruct& ser, Serialization::IArchive& ar)
{
	color_ = ToQColor(*(ColorClass*)ser.pointer());
}

ColorMenuHandler::ColorMenuHandler(QPropertyTree* tree, IPropertyRowColor *propertyRowColor)
	: propertyRowColor(propertyRowColor), tree(tree)
{
}

void ColorMenuHandler::onMenuPickColor()
{
	propertyRowColor->pickColor(tree);
}

typedef PropertyRowColor<SerializableColorB> PropertyRowColorB;
typedef PropertyRowColor<Vec3AsColor> PropertyRowVec3AsColor;
typedef PropertyRowColor<SerializableColorF> PropertyRowColorF;
		
REGISTER_PROPERTY_ROW(SerializableColorB, PropertyRowColorB);
REGISTER_PROPERTY_ROW(Vec3AsColor, PropertyRowVec3AsColor);
REGISTER_PROPERTY_ROW(SerializableColorF, PropertyRowColorF);

#include <QPropertyTree/moc_PropertyRowColor.cpp>
