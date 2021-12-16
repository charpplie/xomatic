// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#pragma once

#include "Range.h"

#include <QRect>

class QPainter;
class QPalette;

namespace DrawingPrimitives
{
	struct STimeSliderOptions
	{
		QRect m_rect;
		int m_precision;
		int m_position;
		float m_time;		
		bool m_bHasFocus;
	};

	void DrawTimeSlider(QPainter& painter, const QPalette& palette, const STimeSliderOptions& options);
}