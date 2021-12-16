// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#pragma once

#include "Range.h"

#include <QRect>

class QPainter;
class QPalette;

namespace DrawingPrimitives
{
	struct SRulerOptions
	{		
		QRect m_rect;		
		Range m_visibleRange;
		Range m_rulerRange;
		int m_markHeight;	
		int m_shadowSize;
	};

	void DrawRuler(QPainter& painter, const QPalette& palette, const SRulerOptions& options, int* pRulerPrecision);
}