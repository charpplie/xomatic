// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#include "TimeSlider.h"

#include <QPainter>
#include <QPalette>

namespace DrawingPrimitives
{
	void DrawTimeSlider(QPainter& painter, const QPalette& palette, const STimeSliderOptions& options)
	{
		char format[16] = "";
		sprintf_s(format, "%%.%df", options.m_precision + 1);

		QString text;
		text.sprintf(format, options.m_time);

		QFontMetrics fm(painter.font());
		const int textWidth = fm.width(text) + fm.height();
		const int markerHeight = fm.height();

		const int thumbX = options.m_position;
		const bool fits = thumbX + textWidth < options.m_rect.right();
		
		const QRect timeRect(fits ? thumbX : thumbX - textWidth, 3, textWidth, fm.height());
		painter.fillRect(timeRect.adjusted(fits ? 0 : -1, 0, fits ? 1 : 0, 0), options.m_bHasFocus ? palette.highlight() : palette.shadow());
		painter.setPen(palette.color(QPalette::HighlightedText));
		painter.drawText(timeRect.adjusted(fits ? 0 : markerHeight * 0.2f, -1, fits ? -markerHeight * 0.2f : 0, 0), text, QTextOption(fits ? Qt::AlignRight : Qt::AlignLeft));

		painter.setPen(palette.color(QPalette::Text));
		painter.drawLine(QPointF(thumbX, 0), QPointF(thumbX, options.m_rect.height()));
		QPointF points[3] =
		{
			QPointF(thumbX,  markerHeight),
			QPointF(thumbX - markerHeight * 0.66f, 0),
			QPointF(thumbX + markerHeight * 0.66f, 0)
		};

		painter.setBrush(palette.base());
		painter.setPen(palette.color(QPalette::Text));
		painter.drawPolygon(points, 3);
	}
}