// CryEngine Source File.
// Copyright (C), Crytek, 1999-2014.

#include "Ruler.h"
#include <Cry_Math.h>

#include <QPainter>
#include <QPalette>

namespace DrawingPrimitives
{
	enum 
	{
		RULER_MIN_PIXELS_PER_TICK = 3,
	};

	QColor Interpolate(const QColor& a, const QColor& b, float k)
	{
		float mk = 1.0f - k;
		return QColor(a.red() * mk  + b.red() * k,
			a.green() * mk + b.green() * k,
			a.blue() * mk + b.blue() * k,
			a.alpha() * mk + b.alpha() * k);
	}

	void DrawRuler(QPainter& painter, const QPalette& palette, const SRulerOptions& options, int* pRulerPrecision)
	{
		const int height = options.m_rect.height();
		if (options.m_rect.width() <= 0 || height <= 0)
			return;

		if (options.m_shadowSize > 0)
		{
			QRect shadowRect = QRect(options.m_rect.left(), options.m_rect.height(), options.m_rect.width(), options.m_shadowSize);
			QLinearGradient upperGradient(shadowRect.left(), shadowRect.top(), shadowRect.left(), shadowRect.bottom());
			upperGradient.setColorAt(0.0f, QColor(0, 0, 0, 128));
			upperGradient.setColorAt(1.0f, QColor(0, 0, 0, 0));
			QBrush upperBrush(upperGradient);
			painter.fillRect(shadowRect, upperBrush);
		}

		painter.fillRect(options.m_rect, Interpolate(palette.color(QPalette::Button), palette.color(QPalette::Midlight), 0.25f));
		
		const float pixelsPerUnit = options.m_visibleRange.Length() > 0.0f ? (float)options.m_rect.width() / options.m_visibleRange.Length() : 1.0f;

		float startTime = options.m_rulerRange.start;
		float endTime = options.m_rulerRange.end;
		float totalDuration = endTime - startTime;

		float ticksMinPower = log10f(RULER_MIN_PIXELS_PER_TICK);
		float ticksPowerDelta = ticksMinPower - log10f(pixelsPerUnit);

		float scaleStep = powf(10.0f, ceil(ticksPowerDelta));
		float scaleStepPixels = scaleStep * pixelsPerUnit;
		int numMarkers = int(totalDuration / scaleStep) + 1;

		float startTimeRound = int(startTime / scaleStep) * scaleStep;
		int startOffsetMod = int(startTime / scaleStep) % 10;
		int scaleOffsetPixels = (startTime - startTimeRound) * pixelsPerUnit;

		QColor midDark = Interpolate(palette.color(QPalette::Dark), palette.color(QPalette::Button), 0.5f);
		painter.setPen(QPen(midDark));

		QFont font;
		font.setPixelSize(10);
		painter.setFont(font);

		const int digitsAfterPoint = max(-int(ceil(ticksPowerDelta))-1, 0);
		if (pRulerPrecision)
		{
			*pRulerPrecision = digitsAfterPoint;
		}
		
		char format[16] = "";
		sprintf_s(format, "%%.%df", digitsAfterPoint);

		const int startX = options.m_rect.left() + (float)(options.m_rulerRange.start - options.m_visibleRange.start) * pixelsPerUnit;
		const int endX = startX + (numMarkers - 1) * scaleStepPixels - scaleOffsetPixels;

		QString str;
		for (int i = 0; i < numMarkers; ++i)
		{
			const int x = startX + i * scaleStepPixels - scaleOffsetPixels;
			const bool tenth = (startOffsetMod + i) % 10 != 0;
			const float value = startTimeRound + i * scaleStep;

			if (tenth)
			{
				painter.drawLine(QPoint(x, height - options.m_markHeight / 2), QPoint(x, height - 1));
			}
			else
			{
				painter.drawLine(QPoint(x, height - options.m_markHeight), QPoint(x, height - 1));
				painter.setPen(palette.color(QPalette::Disabled, QPalette::Text));
				str.sprintf(format, value);
				painter.drawText(QPoint(x + 1, height - options.m_markHeight + 1), str);
				painter.setPen(midDark);
			}
		}

		painter.setPen(QPen(palette.color(QPalette::Dark)));
		painter.drawLine(QPoint(startX, 0), QPoint(startX, height));		
		painter.drawLine(QPoint(endX, 0), QPoint(endX, height));
	}
}
