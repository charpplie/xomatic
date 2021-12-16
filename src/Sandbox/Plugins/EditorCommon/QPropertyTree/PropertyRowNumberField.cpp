/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#include "QPropertyTree.h"
#include "QPropertyTreeStyle.h"
#include "PropertyTreeModel.h"
#include "PropertyRowNumberField.h"
#include "PropertyDrawContext.h"
#include <QStyleOption>
#include <QDesktopWidget>
#include <QApplication>
#include <QPainter>
#include <QBitmap>

#ifdef _WIN32
#include <Windows.h>
#endif

PropertyRowNumberField::PropertyRowNumberField()
: pressed_(false)
{
}

PropertyRowWidget* PropertyRowNumberField::createWidget(QPropertyTree* tree)
{
	return new PropertyRowWidgetNumber(tree->model(), this, tree);
}

QColor interpolateColor(const QColor& a, const QColor& b, float k);

void PropertyRowNumberField::redraw(const PropertyDrawContext& context)
{
	if(multiValue())
		context.drawEntry(L" ... ", false, true, 0);
	else if (userReadOnly())
		context.drawValueText(pulledSelected(), valueAsWString().c_str());
	else 
	{
		QPainter* painter = context.painter;
		const QPropertyTree* tree = context.tree;

		QRect rt = context.widgetRect;
		rt.adjust(0, 0, 0, -1);

		QStyleOptionFrameV2 option;
		option.state = QStyle::State_Sunken;
		option.lineWidth = tree->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &option, 0);
		option.midLineWidth = 0;
		option.features = QStyleOptionFrameV2::None;

		if (context.captured) {
			option.state |= QStyle::State_HasFocus;
			option.state |= QStyle::State_Active;
			option.state |= QStyle::State_MouseOver;
		}
		else if (!userReadOnly()) {
			option.state |= QStyle::State_Enabled;
		}
		option.rect = rt; // option.rect is the rectangle to be drawn on.
		option.palette = tree->palette();
		option.fontMetrics = tree->fontMetrics();
		QRect textRect = tree->style()->subElementRect(QStyle::SE_LineEditContents, &option, 0);
		if (!textRect.isValid()) {
			textRect = rt;
			textRect.adjust(3, 1, -3, -2);
		}
		else {
			textRect.adjust(2, 1, -2, -1);
		}
		// some styles rely on default pens
		painter->setPen(QPen(tree->palette().color(QPalette::WindowText)));
		painter->setBrush(QBrush(tree->palette().color(QPalette::Base)));
		tree->style()->drawPrimitive(QStyle::PE_PanelLineEdit, &option, painter, 0);

		double sliderPos = sliderPosition();
		if (sliderPos != 0.0)
		{
			QRect r = textRect.adjusted(-2, -1, 2, 1);
			QRect sliderOverlayRect(r.left(), r.top(), int(r.width() * sliderPos), r.height());
			QColor sliderOverlayColor = interpolateColor(tree->palette().color(QPalette::Window), tree->palette().color(QPalette::Highlight), tree->treeStyle().sliderSaturation);
			sliderOverlayColor.setAlpha(192);
			painter->setBrush(QBrush(sliderOverlayColor));
			painter->setPen(Qt::NoPen);
			painter->drawRoundedRect(sliderOverlayRect, 1, 1);

			if (pressed_) {
				painter->setPen(QColor(255,255,255));
				painter->setBrush(QBrush(QColor(255,255,255)));
				painter->drawLine(sliderOverlayRect.right(), sliderOverlayRect.top(), sliderOverlayRect.right(), sliderOverlayRect.bottom());
				painter->setRenderHint(QPainter::Antialiasing, true);
				painter->translate(0.5f, 0.5f);
				int r = sliderOverlayRect.right();
				int t = sliderOverlayRect.top();
				int h = sliderOverlayRect.height();
				QPoint points[3] = {
					QPoint(r - 1 - h / 8 - h / 3, t + h / 2),
					QPoint(r - 1 - h / 8, t + h * 1/4),
					QPoint(r - 1 - h / 8, t + h * 3/4)
				};
				QPoint pointsR[3] = {
					QPoint(r + 1 + h / 8 + h / 3, t + h / 2),
					QPoint(r + 1 + h / 8, t + h * 1/4),
					QPoint(r + 1 + h / 8, t + h * 3/4)
				};
				painter->drawPolygon(points, 3);
				painter->drawPolygon(pointsR, 3);
				painter->setRenderHint(QPainter::Antialiasing, false);
				painter->translate(-0.5f, -0.5f);
			}
		}

		painter->setPen(QPen(tree->palette().color(QPalette::WindowText)));
		painter->setBrush(QBrush(tree->palette().color(QPalette::Base)));
		painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, QString(valueAsString().c_str()), 0);


	}
}

QImage getStandardCursorBitmap(QPoint* pos)
{
#ifdef _WIN32
	HCURSOR cursor = LoadCursor(0, IDC_ARROW);
	ICONINFO iconInfo = { 0 };
	if (!GetIconInfo(cursor, &iconInfo))
		return QImage();
	*pos = QPoint(iconInfo.xHotspot, iconInfo.yHotspot);

	BITMAP bitmap = { 0 };
	HBITMAP dbi = (HBITMAP)::CopyImage(iconInfo.hbmColor, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	if (!::GetObjectA(dbi, sizeof(bitmap), &bitmap))
		return QImage();
	vector<uchar> pixels;
	pixels.resize(bitmap.bmWidth * bitmap.bmHeight * 4);
	GetBitmapBits(dbi, pixels.size(), pixels.data());
	QImage result = QImage(pixels.data(), bitmap.bmWidth, bitmap.bmHeight, QImage::Format_ARGB32).copy();
	DeleteObject(dbi);

	return result;
#else
	return QImage();
#endif
}

QCursor createSliderHoverCursor()
{
	QPoint hotSpot(0,0);
	static QImage image = getStandardCursorBitmap(&hotSpot);
	if (image.isNull())
		return QCursor(Qt::SizeHorCursor);

	int w = image.width();
	int h = image.height();
	QImage empty(w*2, h, QImage::Format_ARGB32);
	empty.fill(Qt::transparent);
	QPixmap pixmap = QPixmap::fromImage(empty);
	if (pixmap.isNull())
		return QCursor(Qt::SizeHorCursor);
	QPainter p(&pixmap);
	p.drawImage(image.width() / 2, 0, image);
	p.setRenderHint(QPainter::Antialiasing, true);
	
	QPoint points[3] = {
		QPoint(w / 2 - w * 2 / 8, h / 2),
		QPoint(w / 2 - w / 8, h * 3/8),
		QPoint(w / 2 - w / 8, h * 5/8)
	};
	QPoint pointsR[3] = {
		QPoint(w, h * 3/8),
		QPoint(w, h * 5/8),
		QPoint(w + w / 8, h / 2),
	};
	p.setBrush(QBrush(QColor(255,255,255)));
	p.setPen(QPen(QColor(0,0,0)));
	p.drawPolygon(points, 3);
	p.drawPolygon(pointsR, 3);
	return QCursor(pixmap, image.width() / 2 + hotSpot.x(), hotSpot.y());
}


void PropertyRowNumberField::onMouseDrag(const PropertyDragEvent& e)
{
	QSize screenSize = QApplication::desktop()->screenGeometry(e.tree).size();
	float relativeDelta = float(e.totalDelta.x()) / screenSize.width();
	int fieldRectWidth = widgetRect(e.tree).width();
	if (fieldRectWidth < 16)
		fieldRectWidth = e.tree->treeSize().x() * e.tree->valueColumnWidth();
	float valueFieldFraction = fieldRectWidth < FLT_EPSILON  ? 0 : float(e.totalDelta.x()) / fieldRectWidth;
	incrementLog(relativeDelta, valueFieldFraction);
	setMultiValue(false);
}

bool PropertyRowNumberField::getHoverInfo(PropertyHoverInfo* hit, const QPoint& cursorPos, const QPropertyTree* tree) const
{
	if (pressed_ && !userReadOnly())
		hit->cursor = QCursor(Qt::BlankCursor);
	else if (widgetRect(tree).contains(cursorPos) && !userReadOnly())
		hit->cursor = QCursor(createSliderHoverCursor());
	return true;
}

void PropertyRowNumberField::onMouseStill(const PropertyDragEvent& e)
{
	e.tree->model()->callRowCallback(this);
	e.tree->apply(true);
}

bool PropertyRowNumberField::onMouseDown(QPropertyTree* tree, QPoint point, bool& changed)
{
	changed = false;
	if (widgetRect(tree).contains(point) && !userReadOnly()) {
		startIncrement();
		pressed_ = true;
		return true;
	}
	return false;
}

void PropertyRowNumberField::onMouseUp(QPropertyTree* tree, QPoint point) 
{
	tree->unsetCursor();
	pressed_ = false;

	// endIncrement() can cause PropertyRow to be destroy, 
	// so no "this" members should be accessed after the call.
	endIncrement(tree);
}

bool PropertyRowNumberField::onActivate(const PropertyActivationEvent& e)
{
	if (e.reason == e.REASON_RELEASE || e.reason == e.REASON_DOUBLECLICK)
		return e.tree->spawnWidget(this, false);
	return false;
}

// ---------------------------------------------------------------------------

PropertyRowWidgetNumber::PropertyRowWidgetNumber(PropertyTreeModel* model, PropertyRowNumberField* row, QPropertyTree* tree)
: PropertyRowWidget(row, tree)
, row_(row)
, entry_(new QLineEdit())
, tree_(tree)
{
	//entry_->setAlignment(Qt::AlignCenter);
	entry_->setText(row_->valueAsString().c_str());
	connect(entry_, SIGNAL(editingFinished()), this, SLOT(onEditingFinished()));

	entry_->selectAll();
}


void PropertyRowWidgetNumber::onEditingFinished()
{
	tree_->model()->rowAboutToBeChanged(row());
	string str = entry_->text().toLocal8Bit().data();
	if(row_->setValueFromString(str.c_str()) || row_->multiValue())
		tree_->model()->rowChanged(row());
	else
		tree_->_cancelWidget();
}

void PropertyRowWidgetNumber::commit()
{
	if(entry_)
		onEditingFinished();
}

#include <QPropertyTree/moc_PropertyRowNumberField.cpp>
